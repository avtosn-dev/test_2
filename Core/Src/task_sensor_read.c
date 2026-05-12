/**
  * @file    task_sensor_read.c
  * @brief   Multi‑sensor DS18B20 reading task – implementation
  * @details FSM states:
  *          - STATE_START_CONV: trigger conversion on selected sensor
  *          - STATE_WAIT: non‑blocking delay (~800 ms) using HAL_GetTick
  *          - STATE_READ: read scratchpad, update temperature
  *          On error, keeps last valid temperature and sets error flag.
  */

#include "task_sensor_read.h"
#include "sensor_config.h"
#include "ds18b20.h"
#include "onewire.h"
#include "main.h"
#include <stdio.h>   /* for snprintf */
#include <string.h>  /* for strlen */

/*============================================================================*/
/*                      Private FSM states                                    */
/*============================================================================*/

typedef enum {
    STATE_START_CONVERSION = 0,
    STATE_WAIT_CONVERSION,
    STATE_READ_RESULT
} SENSOR_FSM_STATE_T;

/*============================================================================*/
/*                      Private variables                                     */
/*============================================================================*/

static SENSOR_FSM_STATE_T s_state = STATE_START_CONVERSION;
static uint32_t s_wait_start = 0U;
static uint8_t  s_selected_index = 0U;
static float    s_last_valid_temp = 0.0f;
static bool     s_error_flag = false;
static uint8_t  s_consecutive_errors = 0U;

/*============================================================================*/
/*                      Global variable definitions (shared)                  */
/*============================================================================*/

volatile float g_selected_temperature = 0.0f;
volatile bool  g_sensor_error         = false;
volatile bool  g_has_valid_reading    = false;   /* Новый флаг */

/*============================================================================*/
/*                      Private helper                                        */
/*============================================================================*/

static void print_sensor_list(void)
{
    extern UART_HandleTypeDef huart1;
    char buf[64];

    if (g_sensor_count == 0)
    {
        const char *msg = "No sensors defined in sensor_config.h\r\n";
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100U);
        return;
    }

    for (uint8_t i = 0; i < g_sensor_count; i++)
    {
        int len = snprintf(buf, sizeof(buf), "%u – %s\r\n",
                           (unsigned)(i + 1), g_sensors[i].description);
        if (len > 0 && len < (int)sizeof(buf))
            HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, 100U);
    }
    const char* prompt = "Select sensor number (1-9, a=10) or 'l' to list:\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)prompt, strlen(prompt), 100U);
}

static bool rom_crc_valid(const uint8_t *rom)
{
    if (rom == NULL) return false;
    return (OneWire_CRC8(rom, 7U) == rom[7]);
}

static void print_rom_crc_diagnostics(void)
{
    extern UART_HandleTypeDef huart1;
    char buf[128];

    for (uint8_t i = 0U; i < g_sensor_count; i++)
    {
        if (!rom_crc_valid(g_sensors[i].rom))
        {
            int len = snprintf(
                buf,
                sizeof(buf),
                "[WARN] ROM CRC mismatch for sensor %u (%s). Check sensor_config ROM.\r\n",
                (unsigned)(i + 1U),
                g_sensors[i].description
            );
            if (len > 0 && len < (int)sizeof(buf))
            {
                HAL_UART_Transmit(&huart1, (uint8_t*)buf, (uint16_t)len, 100U);
            }
        }
    }
}

/*============================================================================*/
/*                      Public functions                                      */
/*============================================================================*/

uint8_t task_sensor_read_get_selected_index(void)
{
    return s_selected_index;
}

bool task_sensor_read_select_by_index(uint8_t idx)
{
    if (idx >= g_sensor_count) return false;
    if (s_selected_index != idx)
    {
        s_selected_index = idx;
        /* Reset FSM to start conversion on new sensor immediately */
        s_state = STATE_START_CONVERSION;
        s_consecutive_errors = 0U;
        s_error_flag = false;
        g_sensor_error = false;
    }
    return true;
}

bool task_sensor_read_select_by_rom(const uint8_t* rom)
{
    for (uint8_t i = 0; i < g_sensor_count; i++)
    {
        bool match = true;
        for (uint8_t j = 0; j < 8; j++)
        {
            if (g_sensors[i].rom[j] != rom[j])
            {
                match = false;
                break;
            }
        }
        if (match) return task_sensor_read_select_by_index(i);
    }
    return false;
}

void task_sensor_read_init(void)
{
    OneWire_Init();

    s_state = STATE_START_CONVERSION;
    s_wait_start = 0U;
    s_selected_index = 0U;
    s_last_valid_temp = 0.0f;
    s_error_flag = false;
    s_consecutive_errors = 0U;
    g_selected_temperature = 0.0f;
    g_sensor_error = false;
    g_has_valid_reading = false;

    /* Print sensor list on startup (handles empty list internally) */
    print_sensor_list();
    print_rom_crc_diagnostics();
}

void task_sensor_read_run(void)
{
    /* Если датчики не описаны – ничего не делаем */
    if (g_sensor_count == 0) return;

    const uint8_t* rom = g_sensors[s_selected_index].rom;

    switch (s_state)
    {
        case STATE_START_CONVERSION:
            if (DS18B20_StartConversion(rom))
            {
                s_wait_start = HAL_GetTick();
                s_state = STATE_WAIT_CONVERSION;
            }
            else
            {
                /* Bus error – retry later */
                s_consecutive_errors++;
                if (s_consecutive_errors >= 3 && !s_error_flag)
                {
                    extern UART_HandleTypeDef huart1;
                    char buf[96];
                    int len = snprintf(buf, sizeof(buf),
                        "[ERROR] Sensor %s not responding, keeping last value: %.2f°C\r\n",
                        g_sensors[s_selected_index].description, (double)s_last_valid_temp);
                    if (len > 0 && len < (int)sizeof(buf))
                        HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, 100U);
                    s_error_flag = true;
                    g_sensor_error = true;
                }
                /* Stay in same state, will retry next tick */
            }
            break;

        case STATE_WAIT_CONVERSION:
            if ((HAL_GetTick() - s_wait_start) >= 800U)
            {
                s_state = STATE_READ_RESULT;
            }
            break;

        case STATE_READ_RESULT:
        {
            int16_t raw_temp = 0;
            if (DS18B20_ReadScratchpad(rom, &raw_temp))
            {
                float temp_c = DS18B20_RawToCelsius(raw_temp);
                s_last_valid_temp = temp_c;
                g_selected_temperature = temp_c;
                g_has_valid_reading = true;   /* успешное чтение */
                s_consecutive_errors = 0U;
                if (s_error_flag)
                {
                    s_error_flag = false;
                    g_sensor_error = false;
                }
            }
            else
            {
                s_consecutive_errors++;
                if (s_consecutive_errors >= 3 && !s_error_flag)
                {
                    extern UART_HandleTypeDef huart1;
                    char buf[96];
                    int len = snprintf(buf, sizeof(buf),
                        "[ERROR] Sensor %s CRC error, keeping last value: %.2f°C\r\n",
                        g_sensors[s_selected_index].description, (double)s_last_valid_temp);
                    if (len > 0 && len < (int)sizeof(buf))
                        HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, 100U);
                    s_error_flag = true;
                    g_sensor_error = true;
                }
                /* Keep last valid temperature */
                g_selected_temperature = s_last_valid_temp;
            }
            s_state = STATE_START_CONVERSION;
            break;
        }

        default:
            s_state = STATE_START_CONVERSION;
            break;
    }
}


