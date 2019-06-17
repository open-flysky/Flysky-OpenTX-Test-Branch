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

#include "opentx.h"
#include "dialog.h"

const char *warningText = NULL;
const char *warningInfoText;
uint8_t     warningInfoLength;
uint8_t     warningType;
uint8_t     warningResult = 0;
uint8_t     warningInfoFlags = ZCHAR;
int16_t     warningInputValue;
int16_t     warningInputValueMin;
int16_t     warningInputValueMax;
void        (*popupFunc)(event_t event) = NULL;
const char *popupMenuItems[POPUP_MENU_MAX_LINES];
uint8_t     s_menu_item = 0;
uint16_t    popupMenuNoItems = 0;
uint16_t    popupMenuOffset = 0;
uint8_t     popupMenuOffsetType = MENU_OFFSET_INTERNAL;
void        (*popupMenuHandler)(const char * result);

#define ALERT_FRAME_TOP           70
#define ALERT_FRAME_PADDING       10
#define ALERT_BITMAP_PADDING      15
#define ALERT_TITLE_LEFT          135
#define ALERT_TITLE_LINE_HEIGHT   30
#define ALERT_MESSAGE_TOP         210
#define ALERT_ACTION_TOP          230
#define ALERT_BUTTON_TOP          300

void runPopupWarningBox()
{
}

void drawMessageBox()
{
}

void drawAlertBox(const char * title, const char * text, const char * action)
{
  theme->drawBackground();
  lcd->drawBitmap(ALERT_BITMAP_PADDING, ALERT_FRAME_TOP + ALERT_BITMAP_PADDING, theme->asterisk);

  #if defined(TRANSLATIONS_FR) || defined(TRANSLATIONS_IT) || defined(TRANSLATIONS_CZ)
  lcd->drawText(ALERT_TITLE_LEFT, ALERT_FRAME_TOP + ALERT_FRAME_PADDING, STR_WARNING, ALARM_COLOR|DBLSIZE);
  lcd->drawText(ALERT_TITLE_LEFT, ALERT_FRAME_TOP + ALERT_FRAME_PADDING + ALERT_TITLE_LINE_HEIGHT, title, ALARM_COLOR|DBLSIZE);
  #else
  if(title != nullptr) lcd->drawText(ALERT_TITLE_LEFT, ALERT_FRAME_TOP + ALERT_FRAME_PADDING, title, ALARM_COLOR|DBLSIZE);
  lcd->drawText(ALERT_TITLE_LEFT, ALERT_FRAME_TOP + ALERT_FRAME_PADDING + ALERT_TITLE_LINE_HEIGHT, STR_WARNING, ALARM_COLOR|DBLSIZE);
  #endif

  if(text != nullptr) lcd->drawText(ALERT_FRAME_PADDING+5, ALERT_MESSAGE_TOP, text, MIDSIZE);
}

void showAlertBox(const char * title, const char * text, const char * action, uint8_t sound)
{
}

void showMessageBox(const char * title)
{
  // drawMessageBox();
  lcdDrawSizedText(WARNING_LINE_X, WARNING_LINE_Y, title, WARNING_LINE_LEN, DBLSIZE|WARNING_COLOR);
  lcdRefresh();
}

void runPopupWarning(event_t event)
{
}

const char * runPopupMenu(event_t event)
{
}
