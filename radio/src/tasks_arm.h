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

#ifndef _TASKS_ARM_H_
#define _TASKS_ARM_H_

#include "rtos.h"

// stack sizes should be in multiples of 8 for better alignment
#define MENUS_STACK_SIZE       3000
#define MIXER_STACK_SIZE       300 //504
#define TELEMETRY_STACK_SIZE   240 
#define AUDIO_STACK_SIZE       500
#define TOUCH_STACK_SIZE       100  // TODO: this can be reduced a lot after debug (tracing) is done (on last check only 42 Words are actually used)
#define BLUETOOTH_STACK_SIZE   504  // WTF: there is no BT task.... ???

#define MIXER_TASK_PRIO        5
#define TELEMETRY_TASK_PRIO    6
#define AUDIO_TASK_PRIO        7
#define MENUS_TASK_PRIO        10
#define CLI_TASK_PRIO          10
#define TOUCH_TASK_PRIO        12   // lower prio than GUI! otherwise may block (runs at 1 tick)

extern RTOS_TASK_HANDLE menusTaskId;
extern RTOS_DEFINE_STACK(menusStack, MENUS_STACK_SIZE);

extern RTOS_TASK_HANDLE mixerTaskId;
extern RTOS_DEFINE_STACK(mixerStack, MIXER_STACK_SIZE);

extern RTOS_TASK_HANDLE telemetryTaskId;
extern RTOS_DEFINE_STACK(telemetryStack, TELEMETRY_STACK_SIZE);

extern RTOS_TASK_HANDLE audioTaskId;
extern RTOS_DEFINE_STACK(audioStack, AUDIO_STACK_SIZE);

extern RTOS_FLAG_HANDLE openTxInitCompleteFlag;

void stackPaint();
void tasksStart();

extern volatile uint16_t timeForcePowerOffPressed;
inline void resetForcePowerOffRequest() {timeForcePowerOffPressed = 0; }

extern RTOS_MUTEX_HANDLE mixerMutex;
inline void pauseMixerCalculations() { RTOS_LOCK_MUTEX(mixerMutex); }
inline void resumeMixerCalculations() { RTOS_UNLOCK_MUTEX(mixerMutex); }

#endif // _TASKS_ARM_H_
