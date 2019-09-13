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
#include "keyboard_number.h"
#include "keyboard_text.h"

void DMACopy(void * src, void * dest, unsigned len);
STRUCT_TOUCH touchState;
STRUCT_TOUCH lastTouch;

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

static uint32_t getSlideEvent() {
  coord_t x = touchState.X - touchState.startX;
  coord_t y = touchState.Y - touchState.startY;
  int slideSize = LCD_W / 4;
  if (x > slideSize) return TOUCH_SLIDE_RIGHT;
  else if (x < -slideSize) return TOUCH_SLIDE_LEFT;
  else if (y > slideSize) return TOUCH_SLIDE_UP;
  else if (y < -slideSize) return TOUCH_SLIDE_DOWN;
  return 0;
}

void MainWindow::checkEvents(bool luaActive) {
  // Probably touch manager should be used
  // For now we use this simplified mapping
  // Checking if event is new is necessary

  if(lastTouch.Event != touchState.Event ||
      lastTouch.X != touchState.X || lastTouch.Y != touchState.Y ||
      lastTouch.lastX != touchState.lastX || lastTouch.lastY != touchState.lastY) {

    lastTouch.Event = touchState.Event;
    lastTouch.X = touchState.X;
    lastTouch.Y = touchState.Y;
    lastTouch.lastX = touchState.lastX;
    lastTouch.lastY = touchState.lastY;

    bool handled = false;
    uint32_t event = 0;
    bool screenOff = !isBacklightEnabled();
    if (touchState.Event == TE_DOWN) {
       if(screenOff) handled = true; //just ignore
       else if(!luaActive) handled = onTouchStart(touchState.X, touchState.Y);
       else if(topMostWindow != nullptr) handled = onTouchStart(topMostWindow, touchState.X, touchState.Y);
       if(!handled) event = TOUCH_DOWN;
    }
    else if (touchState.Event == TE_UP) {
       touchState.Event = TE_NONE;
       if(screenOff) handled = true; //just ignore
       else if(!luaActive) handled = onTouchEnd(touchState.startX, touchState.startY);
       else if(topMostWindow != nullptr) handled = onTouchEnd(topMostWindow, touchState.startX, touchState.startY);
       if(!handled){
         //maybe it was long range slide
         uint32_t slideEvent = getSlideEvent();
         event = slideEvent != 0 ? slideEvent : TOUCH_UP;
       }
    }
    else if (touchState.Event == TE_SLIDE) {
       coord_t x = touchState.X - touchState.lastX;
       coord_t y = touchState.Y - touchState.lastY;
       int slideSize = 5;

       x -= slideSize;
       y -= slideSize;

       if(!luaActive) {
         handled = onTouchSlide(touchState.X, touchState.Y, touchState.startX, touchState.startY, x, y);
       }
       else if(topMostWindow != nullptr) {
         handled = onTouchSlide(topMostWindow, touchState.X, touchState.Y, touchState.startX, touchState.startY, x, y);
       }

       if(!handled) event = getSlideEvent();

       touchState.lastX = touchState.X;
       touchState.lastY = touchState.Y;
    }
    if (event && !handled) putEvent(EVT_TOUCH(event), touchState.X, touchState.Y);
  }


  if(!luaActive) Window::checkEvents();
  else if(topMostWindow != nullptr) topMostWindow->checkEvents();
  emptyTrash();
}

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
  return this->refresh(false);
}

bool MainWindow::refresh(bool luaActive)
{
  if(luaActive && topMostWindow != nullptr) topMostWindow->invalidate();
  if (invalidatedRect.w) {
    if(!luaActive) {
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
    }
    lcd->setOffset(0, 0);
    lcd->setClippingRect(invalidatedRect.left(), invalidatedRect.right(), invalidatedRect.top(), invalidatedRect.bottom());
    if(!luaActive) {
      fullPaint(lcd);
    }
    else if(topMostWindow != nullptr) {
      coord_t x = lcd->getOffsetX();
      coord_t y = lcd->getOffsetY();
      coord_t xmin, xmax, ymin, ymax;
      lcd->getClippingRect(xmin, xmax, ymin, ymax);
      paintChild(lcd, topMostWindow, x, y, xmin, xmax, ymin, ymax);
      setMaxClientRect(lcd);
    }

    invalidatedRect.w = 0;
    return true;
  }
  else {
    return luaActive;
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

void MainWindow::setMaxClientRect(BitmapBuffer * dc) {
  dc->setOffset(0, 0);
  dc->clearClippingRect();
}

void MainWindow::drawFatalError(const char * message)
{
    invalidate();
    lcdNextLayer();
    lcd->setOffset(0, 0);
    lcd->clearClippingRect();
    lcd->clear();
    lcd->drawSizedText(LCD_W/2, LCD_H/2-20, message, strlen(message), DBLSIZE|CENTERED|TEXT_BGCOLOR);
    lcdRefresh();
}
Window* MainWindow::getTopMostWindow()
{
  return topMostWindow;
}

void MainWindow::setTopMostWindow(Window* window)
{
  topMostWindow = window;
  invalidate();
}
void MainWindow::showKeyboard(KeyboardType keybordType)
{
    switch(keybordType){
      case KeyboardType::KeyboardNumIncDec:
        NumberKeyboard::instance()->attachTo(this);
        topMostWindow = NumberKeyboard::instance();
        break;
      case KeyboardType::KeyboardAlphabetic:
        TextKeyboard::instance()->attachTo(this);
        topMostWindow = TextKeyboard::instance();
        break;
      case KeyboardType::KeyboardNone:
        if(topMostWindow != nullptr) detachChild(topMostWindow);
        topMostWindow = nullptr;
        invalidate();
        break;
    }
}

void MainWindow::run(bool luaActive)
{
  if(lastLuaState != luaActive) {
    resetDisplayRect();
    lastLuaState = luaActive;
    if(!lastLuaState) {
      showKeyboard(KeyboardType::KeyboardNone);
      invalidate();
    }
  }
  checkEvents(luaActive);
  if (refresh(luaActive)) {
    lcdRefresh();
  }
}
