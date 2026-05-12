/**
  * @file    sensor_config.h
  * @brief   Static sensor database – ROM codes and descriptions
  * @details Manually entered array of known DS18B20 sensors.
  *          Maximum 10 sensors.
  */

#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <stdint.h>

typedef struct {
    uint8_t rom[8];         /**< 64‑bit unique ROM code */
    const char* description; /**< Human‑readable name */
} SENSOR_INFO_T;

/*============================================================================*/
/*                      Extern declarations (defined in sensor_config.c)      */
/*============================================================================*/

extern const SENSOR_INFO_T g_sensors[];
extern const uint8_t g_sensor_count;

#endif /* SENSOR_CONFIG_H */
