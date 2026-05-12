/**
  * @file    task_uart_print.c
  * @brief   Periodically prints the currently selected sensor's temperature.
  */

#include "task_uart_print.h"
#include "main.h"
#include "task_sensor_read.h"
#include "sensor_config.h"
#include <stdio.h>

extern UART_HandleTypeDef huart1;

void task_uart_print_init(void)
{
    /* Nothing to init – UART already configured */
}

void task_uart_print_run(void)
{
    char buffer[64];
    int len;
    uint8_t selected_idx = task_sensor_read_get_selected_index();
    const char *sensor_name = "Unknown";

    if (selected_idx < g_sensor_count)
    {
        sensor_name = g_sensors[selected_idx].description;
    }

    if (!g_sensor_error)
    {
        len = snprintf(buffer, sizeof(buffer),
                       "%s: %.2f °C\r\n",
                       sensor_name,
                       (double)g_selected_temperature);
    }
    else
    {
        len = snprintf(buffer, sizeof(buffer),
                       "%s error - last valid: %.2f °C\r\n",
                       sensor_name,
                       (double)g_selected_temperature);
    }

    if (len > 0 && len < (int)sizeof(buffer))
    {
        HAL_UART_Transmit(&huart1, (uint8_t*)buffer, (uint16_t)len, 100U);
    }
}


