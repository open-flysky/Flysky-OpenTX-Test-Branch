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
#include "radio_trainer.h"
#include "libwindows.h"

#define SET_DIRTY()     storageDirty(EE_GENERAL)

#define TRAINER_COLUMN_WIDTH   60
#define TRAINER_COLUMN_1       MENUS_MARGIN_LEFT+100
#define TRAINER_COLUMN_2       TRAINER_COLUMN_1+TRAINER_COLUMN_WIDTH
#define TRAINER_COLUMN_3       TRAINER_COLUMN_2+TRAINER_COLUMN_WIDTH
#define PPM_CHANNELS_TRAINER   4
RadioTrainerPage::RadioTrainerPage():
  PageTab(STR_MENUTRAINER, ICON_RADIO_TRAINER)
{
}
void RadioTrainerPage::checkEvents()
{
  PageTab::checkEvents();
  
  if (SLAVE_MODE()) {return;}
  for (int i=0; i<PPM_CHANNELS_TRAINER; i++) {
    int32_t value = 0;
#if defined (PPM_UNIT_PERCENT_PREC1)
    value = (ppmInput[i]-g_eeGeneral.trainer.calib[i])*2;
#else
    value = (ppmInput[i]-g_eeGeneral.trainer.calib[i])/5;
#endif
    if(value != numEdits[i]->getValue()) {
      numEdits[i]->invalidate();
    }
  }
}
void RadioTrainerPage::build(Window * window)
{
  bool slave = SLAVE_MODE();
  GridLayout grid;
  grid.spacer(8);
  grid.setLabelWidth(LCD_W/2);
  if(slave) {
    new StaticText(window, grid.getLineSlot(), STR_SLAVE, CENTERED);
    return;
  }
  for (uint8_t i=0; i<NUM_STICKS; i++) {
    uint8_t channel = channel_order(i+1);
    TrainerMix* trainerMix = &g_eeGeneral.trainer.mix[channel-1];
    rect_t labelSlot = grid.getLabelSlot();
    new StaticText(window, labelSlot, getSourceString(channel));
    labelSlot.x += 60;
    labelSlot.w -= 60 + lineSpacing;
    new Choice(window, labelSlot, STR_TRNMODE, 0, 2, GET_SET_DEFAULT(trainerMix->mode));
    auto weight = new NumberEdit(window, grid.getFieldSlot(2, 0), -125, 125, GET_SET_DEFAULT(trainerMix->studWeight));
    weight->setSuffix("%");
    new Choice(window, grid.getFieldSlot(2, 1), STR_TRNCHN, 0, 3, GET_SET_DEFAULT(trainerMix->srcChn));
    grid.nextLine();
  }
  new StaticText(window, grid.getLabelSlot(), STR_MULTIPLIER, 0);
  new NumberEdit(window, grid.getFieldSlot(), -10, 40, GET_SET_WITH_OFFSET(g_eeGeneral.PPM_Multiplier, 10), PREC1);
  grid.nextLine();

  for (int i=0; i<PPM_CHANNELS_TRAINER; i++) {
    NumberEdit* numEdit = nullptr;
    rect_t targetSlot = grid.getLineSlot();
    targetSlot.x += i * (targetSlot.w /4);
    targetSlot.w = (targetSlot.w /4) - lineSpacing;
    if(i > 1) targetSlot = grid.getFieldSlot(2, i-2);
 #if defined (PPM_UNIT_PERCENT_PREC1)
    numEdit = new NumberEdit(window, targetSlot, -100, +100, [=] { return (ppmInput[i]-g_eeGeneral.trainer.calib[i])*2; }, [=](int32_t newValue) {  }, PREC1);
 #else
    numEdit = new NumberEdit(window, targetSlot, -100, +100, [=] { return (ppmInput[i]-g_eeGeneral.trainer.calib[i])/5; }, [=](int32_t newValue) {  });
 #endif
    numEdit->setReadonly(true);
    numEdits[i] = numEdit;
  }
  grid.nextLine();
  new TextButton(window, grid.getLineSlot(), "Calibrate...", [=]() -> uint8_t {
    memcpy(g_eeGeneral.trainer.calib, ppmInput, sizeof(g_eeGeneral.trainer.calib));
    storageDirty(EE_GENERAL);
    AUDIO_WARNING1();
    return 1;
  });
}

#if 0
bool menuRadioTrainer(event_t event)
{
  uint8_t y;
  bool slave = SLAVE_MODE();

  MENU(STR_MENUTRAINER, RADIO_ICONS, menuTabGeneral, MENU_RADIO_TRAINER, (slave ? 0 : 6), { NAVIGATION_LINE_BY_LINE|2, NAVIGATION_LINE_BY_LINE|2, NAVIGATION_LINE_BY_LINE|2, NAVIGATION_LINE_BY_LINE|2, 0, 0});

  if (slave) {
    lcdDrawText(LCD_W/2, 5*FH, STR_SLAVE, |TEXT_COLOR);
    return true;
  }

  LcdFlags attr;
  LcdFlags blink = ((s_editMode>0) ? BLINK|INVERS : INVERS);

  /* lcdDrawText(TRAINER_COLUMN_1, MENU_HEADER_HEIGHT+1, "Mode", HEADER_COLOR);
  lcdDrawText(TRAINER_COLUMN_2, MENU_HEADER_HEIGHT+1, "Weight", HEADER_COLOR);
  lcdDrawText(TRAINER_COLUMN_3, MENU_HEADER_HEIGHT+1, "Source", HEADER_COLOR);
  */

  y = MENU_CONTENT_TOP + FH;

  for (uint8_t i=0; i<NUM_STICKS; i++) {
    uint8_t chan = channel_order(i+1);
    TrainerMix * td = &g_eeGeneral.trainer.mix[chan-1];

    drawSource(MENUS_MARGIN_LEFT, y, MIXSRC_Rud-1+chan, ((menuVerticalPosition==i && CURSOR_ON_LINE()) ? INVERS : 0));

    for (int j=0; j<3; j++) {

      attr = ((menuVerticalPosition==i && menuHorizontalPosition==j) ? blink : 0);

      switch (j) {
        case 0:
          lcdDrawTextAtIndex(TRAINER_COLUMN_1, y, STR_TRNMODE, td->mode, attr);
          if (attr&BLINK) CHECK_INCDEC_GENVAR(event, td->mode, 0, 2);
          break;

        case 1:
          lcdDrawNumber(TRAINER_COLUMN_2, y, td->studWeight, LEFT|attr, 0, NULL, "%");
          if (attr&BLINK) CHECK_INCDEC_GENVAR(event, td->studWeight, -125, 125);
          break;

        case 2:
          lcdDrawTextAtIndex(TRAINER_COLUMN_3, y, STR_TRNCHN, td->srcChn, attr);
          if (attr&BLINK) CHECK_INCDEC_GENVAR(event, td->srcChn, 0, 3);
          break;
      }
    }
    y += FH;
  }

  attr = (menuVerticalPosition==4) ? blink : 0;
  lcdDrawText(MENUS_MARGIN_LEFT, MENU_CONTENT_TOP + 5*FH, STR_MULTIPLIER);
  lcdDrawNumber(TRAINER_COLUMN_1, MENU_CONTENT_TOP + 5*FH, g_eeGeneral.PPM_Multiplier+10, LEFT|attr|PREC1);
  if (attr) CHECK_INCDEC_GENVAR(event, g_eeGeneral.PPM_Multiplier, -10, 40);

  attr = (menuVerticalPosition==5) ? INVERS : 0;
  if (attr) s_editMode = 0;
  lcdDrawText(MENUS_MARGIN_LEFT, MENU_CONTENT_TOP + 6*FH, STR_CAL, attr);
  for (int i=0; i<4; i++) {
#if defined (PPM_UNIT_PERCENT_PREC1)
    lcdDrawNumber(TRAINER_COLUMN_1+i*TRAINER_COLUMN_WIDTH, MENU_CONTENT_TOP + 6*FH, (ppmInput[i]-g_eeGeneral.trainer.calib[i])*2, LEFT|PREC1);
#else
    lcdDrawNumber(TRAINER_COLUMN_1+i*TRAINER_COLUMN_WIDTH, MENU_CONTENT_TOP + 6*FH, (ppmInput[i]-g_eeGeneral.trainer.calib[i])/5, LEFT);
#endif
  }

  if (attr) {
    if (event==EVT_KEY_LONG(KEY_ENTER)){
      memcpy(g_eeGeneral.trainer.calib, ppmInput, sizeof(g_eeGeneral.trainer.calib));
      storageDirty(EE_GENERAL);
      AUDIO_WARNING1();
    }
  }

  return true;
}
#endif
