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

#include "radio_ghost_menu.h"
#include "opentx.h"
#include "libwindows.h"

#include "keyboard_joystick.h"

GhostConfig::GhostConfig(Window * parent, const rect_t & rect) : JoystickTarget(parent, rect) {}

void GhostConfig::doAction(uint8_t action, uint8_t menu) {
  reusableBuffer.ghostMenu.buttonAction = action;
  reusableBuffer.ghostMenu.menuAction = menu;
  moduleState[EXTERNAL_MODULE].counter = GHST_MENU_CONTROL;
}

void GhostConfig::up() {
  doAction(GHST_BTN_JOYUP, GHST_MENU_CTRL_NONE);
}
void GhostConfig::down() {
  doAction(GHST_BTN_JOYDOWN, GHST_MENU_CTRL_NONE);
}
void GhostConfig::right() {
  doAction(GHST_BTN_JOYPRESS, GHST_MENU_CTRL_NONE);
}
void GhostConfig::left() {
  doAction(GHST_BTN_JOYLEFT, GHST_MENU_CTRL_NONE);
}
void GhostConfig::enter() {
  doAction(GHST_BTN_JOYPRESS, GHST_MENU_CTRL_NONE);
}
void GhostConfig::escape() {
  doAction(GHST_BTN_JOYLEFT, GHST_MENU_CTRL_NONE);
}

void GhostConfig::checkEvents() {
  JoystickTarget::checkEvents();
  invalidate();
}

constexpr coord_t offset_ghost = 10;
constexpr coord_t lineSpacing_ghost = 25;

void GhostConfig::paint(BitmapBuffer * dc)
{
  dc->clear(TEXT_BGCOLOR);

  LcdFlags flags = 0;

  bool valid = false;
  for (uint8_t line = 0; line < GHST_MENU_LINES; line++) {
    flags = 0;
    valid |= strlen(reusableBuffer.ghostMenu.line[line].menuText) > 0;
    if (reusableBuffer.ghostMenu.line[line].splitLine) {
      if (reusableBuffer.ghostMenu.line[line].lineFlags & GHST_LINE_FLAGS_LABEL_SELECT)
        flags = INVERS;
      dc->drawText(offset_ghost, offset_ghost + line * lineSpacing_ghost, reusableBuffer.ghostMenu.line[line].menuText, flags);
      flags = 0;
      if (reusableBuffer.ghostMenu.line[line].lineFlags & GHST_LINE_FLAGS_VALUE_SELECT)
        flags |= INVERS;
      if (reusableBuffer.ghostMenu.line[line].lineFlags & GHST_LINE_FLAGS_VALUE_EDIT)
        flags |= BLINK;
      dc->drawText(width()/2, offset_ghost + line * lineSpacing_ghost, &reusableBuffer.ghostMenu.line[line].menuText[reusableBuffer.ghostMenu.line[line].splitLine], flags);
    }
    else {
      if (reusableBuffer.ghostMenu.line[line].lineFlags & GHST_LINE_FLAGS_LABEL_SELECT) {
        dc->drawText(offset_ghost, offset_ghost + line * lineSpacing_ghost, reusableBuffer.ghostMenu.line[line].menuText, INVERS);
      }
      else if (reusableBuffer.ghostMenu.line[line].lineFlags & GHST_LINE_FLAGS_VALUE_EDIT) {
        if (BLINK_ON_PHASE) {
          dc->drawText(offset_ghost, offset_ghost + line * lineSpacing_ghost, reusableBuffer.ghostMenu.line[line].menuText, flags);
        }
      }
      else {
        dc->drawText(offset_ghost, offset_ghost + line * lineSpacing_ghost, reusableBuffer.ghostMenu.line[line].menuText, flags);
      }
    }
  }
  if (!valid) {
    dc->drawText(offset_ghost, offset_ghost, "Waiting for module...", flags);
    doAction(GHST_BTN_NONE, GHST_MENU_CTRL_OPEN);
  }
}

GhostMenuPage::GhostMenuPage():
  PageTab(STR_GHOST_MENU_LABEL, ICON_RADIO_SPECTRUM_ANALYSER)
{
  memclear(&reusableBuffer.ghostMenu, sizeof(reusableBuffer.ghostMenu));
  reusableBuffer.ghostMenu.buttonAction = GHST_BTN_NONE;
  reusableBuffer.ghostMenu.menuAction = GHST_MENU_CTRL_OPEN;
  moduleState[EXTERNAL_MODULE].counter = GHST_MENU_CONTROL;
}

void GhostMenuPage::build(Window * window)
{
  GridLayout grid;
  grid.spacer(8);
  target = window;
  coord_t h = offset_ghost + GHST_MENU_LINES * lineSpacing_ghost;
  GhostConfig* config = new GhostConfig(window, { 0, 0, grid.getLineSlot().w, h});
  JoystickKeyboard * keyboard = JoystickKeyboard::instance();
  if (keyboard->getField() != config) {
    keyboard->setField(config);

  }
  window->adjustInnerHeight();
  //window->setHeight(h);
}

void GhostMenuPage::leave() {
  JoystickKeyboard::instance()->disable(true);
  memclear(&reusableBuffer.ghostMenu, sizeof(reusableBuffer.ghostMenu));
  reusableBuffer.ghostMenu.buttonAction = GHST_BTN_NONE;
  reusableBuffer.ghostMenu.menuAction = GHST_MENU_CTRL_CLOSE;
  moduleState[EXTERNAL_MODULE].counter = GHST_MENU_CONTROL;
  RTOS_WAIT_MS(10);
}


GhostMenu::GhostMenu():
  TabsGroup()
{
  addTab(new GhostMenuPage());
}
