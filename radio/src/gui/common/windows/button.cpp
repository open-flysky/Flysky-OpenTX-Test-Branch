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

#include "button.h"
#include "draw_functions.h"
#include "lcd.h"
#include "theme.h"
#include <algorithm>
bool Button::onTouchEnd(coord_t x, coord_t y)
{
  if (enabled()) {
    bool check = (onPress && onPress());
    if (check != bool(flags & BUTTON_CHECKED)) {
      flags ^= BUTTON_CHECKED;
      invalidate();
    }
    if (!(flags & BUTTON_NOFOCUS)) {
      setFocus();
    }
    AUDIO_KEY_PRESS();
    if(dialogResult != 0){
      flags |= BUTTON_RESULT;
      putEvent(dialogResult);
    }
  }
  return true;
}

event_t Button::getResult() {
  if((flags & BUTTON_RESULT) == BUTTON_RESULT) {
    return dialogResult;
  }
  return 0;
}

void Button::checkEvents()
{
  Window::checkEvents();
  if (onCheck)
    onCheck();
}

void TextButton::paint(BitmapBuffer * dc)
{
  if (checked()) {
    drawSolidRect(dc, 0, 0, rect.w, rect.h, 2, SCROLLBOX_COLOR);
    if (flags & BUTTON_BACKGROUND)
      dc->drawSolidFilledRect(2, 2, rect.w-4, rect.h-4, CURVE_AXIS_COLOR);
  }
  else {
    if (flags & BUTTON_BACKGROUND)
      dc->drawSolidFilledRect(0, 0, rect.w, rect.h, CURVE_AXIS_COLOR);
    else
      drawSolidRect(dc, 0, 0, rect.w, rect.h, 2, CURVE_AXIS_COLOR);
  }

  dc->drawText(rect.w / 2, (rect.h - getFontHeight(flags)) / 2, text.c_str(), CENTERED | (enabled() ? 0 : TEXT_DISABLE_COLOR));
}


const std::list<std::string> FileButton::fileTypeBin = std::list<std::string>({"bin", "hex", "frk"});
const std::list<std::string> FileButton::fileTypeAudio = std::list<std::string>({"wav", "mp3", "acc"});
const std::list<std::string> FileButton::fileTypeGfx = std::list<std::string>({"bmp", "jpeg", "jpg", "png", "gif"});
const std::list<std::string> FileButton::fileTypeTxt = std::list<std::string>({"txt", "doc", "rtf", "log", "html", "htm"});

void FileButton::setColorByExtension()
{
  size_t extIndex = text.rfind(".");
  if (extIndex == std::string::npos) return;
  std::string ext = text.substr(extIndex + 1);

  if (std::find(FileButton::fileTypeBin.begin(), FileButton::fileTypeBin.end(), ext) != FileButton::fileTypeBin.end()) {
    lcdSetColor(RGB(0, 166, 73));
  } else if (std::find(FileButton::fileTypeAudio.begin(), FileButton::fileTypeAudio.end(), ext) != FileButton::fileTypeAudio.end()) {
    lcdSetColor(RGB(203, 47, 41));
  } else if (std::find(FileButton::fileTypeGfx.begin(), FileButton::fileTypeGfx.end(), ext) != FileButton::fileTypeGfx.end()) {
    lcdSetColor(RGB(5, 118, 192));
  } else if (std::find(FileButton::fileTypeTxt.begin(), FileButton::fileTypeTxt.end(), ext) != FileButton::fileTypeTxt.end()) {
    lcdSetColor(RGB(223, 107, 0));
  } else {
    lcdSetColor(RGB(0, 129, 163));
  }
}

void fitToRect(char* t, uint16_t w, LcdFlags flags, bool addDots) {
  while(getTextWidth(t, strlen(t), flags) > w) {
    int last = strlen(t)-1;
    if (addDots) {
      if (last == 2) return;
      t[last-3] = '.';
      t[last-2] = '.';
      t[last-1] = '.';
    }
    t[last] = 0;
  }
}
void FileButton::paint(BitmapBuffer * dc)
{
  coord_t bottom = rect.h / 4;
  bottom = rect.h - bottom;
  const char* t = text.c_str();
  char displayText[32];
  strncpy(displayText, t, sizeof(displayText));
  int lastChar = sizeof(displayText) -1;
  if (displayText[lastChar] != 0) displayText[lastChar] = 0;

  LcdFlags textFlags = TINSIZE;
  fitToRect(displayText, rect.w, textFlags, true);
  if (folder) {
    if(!text.compare("..")) {
       lcdDrawBitmapPattern(0, 0, LBM_UNDO, CENTERED);
    } else {
      lcdSetColor(RGB(255, 209, 85));
      lcdDrawBitmapPattern(0, 0, !text.compare("..") ? LBM_UNDO : LBM_BIG_FOLDER, CUSTOM_COLOR | CENTERED);
      dc->drawText(0, bottom, displayText, textFlags);
    }
  }
  else {
    coord_t iconWidth = (rect.w * 2) / 3;
    coord_t margin = (rect.w - iconWidth) / 2;
    lcdSetColor(RGB(226, 229, 231));
    dc->drawSolidFilledRectRadius(margin, 0, rect.w - (margin*2), bottom, rect.w / 10, CUSTOM_COLOR);
    coord_t h = rect.h / 5;
    coord_t w = (iconWidth * 2) / 3;
    coord_t top = (rect.h / 2) - (h / 2);
    coord_t marginFileName = margin * 3 / 5;
    
    size_t extIndex = text.rfind(".");
    if (extIndex != std::string::npos) {
      displayText[extIndex] = 0;
    }
    dc->drawText(0, bottom, displayText, textFlags);
    if (extIndex != std::string::npos) {
      strncpy(displayText, text.substr(extIndex + 1).c_str(), sizeof(displayText));
      setColorByExtension();
      dc->drawSolidFilledRectRadius(marginFileName, top, w, h, h / 4, CUSTOM_COLOR);
      fitToRect(displayText, w, textFlags, false);
      lcdSetColor(RGB(255, 255, 255));
      dc->drawText(marginFileName + w / 8, top, displayText, textFlags | CUSTOM_COLOR);
    } 
  }
}



#include "alpha_button_on.lbm"
#include "alpha_button_off.lbm"

void IconButton::paint(BitmapBuffer * dc)
{
  dc->drawBitmap(0, 0, theme->getIconBitmap(icon, checked()));
}

FabIconButton::FabIconButton(Window * parent, coord_t x, coord_t y, uint8_t icon, std::function<uint8_t(void)> onPress, uint8_t flags):
  Button(parent, { x - 34, y - 34, 68, 68 }, onPress, flags),
  icon(icon)
{
}

void FabIconButton::paint(BitmapBuffer * dc)
{
  dc->drawBitmap(0, 0, checked() ? &ALPHA_BUTTON_ON : &ALPHA_BUTTON_OFF);
  const BitmapBuffer * mask = theme->getIconMask(icon);
  if(mask) dc->drawMask((68-mask->getWidth())/2, (68-mask->getHeight())/2, mask, TEXT_BGCOLOR);
}
