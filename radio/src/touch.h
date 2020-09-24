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

#ifndef TOUCH_H
#define TOUCH_H

//#include "queue"
#include "opentx_types.h"

enum ENUM_TOUCH_EVENT
{
    TE_NONE,
    TE_DOWN,
    TE_UP,
    TE_SLIDE,
    TE_END
};

typedef struct
{
   unsigned char  Event;
   short X;
   short Y;
   short startX;
   short startY;
   short lastX;
   short lastY;
   uint32_t Time;
   uint32_t LastEvent;
} STRUCT_TOUCH;

typedef struct
{
  uint8_t  touch_type;
  uint16_t touch_x;
  uint16_t touch_y;
  uint16_t slide_start_x;
  uint16_t slide_start_y;
  uint16_t slide_end_x;
  uint16_t slide_end_y;
}touch_event_type;

#define SLIDE_RANGE                 ( 6 )
#define MAX_TOUCH_EVENT_CNT         ( 10 )

#endif // TOUCH_H
