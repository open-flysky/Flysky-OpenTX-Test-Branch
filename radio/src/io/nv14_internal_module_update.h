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

	
#define MS_Start_bit 								0x55
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

PACK(struct sPC_Command {
	unsigned char getFactoryInfo;
	unsigned char getFactoryConfig;
	unsigned char getUID;
	unsigned char Reset;
});

PACK(union uPC_Payload {
	sFactoryInfo info;
	sFactoryConfig config;
	unsigned long UID[8];
});

PACK(struct updateInfo {
	unsigned char type;
	unsigned long FirmwareLength;
	unsigned long FirmwareKey[8];
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


/*

Send frame 
Req start update: 0x01 0x0c 0x00
Resp 0x11 0x0C



*/

/*

FirmwareKey = 0x12345678;

		update.type = tUpdateRequest;
		update.request.UID[0] = UID_0^0x12345678;
		update.request.UID[1] = UID_1^update.request.UID[0];
		update.request.UID[2] = UID_2^update.request.UID[1];
		update.request.UID[3] = FirmwareVersion;
		update.request.UID[4] = HardwareVersion;
		
		update.request.UID[5] = FIRMWARE_SIGNATURE_1^RESET_SIGNATURE_1;
		update.request.UID[6] = FIRMWARE_SIGNATURE_2^RESET_SIGNATURE_2;
		update.request.UID[7] = RESET_SIGNATURE_1^RESET_SIGNATURE_2;
		FirmwareKey = update.request.UID[0]+update.request.UID[1]+update.request.UID[2]+update.request.UID[3];
		SendMSPacket(&TxMsg,RF_Internal,updateMode,UpdateID,(unsigned char *)&update,sizeof(updateRequest));
*/

//update info tUpdateInfo
/*
FirmwareKey = 0x12345678;
FirmwareLength = Update.info.FirmwareLength;

			FinalFirmwareKey = FirmwareKey;
			
			PageCnt = (FirmwareLength%FLASE_PAGE)?(FirmwareLength/FLASE_PAGE):((FirmwareLength/FLASE_PAGE)-1);
				
			for(int i = 0;i<=PageCnt;i++)
			{
				FLH_ErasePage(FLASH_BASE_ADDRESS+BOOTLOADER_SIZE+i*FLASE_PAGE);
			}
			
			updateMode = Msg->u.s.SenderID;
			EndPacketNb = (FirmwareLength%128)?(FirmwareLength/128):((FirmwareLength/128)-1);
			SendAck();
*/

/* tUpdatePacket
static BOOL getMapByIndex(int index)
{
	return 1&(CheckMap[index>>3]>>(index&0x7));
}

static void setMapByIndex(int index,BOOL val)
{
	if(val)
	{
		CheckMap[index>>3]|=1<<(index&0x7);
	}else
	{
		CheckMap[index>>3]&=~(1<<(index&0x7));
	}
}

	if(FirmwareLength!=0x12345678)
			{
				if(getMapByIndex(Update.packet.PacketNb)==FALSE)
				{
					#if 1
					if(Update.packet.PacketNb>=2)
					{
						for(int i=0;i<32;i++)
						{
							((__packed unsigned long*)Update.packet.firmware)[i] ^= FinalFirmwareKey;
						}
					}
					#endif
					FLH_Program(FLASH_BASE_ADDRESS + BOOTLOADER_SIZE+Update.packet.PacketNb*128,Update.packet.firmware,128);
					setMapByIndex(Update.packet.PacketNb,1);
				}

        
*/

/*
	if(updateMode!=RF_Internal)
	{
		if(EndPacketNb!=0x12345678)
		{
			unsigned char couldReset = TRUE;
			for(int i = 0;i<=EndPacketNb;i++)
			{
				if(getMapByIndex(i)==FALSE)
				{
					couldReset = FALSE;
					break;
				}
			}
			if(couldReset)
			{
				if(Timeout)
				{
					update.type = tUpdateFailed+HandlerCheck();
					SendMSPacket(&TxMsg,RF_Internal,updateMode,UpdateID,(unsigned char *)&update,sizeof(unsigned char));
				}else
				{
					ResetSignature[0]=0;
					ResetSignature[1]=0;
					NVIC_SystemReset();
				}
				Timeout --;
				return;
			}
		}
		else
		{
			return;
		}
		update.type = tUpdateAck;
		memcpy(update.ack.map,CheckMap,sizeof(update.ack.map));
		SendMSPacket(&TxMsg,RF_Internal,updateMode,UpdateID,(unsigned char *)&update,sizeof(updateAck));
	}
	else
	{
		update.type = tUpdateRequest;
		update.request.UID[0] = UID_0^0x12345678;
		update.request.UID[1] = UID_1^update.request.UID[0];
		update.request.UID[2] = UID_2^update.request.UID[1];
		update.request.UID[3] = FirmwareVersion;
		update.request.UID[4] = HardwareVersion;
		
		update.request.UID[5] = FIRMWARE_SIGNATURE_1^RESET_SIGNATURE_1;
		update.request.UID[6] = FIRMWARE_SIGNATURE_2^RESET_SIGNATURE_2;
		update.request.UID[7] = RESET_SIGNATURE_1^RESET_SIGNATURE_2;
		FirmwareKey = update.request.UID[0]+update.request.UID[1]+update.request.UID[2]+update.request.UID[3];
		SendMSPacket(&TxMsg,RF_Internal,updateMode,UpdateID,(unsigned char *)&update,sizeof(updateRequest));
	}
  */
#endif
