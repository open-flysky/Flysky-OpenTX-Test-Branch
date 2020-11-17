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

#if defined(SIMU)
#define dumpPrintf(...) debugPrintf(__VA_ARGS__)
#else
#define dumpPrintf(...) serialPrintf(__VA_ARGS__)
#endif

uint8_t dumpPosition;

void dumpStart(unsigned int size)
{
  dumpPrintf("DUMP %d bytes ...\n\r", size);
  dumpPosition = 0;
}

void dumpBody(const uint8_t *data, unsigned int size)
{
  char buffer[64*3];
  char* pos = buffer;
  if (size * 3 > sizeof(buffer)) size = 64;
  for (unsigned i=0; i < size; i++) {
    pos += snprintf(pos, buffer + sizeof(buffer) - pos, "%02X ", data[i]);
  }
  (*pos) = 0;
  TRACE("%s", buffer);
}

void dumpEnd()
{
  dumpPrintf("\r\n");
}

#if defined(DEBUG) || defined(CLI)
void dump(const uint8_t * data, unsigned int size)
{
  dumpStart(size);
  dumpBody(data, size);
  dumpEnd();
}
#endif
