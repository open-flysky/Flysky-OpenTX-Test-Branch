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

#ifndef _NUMBEREDIT2_H_
#define _NUMBEREDIT2_H_

#include "basenumberedit.h"
#include "numberedit.h"
#include "button.h"
#include "choice.h"
#include <string>


class NumberEdit2 : public NumberEdit, public Choice {
public:
  NumberEdit2(Window * parent, const rect_t & rect, int32_t vmin, int32_t vmax, int32_t minSelect, int32_t maxSelect, const char* label, const char * values,
      int buttonWidth, std::function<int32_t()> getValue, std::function<void(int32_t)> setValue = nullptr, LcdFlags flags = 0);

  bool onTouchEnd(coord_t x, coord_t y) override;
  void paint(BitmapBuffer * dc) override;
  void checkEvents() override;
protected:
  uint8_t onButtonCheck();
  virtual void setOutputType();
  bool checked;
  std::function<bool(int)> isValueAvailable;
  std::function<std::string(int32_t)> textHandler;
  std::function<void(uint8_t)> checkBoxChangedHandler;
  TextButton* textButton;
  uint paintCount;
};

#if defined(GVARS)
class GvarNumberEdit : public NumberEdit2 {
public:
  GvarNumberEdit(Window * parent, const rect_t & rect, int32_t vmin, int32_t vmax, std::function<int32_t()> getValue, std::function<void(int32_t)> setValue = nullptr, LcdFlags flags = 0);
protected:
  bool isGvarValue(int value);
  std::string getGVarName(int32_t value);
  void setOutputType() override;
  LcdFlags flags;
};
#else
#define GvarNumberEdit NumberEdit
#endif
#endif
