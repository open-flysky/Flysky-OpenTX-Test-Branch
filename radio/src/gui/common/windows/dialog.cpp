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
#include "keys.h"

#define ALERT_FRAME_TOP           70
#define ALERT_FRAME_PADDING       10
#define ALERT_BITMAP_PADDING      15
#define ALERT_TITLE_LEFT          135
#define ALERT_TITLE_LINE_HEIGHT   30
#define ALERT_MESSAGE_TOP         210
#define ALERT_ACTION_TOP          230
#define ALERT_BUTTON_TOP          300
#define MAX_PROGRESS_WIDTH        (LCD_W - (2*(ALERT_FRAME_PADDING+5)))
#define MAX_PROGRESS_BAR_WIDTH    (LCD_W - (2*(ALERT_FRAME_PADDING+7)))
#define DIALOG_BUTTON_WIDTH       70
#define DIALOG_BUTTON_MARGIN      5
#define MESSAGE_BOX_HEADER        30

Dialog::Dialog(uint8_t type, std::string title, std::string message, std::function<void(void)> onConfirm, std::function<void(void)> onCancel, bool cancellable, bool hasNextStep):
  Window(&mainWindow, {0, 0, LCD_W, LCD_H}, OPAQUE),
  type(type),
  title(std::move(title)),
  message(std::move(message))
{
  nextButton = nullptr;
  if (hasNextStep) {
    nextButton = new FabIconButton(this, LCD_W - 50, ALERT_BUTTON_TOP, ICON_NEXT, [=]() -> uint8_t
      {
        deleteLater();
        if (onConfirm) onConfirm();
        putEvent(EVT_VK(DialogResult::OK));
        return 0;
      });
  }

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
  else if (type != WARNING_TYPE_PROGRESS)
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
  else if (type == WARNING_TYPE_PROGRESS) {
    LcdFlags flags = ALARM_COLOR;
    if (title.length() < 17) {
      flags |= DBLSIZE;
    }
    dc->drawText(ALERT_BITMAP_PADDING, ALERT_FRAME_TOP + ALERT_FRAME_PADDING, title.c_str(), flags);
  }
  else if (!title.empty()) {
    dc->drawText(ALERT_TITLE_LEFT, ALERT_FRAME_TOP + ALERT_FRAME_PADDING, title.c_str(), ALARM_COLOR|DBLSIZE);
  }

  if (!message.empty()) {
    dc->drawText(ALERT_FRAME_PADDING+5, ALERT_MESSAGE_TOP, message.c_str(), MIDSIZE);
  }

  if (type == WARNING_TYPE_PROGRESS) {
    dc->drawSolidFilledRect(ALERT_FRAME_PADDING+5, ALERT_BUTTON_TOP + 50, MAX_PROGRESS_WIDTH, 20, CURVE_AXIS_COLOR);

    if (progressTotal) {
      dc->drawSolidFilledRect(ALERT_FRAME_PADDING+7, ALERT_BUTTON_TOP + 50 + 3, (MAX_PROGRESS_BAR_WIDTH*progress)/progressTotal, 14, BATTERY_CHARGE_COLOR);
    }

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

  if ((closeCondition && closeCondition()) || (getEvent(true) & EVT_KEY_FIRST(0)) != 0)
    deleteLater();
}

void Dialog::deleteLater()
{
  mainWindow.setTopMostWindow(nullptr);
  // Dialog is being removed in message loop
  // Lets wait for new iteration;
  if (running) running = false;
  else Window::deleteLater();
}

void Dialog::setProgress(const char * title, const char * message, int num, int den, bool enableCloseButton) {
  if (title) {
    this->title = std::move(title);
  }
  else {
    this->title.clear();
  }
  if (message) {
    this->message = std::move(message);
  }
  else {
    this->message.clear();
  }
  this->progress = num;
  this->progressTotal = den;

  if (enableCloseButton && nextButton == nullptr) {
      nextButton = new FabIconButton(this, LCD_W - 50, ALERT_BUTTON_TOP, ICON_NEXT, [=]() -> uint8_t
      {
        deleteLater();
        putEvent(EVT_VK(DialogResult::OK));
        return 0;
      });
  }
  this->invalidate();
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

MessageBox::MessageBox(DialogType type, DialogResult buttons, std::string title, std::string message, std::function<void(DialogResult)> onClose) :
    Window(&mainWindow, {25, LCD_H / 2 - 100 , LCD_W - 50, 200 }, OPAQUE),
      type(type),
      title(std::move(title)),
      message(std::move(message)),
      running(true),
      onClose(onClose)
{
  int top = height() - (lineHeight + DIALOG_BUTTON_MARGIN);
  int right = width();

  if(type == DialogType::WARNING_TYPE_INPUT) {
    //TextEdit(this, {right - 100, top - (textBoxHeight + textBotMargin), textBoxWidth *2, textBoxHeight});
  }

  buttons = (DialogResult)(buttons & ~DialogResultMask);
  if(buttons & DialogResult::Cancel) {
    right -= DIALOG_BUTTON_WIDTH + DIALOG_BUTTON_MARGIN;
    new TextButton(this, {right, top, DIALOG_BUTTON_WIDTH, lineHeight}, STR_DIALOG_CANCEL, EVT_VK(DialogResult::Cancel));
  }
  if(buttons & DialogResult::OK) {
    right -= DIALOG_BUTTON_WIDTH + DIALOG_BUTTON_MARGIN;
    new TextButton(this, {right, top, DIALOG_BUTTON_WIDTH, lineHeight}, STR_DIALOG_OK, EVT_VK(DialogResult::OK));
  }
  if(buttons & DialogResult::Yes) {
    right -= DIALOG_BUTTON_WIDTH + DIALOG_BUTTON_MARGIN;
    new TextButton(this, {right, top, DIALOG_BUTTON_WIDTH, lineHeight}, STR_DIALOG_YES, EVT_VK(DialogResult::Yes));
  }
  if(buttons & DialogResult::No) {
    right -= DIALOG_BUTTON_WIDTH + DIALOG_BUTTON_MARGIN;
    new TextButton(this, {right, top, DIALOG_BUTTON_WIDTH, lineHeight}, STR_DIALOG_NO, EVT_VK(DialogResult::No));
  }
  if(buttons & DialogResult::Abort) {
    right -= DIALOG_BUTTON_WIDTH + DIALOG_BUTTON_MARGIN;
    new TextButton(this, {right, top, DIALOG_BUTTON_WIDTH, lineHeight}, STR_DIALOG_ABORT, EVT_VK(DialogResult::Abort));
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

void MessageBox::paint(BitmapBuffer * dc) {
  dc->drawSolidFilledRect(0, 0, width(), MESSAGE_BOX_HEADER, HEADER_BGCOLOR);
  dc->drawSolidFilledRect(0, MESSAGE_BOX_HEADER, width(), height() - MESSAGE_BOX_HEADER, TEXT_BGCOLOR);
  dc->drawRect(0, 0, width(), height(), 1, SOLID, TEXT_COLOR);
  if (icon != nullptr) dc->drawBitmap(ALERT_BITMAP_PADDING, MESSAGE_BOX_HEADER + ALERT_BITMAP_PADDING, icon, 0, 0, 0, 0, 0.5f);
  if (!title.empty()) dc->drawText(ALERT_BITMAP_PADDING, 5, title.c_str(), TEXT_INVERTED_COLOR | STDSIZE);
  if (!message.empty()) {
    coord_t left = ALERT_BITMAP_PADDING + ((icon != nullptr) ? (icon->getWidth() / 2 + 5) : 0);
    coord_t top = MESSAGE_BOX_HEADER + ALERT_BITMAP_PADDING;

    coord_t allowed = width() - (left + ALERT_BITMAP_PADDING);
    int space = -1;
    unsigned pos = 0;
    unsigned startPos = 0;
    while(pos < message.length()) {
      if(message.at(pos) == ' ') space = pos;
      bool draw = false;
      if(getTextWidth(message.substr(startPos, pos - startPos).c_str(), pos - startPos, STDSIZE) > allowed) {
        pos--;
        draw = true;
      }
      else if(pos == message.length()-1) {
        pos++;
        space = -1;
        draw = true;
      }
      if(draw){
        dc->drawText(left, top, message.substr(startPos, (space != -1 ? space : pos) - startPos).c_str(), STDSIZE);
        if(space != -1)  {
          pos = space;
          startPos = pos + 1;
        }
        else startPos = pos;
        space = -1;
        top += getFontHeight(STDSIZE);
      }
      pos++;
    }
  }

};

void MessageBox::deleteLater()
{
  mainWindow.setTopMostWindow(nullptr);
  Window::deleteLater();
}

void MessageBox::setUpdateMethod(std::function<void(void)> update) {
  this->onUpdate = std::move(update);
}

void MessageBox::checkEvents()
{
  Window::checkEvents();
  if(!running) {
    deleteLater();
    return;
  }
  if(onUpdate != nullptr) {
    onUpdate();
  }

  if(closeCondition != nullptr) {
    DialogResult result = closeCondition();
    if(result) {
      if (onClose != nullptr) onClose(result);
      running = false;
      return;
    }
  }

  for (auto child: children) {
      TextButton* button = reinterpret_cast<TextButton*>(child);
      if(button == nullptr) continue;
      event_t result = button->getResult();
      if(result) {
        if(onClose != nullptr) onClose((DialogResult)(result & ~_MSK_VK_EVENT));
        running = false;
        return;
      }
  }
}

void raiseAlert(const char * title, const char * msg, const char * info, uint8_t sound)
{
  AUDIO_ERROR_MESSAGE(sound);
  auto dialog = new Dialog(WARNING_TYPE_ALERT, title, msg);
  dialog->runForever();
}

Dialog* progressDialog = nullptr;

void drawProgressScreen(const char * title, const char * message, int num, int den) {
  if(!progressDialog) {
    progressDialog = new Dialog(WARNING_TYPE_PROGRESS, title, message, nullptr, nullptr, false, false);
  }

  progressDialog->setProgress(title, message, num, den, false);
  backlightOn();
  wdt_reset();
  mainWindow.run();
}

void drawProgressScreenDone(bool success, const char * title, const char * message) {
  if(progressDialog) {
    progressDialog->setProgress(title, message, 100, 100, true);
    backlightOn();
  }

}

void runProgressScreen() {
  if(progressDialog) {
    progressDialog->runForever();
  }
  //clear pointer no more needed - it will be deleted after closing
  //more elegant will be clear on close but it will require more changes
  progressDialog = nullptr;
}
