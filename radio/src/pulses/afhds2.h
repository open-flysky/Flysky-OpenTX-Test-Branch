/*
 * afhds2.h
 *
 *  Created on: 20.11.2019
 *      Author: jklim
 */

#ifndef PULSES_AFHDS2_H_
#define PULSES_AFHDS2_H_

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
