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

#ifndef _KEYS_H_
#define _KEYS_H_

#include "board.h"

#define EVT_KEY_MASK(e)                ((e) & 0x1f)

#if defined(PCBHORUS)
#define _MSK_KEY_BREAK                 0x0200
#define _MSK_KEY_REPT                  0x0400
#define _MSK_KEY_FIRST                 0x0600
#define _MSK_KEY_LONG                  0x0800
#define _MSK_KEY_FLAGS                 0x0e00
#define EVT_ENTRY                      0x1000
#define EVT_ENTRY_UP                   0x2000
#else
#define _MSK_KEY_BREAK                 0x20
#define _MSK_KEY_REPT                  0x40
#define _MSK_KEY_FIRST                 0x60
#define _MSK_KEY_LONG                  0x80
#define _MSK_KEY_FLAGS                 0xe0
#define EVT_ENTRY                      0xbf
#define EVT_ENTRY_UP                   0xbe
#endif

// normal order of events is: FIRST, LONG, REPEAT, REPEAT, ..., BREAK
#define EVT_KEY_FIRST(key)             ((key)|_MSK_KEY_FIRST)  // fired when key is pressed
#define EVT_KEY_LONG(key)              ((key)|_MSK_KEY_LONG)   // fired when key is held pressed for a while
#define EVT_KEY_REPT(key)              ((key)|_MSK_KEY_REPT)   // fired when key is held pressed long enough, fires multiple times with increasing speed
#define EVT_KEY_BREAK(key)             ((key)|_MSK_KEY_BREAK)  // fired when key is released (short or long), but only if the event was not killed

#define IS_KEY_FIRST(evt)              (((evt) & _MSK_KEY_FLAGS) == _MSK_KEY_FIRST)
#define IS_KEY_LONG(evt)               (((evt) & _MSK_KEY_FLAGS) == _MSK_KEY_LONG)
#define IS_KEY_REPT(evt)               (((evt) & _MSK_KEY_FLAGS) == _MSK_KEY_REPT)
#define IS_KEY_BREAK(evt)              (((evt) & _MSK_KEY_FLAGS) == _MSK_KEY_BREAK)

#if defined(STM32)
typedef uint16_t event_t;
#else
typedef uint8_t event_t;
#endif

#if defined(ROTARY_ENCODER_NAVIGATION)
  #if (defined(PCBHORUS) || defined(PCBTARANIS))
    #define EVT_ROTARY_BREAK           EVT_KEY_BREAK(KEY_ENTER)
    #define EVT_ROTARY_LONG            EVT_KEY_LONG(KEY_ENTER)
    #define EVT_ROTARY_LEFT            (0xDF00)
    #define EVT_ROTARY_RIGHT           (0xDE00)
    #define EVT_KEY_LINE_UP            EVT_ROTARY_LEFT
    #define EVT_KEY_LINE_DOWN          EVT_ROTARY_RIGHT
  #else
    #define EVT_ROTARY_BREAK           (0xcf)
    #define EVT_ROTARY_LONG            (0xce)
    #define EVT_ROTARY_LEFT            (0xdf)
    #define EVT_ROTARY_RIGHT           (0xde)
    #define EVT_KEY_LINE_UP            EVT_ROTARY_LEFT
    #define EVT_KEY_LINE_DOWN          EVT_ROTARY_RIGHT
  #endif  // (defined(PCBHORUS) || defined(PCBTARANIS))
  #define IS_NEXT_EVENT(event)         (event == EVT_KEY_LINE_DOWN)
  #define IS_PREVIOUS_EVENT(event)     (event == EVT_KEY_LINE_UP)
  #define IS_ROTARY_LEFT(evt)          (evt == EVT_ROTARY_LEFT)
  #define IS_ROTARY_RIGHT(evt)         (evt == EVT_ROTARY_RIGHT)
  #define IS_ROTARY_BREAK(evt)         (evt == EVT_ROTARY_BREAK)
  #define IS_ROTARY_LONG(evt)          (evt == EVT_ROTARY_LONG)
  #define IS_ROTARY_EVENT(evt)         (EVT_KEY_MASK(evt) >= 0x0e)
  #define CASE_EVT_ROTARY_BREAK        case EVT_ROTARY_BREAK:
  #define CASE_EVT_ROTARY_LONG         case EVT_ROTARY_LONG:
  #define CASE_EVT_ROTARY_LEFT         case EVT_ROTARY_LEFT:
  #define CASE_EVT_ROTARY_RIGHT        case EVT_ROTARY_RIGHT:
  #define CASE_EVT_LINE_UP_RPT
  #define CASE_EVT_LINE_DOWN_RPT
#else  // no rot. enc.
  #define EVT_KEY_LINE_UP              EVT_KEY_BREAK(KEY_UP)
  #define EVT_KEY_LINE_UP_RPT          EVT_KEY_REPT(KEY_UP)
  #define EVT_KEY_LINE_DOWN            EVT_KEY_BREAK(KEY_DOWN)
  #define EVT_KEY_LINE_DOWN_RPT        EVT_KEY_REPT(KEY_DOWN)
  #define IS_NEXT_EVENT(event)         (event == EVT_KEY_LINE_DOWN || event == EVT_KEY_LINE_DOWN_RPT)
  #define IS_PREVIOUS_EVENT(event)     (event == EVT_KEY_LINE_UP   || event == EVT_KEY_LINE_UP_RPT)
  #define IS_ROTARY_LEFT(evt)          (0)
  #define IS_ROTARY_RIGHT(evt)         (0)
  #define IS_ROTARY_BREAK(evt)         (0)
  #define IS_ROTARY_LONG(evt)          (0)
  #define IS_ROTARY_EVENT(evt)         (0)
  #define CASE_EVT_ROTARY_BREAK
  #define CASE_EVT_ROTARY_LONG
  #define CASE_EVT_ROTARY_LEFT
  #define CASE_EVT_ROTARY_RIGHT
  #define CASE_EVT_LINE_UP_RPT         case EVT_KEY_LINE_UP_RPT:
  #define CASE_EVT_LINE_DOWN_RPT       case EVT_KEY_LINE_DOWN_RPT:
#endif  // defined(ROTARY_ENCODER_NAVIGATION)

#if defined(PCBHORUS)
  #define EVT_KEY_NEXT_PAGE        EVT_KEY_BREAK(KEY_PGDN)
  #define EVT_KEY_PREVIOUS_PAGE    EVT_KEY_BREAK(KEY_PGUP)
#elif defined(PCBTARANIS)
  #define EVT_KEY_NEXT_PAGE        EVT_KEY_BREAK(KEY_PAGE)
  #define EVT_KEY_PREVIOUS_PAGE    EVT_KEY_LONG(KEY_PAGE)
#else
  #define EVT_KEY_NEXT_PAGE        EVT_KEY_BREAK(KEY_RIGHT)
  #define EVT_KEY_PREVIOUS_PAGE    EVT_KEY_BREAK(KEY_LEFT)
#endif

#if defined(COLORLCD)
  #define EVT_REFRESH                  (0xDD00)
#endif

class Key
{
  private:
    uint8_t m_vals;
    uint8_t m_cnt;
    uint8_t m_state;
  public:
    void input(bool val);
    bool state() const { return m_vals > 0; }
    void pauseEvents();
    void killEvents();
    uint8_t key() const;
};

extern Key keys[NUM_KEYS];
extern event_t s_evt;

#define putEvent(evt) s_evt = evt

void pauseEvents(event_t event);
void killEvents(event_t event);

#if defined(CPUARM)
  bool clearKeyEvents();
  event_t getEvent(bool trim=false);
#else
  void clearKeyEvents();
  event_t getEvent();
#endif

uint8_t keyDown();

#endif // _KEYS_H_
