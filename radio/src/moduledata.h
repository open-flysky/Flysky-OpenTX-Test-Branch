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

/*
 * Module structure
 */
// Only used in case switch and if statements as "virtual" protocol
// When using unions, wrap all but the largest member in NOBACKUP()
#ifndef _MODULEDATA_H_
#define _MODULEDATA_H_

#if defined(BACKUP)
  #define NOBACKUP2(...)
#else
  #define NOBACKUP2(...)                __VA_ARGS__
#endif

#include "dataconstants.h"
#define MM_RF_CUSTOM_SELECTED 0xff

struct ModuleData {
  uint8_t type:4;
  int8_t  rfProtocol:4;
  uint8_t channelsStart;
  int8_t  channelsCount; // 0=8 channels
  uint8_t failsafeMode:4;  // only 3 bits used
  uint8_t subType:3;
  uint8_t invertedSerial:1; // telemetry serial inverted from standard
  int16_t failsafeChannels[MAX_OUTPUT_CHANNELS];
  union {
    NOBACKUP2(struct {
      int8_t  delay:6;
      uint8_t pulsePol:1;
      uint8_t outputType:1;    // false = open drain, true = push pull
      int8_t  frameLength;
    } ppm);
    NOBACKUP2(struct {
      uint8_t rfProtocolExtra:2;
      uint8_t spare1:3;
      uint8_t customProto:1;
      uint8_t autoBindMode:1;
      uint8_t lowPowerMode:1;
      int8_t optionValue;
    } multi);

    struct {
      uint8_t rx_id[4];
      uint8_t mode;
      uint8_t rx_freq[2];
    } romData;

    NOBACKUP2(struct {
      uint8_t bindPower:3;
      uint8_t runPower:3;
      uint8_t emi:1;
      uint8_t spare:1;
      uint8_t telemetry:1;
      uint8_t mode:4;
      uint8_t spare2:3;
      uint16_t failsafeTimeout; //4
      uint16_t rxFreq;
      //one more free byte
      bool isSbus() {
        return (mode & 1);
      }
      bool isPWM() {
        return mode < 2;
      }
    } afhds3);
    NOBACKUP2(struct {
      uint8_t power:2;                  // 0=10 mW, 1=100 mW, 2=500 mW, 3=1W
      uint8_t spare1:2;
      uint8_t receiver_telem_off:1;     // false = receiver telem enabled
      uint8_t receiver_channel_9_16:1;  // false = pwm out 1-8, true 9-16
      uint8_t external_antenna:1;       // false = internal antenna, true = external antenna
      uint8_t spare2:1;
      uint8_t spare3;
    } pxx);
    NOBACKUP2(struct {
      uint8_t spare1:6;
      uint8_t noninverted:1;
      uint8_t spare2:1;
      int8_t refreshRate;  // definition as framelength for ppm (* 5 + 225 = time in 1/10 ms)
    } sbus);
  };

  // Helper functions to set both of the rfProto protocol at the same time
  NOBACKUP2(inline uint8_t getMultiProtocol(bool returnCustom) {
    if (returnCustom && multi.customProto)
      return MM_RF_CUSTOM_SELECTED;
    return ((uint8_t) (rfProtocol & 0x0f)) + (multi.rfProtocolExtra << 4);
  })

  NOBACKUP2(inline void setMultiProtocol(uint8_t proto)
  {
    rfProtocol = (uint8_t) (proto & 0x0f);
    multi.rfProtocolExtra = (proto & 0x30) >> 4;
  })
} __attribute__((__packed__));

#endif /* _MODULEDATA_H_ */
