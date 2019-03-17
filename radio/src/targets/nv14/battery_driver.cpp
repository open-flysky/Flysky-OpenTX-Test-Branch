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

#define  __BATTERY_DRIVER_C__

#define BATTERY_W 140
#define BATTERY_H 320
#define BATTERY_TOP ((LCD_H - BATTERY_H)/2)
#define BATTERY_CONNECTOR_W 32
#define BATTERY_CONNECTOR_H 10
#define BATTERY_BORDER 4
#define BATTERY_W_INNER (BATTERY_W - 2*BATTERY_BORDER)
#define BATTERY_H_INNER (BATTERY_H - 2*BATTERY_BORDER)
#define BATTERY_TOP_INNER (BATTERY_TOP + BATTERY_BORDER)

void battery_charge_init()
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = PWR_CHARGE_FINISHED_GPIO_PIN | PWR_CHARGING_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(PWR_CHARGING_GPIO, &GPIO_InitStructure);
  GPIO_SetBits(PWR_CHARGING_GPIO, PWR_CHARGE_FINISHED_GPIO_PIN | PWR_CHARGING_GPIO_PIN);
}

uint16_t get_battery_charge_state()
{
  if (!READ_CHARGE_FINISHED_STATE())
  {
      return CHARGE_FINISHED;
  }
  else if (!READ_CHARGING_STATE())
  {
      return CHARGE_STARTED;
  }

  return CHARGE_NONE;
}

void drawChargingInfo(uint16_t chargeState){
	static int progress = 0;
	const char* text = chargeState == CHARGE_STARTED ? STR_BATTERYCHARGING : STR_BATTERYFULL;
	lcd->drawSizedText(LCD_W/2, LCD_H-50, text, strlen(text), CENTERED|TEXT_BGCOLOR);
	lcd->drawFilledRect((LCD_W - BATTERY_W)/2, BATTERY_TOP, BATTERY_W, BATTERY_H, SOLID, ROUND|TEXT_BGCOLOR);
	lcd->drawFilledRect((LCD_W - BATTERY_W_INNER)/2, BATTERY_TOP_INNER, BATTERY_W_INNER, BATTERY_H_INNER, SOLID, ROUND|TEXT_COLOR);
	int h = chargeState == CHARGE_STARTED ? ((BATTERY_H_INNER * progress) / 100) : BATTERY_H_INNER;
	lcd->drawFilledRect((LCD_W - BATTERY_W_INNER)/2, BATTERY_TOP_INNER + BATTERY_H_INNER - h , BATTERY_W_INNER, h, SOLID, ROUND|BATTERY_CHARGE_COLOR);
	lcd->drawFilledRect((LCD_W - BATTERY_CONNECTOR_W)/2, BATTERY_TOP-BATTERY_CONNECTOR_H , BATTERY_CONNECTOR_W, BATTERY_CONNECTOR_H, SOLID, TEXT_BGCOLOR);
	if(progress >= 100) progress = 0;
	else progress+=25;
}

//this method should be called by timer interrupt or by GPIO interrupt
void handle_battery_charge()
{
#if !defined(SIMU)
  static uint16_t chargeState = CHARGE_NONE;
  static uint32_t updateTime = 0;
  if(boardState != BOARD_POWER_OFF) return;
  if(get_battery_charge_state() == CHARGE_NONE)
  {
    if(chargeState != CHARGE_NONE)
    {
    	BACKLIGHT_DISABLE();
    	lcd->drawFilledRect(0, 0, LCD_WIDTH, LCD_HEIGHT, SOLID, HEADER_BGCOLOR);
    	lcdRefresh();
    }
  }
  else
  {
    if(chargeState == CHARGE_NONE)
    {
    	backlightInit();
    	lcdInit();
    }
    if(updateTime == 0 || ((get_tmr10ms() - updateTime) >= 50)) {
      updateTime = get_tmr10ms();
      BACKLIGHT_ENABLE();
      lcdNextLayer();
      lcd->clear();
      drawChargingInfo(get_battery_charge_state());
      lcdRefresh();
    }
  }
  chargeState = get_battery_charge_state();
#endif
}

uint16_t getBatteryVoltage()
{
  int32_t instant_vbat = anaIn(TX_VOLTAGE);  // using filtered ADC value on purpose
  return (uint16_t)((instant_vbat * (1000 + g_eeGeneral.txVoltageCalibration)) / 2942);
}
