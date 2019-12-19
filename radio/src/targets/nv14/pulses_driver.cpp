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

void intmoduleStop(void);
void extmoduleStop(void);

void intmoduleNoneStart(void);
void intmodulePxxStart(void);

void extmoduleNoneStart(void);
void extmodulePpmStart(void);
void extmodulePxxStart(void);
#if defined(DSM2)
void extmoduleDsm2Start(void);
#endif
void extmoduleCrossfireStart(void);
#if defined(AFHDS3)
void extmoduleAFHDS3Start(uint32_t baudRate, uint16_t wordLength, uint16_t stopBits, uint16_t parity);
#endif

void init_no_pulses(uint32_t port) {
  if (port == INTERNAL_MODULE)
    intmoduleNoneStart();
  else
    extmoduleNoneStart();
}

void disable_no_pulses(uint32_t port) {
  if (port == INTERNAL_MODULE)
    intmoduleStop();
  else
    extmoduleStop();
}

void init_ppm(uint32_t port) {
  if (port == EXTERNAL_MODULE) {
    extmodulePpmStart();
  }
}

void disable_ppm(uint32_t port) {
  if (port == EXTERNAL_MODULE) {
    extmoduleStop();
  }
}

void init_pxx(uint32_t port) {
  if (port == INTERNAL_MODULE)
    intmodulePxxStart();
  else
    extmodulePxxStart();
}

void init_afhds3(uint32_t port) {
  if (port == EXTERNAL_MODULE) extmoduleAFHDS3Start(afhds3uart.baudrate, afhds3uart.wordLength, afhds3uart.stopBits, afhds3uart.parity);
}
void disable_afhds3(uint32_t port) {
  if (port == EXTERNAL_MODULE) extmoduleStop();
}

void disable_pxx(uint32_t port) {
  if (port == INTERNAL_MODULE)
    intmoduleStop();
  else
    extmoduleStop();
}

#if defined(DSM2)
void init_dsm2(uint32_t port)
{
  if (port == EXTERNAL_MODULE) {
    extmoduleDsm2Start();
  }
}

void disable_dsm2(uint32_t port)
{
  if (port == EXTERNAL_MODULE) {
    extmoduleStop();
  }
}
#endif

void init_sbusOut(uint32_t port) {
  if (port == EXTERNAL_MODULE) {
    extmoduleDsm2Start();
  }
}

void disable_sbusOut(uint32_t port) {
  if (port == EXTERNAL_MODULE) {
    extmoduleStop();
  }
}

void init_crossfire(uint32_t port) {
  if (port == EXTERNAL_MODULE) {
    extmoduleCrossfireStart();
  }
}

void disable_crossfire(uint32_t port) {
  if (port == EXTERNAL_MODULE) {
    extmoduleStop();
  }
}
