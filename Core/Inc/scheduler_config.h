/**
  * @file    scheduler_config.h
  * @brief   Cooperative scheduler compile-time configuration parameters
  * @details This file contains all tunable constants for the scheduler.
  *          Modify values here to adjust maximum number of tasks, timing,
  *          overrun detection, and debug features.
  */

#ifndef SCHEDULER_CONFIG_H
#define SCHEDULER_CONFIG_H

/*============================================================================*/
/*                          Task table configuration                          */
/*============================================================================*/

/**
  * @brief Maximum number of tasks that can be registered simultaneously
  * @note  Valid range: 1 .. 16 (limited by scheduler design)
  */
#define SCHEDULER_MAX_TASKS     16U

/**
  * @brief Length of task name string (including null terminator)
  * @note  Longer names will be truncated during registration
  */
#define SCHEDULER_NAME_LEN      16U

/*============================================================================*/
/*                          Timing and overrun detection                      */
/*============================================================================*/

/**
  * @brief Scheduler tick period in milliseconds
  * @note  Must match the period of the timebase (SysTick or TIM2).
  *        HAL_GetTick() provides 1 ms tick – this value must stay 1.
  */
#define SCHEDULER_TICK_MS       1U

/**
  * @brief Overrun detection margin, percent of task period
  * @details A task is considered overrun if its actual execution delay
  *          exceeds (period_ms + period_ms * OVERRUN_PCT / 100).
  * @example With 10%, a 100 ms task overruns if delayed by >110 ms.
  */
#define SCHEDULER_OVERRUN_PCT   10U

/**
  * @brief Burst limit for consecutive task executions after a long delay
  * @details If the scheduler was blocked for a long time, the task may be
  *          executed multiple times in a row to catch up.
  *          This limit prevents excessive back‑to‑back executions.
  *          With BURST_LIMIT = 2, at most 2 consecutive runs are allowed.
  */
#define SCHEDULER_BURST_LIMIT   2U

#endif /* SCHEDULER_CONFIG_H */
