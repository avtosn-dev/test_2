/**
  * @file    scheduler.h
  * @brief   Simple cooperative scheduler for STM32 HAL projects
  *
  * WHAT IS THIS?
  * -------------
  * A cooperative scheduler calls your functions on a timetable. It is NOT an RTOS:
  * there are no priorities, no preemption, and no stacks per task. You must call
  * scheduler_run() often from the main while(1) loop.
  *
  * TIME BASE (HAL)
  * ---------------
  * Timing uses HAL_GetTick() — milliseconds since HAL_Init(), updated in SysTick.
  *
  * HOW TO ADD A TASK (minimal example)
  * ---------------------------------
  *   void MyTask_Run(void) { ... keep it short, no long HAL_Delay() ... }
  *
  *   scheduler_init();
  *   scheduler_add_task(MyTask_Run, 200U, TASK_MODE_PERIODIC_T, "MyTask");
  *
  *   while (1) {
  *       scheduler_run();
  *   }
  *
  * See the long comment block at the top of scheduler.c for a step-by-step guide.
  */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "main.h"
#include "scheduler_config.h"
#include <stdint.h>
#include <stdbool.h>

/* Invalid slot index returned by internal lookups */
#define SCHEDULER_INVALID_SLOT    SCHEDULER_MAX_TASKS

typedef enum {
    TASK_MODE_PERIODIC_T = 0,   /**< Call every period_ms until removed */
    TASK_MODE_ONE_SHOT_T = 1    /**< Call once, then slot is freed */
} TASK_MODE_T;

typedef enum {
    TASK_STATE_DISABLED_T = 0,
    TASK_STATE_ENABLED_T  = 1
} TASK_STATE_T;

typedef enum {
    SCH_STATUS_OK_T               = 0,
    SCH_STATUS_ERR_FULL_T         = 1,
    SCH_STATUS_ERR_NULL_T         = 2,
    SCH_STATUS_ERR_PERIOD_T       = 3,
    SCH_STATUS_ERR_NOT_FOUND_T    = 4,
    SCH_STATUS_ERR_INVALID_ID_T   = 5,
    SCH_STATUS_ERR_DUPLICATE_T    = 6,
    SCH_STATUS_ERR_INVALID_MODE_T = 7
} SCH_STATUS_T;

/** Pointer to a task function: no parameters, no return value. */
typedef void (*TASK_FUNC_T)(void);

/**
  * Task control block — one row in the scheduler table.
  * You normally do not fill this yourself; scheduler_add_task() does it.
  */
typedef struct {
    bool             in_use;          /**< 1 = this table row is occupied */
    TASK_FUNC_T      func;            /**< Function to call */
    uint32_t         period_ms;       /**< Interval between runs (ms) */
    uint32_t         last_run_tick;   /**< HAL_GetTick() value at last run */
    TASK_MODE_T      mode;
    TASK_STATE_T     state;
    bool             remove_pending;  /**< Cleared on next scheduler_run() */
    bool             overrun;         /**< 1 = last interval was too long */
    uint32_t         overrun_count;
    char             name[SCHEDULER_NAME_LEN];
} TASK_CB_T;

void scheduler_init(void);
void scheduler_set_debug_uart(UART_HandleTypeDef *huart);

SCH_STATUS_T scheduler_add_task(TASK_FUNC_T func, uint32_t period_ms,
                                TASK_MODE_T mode, const char *name);

void scheduler_run(void);

SCH_STATUS_T scheduler_enable_task(const char *name);
SCH_STATUS_T scheduler_disable_task(const char *name);
SCH_STATUS_T scheduler_remove_task(const char *name);
SCH_STATUS_T scheduler_set_period(const char *name, uint32_t period_ms);

SCH_STATUS_T scheduler_enable_task_by_id(uint8_t id);
SCH_STATUS_T scheduler_disable_task_by_id(uint8_t id);

uint8_t scheduler_get_task_count(void);
const TASK_CB_T *scheduler_get_task_info(const char *name);
SCH_STATUS_T scheduler_clear_overrun(const char *name);

void scheduler_print_status(void);

#endif /* SCHEDULER_H */
