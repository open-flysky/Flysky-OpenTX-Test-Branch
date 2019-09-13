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

#include "curveedit.h"
#include "keyboard_curve.h"
#include "opentx.h" // TODO for applyCustomCurve

CurveEdit::CurveEdit(Window * parent, const rect_t &rect, uint8_t index) :
  Curve(parent, rect, [=](int x) -> int {
    return applyCustomCurve(x, index);
  }),
  index(index),
  current(-1)
{
  update();
}

void CurveEdit::update()
{
  clearPoints();
  pointsPtr = curveAddress(index);
  uint8_t size = curveAddress(index+1) - pointsPtr;
  if ((size & 1) == 0) {
    pointsTotal = (size / 2) + 1;
    custom = true;
  }
  else {
    pointsTotal = size;
    custom = false;
  }
  for (uint8_t i = 0; i < pointsTotal; i++) {
    if (hasFocus() && current == i) {
      position = [=] () -> int {
        return getPoint(index, i).x;
      };
    }
    else {
      addPoint(getPoint(index, i), TEXT_COLOR);
    }
  }
  invalidate();
}

bool CurveEdit::onTouchEnd(coord_t x, coord_t y)
{
  if (!hasFocus()) {
    setFocus();
    update();
  }

  CurveKeyboard * keyboard = CurveKeyboard::instance();
  if (keyboard->getField() != this) {
    keyboard->setField(this);
  }
  bool pointSelect = false;

  for (int i=0; i < pointsTotal; i++) {
    if (i != current) {
      point_t point = getPoint(index, i);
      if (abs(getPointX(point.x) - x) <= 10 && abs(getPointY(point.y) - y) <= 10) {
        current = i;
        pointSelect = true;
        update();
        break;
      }
    }
  }

  if(!pointSelect){
    int8_t* point = pointsPtr + current;
    *point = (int8_t)((((y) * (2000) / (height()) - 1000) / 10)*-1);
    if(*point < -100) *point = -100;
    if(*point > 100) *point = 100;
    storageDirty(EE_MODEL);
    invalidate();
  }
  return true;
}

void CurveEdit::onFocusLost()
{
  CurveKeyboard::instance()->disable(true);
}

void CurveEdit::next()
{
  if (++current > (int8_t)points.size()) {
    current = 0;
  }
  update();
}

void CurveEdit::previous()
{
  if (current == 0) current = points.size();
  else current--;
  update();
}

void CurveEdit::up()
{
  //move on Y axis
  if (current < pointsTotal) {
    int8_t* point = pointsPtr + current;
    *point = min<int8_t>(100, ++(*point));
    storageDirty(EE_MODEL);
    invalidate();
  }
}

void CurveEdit::down()
{
  //move on Y axis
  if (current < pointsTotal) {
    int8_t* point = pointsPtr + current;
    *point = max<int8_t>(-100, --(*point));
    storageDirty(EE_MODEL);
    invalidate();
  }
}

void CurveEdit::right() {
  if (!custom || current == 0 || current == pointsTotal-1)
    return;

  int targetOffset = pointsTotal + (current-1);
  int8_t* point = pointsPtr + targetOffset;
  int8_t next = current == pointsTotal-2 ? 95 : *(point+1) - 5;
  if((*point)+1 < next) (*point)++;
  storageDirty(EE_MODEL);
  invalidate();
}

void CurveEdit::left()
{
  if (!custom || current == 0 || current == pointsTotal-1)
    return;

  int targetOffset = pointsTotal + (current-1);
  int8_t* point = pointsPtr + targetOffset;
  int8_t prev = current == 1 ? -95 : *(point-1) + 5;
  if((*point)-1 > prev) (*point)--;
  storageDirty(EE_MODEL);
  invalidate();
}
bool CurveEdit::isCustomCurve() {
  return custom;
}

