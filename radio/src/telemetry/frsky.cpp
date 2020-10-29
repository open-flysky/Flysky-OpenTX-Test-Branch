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
    memset(telemetryRxBuffer, 0, sizeof(telemetryRxBuffer));
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

static uint8_t dataState = STATE_DATA_IDLE;
//#define DEBUG_SPORT
#if defined(DEBUG_SPORT)
#define TRACE_SPORT(f_, ...)        debugPrintf((f_ "\r\n"), ##__VA_ARGS__)
#else
#define TRACE_SPORT(...)
#endif

inline void setStateFrsky(enum FrSkyDataState target) {
    dataState = (uint8_t)target;
    switch(dataState) {
    case STATE_DATA_IDLE:
      TRACE_SPORT("SPORT SET IDLE [%d]", telemetryRxBufferCount);
    break;
    case STATE_DATA_START:
      TRACE_SPORT("SPORT SET START [%d]", telemetryRxBufferCount);
    break;
    case STATE_DATA_IN_FRAME:
      TRACE_SPORT("SPORT SET FRAME [%d]", telemetryRxBufferCount);
    break;
    case STATE_DATA_XOR:
      TRACE_SPORT("SPORT SET XOR [%d]", telemetryRxBufferCount);
      break;
    default:
      TRACE_SPORT("SPORT SET UNKOWN [%d]", telemetryRxBufferCount);
    break;
  }
}

bool pushFrskyTelemetryData(uint8_t data)
{
  static uint16_t lastSport = 0;
 
  if (IS_FRSKY_SPORT_PROTOCOL()) {
    uint16_t now = getTmr2MHz();
    uint16_t timeDiff = now - lastSport;
    bool timout = timeDiff > 20000;
    lastSport = now;
    if (timout && data != START_STOP) { //> 10ms
      setStateFrsky(STATE_DATA_IDLE);
      telemetryRxBufferCount = 0;
      memset(telemetryRxBuffer, 0, sizeof(telemetryRxBuffer));
      //TRACE("SPORT TIMEOUT %d", timeDiff/2000);
      return false;
    } 
  }
  switch (dataState) {
    case STATE_DATA_START:
      TRACE_SPORT("SPORT START[%d] %02X", telemetryRxBufferCount, data);
      if (data == START_STOP) {
        if (IS_FRSKY_SPORT_PROTOCOL()) {
          setStateFrsky(STATE_DATA_IN_FRAME);
          telemetryRxBufferCount = 0;
        }
      }
      else {
        if (telemetryRxBufferCount < TELEMETRY_RX_PACKET_SIZE) {
          telemetryRxBuffer[telemetryRxBufferCount++] = data;
        }
        setStateFrsky(STATE_DATA_IN_FRAME);
      }
      break;

    case STATE_DATA_IN_FRAME:
      TRACE_SPORT("SPORT FRAME[%d] %02X", telemetryRxBufferCount, data);
      
      if (data == BYTE_STUFF) {
        setStateFrsky(STATE_DATA_XOR); // XOR next byte
      }
      else if (data == START_STOP) {
        if (IS_FRSKY_SPORT_PROTOCOL()) {
          setStateFrsky(STATE_DATA_IN_FRAME);
          telemetryRxBufferCount = 0;
          TRACE_SPORT("SPORT RESET");
        }
        else {
          // end of frame detected
          setStateFrsky(STATE_DATA_IDLE);
          telemetryRxBufferCount = 0;
          TRACE_SPORT("SPORT RESET");
          return true;
        }
        break;
      }
      else if (telemetryRxBufferCount < TELEMETRY_RX_PACKET_SIZE) {
        telemetryRxBuffer[telemetryRxBufferCount++] = data;
      }
      break;
    case STATE_DATA_XOR:
      TRACE_SPORT("SPORT XOR[%d] %02X", telemetryRxBufferCount, data);
      if (telemetryRxBufferCount < TELEMETRY_RX_PACKET_SIZE) {
        telemetryRxBuffer[telemetryRxBufferCount++] = data ^ STUFF_MASK;
      }
      setStateFrsky(STATE_DATA_IN_FRAME);
      break;

    case STATE_DATA_IDLE:
      TRACE_SPORT("SPORT IDLE[%d] %02X", telemetryRxBufferCount, data);
      if (data == START_STOP) {
        setStateFrsky(STATE_DATA_START);
        telemetryRxBufferCount = 0;
 
      }
      break;
    default:
      TRACE_SPORT("SPORT UNKOWN[%d] %02X", telemetryRxBufferCount, data);
      break;
  } // switch

  if (IS_FRSKY_SPORT_PROTOCOL() && dataState == STATE_DATA_IN_FRAME && telemetryRxBufferCount >= FRSKY_SPORT_PACKET_SIZE) {
    // end of frame detected
    setStateFrsky(STATE_DATA_IDLE);
    telemetryRxBufferCount = 0;
    TRACE_SPORT("SPORT RESET");
    return true;
  }

  return false;
}

