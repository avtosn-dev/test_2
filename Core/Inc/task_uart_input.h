/**
  * @file    task_uart_input.h
  * @brief   Non‑blocking UART command parser – public interface
  * @details Reads characters from USART1, buffers until newline,
  *          then processes commands: digits, 'a', 'l'.
  */

#ifndef TASK_UART_INPUT_H
#define TASK_UART_INPUT_H

void task_uart_input_init(void);
void task_uart_input_run(void);

#endif /* TASK_UART_INPUT_H */
