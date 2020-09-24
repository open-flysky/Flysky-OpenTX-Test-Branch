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
#include "mixer_scheduler.h"

#if !defined(SIMU)

// Global trigger flag
RTOS_FLAG_HANDLE mixerFlag;

// Mixer schedule
struct MixerSchedule {

  // period in us
  volatile uint16_t period;
};

static MixerSchedule mixerSchedules[NUM_MODULES];

uint16_t getMixerSchedulerPeriod()
{
  uint16_t period = std::max(mixerSchedules[INTERNAL_MODULE].period, mixerSchedules[EXTERNAL_MODULE].period);
  if (!period) {
    period = MIXER_SCHEDULER_DEFAULT_PERIOD_US;
  }
    
  return period;
}

void mixerSchedulerInit()
{
  RTOS_CREATE_FLAG(mixerFlag);
  memset(mixerSchedules, 0, sizeof(mixerSchedules));
}

void mixerSchedulerSetPeriod(uint8_t moduleIdx, uint16_t periodUs)
{
  if ((periodUs > 0) && (periodUs < MIN_REFRESH_RATE)) {
    periodUs = MIN_REFRESH_RATE;
  }
  else if ((periodUs > 0) && (periodUs > MAX_REFRESH_RATE)) {
    periodUs = MAX_REFRESH_RATE;
  }
  if (mixerSchedules[moduleIdx].period != periodUs) {
    TRACE("mixerSchedulerSetPeriod mod %d period %d us", moduleIdx, periodUs);
    mixerSchedules[moduleIdx].period = periodUs;
  }
}

bool mixerSchedulerWaitForTrigger(uint8_t timeoutMs)
{
  RTOS_CLEAR_FLAG(mixerFlag);
  return RTOS_WAIT_FLAG(mixerFlag, timeoutMs);
}

void mixerSchedulerISRTrigger()
{
  RTOS_ISR_SET_FLAG(mixerFlag);
}

#endif
