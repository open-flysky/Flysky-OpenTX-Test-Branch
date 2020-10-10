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

#ifndef OPENTX_NV14_INTERNAL_MOD_FIRMWARE_H
#define OPENTX_NV14_INTERNAL_MOD_FIRMWARE_H

#include "definitions.h"
#include "hallStick_driver.h"
#include "ff.h"
	
#define StartByte 								  0x55
#define HallStick										0x00
#define RemoteController						0x01
#define PersonalComputer						0x02
#define RF_Internal									0x03

#define FactoryInfoID								((unsigned char)0x8)
#define FactoryCFGID								((unsigned char)0x9)
#define UpdateID									  ((unsigned char)0xA)
#define ChannelID									  ((unsigned char)0xC)
#define CommandID									  ((unsigned char)0xD)
#define PayloadID									  ((unsigned char)0xE)

//update type request from PC to module
#define tUpdateInfo   							(0x1)
//update type date to be updated
#define tUpdatePacket 							(0x2)
//response for tUpdateAck -> we get in reponse map
#define tUpdateAck   								(0x3)
#define tUpdateRequest   						(0x4)
#define tUpdateFailed								(0x5)
#define tUpdateSuccess							(0x6)
#define tUpdateEnd									(0x7)

PACK(struct updateCommand {
  unsigned char Reset;
});

PACK(struct pcCommand {
	unsigned char getFactoryInfo;
	unsigned char getFactoryConfig;
	unsigned char getUID;
	unsigned char Reset;
});


PACK(struct factoryInfo{
	unsigned char type;
	short productorNameLen;
	short productorName[7];
	short productorNumberLen;
	short productorNumber[19];
	long long time;
	unsigned short CRC16;
});

PACK(union pcPayload {
	factoryInfo info;
	unsigned long UID[8];
});

PACK(struct updateInfo {
	unsigned char type;
	unsigned long firmwareLength;
	unsigned long firmwareKey[8];
});

PACK(struct updatePacket {
	unsigned char type;
	unsigned long PacketNb;
	unsigned char firmware[128];
});


PACK(struct updateAck {
	unsigned char type;
	unsigned char map[128];
});

PACK(struct updateRequest {
	unsigned char type;
	unsigned long UID[8];
});

PACK(union updateDetails {
	unsigned char type;
	updateAck ack;
	updatePacket packet;
	updateInfo info;
	updateRequest request;
});

PACK(union updateData {
	updateCommand command;
	updateDetails update;
	char raw[256];
});

class Nv14FirmwareInformation {
  public:
    bool valid() const;
    const char* read(const char * filename);
  private:
    bool crcValid;
};

class Nv14UpdateDriver {
  public:
    const char* flashFirmware(STRUCT_HALL* tx, STRUCT_HALL* rx, FIL* file, const char* label) const;
  private:
    bool getModuleResponse(uint8_t* data, uint16_t maxSize, uint16_t timeoutMs) const;
    void sendModuleCommand(uint8_t type, uint8_t cmd) const;
    bool getBootloaderResponse(STRUCT_HALL* rx, uint16_t timeoutMs, bool checkReceiverID = true) const;
    void debug(const uint8_t* rxBuffer, uint8_t rxBufferCount) const;
    void sendPacket(STRUCT_HALL* tx, uint8_t senderID, uint8_t receiverID, uint8_t packetID, uint8_t* payload, uint16_t length) const;
};

bool nv14FlashFirmware(const char * filename);

#endif
