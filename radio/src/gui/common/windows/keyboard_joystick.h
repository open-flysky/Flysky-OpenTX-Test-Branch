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

#ifndef _KEYBOARD_JOYSTICK_H_
#define _KEYBOARD_JOYSTICK_H_

#include "keyboard_base.h"
#include "joystick_target.h"

class Button;

class JoystickKeyboard : public Keyboard<JoystickTarget> {
  friend class JoystickTarget;

  public:
    JoystickKeyboard();
    ~JoystickKeyboard();

#if defined(DEBUG_WINDOWS)
    std::string getName() override
    {
      return "JoystickKeyboard";
    }
#endif

    static JoystickKeyboard * instance()
    {
      if (!_instance)
        _instance = new JoystickKeyboard();
      return _instance;
    } 

    void paint(BitmapBuffer * dc) override;

  protected:
    static JoystickKeyboard * _instance;
    Button * left = nullptr;
    Button * right = nullptr;
};

#endif // _KEYBOARD_JOYSTICK_H_
