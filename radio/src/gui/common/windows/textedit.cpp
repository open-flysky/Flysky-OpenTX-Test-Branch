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

#include "textedit.h"
#include "keyboard_text.h"
#include "draw_functions.h"

void TextEdit::paint(BitmapBuffer * dc)
{
  bool hasFocus = this->hasFocus();
  LcdFlags textColor = 0;
  LcdFlags lineColor = CURVE_AXIS_COLOR;
  if (hasFocus) {
    textColor = TEXT_INVERTED_BGCOLOR;
    lineColor = TEXT_INVERTED_BGCOLOR;
  }
  size_t textLength = strlen(value);
  if((lcdFlags & ZCHAR) != 0) textLength = zlen(value, length);

  if (!hasFocus && textLength == 0)
    dc->drawSizedText(3, Y_ENLARGEABLE, "---", length, CURVE_AXIS_COLOR);
  else if(textLength > 0)
    dc->drawSizedText(3, Y_ENLARGEABLE, value, textLength, lcdFlags | textColor);
  drawSolidRect(dc, 0, 0, rect.w, rect.h, 1, lineColor);

  auto keyboard = TextKeyboard::instance();
  if (hasFocus && keyboard->getField() == this) {
    coord_t cursorPos = keyboard->getCursorPos();
    coord_t cursorPosY = (height() - 21)/2;
    dc->drawSolidFilledRect(cursorPos + 2, cursorPosY, 2, 21, 0); // TEXT_INVERTED_BGCOLOR);
  }
}

bool TextEdit::onTouchEnd(coord_t x, coord_t y)
{
  if (!hasFocus()) {
      AUDIO_KEY_PRESS();
      setFocus();
  }

  auto keyboard = TextKeyboard::instance();
  if (keyboard->getField() != this) {
    keyboard->setField(this);
  }

  keyboard->setCursorPos(x);

  return true;
}

void TextEdit::onFocusLost()
{
  TextKeyboard::instance()->disable(true);
  commit();
}
void TextEdit::textChaged() {
  isTextChanged = true;
}
void TextEdit::commit(){
  if(isTextChanged) 
  {
    isTextChanged = false;
    if (modelData) storageDirty(EE_MODEL);
    if (textChanged) textChanged(value);
  }
}

