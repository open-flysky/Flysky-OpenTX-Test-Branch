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
#define WAIT_20MS 10
extern Fifo<uint8_t, BT_FIFO_SIZE> btTxFifo;
extern Fifo<uint8_t, BT_FIFO_SIZE> btRxFifo;

BluetoothLE bluetooth;


int32_t getBtPlatfrom() {
  TRACE("ADV %d", bluetooth.config.advertising_interval);
  TRACE("BROADCAST %d", bluetooth.config.broadcast_interval);

  if (bluetooth.config.advertising_interval == ANDROID_ADV_INTERVAL && 
    bluetooth.config.broadcast_interval == ANDROID_BROADCAST_INTERVAL) {
    return static_cast<int>(BLUETOOTH_TARGET_PLATFORM_ANDROID);
  }
  if (bluetooth.config.advertising_interval == IOS_ADV_INTERVAL && 
    bluetooth.config.broadcast_interval == IOS_BROADCAST_INTERVAL) {
    return static_cast<int>(BLUETOOTH_TARGET_PLATFORM_IOS);
  }
  return static_cast<int>(BLUETOOTH_TARGET_PLATFORM_UNDEFINED);
}

void setBtPlatform(int32_t platform) {
  switch(static_cast<BLUETOOTH_TARGET_PLATFORM_TYPE>(platform)) {
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

int32_t getBtPasscode() {
  return (int32_t)bluetooth.config.passcode;
}

void setBtPasscode(int32_t passcode) {
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
void setBtPasscodeEnabled(int32_t enabled){
  bluetooth.config.passcode_protection = enabled != 0;
  bluetooth.config.dirty = true;
}

void setBtBaudrate(int32_t baudIndex) {
  bluetooth.config.baudrate = static_cast<btle::Baudrate>(baudIndex);
  bluetooth.config.dirty = true;
}

void BluetoothLE::writeTelemetryPacket(const uint8_t * data, uint8_t length) {
   if (g_eeGeneral.bluetoothMode == BLUETOOTH_TELEMETRY && bluetooth.state == BLUETOOTH_STATE_CONNECTED) {
     write(data, length);
   }
}

void BluetoothLE::write(const uint8_t * data, uint8_t length)
{
  if (btTxFifo.hasSpace(length)) {
    for (int i = 0; i < length; i++) {
      btTxFifo.push(data[i]);
    }
    bluetoothWriteWakeup();
  }
  else {
    TRACE("[BT] TX fifo full!");
  }
}

void BluetoothLE::writeString(const char * str)
{
  if (btTxFifo.hasSpace(strlen(str)+2)) {
    while (*str != 0) {
      btTxFifo.push(*str++);
    }
    btTxFifo.push('\r');
    btTxFifo.push('\n');
    bluetoothWriteWakeup();
  }
  else {
    TRACE("[BT] TX fifo full!");
  }
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
    if (byte == '\n') {
      if (bufferIndex > 2 && buffer[bufferIndex-1] == '\r') {
        buffer[bufferIndex-1] = '\0';
        TRACE("BT< %s" CRLF, buffer);
        return;
      }
      bufferIndex = 0;
    }
    else buffer[bufferIndex++] = byte;
  }
}


BluetoothLE::BluetoothLE() {
  state = BLUETOOTH_LE_STATE_OFF;
  currentMode = BLUETOOTH_UNKNOWN;
  rxDataState = STATE_DATA_IDLE;
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
      strncpy(buffer, "Connected", bufferSize);
      break;
    case BLUETOOTH_LE_STATE_READY:
       strncpy(buffer, "Ready", bufferSize);
       break;
    default:
      strncpy(buffer, "Unknown", bufferSize);
      break;
  }
}

void BluetoothLE::start() {
  setState(g_eeGeneral.bluetoothMode != BLUETOOTH_OFF ? BLUETOOTH_LE_STATE_BAUD_DETECT : BLUETOOTH_LE_STATE_OFF);
}

void BluetoothLE::stop() {
  setState(BLUETOOTH_LE_STATE_OFF);
}


void BluetoothLE::pushByte(uint8_t * buffer, uint8_t byte, unsigned& bufferIndex, uint8_t& crc) { 
  crc ^= byte;
  if (byte == START_STOP || byte == BYTE_STUFF) {
    buffer[bufferIndex++] = BYTE_STUFF;
    byte ^= STUFF_MASK;
  }
  buffer[bufferIndex++] = byte;
}

void BluetoothLE::receiveTrainer() {
  uint8_t byte;
  while (true) {
    if (!btRxFifo.pop(byte)) {
      return;
    }
    switch (rxDataState) {
    case STATE_DATA_START:
      if (byte == START_STOP) {
        rxDataState = STATE_DATA_IN_FRAME;
        rxIndex = 0;
      }
      else {
        appendTrainerByte(byte);
      }
      break;

    case STATE_DATA_IN_FRAME:
      if (byte == BYTE_STUFF) {
        rxDataState = STATE_DATA_XOR; // XOR next byte
      }
      else if (byte == START_STOP) {
        rxDataState = STATE_DATA_IN_FRAME;
        rxIndex = 0;
      }
      else {
        appendTrainerByte(byte);
      }
      break;

    case STATE_DATA_XOR:
      appendTrainerByte(byte ^ STUFF_MASK);
      rxDataState = STATE_DATA_IN_FRAME;
      break;

    case STATE_DATA_IDLE:
      if (byte == START_STOP) {
        rxIndex = 0;
        rxDataState = STATE_DATA_START;
      }
      else {
        appendTrainerByte(byte);
      }
      break;
    }

    if (rxIndex >= BLUETOOTH_TRAINER_PACKET_SIZE) {
      uint8_t crc = 0x00;
      for (int i=0; i<13; i++) {
        crc ^= bt_data[i];
      }
      if (crc == bt_data[13]) {
        if (bt_data[0] == 0x80) {
          BLUETOOTH_TRACE(CRLF);
          for (uint8_t channel=0, i=1; channel<8; channel+=2, i+=3) {
            // +-500 != 512, but close enough.
            ppmInput[channel] = bt_data[i] + ((bt_data[i+1] & 0xf0) << 4) - 1500;
            ppmInput[channel+1] = ((bt_data[i+1] & 0x0f) << 4) + ((bt_data[i+2] & 0xf0) >> 4) + ((bt_data[i+2] & 0x0f) << 8) - 1500;
          }
          ppmInputValidityTimer = PPM_IN_VALID_TIMEOUT;
        }
      }
      rxDataState = STATE_DATA_IDLE;
    }
  }
}


void BluetoothLE::appendTrainerByte(uint8_t byte)
{
  if (rxIndex < BLUETOOTH_LE_LINE_LENGTH) {
    bt_data[rxIndex++] = byte;
    // we check for "DisConnected", but the first byte could be altered (if received in state STATE_DATA_XOR)
    if (byte == '\n') {
      if (!strncmp((char *)&bt_data[rxIndex-13], "isConnected", 11)) {
        BLUETOOTH_TRACE("BT< DisConnected" CRLF);
        //TBD not sure is we can disconnect
        //setState(BLUETOOTH_STATE_DISCONNECTED);
        rxIndex = 0;
        //force next reques in one second
        //wakeupTime += 200; // 1s
      }
    }
  }
}

void BluetoothLE::sendTrainer() {
  int16_t PPM_range = g_model.extendedLimits ? 640*2 : 512*2;
  int firstCh = g_model.moduleData[TRAINER_MODULE].channelsStart;
  int lastCh = firstCh + 8;
  uint8_t crc = 0x00;
  unsigned i = 0;
  
  bt_data[i++] = START_STOP; // start byte
  pushByte(bt_data, 0x80, i, crc); // trainer frame type?
  for (int channel=0; channel<lastCh; channel+=2) {
    uint16_t channelValue1 = PPM_CH_CENTER(channel) + limit((int16_t)-PPM_range, channelOutputs[channel], (int16_t)PPM_range) / 2;
    uint16_t channelValue2 = PPM_CH_CENTER(channel+1) + limit((int16_t)-PPM_range, channelOutputs[channel+1], (int16_t)PPM_range) / 2;
    pushByte(bt_data, channelValue1 & 0x00ff, i, crc);
    pushByte(bt_data, ((channelValue1 & 0x0f00) >> 4) + ((channelValue2 & 0x00f0) >> 4), i, crc);
    pushByte(bt_data, ((channelValue2 & 0x000f) << 4) + ((channelValue2 & 0x0f00) >> 8), i, crc);
  }
  bt_data[i++] = crc;
  bt_data[i++] = START_STOP; // end byte

  write(bt_data, i);
}

void BluetoothLE::forwardTelemetry(const uint8_t * packet) {
  uint8_t crc = 0x00;
  unsigned idx = 0;
  bt_data[idx++] = START_STOP; // start byte
  for (uint8_t i=0; i<sizeof(SportTelemetryPacket); i++) {
    pushByte(bt_data, (uint8_t)packet[i], idx, crc);
  }
  bt_data[idx++] = crc;
  bt_data[idx++] = START_STOP; // end byte

  if (idx >= 2*FRSKY_SPORT_PACKET_SIZE) {
    write(bt_data, idx);
  }
}


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
    case BLUETOOTH_LE_STATE_READY:
      break;
    case BLUETOOTH_STATE_CONNECTED:
      BT_COMMAND_OFF();
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


uint32_t BluetoothLE::handleConfiguration() {
  size_t cmd_size = 0;
  uint8_t byte;
  rxIndex = 0;
  while (btRxFifo.pop(byte) && rxIndex < sizeof(bt_data)) {
    bt_data[rxIndex++] = byte;
  }
  
  btle::ResponseFoxware* foxwareFrame = reinterpret_cast<btle::ResponseFoxware*>(bt_data);
  
  switch(state) {
    case BLUETOOTH_LE_STATE_BAUD_DETECT:
      if (btle::valid(foxwareFrame, btle::GET_FW_VERSION, rxIndex)) {
        setState(BLUETOOTH_LE_STATE_REQUESTING_CONFIG);
        cmd_size = config.load(bt_data);
        break;
      } else {
        g_eeGeneral.bluetoothBaudrate = (g_eeGeneral.bluetoothBaudrate + 1) % sizeof(btle::baudRateMap);
      }
      return WAIT_100MS;
      break;
    case BLUETOOTH_LE_STATE_REQUESTING_CONFIG:
      if (config.response(foxwareFrame, rxIndex) == btle::ResponseStatus::Done) {
        setState(BLUETOOTH_LE_STATE_READY);
        return WAIT_100MS;
      }
      cmd_size = config.load(bt_data);
      break;
    case BLUETOOTH_LE_STATE_SAVING_CONFIG:
      if (config.responseSave(foxwareFrame, rxIndex) == btle::ResponseStatus::Done) {
        setState(BLUETOOTH_LE_STATE_READY);
        return WAIT_100MS;
      }
      cmd_size = config.save(bt_data, &this->setBaudrateFromConfig);
      break;
  }
  uint32_t taskDuration = send(bt_data, cmd_size);
  return taskDuration;
}

void BluetoothLE::sendSensors()
{ 
  static int index = 0;
  char buffer[64];
  sprintf(buffer, "DEAD FOOD %d", index++);
  if (btTxFifo.hasSpace(strlen(buffer)+2)) {
    const char* str = buffer;
    while (*str != 0) {
      btTxFifo.push(*str++);
    }
    btTxFifo.push('\r');
    btTxFifo.push('\n');
  }
}
uint32_t BluetoothLE::wakeup()
{
  if (currentMode != g_eeGeneral.bluetoothMode) {
    currentMode = g_eeGeneral.bluetoothMode;
    setState(g_eeGeneral.bluetoothMode == BLUETOOTH_OFF ? BLUETOOTH_LE_STATE_OFF : BLUETOOTH_LE_STATE_BAUD_DETECT);
    return WAIT_100MS;
  }
  
  if (currentBaudrate != g_eeGeneral.bluetoothBaudrate) {
    currentBaudrate = g_eeGeneral.bluetoothBaudrate;
    setState(BLUETOOTH_LE_STATE_BAUD_DETECT);
    rxIndex = 0;
    return send(bt_data, btle::fw_version(bt_data));
  } 

  if (setBaudrateFromConfig) {
    setBaudrateFromConfig = false;
    g_eeGeneral.bluetoothBaudrate = config.baudrate;
  }

  if (config.dirty) {
    setState(BLUETOOTH_LE_STATE_SAVING_CONFIG);
  } else if (state == BLUETOOTH_LE_STATE_READY && IS_BT_CONNECTED()) {
    setState(BLUETOOTH_STATE_CONNECTED);
  } else if (state == BLUETOOTH_STATE_CONNECTED && !IS_BT_CONNECTED()) {
    setState(BLUETOOTH_LE_STATE_READY);
  }

  if (state == BLUETOOTH_STATE_CONNECTED) {
    if (g_eeGeneral.bluetoothMode == BLUETOOTH_TRAINER) {
      char line[BLUETOOTH_LE_LINE_LENGTH];
      switch(g_model.trainerMode) {
        case TRAINER_MODE_MASTER_BLUETOOTH:
          receiveTrainer();
          break;
        case TRAINER_MODE_SLAVE_BLUETOOTH:
          sendTrainer();
          //tbd VERIFY
          readline(line, BLUETOOTH_LE_LINE_LENGTH); // to deal with "ERROR"
          break;
      }
      //disconnect after error
      if (state != BLUETOOTH_STATE_CONNECTED) return WAIT_100MS;
      return WAIT_20MS;
    }
    else if (g_eeGeneral.bluetoothMode == BLUETOOTH_SENSORS) {
      sendSensors();
    } 
  
    size_t fifoSize = btTxFifo.size();
    bluetoothWriteWakeup();
    return btle::transmit_time_ms(fifoSize, (btle::Baudrate)currentBaudrate, btle::DeviceType::ANDROID) / 2;
  } 
  return handleConfiguration();
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

