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
#include "view_logical_switches.h"

#define X_OFF                          45
#define Y_OFF                          70
#define HLINE_Y_OFF                    215
#define LS_COL_WIDTH                   50
#define LS_LINE_HEIGHT                 17
#define X_FUNC                         50
#define Y_FUNC                         225

#define CSW_1ST_COLUMN                 20
#define CSW_2ND_COLUMN                 70
#define CSW_3RD_COLUMN                 120
#define CSW_4TH_COLUMN                 150
#define CSW_5TH_COLUMN                 220
#define CSW_6TH_COLUMN                 280
#define MAX_LOGICAL_SWITCHES             64

bool LSMonitorBody::onTouchEnd(coord_t x, coord_t y) {
  bool result = Window::onTouchEnd(x, y);
  int16_t index = -1;
  for (uint8_t i = 0; i < MAX_LOGICAL_SWITCHES; i++) {
    rect_t rect = { logicalSwitches[i]->left(), logicalSwitches[i]->top(),
        logicalSwitches[i]->width(), logicalSwitches[i]->height() };
    if (pointInRect(x, y, rect)) {
      index = i;
      break;
    }
  }
  //set item as not defined and reset index to 0
  lsButton->setDefined(index >= 0);
  if(index < 0) index = 0;
  else {
    char lsString[] = "L64";
    strAppendSigned(&lsString[1], index + 1, 2);
    lsNameButton->setText(lsString);
  }
  lsButton->setLsIndex((uint8_t)index);

  return result || index !=-1;
}
void LSMonitorBody::checkEvents() {
  Window::checkEvents();
  for (uint8_t i = 0; i < MAX_LOGICAL_SWITCHES; i++) {
    LogicalSwitchData * cs = lswAddress(i);
    LcdFlags attr = CENTERED;
    if (cs->func == LS_FUNC_NONE)
      attr += CUSTOM_COLOR;
    else if (getSwitch(SWSRC_SW1 + i))
      attr += BOLD;
    logicalSwitches[i]->setFlags(attr);
  }
}
void LSMonitorBody::build() {
  lcdColorTable[CUSTOM_COLOR_INDEX] = RGB(160, 160, 160);
  GridLayout grid;
  grid.setMarginLeft(0);
  grid.setMarginRight(0);
  grid.spacer(10);
  grid.setLabelWidth(0);
  char lsString[] = "L64";

  for (uint8_t i = 0; i < MAX_LOGICAL_SWITCHES; i++) {
    LcdFlags attr = CENTERED;
    LogicalSwitchData * cs = lswAddress(i);
    strAppendSigned(&lsString[1], i + 1, 2);
    if (cs->func == LS_FUNC_NONE)
      attr += CUSTOM_COLOR;
    else if (getSwitch(SWSRC_SW1 + i))
      attr += BOLD;
    logicalSwitches[i] = new StaticText(this, grid.getFieldSlot(5, i % 5),
        std::string(lsString), attr);
    if (i % 5 == 4)
      grid.nextLine();
  }
  grid.nextLine();
  setInnerHeight(grid.getWindowHeight());
}

