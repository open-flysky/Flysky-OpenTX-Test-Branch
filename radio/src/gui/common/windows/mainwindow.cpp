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

#include "touch_driver.h"
#include "lcd.h"
#include "mainwindow.h"
#include "keys.h"
#include <queue>
#include "opentx.h"

void DMACopy(void * src, void * dest, unsigned len);
STRUCT_TOUCH touchState;
MainWindow mainWindow;

std::queue<touch_event_type>TouchQueue;

void MainWindow::emptyTrash()
{
  for (auto window: trash) {
    delete window;
  }
  trash.clear();
}
void MainWindow::checkEvents()
{
	this->checkEvents(false);
}

void MainWindow::checkEvents(bool luaActive) {
  if (touchState.Event == TE_DOWN) {
	if(!luaActive) onTouchStart(touchState.X, touchState.Y);
    putEvent(EVT_TOUCH(TOUCH_DOWN));
  }
  else if (touchState.Event == TE_UP) {
    touchState.Event = TE_NONE;
    if(!luaActive) onTouchEnd(touchState.startX, touchState.startY);
    putEvent(EVT_TOUCH(TOUCH_UP));
  }
  else if (touchState.Event == TE_SLIDE) {
    coord_t x = touchState.X - touchState.lastX;
    coord_t y = touchState.Y - touchState.lastY;

    if (x > 5)
    {
      putEvent(EVT_TOUCH(TOUCH_SLIDE_RIGHT));
    }
    else if (x < -5)
    {
      putEvent(EVT_TOUCH(TOUCH_SLIDE_LEFT));
    }

    if (y > 5)
    {
      putEvent(EVT_TOUCH(TOUCH_SLIDE_DOWN));
    }
    else if (y < -5)
    {
      putEvent(EVT_TOUCH(TOUCH_SLIDE_UP));
    }

    if(!luaActive) onTouchSlide(touchState.X, touchState.Y, touchState.startX, touchState.startY, x, y);
    touchState.lastX = touchState.X;
    touchState.lastY = touchState.Y;
  }

  if(!luaActive) Window::checkEvents();

  emptyTrash();
}
/*
void MainWindow::checkEvents()
{
  if(legacyUImode) return;
  touch_event_type touch_evt;
  static touch_event_type last_evt;

  memset(&touch_evt, 0, sizeof(touch_evt));
  touch_evt.touch_type = TE_NONE;

  if (touchState.Event == TE_DOWN) {
    //onTouchStart(touchState.X + scrollPositionX, touchState.Y + scrollPositionY);
    onTouchStart(touchState.X, touchState.Y);
    //putEvent(EVT_TOUCH(TOUCH_DOWN));
    touch_evt.touch_type = TE_DOWN;
    touch_evt.touch_x = touchState.X;
    touch_evt.touch_y = touchState.Y;

  }
  else if (touchState.Event == TE_UP) {
    touchState.Event = TE_NONE;
    //onTouchEnd(touchState.startX + scrollPositionX, touchState.startY + scrollPositionY);
    onTouchEnd(touchState.startX, touchState.startY);

    touch_evt.touch_type = TE_UP;
    touch_evt.touch_x = touchState.startX;
    touch_evt.touch_y = touchState.startY;
  }
  else if (touchState.Event == TE_SLIDE) {
    coord_t x = touchState.X - touchState.lastX;
    coord_t y = touchState.Y - touchState.lastY;

    if (x != 0 && y != 0)
    {
      touch_evt.touch_type = TE_SLIDE;
      touch_evt.slide_start_x = touchState.lastX;
      touch_evt.slide_start_y = touchState.lastY;
      touch_evt.slide_end_x = touchState.X;
      touch_evt.slide_end_y = touchState.Y;
    }

    onTouchSlide(touchState.X, touchState.Y, touchState.startX, touchState.startY, x, y);
    touchState.lastX = touchState.X;
    touchState.lastY = touchState.Y;
  }


  if (touch_evt.touch_type != TE_NONE  && TouchQueue.size() < MAX_TOUCH_EVENT_CNT)
  {
    if (0 != memcmp(&touch_evt, &last_evt, sizeof(touch_event_type)))
    {
      TouchQueue.push(touch_evt);
      last_evt = touch_evt;
      //TRACE("empty = %d, size = %d\r\n", TouchQueue.empty() ? 1:0, TouchQueue.size());
    }
  }

  Window::checkEvents();

  emptyTrash();
}
*/
void MainWindow::invalidate(const rect_t & rect)
{
  if (invalidatedRect.w) {
    coord_t left = min(invalidatedRect.left(), rect.left());
    coord_t right = max(invalidatedRect.right(), rect.right());
    coord_t top = min(invalidatedRect.top(), rect.top());
    coord_t bottom = max(invalidatedRect.bottom(), rect.bottom());
    invalidatedRect = {left, top, right - left, bottom - top};
  }
  else {
    invalidatedRect = rect;
  }
}

bool MainWindow::refresh()
{
  if (invalidatedRect.w) {
    if (invalidatedRect.x > 0 || invalidatedRect.y > 0 || invalidatedRect.w < LCD_W || invalidatedRect.h < LCD_H) {
      //TRACE("Refresh rect: left=%d top=%d width=%d height=%d", invalidatedRect.left(), invalidatedRect.top(), invalidatedRect.w, invalidatedRect.h);
      BitmapBuffer * previous = lcd;
      lcdNextLayer();
      DMACopy(previous->getData(), lcd->getData(), DISPLAY_BUFFER_SIZE);
    }
    else {
      //TRACE("Refresh full screen");
      lcdNextLayer();
    }
    lcd->setOffset(0, 0);
    lcd->setClippingRect(invalidatedRect.left(), invalidatedRect.right(), invalidatedRect.top(), invalidatedRect.bottom());
    fullPaint(lcd);
    invalidatedRect.w = 0;
    return true;
  }
  else {
    return false;
  }
}

void MainWindow::resetDisplayRect(){
  lcd->setOffset(0, 0);
  lcd->clearClippingRect();
  lcdNextLayer();
  lcd->setOffset(0, 0);
  lcd->clearClippingRect();
  lcdNextLayer();
}

void MainWindow::run(bool luaActive)
{
  if(lastLuaState != luaActive) {
    resetDisplayRect();
    lastLuaState = luaActive;
    if(!lastLuaState) {
      invalidate();
    }
  }

  checkEvents(luaActive);
  if (luaActive || refresh()) {
    lcdRefresh();
  }
}
