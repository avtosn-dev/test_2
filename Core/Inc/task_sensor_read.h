/**
  * @file    task_sensor_read.h
  * @brief   Multi‑sensor DS18B20 reading task – public interface
  */

#ifndef TASK_SENSOR_READ_H
#define TASK_SENSOR_READ_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/*                      Global variables (shared)                             */
/*============================================================================*/

extern volatile float g_selected_temperature;  /**< Last valid temperature of selected sensor */
extern volatile bool  g_sensor_error;          /**< True if last read failed */
extern volatile bool  g_has_valid_reading;     /**< True after at least one successful reading */

/**
  * @brief Get current selected sensor index (0‑based).
  * @return Index into g_sensors array.
  */
uint8_t task_sensor_read_get_selected_index(void);

/**
  * @brief Set selected sensor by index.
  * @param idx 0‑based index (validated internally).
  * @return true if index changed, false if invalid.
  */
bool task_sensor_read_select_by_index(uint8_t idx);

/**
  * @brief Set selected sensor by ROM code (if present in database).
  * @param rom Pointer to 8‑byte ROM.
  * @return true if found and selected, false otherwise.
  */
bool task_sensor_read_select_by_rom(const uint8_t* rom);

/**
  * @brief Initialise sensor reading task.
  */
void task_sensor_read_init(void);

/**
  * @brief Task entry point – called periodically by scheduler (recommended 100 ms).
  */
void task_sensor_read_run(void);

#endif /* TASK_SENSOR_READ_H */
