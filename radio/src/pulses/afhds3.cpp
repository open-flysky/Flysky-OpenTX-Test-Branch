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


#include "afhds3.h"
#include "../debug.h"
#include "../definitions.h"
#include "../translations.h"
#define FAILSAFE_HOLD 1
#define FAILSAFE_CUSTOM 2
namespace afhds3 {

static const char* const moduleStateText[] = {
   "Not ready",
   "HW Error",
   "Binding",
   "Connecting",
   "Connected",
   "Standby",
   "Waiting for update",
   "Updating",
   "Updating RX",
   "Updating RX failed",
   "Testing"
   "Ready",
   "HW test"
};

static const char* const powerSourceText[] = {
   "Unknown",
   "Internal",
   "External"
};

static const COMMAND periodicRequestCommands[] = {
    COMMAND::MODULE_STATE,
    COMMAND::MODULE_POWER_STATUS,
    COMMAND::MODULE_GET_CONFIG,
    COMMAND::VIRTUAL_FAILSAFE
};

ModuleState afhds3::getStateEnum() {
  return (ModuleState)data->state;
}
void afhds3::getState(char* buffer) {
  strcpy(buffer, "Unknown");
  if(data->state <= ModuleState::STATE_READY) strcpy(buffer, moduleStateText[data->state]);
}

void afhds3::getOpMode(char* buffer) {
  strcpy(buffer, cfg.config.telemetry ? STR_AFHDS3_ONE_TO_ONE_TELEMETRY : STR_AFHDS3_ONE_TO_MANY);
}

void afhds3::getHwFw(char* buffer) {

}

void afhds3::getPowerSource(char* buffer) {
   strcpy(buffer, "Unknown");
   if(powerSource <= MODULE_POWER_SOURCE::EXTERNAL) strcpy(buffer, powerSourceText[powerSource]);
}

void afhds3::putByte(uint8_t byte) {
  this->putBytes(&byte, 1);
}

void afhds3::putBytes(uint8_t* data, int length) {
  for (int i = 0; i < length; i++) {
    uint8_t byte = data[i];
    this->data->crc += byte;
    if (END == byte) {
      *this->data->ptr++ = ESC;
      *this->data->ptr++ = ESC_END;
    }
    else if (ESC == byte) {
      *this->data->ptr++ = ESC;
      *this->data->ptr++ = ESC_ESC;
    }
    else {
      *this->data->ptr++ = byte;
    }
  }
}

void afhds3::putHeader(COMMAND command, FRAME_TYPE frame) {
  operationState = State::SENDING_COMMAND;
  data->ptr = this->data->pulses;
  data->crc = 0;
  *data->ptr++ = END;
  uint8_t buffer[] = { FrameAddress, data->frame_index, frame, command};
  putBytes(buffer, 4);
}


void afhds3::putFooter() {
  putByte(data->crc ^ 0xff);
  *data->ptr++ = END;
  data->frame_index++;

  switch((FRAME_TYPE)data->pulses[3])
  {
    case FRAME_TYPE::REQUEST_GET_DATA:
    case FRAME_TYPE::REQUEST_SET_EXPECT_ACK:
    case FRAME_TYPE::REQUEST_SET_EXPECT_DATA:
      operationState = State::AWAITING_RESPONSE;
      break;
    default:
      operationState = State::IDLE;
  }
}

void afhds3::putFrame(COMMAND command, FRAME_TYPE frame, uint8_t* payload, uint8_t dataLength){
  putHeader(command, frame);
  if(dataLength > 0) putBytes(payload, dataLength);
  putFooter();
}

void afhds3::addAckToQueue(COMMAND command, uint8_t frameNumber) {
  request* r = new request(command, FRAME_TYPE::RESPONSE_ACK, nullptr, 0);
  r->frameNumber = frameNumber;
  commandQueue.push(r);
}

void afhds3::addToQueue(COMMAND command, FRAME_TYPE frameType, uint8_t* data, uint8_t dataLength) {

  request* r = new request(command, frameType, data, dataLength);
  commandQueue.push(r);
}

void afhds3::clearQueue() {
  while(!commandQueue.empty()) {
    request* r = commandQueue.front();
    delete r;
    commandQueue.pop();
  }
}

bool checkCRC(const uint8_t* data, uint8_t size)
{
  uint8_t crc = 0;
  //skip start byte
  for (uint8_t i = 1; i < size; i++) {
    crc += data[i];
  }
  return (crc ^ 0xff) == data[size];
}

bool containsData(enum FRAME_TYPE frameType) {
  return frameType == FRAME_TYPE::RESPONSE_DATA || frameType == FRAME_TYPE::REQUEST_SET_EXPECT_DATA ||
      frameType == FRAME_TYPE::REQUEST_SET_EXPECT_ACK || frameType == FRAME_TYPE::REQUEST_SET_EXPECT_DATA ||
      frameType == FRAME_TYPE::REQUEST_SET_NO_RESP;
}

void afhds3::setState(uint8_t state) {
  if(state == data->state) return;
  uint8_t oldState = data->state;
  data->state = state;
  if(oldState == ModuleState::STATE_BINDING) {
    if(operationCallback != nullptr) {
      operationCallback(false);
      operationCallback = nullptr;
    }
  }
  if(state == ModuleState::STATE_NOT_READY) {
    operationState = State::UNKNOWN;
  }
}

void afhds3::requestInfoAndRun(bool send) {
  if(!send) addToQueue(COMMAND::MODULE_VERSION, FRAME_TYPE::REQUEST_GET_DATA);
  addToQueue(COMMAND::MODULE_POWER_STATUS, FRAME_TYPE::REQUEST_GET_DATA);
  requestedModuleMode = MODULE_MODE_E::RUN;
  addToQueue(COMMAND::MODULE_MODE, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, &requestedModuleMode, 1);
  if(send) putFrame(COMMAND::MODULE_VERSION, FRAME_TYPE::REQUEST_GET_DATA);
}

void afhds3::parseData(uint8_t* rxBuffer, uint8_t rxBufferCount) {

  if(!checkCRC(rxBuffer, rxBufferCount - 2)) {
    TRACE("AFHDS3 [INVALID CRC]");
    return;
  }
  AfhdsFrame* responseFrame = reinterpret_cast<AfhdsFrame*>(rxBuffer);
  AfhdsFrame* requestFrame = reinterpret_cast<AfhdsFrame*>(data->pulses);
  uint8_t oldState = data->state;
  if(containsData((enum FRAME_TYPE)responseFrame->frameType)) {
    switch(responseFrame->command)
    {
      case COMMAND::MODULE_READY:
        TRACE("AFHDS3 [MODULE_READY] %02X", responseFrame->value);
        if(responseFrame->value == MODULE_STATUS_READY) {
          setState(ModuleState::STATE_READY);
          requestInfoAndRun();
        }
        else setState(ModuleState::STATE_NOT_READY);
        break;
      case COMMAND::MODULE_GET_CONFIG:
          std::memcpy((void*)cfg.buffer, &responseFrame->value, sizeof(cfg.buffer));
          TRACE("AFHDS3 [MODULE_GET_CONFIG] bpow %d rpow %d tele %d pwm/ppm %d ibus/sbus %d", cfg.config.bindPower, cfg.config.runPower, cfg.config.telemetry, cfg.config.pulseMode, cfg.config.serialMode);
          break;
      case COMMAND::MODULE_VERSION:
        std::memcpy((void*)&version, &responseFrame->value, sizeof(version));
        TRACE("AFHDS3 [MODULE_VERSION] Product %d, HW %d, BOOT %d, FW %d", version.productNumber, version.hardwereVersion, version.bootloaderVersion, version.firmwareVersion);
        break;
      case COMMAND::MODULE_POWER_STATUS:
        powerSource = (enum MODULE_POWER_SOURCE)responseFrame->value;
        TRACE("AFHDS3 [MODULE_POWER_STATUS], %d", powerSource);
        break;
      case COMMAND::MODULE_STATE:
        TRACE("AFHDS3 [MODULE_STATE] %02X", responseFrame->value);
        setState(responseFrame->value);
        break;
      case COMMAND::MODULE_MODE:
        TRACE("AFHDS3 [MODULE_MODE] %02X", responseFrame->value);
        if(responseFrame->value != CMD_RESULT::SUCCESS) {
            setState(ModuleState::STATE_NOT_READY);
        }
        else {
          if(requestedModuleMode == MODULE_MODE_E::RUN) {
            addToQueue(COMMAND::MODULE_GET_CONFIG, FRAME_TYPE::REQUEST_GET_DATA);
            addToQueue(COMMAND::MODULE_STATE, FRAME_TYPE::REQUEST_GET_DATA);
          }
          requestedModuleMode = MODULE_MODE_UNKNOWN;
        }
        break;
      case COMMAND::MODULE_SET_CONFIG:
        if(responseFrame->value != CMD_RESULT::SUCCESS) {
          setState(ModuleState::STATE_NOT_READY);
        }
        TRACE("AFHDS3 [MODULE_SET_CONFIG], %02X", responseFrame->value);
        break;
      case COMMAND::TELEMETRY_DATA:
      {
        uint8_t* telemetry = &responseFrame->value;

        if(telemetry[0] == 0x22) {
          telemetry++;
          while(telemetry < rxBuffer + rxBufferCount) {
            uint8_t length = telemetry[0];
            uint8_t id = telemetry[1];
            if(id == 0xFE) id = 0xF7;  //use new id because format is different
            if(length == 0 || telemetry + length > rxBuffer + rxBufferCount) break;
            if(length == 4) { //one byte value fill missing byte
              uint8_t data[] = { id, telemetry[2], telemetry[3], 0};
              processSensor(data, 0xAA);
            }
            if(length == 5) {
              if(id == 0xFA) telemetry[1] = 0xF8; //remap to afhds3 snr
              processSensor(telemetry + 1, 0xAA);
            }
            else if(length == 6 && id == FRM302_STATUS) {
              //convert to ibus
              uint16_t t = (uint16_t)(((int16_t)telemetry[3] *10) + 400);
              uint8_t dataTemp[] = { ++id, telemetry[2], (uint8_t)(t & 0xFF), (uint8_t)(t >> 8)};
              processSensor(dataTemp, 0xAA);
              uint8_t dataVoltage[] = { ++id, telemetry[2], telemetry[4], telemetry[5] };
              processSensor(dataVoltage, 0xAA);
            }
            else if(length == 7) processSensor(telemetry + 1, 0xAC);
            telemetry += length;
          }
        }
      }
      break;
      case COMMAND::COMMAND_RESULT:
      {
          AfhdsFrameData* respData = responseFrame->GetData();
          TRACE("COMMAND RESULT %02X result %d datalen %d", respData->CommandResult.command, respData->CommandResult.result, respData->CommandResult.respLen);
      }
      break;
    }
  }

  if(responseFrame->frameType == FRAME_TYPE::REQUEST_GET_DATA || responseFrame->frameType == FRAME_TYPE::REQUEST_SET_EXPECT_DATA)
  {
    TRACE("Command %02X NOT IMPLEMENTED!", responseFrame->command);
  }
  else if (responseFrame->frameType == FRAME_TYPE::REQUEST_SET_EXPECT_ACK) {
    //we need to respond now - it may break messaging context
    if(!commandQueue.empty()) { //check if such request is not queued
      request* r = commandQueue.front();
      if(r->command == (enum COMMAND)responseFrame->command &&
          r->frameType == FRAME_TYPE::RESPONSE_ACK &&
          r->frameNumber == responseFrame->frameNumber) return;
    }

    TRACE("SEND ACK cmd %02X type %02X %02X %02X result %02X", responseFrame->command, responseFrame->frameType, responseFrame->value, (*((&responseFrame->value)+1)), (*((&responseFrame->value)+2)));

    addAckToQueue((enum COMMAND)responseFrame->command, responseFrame->frameNumber);
    //not tested danger function
    //::sendExtModuleNow();
  }
  else if(responseFrame->frameType == FRAME_TYPE::RESPONSE_DATA || responseFrame->frameType == FRAME_TYPE::RESPONSE_ACK) {
    if(operationState == State::AWAITING_RESPONSE /* && requestFrame->command == responseFrame->command*/) {
      operationState = State::IDLE;
    }
  }
}

void afhds3::onDataReceived(uint8_t byte, uint8_t* rxBuffer, uint8_t& rxBufferCount, uint8_t maxSize) {
  if (rxBufferCount == 0 && byte != AfhdsSpecialChars::START) {
    TRACE("AFHDS3 [SKIP] %02X", byte);
    data->esc_state = 0;
    return;
  }

  if (byte == AfhdsSpecialChars::ESC) {
     data->esc_state = rxBufferCount;
     return;
  }

  if (rxBufferCount > 1 && byte == AfhdsSpecialChars::END) {
      rxBuffer[rxBufferCount++] = byte;
      parseData(rxBuffer, rxBufferCount);
      rxBufferCount = 0;
      return;
  }

  if (data->esc_state && byte == AfhdsSpecialChars::ESC_END) byte = AfhdsSpecialChars::END;
  else if (data->esc_state &&  byte == AfhdsSpecialChars::ESC_ESC) byte = AfhdsSpecialChars::ESC;
  //reset esc index
  data->esc_state = 0;

  if (rxBufferCount >= maxSize) {
    TRACE("AFHDS3 [BUFFER OVERFLOW]");
    rxBufferCount = 0;
  }
  rxBuffer[rxBufferCount++] = byte;
}
void afhds3::trace(const char* message) {
  char buffer[256];
  char *pos = buffer;
  for (int i = 0; i < data->ptr - data->pulses; i++) {
    pos += std::snprintf(pos, buffer + sizeof(buffer) - pos, "%02X ", data->pulses[i]);
  }
  (*pos) = 0;
  TRACE("%s size = %d data %s", message, data->ptr - data->pulses, buffer);
}

bool afhds3::isConnectedUnicast() {
  return cfg.config.telemetry == TELEMETRY::TELEMETRY_ENABLED && data->state == ModuleState::STATE_SYNC_DONE;
}
bool afhds3::isConnectedMulticast(){
  return cfg.config.telemetry == TELEMETRY::TELEMETRY_DISABLED && data->state == ModuleState::STATE_SYNC_RUNNING;
}

void afhds3::setupPulses() {
  //TRACE("%d state %d repeatCount %d", (int)operationState, this->data->state, repeatCount);
  if(operationState == State::AWAITING_RESPONSE) {
    if(repeatCount++ < 5) {
      TRACE("AFHDS3 [RESEND]");
      return; //re-send
    }
    else {
        TRACE("AFHDS3 [NO RESP] Frame %02X", data->pulses[3]);
        reset(false);
    }
  }
  else if(operationState == State::UNKNOWN){
    data->state = ModuleState::STATE_NOT_READY;
  }
  data->ptr = data->pulses;
  repeatCount = 0;
  if (data->state == ModuleState::STATE_NOT_READY) {
    TRACE("AFHDS3 [GET MODULE READY]");
    putFrame(COMMAND::MODULE_READY, FRAME_TYPE::REQUEST_GET_DATA);
    return;
  }

  //not allow more than 10 commands
  if(commandQueue.size() > 10) {
    clearQueue();
  }

  //check waiting commands
  if(!commandQueue.empty()) {
    request* r = commandQueue.front();
    commandQueue.pop();
    uint8_t frameIndexBackup = data->frame_index;
    if(r->frameNumber >= 0) {
      data->frame_index = r->frameNumber;
    }
    putFrame(r->command, r->frameType, r->payload, r->payloadSize);
    trace("AFHDS3 [CMD QUEUE] data");
    if(r->frameNumber >= 0) {
      data->frame_index = frameIndexBackup;
    }
    delete r;
    return;
  }
  
  if(syncSettings()) return;

  if(data->state == ModuleState::STATE_READY || data->state == ModuleState::STATE_STANDBY) {
    cmdCount = 0;
    repeatCount = 0;
    requestInfoAndRun(true);
    return;
  }

  bool isConnected = isConnectedUnicast() || isConnectedMulticast();
  if(cmdCount++ == 150)
  {
    cmdCount = 0;
    uint32_t max = sizeof(periodicRequestCommands);
    if(commandIndex == max) commandIndex = 0;
    COMMAND cmd = periodicRequestCommands[commandIndex];
    if(cmd == COMMAND::VIRTUAL_FAILSAFE)
    {
      if(isConnected) {
        if (isConnectedMulticast()) {
          TRACE("AFHDS ONE WAY FAILSAFE");
          uint16_t failSafe[MAX_CHANNELS + 1] = { ((MAX_CHANNELS << 8) | CHANNELS_DATA_MODE::FAIL_SAFE), 0 };
          setFailSafe((int16_t*) (&failSafe[1]));
          putFrame(COMMAND::CHANNELS_FAILSAFE_DATA, FRAME_TYPE::REQUEST_SET_NO_RESP, (uint8_t*) failSafe, MAX_CHANNELS * 2 + 2);
        }
        else {
          TRACE("AFHDS TWO WAYS FAILSAFE");
          uint8_t failSafe[3 + MAX_CHANNELS * 2] = { 0x11, 0x60, MAX_CHANNELS * 2 };
          setFailSafe((int16_t*) (failSafe + 3));
          putFrame(COMMAND::SEND_COMMAND, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, failSafe, 3 + MAX_CHANNELS * 2);
        }
      }
    }
    else
    {
      putFrame(cmd, FRAME_TYPE::REQUEST_GET_DATA);
    }
    //ensure commands will not be resend
    operationState = cmd == COMMAND::MODULE_STATE ? State::AWAITING_RESPONSE : State::IDLE;
    commandIndex++;
  }
  else if (isConnected)
  {
    sendChannelsData();
  }
}
RUN_POWER afhds3::getRunPower() {
  RUN_POWER targetPower = (RUN_POWER)moduleData->afhds3.runPower;
  if(getMaxRunPower() < targetPower)
    targetPower = getMaxRunPower();
  return targetPower;
}
bool afhds3::syncSettings() {
  RUN_POWER targetPower = getRunPower();
  if (targetPower != cfg.config.runPower) {
    cfg.config.runPower = targetPower;
    uint8_t data[] = { 0x13, 0x20, 0x02, targetPower, 0 };
    TRACE("AFHDS3 SET TX POWER %d", targetPower);
    putFrame(COMMAND::SEND_COMMAND, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, data, sizeof(data));
    return true;
  }

  //other settings only in 2 way mode (state must be synchronized)
  if(data->state != ModuleState::STATE_SYNC_DONE) return false;

  if(moduleData->afhds3.rxFreq != cfg.config.pwmFreq) {
    cfg.config.pwmFreq = moduleData->afhds3.rxFreq;
    uint8_t data[] = {0x17, 0x70, 0x02, (uint8_t)(moduleData->afhds3.rxFreq & 0xFF), (uint8_t)(moduleData->afhds3.rxFreq >> 8)};
    TRACE("AFHDS3 SET RX FREQ");
    putFrame(COMMAND::SEND_COMMAND, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, data, sizeof(data));
    return true;
  }
  PULSE_MODE modelPulseMode = moduleData->afhds3.isPWM() ? PULSE_MODE::PWM: PULSE_MODE::PPM;
  if(modelPulseMode != cfg.config.pulseMode) {
    cfg.config.pulseMode = modelPulseMode;
    TRACE("AFHDS3 PWM/PPM %d", modelPulseMode);
    uint8_t data[] = {0x16, 0x70, 0x01, (uint8_t)(modelPulseMode)};
    putFrame(COMMAND::SEND_COMMAND, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, data, sizeof(data));
    return true;
  }

  SERIAL_MODE modelSerialMode = moduleData->afhds3.isSbus() ? SERIAL_MODE::SBUS : SERIAL_MODE::IBUS;
  if(modelSerialMode != cfg.config.serialMode) {
    cfg.config.serialMode = modelSerialMode;
    TRACE("AFHDS3 IBUS/SBUS %d", modelSerialMode);
    uint8_t data[] = {0x18, 0x70, 0x01, (uint8_t)(modelSerialMode)};
    putFrame(COMMAND::SEND_COMMAND, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, data, sizeof(data));
    return true;
  }

  if(moduleData->afhds3.failsafeTimeout != cfg.config.failSafeTimout) {
    moduleData->afhds3.failsafeTimeout = cfg.config.failSafeTimout;
    uint8_t data[] = {0x12,  0x60, 0x02, (uint8_t)(moduleData->afhds3.failsafeTimeout & 0xFF), (uint8_t)(moduleData->afhds3.failsafeTimeout >> 8) };
    putFrame(COMMAND::SEND_COMMAND, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, data, sizeof(data));
    TRACE("AFHDS3 TRACE FAILSAFE TMEOUT");
    return true;
  }

  return false;
}

void afhds3::sendChannelsData() {
  uint16_t channels_start = moduleData->channelsStart;
  uint16_t channelsCount = 8 + moduleData->channelsCount;
  uint16_t channels_last = channels_start + channelsCount;

  int16_t buffer[MAX_CHANNELS + 1] = { ((MAX_CHANNELS << 8) | CHANNELS_DATA_MODE::CHANNELS), 0 };

  for(uint8_t channel = channels_start, index = 1; channel < channels_last; channel++, index++) {
    int16_t channelValue = convert(getChannelValue(channel));
    buffer[index] = channelValue;
  }
  putFrame(COMMAND::CHANNELS_FAILSAFE_DATA, FRAME_TYPE::REQUEST_SET_NO_RESP, (uint8_t*)buffer, sizeof(buffer));
}

void afhds3::bind(bindCallback_t callback) {
  operationCallback = callback;
  TRACE("AFHDS3 [BIND]");
  setModelData();
  addToQueue(COMMAND::MODULE_SET_CONFIG, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, cfg.buffer, sizeof(cfg.buffer));
  requestedModuleMode = MODULE_MODE_E::BIND;
  addToQueue(COMMAND::MODULE_MODE, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, &requestedModuleMode, 1);
}

void afhds3::range(bindCallback_t callback) {
  TRACE("AFHDS3 [RANGE CHECK] NOT IMPLEMENTED");
}

void afhds3::cancel() {
  if(operationCallback!=nullptr) operationCallback(false);
  if(data->state == ModuleState::STATE_BINDING && !cfg.config.telemetry) {
    setState(ModuleState::STATE_SYNC_DONE);
    requestedModuleMode = MODULE_MODE_E::RUN;
    addToQueue(COMMAND::MODULE_MODE, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, &requestedModuleMode, 1);
  }
  else reset(false);
}
void afhds3::stop() {
  TRACE("AFHDS3 STOP");
  requestedModuleMode = MODULE_MODE_E::STANDBY;
  //ensure this frame will be send out
  putFrame(COMMAND::MODULE_MODE, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, &requestedModuleMode, 1);
  reset(true);
}


void afhds3::setToDefault() {
  moduleData->afhds3.bindPower = BIND_POWER::MIN_0dbm;
  moduleData->afhds3.runPower = RUN_POWER::PLUS_15bBm;
  moduleData->afhds3.emi = EMI_STANDARD::FCC;
  moduleData->afhds3.telemetry = TELEMETRY::TELEMETRY_ENABLED;
  moduleData->afhds3.rxFreq = 50;
  moduleData->afhds3.failsafeTimeout = 1000;
  moduleData->channelsCount = 14 - 8;
  moduleData->failsafeMode = FAILSAFE_HOLD;
  //" PWM+i"" PWM+s"" PPM+i"" PPM+s"
  moduleData->afhds3.mode = 0;
  for (uint8_t channel = 0; channel < MAX_OUTPUT_CHANNELS; channel++) {
    moduleData->failsafeChannels[channel] = 0;
  }
}

RUN_POWER afhds3::getMaxRunPower()
{
  if(powerSource == MODULE_POWER_SOURCE::EXTERNAL) return RUN_POWER::PLUS_33dBm;
  return RUN_POWER::PLUS_20bBm;
}

RUN_POWER afhds3::actualRunPower()
{
  uint8_t actualRfPower = cfg.config.runPower;
  if(getMaxRunPower() < actualRfPower)
    actualRfPower = getMaxRunPower();
  return (RUN_POWER)actualRfPower;
}

int16_t afhds3::convert(int channelValue) {
  //pulseValue = limit<uint16_t>(0, 988 + ((channelValue + 1024) / 2), 0xfff);
  //988 - 750 = 238
  //238 * 20 = 4760
  //2250 - 2012 = 238
  //238 * 20 = 4760
  // 988   ---- 2012
  //-10240 ---- 10240
  //-1024  ---- 1024
  return limit<int16_t>(FAILSAFE_MIN, channelValue*10, FAILSAFE_MAX);
}
void afhds3::setModelData() {
  cfg.config.bindPower = moduleData->afhds3.bindPower;
  cfg.config.runPower = getRunPower();
  cfg.config.emiStandard = EMI_STANDARD::FCC;
  cfg.config.telemetry = moduleData->afhds3.telemetry;
  cfg.config.pwmFreq = moduleData->afhds3.rxFreq;
  cfg.config.serialMode = moduleData->afhds3.isSbus() ? SERIAL_MODE::SBUS: SERIAL_MODE::IBUS;
  cfg.config.pulseMode = moduleData->afhds3.isPWM() ? PULSE_MODE::PWM: PULSE_MODE::PPM;
  //use max channels - because channel count can not be changed after bind
  cfg.config.channelCount = MAX_CHANNELS;
  cfg.config.failSafeTimout = moduleData->afhds3.failsafeTimeout;
  setFailSafe(cfg.config.failSafeMode);
}
uint8_t afhds3::setFailSafe(int16_t* target) {
  int16_t pulseValue = 0;
  uint8_t channels_start = moduleData->channelsStart;
  uint8_t channels_last = channels_start + 8 + moduleData->channelsCount;

  for (uint8_t channel = channels_start; channel < channels_last; channel++) {
     if (moduleData->failsafeMode == FAILSAFE_CUSTOM) pulseValue = convert(moduleData->failsafeChannels[channel]);
     else if (moduleData->failsafeMode == FAILSAFE_HOLD) pulseValue = FAILSAFE_KEEP_LAST;
     else pulseValue = convert(getChannelValue(channel));
     target[channel-channels_start] = pulseValue;
   }
  //return max channels because channel count can not be change after bind
  return (uint8_t)(MAX_CHANNELS);
}




void afhds3::onModelSwitch() {
  //uint8_t cmd = MODULE_MODE_E::STANDBY;
  //putFrame(COMMAND::MODULE_MODE, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, &cmd, 1);
  setModelData();
}

void afhds3::reset(bool resetFrameCount) {
  TRACE("AFHDS3 RESET");
  clearQueue();
  repeatCount = 0;
  cmdCount = 0;
  setState(ModuleState::STATE_NOT_READY);
  this->data->frame_index = 1;
  this->data->timeout = 0;
  this->data->esc_state = 0;
}

}
