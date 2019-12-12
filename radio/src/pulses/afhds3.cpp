#include "afhds3.h"
#include "../debug.h"
#include "../definitions.h"

#define FAILSAFE_HOLD 1
#define FAILSAFE_CUSTOM 2
namespace afhds3 {


uint8_t ModuleModeToState [] = {
    ModuleState::STATE_READY,   //not defined
    ModuleState::STATE_STANDBY, //MODULE_MODE_E::STANDBY
    ModuleState::STATE_BINDING, //MODULE_MODE_E::BIND
    ModuleState::STATE_UPDATING_RX, //MODULE_MODE_E::RX_UPDATE
    ModuleState::STATE_SYNC_DONE, //MODULE_MODE_E::RUN
};

static const char* const moduleStateText[] = {
   "",
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
   "Not ready",
   "Ready",
   "HW test"
};

const char* afhds3::getState() {
  if(data->state <= ModuleState::STATE_READY) return moduleStateText[data->state];
  return "Unknown";
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

void afhds3::putFrame(COMMAND command, FRAME_TYPE frame, uint8_t* data, uint8_t dataLength){
  putHeader(command, frame);
  if(dataLength > 0) putBytes(data, dataLength);
  putFooter();
  // trace("FRAME");
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
      frameType == FRAME_TYPE::REQUEST_SET_EXPECT_ACK || frameType == FRAME_TYPE::REQUEST_SET_EXPECT_DATA;
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
        setState(responseFrame->value == MODULE_STATUS_READY ? ModuleState::STATE_READY : ModuleState::STATE_NOT_READY);
        if(data->state == ModuleState::STATE_READY && oldState != data->state) {
          addToQueue(COMMAND::MODULE_VERSION, FRAME_TYPE::REQUEST_GET_DATA);
          addToQueue(COMMAND::MODULE_POWER_STATUS, FRAME_TYPE::REQUEST_GET_DATA);
          addToQueue(COMMAND::MODULE_GET_CONFIG, FRAME_TYPE::REQUEST_GET_DATA);
          uint8_t mode = MODULE_MODE_E::RUN;
          addToQueue(COMMAND::MODULE_MODE, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, &mode, 1);
          //addToQueue(COMMAND::MODULE_STATE, FRAME_TYPE::REQUEST_GET_DATA);
        }
        break;
      case COMMAND::MODULE_GET_CONFIG:
          TRACE("AFHDS3 [MODULE_GET_CONFIG]");
          std::memcpy((void*)cfg.buffer, &responseFrame->value, sizeof(cfg.buffer));
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
        else addToQueue(COMMAND::MODULE_STATE, FRAME_TYPE::REQUEST_GET_DATA);

        break;
      case COMMAND::MODULE_SET_CONFIG:
        if(responseFrame->value != CMD_RESULT::SUCCESS) {
          setState(ModuleState::STATE_NOT_READY);
        }
        TRACE("AFHDS3 [MODULE_SET_CONFIG], %02X", responseFrame->value);
        break;
      case COMMAND::TELEMETRY_DATA:
        //parse telemetry
        break;
    }
  }

  if(responseFrame->frameType == FRAME_TYPE::REQUEST_GET_DATA || responseFrame->frameType == FRAME_TYPE::REQUEST_SET_EXPECT_DATA)
  {
    TRACE("Command %02X NOT IMPLEMENTED!", responseFrame->command);
  }
  else if (responseFrame->frameType == FRAME_TYPE::REQUEST_SET_EXPECT_ACK) {
    addToQueue((enum COMMAND)responseFrame->command, FRAME_TYPE::RESPONSE_ACK);
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
  char buffer[128];
  char *pos = buffer;
  for (int i = 0; i < data->ptr - data->pulses; i++) {
    pos += std::snprintf(pos, buffer + sizeof(buffer) - pos, "%02X ",
        data->pulses[i]);
  }
  (*pos) = 0;
  TRACE("%s %s", message, buffer);
}
void afhds3::setupPulses() {
  //TRACE("%d state %d repeatCount %d", (int)operationState, this->data->state, repeatCount);
  if(operationState == State::AWAITING_RESPONSE) {
    if(repeatCount++ < 5) return; //re-send
    else reset(false);
  }
  data->ptr = data->pulses;

  if (operationState == State::UNKNOWN || data->state == ModuleState::STATE_NOT_READY) {
    putFrame(COMMAND::MODULE_READY, FRAME_TYPE::REQUEST_GET_DATA);
    return;
  }


  //check waiting commands
  if(!commandQueue.empty()) {
    request* r = commandQueue.front();
    commandQueue.pop();
    putFrame(r->command, r->frameType, r->payload, r->payloadSize);
    trace("AFHDS3 [CMD QUEUE] data");
    delete r;
    return;
  }

  //config should be loaded already
  if(syncSettings()) return;

  switch(data->state) {
  	case ModuleState::STATE_READY:
  		reset();
  		break;
    case ModuleState::STATE_HW_ERROR:
    case ModuleState::STATE_BINDING:
    case ModuleState::STATE_UPDATING_RX:
    case ModuleState::STATE_UPDATING_WAIT:
    case ModuleState::STATE_UPDATING_MOD:
    case ModuleState::STATE_SYNC_RUNNING:
    case ModuleState::STATE_HW_TEST:
        if(idleCount++ % 100 == 0) {
            putFrame(COMMAND::MODULE_STATE, FRAME_TYPE::REQUEST_GET_DATA);
        }
        break;
     case ModuleState::STATE_SYNC_DONE:
       /*
       if(false && repeatCount++ % 200 == 0) {
    	 uint8_t failSafe[3+ MAX_CHANNELS*2];
         uint8_t channels = setFailSafe((int16_t*)failSafe + 3);
         failSafe[0] = 0x60;
         failSafe[1] = 0x12;
         failSafe[2] = channels *2;
         putFrame(COMMAND::SEND_COMMAND, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, failSafe, channels*2 + 3);
         trace("AFHDS3 [AFHDS3 SET FAILSAFE] data");
       }
	   */
       sendChannelsData();
      break;
  }
}

bool afhds3::syncSettings() {
  if (moduleData->afhds3.runPower != cfg.config.runPower) {
    cfg.config.runPower = moduleData->afhds3.runPower;
    uint8_t data[] = { 0x20, 0x13, 0x01, moduleData->afhds3.runPower };
    TRACE("AFHDS3 SET TX POWER %d", moduleData->afhds3.runPower);
    putFrame(COMMAND::SEND_COMMAND, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, data, sizeof(data));
    return true;
  }
  if(moduleData->afhds3.rxFreq != cfg.config.pwmFreq) {
    cfg.config.pwmFreq = moduleData->afhds3.rxFreq;
    uint8_t data[] = {0x70, 0x17, 0x02, (uint8_t)(moduleData->afhds3.rxFreq >> 8), (uint8_t)(moduleData->afhds3.rxFreq & 0xFF)};
    TRACE("AFHDS3 SET RX FREQ");
    putFrame(COMMAND::SEND_COMMAND, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, data, sizeof(data));
    return true;
  }
  if(moduleData->afhds3.mode != cfg.config.pulseMode) {
    cfg.config.pulseMode = moduleData->afhds3.mode;
    uint8_t data[] = {0x70, 0x16, 0x01, (uint8_t)(moduleData->afhds3.mode < 2 ? PULSE_MODE::PWM: PULSE_MODE::PPM)};
    TRACE("AFHDS3 SET PWM/PPM");
    putFrame(COMMAND::SEND_COMMAND, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, data, sizeof(data));
    uint8_t data2[] = {0x70, 0x18, 0x01, (uint8_t)(moduleData->afhds3.mode & 1 ? SERIAL_MODE::SBUS : SERIAL_MODE::IBUS)};
    TRACE("AFHDS3 QUEUE IBUS/SBUS");
    addToQueue(COMMAND::SEND_COMMAND, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, data2, sizeof(data2));
    return true;
  }
  if(moduleData->afhds3.failsafeTimeout != cfg.config.failSafeTimout) {
    moduleData->afhds3.failsafeTimeout = cfg.config.failSafeTimout;
	uint8_t data[] = { 0x60, 0x12, 0x02, (uint8_t)(moduleData->afhds3.failsafeTimeout >> 8), (uint8_t)(moduleData->afhds3.failsafeTimeout & 0xFF) };
	putFrame(COMMAND::SEND_COMMAND, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, data, sizeof(data));
	TRACE("AFHDS3 TRACE FAILSAFE TMEOUT");
	return true;
  }

  return false;
}

void afhds3::sendChannelsData() {
  uint8_t channels_start = moduleData->channelsStart;
  uint8_t channelsCount = 8 + moduleData->channelsCount;
  uint8_t channels_last = channels_start + channelsCount;

  uint8_t channels[2*((8 + moduleData->channelsCount) + 1)];
  channels[0] = 0x01;
  channels[1] = channelsCount;

  for(uint8_t channel = channels_start; channel < channels_last; channel++) {
    int16_t channelValue = convert(getChannelValue(channel));
    *((int16_t*)(channels + (channel * 2) + 2)) = channelValue;
  }
  putFrame(COMMAND::CHANNELS_FAILSAFE_DATA, FRAME_TYPE::REQUEST_SET_NO_RESP, channels, sizeof(channels));
  static uint32_t count = 0;
  if(!(count++ % 200)) {

  }

}

void afhds3::bind(bindCallback_t callback) {
  operationCallback = callback;
  TRACE("AFHDS3 [BIND]");
  setConfigFromModelData();
  addToQueue(COMMAND::MODULE_SET_CONFIG, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, cfg.buffer, sizeof(cfg.buffer));
  uint8_t cmd = MODULE_MODE_E::BIND;
  addToQueue(COMMAND::MODULE_MODE, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, &cmd, 1);
}

void afhds3::range(bindCallback_t callback) {
  TRACE("AFHDS3 [RANGE CHECK] NOT IMPLEMENTED");
}

void afhds3::cancel() {
  if(operationCallback!=nullptr) operationCallback(false);
  reset(false);
}
void afhds3::stop() {
  TRACE("AFHDS3 STOP");
  uint8_t cmd = MODULE_MODE_E::STANDBY;
  putFrame(COMMAND::MODULE_MODE, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, &cmd, 1);
  reset(true);
}


void afhds3::setConfigToDefault() {
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
int16_t afhds3::convert(int channelValue) {
  return limit<int16_t>(FAILSAFE_MIN, ((channelValue + 1024) * 30000 / 2048) - 15000, FAILSAFE_MAX);
}
void afhds3::setConfigFromModelData() {
  cfg.config.bindPower = moduleData->afhds3.bindPower;
  cfg.config.runPower = moduleData->afhds3.runPower;
  cfg.config.emiStandard = moduleData->afhds3.emi;
  cfg.config.telemetry = moduleData->afhds3.telemetry;
  cfg.config.pwmFreq = moduleData->afhds3.rxFreq;
  cfg.config.serialMode = moduleData->afhds3.isSbus() ? SERIAL_MODE::SBUS: SERIAL_MODE::IBUS;
  cfg.config.pulseMode = moduleData->afhds3.isPWM() ? PULSE_MODE::PWM: PULSE_MODE::PPM;
  cfg.config.channelCount = 8 + moduleData->channelsCount;
  cfg.config.failSafeTimout = moduleData->afhds3.failsafeTimeout;
  setFailSafe(cfg.config.failSafeMode);

}
uint8_t afhds3::setFailSafe(int16_t* target) {
  int16_t pulseValue = 0;
  uint8_t channels_start = moduleData->channelsStart;
  uint8_t channels_last = channels_start + 8 + moduleData->channelsCount;;

  for (uint8_t channel = channels_start; channel < channels_last; channel++) {
     if (moduleData->failsafeMode == FAILSAFE_CUSTOM) pulseValue = convert(moduleData->failsafeChannels[channel]);
     else if (moduleData->failsafeMode == FAILSAFE_HOLD) pulseValue = FAILSAFE_KEEP_LAST;
     else pulseValue = convert(getChannelValue(channel));
     target[channel-channels_start] = pulseValue;
   }
  return (uint8_t)(channels_last - channels_start);
}


void afhds3::onModelSwitch() {
  uint8_t cmd = MODULE_MODE_E::STANDBY;
  putFrame(COMMAND::MODULE_MODE, FRAME_TYPE::REQUEST_SET_EXPECT_DATA, &cmd, 1);
  setConfigFromModelData();
}

void afhds3::reset(bool resetFrameCount) {
  TRACE("AFHDS3 RESET");
  clearQueue();
  repeatCount = 0;
  this->data->state = ModuleState::STATE_NOT_READY;
  this->data->frame_index = 1;
  this->data->timeout = 0;
  this->data->esc_state = 0;
}

}
