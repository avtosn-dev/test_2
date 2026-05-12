/**
  * @file    sensor_config.c
  * @brief   Definitions of the static sensor database
  */

#include "sensor_config.h"

/*============================================================================*/
/*                      Sensor list – EDIT HERE                               */
/*============================================================================*/

const SENSOR_INFO_T g_sensors[] = {
    { .rom = {0x28, 0x4F, 0xA8, 0x79, 0x00, 0x00, 0x00, 0x79}, .description = "Living Room" },
    { .rom = {0x28, 0xCA, 0x06, 0x7B, 0x00, 0x00, 0x00, 0xEE}, .description = "Kitchen" },
    /* Заглушка: замените 8 байт ROM на реальный с третьего DS18B20 (последний байт — CRC первых семи). */
    { .rom = {0x28, 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x0A}, .description = "Third (replace ROM)" },

    /* Add up to 10 sensors */
};

const uint8_t g_sensor_count = sizeof(g_sensors) / sizeof(g_sensors[0]);


