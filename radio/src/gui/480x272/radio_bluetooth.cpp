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
#include "radio_bluetooth.h"
#include "libwindows.h"
#include "bluetoothle.h"

#define SET_DIRTY()     storageDirty(EE_GENERAL)

RadioBluetoothPage::RadioBluetoothPage():
  PageTab(STR_BLUETOOTH, ICON_RADIO_BT)
{
  strcpy(reusableBuffer.bt.pin, "00000");
  strcpy(reusableBuffer.bt.messageStatus, "...");
  
}
void RadioBluetoothPage::checkEvents()
{
  PageTab::checkEvents();
  bluetooth.getStatus(reusableBuffer.bt.messageStatus, sizeof(reusableBuffer.bt.messageStatus));
  if(bluetoothBaudrate && lastBaudrate != g_eeGeneral.bluetoothBaudrate) {
    lastBaudrate = g_eeGeneral.bluetoothBaudrate;
    bluetoothBaudrate->invalidate();
  }
  // if (SLAVE_MODE()) {return;}
  // for (int i=0; i<PPM_CHANNELS_TRAINER; i++) {
  //   int32_t value = numEdits[i]->getValue();
  //   if(value != last_values[i]) {
  //     numEdits[i]->invalidate();
  //   }
  //   last_values[i] = value;
  // }
}
void RadioBluetoothPage::build(Window * window)
{
  GridLayout grid;
  grid.spacer(8);

  new StaticText(window, grid.getLabelSlot(), STR_MODE);
  new Choice(window, grid.getFieldSlot(), STR_BLUETOOTH_MODES, BLUETOOTH_OFF, BLUETOOTH_SERVER, [=] { return g_eeGeneral.bluetoothMode; },  [=](int32_t newValue) { 
    g_eeGeneral.bluetoothMode = newValue;
    SET_DIRTY(); 
    if ((BluetoothModes)newValue == BLUETOOTH_OFF) {
      bluetooth.stop();
    } else {
      bluetooth.start();
    }
  });
  grid.nextLine();
  StaticText* status = new StaticText(window, grid.getFieldSlot(), reusableBuffer.bt.messageStatus);
  status->setCheckHandler([=]() {
    if(!status->isTextEqual(reusableBuffer.bt.messageStatus)) {
      status->setText(std::string(reusableBuffer.bt.messageStatus));
    }
  });
  grid.nextLine();
  new StaticText(window, grid.getLabelSlot(), STR_NAME);
  new TextEdit(window, grid.getFieldSlot(), bluetooth.config.name, LEN_BLUETOOTH_NAME, 0, [=](char* newValue) {
    bluetooth.config.dirty = true;
	});
  grid.nextLine();

  new StaticText(window, grid.getLabelSlot(), "MAC");
  new StaticText(window, grid.getFieldSlot(), bluetooth.config.mac);
 
  grid.nextLine();
 
  new StaticText(window, grid.getLabelSlot(), STR_BLUETOOTH_PIN_CODE);
  auto slotCheck = grid.getFieldSlot();
  new CheckBox(window, slotCheck, isPasscodeEnabled, setPasscodeEnabled);
  sprintf(reusableBuffer.bt.pin, "%05lu", getPasscode());
  rect_t slotText = rect_t {
   slotCheck.x + 30, slotCheck.y, slotCheck.w - 30, slotCheck.h
  };
	new TextEdit(window, slotText, reusableBuffer.bt.pin, LEN_BLUETOOTH_PIN, 0, [=](char* newValue) {
    uint32_t pin; 
    sscanf(newValue, "%lu", &pin); 
    setPasscode(pin);
	}, false);

  grid.nextLine();

  new StaticText(window, grid.getLabelSlot(), TR_BLUETOOTH_PLATFORM);
  new Choice(window, grid.getFieldSlot(), STR_BLUETOOTH_PLATFORMS, BLUETOOTH_TARGET_PLATFORM_FIRST, BLUETOOTH_TARGET_PLATFORM_LAST, 
    [=] { return static_cast<int>(getPlatfrom());}, 
    [=] (int32_t newValue) { setPlatform(static_cast<BLUETOOTH_TARGET_PLATFORM_TYPE>(newValue));
  });
  grid.nextLine();

  new StaticText(window, grid.getLabelSlot(), STR_BAUDRATE);
  bluetoothBaudrate = new Choice(window, grid.getFieldSlot(), STR_BLUETOOTH_BAUDRATE, 0, 9, [=] { return g_eeGeneral.bluetoothBaudrate; }, setBtBaudrate);

  grid.nextLine();

  new StaticText(window, grid.getLabelSlot(), STR_POWER);
  auto power = new Choice(window, grid.getFieldSlot(), "", btle::TxPower::power_first, btle::TxPower::power_last, [=] { return bluetooth.config.tx_power; }, setBtTxPower);
  power->setTextHandler([](uint8_t value) {
    return btle::tx_power_map[value];
  });
}
