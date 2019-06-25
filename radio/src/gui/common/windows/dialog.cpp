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

#include "dialog.h"
#include "mainwindow.h"
#include "opentx.h"
#include "gridlayout.h"

#define ALERT_FRAME_TOP           70
#define ALERT_FRAME_PADDING       10
#define ALERT_BITMAP_PADDING      15
#define ALERT_TITLE_LEFT          135
#define ALERT_TITLE_LINE_HEIGHT   30
#define ALERT_MESSAGE_TOP         210
#define ALERT_ACTION_TOP          230
#define ALERT_BUTTON_TOP          300

#define DIALOG_BUTTON_WIDTH 70
#define DIALOG_BUTTON_MARGIN 5

Dialog::Dialog(uint8_t type, std::string title, std::string message, std::function<void(void)> onConfirm, std::function<void(void)> onCancel, bool cancellable):
  Window(&mainWindow, {0, 0, LCD_W, LCD_H}, OPAQUE),
  type(type),
  title(std::move(title)),
  message(std::move(message))
{
  new FabIconButton(this, LCD_W - 50, ALERT_BUTTON_TOP, ICON_NEXT, [=]() -> uint8_t
      {
        deleteLater();
        if (onConfirm) onConfirm();
        putEvent(EVT_VK(DialogResult::OK));
        return 0;
      });
  if (cancellable){
    new FabIconButton(this, 50, ALERT_BUTTON_TOP, ICON_BACK, [=]() -> uint8_t
        {
          deleteLater();
          if (onCancel) onCancel();
          putEvent(EVT_VK(DialogResult::Cancel));
          return 0;
        });
  }
  mainWindow.setTopMostWindow(this);
  bringToTop();
}

Dialog::~Dialog()
{
  deleteChildren();
}

void Dialog::paint(BitmapBuffer * dc)
{
  theme->drawBackground();

  if (type == WARNING_TYPE_ALERT || type == WARNING_TYPE_ASTERISK)
    dc->drawBitmap(ALERT_BITMAP_PADDING, ALERT_FRAME_TOP + ALERT_BITMAP_PADDING, theme->asterisk);
  else if (type == WARNING_TYPE_INFO)
    dc->drawBitmap(ALERT_BITMAP_PADDING, ALERT_FRAME_TOP + ALERT_BITMAP_PADDING, theme->busy);
  else
    dc->drawBitmap(ALERT_BITMAP_PADDING, ALERT_FRAME_TOP + ALERT_BITMAP_PADDING, theme->question);

  if (type == WARNING_TYPE_ALERT) {
#if defined(TRANSLATIONS_FR) || defined(TRANSLATIONS_IT) || defined(TRANSLATIONS_CZ)
    dc->drawText(ALERT_TITLE_LEFT, ALERT_FRAME_TOP + ALERT_FRAME_PADDING, STR_WARNING, ALARM_COLOR|DBLSIZE);
    dc->drawText(ALERT_TITLE_LEFT, ALERT_FRAME_TOP + ALERT_FRAME_PADDING + ALERT_TITLE_LINE_HEIGHT, title, ALARM_COLOR|DBLSIZE);
#else
    dc->drawText(ALERT_TITLE_LEFT, ALERT_FRAME_TOP + ALERT_FRAME_PADDING, title.c_str(), ALARM_COLOR|DBLSIZE);
    dc->drawText(ALERT_TITLE_LEFT, ALERT_FRAME_TOP + ALERT_FRAME_PADDING + ALERT_TITLE_LINE_HEIGHT, STR_WARNING, ALARM_COLOR|DBLSIZE);
#endif
  }
  else if (!title.empty()) {
    dc->drawText(ALERT_TITLE_LEFT, ALERT_FRAME_TOP + ALERT_FRAME_PADDING, title.c_str(), ALARM_COLOR|DBLSIZE);
  }

  if (!message.empty()) {
    dc->drawText(ALERT_FRAME_PADDING+5, ALERT_MESSAGE_TOP, message.c_str(), MIDSIZE);
  }

#if 0
  if (action) {
   // dc->drawText(ALERT_FRAME_PADDING+5, ALERT_ACTION_TOP, action);
  }
#endif
}

bool Dialog::onTouchEnd(coord_t x, coord_t y)
{
  Window::onTouchEnd(x, y);
  return true;
}

void Dialog::checkEvents()
{
  Window::checkEvents();
  if (closeCondition && closeCondition())
    deleteLater();
}

void Dialog::deleteLater()
{
  mainWindow.setTopMostWindow(nullptr);
  if (running)
    running = false;
  else
    Window::deleteLater();
}

void Dialog::runForever()
{
  running = true;

  while (running) {
    auto check = pwrCheck();
    if (check == e_power_off) {
      boardOff();
    }
    else if (check == e_power_press) {
      RTOS_WAIT_MS(20);
      continue;
    }

    checkBacklight();
    wdt_reset();

    RTOS_WAIT_MS(20);
    mainWindow.run();
  }

  Window::deleteLater();
}

MessageBox::MessageBox(DialogType type, DialogResult buttons, std::string title, std::string message) :
    Window(&mainWindow, {25, LCD_H / 2 - 100 , LCD_W - 50, 200 }, OPAQUE),
      type(type),
      title(std::move(title)),
      message(std::move(message))
{

  int top = height() - (lineHeight + DIALOG_BUTTON_MARGIN);
  int right = width();

  if(type == DialogType::WARNING_TYPE_INPUT) {
    //TextEdit(this, {right - 100, top - (textBoxHeight + textBotMargin), textBoxWidth *2, textBoxHeight});
  }

  TextButton* button = nullptr;
  buttons = (DialogResult)(buttons & ~DialogResultMask);
  if(buttons & DialogResult::Cancel) {
    right -= DIALOG_BUTTON_WIDTH + DIALOG_BUTTON_MARGIN;
    button = new TextButton(this, {right, top, DIALOG_BUTTON_WIDTH, lineHeight}, "Cancel", EVT_VK(DialogResult::Cancel));
  }
  if(buttons & DialogResult::OK) {
    right -= DIALOG_BUTTON_WIDTH + DIALOG_BUTTON_MARGIN;
    button = new TextButton(this, {right, top, DIALOG_BUTTON_WIDTH, lineHeight}, "OK", EVT_VK(DialogResult::OK));
  }
  if(buttons & DialogResult::Yes) {
    right -= DIALOG_BUTTON_WIDTH + DIALOG_BUTTON_MARGIN;
    button = new TextButton(this, {right, top, DIALOG_BUTTON_WIDTH, lineHeight}, "Yes", EVT_VK(DialogResult::Yes));
  }
  if(buttons & DialogResult::No) {
    right -= DIALOG_BUTTON_WIDTH + DIALOG_BUTTON_MARGIN;
    button = new TextButton(this, {right, top, DIALOG_BUTTON_WIDTH, lineHeight}, "No", EVT_VK(DialogResult::No));
  }
  if(buttons & DialogResult::Abort) {
    right -= DIALOG_BUTTON_WIDTH + DIALOG_BUTTON_MARGIN;
    button = new TextButton(this, {right, top, DIALOG_BUTTON_WIDTH, lineHeight}, "Abort", EVT_VK(DialogResult::Abort));
  }


  if (type == WARNING_TYPE_ALERT || type == WARNING_TYPE_ASTERISK) icon = theme->asterisk;
  else if (type == WARNING_TYPE_INFO) icon = theme->busy;
  else icon = theme->question;
  mainWindow.setTopMostWindow(this);
  bringToTop();
}

MessageBox::~MessageBox()
{
  deleteChildren();
}
#define MESSAGE_BOX_HEADER 30
void MessageBox::paint(BitmapBuffer * dc) {
  dc->drawSolidFilledRect(0, 0, width(), MESSAGE_BOX_HEADER, HEADER_BGCOLOR);
  dc->drawSolidFilledRect(0, MESSAGE_BOX_HEADER, width(), height() - MESSAGE_BOX_HEADER, TEXT_BGCOLOR);
  dc->drawRect(0, 0, width(), height(), 1, SOLID, TEXT_COLOR);
  dc->drawBitmap(ALERT_BITMAP_PADDING, MESSAGE_BOX_HEADER + ALERT_BITMAP_PADDING, icon);
  if (!title.empty()) dc->drawText(ALERT_BITMAP_PADDING, 0, title.c_str(), ALARM_COLOR|MIDSIZE);
  if (!message.empty()) dc->drawText(ALERT_BITMAP_PADDING + icon->getWidth(), MESSAGE_BOX_HEADER + ALERT_BITMAP_PADDING, message.c_str(), STDSIZE);
};

void MessageBox::deleteLater()
{
  mainWindow.setTopMostWindow(nullptr);
  Window::deleteLater();
}


void raiseAlert(const char * title, const char * msg, const char * info, uint8_t sound)
{
  AUDIO_ERROR_MESSAGE(sound);
  auto dialog = new Dialog(WARNING_TYPE_ALERT, title, msg);
  dialog->runForever();
}
