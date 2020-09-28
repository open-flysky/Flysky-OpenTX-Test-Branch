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

bool Nv14FirmwareInformation::valid() const {
  return true;
}

const char * Nv14FirmwareInformation::read(FIL * file) const {
  return nullptr;
}

uint16_t Nv14FirmwareInformation::getCrc16(const char * buffer) const {
  return 0;
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
  *((uint16_t*)(tx->data+length)) = calc_crc16(tx, length + 3);
  intmoduleSendBufferDMA((uint8_t*)tx, length + 5);
  while(intmoduleActiveDMA());

}

void Nv14UpdateDriver::sendResetRequest(STRUCT_HALL* tx) {
  
}

bool getResponse(STRUCT_HALL* rx, uint16_t timeoutMs) {
  uint16_t time = getTmr2MHz();
  while ((uint16_t)(getTmr2MHz() - time) < ((timeoutMs *2)*1000)) {
    uint8_t data = 0;
    if(intmoduleGetByte(&data)) {
      parseFlyskyData(rx, data);
      if (rx->valid) {
        rx->valid = false;
        if (rx->hallID.hall_Id.receiverID != RemoteController) continue;
        return true;
      }
    }
  }
  return false;
}

const char* Nv14UpdateDriver::flashFirmware(STRUCT_HALL* tx, STRUCT_HALL* rx, FIL* file, const char* label) const {
  //reset module to bootloader
  //init
  //set start update command
  uint32_t fwLength = (uint32_t)file->obj.objsize;

  updateInfo info = {
    .type = tUpdateInfo,
    .firmwareLength = fwLength,
    .firmwareKey = {0}
  };

  //Wait for ACK and firmware key
  if (getResponse(rx, 10000) && rx->hallID.hall_Id.packetID == UpdateID) {
    updateDetails* u = (updateDetails*)&rx->data;
    if (u->request.type == tUpdateRequest) {
      memcpy(info.firmwareKey, u->request.UID, sizeof(info.firmwareKey));
    }
  }

  if (!info.firmwareKey[0]) return "ACK timeout";
  unsigned long magic = info.firmwareKey[0] + info.firmwareKey[1] + info.firmwareKey[2] + info.firmwareKey[3];

  //clear flash
  sendPacket(tx, RemoteController, RF_Internal, UpdateID, (uint8_t*)(&info), sizeof(info));

  if (!getResponse(rx, 30000) || rx->hallID.hall_Id.packetID != UpdateID || rx->data[0] != tUpdateAck) {
    return "Clearing chip failed";
  }
  
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
    sendPacket(tx, RemoteController, RF_Internal, UpdateID, (uint8_t*)(&packet), sizeof(packet));
    if (getResponse(rx, 10000) && rx->hallID.hall_Id.packetID == UpdateID) {
      updateDetails* u = (updateDetails*)&rx->data;
      switch(u->ack.type) {
        case tUpdateAck:
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

  Nv14FirmwareInformation firmwareFile;
  if (firmwareFile.read(&file) || !firmwareFile.valid()) {
    f_close(&file);
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
  watchdogSuspend(500 /*5s*/);
  RTOS_WAIT_MS(3000);
  STRUCT_HALL rx = {0};
  STRUCT_HALL tx = {0};
  const char * result = nv14UpdateDriver.flashFirmware(&tx, &rx, &file, getBasename(filename));
  f_close(&file);

  drawProgressScreenDone(result == 0, result ? STR_FIRMWARE_UPDATE_ERROR : STR_FIRMWARE_UPDATE_SUCCESS, result);

  INTERNAL_MODULE_OFF();
  EXTERNAL_MODULE_OFF();
  SPORT_UPDATE_POWER_OFF();

  /* wait 2s off */
  watchdogSuspend(500 /*5s*/);
  RTOS_WAIT_MS(2000);

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
