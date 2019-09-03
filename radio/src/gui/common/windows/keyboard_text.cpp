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

#include "keyboard_text.h"
#include "textedit.h"
#include "fonts.h"
#include "lcd.h"

int8_t char2idx(char c);
char idx2char(int8_t idx);

constexpr coord_t KEYBOARD_HEIGHT = 190;

TextKeyboard * TextKeyboard::_instance = nullptr;

const uint8_t LBM_KEY_UPPERCASE[] = {
#include "mask_key_uppercase.lbm"
};

const uint8_t LBM_KEY_LOWERCASE[] = {
#include "mask_key_lowercase.lbm"
};

const uint8_t LBM_KEY_BACKSPACE[] = {
#include "mask_key_backspace.lbm"
};

const uint8_t LBM_KEY_LETTERS[] = {
#include "mask_key_letters.lbm"
};

const uint8_t LBM_KEY_NUMBERS[] = {
#include "mask_key_numbers.lbm"
};

const uint8_t LBM_KEY_SPACEBAR[] = {
#include "mask_key_spacebar.lbm"
};

const uint8_t * const LBM_SPECIAL_KEYS[] = {
  LBM_KEY_BACKSPACE,
  LBM_KEY_UPPERCASE,
  LBM_KEY_LOWERCASE,
  LBM_KEY_LETTERS,
  LBM_KEY_NUMBERS,
};

const char * const KEYBOARD_LOWERCASE[] = {
  "qwertyuiop",
  " asdfghjkl",
  "\201zxcvbnm\200",
  "\204\t\n"
};

const char * const LARGE_KEYBOARD_LOWERCASE[] = {
  "abcdefg",
  "hijklmn",
  "opqrstu",
  "\201vwxyz\200",
  "\204\t\n"
};

const char * const KEYBOARD_UPPERCASE[] = {
  "QWERTYUIOP",
  " ASDFGHJKL",
  "\202ZXCVBNM\200",
  "\204\t\n"
};

const char * const LARGE_KEYBOARD_UPPERCASE[] = {
  "ABCDEFG",
  "HIJKLMN",
  "OPQRSTU",
  "\202VWXYZ\200",
  "\204\t\n"
};

const char * const KEYBOARD_NUMBERS[] = {
  "1234567890",
  "_-",
  "                 \200",
  "\203\t\n"
};
const char * const LARGE_KEYBOARD_NUMBERS[] = {
  "1234567",
  "890_-",
  "                 \200",
  "\203\t\n"
};

const char * const * KEYBOARD_LAYOUTS[] = {
  KEYBOARD_UPPERCASE,
  KEYBOARD_LOWERCASE,
  KEYBOARD_LOWERCASE,
  KEYBOARD_NUMBERS,
};

const char * const * LARGE_KEYBOARD_LAYOUTS[] = {
  LARGE_KEYBOARD_UPPERCASE,
  LARGE_KEYBOARD_LOWERCASE,
  LARGE_KEYBOARD_LOWERCASE,
  LARGE_KEYBOARD_NUMBERS,
};

TextKeyboard::TextKeyboard():
  Keyboard<TextEdit>(KEYBOARD_HEIGHT),
  layout(KEYBOARD_LOWERCASE)
{
  setSize();
}

void TextKeyboard::setSize()
{
  {
    layout = KEYBOARD_LOWERCASE;
    x_space = 15;
    x_spacebar = 135;
    x_special = 45;
    x_regular = 30;
    x_enter = 80;
    lines = 4;
    setHeight(KEYBOARD_HEIGHT);
  }
  setTop(LCD_H - height());
}

TextKeyboard::~TextKeyboard()
{
  _instance = nullptr;
}

void TextKeyboard::setCursorPos(coord_t x)
{
  if (!field)
    return;

  uint8_t size = field->getMaxLength();
  char * data = field->getData();
  bool isZChar = (field->lcdFlags & ZCHAR) != 0;
  coord_t rest = x;
  if(strlen(data) == 0){
    if(!isZChar || zlen(data, size) ==0){
      cursorPos = 0;
      cursorIndex = 0;
    }
  }
  else{
    for (cursorIndex = 0; cursorIndex < size; cursorIndex++) {
      char c = data[cursorIndex];
      if(isZChar) c = idx2char(static_cast<int8_t>(c));
      if (c == '\0') break;
      uint8_t w = getCharWidth(c, fontspecsTable[0]);
      if (rest < w)
        break;
      rest -= w;
      }
      cursorPos = x - rest;
  }
  field->invalidate();
}

void TextKeyboard::paint(BitmapBuffer * dc)
{
  lcdSetColor(RGB(0xE0, 0xE0, 0xE0));
  dc->clear(CUSTOM_COLOR);

  for (uint8_t i=0; i<lines; i++) {
    coord_t y = 15 + i * 40;
    coord_t x = 15;
    const char * c = layout[i];
    while(*c) {
      if (*c == ' ') {
        x += x_space;
      }
      else if (*c == '\t') {
        // spacebar
        dc->drawBitmapPattern(x, y, LBM_KEY_SPACEBAR, TEXT_COLOR);
        x += x_spacebar;
      }
      else if (*c == '\n') {
        // enter
        dc->drawSolidFilledRect(x, y-2, 80, 25, TEXT_DISABLE_COLOR);
        dc->drawText(x+40, y, "ENTER", CENTERED);
        x += x_enter;
      }
      else if (int8_t(*c) < 0) {
        dc->drawBitmapPattern(x, y, LBM_SPECIAL_KEYS[uint8_t(*c - 128)], TEXT_COLOR);
        x += x_special;
      }
      else {
        dc->drawSizedText(x, y, c, 1);
        x += x_regular;
      }
      c++;
    }
  }
}

bool TextKeyboard::onTouchEnd(coord_t x, coord_t y)
{
  if (!field)
    return false;
  AUDIO_KEY_PRESS();
  uint8_t size = field->getMaxLength();
  char * data = field->getData();

  char c = 0;

  uint8_t row = max<coord_t>(0, y - lines) / 40;
  const char * key = layout[row];
  field->textChaged();
  while(*key) {
    if (*key == ' '){
      x -= x_space;
    }
    else if (*key == '\t') {
      if (x <= x_spacebar) {
        c = ' ';
        break;
      }
      x -= x_spacebar;
    }
    else if (*key == '\n') {
      if (x <= x_enter) {
        // enter
        field->commit();
        disable(true);
        return true;
      }
      x -= x_enter;
    }
    else if (int8_t(*key) < 0) {
      if (x <= x_special) {
        uint8_t specialKey = *key;
        if (specialKey == 128) {
          // backspace
          if (cursorIndex > 0) {
            char c = data[cursorIndex - 1];
            if((field->lcdFlags & ZCHAR) != 0) c = idx2char(c);
            memmove(data + cursorIndex - 1, data + cursorIndex, size - cursorIndex);
            data[size - 1] = '\0';
            cursorPos -= getCharWidth(c, fontspecsTable[0]);
            --cursorIndex;
          }
        }
        else {
          layout = KEYBOARD_LAYOUTS[specialKey - 129];
          invalidate();
        }
        break;
      }
      x -= x_special;
    }
    else {
      if (x <= x_regular) {
        c = *key;
        break;
      }
      x -= x_regular;
    }
    key++;
  }

  if (c && cursorIndex < size) {
    //move data from right site of cursor
    char* curPos = data + cursorIndex;
    size_t toMove = size - cursorIndex;
    if((field->lcdFlags & ZCHAR)) toMove--;
    memmove(curPos + 1, curPos, size - cursorIndex - 1);
    if((field->lcdFlags & ZCHAR)) data[cursorIndex++] = char2idx(c);
    else data[cursorIndex++] = c;
    cursorPos += getCharWidth(c, fontspecsTable[0]);
  }
  field->invalidate();
  return true;
}
