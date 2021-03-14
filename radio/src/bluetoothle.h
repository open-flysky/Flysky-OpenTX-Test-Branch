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
#ifndef BLUETOOTH_LE_H
#define BLUETOOTH_LE_H

enum BluetoothLeStates {
  BLUETOOTH_LE_STATE_OFF,
  BLUETOOTH_LE_STATE_BAUD_DETECT,
  BLUETOOTH_LE_STATE_REQUESTING_CONFIG,
  BLUETOOTH_LE_STATE_SAVING_CONFIG,
  BLUETOOTH_STATE_CONNECTED, //compatybile with other interface
};

#define BT_FIFO_SIZE 256

#define BLUETOOTH_LE_PACKET_SIZE           200
#define BLUETOOTH_LE_LINE_LENGTH           32
#define LEN_BLUETOOTH_ADDR                16
#define MAX_BLUETOOTH_DISTANT_ADDR        6


enum BLUETOOTH_TARGET_PLATFORM_TYPE {
  BLUETOOTH_TARGET_PLATFORM_FIRST = 0,
  BLUETOOTH_TARGET_PLATFORM_UNDEFINED = 0,
  BLUETOOTH_TARGET_PLATFORM_ANDROID = 1,
  BLUETOOTH_TARGET_PLATFORM_IOS = 2,
  BLUETOOTH_TARGET_PLATFORM_LAST = 2,
};

#if defined(LOG_BLUETOOTH)
  #define BLUETOOTH_TRACE(...)  \
    f_printf(&g_bluetoothFile, __VA_ARGS__); \
    TRACE_NOCRLF(__VA_ARGS__);
#else
  #define BLUETOOTH_TRACE(...)  \
    TRACE_NOCRLF(__VA_ARGS__);
#endif

#include "lierda_bt.h"

class BluetoothLE
{
  public:
    BluetoothLE();
    void start();
    void stop();
    uint8_t read(uint8_t * data, uint8_t size, uint32_t timeout=1000/*ms*/);
    void readline(char * str, uint8_t length);
    void write(const uint8_t * data, uint8_t length);
    void writeString(const char * str);
    void forwardTelemetry(const uint8_t * data);
    //returns task delay in COOS ticks (ms / 2)
    uint32_t wakeup();
    void getStatus(char* buffer, size_t bufferSize);

    volatile uint8_t state;
    struct btle::configFoxware config;
    
  protected:
    uint32_t send(uint8_t* frame, size_t cmd_size);
    void setState(enum BluetoothLeStates state);
    void pushByte(uint8_t * buffer, int index, uint8_t byte, uint8_t crc);
    void appendTrainerByte(uint8_t data);
    void processTrainerFrame(const uint8_t * buffer);
    void processTrainerByte(uint8_t data);
    void sendTrainer();
    void receiveTrainer();
    
    uint8_t currentBaudrate;
    uint8_t currentMode;
    bool setBaudrateFromConfig;
};

extern BluetoothLE bluetooth;

BLUETOOTH_TARGET_PLATFORM_TYPE getPlatfrom();
void setPlatform(enum BLUETOOTH_TARGET_PLATFORM_TYPE platform);

int32_t getPasscode();
void setPasscode(int32_t passcode);

int32_t isPasscodeEnabled();
void setPasscodeEnabled(int32_t enabled);
void setBtTxPower(int32_t power);
void setBtBaudrate(int32_t baudIndex);
#endif
