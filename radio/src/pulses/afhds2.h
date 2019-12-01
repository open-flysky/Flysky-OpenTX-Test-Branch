/*
 * afhds2.h
 *
 *  Created on: 20.11.2019
 *      Author: jklim
 */

#ifndef PULSES_AFHDS2_H_
#define PULSES_AFHDS2_H_
#include <functional>
#include <map>
#include <list>

struct FlySkySerialPulsesData {
  uint8_t  pulses[64];
  uint8_t  * ptr;
  uint8_t  frame_index;
  uint8_t  crc;
  uint8_t  state;
  uint8_t  timeout;
  uint8_t  esc_state;
  uint8_t  telemetry[64];
  uint8_t  telemetry_index;
} __attribute__((__packed__));

enum AfhdsSpecialChars {
  END = 0xC0,             //Frame end
  ESC_END = 0xDC,         //Escaped frame end - in case END occurs in fame then ESC ESC_END must be used
  ESC = 0xDB,             //Escaping character
  ESC_ESC = 0xDD,         //Escaping character in case ESC occurs in fame then ESC ESC_ESC  must be used
};


class afhds2 {
public:
  afhds2();
  virtual ~afhds2();
};

#endif /* PULSES_AFHDS2_H_ */
