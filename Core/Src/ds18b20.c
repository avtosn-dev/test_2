/**
  * @file    ds18b20.c
  * @brief   DS18B20 temperature sensor high‑level driver – implementation
  */

#include "ds18b20.h"
#include "onewire.h"
#include <stddef.h>



/* DS18B20 commands */
#define DS18B20_CMD_CONVERT     0x44
#define DS18B20_CMD_READ_SCRATCH 0xBE
#define DS18B20_CMD_SKIP_ROM    0xCC

/* Scratchpad size */
#define SCRATCHPAD_SIZE 9

bool DS18B20_StartConversion(const uint8_t* rom)
{
    if (!OneWire_Reset()) return false;

    if (rom != NULL)
    {
        /* Match ROM command (0x55) + send 8 bytes */
        OneWire_WriteByte(0x55);
        for (uint8_t i = 0; i < 8; i++) OneWire_WriteByte(rom[i]);
    }
    else
    {
        OneWire_WriteByte(DS18B20_CMD_SKIP_ROM);
    }

    OneWire_WriteByte(DS18B20_CMD_CONVERT);
    return true;
}

bool DS18B20_ReadScratchpad(const uint8_t* rom, int16_t* raw_temp)
{
    if (raw_temp == NULL) return false;
    if (!OneWire_Reset()) return false;

    if (rom != NULL)
    {
        OneWire_WriteByte(0x55);
        for (uint8_t i = 0; i < 8; i++) OneWire_WriteByte(rom[i]);
    }
    else
    {
        OneWire_WriteByte(DS18B20_CMD_SKIP_ROM);
    }

    OneWire_WriteByte(DS18B20_CMD_READ_SCRATCH);

    uint8_t scratch[SCRATCHPAD_SIZE];
    for (uint8_t i = 0; i < SCRATCHPAD_SIZE; i++)
    {
        scratch[i] = OneWire_ReadByte();
    }

       /* Verify CRC */
    if (OneWire_CRC8(scratch, SCRATCHPAD_SIZE - 1) != scratch[8])
    {
        return false;
    }

    *raw_temp = (int16_t)(scratch[1] << 8 | scratch[0]);
    return true;
}

float DS18B20_RawToCelsius(int16_t raw_temp)
{
    return (float)raw_temp / 16.0f;
}


