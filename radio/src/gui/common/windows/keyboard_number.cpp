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

#include "keyboard_number.h"
#include "button.h"
#include "basenumberedit.h"
#include "lcd.h"
#include "keys.h"

constexpr coord_t KEYBOARD_HEIGHT = 90;

NumberKeyboard * NumberKeyboard::_instance = nullptr;

NumberKeyboard::NumberKeyboard():
  Keyboard<BaseNumberEdit>(KEYBOARD_HEIGHT)
{
  new TextButton(this, { LCD_W/2 - 115, 10, 50, 30 }, "<<",
                 [=]() -> uint8_t {
                   if (field) {
                     field->setValue(field->getValue() - 10 * field->getStep());
                   }
                   else putEvent(EVT_VK(VKEY_DEC_LARGE));
                   return 0;
                 }, BUTTON_BACKGROUND | BUTTON_NOFOCUS);

  new TextButton(this, { LCD_W/2 - 55, 10, 50, 30 }, "-",
                 [=]() -> uint8_t {
                   if (field) {
                     field->setValue(field->getValue() - field->getStep());
                   }
                   else putEvent(EVT_VK(VKEY_DEC));
                   return 0;
                 }, BUTTON_BACKGROUND | BUTTON_NOFOCUS);

  new TextButton(this, { LCD_W/2 + 5, 10, 50, 30 }, "+",
                 [=]() -> uint8_t {
                   if (field) {
                     field->setValue(field->getNextValue());
                   }
                   else putEvent(EVT_VK(VKEY_INC));
                   return 0;
                 }, BUTTON_BACKGROUND | BUTTON_NOFOCUS);

  new TextButton(this, { LCD_W/2 + 65, 10, 50, 30 }, ">>",
                 [=]() -> uint8_t {
                   if (field) {
                     field->setValue(field->getValue() + 10 * field->getStep());
                   }
                   else putEvent(EVT_VK(VKEY_INC_LARGE));
                   return 0;
                 }, BUTTON_BACKGROUND | BUTTON_NOFOCUS);

  new TextButton(this, { LCD_W/2 - 115, 50, 50, 30 }, "MIN",
                 [=]() -> uint8_t {
                   if (field) {
                     field->setValue(field->getMin());
                   }
                   else putEvent(EVT_VK(VKEY_MIN));
                   return 0;
                 }, BUTTON_BACKGROUND | BUTTON_NOFOCUS);

  new TextButton(this, { LCD_W/2 + 65, 50, 50, 30 }, "MAX",
                 [=]() -> uint8_t {
                   if (field) {
                     field->setValue(field->getMax());
                   }
                   else putEvent(EVT_VK(VKEY_MAX));
                   return 0;
                 }, BUTTON_BACKGROUND | BUTTON_NOFOCUS);

  new TextButton(this, { LCD_W/2 - 55, 50, 50, 30 }, "DEF",
                 [=]() -> uint8_t {
                   if (field) {
                     field->setValue(field->getDefault());
                   }
                   else putEvent(EVT_VK(VKEY_DEFAULT));
                   return 0;
                 }, BUTTON_BACKGROUND | BUTTON_NOFOCUS);
				 
  new TextButton(this, { LCD_W/2 + 5, 50, 50, 30 }, "+/-",
                 [=]() -> uint8_t {
                   if (field) {
                     field->setValue(-field->getValue());
                   }
                   else putEvent(EVT_VK(VKEY_SIGN));
                   return 0;
                 }, BUTTON_BACKGROUND | BUTTON_NOFOCUS);
}

NumberKeyboard::~NumberKeyboard()
{
  _instance = nullptr;
}

void NumberKeyboard::paint(BitmapBuffer * dc)
{
  lcdSetColor(RGB(0xE0, 0xE0, 0xE0));
  dc->clear(CUSTOM_COLOR);
}
