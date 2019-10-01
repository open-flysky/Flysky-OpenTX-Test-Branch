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

void drawStringWithIndex(coord_t x, coord_t y, const char * str, int idx, LcdFlags flags, const char * prefix, const char * suffix)
{
  char s[64];
  char * tmp = (prefix ? strAppend(s, prefix) : s);
  tmp = strAppend(tmp, str);
  tmp = strAppendUnsigned(tmp, abs(idx));
  if (suffix)
    strAppend(tmp, suffix);
  lcdDrawText(x, y, s, flags);
}

void drawValueWithUnit(coord_t x, coord_t y, int32_t val, uint8_t unit, LcdFlags att)
{
  // convertUnit(val, unit);
  if (!(att & NO_UNIT) && unit != UNIT_RAW) {
    char unitStr[8];
    strAppend(unitStr, STR_VTELEMUNIT+1+unit*STR_VTELEMUNIT[0], STR_VTELEMUNIT[0]);
    lcdDrawNumber(x, y, val, att, 0, NULL, unitStr);
  }
  else {
    lcdDrawNumber(x, y, val, att);
  }
}

int editChoice(coord_t x, coord_t y, const char * values, int value, int min, int max, LcdFlags attr, event_t event)
{
  if (attr & INVERS) value = checkIncDec(event, value, min, max, (menuVerticalPositions[0] == 0) ? EE_MODEL : EE_GENERAL);
  if (values) lcdDrawTextAtIndex(x, y, values, value-min, attr);
  return value;
}

uint8_t editCheckBox(uint8_t value, coord_t x, coord_t y, LcdFlags attr, event_t event )
{
  value = editChoice(x, y, NULL, value, 0, 1, attr, event);
  drawCheckBox(x, y, value, attr);
  return value;
}

swsrc_t editSwitch(coord_t x, coord_t y, swsrc_t value, LcdFlags attr, event_t event)
{
  if (attr & INVERS) CHECK_INCDEC_MODELSWITCH(event, value, SWSRC_FIRST_IN_MIXES, SWSRC_LAST_IN_MIXES, isSwitchAvailableInMixes);
  drawSwitch(x, y, value, attr);
  return value;
}


void drawGVarValue(coord_t x, coord_t y, uint8_t gvar, gvar_t value, LcdFlags flags)
{
  uint8_t prec = g_model.gvars[gvar].prec;
  if (prec > 0) {
    flags |= (prec == 1 ? PREC1 : PREC2);
  }
  drawValueWithUnit(x, y, value, g_model.gvars[gvar].unit ? UNIT_PERCENT : UNIT_RAW, flags);
}

#if defined(GVARS)
int16_t displayGVar(coord_t x, coord_t y, int16_t value, int16_t min, int16_t max, LcdFlags attr)
{
  uint16_t delta = GV_GET_GV1_VALUE(max);
  TRACE("displayGVar(val=%d min=%d max=%d)", value, min, max);
  if (GV_IS_GV_VALUE(value, min, max)) {
    attr &= ~PREC1;

    int8_t idx = (int16_t) GV_INDEX_CALC_DELTA(value, delta);
    if (idx >= 0) ++idx;    // transform form idx=0=GV1 to idx=1=GV1 in order to handle double keys invert
    if (idx < 0) {
      value = (int16_t) GV_CALC_VALUE_IDX_NEG(idx, delta);
      idx = -idx;
      drawStringWithIndex(x, y, STR_GV, idx, attr, "-");
    }
    else {
      drawStringWithIndex(x, y, STR_GV, idx, attr);
      value = (int16_t) GV_CALC_VALUE_IDX_POS(idx-1, delta);
    }
  }
  else {
    lcdDrawNumber(x, y, value, attr, 0, NULL, "%");
  }
  return value;
}
#else
int16_t displayGVar(coord_t x, coord_t y, int16_t value, int16_t min, int16_t max, LcdFlags attr)
{
  lcdDrawNumber(x, y, value, attr);
  return value;
}
#endif

void drawFatalErrorScreen(const char * message)
{
  static uint32_t updateTime = 0;
  if(updateTime == 0 || ((get_tmr10ms() - updateTime) >= 10)) {
    updateTime = get_tmr10ms();
    lcdNextLayer();
    lcd->clear();
    lcd->drawSizedText(LCD_W/2, LCD_H/2-20, message, strlen(message), DBLSIZE|CENTERED|TEXT_BGCOLOR);
    lcdRefresh();
  }
}

void runFatalErrorScreen(const char * message)
{
  while (1) {
    BACKLIGHT_ENABLE();
    drawFatalErrorScreen(message);
    uint8_t refresh = false;
    while (1) {
      uint32_t pwr_check = pwrCheck();
      if (pwr_check == e_power_off) {
        boardOff();
      }
      else if (pwr_check == e_power_press) {
        refresh = true;
      }
      else if (pwr_check == e_power_on && refresh) {
        break;
      }
      SIMU_SLEEP_NORET(1);
    }
  }
}
