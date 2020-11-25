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

#include "tabsgroup.h"
#include "libwindows.h"
#include "joystick_target.h"

class GhostConfig: public JoystickTarget {
  public:
    GhostConfig(Window * parent, const rect_t & rect);
    void paint(BitmapBuffer * dc) override;
    void checkEvents() override;
  protected:
    void up();
    void down();
    void right();
    void left();
    void enter();
    void escape();
    void doAction(uint8_t action, uint8_t menu);
};


class GhostMenuPage: public PageTab{
  public:
    GhostMenuPage();
    void build(Window * window) override;
    void leave() override; 
  private:
    Window* target;
};


class GhostMenu: public TabsGroup {
  public:
    GhostMenu();
};
