/**
  * @file    scheduler.h
  * @brief   Cooperative scheduler – public API
  * @details Provides task registration, execution control, enable/disable,
  *          removal, period change, overrun monitoring, and debug print.
  *          Tasks are executed periodically based on their period_ms.
  *          All user-defined types follow ALL_CAPS_T naming convention.
  */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "main.h"
#include "scheduler_config.h"
#include <stdint.h>
#include <stddef.h>

/*============================================================================*/
/*                      Public enumerated types (ALL_CAPS_T)                  */
/*============================================================================*/

/**
  * @brief Task execution mode
  */
typedef enum {
    TASK_MODE_PERIODIC_T = 0,  /**< Runs repeatedly every period_ms */
    TASK_MODE_ONE_SHOT_T = 1   /**< Runs once, then auto‑removed */
} TASK_MODE_T;

/**
  * @brief Task internal state (enabled/disabled by user)
  */
typedef enum {
    TASK_STATE_DISABLED_T = 0, /**< Task is not executed */
    TASK_STATE_ENABLED_T  = 1  /**< Task is eligible for execution */
} TASK_STATE_T;

/**
  * @brief Pending removal flags (internal use only)
  */
typedef enum {
    PENDING_REMOVE_NONE_T     = 0, /**< No removal pending */
    PENDING_REMOVE_ONESHOT_T  = 1, /**< One‑shot task has finished */
    PENDING_REMOVE_EXPLICIT_T = 2  /**< User requested removal */
} PENDING_REMOVE_T;

/**
  * @brief Scheduler function return status
  */
typedef enum {
    SCH_STATUS_OK_T               = 0, /**< Operation successful */
    SCH_STATUS_ERR_FULL_T         = 1, /**< Task table is full */
    SCH_STATUS_ERR_NULL_T         = 2, /**< Null function pointer or name */
    SCH_STATUS_ERR_PERIOD_T       = 3, /**< Invalid period (zero) */
    SCH_STATUS_ERR_NOT_FOUND_T    = 4, /**< Task name not found */
    SCH_STATUS_ERR_INVALID_ID_T   = 5, /**< Task ID out of range */
    SCH_STATUS_ERR_DUPLICATE_T    = 6, /**< Task name already exists */
    SCH_STATUS_ERR_INVALID_MODE_T = 7  /**< Invalid task mode */
} SCH_STATUS_T;

/*============================================================================*/
/*                      Public types                                          */
/*============================================================================*/

/**
  * @brief Task function pointer type
  */
typedef void (*TASK_FUNC_T)(void);

/**
  * @brief Task Control Block (exposed for debug and info)
  */
typedef struct {
    TASK_FUNC_T      func;            /**< Task entry point */
    uint32_t         period_ms;       /**< Desired execution period */
    uint32_t         last_tick;       /**< Last execution timestamp */
    TASK_MODE_T      mode;            /**< Periodic or one‑shot */
    TASK_STATE_T     state;           /**< Enabled/disabled */
    PENDING_REMOVE_T pending_remove;  /**< Internal removal flag */
    uint8_t          overrun;         /**< 1 if last run exceeded threshold */
    uint32_t         overrun_count;   /**< Total number of overruns */
    char             name[SCHEDULER_NAME_LEN]; /**< Human‑readable name */
} TASK_CB_T;

/*============================================================================*/
/*                      Public function prototypes                            */
/*============================================================================*/

/**
  * @brief Initialise scheduler internal structures.
  * @note  Must be called before any other scheduler function.
  */
void scheduler_init(void);

/**
  * @brief Set UART handle for debug printouts.
  * @param huart Pointer to UART_HandleTypeDef (can be NULL to disable)
  */
void scheduler_set_debug_uart(UART_HandleTypeDef *huart);

/**
  * @brief Add a task to the scheduler.
  * @param func      Task function pointer (must not be NULL)
  * @param period_ms Execution period in milliseconds (must be >0)
  * @param mode      TASK_MODE_PERIODIC_T or TASK_MODE_ONE_SHOT_T
  * @param name      Human‑readable name (copied internally, must be unique)
  * @return SCH_STATUS_OK_T on success, otherwise an error code
  */
SCH_STATUS_T scheduler_add_task(TASK_FUNC_T func, uint32_t period_ms,
                                TASK_MODE_T mode, const char *name);

/**
  * @brief Main scheduler dispatcher. Call this in an infinite loop.
  * @details Checks all enabled tasks and executes those whose period elapsed.
  *          One‑shot tasks are automatically removed after execution.
  */
void scheduler_run(void);

/**
  * @brief Enable a previously added task by name.
  * @param name Task name (case‑sensitive)
  * @return SCH_STATUS_OK_T if found and enabled
  */
SCH_STATUS_T scheduler_enable_task(const char *name);

/**
  * @brief Disable a task by name (stops execution).
  * @param name Task name
  * @return SCH_STATUS_OK_T on success
  * @note  One‑shot tasks are removed instead of disabled.
  */
SCH_STATUS_T scheduler_disable_task(const char *name);

/**
  * @brief Remove a task completely by name (frees its slot).
  * @param name Task name
  * @return SCH_STATUS_OK_T on success
  */
SCH_STATUS_T scheduler_remove_task(const char *name);

/**
  * @brief Change the period of an existing task.
  * @param name      Task name
  * @param period_ms New period in milliseconds (must be >0)
  * @return SCH_STATUS_OK_T on success
  */
SCH_STATUS_T scheduler_set_period(const char *name, uint32_t period_ms);

/**
  * @brief Enable task by its array index (for debugging).
  * @param id Index in task table (0 .. SCHEDULER_MAX_TASKS-1)
  * @return SCH_STATUS_OK_T if slot valid and task exists
  */
SCH_STATUS_T scheduler_enable_task_by_id(uint8_t id);

/**
  * @brief Disable task by its array index.
  * @param id Index in task table
  * @return SCH_STATUS_OK_T on success
  */
SCH_STATUS_T scheduler_disable_task_by_id(uint8_t id);

/**
  * @brief Get current number of active tasks.
  * @return Number of registered tasks (excluding pending removal)
  */
uint8_t scheduler_get_task_count(void);

/**
  * @brief Get pointer to task control block by name.
  * @param name Task name
  * @return Pointer to TASK_CB_T or NULL if not found
  */
const TASK_CB_T* scheduler_get_task_info(const char *name);

/**
  * @brief Clear overrun flag and counter for a task.
  * @param name Task name
  * @return SCH_STATUS_OK_T on success
  */
SCH_STATUS_T scheduler_clear_overrun(const char *name);

/**
  * @brief Print scheduler status via debug UART (if set).
  * @details Lists all tasks with their parameters and overrun counts.
  * @warning This function uses blocking UART transmit. Use with care.
  */
void scheduler_print_status(void);

#endif /* SCHEDULER_H */
