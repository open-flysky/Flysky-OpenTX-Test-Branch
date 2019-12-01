#include "afhds3.h"
#include "../debug.h"
namespace afhds3 {
#define frameType
std::list<CmdDesc> Afhds3Commands =
    {
        {   //4.1
        COMMAND::MODULE_READY, FRAME_TYPE::REQUEST_GET_DATA,
            DATA_TYPE::EMPTY_DT, FRAME_TYPE::RESPONSE_DATA, DATA_TYPE::READY_DT, },
        {   //4.2
        COMMAND::MODULE_STATE, FRAME_TYPE::REQUEST_GET_DATA,
            DATA_TYPE::EMPTY_DT, FRAME_TYPE::RESPONSE_DATA, DATA_TYPE::STATE_DT, },
        {   //4.2
        COMMAND::MODULE_STATE, FRAME_TYPE::REQUEST_SET_EXPECT_ACK,
            DATA_TYPE::STATE_DT, FRAME_TYPE::RESPONSE_DATA, DATA_TYPE::EMPTY_DT, },
        {   //4.3
        COMMAND::MODULE_MODE, FRAME_TYPE::REQUEST_SET_EXPECT_DATA,
            DATA_TYPE::MODE_DT, FRAME_TYPE::RESPONSE_DATA, DATA_TYPE::READY_DT, },
        {   //4.4
        COMMAND::MODULE_SET_CONFIG, FRAME_TYPE::REQUEST_SET_EXPECT_DATA,
            DATA_TYPE::MOD_CONFIG_DT, FRAME_TYPE::RESPONSE_DATA,
            DATA_TYPE::READY_DT, },
        {   //4.5
        COMMAND::MODULE_GET_CONFIG, FRAME_TYPE::REQUEST_GET_DATA,
            DATA_TYPE::EMPTY_DT, FRAME_TYPE::RESPONSE_DATA,
            DATA_TYPE::MOD_CONFIG_DT, },
        {   //4.6
        COMMAND::CHANNELS_FAILSAFE_DATA, FRAME_TYPE::REQUEST_SET_NO_RESP,
            DATA_TYPE::CHANNELS_DT, FRAME_TYPE::NOT_USED, DATA_TYPE::EMPTY_DT, },
        {   //4.7
        COMMAND::TELEMETRY_DATA, FRAME_TYPE::REQUEST_SET_NO_RESP,
            DATA_TYPE::TELEMETRY_DT, FRAME_TYPE::NOT_USED, DATA_TYPE::EMPTY_DT, },
        {   //4.9
        COMMAND::MODULE_POWER_STATUS, FRAME_TYPE::REQUEST_GET_DATA,
            DATA_TYPE::EMPTY_DT, FRAME_TYPE::RESPONSE_DATA,
            DATA_TYPE::MODULE_POWER_DT, }, {   //4.10
        COMMAND::MODULE_VERSION, FRAME_TYPE::REQUEST_GET_DATA,
            DATA_TYPE::EMPTY_DT, FRAME_TYPE::RESPONSE_DATA,
            DATA_TYPE::MODULE_POWER_DT, }, };



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
  this->data->state = State::SENDING_COMMAND;
  this->data->ptr = this->data->pulses;
  this->data->crc = 0;
  *this->data->ptr++ = END;
  uint8_t data[] = { FrameAddress, this->data->frame_index, frame, command};
  putBytes(data, 4);
}


void afhds3::putFooter() {
  putByte(this->data->crc ^ 0xff);
  *this->data->ptr++ = END;
  this->data->frame_index++;

  switch((FRAME_TYPE)this->data->pulses[2])
  {
    case FRAME_TYPE::REQUEST_GET_DATA:
    case FRAME_TYPE::REQUEST_SET_EXPECT_ACK:
    case FRAME_TYPE::REQUEST_SET_EXPECT_DATA:
      this->data->state = State::AWAITING_RESPONSE;
    default:
      this->data->state = State::IDLE;
  }
}

void afhds3::putFrame(COMMAND command, FRAME_TYPE frame, uint8_t* data, uint8_t dataLength){
  putHeader(command, frame);
  if(dataLength > 0) putBytes(data, dataLength);
  putFooter();
}

void afhds3::onDataReceived(uint8_t data, uint8_t* rxBuffer, uint8_t& rxBufferCount) {
  TRACE("RX data %d", data);
}

void afhds3::setupPulses() {
  if(operationState == State::AWAITING_RESPONSE) {
    if(repeatCount++ < 5) return; //re-send
  }

  switch ((State)this->data->state) {
    case State::UNKNOWN:
      putFrame(COMMAND::MODULE_READY, FRAME_TYPE::REQUEST_GET_DATA, nullptr, 0);
      break;
  }
}



void afhds3::reset() {
  this->data->state = State::UNKNOWN;
  this->data->frame_index = 1;
  this->data->timeout = 0;
  this->data->esc_state = 0;
  uint16_t rx_freq = moduleData->romData.rx_freq[0] | (moduleData->romData.rx_freq[0] << 8);
  if (MIN_FREQ > rx_freq || MAX_FREQ < rx_freq) {
    moduleData->romData.rx_freq[0] = MIN_FREQ;
    moduleData->romData.rx_freq[1] = 0;
  }
}

}
