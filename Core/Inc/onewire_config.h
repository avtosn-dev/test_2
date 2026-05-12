/**
  * @file    onewire_config.h
  * @brief   Hardware configuration for the 1‑Wire bus
  * @details Defines the GPIO port and pin used by the onewire.c driver.
  *          Modify these macros when changing the wiring.
  */

#ifndef ONEWIRE_CONFIG_H
#define ONEWIRE_CONFIG_H

#include "main.h"

/* 1‑Wire bus port and pin (open‑drain, external pull‑up required) */
#define ONEWIRE_PORT        GPIOA
#define ONEWIRE_PIN         GPIO_PIN_1

#endif /* ONEWIRE_CONFIG_H */
