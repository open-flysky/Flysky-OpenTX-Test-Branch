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

#ifndef _DIALOG_H_
#define _DIALOG_H_

#include "button.h"

enum DialogType {
WARNING_TYPE_ASTERISK,
WARNING_TYPE_CONFIRM,
WARNING_TYPE_INPUT,
WARNING_TYPE_ALERT,
WARNING_TYPE_INFO,
WARNING_TYPE_PROGRESS,
};

#define DialogResultMask  0x100

enum DialogResult {
  Abort = 0x2   | DialogResultMask,
  Cancel = 0x4  | DialogResultMask,
  No = 0x10     | DialogResultMask,
  Yes = 0x20    | DialogResultMask,
  OK = 0x40     | DialogResultMask,
};

#define ALERT_FRAME_TOP           70
#define ALERT_FRAME_PADDING       10
#define ALERT_BITMAP_PADDING      15
#define ALERT_TITLE_LEFT          135
#define ALERT_TITLE_LINE_HEIGHT   30
#define ALERT_MESSAGE_TOP         210
#define ALERT_ACTION_TOP          230
#define ALERT_BUTTON_TOP          300


class Dialog : public Window {
  public:
    Dialog(uint8_t type, std::string title, std::string message="", std::function<void(void)> onConfirm=nullptr, std::function<void(void)> onCancel=nullptr, bool cancellable = false, bool hasNextStep = true);

    ~Dialog() override;

#if defined(DEBUG_WINDOWS)
    std::string getName() override
    {
      return "Dialog";
    }
#endif

    void paint(BitmapBuffer * dc) override;

    bool onTouchEnd(coord_t x, coord_t y) override;

    void deleteLater();

    void checkEvents() override;

    void setCloseCondition(std::function<bool(void)> handler)
    {
      closeCondition = std::move(handler);
    }

    void runForever();
    void setProgress(const char * title, const char * message, int num, int den, bool enableCloseButton);
  protected:
    uint8_t type;
    std::string title;
    std::string message;
    bool running = false;
    int32_t progress;
    int32_t progressTotal;
    std::function<bool(void)> closeCondition;
    FabIconButton* nextButton;
};

class MessageBox : public Window {
public:
  MessageBox(DialogType type, DialogResult buttons, std::string title, std::string message="", std::function<void(DialogResult)> onClose=nullptr);

  ~MessageBox() override;

#if defined(DEBUG_WINDOWS)
  std::string getName() override
  {
    return "MessageBox";
  }
#endif

  void paint(BitmapBuffer * dc) override;

  void deleteLater();

  void checkEvents() override;

  void setMessage(std::string message) {
    this->message = message;
    invalidate();
  }
  void close(){
    running = false;
  }
  void setCloseCondition(std::function<DialogResult(void)> handler) {
    closeCondition = std::move(handler);
  }
  uint8_t getType(){
    return type;
  }
  void setUpdateMethod(std::function<void(void)> update);
protected:
  uint8_t type;
  std::string title;
  std::string message;
  const BitmapBuffer* icon;
  bool running;
  std::function<void(void)> onUpdate;
  std::function<void(DialogResult)> onClose;
  std::function<DialogResult(void)> closeCondition;
};

#endif // _CONFIRMATION_H_
