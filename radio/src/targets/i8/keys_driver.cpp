/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "opentx.h"

uint32_t readKeys()
{
  uint32_t result = 0;

  // TODO: Remove once touch is there
  if (~TRIMS_GPIO_REG_RVU & TRIMS_GPIO_PIN_RVU)
    result |= 1 << KEY_UP;
  if (~TRIMS_GPIO_REG_RVD & TRIMS_GPIO_PIN_RVD)
    result |= 1 << KEY_DOWN;
  if (~TRIMS_GPIO_REG_RHR & TRIMS_GPIO_PIN_RHR)
    result |= 1 << KEY_RIGHT;
  if (~TRIMS_GPIO_REG_LHL & TRIMS_GPIO_PIN_LHL)
    result |= 1 << KEY_LEFT;
//  if (~TRIMS_GPIO_REG_LVD & TRIMS_GPIO_PIN_LVD)
//    result |= 1 << KEY_MINUS;
//  if (~TRIMS_GPIO_REG_LVU & TRIMS_GPIO_PIN_LVU)
//    result |= 1 << KEY_PLUS;
  if (~KEYS_GPIO_REG_K2 & KEYS_GPIO_PIN_K2)
    result |= 1 << KEY_EXIT;
  if (~KEYS_GPIO_REG_K1 & KEYS_GPIO_PIN_K1)
    result |= 1 << KEY_MENU;

  // if (result != 0) TRACE("readKeys(): result=0x%02x", result);

  return result;
}

uint32_t readTrims()
{
  uint32_t result = 0;

// TODO: Reenable once touch is there
//  if (~TRIMS_GPIO_REG_LHL & TRIMS_GPIO_PIN_LHL)
//    result |= 0x01;
  if (~TRIMS_GPIO_REG_LHR & TRIMS_GPIO_PIN_LHR)
    result |= 0x02;
  if (~TRIMS_GPIO_REG_LVD & TRIMS_GPIO_PIN_LVD)
    result |= 0x04;
  if (~TRIMS_GPIO_REG_LVU & TRIMS_GPIO_PIN_LVU)
    result |= 0x08;
//  if (~TRIMS_GPIO_REG_RVD & TRIMS_GPIO_PIN_RVD)
//    result |= 0x10;
//  if (~TRIMS_GPIO_REG_RVU & TRIMS_GPIO_PIN_RVU)
//    result |= 0x20;
  if (~TRIMS_GPIO_REG_RHL & TRIMS_GPIO_PIN_RHL)
    result |= 0x40;
//  if (~TRIMS_GPIO_REG_RHR & TRIMS_GPIO_PIN_RHR)
//    result |= 0x80;

  // TRACE("readTrims(): result=0x%02x", result);

  return result;
}

uint8_t trimDown(uint8_t idx)
{
  return readTrims() & (1 << idx);
}

uint8_t keyDown()
{
  return readKeys();
}

/* TODO common to ARM */
void readKeysAndTrims()
{
  uint8_t index = 0;
  uint32_t in = readKeys();
  for (uint8_t i = 1; i != uint8_t(1 << TRM_BASE); i <<= 1) {
    keys[index++].input(in & i);
  }

  in = readTrims();
  for (uint8_t i = 1; i != uint8_t(1 << 8); i <<= 1) {
    keys[index++].input(in & i);
  }
}

uint8_t keyState(uint8_t index)
{
  return keys[index].state();
}

#if !defined(BOOT)
uint32_t switchState(uint8_t index)
{
  uint16_t value = adcValues[SWITCH_SA + (index / 3)];
  uint8_t position;

  if (value < 1024)
    position = 0;
  else if (value > 3*1024)
    position = 2;
  else
    position = 1;

  return position == index % 3;
}
#endif

void keysInit()
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;

  GPIO_InitStructure.GPIO_Pin = KEYS_GPIOE_PINS;
  GPIO_Init(GPIOE, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = KEYS_GPIOD_PINS;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
}
