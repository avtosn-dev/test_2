/**
  * @file    onewire.h
  * @brief   1‑Wire protocol low‑level driver – public interface
  * @details Provides reset, read/write byte, and CRC‑8 functions.
  *          Uses PB5 as data line. Requires DWT for microsecond delays.
  */

#ifndef ONEWIRE_H
#define ONEWIRE_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================*/
/*                      Public functions                                      */
/*============================================================================*/

/**
  * @brief Initialise 1‑Wire bus (GPIO PB5) and DWT delay timer.
  * @note  Must be called once before any other 1‑Wire function.
  */
void OneWire_Init(void);

/**
  * @brief Perform 1‑Wire reset pulse and check for presence.
  * @return true if at least one device present, false otherwise.
  */
bool OneWire_Reset(void);

/**
  * @brief Write one byte to the 1‑Wire bus.
  * @param data Byte to send (LSB first).
  */
void OneWire_WriteByte(uint8_t data);

/**
  * @brief Read one byte from the 1‑Wire bus.
  * @return Received byte.
  */
uint8_t OneWire_ReadByte(void);

/**
  * @brief Compute Dallas CRC‑8 of a data buffer.
  * @param data Pointer to data bytes.
  * @param len  Number of bytes.
  * @return CRC‑8 value.
  */
uint8_t OneWire_CRC8(const uint8_t* data, uint8_t len);

#endif /* ONEWIRE_H */
