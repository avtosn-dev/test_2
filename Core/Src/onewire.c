/**
  * @file    onewire.c
  * @brief   1‑Wire protocol low‑level driver – implementation
  * @details Uses GPIO with open‑drain mode (external pull‑up required).
  *          Microsecond delays via DWT cycle counter (72 MHz).
  *          Critical timings are protected by disabling interrupts.
  */

#include "onewire.h"
#include "onewire_config.h"
#include "main.h"
#include <stddef.h>

/*============================================================================*/
/*                      Private definitions                                   */
/*============================================================================*/

/* DWT control register and cycle counter */
#define DWT_CTRL            (*((volatile uint32_t*)0xE0001000))
#define DWT_CYCCNT          (*((volatile uint32_t*)0xE0001004))
#define DEMCR               (*((volatile uint32_t*)0xE000EDFC))
#define DEMCR_TRCENA        (1 << 24)

/*============================================================================*/
/*                      Private helper functions                              */
/*============================================================================*/

/**
  * @brief Busy‑wait delay in microseconds (blocking, uses DWT cycle counter)
  * @param us Number of microseconds to delay
  * @details DWT_CYCCNT is reset to 0 at the start of the delay, making the
  *          difference calculation immune to counter overflow.
  */
static void delay_us(uint32_t us)
{
    DWT_CYCCNT = 0;                     /* Reset counter – eliminates overflow jitter */
    uint32_t cycles = us * 72;          /* 72 MHz → 72 cycles per microsecond */
    while (DWT_CYCCNT < cycles);
}

/*============================================================================*/
/*                      Public functions                                      */
/*============================================================================*/

/**
  * @brief Initialize 1‑Wire bus (GPIO) and DWT delay timer.
  */
/**
  * @brief  Initialize the 1-Wire bus and the DWT cycle counter.
  * @note   The DWT is unlocked via the Lock Access Register (LAR),
  *         then enabled through the standard control registers.
  *         All register addresses are hard-coded to avoid dependency
  *         on the exact CMSIS version.
  */
void OneWire_Init(void)
{
    /* ---- 1. Enable GPIOB clock ---- */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* ---- 2. Set up the DWT cycle counter ---- */

    /* Debug Exception and Monitor Control Register (DEMCR)
     * Address: 0xE000EDFC
     * TRCENA (bit 24): must be set to use DWT and ITM. */
    volatile uint32_t *demcr = (volatile uint32_t *)0xE000EDFC;
    *demcr |= (1UL << 24);   // TRCENA = 1

    /* DWT Lock Access Register
     * Address: 0xE0001FB0
     * Writing the key 0xC5ACCE55 unlocks the DWT registers. */
    volatile uint32_t *dwt_lar = (volatile uint32_t *)0xE0001FB0;
    *dwt_lar = 0xC5ACCE55UL;

    /* DWT Control Register
     * Address: 0xE0001000
     * Bit 0 (CYCCNTENA): enable the cycle counter. */
    volatile uint32_t *dwt_ctrl = (volatile uint32_t *)0xE0001000;
    *dwt_ctrl |= 1UL;        // CYCCNTENA = 1

    /* DWT Cycle Counter Register
     * Address: 0xE0001004
     * Reset the counter to start from 0. */
    volatile uint32_t *dwt_cyccnt = (volatile uint32_t *)0xE0001004;
    *dwt_cyccnt = 0UL;

    /* ---- 3. Configure PB5 as open-drain for 1-Wire ---- */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = ONEWIRE_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(ONEWIRE_PORT, &GPIO_InitStruct);

    /* ---- 4. Release the bus ---- */
    HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_SET);
}

/**
  * @brief Perform a 1‑Wire reset pulse and check for presence.
  */
bool OneWire_Reset(void)
{
    bool presence;

    __disable_irq();

    /* Master drives low for 480 µs */
    HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_RESET);
    delay_us(480);

    /* Release line */
    HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_SET);
    delay_us(70);

    /* Read presence pulse */
    presence = (HAL_GPIO_ReadPin(ONEWIRE_PORT, ONEWIRE_PIN) == GPIO_PIN_RESET);

    /* Wait for the remainder of the reset window */
    delay_us(410);

    __enable_irq();

    return presence;
}

/**
  * @brief Write one byte to the 1‑Wire bus (LSB first).
  */
void OneWire_WriteByte(uint8_t data)
{
    for (uint8_t bit = 0; bit < 8; bit++)
    {
        __disable_irq();

        /* Drive low for 2 µs to start the time slot */
        HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_RESET);
        delay_us(2);

        /* Output the data bit (LSB first) */
        if (data & 1)
        {
            HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_SET);
        }

        /* Hold the line for a total slot length of 60 µs */
        delay_us(58);

        /* Release line for inter‑slot recovery */
        HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_SET);
        delay_us(2);

        __enable_irq();

        data >>= 1;
    }
}

/**
  * @brief Read one byte from the 1‑Wire bus.
  */
uint8_t OneWire_ReadByte(void)
{
    uint8_t result = 0;

    for (uint8_t bit = 0; bit < 8; bit++)
    {
        __disable_irq();

        /* Drive low for 2 µs to initiate the read slot */
        HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_RESET);
        delay_us(2);

        /* Release line */
        HAL_GPIO_WritePin(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PIN_SET);
        /* Sample DS18B20 around 15 us from slot start */
        delay_us(13);

        /* Sample the bus */
        if (HAL_GPIO_ReadPin(ONEWIRE_PORT, ONEWIRE_PIN) == GPIO_PIN_SET)
        {
            result |= (1 << bit);
        }

        /* Complete ~60 us read time slot */
        delay_us(45);

        __enable_irq();
    }

    return result;
}

/**
  * @brief Compute Dallas CRC‑8 over a data buffer.
  */
uint8_t OneWire_CRC8(const uint8_t* data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++)
    {
        uint8_t byte = data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            uint8_t mix = (crc ^ byte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            byte >>= 1;
        }
    }
    return crc;
}


