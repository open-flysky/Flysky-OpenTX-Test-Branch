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

static const Nv14UpdateDriver nv14UpdateDriver;
static const char* ReadError = "Error opening file";

bool Nv14FirmwareInformation::valid() const {
  return crcValid;
}

const char * Nv14FirmwareInformation::read(const char * filename) {
  FIL file;
  if (f_open(&file, filename, FA_READ) != FR_OK)
    return ReadError;
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


static int parseState = 0;
static void parseData(STRUCT_HALL *buffer, unsigned char ch)
{
    if (parseState != 0) return;
    parseState = 1;

    switch (buffer->status)
    {
      case GET_START:
      {
        if (HALL_PROTOLO_HEAD == ch)
        {
          buffer->head = HALL_PROTOLO_HEAD;
          buffer->status = GET_ID;
          buffer->valid = 0;
        }
        break;
      }
      case GET_ID:
      {
        buffer->hallID.ID = ch;
        buffer->status = GET_LENGTH;
        break;
      }
      case GET_LENGTH:
      {
        buffer->length = ch;
        buffer->dataIndex = 0;
        buffer->status = GET_DATA;
        if (0 == buffer->length)
        {
          buffer->status = GET_CHECKSUM;
          buffer->checkSum=0;
        }
        break;
      }
      case GET_DATA:
      {
        buffer->data[buffer->dataIndex++] = ch;
        if (buffer->dataIndex >= buffer->length)
        {
          buffer->checkSum = 0;
          buffer->dataIndex = 0;
          buffer->status = GET_STATE;
        }
        break;
      }
      case GET_STATE:
      {
        buffer->checkSum = 0;
        buffer->dataIndex = 0;
        buffer->status = GET_CHECKSUM;
      }
      case GET_CHECKSUM:
      {
        buffer->checkSum |= ch << ((buffer->dataIndex++) * 8);
        if (buffer->dataIndex >= 2 )
        {
          buffer->dataIndex = 0;
          buffer->status = CHECKSUM;
        }
        else
        {
          break;
        }
      }
      case CHECKSUM:
      {
        if(buffer->checkSum == crc16(CRC_1021, (const uint8_t*)&buffer->head, buffer->length + 3, 0xffff))
        {
          buffer->valid = 1;
          goto Label_restart;
        }
        else
        {
          goto Label_error;
        }
      }
    }

    goto exit;

    Label_error:
    Label_restart:
        buffer->status = GET_START;
exit: 
    parseState = 0;
    return ;
}

void Nv14UpdateDriver::sendPacket(STRUCT_HALL* tx, uint8_t senderID, uint8_t receiverID, uint8_t packetID, uint8_t* payload, uint16_t length) const {
  tx->head = StartByte;
  tx->hallID.hall_Id.senderID = senderID;
  tx->hallID.hall_Id.receiverID = receiverID;
  tx->hallID.hall_Id.packetID = packetID;
  if (payload == nullptr || length > HALLSTICK_BUFF_SIZE) {
    return;
  }
	memcpy(tx->data, payload, length);
  *((uint16_t*)(tx->data+length)) = crc16(CRC_1021, (const uint8_t*)tx, length + 3, 0xffff);
  intmoduleSendBufferDMA((uint8_t*)tx, length + 5);
}

void debug(const uint8_t* rxBuffer, uint8_t rxBufferCount){
  // debug print the content of the packet
  char buffer[240];
  char* pos = buffer;
  for (int i=0; i < rxBufferCount; i++) {
    pos += snprintf(pos, buffer + sizeof(buffer) - pos, "%02X ", rxBuffer[i]);
  }
  (*pos) = 0;
  TRACE("count [%d] data: %s \r\n", rxBufferCount, buffer);
}

bool getBootloaderResponse(STRUCT_HALL* rx, uint16_t timeoutMs, bool checkReceiverID = true) {
  uint16_t time = getTmr2MHz();
  while ((uint16_t)(getTmr2MHz() - time) < ((timeoutMs *2)*1000)) {
    uint8_t byte = 0;
    if(intmoduleGetByte(&byte)) {
      parseData(rx, byte);
      if (rx->valid) { 
        rx->valid = false;
        TRACE("VALID PACKET ID %02X Sender %02X Reciever %02X \r\n", rx->hallID.hall_Id.packetID, rx->hallID.hall_Id.senderID, rx->hallID.hall_Id.receiverID);
        debug((const uint8_t*)rx, rx->length + 3);
        //if (checkReceiverID && rx->hallID.hall_Id.receiverID != RemoteController) continue;
        return true;
      }
    }
  }
  return false;
}

void sendModuleCommand(uint8_t type, uint8_t cmd) {
  afhds2Command(type, cmd);
  uint8_t* data = intmodulePulsesData.flysky.pulses;
  uint16_t size = intmodulePulsesData.flysky.ptr - data;
  intmoduleSendBufferDMA(data, size);
  debug(data, size);
}

bool getModuleResponse(uint8_t* data, uint16_t maxSize, uint16_t timeoutMs) {
  uint16_t time = getTmr2MHz();
  uint16_t index = 0;
  bool escape = false;
  while ((uint16_t)(getTmr2MHz() - time) < ((timeoutMs *2)*1000)) {
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

#define MAX_ATTEMPTS 5
const char* Nv14UpdateDriver::flashFirmware(STRUCT_HALL* tx, STRUCT_HALL* rx, FIL* file, const char* label) const {
  TRACE("INIT INTERNAL MODULE");
  intmoduleSerialStart(INTMODULE_USART_AFHDS2_BAUDRATE, true, USART_Parity_No, USART_StopBits_1, USART_WordLength_8b);
  intmodulePulsesData.flysky.frame_index = 1;
  int attempt = 1;
  uint8_t commands[] {
    CMD_RF_INIT,
    CMD_UPDATE_RF_FIRMWARE
  };

  for (unsigned i = 0; i < sizeof(commands); i++) {
    watchdogSuspend(500 /*6s*/);
    while (attempt <= MAX_ATTEMPTS) {
      TRACE("CMD_RF_INIT %d", attempt);
      sendModuleCommand(FRAME_TYPE_REQUEST_ACK, commands[i]);
      if (getModuleResponse(rx->data, sizeof(rx->data), 1000)) {
        break;
      }
      attempt++;
    }
    if (attempt == MAX_ATTEMPTS) return "RF INIT FAILED";
  }

  uint32_t fwLength = (uint32_t)file->obj.objsize;

  updateInfo info = {
    .type = tUpdateInfo,
    .firmwareLength = fwLength,
    .firmwareKey = {0}
  };

  TRACE("update key");
  attempt = 1;
  while(attempt <= MAX_ATTEMPTS) {
    watchdogSuspend(500);
    if (getBootloaderResponse(rx, 5000)) {
      TRACE("Attempt %d", attempt);
      if(rx->hallID.hall_Id.packetID == UpdateID) {
        updateDetails* u = (updateDetails*)&rx->data;
        if (u->request.type == tUpdateRequest) {
          memcpy(info.firmwareKey, u->request.UID, sizeof(info.firmwareKey));
          break;
        }
      }
    }
    attempt++;
  }
  if (attempt == MAX_ATTEMPTS) return "ACK timeout";
  unsigned long magic = info.firmwareKey[0] + info.firmwareKey[1] + info.firmwareKey[2] + info.firmwareKey[3];

  TRACE("Magic %d", magic);

  //clear flash
  attempt = 1;
  while(attempt <= MAX_ATTEMPTS) {
    watchdogSuspend(3000);
    sendPacket(tx, RemoteController, RF_Internal, UpdateID, (uint8_t*)(&info), sizeof(info));
    if (getBootloaderResponse(rx, 30000)) {
      TRACE("PacketID %d type %d", rx->hallID.hall_Id.packetID, rx->data[0]);
      if(rx->hallID.hall_Id.packetID == UpdateID && rx->data[0] == tUpdateAck) {
        break;
      }
    }
    attempt++;
  }
  if (attempt == MAX_ATTEMPTS) return "Clearing chip failed";
  
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
  UINT count = 0;
  for (uint32_t packetNb = 0; packetNb < totalPackets; packetNb += 1) {
    TRACE("UPDATE %d", packetNb);
    memclear(packet.firmware, pageSize);
    packet.PacketNb = packetNb;
    if (f_read(file, packet.firmware, pageSize, &count) != FR_OK) {
      return "Error reading file";
    }
    if (packetNb >= 2){
      for (int i = 0; i < 32; i++) {
        ((unsigned long *)packet.firmware)[i] ^= magic;
      }
    }
    watchdogSuspend(1000);
    sendPacket(tx, RemoteController, RF_Internal, UpdateID, (uint8_t*)(&packet), sizeof(packet));
    if (getBootloaderResponse(rx, 10000) && rx->hallID.hall_Id.packetID == UpdateID) {
      updateDetails* u = (updateDetails*)&rx->data;
      TRACE("Written %d ack %d", packetNb, u->ack.type);
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
  uint8_t intPwr = IS_INTERNAL_MODULE_ON();
  INTERNAL_MODULE_OFF();
  uint8_t extPwr = IS_EXTERNAL_MODULE_ON();
  EXTERNAL_MODULE_OFF();
  uint8_t spuPwr = IS_SPORT_UPDATE_POWER_ON();
  SPORT_UPDATE_POWER_OFF();
  drawProgressScreen(getBasename(filename), STR_DEVICE_RESET, 0, 0);

  /* wait 2s off */
  watchdogSuspend(200 /*2s*/);
  RTOS_WAIT_MS(1000);
  STRUCT_HALL rx = {0};
  STRUCT_HALL tx = {0};
  const char * result = nv14UpdateDriver.flashFirmware(&tx, &rx, &file, getBasename(filename));
  f_close(&file);

  drawProgressScreenDone(result == 0, result ? STR_FIRMWARE_UPDATE_ERROR : STR_FIRMWARE_UPDATE_SUCCESS, result);

  INTERNAL_MODULE_OFF();
  EXTERNAL_MODULE_OFF();
  SPORT_UPDATE_POWER_OFF();

  /* wait 2s off */
  watchdogSuspend(200 /*5s*/);
  RTOS_WAIT_MS(1000);

  // reset telemetry protocol
  telemetryInit(255);
  
  if (intPwr) {
    INTERNAL_MODULE_ON();
    setupPulsesInternalModule();
  }

  if (extPwr) {
    EXTERNAL_MODULE_ON();
    setupPulsesExternalModule();
  }

  if (spuPwr) {
    SPORT_UPDATE_POWER_ON();
  }

  resumePulses();

  return result == nullptr;
}
