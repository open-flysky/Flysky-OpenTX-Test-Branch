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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#ifndef __LIERDA_BT_H__
#define __LIERDA_BT_H__

#define LIERDA_BT_FACTORY_BAUDRATE 9600
#define LIERDA_BT_MAX_BAUDRATE 921600
#define LIERDA_MAX_CMD_LENGTH 64
#define FOXWARE_MAX_CMD_LENGTH 64
#define FOXWARE_MAX_PASSCODE_LEN 6
#define FOXWARE_MAX_NAME_LEN 20


#if !defined(bswap)
#define bswap
#if defined(SIMU)
  #define bswapu16 __builtin_bswap16
  #define bswaps16 __builtin_bswap16
  #define bswapu32 __builtin_bswap32
#else
  #define bswapu16 __REV16
  #define bswaps16 __REVSH
  #define bswapu32 __REV
#endif
#endif


#undef DIM
#define DIM(arr) (sizeof((arr))/sizeof((arr)[0]))

/*

GATT Services

S/C Describle UUID    Authority Remarks

S   /         0xFE60  /         Lierda Bluetooth low-energy data transmission service
C   Data M-S  0xFE61  WN/W      Data pass-through channel: The mobile terminal issues the data to MCU via this channel
C   Data S-M  0xFE62  N         Data pass-through channel: The Bluetooth module uploads the data to the mobile terminal via this channel 
C   Settings  0xFE63  WN/W/R/N  Setting channel: The mobile terminal modifies and inquires about the attribute settings of the Bluetooth module via this channel
C   RFU       0xFE6   WN/W/R/N  Reserve for Future Use
*/

//time bettwen sending 2 frames 200 bytes each
#define BT_MIN_IDLE_TIME 5 /*ms*/
#define BT_IDLE_TIME_BITS 64

namespace btle {

enum DeviceType {
  ANDROID = 0,
  IOS = 1,
};

//about 16ms
#define DEFAULT_ADV_INTERVAL 36
//6*1.25
#define ANDROID_ADV_INTERVAL 6 
//10*12.5
#define IOS_ADV_INTERVAL 10


#define DEFAULT_BROADCAST_INTERVAL 48
//32(20ms)
#define ANDROID_BROADCAST_INTERVAL 32
//32(*625μs 20ms)
#define IOS_BROADCAST_INTERVAL 32


const uint16_t CIMins[] = {
  8, //ANDROID 
  12, //IOS
};

const uint16_t CIMaxs[] = {
  12, //ANDROID 
  15, //IOS
};

const uint16_t IdleTimeUs[] {
  26667, //2400
  13333, //4800
  6667,  //9600
  3333,  //19200
  1666,  //38400
  1111,  //57600
  556,   //115200
  278,   //230400
  139,   //460800
  70,    //921600
};

enum StartSequence : uint16_t {
  REQUEST = 0x01FC,
  REPONSE = 0x04FC,
};

enum Command : uint8_t {
  SET_FOX_BAUDRATE = 0x01,
  RES_FOX_VALID = 0x01,
  GET_FOX_BAUDRATE = 0x02,
  SET_FOX_ADVERTISING_INTERNVAL = 0x03,
  GET_FOX_ADVERTISING_INTERNVAL = 0x04, 
  SET_FOX_PASSCODE = 0x05,
  GET_FOX_PASSCODE = 0x06,
  GET_FOX_MAC = 0x07,
  SET_FOX_NAME = 0x08,
  GET_FOX_NAME = 0x09,
  SET_FOX_PASSCODE_PROTECTION = 0x0A,
  GET_FOX_PASSCODE_PROTECTION = 0x0B,
  SET_FOX_BROADCAST_INTERNVAL = 0x0C,
  GET_FOX_BROADCAST_INTERNVAL = 0x0D, 
  SET_FOX_TX_POWER = 0x0E,
  GET_FOX_TX_POWER = 0x0F, 
  SET_FOX_FACTORY_SETTINGS = 0x80,
  SET_FOX_RESET = 0x81,
  GET_FW_VERSION = 0xF0,
};

enum Baudrate : uint8_t {
  BPS_2400 = 0x00,
  BPS_4800 = 0x01,
  BPS_9600 = 0x02,
  BPS_19200 = 0x03,
  BPS_38400 = 0x04,
  BPS_57600 = 0x05,
  BPS_115200 = 0x06,
  BPS_230400 = 0x07,
  BPS_460800 = 0x08,
  BPS_921600 = 0x09,
  BPS_MAX = BPS_921600,
};

const uint32_t baudRateMap[] = {
  2400,
  4800,
  9600,
  19200,
  38400,
  57600,
  115200,
  230400,
  460800,
  921600,
};

enum TxPower : uint8_t {
  power_first = 0x00,
  dBm8 = power_first,
  dBm4 = 0x01,
  dBm0 = 0x02,
  dBm_minus4 = 0x03,
  dBm_minus10 = 0x04,
  dBm_minus14 = 0x05,
  dBm_minus20 = 0x06,
  power_last = dBm_minus20,
};

extern const char* const tx_power_map[];

typedef uint16_t ADVInterval;

//Foxware
uint8_t const correct_cmd[] = {
  0x04, 0xFC, 0x01, 0x00
};

uint8_t const incorrect_cmd[] = {
  0x04, 0xFC, 0x02, 0x00
};

Command const commands_with_ack_only[] = {
  SET_FOX_BAUDRATE,
  SET_FOX_ADVERTISING_INTERNVAL,
  SET_FOX_PASSCODE,
  SET_FOX_NAME,
  SET_FOX_PASSCODE_PROTECTION,
  SET_FOX_BROADCAST_INTERNVAL,
  SET_FOX_TX_POWER,
  SET_FOX_FACTORY_SETTINGS,
  SET_FOX_RESET,
};

//not used for now
union FrameDataLierda {
  uint8_t bytes[LIERDA_MAX_CMD_LENGTH];
};

union FrameDataFoxware {
  uint8_t bytes[FOXWARE_MAX_CMD_LENGTH];
  Baudrate baudrate;
  uint16_t internval;
  char passcode[FOXWARE_MAX_PASSCODE_LEN];
  char name[FOXWARE_MAX_NAME_LEN];
  TxPower power;
};

union FrameData{
  FrameDataFoxware foxware;
  FrameDataLierda lierda;
};

struct __attribute__ ((packed)) Request {
  uint16_t start;
  Command command;
  uint8_t length;
  FrameData data;
};

struct __attribute__ ((packed)) RepsonseLierda {
  uint16_t start;
  Command command;
  uint8_t length;
  uint8_t status;
  FrameDataLierda data;
};

struct __attribute__ ((packed)) ResponseFoxware {
  uint16_t start;
  Command command;
  uint8_t length;
  FrameDataFoxware data;
};

enum ResponseStatus : uint8_t {
  ResponseFailed,
  ResponseSuccess,
  Done,
};

struct configFoxware {
  Baudrate baudrate;
  uint16_t advertising_interval;
  uint32_t passcode;
  char mac[18];
  char name[FOXWARE_MAX_NAME_LEN];
  bool passcode_protection;
  uint16_t broadcast_interval;
  TxPower tx_power;
  uint16_t fw_version;
  int next;
  bool dirty;
  int retry = 0;

  ResponseStatus response(btle::ResponseFoxware* response, size_t size);
  ResponseStatus responseSave(btle::ResponseFoxware* response, size_t size);
  void reset();
  void preLoad();
  void preSave();
  size_t load(uint8_t* frame);
  size_t save(uint8_t* frame, bool* isBaudUpdate);
};

Request* initReqest(uint8_t* frame, Command command, size_t len);
bool valid(ResponseFoxware* frame, Command cmd, size_t length);
uint32_t transmit_time_ms(size_t frameSize, Baudrate baudrate, DeviceType device);
size_t baudrate(uint8_t* frame);
size_t baudrate(uint8_t* frame, Baudrate baudrate);
size_t advertising_interval(uint8_t* frame);
//internval * 1.25ms
size_t advertising_interval(uint8_t* frame, uint16_t interval);
size_t passcode(uint8_t* frame);
size_t passcode(uint8_t* frame, uint32_t code);
size_t mac(uint8_t* frame);
size_t name(uint8_t* frame);
size_t name(uint8_t* frame, const char* name);
size_t passcode_protection(uint8_t* frame);
size_t passcode_protection(uint8_t* frame, bool enabled);
size_t broadcast_interval(uint8_t* frame);
//internval * 625μs - 32(20ms)~8000(5s)
size_t broadcast_interval(uint8_t* frame, uint16_t interval);
size_t tx_power(uint8_t* frame);
size_t tx_power(uint8_t* frame, TxPower power);
size_t factory_settings(uint8_t* frame);
size_t reset(uint8_t* frame);
size_t fw_version(uint8_t* frame);

}
#endif

