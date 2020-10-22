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

#if !defined(CPUARM)
uint8_t frskyTxBuffer[FRSKY_TX_PACKET_SIZE];
uint8_t frskyTxBufferCount = 0;
#endif

#if defined(TELEMETREZ)
#define PRIVATE         0x1B
uint8_t privateDataLen;
uint8_t privateDataPos;
#endif

#if defined(ROTARY_ENCODER_NAVIGATION) && defined(TELEMETREZ)
extern uint8_t TrotCount;
extern uint8_t TezRotary;
#endif

NOINLINE void processFrskyTelemetryData(uint8_t data)
{
  static uint8_t dataState = STATE_DATA_IDLE;

#if defined(PCBSKY9X) && defined(BLUETOOTH)
  // TODO if (g_model.bt_telemetry)
  btPushByte(data);
#endif

#if defined(AUX_SERIAL)
  if (g_eeGeneral.auxSerialMode == UART_MODE_TELEMETRY_MIRROR) {
    auxSerialPutc(data);
  }
#endif

#if defined(AUX2_SERIAL)
  if (g_eeGeneral.aux2SerialMode == UART_MODE_TELEMETRY_MIRROR) {
    aux2SerialPutc(data);
  }
#endif
if (pushFrskyTelemetryData(data)) {
    if (IS_FRSKY_SPORT_PROTOCOL()) {
      sportProcessTelemetryPacket(telemetryRxBuffer);
    }
    else {
      frskyDProcessPacket(telemetryRxBuffer);
    }
  }
}

#if defined(FRSKY_HUB) && !defined(CPUARM)
void frskyUpdateCells(void)
{
  // Voltage => Cell number + Cell voltage
  uint8_t battnumber = ((telemetryData.hub.volts & 0x00F0) >> 4);
  if (battnumber < 12) {
    if (telemetryData.hub.cellsCount < battnumber+1) {
      telemetryData.hub.cellsCount = battnumber+1;
    }
    uint8_t cellVolts = (uint8_t)(((((telemetryData.hub.volts & 0xFF00) >> 8) + ((telemetryData.hub.volts & 0x000F) << 8))) / 10);
    telemetryData.hub.cellVolts[battnumber] = cellVolts;
    if (!telemetryData.hub.minCellVolts || cellVolts<telemetryData.hub.minCellVolts || battnumber==telemetryData.hub.minCellIdx) {
      telemetryData.hub.minCellIdx = battnumber;
      telemetryData.hub.minCellVolts = cellVolts;
      if (!telemetryData.hub.minCell || telemetryData.hub.minCellVolts<telemetryData.hub.minCell)
        telemetryData.hub.minCell = telemetryData.hub.minCellVolts;
    }
  }
}
#endif

bool pushFrskyTelemetryData(uint8_t data)
{
  static uint8_t dataState = STATE_DATA_IDLE;

  switch (dataState) {
    case STATE_DATA_START:
      if (data == START_STOP) {
        if (IS_FRSKY_SPORT_PROTOCOL()) {
          dataState = STATE_DATA_IN_FRAME ;
          telemetryRxBufferCount = 0;
        }
      }
      else {
        if (telemetryRxBufferCount < TELEMETRY_RX_PACKET_SIZE) {
          telemetryRxBuffer[telemetryRxBufferCount++] = data;
        }
        dataState = STATE_DATA_IN_FRAME;
      }
      break;

    case STATE_DATA_IN_FRAME:
      if (data == BYTE_STUFF) {
        dataState = STATE_DATA_XOR; // XOR next byte
      }
      else if (data == START_STOP) {
        if (IS_FRSKY_SPORT_PROTOCOL()) {
          dataState = STATE_DATA_IN_FRAME ;
          telemetryRxBufferCount = 0;
        }
        else {
          // end of frame detected
          dataState = STATE_DATA_IDLE;
          return true;
        }
        break;
      }
      else if (telemetryRxBufferCount < TELEMETRY_RX_PACKET_SIZE) {
        telemetryRxBuffer[telemetryRxBufferCount++] = data;
      }
      break;

    case STATE_DATA_XOR:
      if (telemetryRxBufferCount < TELEMETRY_RX_PACKET_SIZE) {
        telemetryRxBuffer[telemetryRxBufferCount++] = data ^ STUFF_MASK;
      }
      dataState = STATE_DATA_IN_FRAME;
      break;

    case STATE_DATA_IDLE:
      if (data == START_STOP) {
        telemetryRxBufferCount = 0;
        dataState = STATE_DATA_START;
      }
      break;

  } // switch

  if (IS_FRSKY_SPORT_PROTOCOL() && telemetryRxBufferCount >= FRSKY_SPORT_PACKET_SIZE) {
    // end of frame detected
    dataState = STATE_DATA_IDLE;
    return true;
  }

  return false;
}

