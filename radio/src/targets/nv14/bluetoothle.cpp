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
#include "bluetoothle.h"


#if defined(LOG_BLUETOOTH)
extern FIL g_bluetoothFile;
#endif
#define WAIT_100MS 50

extern Fifo<uint8_t, BT_FIFO_SIZE> btTxFifo;
extern Fifo<uint8_t, BT_FIFO_SIZE> btRxFifo;

BluetoothLE bluetooth;


BLUETOOTH_TARGET_PLATFORM_TYPE getPlatfrom() {
  TRACE("ADV %d", bluetooth.config.advertising_interval);
  TRACE("BROADCAST %d", bluetooth.config.broadcast_interval);

  if (bluetooth.config.advertising_interval == ANDROID_ADV_INTERVAL && 
    bluetooth.config.broadcast_interval == ANDROID_BROADCAST_INTERVAL) {
    return BLUETOOTH_TARGET_PLATFORM_ANDROID;
  }
  if (bluetooth.config.advertising_interval == IOS_ADV_INTERVAL && 
    bluetooth.config.broadcast_interval == IOS_BROADCAST_INTERVAL) {
    return BLUETOOTH_TARGET_PLATFORM_IOS;
  }
  return BLUETOOTH_TARGET_PLATFORM_UNDEFINED;
}

void setPlatform(enum BLUETOOTH_TARGET_PLATFORM_TYPE platform) {
  switch(platform) {
    case BLUETOOTH_TARGET_PLATFORM_ANDROID:
      bluetooth.config.advertising_interval = ANDROID_ADV_INTERVAL;
      bluetooth.config.broadcast_interval = ANDROID_BROADCAST_INTERVAL;
      bluetooth.config.dirty = true;
      break;
    case BLUETOOTH_TARGET_PLATFORM_IOS:
      bluetooth.config.advertising_interval = IOS_ADV_INTERVAL;
      bluetooth.config.broadcast_interval = IOS_BROADCAST_INTERVAL;
      bluetooth.config.dirty = true;
      break;
    default:
      bluetooth.config.advertising_interval = DEFAULT_ADV_INTERVAL;
      bluetooth.config.broadcast_interval = DEFAULT_BROADCAST_INTERVAL;
      bluetooth.config.dirty = true;
      break;
  }
}

int32_t getPasscode() {
  return (int32_t)bluetooth.config.passcode;
}

void setPasscode(int32_t passcode) {
  bluetooth.config.passcode = (uint32_t)passcode;
  bluetooth.config.dirty = true;
}

void setBtTxPower(int32_t power) {
  bluetooth.config.tx_power = static_cast<btle::TxPower>(power);
  bluetooth.config.dirty = true;
}

int32_t isPasscodeEnabled() {
  return(int32_t) bluetooth.config.passcode_protection;
}
void setPasscodeEnabled(int32_t enabled){
  bluetooth.config.passcode_protection = enabled != 0;
  bluetooth.config.dirty = true;
}

void setBtBaudrate(int32_t baudIndex) {
  bluetooth.config.baudrate = static_cast<btle::Baudrate>(baudIndex);
  bluetooth.config.dirty = true;
}

void BluetoothLE::write(const uint8_t * data, uint8_t length)
{
  if (btTxFifo.hasSpace(length)) {
    BLUETOOTH_TRACE("BT>");
    for (int i = 0; i < length; i++) {
      BLUETOOTH_TRACE(" %02X", data[i]);
      btTxFifo.push(data[i]);
    }
    BLUETOOTH_TRACE(CRLF);
  }
  else {
    BLUETOOTH_TRACE("[BT] TX fifo full!" CRLF);
  }

  bluetoothWriteWakeup();
}

void BluetoothLE::writeString(const char * str)
{
  BLUETOOTH_TRACE("BT> %s" CRLF, str);
  while (*str != 0) {
    btTxFifo.push(*str++);
  }
  btTxFifo.push('\r');
  btTxFifo.push('\n');
  bluetoothWriteWakeup();
}

void BluetoothLE::readline(char * buffer, uint8_t length)
{
  uint8_t byte;
  int bufferIndex = 0;
  buffer[0] = '\0';
  while (bufferIndex < length) {
    if (!btRxFifo.pop(byte)) {
      return;
    }
    BLUETOOTH_TRACE("%02X ", byte);
    if (byte == '\n') {
      if (bufferIndex > 2 && buffer[bufferIndex-1] == '\r') {
        buffer[bufferIndex-1] = '\0';
        BLUETOOTH_TRACE("BT< %s" CRLF, buffer);
        return;
      }
      bufferIndex = 0;
    }
    else buffer[bufferIndex++] = byte;
  }
}

void BluetoothLE::processTrainerFrame(const uint8_t * buffer)
{
  BLUETOOTH_TRACE(CRLF);

  for (uint8_t channel=0, i=1; channel<8; channel+=2, i+=3) {
    // +-500 != 512, but close enough.
    ppmInput[channel] = buffer[i] + ((buffer[i+1] & 0xf0) << 4) - 1500;
    ppmInput[channel+1] = ((buffer[i+1] & 0x0f) << 4) + ((buffer[i+2] & 0xf0) >> 4) + ((buffer[i+2] & 0x0f) << 8) - 1500;
  }

  ppmInputValidityTimer = PPM_IN_VALID_TIMEOUT;
}


void BluetoothLE::appendTrainerByte(uint8_t data)
{
  // if (bufferIndex < BLUETOOTH_LINE_LENGTH) {
  //   buffer[bufferIndex++] = data;
  //   // we check for "DisConnected", but the first byte could be altered (if received in state STATE_DATA_XOR)
  //   if (data == '\n') {
  //     if (!strncmp((char *)&buffer[bufferIndex-13], "isConnected", 11)) {
  //       BLUETOOTH_TRACE("BT< DisConnected" CRLF);
  //       state = BLUETOOTH_STATE_DISCONNECTED;
  //       bufferIndex = 0;
  //       wakeupTime += 200; // 1s
  //     }
  //   }
  // }
}

BluetoothLE::BluetoothLE() {
  state = BLUETOOTH_LE_STATE_OFF;
  currentMode = BLUETOOTH_UNKNOWN;
}

void BluetoothLE::getStatus(char* buffer, size_t bufferSize) { 
  switch(state) {
   case BLUETOOTH_LE_STATE_OFF:
      strncpy(buffer, "Off", bufferSize);
      break;
    case BLUETOOTH_LE_STATE_BAUD_DETECT: 
      strncpy(buffer, "Detecting...", bufferSize);
      break;
    case BLUETOOTH_LE_STATE_REQUESTING_CONFIG: 
      strncpy(buffer, "Loading config...", bufferSize);
      break;
    case BLUETOOTH_LE_STATE_SAVING_CONFIG: 
      strncpy(buffer, "Saving config...", bufferSize);
      break;
    case BLUETOOTH_STATE_CONNECTED: 
      strncpy(buffer, "Active", bufferSize);
      break;
    default:
      strncpy(buffer, "Unknown", bufferSize);
  }
}

void BluetoothLE::start() {
  setState(g_eeGeneral.bluetoothMode != BLUETOOTH_OFF ? BLUETOOTH_LE_STATE_BAUD_DETECT : BLUETOOTH_LE_STATE_OFF);
}

void BluetoothLE::stop() {
  setState(BLUETOOTH_LE_STATE_OFF);
}


void BluetoothLE::pushByte(uint8_t * buffer, int index, uint8_t byte, uint8_t crc) { 
}

//tbd
void BluetoothLE::processTrainerByte(uint8_t data) { }
void BluetoothLE::sendTrainer() { }
void BluetoothLE::receiveTrainer() {}
void BluetoothLE::forwardTelemetry(const uint8_t * data) {}


void BluetoothLE::setState(enum BluetoothLeStates state) {
  switch(state) {
    case BLUETOOTH_LE_STATE_BAUD_DETECT:
      BT_COMMAND_ON();
      bluetoothInit(btle::baudRateMap[currentBaudrate]);
      break;
    case BLUETOOTH_LE_STATE_OFF:
      TRACE("SET OFF");
      bluetoothDone();
      //BT_COMMAND_OFF();
      config.reset();
    case BLUETOOTH_LE_STATE_REQUESTING_CONFIG:
      BT_COMMAND_ON();
      config.preLoad();
      break;
    case BLUETOOTH_LE_STATE_SAVING_CONFIG:
      BT_COMMAND_ON();
      config.preSave();
    case BLUETOOTH_STATE_CONNECTED:
      //BT_COMMAND_OFF();
      break;
  }
  this->state = state;
}

uint32_t BluetoothLE::send(uint8_t* frame, size_t cmd_size) {
  if (cmd_size) {
    btTxFifo.clear();
    for (size_t i = 0; i < cmd_size; i++) {
      btTxFifo.push(frame[i]);
    }
  }
  size_t fifoSize = btTxFifo.size();
  bluetoothWriteWakeup();

  if (state == BLUETOOTH_STATE_CONNECTED) {
    return btle::transmit_time_ms(fifoSize, (btle::Baudrate)currentBaudrate, btle::DeviceType::ANDROID) / 2;
  }
  return WAIT_100MS;
}

uint32_t BluetoothLE::wakeup()
{
  size_t cmd_size = 0;
  uint8_t frame[200];
  uint8_t rx_size = 0;
  uint8_t byte;
  

  if (currentMode != g_eeGeneral.bluetoothMode) {
    currentMode = g_eeGeneral.bluetoothMode;
    setState(g_eeGeneral.bluetoothMode == BLUETOOTH_OFF ? BLUETOOTH_LE_STATE_OFF : BLUETOOTH_LE_STATE_BAUD_DETECT);
    return WAIT_100MS;
  }
  
  if (currentBaudrate != g_eeGeneral.bluetoothBaudrate) {
    currentBaudrate = g_eeGeneral.bluetoothBaudrate;
    setState(BLUETOOTH_LE_STATE_BAUD_DETECT);
    cmd_size = btle::fw_version(frame);
    return send(frame, cmd_size);
  } 

  if (setBaudrateFromConfig) {
    setBaudrateFromConfig = false;
    g_eeGeneral.bluetoothBaudrate = config.baudrate;
  }

  if (config.dirty) {
    setState(BLUETOOTH_LE_STATE_SAVING_CONFIG);
  }

  while (btRxFifo.pop(byte) && rx_size < sizeof(frame)) {
    frame[rx_size++] = byte;
  }
  
  
  btle::ResponseFoxware* foxwareFrame = reinterpret_cast<btle::ResponseFoxware*>(frame);
  
  switch(state) {
    case BLUETOOTH_LE_STATE_BAUD_DETECT:
      if (btle::valid(foxwareFrame, btle::GET_FW_VERSION, rx_size)) {
        setState(BLUETOOTH_LE_STATE_REQUESTING_CONFIG);
        cmd_size = config.load(frame);
        break;
      } else {
        g_eeGeneral.bluetoothBaudrate = (g_eeGeneral.bluetoothBaudrate + 1) % sizeof(btle::baudRateMap);
      }
      return WAIT_100MS;
      break;
    case BLUETOOTH_LE_STATE_REQUESTING_CONFIG:
      if (config.response(foxwareFrame, rx_size) == btle::ResponseStatus::Done) {
        setState(BLUETOOTH_STATE_CONNECTED);
        return WAIT_100MS;
      }
      cmd_size = config.load(frame);
      break;
    case BLUETOOTH_LE_STATE_SAVING_CONFIG:
      if (config.responseSave(foxwareFrame, rx_size) == btle::ResponseStatus::Done) {
        setState(BLUETOOTH_STATE_CONNECTED);
        return WAIT_100MS;
      }
      cmd_size = config.save(frame, &this->setBaudrateFromConfig);
      break;
    case BLUETOOTH_STATE_CONNECTED:
      //communication handling
      break;
  }
  uint32_t taskDuration = send(frame, cmd_size);
  return taskDuration;
}

uint8_t BluetoothLE::read(uint8_t * data, uint8_t size, uint32_t timeout)
{
  watchdogSuspend(timeout / 10);

  uint8_t len = 0;
  while (len < size) {
    uint32_t elapsed = 0;
    uint8_t byte;
    while (!btRxFifo.pop(byte)) {
      if (elapsed++ >= timeout) {
        return len;
      }
      RTOS_WAIT_MS(1);
    }
    data[len++] = byte;
  }
  return len;
}

