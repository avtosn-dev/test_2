/**
  * @file    ds18b20.h
  * @brief   DS18B20 temperature sensor high‑level driver – public interface
  * @details Provides functions to start conversion, read scratchpad,
  *          and convert raw value to Celsius.
  */

#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/*                      Public functions                                      */
/*============================================================================*/

/**
  * @brief Start a temperature conversion on a specific sensor.
  * @param rom Pointer to 8‑byte ROM code (can be NULL for Skip ROM).
  * @return true if conversion started successfully, false on bus error.
  */
bool DS18B20_StartConversion(const uint8_t* rom);

/**
  * @brief Read the scratchpad and compute raw temperature value.
  * @param rom      Pointer to 8‑byte ROM code (can be NULL for Skip ROM).
  * @param raw_temp Pointer to store the raw 16‑bit temperature (12‑bit resolution).
  * @return true if read succeeded and CRC matches, false otherwise.
  */
bool DS18B20_ReadScratchpad(const uint8_t* rom, int16_t* raw_temp);

/**
  * @brief Convert raw temperature value to degrees Celsius.
  * @param raw_temp Raw 16‑bit value from DS18B20.
  * @return Temperature in °C (float).
  */
float DS18B20_RawToCelsius(int16_t raw_temp);

#endif /* DS18B20_H */
