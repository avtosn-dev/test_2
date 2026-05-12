/**
  * @file    scheduler.c
  * @brief   Cooperative scheduler implementation
  * @details Implements tick‑based task scheduling with period checking,
  *          overrun detection, one‑shot task auto‑removal, and debug UART.
  *          Tasks are executed in round‑robin order when their period expires.
  *          No blocking calls inside scheduler_run() except user tasks.
  */

#include "scheduler.h"
#include <string.h>
#include <stdio.h>

/*============================================================================*/
/*                        Compile‑time assertions                             */
/*============================================================================*/

_Static_assert(SCHEDULER_MAX_TASKS > 0U, "SCHEDULER_MAX_TASKS must be > 0");
_Static_assert(SCHEDULER_MAX_TASKS <= 16U, "SCHEDULER_MAX_TASKS max is 16");
_Static_assert(SCHEDULER_NAME_LEN > 1U, "SCHEDULER_NAME_LEN must be > 1");
_Static_assert(SCHEDULER_TICK_MS == 1U, "SCHEDULER_TICK_MS must be 1 (HAL_GetTick)");
_Static_assert(SCHEDULER_BURST_LIMIT > 0U, "SCHEDULER_BURST_LIMIT must be > 0");

/*============================================================================*/
/*                        Private variables                                   */
/*============================================================================*/

static TASK_CB_T           s_tasks[SCHEDULER_MAX_TASKS];
static uint8_t             s_task_count = 0U;
static UART_HandleTypeDef *s_debug_uart = NULL;

/*============================================================================*/
/*                        Private function prototypes                         */
/*============================================================================*/

static uint8_t find_task_by_name(const char *name);
static uint8_t find_name_any_state(const char *name);
static uint8_t find_empty_slot(void);
static void    mark_for_removal(uint8_t idx);

/*============================================================================*/
/*                        Private functions                                   */
/*============================================================================*/

/**
  * @brief Find a task that is active (not pending removal)
  * @param name Task name (must not be NULL)
  * @return Task index or SCHEDULER_MAX_TASKS if not found
  */
static uint8_t find_task_by_name(const char *name)
{
    if (name == NULL) return SCHEDULER_MAX_TASKS;

    for (uint8_t i = 0U; i < SCHEDULER_MAX_TASKS; i++)
    {
        if ((s_tasks[i].func != NULL) &&
            (s_tasks[i].pending_remove == PENDING_REMOVE_NONE_T) &&
            (strncmp(s_tasks[i].name, name, SCHEDULER_NAME_LEN) == 0))
        {
            return i;
        }
    }
    return SCHEDULER_MAX_TASKS;
}

/**
  * @brief Find a task by name even if pending removal (used for duplicate check)
  * @param name Task name
  * @return Task index or SCHEDULER_MAX_TASKS
  */
static uint8_t find_name_any_state(const char *name)
{
    if (name == NULL) return SCHEDULER_MAX_TASKS;

    for (uint8_t i = 0U; i < SCHEDULER_MAX_TASKS; i++)
    {
        if ((s_tasks[i].func != NULL) &&
            (strncmp(s_tasks[i].name, name, SCHEDULER_NAME_LEN) == 0))
        {
            return i;
        }
    }
    return SCHEDULER_MAX_TASKS;
}

/**
  * @brief Find first empty slot in task table (func == NULL)
  * @return Slot index or SCHEDULER_MAX_TASKS if table is full
  */
static uint8_t find_empty_slot(void)
{
    for (uint8_t i = 0U; i < SCHEDULER_MAX_TASKS; i++)
    {
        if (s_tasks[i].func == NULL) return i;
    }
    return SCHEDULER_MAX_TASKS;
}

/**
  * @brief Mark a task for removal (cleared in next scheduler_run())
  * @param idx Task index
  */
static void mark_for_removal(uint8_t idx)
{
    if (idx >= SCHEDULER_MAX_TASKS) return;
    s_tasks[idx].state = TASK_STATE_DISABLED_T;
    s_tasks[idx].pending_remove = PENDING_REMOVE_EXPLICIT_T;
    if (s_task_count > 0U) s_task_count--;
}

/*============================================================================*/
/*                        Public functions                                    */
/*============================================================================*/

void scheduler_init(void)
{
    memset(s_tasks, 0, sizeof(s_tasks));
    s_task_count = 0U;
    s_debug_uart = NULL;
}

void scheduler_set_debug_uart(UART_HandleTypeDef *huart)
{
    s_debug_uart = huart;
}

SCH_STATUS_T scheduler_add_task(TASK_FUNC_T func, uint32_t period_ms,
                                TASK_MODE_T mode, const char *name)
{
    /* Parameter validation */
    if (func == NULL)       return SCH_STATUS_ERR_NULL_T;
    if (name == NULL)       return SCH_STATUS_ERR_NULL_T;
    if (period_ms == 0U)    return SCH_STATUS_ERR_PERIOD_T;
    if ((mode != TASK_MODE_PERIODIC_T) && (mode != TASK_MODE_ONE_SHOT_T))
        return SCH_STATUS_ERR_INVALID_MODE_T;

    /* Check for duplicate name */
    if (find_name_any_state(name) != SCHEDULER_MAX_TASKS)
        return SCH_STATUS_ERR_DUPLICATE_T;

    /* Find free slot */
    uint8_t slot = find_empty_slot();
    if (slot == SCHEDULER_MAX_TASKS) return SCH_STATUS_ERR_FULL_T;

    /* Initialize task control block */
    s_tasks[slot].func           = func;
    s_tasks[slot].period_ms      = period_ms;
    s_tasks[slot].last_tick      = HAL_GetTick();
    s_tasks[slot].mode           = mode;
    s_tasks[slot].state          = TASK_STATE_ENABLED_T;
    s_tasks[slot].pending_remove = PENDING_REMOVE_NONE_T;
    s_tasks[slot].overrun        = 0U;
    s_tasks[slot].overrun_count  = 0U;
    strncpy(s_tasks[slot].name, name, SCHEDULER_NAME_LEN - 1U);
    s_tasks[slot].name[SCHEDULER_NAME_LEN - 1U] = '\0';

    s_task_count++;
    return SCH_STATUS_OK_T;
}

void scheduler_run(void)
{
    uint32_t now = HAL_GetTick();

    /* First pass: execute tasks whose period has elapsed */
    for (uint8_t i = 0U; i < SCHEDULER_MAX_TASKS; i++)
    {
        /* Skip invalid, disabled, or pending‑removal tasks */
        if (s_tasks[i].func == NULL) continue;
        if (s_tasks[i].state == TASK_STATE_DISABLED_T) continue;
        if (s_tasks[i].pending_remove != PENDING_REMOVE_NONE_T) continue;

        uint32_t elapsed = now - s_tasks[i].last_tick;

        if (elapsed >= s_tasks[i].period_ms)
        {
            /* Overrun detection with overflow‑safe calculation */
            uint32_t overrun_threshold;
            if (s_tasks[i].period_ms <= (UINT32_MAX / (SCHEDULER_OVERRUN_PCT + 100U)))
            {
                overrun_threshold = s_tasks[i].period_ms +
                    (s_tasks[i].period_ms * SCHEDULER_OVERRUN_PCT / 100U);
            }
            else
            {
                overrun_threshold = UINT32_MAX;
            }

            if (elapsed > overrun_threshold)
            {
                s_tasks[i].overrun = 1U;
                if (s_tasks[i].overrun_count < UINT32_MAX)
                    s_tasks[i].overrun_count++;
            }
            else
            {
                s_tasks[i].overrun = 0U;
            }

            /* Burst limit: if the task has been delayed by more than
               period_ms * BURST_LIMIT, reset last_tick to 'now' to avoid
               excessive consecutive executions. Otherwise, advance by one period. */
            if (elapsed >= (s_tasks[i].period_ms * SCHEDULER_BURST_LIMIT))
            {
                s_tasks[i].last_tick = now;
            }
            else
            {
                s_tasks[i].last_tick += s_tasks[i].period_ms;
            }

            /* Execute the task */
            s_tasks[i].func();

            /* Mark one‑shot tasks for removal after execution */
            if ((s_tasks[i].mode == TASK_MODE_ONE_SHOT_T) &&
                (s_tasks[i].pending_remove == PENDING_REMOVE_NONE_T))
            {
                s_tasks[i].pending_remove = PENDING_REMOVE_ONESHOT_T;
            }
        }
    }

    /* Second pass: clean up tasks marked for removal */
    for (uint8_t i = 0U; i < SCHEDULER_MAX_TASKS; i++)
    {
        if (s_tasks[i].pending_remove != PENDING_REMOVE_NONE_T)
        {
            if (s_tasks[i].pending_remove == PENDING_REMOVE_ONESHOT_T)
            {
                if (s_task_count > 0U) s_task_count--;
            }
            memset(&s_tasks[i], 0, sizeof(TASK_CB_T));
        }
    }
}

SCH_STATUS_T scheduler_enable_task(const char *name)
{
    uint8_t idx = find_task_by_name(name);
    if (idx == SCHEDULER_MAX_TASKS) return SCH_STATUS_ERR_NOT_FOUND_T;
    s_tasks[idx].state = TASK_STATE_ENABLED_T;
    s_tasks[idx].last_tick = HAL_GetTick();  /* Resync to avoid immediate run */
    return SCH_STATUS_OK_T;
}

SCH_STATUS_T scheduler_disable_task(const char *name)
{
    uint8_t idx = find_task_by_name(name);
    if (idx == SCHEDULER_MAX_TASKS) return SCH_STATUS_ERR_NOT_FOUND_T;

    if (s_tasks[idx].mode == TASK_MODE_ONE_SHOT_T)
    {
        mark_for_removal(idx);
        return SCH_STATUS_OK_T;
    }
    s_tasks[idx].state = TASK_STATE_DISABLED_T;
    return SCH_STATUS_OK_T;
}

SCH_STATUS_T scheduler_remove_task(const char *name)
{
    uint8_t idx = find_task_by_name(name);
    if (idx == SCHEDULER_MAX_TASKS) return SCH_STATUS_ERR_NOT_FOUND_T;
    mark_for_removal(idx);
    return SCH_STATUS_OK_T;
}

SCH_STATUS_T scheduler_set_period(const char *name, uint32_t period_ms)
{
    if (period_ms == 0U) return SCH_STATUS_ERR_PERIOD_T;
    uint8_t idx = find_task_by_name(name);
    if (idx == SCHEDULER_MAX_TASKS) return SCH_STATUS_ERR_NOT_FOUND_T;
    s_tasks[idx].period_ms = period_ms;
    return SCH_STATUS_OK_T;
}

SCH_STATUS_T scheduler_enable_task_by_id(uint8_t id)
{
    if (id >= SCHEDULER_MAX_TASKS) return SCH_STATUS_ERR_INVALID_ID_T;
    if (s_tasks[id].func == NULL) return SCH_STATUS_ERR_INVALID_ID_T;
    if (s_tasks[id].pending_remove != PENDING_REMOVE_NONE_T)
        return SCH_STATUS_ERR_INVALID_ID_T;
    s_tasks[id].state = TASK_STATE_ENABLED_T;
    s_tasks[id].last_tick = HAL_GetTick();
    return SCH_STATUS_OK_T;
}

SCH_STATUS_T scheduler_disable_task_by_id(uint8_t id)
{
    if (id >= SCHEDULER_MAX_TASKS) return SCH_STATUS_ERR_INVALID_ID_T;
    if (s_tasks[id].func == NULL) return SCH_STATUS_ERR_INVALID_ID_T;
    if (s_tasks[id].pending_remove != PENDING_REMOVE_NONE_T)
        return SCH_STATUS_ERR_INVALID_ID_T;

    if (s_tasks[id].mode == TASK_MODE_ONE_SHOT_T)
    {
        mark_for_removal(id);
        return SCH_STATUS_OK_T;
    }
    s_tasks[id].state = TASK_STATE_DISABLED_T;
    return SCH_STATUS_OK_T;
}

uint8_t scheduler_get_task_count(void)
{
    return s_task_count;
}

const TASK_CB_T* scheduler_get_task_info(const char *name)
{
    uint8_t idx = find_task_by_name(name);
    if (idx == SCHEDULER_MAX_TASKS) return NULL;
    return &s_tasks[idx];
}

SCH_STATUS_T scheduler_clear_overrun(const char *name)
{
    uint8_t idx = find_task_by_name(name);
    if (idx == SCHEDULER_MAX_TASKS) return SCH_STATUS_ERR_NOT_FOUND_T;
    s_tasks[idx].overrun = 0U;
    s_tasks[idx].overrun_count = 0U;
    return SCH_STATUS_OK_T;
}

void scheduler_print_status(void)
{
    if (s_debug_uart == NULL) return;

    char buffer[96];
    int len;

    len = snprintf(buffer, sizeof(buffer),
                   "\r\n=== SCHEDULER (max %u tasks, active %u) ===\r\n",
                   (unsigned)SCHEDULER_MAX_TASKS, (unsigned)s_task_count);
    if ((len > 0) && (len < (int)sizeof(buffer)))
        HAL_UART_Transmit(s_debug_uart, (uint8_t*)buffer, (uint16_t)len, 100U);

    for (uint8_t i = 0U; i < SCHEDULER_MAX_TASKS; i++)
    {
        if (s_tasks[i].func == NULL) continue;
        if (s_tasks[i].pending_remove != PENDING_REMOVE_NONE_T) continue;

        len = snprintf(buffer, sizeof(buffer),
                       "[%2u] %-15s %5lu ms  %s  %s  overrun:%lu\r\n",
                       (unsigned)i,
                       s_tasks[i].name,
                       (unsigned long)s_tasks[i].period_ms,
                       (s_tasks[i].state == TASK_STATE_ENABLED_T) ? "EN" : "DIS",
                       (s_tasks[i].mode == TASK_MODE_PERIODIC_T) ? "PER" : "ONE",
                       (unsigned long)s_tasks[i].overrun_count);
        if ((len > 0) && (len < (int)sizeof(buffer)))
            HAL_UART_Transmit(s_debug_uart, (uint8_t*)buffer, (uint16_t)len, 100U);
    }

    len = snprintf(buffer, sizeof(buffer), "================================\r\n");
    if ((len > 0) && (len < (int)sizeof(buffer)))
        HAL_UART_Transmit(s_debug_uart, (uint8_t*)buffer, (uint16_t)len, 100U);
}


