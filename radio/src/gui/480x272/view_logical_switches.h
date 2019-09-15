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

#ifndef _VIEW_LOGICAL_SWITCHES_H_
#define _VIEW_LOGICAL_SWITCHES_H_

#include "opentx.h"
#include "libwindows.h"
#include "model_logical_switches.h"

class LSMonitorBody: public Window {
public:
  LSMonitorBody(Window * parent, const rect_t & rect) :
      Window(parent, rect) {
    logicalSwitches = new StaticText*[MAX_LOGICAL_SWITCHES];
    build();
  }
  ~LSMonitorBody() {
    delete[] logicalSwitches;
  }
  virtual bool onTouchEnd(coord_t x, coord_t y) override;
  virtual void checkEvents() override;
  void build();
  void setButton(LogicalSwitchButton* button) {
    lsButton = button;
  }
protected:
  StaticText** logicalSwitches;
  LogicalSwitchButton* lsButton;
};


class LogicalSwitchesMonitorPage: public PageTab {
public:
  LogicalSwitchesMonitorPage() :
      PageTab(STR_MONITOR_SWITCHES, ICON_MONITOR_LOGICAL_SWITCHES) {
  }

  virtual void build(Window * window) override
  {
    auto body = new LSMonitorBody(window,
        { 0, 0, LCD_W, window->height() - footerHeight });
    LogicalSwitchButton *button = new LogicalSwitchButton(window, { 0, window->height() - footerHeight, LCD_W, footerHeight }, -1, nullptr);
    body->setButton(button);
  }

protected:
  static constexpr coord_t footerHeight = 52;
};
#endif // _VIEW_CHANNELS_H_
