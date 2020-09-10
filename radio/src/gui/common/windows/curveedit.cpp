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
  Curve(parent, rect, std::bind(&CurveEdit::getValue, this, std::placeholders::_1)),
  index(index),
  current(-1)
{
  update();
  position = std::bind(&CurveEdit::getPos, this);
}
int CurveEdit::getValue(int x) {
  return applyCustomCurve(x, index);
}

int CurveEdit::getPos() {
  if(current != -1 && hasFocus()) {
    return getPoint(index, current).x;
  }
  return 0;
}

bool CurveEdit::hasValidPosition()
{
  return current != -1 && hasFocus();
}
void CurveEdit::update()
{
  clearPoints();
  pointsPtr = curveAddress(index);

  CurveInfo &curve = g_model.curves[index];
  custom = curve.type == CURVE_TYPE_CUSTOM;
  uint8_t size = curveAddress(index+1) - pointsPtr;

  if(custom) pointsTotal = (size / 2) + 1;
  else pointsTotal = size;

  for (uint8_t i = 0; i < pointsTotal; i++) {
    if(i==current && hasFocus()) continue;
    addPoint(getPoint(index, i), TEXT_COLOR);
  }
  invalidate();
}

bool CurveEdit::onTouchEnd(coord_t x, coord_t y)
{
  bool updateContent = false;
  if (!hasFocus()) {
    setFocus();
    if (LCD_W < LCD_H) {
      parent->setInnerHeight(rect.y);
    }
    updateContent = true;
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
        updateContent = true;
        break;
      }
    }
  }

  if (updateContent) {
    update();
  } else if (!pointSelect && current >= 0 && current < pointsTotal){
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
  current = -1;
  parent->adjustInnerHeight();
  update();
  CurveKeyboard::instance()->disable(true);
}

void CurveEdit::next()
{
  if (++current >= pointsTotal) {
    current = 0;
  }
  update();
}

void CurveEdit::previous()
{
  if (current == 0) current = pointsTotal-1;
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
  int8_t next = current == pointsTotal-2 ? 100 : *(point+1);
  if((*point)+1 <= next) (*point)++;
  storageDirty(EE_MODEL);
  invalidate();
}

void CurveEdit::left()
{
  if (!custom || current == 0 || current == pointsTotal-1)
    return;

  int targetOffset = pointsTotal + (current-1);
  int8_t* point = pointsPtr + targetOffset;
  int8_t prev = current == 1 ? -100 : *(point-1);
  if((*point)-1 >= prev) (*point)--;
  storageDirty(EE_MODEL);
  invalidate();
}
bool CurveEdit::isCustomCurve() {
  return custom;
}

