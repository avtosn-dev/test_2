/**
  * @file    debug.c
  * @brief   Debug and system service tasks – implementation
  */

#include "debug.h"
#include "scheduler.h"
#include "main.h"

/*============================================================================*/
/*                      Debug status printing task                            */
/*============================================================================*/

void debug_status_init(void)
{
    /* No initialisation required */
}

void debug_status_run(void)
{
    scheduler_print_status();
}

/*============================================================================*/
/*                      One‑shot initialisation print task                    */
/*============================================================================*/

void debug_init_print_init(void)
{
    /* Nothing to initialise */
}

void debug_init_print_run(void)
{
    scheduler_print_status();
}

/*============================================================================*/
/*                      Watchdog feed task                                    */
/*============================================================================*/

/* Подключаем сторожевой таймер только если он активирован в CubeMX */
#ifdef HAL_IWDG_MODULE_ENABLED
extern IWDG_HandleTypeDef hiwdg;
#endif

void debug_watchdog_feed_init(void)
{
    /* The watchdog is configured by CubeMX – no additional init needed */
}

void debug_watchdog_feed_run(void)
{
#ifdef HAL_IWDG_MODULE_ENABLED
    HAL_IWDG_Refresh(&hiwdg);
#endif
}


