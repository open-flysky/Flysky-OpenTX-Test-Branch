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
#include "nv14_internal_module_update.h"
#include "hallStick_parser.h"

#define MAX_ATTEMPTS 5

static const Nv14UpdateDriver nv14UpdateDriver;
static const char* ReadError = "Error opening file";
static hallStickParser parser;

bool Nv14FirmwareInformation::valid() const {
  return crcValid;
}

const char * Nv14FirmwareInformation::read(const char * filename) {
  FIL file;
  if (f_open(&file, filename, FA_READ) != FR_OK) return ReadError;
  UINT size = f_size(&file) - 2;
  uint8_t buffer[128];
  UINT readTotal = 0;
  UINT readCount = 0;
  UINT remaining = 0;
  UINT fragmentSize = 0;
  uint16_t crcResult = 0xffff;
  while(readTotal < size) {
    remaining = size - readTotal;
    fragmentSize = sizeof(buffer);
    if (remaining < sizeof(buffer)) {
      fragmentSize = remaining;
    }
    if (f_read(&file, buffer, fragmentSize, &readCount) != FR_OK) {
      return ReadError;
    }
    readTotal += readCount;
    crcResult = crc16(CRC_1021, (const uint8_t*)buffer, fragmentSize, crcResult);
  }

  if (f_read(&file, buffer, 2, &readCount) != FR_OK) {
    return ReadError;
  }
  crcValid = (((uint16_t)buffer[1]) << 8 | (uint16_t)buffer[0]) == crcResult;

  TRACE("CRC 0x%04x valid %d", crcResult, crcValid);
  f_close(&file);

  return nullptr;
}

void Nv14UpdateDriver::sendPacket(STRUCT_HALL* tx, uint8_t senderID, uint8_t receiverID, uint8_t packetID, uint8_t* payload, uint16_t length) const {
  tx->head = StartByte;
  tx->hallID.hall_Id.senderID = senderID;
  tx->hallID.hall_Id.receiverID = receiverID;
  tx->hallID.hall_Id.packetID = packetID;
  tx->length = length;
  if (payload == nullptr || length > HALLSTICK_BUFF_SIZE) {
    return;
  }
  TRACE("TX PacketID %d Type %d Sender %02X Reciever %02X Length %d", tx->hallID.hall_Id.packetID, tx->data[0], senderID, receiverID, length);
	memcpy(tx->data, payload, length);
  *((uint16_t*)(tx->data+length)) = crc16(CRC_1021, (const uint8_t*)tx, length + 3, 0xffff);
  intmoduleSendBufferDMA((uint8_t*)tx, length + 5);
  
  //debug((uint8_t*)tx, length + 5);
}

void Nv14UpdateDriver::debug(const uint8_t* rxBuffer, uint8_t rxBufferCount) const {
  if(rxBufferCount > 50) {
    rxBufferCount = 50;
  }
  // debug print the content of the packet
  char buffer[512];
  char* pos = buffer;
  for (int i=0; i < rxBufferCount; i++) {
    pos += snprintf(pos, buffer + sizeof(buffer) - pos, "%02X", rxBuffer[i]);
  }
  (*pos) = 0;
  TRACE("%s", buffer);
}

bool Nv14UpdateDriver::getBootloaderResponse(STRUCT_HALL* rx, uint16_t timeoutMs, bool checkReceiverID) const {
  uint16_t time = getTmr2MHz();
  while ((uint16_t)(getTmr2MHz() - time) < ((timeoutMs *2)*1000)) {
    uint8_t byte = 0;
    if(intmoduleGetByte(&byte)) {
      parser.parse(rx, byte);
      if (rx->valid) { 
        rx->valid = false;
        TRACE("RX PacketID %d Type %d Sender %02X Reciever %02X Length %d", rx->hallID.hall_Id.packetID, rx->data[0], rx->hallID.hall_Id.senderID, rx->hallID.hall_Id.receiverID, rx->length);
        //debug((uint8_t*)rx, rx->length + 5);
        //if (checkReceiverID && rx->hallID.hall_Id.receiverID != RemoteController) continue;
        return true;
      }
    }
  }
  return false;
}

void Nv14UpdateDriver::sendModuleCommand(uint8_t type, uint8_t cmd) const {
  afhds2Command(type, cmd);
  uint8_t* data = intmodulePulsesData.flysky.pulses;
  uint16_t size = intmodulePulsesData.flysky.ptr - data;
  intmoduleSendBufferDMA(data, size);
  //(data, size);
}

bool Nv14UpdateDriver::getModuleResponse(uint8_t* data, uint16_t maxSize, uint16_t timeoutMs) const {
  uint16_t index = 0;
  bool escape = false;
  uint16_t timeout = getTmr2MHz() + (timeoutMs *2)*1000;
  while (getTmr2MHz() < timeout) {
    uint8_t byte = 0;
    if(!intmoduleGetByte(&byte)) continue;
    if (byte == END && index > 0) {
      return true;
    } else {
      if (byte == ESC) escape = true;
      else {
        if (escape) {
          escape = false;
          if (byte == ESC_END) byte = END;
          else if (byte == ESC_ESC) byte = ESC;
        }
      }
      data[index++] = byte;
      if (index >= maxSize) index = 0;
    }
  }
  return false;
}

const char* Nv14UpdateDriver::flashFirmware(STRUCT_HALL* tx, STRUCT_HALL* rx, FIL* file, const char* label) const {
  uint32_t fwLength = (uint32_t)file->obj.objsize;
  int attempt = 1;
  uint8_t commands[] {
    CMD_RF_INIT,
    CMD_UPDATE_RF_FIRMWARE
  };
  
  TRACE("INIT INTERNAL MODULE");
  
  intmoduleSerialStart(INTMODULE_USART_AFHDS2_BAUDRATE, true, USART_Parity_No, USART_StopBits_1, USART_WordLength_8b);
  intmodulePulsesData.flysky.frame_index = 1;

  for (unsigned i = 0; i < sizeof(commands); i++) {
    attempt = 1;
    while (attempt <= MAX_ATTEMPTS) {
      TRACE("CMD %02X %d", commands[i], attempt);
      sendModuleCommand(FRAME_TYPE_REQUEST_ACK, commands[i]);
      watchdogSuspend(100);
      if (getModuleResponse(rx->data, sizeof(rx->data), 200)) {
        break;
      }
      attempt++;
    }
  }

  //there will be no response in case RF firmware is not working
  //if (attempt == MAX_ATTEMPTS) return "RF INIT FAILED";

  updateInfo info = {
    .type = tUpdateInfo,
    .firmwareLength = fwLength,
    .firmwareKey = {0}
  };

  TRACE("FIND XOR FOR FIRMWARE");

  attempt = 1;
  while(attempt <= MAX_ATTEMPTS) {
    watchdogSuspend(510);
    if (getBootloaderResponse(rx, 5000)) {
      if(rx->hallID.hall_Id.packetID && rx->hallID.hall_Id.packetID == UpdateID) {
        updateDetails* u = (updateDetails*)&rx->data;
        if (u->request.type == tUpdateRequest) {
          TRACE("COPY FW key %s", sizeof(info.firmwareKey));
          memcpy(info.firmwareKey, u->request.UID, sizeof(info.firmwareKey));
          break;
        }
      }
    }
    attempt++;
  }
  if (attempt > MAX_ATTEMPTS) return "ACK timeout";

  unsigned long magic = info.firmwareKey[0] + info.firmwareKey[1] + info.firmwareKey[2] + info.firmwareKey[3];

  TRACE("MAGIC %04X", magic);
  //clear flash
  attempt = 1;
  while(attempt <= MAX_ATTEMPTS) {
    watchdogSuspend(3010);
    sendPacket(tx, RemoteController, RF_Internal, UpdateID, (uint8_t*)(&info), sizeof(info));
    if (getBootloaderResponse(rx, 30000) && rx->hallID.hall_Id.packetID == UpdateID) {
      updateDetails* u = (updateDetails*)&rx->data;
      if (u->ack.type == tUpdateAck && rx->hallID.hall_Id.receiverID == RemoteController) {
          break;
      }
    }
    attempt++;
  }
  if (attempt > MAX_ATTEMPTS) return "Clearing chip failed";

  updatePacket packet = {
    .type = tUpdatePacket,
    .PacketNb = 0,
    .firmware = {0}
  };
  pcCommand resetCommand = {
    .getFactoryInfo = 0,
	  .getFactoryConfig = 0,
	  .getUID = 0,
	  .Reset = 1
  };

  uint16_t pageSize = sizeof(packet.firmware);
  uint16_t totalPackets = fwLength / pageSize;
  uint16_t lastPageSize = fwLength % pageSize;
  if (lastPageSize) {
    totalPackets += 1;
  }
  UINT count = 0;
  for (packet.PacketNb = 0; packet.PacketNb < totalPackets; packet.PacketNb += 1) {
    TRACE("UPDATE %d size %d", packet.PacketNb, pageSize);
    memclear(packet.firmware, sizeof(packet.firmware));

    if (f_read(file, packet.firmware, pageSize, &count) != FR_OK) {
      return "Error reading file";
    }
    if (packet.PacketNb >= 2){
      
      for (unsigned i = 0; i < sizeof(packet.firmware); i+=4) {
        *(unsigned long *)(packet.firmware + i) ^= magic;
        
      }
    }
    watchdogSuspend(1000);

    sendPacket(tx, RemoteController, RF_Internal, UpdateID, (uint8_t*)(&packet), sizeof(packet));
    if (getBootloaderResponse(rx, 9000) && rx->hallID.hall_Id.packetID == UpdateID) {
      updateDetails* u = (updateDetails*)rx->data;
      TRACE("ACK %d", u->ack.type);
      switch(u->ack.type) {
        case tUpdateAck:
        case tUpdateRequest: //should not be but it seems responses are using this value
          drawProgressScreen(label, STR_WRITING, file->fptr, file->obj.objsize);
          continue;
        break;
        case tUpdateFailed:
        case tUpdateSuccess:
          sendPacket(tx, RemoteController, RF_Internal, CommandID, (uint8_t*)(&resetCommand), sizeof(resetCommand));
          return u->ack.type == tUpdateFailed ? "Update failed" : nullptr;
        break;
      }
    }
  }
  return nullptr;
}

bool nv14FlashFirmware(const char * filename) {
  FIL file;
  if (f_open(&file, filename, FA_READ) != FR_OK) {
    raiseAlert("Error", "Not a valid file", nullptr, AU_ERROR);
    return false;
  }
  f_lseek(&file, 0);
  pausePulses();
  bool pwr[] = {
    IS_INTERNAL_MODULE_ON(),
    IS_EXTERNAL_MODULE_ON()
  };

  EXTERNAL_MODULE_OFF();
  INTERNAL_MODULE_OFF();
  drawProgressScreen(getBasename(filename), STR_DEVICE_RESET, 0, 0);

  /* wait 1s off */
  watchdogSuspend(100 /*1s*/);
  RTOS_WAIT_MS(500);
  STRUCT_HALL rx = {0};
  STRUCT_HALL tx = {0};
  const char * result = nv14UpdateDriver.flashFirmware(&tx, &rx, &file, getBasename(filename));
  f_close(&file);

  drawProgressScreenDone(result == 0, result ? STR_FIRMWARE_UPDATE_ERROR : STR_FIRMWARE_UPDATE_SUCCESS, result);

  INTERNAL_MODULE_OFF();
  EXTERNAL_MODULE_OFF();

  watchdogSuspend(200 /*2s*/);
  RTOS_WAIT_MS(1000);

  // reset telemetry protocol
  telemetryInit(255);
  
  if (pwr[0]) {
    INTERNAL_MODULE_ON();
    setupPulsesInternalModule();
  }

  if (pwr[1]) {
    EXTERNAL_MODULE_ON();
    setupPulsesExternalModule();
  }

  resumePulses();
  return result == nullptr;
}
