#include <utility>

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

#include "choice.h"
#include "menu.h"
#include "draw_functions.h"
#include "strhelpers.h"

const uint8_t LBM_DROPDOWN[] = {
#include "mask_dropdown.lbm"
};

ChoiceBase::ChoiceBase(const char * values, int16_t vmin, int16_t vmax, std::function<int16_t()> getValue,
    std::function<void(int16_t)> setValue, LcdFlags flags) :
      values(values),
      vmin(vmin),
      vmax(vmax),
      getValue(std::move(getValue)),
      setValue(std::move(setValue)),
      flags(flags),
      readOnly(0)
{

}


Choice::Choice(Window * parent, const rect_t &rect, const char * values, int16_t vmin, int16_t vmax,
               std::function<int16_t()> getValue, std::function<void(int16_t)> setValue, LcdFlags flags) :
  Window(parent, rect),
  ChoiceBase(values, vmin, vmax, getValue, setValue, flags)
{
}

void ChoiceBase::paintChoice(BitmapBuffer * dc, bool hasFocus, const rect_t rect) {
  LcdFlags textColor = 0;
  LcdFlags lineColor = CURVE_AXIS_COLOR;
  if (hasFocus) {
    textColor = TEXT_INVERTED_BGCOLOR;
    lineColor = TEXT_INVERTED_BGCOLOR;
  }
  if(readOnly) {
    textColor = 0;
    lineColor = TEXT_BGCOLOR;
  }
  if (textHandler) dc->drawText(3, Y_ENLARGEABLE, textHandler(getValue()).c_str(), flags | textColor);
  else if (values) drawTextAtIndex(dc, 3, Y_ENLARGEABLE, values, getValue() - vmin, flags | textColor);
  else drawNumber(dc, 3, Y_ENLARGEABLE, getValue(), flags | textColor);
  drawSolidRect(dc, 0, 0, rect.w, rect.h, 1, lineColor);
  dc->drawBitmapPattern(rect.w - 14, (rect.h - 5) / 2, LBM_DROPDOWN, lineColor);
}

bool ChoiceBase::handleTouchEnd(coord_t x, coord_t y)
{
  if(readOnly) return false;
  AUDIO_KEY_PRESS();
  auto menu = new Menu();
  auto value = getValue();
  int count = 0;
  int current = -1;

  for (int i = vmin; i <= vmax; ++i) {
    if (isValueAvailable && !isValueAvailable(i))
      continue;
    if (textHandler) {
      menu->addLine(textHandler(i), [=]() {
        setValue(i);
      });
    }
    else if(values){
      menu->addLine(TEXT_AT_INDEX(values, i - vmin), [=]() {
        setValue(i);
      });
    }
    else {
      char buffer[8];
      sprintf(buffer, "%d", i);
      menu->addLine(std::string(buffer), [=]() {
        setValue(i);
      });
    }
    if (value == i) {
      current = count;
    }
    ++count;
  }

  if (current >= 0) {
    menu->select(current);
  }

  return true;
}


void Choice::paint(BitmapBuffer * dc)
{
  paintChoice(dc, this->hasFocus(), rect);
}

bool Choice::onTouchEnd(coord_t x, coord_t y)
{
  bool result = handleTouchEnd(x,y);
  setFocus();
  return result;
}

CustomCurveChoice::CustomCurveChoice(Window * parent, const rect_t &rect, int16_t vmin, int16_t vmax,
                                     std::function<int16_t()> getValue, std::function<void(int16_t)> setValue, LcdFlags flags) :
  Window(parent, rect),
  vmin(vmin),
  vmax(vmax),
  getValue(std::move(getValue)),
  setValue(std::move(setValue)),
  flags(flags)
{
}

void CustomCurveChoice::paint(BitmapBuffer * dc)
{
  bool hasFocus = this->hasFocus();
  char s[8];
  int16_t value = getValue();
  LcdFlags textColor = 0;
  LcdFlags lineColor = CURVE_AXIS_COLOR;
  if (hasFocus) {
    textColor = TEXT_INVERTED_BGCOLOR;
    lineColor = TEXT_INVERTED_BGCOLOR;
  }
  dc->drawText(3, 2, getCurveString(s, value), flags | textColor);
  drawSolidRect(dc, 0, 0, rect.w, rect.h, 1, lineColor);
  dc->drawBitmapPattern(rect.w - 14, (rect.h - 5) / 2, LBM_DROPDOWN, lineColor);
}

bool CustomCurveChoice::onTouchEnd(coord_t x, coord_t y)
{
  auto menu = new Menu();
  auto value = getValue();
  int count = 0;
  int current = -1;
  char s[8];

  for (int i = vmin; i <= vmax; ++i) {
      menu->addLine(getCurveString(s, i), [=]() {
        setValue(i);
      });

    if (value == i) {
      current = count;
    }
    ++count;
  }

  if (current >= 0) {
    menu->select(current);
  }

  setFocus();
  return true;
}
