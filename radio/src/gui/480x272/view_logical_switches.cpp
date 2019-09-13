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
#define CSW_4TH_COLUMN                 170
#define CSW_5TH_COLUMN                 220
#define CSW_6TH_COLUMN                 280
#define MAX_LOGICAL_SWITCHES             64

void LSMonitorFooter::paint(BitmapBuffer * dc) {
  dc->clear(TITLE_BGCOLOR);
  if (selectedLS == -1)
    return;
  coord_t y = 2;
  LogicalSwitchData * cs = lswAddress(selectedLS);
  lcdDrawTextAtIndex(CSW_1ST_COLUMN, y, STR_VCSWFUNC, cs->func, TEXT_INVERTED_COLOR);
  // CSW params
  unsigned int cstate = lswFamily(cs->func);

  if (cstate == LS_FAMILY_BOOL || cstate == LS_FAMILY_STICKY) {
    drawSwitch(CSW_2ND_COLUMN, y, cs->v1, TEXT_INVERTED_COLOR);
    drawSwitch(CSW_3RD_COLUMN, y, cs->v2, TEXT_INVERTED_COLOR);
  } else if (cstate == LS_FAMILY_EDGE) {
    drawSwitch(CSW_2ND_COLUMN, y, cs->v1, TEXT_INVERTED_COLOR);
    //    putsEdgeDelayParam(CSW_3RD_COLUMN, y, cs, 0, 0);
  } else if (cstate == LS_FAMILY_COMP) {
    drawSource(CSW_2ND_COLUMN, y, cs->v1, TEXT_INVERTED_COLOR);
    drawSource(CSW_3RD_COLUMN, y, cs->v2, TEXT_INVERTED_COLOR);
  } else if (cstate == LS_FAMILY_TIMER) {
    lcdDrawNumber(CSW_2ND_COLUMN, y, lswTimerValue(cs->v1), LEFT | PREC1 | TEXT_INVERTED_COLOR);
    lcdDrawNumber(CSW_3RD_COLUMN, y, lswTimerValue(cs->v2), LEFT | PREC1 | TEXT_INVERTED_COLOR);
  } else {
    drawSource(CSW_2ND_COLUMN, y, cs->v1, TEXT_INVERTED_COLOR);
    drawSourceCustomValue(CSW_3RD_COLUMN, y, cs->v1,
        cs->v1 <= MIXSRC_LAST_CH ? calc100toRESX(cs->v2) : cs->v2, LEFT | TEXT_INVERTED_COLOR);
  }

  // CSW AND switch
  drawSwitch(CSW_4TH_COLUMN, y, cs->andsw, TEXT_INVERTED_COLOR);

  // CSW duration
  if (cs->duration > 0)
    lcdDrawNumber(CSW_5TH_COLUMN, y, cs->duration, PREC1 | LEFT | TEXT_INVERTED_COLOR);
  else
    lcdDrawMMM(CSW_5TH_COLUMN, y, TEXT_INVERTED_COLOR);

  // CSW delay
  if (cstate == LS_FAMILY_EDGE) {
    lcdDrawText(CSW_6TH_COLUMN, y, STR_NA, TEXT_INVERTED_COLOR);
  } else if (cs->delay > 0) {
    lcdDrawNumber(CSW_6TH_COLUMN, y, cs->delay, PREC1 | LEFT | TEXT_INVERTED_COLOR);
  } else {
    lcdDrawMMM(CSW_6TH_COLUMN, y, TEXT_INVERTED_COLOR);
  }
}

bool LSMonitorBody::onTouchEnd(coord_t x, coord_t y) {
  bool result = Window::onTouchEnd(x, y);
  bool found = false;
  for (uint8_t i = 0; i < MAX_LOGICAL_SWITCHES; i++) {
    rect_t rect = { logicalSwitches[i]->left(), logicalSwitches[i]->top(),
        logicalSwitches[i]->width(), logicalSwitches[i]->height() };
    if (pointInRect(x, y, rect)) {
      lsFooter->setSelected(i);
      found = true;
    }
  }
  if (!found)
    lsFooter->setSelected(-1);
  return result || found;
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
    logicalSwitches[i] = new StaticText(this, grid.getFieldSlot(4, i % 4),
        std::string(lsString), attr);
    if (i % 4 == 3)
      grid.nextLine();
  }
  grid.nextLine();
  setInnerHeight(grid.getWindowHeight());
}

