/**
  * @file    task_uart_input.c
  * @brief   Non‑blocking UART command parser – implementation
  */

#include "task_uart_input.h"
#include "main.h"
#include "sensor_config.h"
#include "task_sensor_read.h"
#include <string.h>
#include <stdio.h>

/*============================================================================*/
/*                      Private variables                                     */
/*============================================================================*/
extern UART_HandleTypeDef huart1;
#define INPUT_BUFFER_SIZE 16U
static char s_buffer[INPUT_BUFFER_SIZE];
static uint8_t s_buffer_pos = 0U;
static UART_HandleTypeDef* s_huart = &huart1;

/*============================================================================*/
/*                      Private helper                                        */
/*============================================================================*/

static void process_command(void)
{
    char cmd = s_buffer[0];
    if (cmd == 'l' || cmd == 'L')
    {
        /* Print sensor list */
        char msg[] = "\r\nSensor list:\r\n";
        HAL_UART_Transmit(s_huart, (uint8_t*)msg, strlen(msg), 100U);
        for (uint8_t i = 0; i < g_sensor_count; i++)
        {
            char line[48];
            int len = snprintf(line, sizeof(line), "%u – %s\r\n",
                               (unsigned)(i + 1), g_sensors[i].description);
            if (len > 0 && len < (int)sizeof(line))
                HAL_UART_Transmit(s_huart, (uint8_t*)line, len, 100U);
        }
        const char* prompt = "Select sensor number (1-9, a=10): ";
        HAL_UART_Transmit(s_huart, (uint8_t*)prompt, strlen(prompt), 100U);
    }
    else if (cmd >= '1' && cmd <= '9')
    {
        uint8_t idx = cmd - '1';
        if (idx < g_sensor_count)
        {
            if (task_sensor_read_select_by_index(idx))
            {
                char ack[64];
                int len = snprintf(ack, sizeof(ack),
                                   "Selected: %s\r\n", g_sensors[idx].description);
                HAL_UART_Transmit(s_huart, (uint8_t*)ack, len, 100U);
                /* Надёжная проверка наличия данных */
                if (g_has_valid_reading)
                {
                    char temp_buf[64];
                    int tlen = snprintf(temp_buf, sizeof(temp_buf),
                                        "Current temperature: %.2f°C\r\n",
                                        (double)g_selected_temperature);
                    if (tlen > 0 && tlen < (int)sizeof(temp_buf))
                        HAL_UART_Transmit(s_huart, (uint8_t*)temp_buf, tlen, 100U);
                }
                else
                {
                    const char* wait = "Waiting for first reading...\r\n";
                    HAL_UART_Transmit(s_huart, (uint8_t*)wait, strlen(wait), 100U);
                }
            }
            else
            {
                const char* err = "Invalid sensor index.\r\n";
                HAL_UART_Transmit(s_huart, (uint8_t*)err, strlen(err), 100U);
            }
        }
        else
        {
            const char* err = "Sensor not available.\r\n";
            HAL_UART_Transmit(s_huart, (uint8_t*)err, strlen(err), 100U);
        }
    }
    else if (cmd == 'a' || cmd == '0')
    {
        if (g_sensor_count >= 10)
        {
            if (task_sensor_read_select_by_index(9))
            {
                char ack[64];
                int len = snprintf(ack, sizeof(ack),
                                   "Selected: %s\r\n", g_sensors[9].description);
                HAL_UART_Transmit(s_huart, (uint8_t*)ack, len, 100U);
                if (g_has_valid_reading)
                {
                    char temp_buf[64];
                    int tlen = snprintf(temp_buf, sizeof(temp_buf),
                                        "Current temperature: %.2f°C\r\n",
                                        (double)g_selected_temperature);
                    if (tlen > 0 && tlen < (int)sizeof(temp_buf))
                        HAL_UART_Transmit(s_huart, (uint8_t*)temp_buf, tlen, 100U);
                }
                else
                {
                    const char* wait = "Waiting for first reading...\r\n";
                    HAL_UART_Transmit(s_huart, (uint8_t*)wait, strlen(wait), 100U);
                }
            }
        }
        else
        {
            const char* err = "Sensor 10 not defined.\r\n";
            HAL_UART_Transmit(s_huart, (uint8_t*)err, strlen(err), 100U);
        }
    }
    else
    {
        const char* err = "Unknown command. Use 1-9, a (or 0) for sensor 10, l for list.\r\n";
        HAL_UART_Transmit(s_huart, (uint8_t*)err, strlen(err), 100U);
    }
}

/*============================================================================*/
/*                      Public functions                                      */
/*============================================================================*/

void task_uart_input_init(void)
{
    s_buffer_pos = 0U;
    memset(s_buffer, 0, INPUT_BUFFER_SIZE);
}

void task_uart_input_run(void)
{
    if (__HAL_UART_GET_FLAG(s_huart, UART_FLAG_RXNE))
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        uint8_t ch;
        if (HAL_UART_Receive(s_huart, &ch, 1, 0) == HAL_OK)
        {
            /* Echo back */
            HAL_UART_Transmit(s_huart, &ch, 1, 100U);

            if (ch == '\n' || ch == '\r')
            {
                if (s_buffer_pos > 0)
                {
                    s_buffer[s_buffer_pos] = '\0';
                    process_command();
                    s_buffer_pos = 0U;
                    memset(s_buffer, 0, INPUT_BUFFER_SIZE);
                }
                /* Optional prompt */
                const char* prompt = "\r\nSelect: ";
                HAL_UART_Transmit(s_huart, (uint8_t*)prompt, strlen(prompt), 100U);
            }
            else if (s_buffer_pos < INPUT_BUFFER_SIZE - 1)
            {
                s_buffer[s_buffer_pos++] = ch;
            }
            else
            {
                const char* err = "\r\nCommand too long\r\n";
                HAL_UART_Transmit(s_huart, (uint8_t*)err, strlen(err), 100U);
                s_buffer_pos = 0U;
                memset(s_buffer, 0, INPUT_BUFFER_SIZE);
            }
        }
    }
}


