#include "opentx.h"
#include <string>
namespace btle {


const char* const tx_power_map[] = {
  "8 dBm",
  "4 dBm",
  "0 dBm",
  "-4 dBm",
  "-10 dBm",
  "-14 dBm",
  "-20 dBm",
};

inline size_t frameSize(Request* frame) {
  return frame->length + (frame->data.lierda.bytes - reinterpret_cast<uint8_t*>(frame));
}


Request* initReqest(uint8_t* frame, Command command, size_t len) {
  Request* cmd = reinterpret_cast<Request*>(frame);
  cmd->start = bswapu16((uint16_t)REQUEST);
  cmd->command = command;
  cmd->length = len;
  return cmd;
}

bool valid(ResponseFoxware* frame, Command cmd, size_t length) {
  if (!length) return false;
  if (frame->start != bswapu16(REPONSE)) {
    TRACE("INVALID START %04X", frame->start);
    return false;
  }

  for (unsigned i = 0; i < DIM(commands_with_ack_only); i++) {
    if (commands_with_ack_only[i] == cmd) {   //0x04, 0xFC, 0x01, 0x00
      bool valid = frame->command == RES_FOX_VALID && frame->length == 0 && length == sizeof(correct_cmd);
      if (!valid) TRACE("INVALID RESPONSE");
      return valid;
    }
  }

  if (frame->command != cmd) {
    TRACE("INVALID CMD %02X", frame->command);
    return false;
  }

  //corret command with data 0x04, 0xFC, CMD, LENGTH, DATA

  if ((size_t)(frame->length + 4) != length) {
    TRACE("INVALID LENGTH %02", frame->length);
    return false;
  } 
  return true;
} 

uint32_t transmit_time_ms(size_t frameSize, Baudrate baudrate, DeviceType device){
  int mcuDelayMs = IdleTimeUs[(uint32_t)baudrate] / 1000;
  if (mcuDelayMs < BT_MIN_IDLE_TIME) mcuDelayMs = BT_MIN_IDLE_TIME;
  uint32_t d = (uint32_t)device;
  uint16_t I = (CIMins[d] + CIMaxs[d]) / 2; 
  return mcuDelayMs + ((frameSize * (I+1))/60);
}

size_t baudrate(uint8_t* frame) {
  auto cmd = initReqest(frame, GET_FOX_BAUDRATE, 0);
  return frameSize(cmd);
}

size_t set_baudrate(uint8_t* frame, Baudrate baudrate) {
  auto cmd = initReqest(frame, SET_FOX_BAUDRATE, sizeof(Baudrate));
  cmd->data.foxware.baudrate = baudrate;
  return frameSize(cmd);
}

size_t advertising_interval(uint8_t* frame) {
  auto cmd = initReqest(frame, GET_FOX_ADVERTISING_INTERNVAL, 0); 
  return frameSize(cmd);
}

//internval * 1.5ms
size_t set_advertising_interval(uint8_t* frame, uint16_t interval) {
  if (interval < 6) interval = 6;
  if (interval > 1600) interval = 1600;
  auto cmd = initReqest(frame, SET_FOX_ADVERTISING_INTERNVAL, sizeof(uint16_t)); 
  cmd->data.foxware.internval = interval; //make sure it is little endian
  return frameSize(cmd);
}

size_t passcode(uint8_t* frame) {
  auto cmd = initReqest(frame, GET_FOX_PASSCODE, 0);
  return frameSize(cmd);
}

size_t set_passcode(uint8_t* frame, uint32_t code) {
  if (!code) code = 1;
  if (code > 999999) code = 999999;
  char passcode[FOXWARE_MAX_PASSCODE_LEN + 1];
  sprintf(passcode, "%06lu", code);
  auto cmd = initReqest(frame, SET_FOX_PASSCODE, FOXWARE_MAX_PASSCODE_LEN);
  strncpy(cmd->data.foxware.passcode, passcode, FOXWARE_MAX_PASSCODE_LEN);
  return frameSize(cmd);
}


size_t mac(uint8_t* frame) {
  auto cmd = initReqest(frame, GET_FOX_MAC, 0);
  return frameSize(cmd);
}

size_t name(uint8_t* frame) {
  auto cmd = initReqest(frame, GET_FOX_NAME, 0);
  return frameSize(cmd);
}

size_t set_name(uint8_t* frame, const char* name) {
  size_t len = strlen(name);
  if (len > FOXWARE_MAX_NAME_LEN) len = FOXWARE_MAX_NAME_LEN;

  auto cmd = initReqest(frame, SET_FOX_NAME, len);
  strncpy(cmd->data.foxware.name, name, len);
  return frameSize(cmd);
}

size_t passcode_protection(uint8_t* frame) {
  auto cmd = initReqest(frame, GET_FOX_PASSCODE_PROTECTION, 0);
  return frameSize(cmd);
}

size_t set_passcode_protection(uint8_t* frame, bool enabled) {
  auto cmd = initReqest(frame, SET_FOX_PASSCODE_PROTECTION, 1);
  cmd->data.foxware.bytes[0] = enabled ? 0x02: 0x01;
  return frameSize(cmd);
}

size_t broadcast_interval(uint8_t* frame) {
  auto cmd = initReqest(frame, GET_FOX_BROADCAST_INTERNVAL, 0); 
  return frameSize(cmd);
}

//internval * 625μs - 32(20ms)~8000(5s)
size_t set_broadcast_interval(uint8_t* frame, uint16_t interval) {
  if (interval < 32) interval = 32;
  if (interval > 8000) interval = 8000;

  auto cmd = initReqest(frame, SET_FOX_BROADCAST_INTERNVAL, sizeof(uint16_t)); 
  cmd->data.foxware.internval = interval; //make sure it is little endian
  return frameSize(cmd);
}

size_t tx_power(uint8_t* frame) {
  auto cmd = initReqest(frame, GET_FOX_TX_POWER, 0);
  return frameSize(cmd);
}

size_t set_tx_power(uint8_t* frame, TxPower power) {
  auto cmd = initReqest(frame, SET_FOX_TX_POWER, sizeof(TxPower));
  cmd->data.foxware.power = power;
  return frameSize(cmd);
}

size_t factory_settings(uint8_t* frame) {
  auto cmd = initReqest(frame, SET_FOX_FACTORY_SETTINGS, 0);
  return frameSize(cmd);
}

size_t reset(uint8_t* frame) {
  auto cmd = initReqest(frame, SET_FOX_RESET, 0);
  return frameSize(cmd);
}

size_t fw_version(uint8_t* frame) {
  auto cmd = initReqest(frame, GET_FW_VERSION, 0);
  return frameSize(cmd);
}

typedef size_t (*load_cmd_func_t)(uint8_t* frame);

struct desc {
  Command cmd;
  load_cmd_func_t function;
};

void configFoxware::reset() {
  TRACE("reset");
  next = -1;
};

void configFoxware::preLoad() {
  TRACE("preLoad");
  next = 0;
};

void configFoxware::preSave() {
  TRACE("preSave");
  next = 0;
  retry = 0;
  dirty = false;
};
      
const desc loadList[] =  {
  { Command::GET_FOX_BAUDRATE, baudrate },
  { Command::GET_FOX_ADVERTISING_INTERNVAL, advertising_interval},
  { Command::GET_FOX_PASSCODE, passcode},
  { Command::GET_FOX_MAC, mac},
  { Command::GET_FOX_NAME, name },
  { Command::GET_FOX_PASSCODE_PROTECTION, passcode_protection },
  { Command::GET_FOX_BROADCAST_INTERNVAL, broadcast_interval },
  { Command::GET_FOX_TX_POWER, tx_power },
  { Command::GET_FW_VERSION, fw_version },
 };

 const desc saveList[] =  {
  { Command::SET_FOX_ADVERTISING_INTERNVAL, nullptr },
  { Command::SET_FOX_PASSCODE_PROTECTION, nullptr},
  { Command::SET_FOX_PASSCODE, nullptr},
  { Command::SET_FOX_NAME, nullptr},
  { Command::SET_FOX_BROADCAST_INTERNVAL, nullptr },
  { Command::SET_FOX_TX_POWER, nullptr },
  { Command::SET_FOX_BAUDRATE, nullptr },
 };


ResponseStatus configFoxware::response(btle::ResponseFoxware* response, size_t size) {
  TRACE("LOAD RESP");
  if (next >= (int)(sizeof(loadList)/sizeof(loadList[0]))) {
    next = -1;
    return ResponseStatus::Done;
  }
  if (!valid(response, loadList[next].cmd, size)) {
    TRACE("INVALID FRAME %02X", loadList[next].cmd);
    if (++retry == 3) {
      retry = 0;
      next++;
    }
    return ResponseStatus::ResponseFailed;
  }
  switch(loadList[next].cmd) {
    case Command::GET_FOX_BAUDRATE: 
      TRACE("baudrate %d", response->data.baudrate);
      this->baudrate = response->data.baudrate;
    break;
    case Command::GET_FOX_ADVERTISING_INTERNVAL: 
      TRACE("advertising_interval %d", response->data.internval);
      this->advertising_interval = response->data.internval;
    break;
    case Command::GET_FOX_PASSCODE: 
      //make sure it is string
      response->data.passcode[response->length] = 0;
      this->passcode = (uint32_t)std::stoi(response->data.passcode);
      TRACE("passcode %06d", this->passcode);
      break;
    case Command::GET_FOX_MAC: 
      sprintf(this->mac, "%02X:%02X:%02X:%02X:%02X:%02X", response->data.bytes[0], response->data.bytes[1], response->data.bytes[2], response->data.bytes[3], response->data.bytes[4], response->data.bytes[5]);
      TRACE("mac %d", this->mac);
      break;
    case Command::GET_FOX_NAME: 
      response->data.name[response->length] = 0;
      if (response->length > FOXWARE_MAX_NAME_LEN - 1)  {
        response->length = FOXWARE_MAX_NAME_LEN - 1;
      }
      strncpy(this->name, response->data.name, response->length);
      TRACE("name %d", this->name);
    break;
    case Command::GET_FOX_PASSCODE_PROTECTION: 
      this->passcode_protection = response->data.bytes[0] == 0x02;
      TRACE("passcode_protection %d", this->passcode_protection);
      break;
    case Command::GET_FOX_BROADCAST_INTERNVAL: 
      this->broadcast_interval = response->data.internval;
      TRACE("broadcast_interval %d", this->broadcast_interval);
      break;
    case Command::GET_FOX_TX_POWER: 
      this->tx_power = response->data.power;
      TRACE("power %d", this->tx_power);
      break;
    case Command::GET_FW_VERSION: 
      this->fw_version = ((uint16_t)response->data.bytes[0] << 8) | ((uint16_t)response->data.bytes[1]);
      TRACE("fw %02X.%02X", (this->fw_version & 0xFF00) >> 8, this->fw_version & 0xFF);
      break;
    default:
      return ResponseStatus::ResponseFailed;
    }
    if (++next == sizeof(loadList)/sizeof(loadList[0])) {
      TRACE("LOAD DONE");
      return ResponseStatus::Done;
    }
    return ResponseStatus::ResponseSuccess;
}

ResponseStatus configFoxware::responseSave(btle::ResponseFoxware* response, size_t size) {
  TRACE("SAVE RESP [%d] FRAME %02X", next, saveList[next].cmd);
  bool isValid = valid(response, saveList[next].cmd, size);
  if (isValid) {
    retry = 0;
    next++;
  } else {
    TRACE("SAVE [%d] size [%d] INVALID FRAME %02X retry %d", next, size, saveList[next].cmd, retry);
    for (size_t i = 0; i < size; i++) {
      TRACE("%02X", (reinterpret_cast<uint8_t*>(response))[i]);
    }
    if (++retry == 3) {
      retry = 0;
      next++;
    }
  }
  if (next >= (int)(sizeof(saveList)/sizeof(saveList[0]))) {
    TRACE("SAVE DONE");
    return ResponseStatus::Done;
  }
  return isValid ? ResponseStatus::ResponseSuccess : ResponseStatus::ResponseFailed;
}

size_t configFoxware::load(uint8_t* frame) {
  size_t numberItems = sizeof(loadList)/sizeof(loadList[0]);
  if (next >= (int)numberItems || next < 0) {
    return 0;
  }
  return loadList[next].function(frame);
}

size_t configFoxware::save(uint8_t* frame, bool* isBaudUpdate) {
  *isBaudUpdate = false;
  size_t numberItems = sizeof(saveList)/sizeof(saveList[0]);
  if (next >= (int)numberItems || next < 0) {
    return 0;
  }
  TRACE("SAVE [%d] %02X", next, saveList[next].cmd);
  switch(saveList[next].cmd) {
    case Command::SET_FOX_ADVERTISING_INTERNVAL:
      return set_advertising_interval(frame, this->advertising_interval);
    break;
    case Command::SET_FOX_PASSCODE:
      return set_passcode(frame, this->passcode);
    break;
    case Command::SET_FOX_NAME:
      return set_name(frame, this->name);
    break;
    case Command::SET_FOX_PASSCODE_PROTECTION:
      return set_passcode_protection(frame, this->passcode_protection);
    break;
    case Command::SET_FOX_BROADCAST_INTERNVAL:
      return set_broadcast_interval(frame, this->broadcast_interval);
    break;
    case Command::SET_FOX_TX_POWER:
      return set_tx_power(frame, this->tx_power);
    break;
    case Command::SET_FOX_BAUDRATE:
      *isBaudUpdate = true;
      return set_baudrate(frame, this->baudrate);
    break;
  }
  return 0;
}


}
