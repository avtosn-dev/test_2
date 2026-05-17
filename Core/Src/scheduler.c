/**
  * @file    scheduler.c
  * @brief   Cooperative scheduler — beginner-oriented implementation
  *
  * =============================================================================
  * HOW THE SCHEDULER WORKS (read this first)
  * =============================================================================
  *
  * 1) You register tasks with scheduler_add_task().
  *    Each task is just a normal C function void MyTask(void).
  *
  * 2) In main(), you call scheduler_run() inside while(1).
  *
  * 3) Every time scheduler_run() is called, it:
  *      a) Reads the current time: now = HAL_GetTick()  (HAL, 1 ms tick)
  *      b) For each registered task, checks: has period_ms passed since last run?
  *      c) If yes, calls the task function once
  *      d) Removes tasks marked for deletion (one-shot finished, or user remove)
  *
  * 4) Because nothing runs in the background, a task that blocks (HAL_Delay, long
  *    UART transmit, etc.) delays ALL other tasks. Write short tasks or use a
  *    state machine with HAL_GetTick() instead of blocking delays.
  *
  * =============================================================================
  * HOW TO ADD YOUR OWN TASK (copy this pattern)
  * =============================================================================
  *
  *   // file: my_blink.c
  *   #include "main.h"
  *
  *   static uint32_t s_last_toggle = 0U;
  *
  *   void MyBlink_Run(void)
  *   {
  *       uint32_t now = HAL_GetTick();
  *       if ((now - s_last_toggle) >= 500U) {
  *           s_last_toggle = now;
  *           HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
  *       }
  *   }
  *
  *   // file: main.c (after MX_GPIO_Init / peripherals)
  *   scheduler_init();
  *   if (scheduler_add_task(MyBlink_Run, 50U, TASK_MODE_PERIODIC_T, "Blink")
  *       != SCH_STATUS_OK_T) {
  *       Error_Handler();
  *   }
  *   while (1) {
  *       scheduler_run();
  *   }
  *
  * =============================================================================
  * KNOWN LIMITATIONS (previous version issues — fixed or documented here)
  * =============================================================================
  *
  * FIXED: Task counter (s_task_count) was decremented twice in some paths.
  * FIXED: Burst limit multiplication could overflow for huge period_ms.
  * FIXED: Name lookup now uses strcmp() on null-terminated names (exact match).
  *
  * STILL TRUE (by design):
  *   - Cooperative only: no preemption, no priorities.
  *   - While a task runs, scheduler_run() does not visit other tasks.
  *   - Duplicate names are rejected until the old slot is cleaned up.
  *
  * =============================================================================
  */

#include "scheduler.h"
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------------------------- */
/* Compile-time checks                                                         */
/* -------------------------------------------------------------------------- */

_Static_assert(SCHEDULER_MAX_TASKS > 0U, "SCHEDULER_MAX_TASKS must be > 0");
_Static_assert(SCHEDULER_MAX_TASKS <= 16U, "SCHEDULER_MAX_TASKS max is 16");
_Static_assert(SCHEDULER_NAME_LEN >= 2U, "SCHEDULER_NAME_LEN must be >= 2");
_Static_assert(SCHEDULER_TICK_MS == 1U, "SCHEDULER_TICK_MS must be 1 for HAL_GetTick");
_Static_assert(SCHEDULER_BURST_LIMIT > 0U, "SCHEDULER_BURST_LIMIT must be > 0");

/* -------------------------------------------------------------------------- */
/* Private data                                                                */
/* -------------------------------------------------------------------------- */

/** Fixed-size table: one entry per possible task */
static TASK_CB_T s_tasks[SCHEDULER_MAX_TASKS];

/** Number of rows with in_use == true (maintained on add / cleanup only) */
static uint8_t s_active_count = 0U;

/** Optional UART for scheduler_print_status(); NULL = print disabled */
static UART_HandleTypeDef *s_debug_uart = NULL;

/* -------------------------------------------------------------------------- */
/* Private helpers                                                             */
/* -------------------------------------------------------------------------- */

/**
  * @brief Send a null-terminated string on the debug UART (blocking HAL call).
  */
static void uart_print(const char *text)
{
    if ((s_debug_uart == NULL) || (text == NULL)) {
        return;
    }
    size_t len = strlen(text);
    if (len == 0U) {
        return;
    }
    (void)HAL_UART_Transmit(s_debug_uart, (const uint8_t *)text, (uint16_t)len,
                            SCHEDULER_UART_TIMEOUT_MS);
}

/**
  * @brief Find table index by exact task name (active rows only).
  * @return Slot index or SCHEDULER_INVALID_SLOT.
  */
static uint8_t find_slot_by_name(const char *name)
{
    if (name == NULL) {
        return SCHEDULER_INVALID_SLOT;
    }

    for (uint8_t i = 0U; i < SCHEDULER_MAX_TASKS; i++) {
        if (!s_tasks[i].in_use) {
            continue;
        }
        if (strcmp(s_tasks[i].name, name) == 0) {
            return i;
        }
    }
    return SCHEDULER_INVALID_SLOT;
}

/**
  * @brief First free row in the table.
  */
static uint8_t find_free_slot(void)
{
    for (uint8_t i = 0U; i < SCHEDULER_MAX_TASKS; i++) {
        if (!s_tasks[i].in_use) {
            return i;
        }
    }
    return SCHEDULER_INVALID_SLOT;
}

/**
  * @brief Clear one row and update the active task counter.
  */
static void free_slot(uint8_t idx)
{
    if (idx >= SCHEDULER_MAX_TASKS) {
        return;
    }
    if (s_tasks[idx].in_use && (s_active_count > 0U)) {
        s_active_count--;
    }
    memset(&s_tasks[idx], 0, sizeof(TASK_CB_T));
}

/**
  * @brief Schedule row for deletion on the next cleanup pass.
  */
static void request_remove(uint8_t idx)
{
    if (idx >= SCHEDULER_MAX_TASKS) {
        return;
    }
    s_tasks[idx].remove_pending = true;
    s_tasks[idx].state = TASK_STATE_DISABLED_T;
}

/**
  * @brief Overrun threshold in ms: period + margin percent (overflow-safe).
  */
static uint32_t overrun_limit_ms(uint32_t period_ms)
{
    if (period_ms > (UINT32_MAX / (100U + SCHEDULER_OVERRUN_PCT))) {
        return UINT32_MAX;
    }
    return period_ms + (period_ms * SCHEDULER_OVERRUN_PCT / 100U);
}

/**
  * @brief True if delay is so large we should not catch up multiple runs.
  */
static bool burst_catchup_exceeded(uint32_t delay_ms, uint32_t period_ms)
{
    if (period_ms == 0U) {
        return false;
    }
    if (period_ms > (UINT32_MAX / SCHEDULER_BURST_LIMIT)) {
        return (delay_ms >= UINT32_MAX);
    }
    return (delay_ms >= (period_ms * SCHEDULER_BURST_LIMIT));
}

/**
  * @brief Update last_run_tick after a run (catch-up or single period step).
  */
static void advance_last_run_tick(uint8_t idx, uint32_t now, uint32_t delay_ms)
{
    if (burst_catchup_exceeded(delay_ms, s_tasks[idx].period_ms)) {
        s_tasks[idx].last_run_tick = now;
    } else {
        s_tasks[idx].last_run_tick += s_tasks[idx].period_ms;
    }
}

/**
  * @brief Run one task if it is enabled and its period has elapsed.
  */
static void try_run_task(uint8_t idx, uint32_t now)
{
    TASK_CB_T *t = &s_tasks[idx];

    if (!t->in_use || t->remove_pending) {
        return;
    }
    if (t->state != TASK_STATE_ENABLED_T) {
        return;
    }
    if (t->func == NULL) {
        return;
    }

    uint32_t delay_ms = now - t->last_run_tick;
    if (delay_ms < t->period_ms) {
        return;
    }

    /* Diagnostic: task is late compared to its period */
    if (delay_ms > overrun_limit_ms(t->period_ms)) {
        t->overrun = true;
        if (t->overrun_count < UINT32_MAX) {
            t->overrun_count++;
        }
    } else {
        t->overrun = false;
    }

    advance_last_run_tick(idx, now, delay_ms);

    /* User code runs here — keep it short */
    t->func();

    if (t->mode == TASK_MODE_ONE_SHOT_T) {
        request_remove(idx);
    }
}

/**
  * @brief Second pass: free rows that were marked remove_pending.
  */
static void cleanup_removed_tasks(void)
{
    for (uint8_t i = 0U; i < SCHEDULER_MAX_TASKS; i++) {
        if (s_tasks[i].in_use && s_tasks[i].remove_pending) {
            free_slot(i);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
/* -------------------------------------------------------------------------- */

void scheduler_init(void)
{
    memset(s_tasks, 0, sizeof(s_tasks));
    s_active_count = 0U;
    s_debug_uart = NULL;
}

void scheduler_set_debug_uart(UART_HandleTypeDef *huart)
{
    s_debug_uart = huart;
}

SCH_STATUS_T scheduler_add_task(TASK_FUNC_T func, uint32_t period_ms,
                                TASK_MODE_T mode, const char *name)
{
    if ((func == NULL) || (name == NULL)) {
        return SCH_STATUS_ERR_NULL_T;
    }
    if (period_ms == 0U) {
        return SCH_STATUS_ERR_PERIOD_T;
    }
    if ((mode != TASK_MODE_PERIODIC_T) && (mode != TASK_MODE_ONE_SHOT_T)) {
        return SCH_STATUS_ERR_INVALID_MODE_T;
    }
    if (find_slot_by_name(name) != SCHEDULER_INVALID_SLOT) {
        return SCH_STATUS_ERR_DUPLICATE_T;
    }

    uint8_t slot = find_free_slot();
    if (slot == SCHEDULER_INVALID_SLOT) {
        return SCH_STATUS_ERR_FULL_T;
    }

    TASK_CB_T *t = &s_tasks[slot];
    t->in_use = true;
    t->func = func;
    t->period_ms = period_ms;
    t->last_run_tick = HAL_GetTick();
    t->mode = mode;
    t->state = TASK_STATE_ENABLED_T;
    t->remove_pending = false;
    t->overrun = false;
    t->overrun_count = 0U;

    strncpy(t->name, name, SCHEDULER_NAME_LEN - 1U);
    t->name[SCHEDULER_NAME_LEN - 1U] = '\0';

    s_active_count++;
    return SCH_STATUS_OK_T;
}

void scheduler_run(void)
{
    /*
     * Step 1 — snapshot time once per pass (all tasks use the same "now")
     */
    uint32_t now = HAL_GetTick();

    /*
     * Step 2 — visit every table row; run due tasks
     */
    for (uint8_t i = 0U; i < SCHEDULER_MAX_TASKS; i++) {
        try_run_task(i, now);
    }

    /*
     * Step 3 — delete finished one-shot tasks and user-removed tasks
     */
    cleanup_removed_tasks();
}

SCH_STATUS_T scheduler_enable_task(const char *name)
{
    uint8_t idx = find_slot_by_name(name);
    if (idx == SCHEDULER_INVALID_SLOT) {
        return SCH_STATUS_ERR_NOT_FOUND_T;
    }
    s_tasks[idx].state = TASK_STATE_ENABLED_T;
    s_tasks[idx].last_run_tick = HAL_GetTick();
    return SCH_STATUS_OK_T;
}

SCH_STATUS_T scheduler_disable_task(const char *name)
{
    uint8_t idx = find_slot_by_name(name);
    if (idx == SCHEDULER_INVALID_SLOT) {
        return SCH_STATUS_ERR_NOT_FOUND_T;
    }

    if (s_tasks[idx].mode == TASK_MODE_ONE_SHOT_T) {
        request_remove(idx);
    } else {
        s_tasks[idx].state = TASK_STATE_DISABLED_T;
    }
    return SCH_STATUS_OK_T;
}

SCH_STATUS_T scheduler_remove_task(const char *name)
{
    uint8_t idx = find_slot_by_name(name);
    if (idx == SCHEDULER_INVALID_SLOT) {
        return SCH_STATUS_ERR_NOT_FOUND_T;
    }
    request_remove(idx);
    return SCH_STATUS_OK_T;
}

SCH_STATUS_T scheduler_set_period(const char *name, uint32_t period_ms)
{
    if (period_ms == 0U) {
        return SCH_STATUS_ERR_PERIOD_T;
    }
    uint8_t idx = find_slot_by_name(name);
    if (idx == SCHEDULER_INVALID_SLOT) {
        return SCH_STATUS_ERR_NOT_FOUND_T;
    }
    s_tasks[idx].period_ms = period_ms;
    return SCH_STATUS_OK_T;
}

SCH_STATUS_T scheduler_enable_task_by_id(uint8_t id)
{
    if (id >= SCHEDULER_MAX_TASKS) {
        return SCH_STATUS_ERR_INVALID_ID_T;
    }
    if (!s_tasks[id].in_use || s_tasks[id].remove_pending) {
        return SCH_STATUS_ERR_INVALID_ID_T;
    }
    s_tasks[id].state = TASK_STATE_ENABLED_T;
    s_tasks[id].last_run_tick = HAL_GetTick();
    return SCH_STATUS_OK_T;
}

SCH_STATUS_T scheduler_disable_task_by_id(uint8_t id)
{
    if (id >= SCHEDULER_MAX_TASKS) {
        return SCH_STATUS_ERR_INVALID_ID_T;
    }
    if (!s_tasks[id].in_use || s_tasks[id].remove_pending) {
        return SCH_STATUS_ERR_INVALID_ID_T;
    }

    if (s_tasks[id].mode == TASK_MODE_ONE_SHOT_T) {
        request_remove(id);
    } else {
        s_tasks[id].state = TASK_STATE_DISABLED_T;
    }
    return SCH_STATUS_OK_T;
}

uint8_t scheduler_get_task_count(void)
{
    return s_active_count;
}

const TASK_CB_T *scheduler_get_task_info(const char *name)
{
    uint8_t idx = find_slot_by_name(name);
    if (idx == SCHEDULER_INVALID_SLOT) {
        return NULL;
    }
    return &s_tasks[idx];
}

SCH_STATUS_T scheduler_clear_overrun(const char *name)
{
    uint8_t idx = find_slot_by_name(name);
    if (idx == SCHEDULER_INVALID_SLOT) {
        return SCH_STATUS_ERR_NOT_FOUND_T;
    }
    s_tasks[idx].overrun = false;
    s_tasks[idx].overrun_count = 0U;
    return SCH_STATUS_OK_T;
}

void scheduler_print_status(void)
{
    char line[96];
    int len;

    len = snprintf(line, sizeof(line),
                   "\r\n=== Scheduler: %u / %u tasks ===\r\n",
                   (unsigned)s_active_count, (unsigned)SCHEDULER_MAX_TASKS);
    if ((len > 0) && (len < (int)sizeof(line))) {
        uart_print(line);
    }

    for (uint8_t i = 0U; i < SCHEDULER_MAX_TASKS; i++) {
        const TASK_CB_T *t = &s_tasks[i];

        if (!t->in_use || t->remove_pending) {
            continue;
        }

        len = snprintf(line, sizeof(line),
                       "[%u] %-12s %lu ms  %s  %s  late:%lu\r\n",
                       (unsigned)i,
                       t->name,
                       (unsigned long)t->period_ms,
                       (t->state == TASK_STATE_ENABLED_T) ? "ON" : "OFF",
                       (t->mode == TASK_MODE_PERIODIC_T) ? "PER" : "1SHOT",
                       (unsigned long)t->overrun_count);
        if ((len > 0) && (len < (int)sizeof(line))) {
            uart_print(line);
        }
    }

    uart_print("================================\r\n");
}
