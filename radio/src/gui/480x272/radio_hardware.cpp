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

#include "radio_hardware.h"
#include "opentx.h"
#include "libwindows.h"

#define SET_DIRTY() storageDirty(EE_GENERAL)

#define SWITCH_TYPE_MAX(sw)            ((MIXSRC_SB-MIXSRC_FIRST_SWITCH == sw || MIXSRC_SF-MIXSRC_FIRST_SWITCH == sw || MIXSRC_SG-MIXSRC_FIRST_SWITCH == sw) ? SWITCH_3POS : SWITCH_2POS)

class SwitchDynamicLabel : public StaticText {
  public:
    SwitchDynamicLabel(Window * parent, const rect_t & rect, uint8_t index):
      StaticText(parent, rect),
      index(index)
    {
      update();
    }

    std::string label()
    {
      return TEXT_AT_INDEX(STR_VSRCRAW, (index + MIXSRC_FIRST_SWITCH - MIXSRC_Rud + 1)) + std::string(&"\300-\301"[lastpos], 1);
    }

    void update()
    {
      uint8_t newpos = position();
      if (newpos != lastpos) {
        lastpos = newpos;
        setText(label());
      }
    }

    uint8_t position()
    {
      auto value = getValue(MIXSRC_FIRST_SWITCH + index);
      if (value > 0)
        return 2;
      else if (value < 0)
        return 0;
      else
        return 1;
    }

    void checkEvents() override
    {
      update();
    }

  protected:
    uint8_t index;
    uint8_t lastpos = 0xff;
};

class BatteryNumberEdit : public NumberEdit {
public:
  BatteryNumberEdit(Window * parent, const rect_t & rect):
    NumberEdit(parent, rect, -127, 127, GET_SET_DEFAULT(g_eeGeneral.txVoltageCalibration)) {
    setDisplayHandler([=](BitmapBuffer * dc, LcdFlags flags, int32_t value) {
        lastBatteryVoltage = getBatteryVoltage();
        drawNumber(dc, 2, 2, lastBatteryVoltage, flags | PREC2, 0, nullptr, "V");
    });
  }
  void checkEvents() override
  {
    NumberEdit::checkEvents();
    if(lastBatteryVoltage != getBatteryVoltage()) invalidate();
  }
protected:
  uint16_t lastBatteryVoltage = 0;
};

RadioHardwarePage::RadioHardwarePage():
  PageTab(STR_HARDWARE, ICON_RADIO_HARDWARE)
{
}


void RadioHardwarePage::build(Window * window)
{
  GridLayout grid;
  grid.spacer(8);
  grid.setLabelWidth(90);

  // Sticks
  new Subtitle(window, grid.getLineSlot(), STR_STICKS);
  grid.nextLine();
  for(int i=0; i < NUM_STICKS; i++){
    new StaticText(window, grid.getLabelSlot(true), TEXT_AT_INDEX(STR_VSRCRAW, (i + 1)));
    new TextEdit(window, grid.getFieldSlot(2,0), g_eeGeneral.anaNames[i], LEN_ANA_NAME);
    grid.nextLine();
  }

  // Pots
  new Subtitle(window, grid.getLineSlot(), STR_POTS);
  grid.nextLine();
  for(int i=0; i < NUM_POTS; i++){
    new StaticText(window, grid.getLabelSlot(true), TEXT_AT_INDEX(STR_VSRCRAW, (i + NUM_STICKS + 1)));
    new TextEdit(window, grid.getFieldSlot(2,0), g_eeGeneral.anaNames[i + NUM_STICKS], LEN_ANA_NAME);
    new Choice(window, grid.getFieldSlot(2,1), STR_POTTYPES, POT_NONE, POT_WITHOUT_DETENT, GET_SET_BF(g_eeGeneral.potsConfig, 2 * i, 2));
    grid.nextLine();
  }

  // Switches
  new Subtitle(window, grid.getLineSlot(), STR_SWITCHES);
  grid.nextLine();
  for(int i=0; i < NUM_SWITCHES; i++) {
    new SwitchDynamicLabel(window, grid.getLabelSlot(true), i);
    new TextEdit(window, grid.getFieldSlot(2, 0), g_eeGeneral.switchNames[i], LEN_SWITCH_NAME);
    new Choice(window, grid.getFieldSlot(2, 1), STR_SWTYPES, SWITCH_NONE, SWITCH_TYPE_MAX(i), GET_SET_BF(g_eeGeneral.switchConfig, 2 * i, 2));
    grid.nextLine();
  }

  grid.setLabelWidth(150);

  // ADC filter
  new StaticText(window, grid.getLabelSlot(), STR_JITTER_FILTER);
  new CheckBox(window, grid.getFieldSlot(), GET_SET_INVERTED(g_eeGeneral.jitterFilter));
  grid.nextLine();

#if SPORT_MAX_BAUDRATE < 400000
  new StaticText(window, grid.getLabelSlot(), STR_MAXBAUDRATE);
  auto baudrate = new Choice(window, grid.getFieldSlot(), "\013 400000bps\0 115200bps", 0, DIM(CROSSFIRE_BAUDRATES)-1, GET_DEFAULT(g_eeGeneral.telemetryBaudrate), [=](int32_t newValue) { 
    g_eeGeneral.telemetryBaudrate = newValue; 
    SET_DIRTY(); 
    if (IS_EXTERNAL_MODULE_ON()) {
      pauseMixerCalculations();
      pausePulses();
      EXTERNAL_MODULE_OFF();
      RTOS_WAIT_MS(20); // 20ms so that the pulses interrupt will reinit the frame rate
      telemetryProtocol = 255; // force telemetry port + module reinitialization
      EXTERNAL_MODULE_ON();
      resumePulses();
      resumeMixerCalculations();
    }
  });
  grid.nextLine();
#endif

#if defined(STICK_DEAD_ZONE)
  new StaticText(window, grid.getLabelSlot(), STR_DEAD_ZONE);
  auto deadZone = new NumberEdit(window, grid.getFieldSlot(), 0, 7, GET_SET_DEFAULT(g_eeGeneral.stickDeadZone));
  deadZone->setDisplayHandler([](BitmapBuffer * dc, LcdFlags flags, int32_t value) {
    drawNumber(dc, 2, Y_ENLARGEABLE, value ? 2 << (value -1) : 0, flags);
  });
  deadZone->setDefault(2);
  grid.nextLine();
#endif
  // Bat calibration
  new StaticText(window, grid.getLabelSlot(), STR_BATT_CALIB);
  new BatteryNumberEdit(window, grid.getFieldSlot());
  window->invalidate();
  grid.nextLine();
  grid.nextLine();

  window->setInnerHeight(grid.getWindowHeight());
}
