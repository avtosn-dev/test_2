/**
  * @file    scheduler_config.h
  * @brief   Compile-time settings for the cooperative scheduler
  *
  * Change these values to fit your project. All timing assumes HAL_GetTick()
  * advances every 1 ms (default after HAL_Init() on STM32).
  */

#ifndef SCHEDULER_CONFIG_H
#define SCHEDULER_CONFIG_H

/** Maximum tasks registered at the same time (1 .. 16). */
#define SCHEDULER_MAX_TASKS         8U

/** Task name buffer size in bytes (includes the '\0' terminator). */
#define SCHEDULER_NAME_LEN          16U

/**
  * Must match HAL tick period in milliseconds (HAL_GetTick step).
  * Do not change unless you use a custom HAL time base.
  */
#define SCHEDULER_TICK_MS           1U

/**
  * If a task is late by more than (period + period * OVERRUN_PCT / 100),
  * the overrun flag is set (diagnostic only).
  */
#define SCHEDULER_OVERRUN_PCT       10U

/**
  * After a long pause, a task could run many times in one scheduler_run() pass.
  * If delay >= period * BURST_LIMIT, last_tick is reset to "now" (drop backlog).
  */
#define SCHEDULER_BURST_LIMIT       2U

/** Timeout for HAL_UART_Transmit() inside scheduler_print_status(). */
#define SCHEDULER_UART_TIMEOUT_MS   100U

#endif /* SCHEDULER_CONFIG_H */
