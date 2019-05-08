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

#ifndef _TEXTEDIT_H_
#define _TEXTEDIT_H_

#include "window.h"

uint8_t zlen(const char *str, uint8_t size);

class TextEdit : public Window {
  friend class TextKeyboard;
  public:
    TextEdit(Window * parent, const rect_t &rect, char * value, uint8_t length, LcdFlags flags = ZCHAR, std::function<void(char*)> textChangedFun = nullptr, bool modelData = true) :
      Window(parent, rect),
      value(value),
      length(length),
      lcdFlags(flags),
      textChanged(std::move(textChangedFun)),
      modelData(modelData)
    {
    }

#if defined(DEBUG_WINDOWS)
    std::string getName() override
    {
      return "TextEdit";
    }
#endif

    uint8_t getMaxLength()
    {
      return length;
    }

    char * getData()
    {
      return value;
    }

    void paint(BitmapBuffer * dc) override;

    bool onTouchEnd(coord_t x, coord_t y) override;

    void onFocusLost() override;

  protected:
    void textChaged();
	void commit();
    char * value;
    uint8_t length;
    LcdFlags lcdFlags;
    std::function<void(char*)> textChanged;
    bool modelData;
	bool isTextChanged;

};

#endif // _TEXTEDIT_H_
