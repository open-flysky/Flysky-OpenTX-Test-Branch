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

#ifndef _CROSSFIRE_H_
#define _CROSSFIRE_H_

// Device address
#define BROADCAST_ADDRESS              0x00
#define RADIO_ADDRESS                  0xEA
#define MODULE_ADDRESS                 0xEE
#define SYNC_BYTE                      0xC8

// Frame id
#define GPS_ID                         0x02
#define BATTERY_ID                     0x08
#define HEARTBEAT_ID                   0x0B
#define VIDEO_TRANSMITTER_ID           0x0F
#define LINK_ID                        0x14
#define CHANNELS_ID                    0x16
#define ATTITUDE_ID                    0x1E
#define FLIGHT_MODE_ID                 0x21
#define PING_DEVICES_ID                0x28
#define DEVICE_INFO_ID                 0x29
#define REQUEST_SETTINGS_ID            0x2A
#define ENTRY_SETTINGS_ID              0x2B
#define READ_SETTINGS_ID               0x2C
#define WRITE_SETTINGS_ID              0x2D
#define CROSSFIRE_COMMAND_ID           0x32
#define RADIO_ID                       0x3A


void processCrossfireTelemetryData(uint8_t data);
void crossfireSetDefault(int index, uint8_t id, uint8_t subId);
bool crossfireGet(uint8_t* buffer, uint8_t& dataSize);
void crossfireSend(uint8_t* payload, size_t size);
bool isCrossfireError();

const uint32_t CROSSFIRE_BAUDRATES[] = {
  400000,
  115200,
};

const uint32_t CROSSFIRE_PERIODS[] = {
  4000,
  16000,
};
#if SPORT_MAX_BAUDRATE < 400000
#define CROSSFIRE_BAUDRATE    CROSSFIRE_BAUDRATES[g_eeGeneral.telemetryBaudrate]
#define CROSSFIRE_PERIOD      (CROSSFIRE_PERIODS[g_eeGeneral.telemetryBaudrate]*1000)
#else
#define CROSSFIRE_BAUDRATE       400000
#define CROSSFIRE_PERIOD         4000 /*us*/
#endif

#define CROSSFIRE_TELEM_MIRROR_BAUDRATE   115200

#endif // _CROSSFIRE_H_
