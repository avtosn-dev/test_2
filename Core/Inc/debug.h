/**
  * @file    debug.h
  * @brief   Debug and system service tasks – public interface
  * @details This file groups tasks related to system monitoring, diagnostics,
  *          and watchdog feeding. These tasks are not part of the main
  *          application logic but aid in debugging and system reliability.
  */

#ifndef DEBUG_H
#define DEBUG_H

/*============================================================================*/
/*                      Debug status printing task                            */
/*============================================================================*/

/**
  * @brief Initialise debug status task.
  * @details UART for debug output should be already initialised.
  */
void debug_status_init(void);

/**
  * @brief Print scheduler status (periodic, e.g., every 10 seconds).
  * @details Calls scheduler_print_status() which outputs task list,
  *          periods, states, and overrun counts via debug UART.
  */
void debug_status_run(void);

/*============================================================================*/
/*                      One‑shot initialisation print task                    */
/*============================================================================*/

/**
  * @brief Initialise one‑shot init print task.
  */
void debug_init_print_init(void);

/**
  * @brief Print scheduler status once at startup.
  * @details Registered as a one‑shot task with 200 ms delay.
  *          Useful to verify task registration before normal operation.
  */
void debug_init_print_run(void);

/*============================================================================*/
/*                      Watchdog feed task                                    */
/*============================================================================*/

/**
  * @brief Initialise watchdog feed task (IWDG or WWDG).
  * @details The watchdog peripheral must be configured in CubeMX.
  */
void debug_watchdog_feed_init(void);

/**
  * @brief Refresh the independent watchdog (periodic, e.g., every 1000 ms).
  * @details Prevents system reset by resetting the watchdog counter.
  *          Only active if watchdog is enabled in CubeMX.
  */
void debug_watchdog_feed_run(void);

#endif /* DEBUG_H */
