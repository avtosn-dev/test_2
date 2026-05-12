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

    /* Add up to 10 sensors */
};

const uint8_t g_sensor_count = sizeof(g_sensors) / sizeof(g_sensors[0]);


