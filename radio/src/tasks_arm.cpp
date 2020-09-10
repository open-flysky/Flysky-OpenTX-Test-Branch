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
#include "shutdown_animation.h"
#include "mixer_scheduler.h"

RTOS_TASK_HANDLE menusTaskId;
RTOS_DEFINE_STACK(menusStack, MENUS_STACK_SIZE);

RTOS_TASK_HANDLE mixerTaskId;
RTOS_DEFINE_STACK(mixerStack, MIXER_STACK_SIZE);

RTOS_TASK_HANDLE audioTaskId;
RTOS_DEFINE_STACK(audioStack, AUDIO_STACK_SIZE);

RTOS_TASK_HANDLE telemetryTaskId;
RTOS_DEFINE_STACK(telemetryStack, TELEMETRY_STACK_SIZE);

RTOS_MUTEX_HANDLE audioMutex;
RTOS_MUTEX_HANDLE mixerMutex;

RTOS_FLAG_HANDLE openTxInitCompleteFlag;

enum TaskIndex {
  MENU_TASK_INDEX,
  MIXER_TASK_INDEX,
  TELEMETRY_TASK_INDEX,
  AUDIO_TASK_INDEX,
  CLI_TASK_INDEX,
  TOUCH_TASK_INDEX,
  BLUETOOTH_TASK_INDEX,
  TASK_INDEX_COUNT,
  MAIN_TASK_INDEX = 255
};

void stackPaint()
{
  menusStack.paint();
  mixerStack.paint();
  audioStack.paint();
  telemetryStack.paint();
#if defined(CLI)
  cliStack.paint();
#endif
#if IS_TOUCH_ENABLED()
  TouchManager::taskStack().paint();
#endif
}

volatile uint16_t timeForcePowerOffPressed = 0;

bool isForcePowerOffRequested()
{
  if (pwrOffPressed()) {
    if (timeForcePowerOffPressed == 0) {
      timeForcePowerOffPressed = get_tmr10ms();
    }
    else {
      uint16_t delay = (uint16_t)get_tmr10ms() - timeForcePowerOffPressed;
      if (delay > 1000/*10s*/) {
        return true;
      }
    }
  }
  else {
    resetForcePowerOffRequest();
  }
  return false;
}

bool isModuleSynchronous(uint8_t moduleIdx)
{
    switch(moduleState[moduleIdx].protocol) {
    case PROTOCOL_CHANNELS_PXX2_HIGHSPEED:
    case PROTOCOL_CHANNELS_PXX2_LOWSPEED:
    case PROTOCOL_CHANNELS_CROSSFIRE:
    case PROTOCOL_CHANNELS_NONE:
#if defined(AFHDS2)
    case PROTOCOL_CHANNELS_AFHDS2:
#endif
#if defined(AFHDS2)
    case PROTOCOL_CHANNELS_AFHDS3:
#endif
#if defined(MULTIMODULE)
    case PROTOCOL_CHANNELS_MULTIMODULE:
#endif
#if defined(INTMODULE_USART) || defined(EXTMODULE_USART)
    case PROTOCOL_CHANNELS_PXX1_SERIAL:
    return true;
#endif
#if defined(DSM2)
    case PROTOCOL_CHANNELS_SBUS:
    case PROTOCOL_CHANNELS_DSM2_LP45:
    case PROTOCOL_CHANNELS_DSM2_DSM2:
    case PROTOCOL_CHANNELS_DSM2_DSMX:
#endif
      return true;
  }
  return false;
}

void sendSynchronousPulses()
{
   static bool last[EXTERNAL_MODULE+1];
   bool current[EXTERNAL_MODULE+1];  
#if defined(HARDWARE_INTERNAL_MODULE)
  current[INTERNAL_MODULE] = isModuleSynchronous(INTERNAL_MODULE);
  if (current[INTERNAL_MODULE] || last[INTERNAL_MODULE]) {
    if (setupPulsesInternalModule())
      intmoduleSendNextFrame();
  }
  last[INTERNAL_MODULE] = current[INTERNAL_MODULE];
#endif
  current[EXTERNAL_MODULE] = isModuleSynchronous(EXTERNAL_MODULE);
  if (current[EXTERNAL_MODULE] || last[EXTERNAL_MODULE]) {
    if (setupPulsesExternalModule())
      extmoduleSendNextFrame();
  }
  last[INTERNAL_MODULE] = current[INTERNAL_MODULE];
}

RTOS_FLAG_HANDLE telemetryFlag;

TASK_FUNCTION(telemetryTask)
{
  while(1) {
#if defined(SIMU)
  if (pwrCheck() == e_power_off || main_thread_running == 0) {
    TASK_RETURN();
  }
#endif
  // if(!telemetryFlag) 
  //   break;
  RTOS_CLEAR_FLAG(telemetryFlag);
  RTOS_WAIT_FLAG(telemetryFlag, 10);
  DEBUG_TIMER_START(debugTimerTelemetryWakeup);
  telemetryWakeup();
  DEBUG_TIMER_STOP(debugTimerTelemetryWakeup);
  }
}


TASK_FUNCTION(mixerTask)
{
  s_pulses_paused = true;
  mixerSchedulerInit();
  mixerSchedulerStart();
  RTOS_CREATE_FLAG(telemetryFlag);
  while(1) {
#if defined(SBUS_TRAINER)
    processSbusInput();
#endif
#if defined(BLUETOOTH)
    bluetoothWakeup();
#endif
  //process telemetry when waiting
  RTOS_ISR_SET_FLAG(telemetryFlag);
  // run mixer at least every 30ms
  bool timeout = mixerSchedulerWaitForTrigger(30);
  // re-enable trigger
  mixerSchedulerEnableTrigger();

#if defined(SIMU)
  if (pwrCheck() == e_power_off || main_thread_running == 0) {
      TASK_RETURN();
  }
#else
    if (isForcePowerOffRequested()) {
      pwrOff();
    }
#endif

    if (!s_pulses_paused) {
      uint16_t t0 = getTmr2MHz();
      DEBUG_TIMER_START(debugTimerMixer);
      RTOS_LOCK_MUTEX(mixerMutex);
      doMixerCalculations();
      DEBUG_TIMER_START(debugTimerMixerCalcToUsage);
      DEBUG_TIMER_SAMPLE(debugTimerMixerIterval);
      RTOS_UNLOCK_MUTEX(mixerMutex);
      DEBUG_TIMER_STOP(debugTimerMixer);

#if defined(STM32) && !defined(SIMU)
      if (getSelectedUsbMode() == USB_JOYSTICK_MODE) {
        usbJoystickUpdate();
      }
#endif

      // DEBUG_TIMER_START(debugTimerTelemetryWakeup);
      // telemetryWakeup();
      // DEBUG_TIMER_STOP(debugTimerTelemetryWakeup);

      if (heartbeat == HEART_WDT_CHECK) {
        wdt_reset();
        heartbeat = 0;
      }

      t0 = getTmr2MHz() - t0;
      if (t0 > maxMixerDuration) 
        maxMixerDuration = t0;
      
      // TODO:
      // - check the cause of timeouts when switching
      //    between protocols with multi-proto RF
      if (timeout)
        TRACE("mix sched timeout!");

      sendSynchronousPulses();
    }
  }
}

#define MENU_TASK_PERIOD_TICKS      10    // 50ms
#define MENU_LUA_PERIOD_TICKS  250

#if defined(COLORLCD) && defined(CLI)
bool perMainEnabled = true;
#endif
uint8_t UsbModes = USB_UNSELECTED_MODE;

TASK_FUNCTION(menusTask)
{
  opentxInit();
#if defined(PWR_BUTTON_PRESS)
  while (1) {
 #if defined(PCBFLYSKY)
      static uint32_t UsbModeFlag = 0;
      if(USB_UNSELECTED_MODE ==UsbModes)
      {
          UsbModes = g_eeGeneral.USBMode;
      }
      if(usbPlugged() && (USB_UNSELECTED_MODE ==UsbModes) && (!unexpectedShutdown))
      {
         UsbModes = UsbModeSelect(UsbModeFlag);
         UsbModeFlag = 1;
         CoTickDelay(MENU_TASK_PERIOD_TICKS);
         continue;
      }
      else
      {
        UsbModeFlag = 0;
      }
 #endif
    uint32_t pwr_check = pwrCheck();
    if (pwr_check == e_power_off) {
      break;
    }
    else if (pwr_check == e_power_press) {
      CoTickDelay(MENU_TASK_PERIOD_TICKS);
      continue;
    }
#else
  while (pwrCheck() != e_power_off) {
#endif
    uint32_t start = (uint32_t)RTOS_GET_TIME();
    DEBUG_TIMER_START(debugTimerPerMain);
#if defined(COLORLCD) && defined(CLI)
    if (perMainEnabled) {
      perMain();
    }
#else
    perMain();
#endif
    DEBUG_TIMER_STOP(debugTimerPerMain);

#if defined (PCBFLYSKY) && defined(FLYSKY_AUTO_POWER_DOWN)
    uint32_t lowpwr_check = lowPowerCheck();
    if (lowpwr_check == e_power_off) {
      break;
      CoTickDelay(MENU_TASK_PERIOD_TICKS);
    }
    else if (lowpwr_check == e_power_low) {
      CoTickDelay(MENU_TASK_PERIOD_TICKS);
      continue;
    }
#endif
    // TODO remove completely massstorage from sky9x firmware
    uint32_t runtime = ((uint32_t)RTOS_GET_TIME() - start);
    // deduct the thread run-time from the wait, if run-time was more than
    // desired period, then skip the wait all together
    if (runtime < MENU_TASK_PERIOD_TICKS) {
      CoTickDelay(MENU_TASK_PERIOD_TICKS - runtime);
    }

    resetForcePowerOffRequest();

#if defined(SIMU)
    if (main_thread_running == 0)
      break;
#endif
  }

#if defined(PCBX9E)
  toplcdOff();
#endif

#if defined(PCBHORUS)
  ledOff();
#endif
  drawSleepBitmap();
  opentxClose();
  CoTickDelay(100);
  boardOff(); // Only turn power off if necessary
  TASK_RETURN();
}

void tasksStart()
{
  RTOS_INIT();

#if defined(CLI)
  cliStart();
#endif

  RTOS_CREATE_TASK(mixerTaskId, mixerTask, "Mixer", mixerStack, MIXER_STACK_SIZE, MIXER_TASK_PRIO);
  RTOS_CREATE_TASK(telemetryTaskId, telemetryTask, "Telemetry", telemetryStack, TELEMETRY_STACK_SIZE, TELEMETRY_TASK_PRIO);
  RTOS_CREATE_TASK(menusTaskId, menusTask, "Menus", menusStack,  MENUS_STACK_SIZE, MENUS_TASK_PRIO);

#if !defined(SIMU)
  RTOS_CREATE_TASK(audioTaskId, audioTask, "Audio", audioStack, AUDIO_STACK_SIZE, AUDIO_TASK_PRIO);
#endif

#if IS_TOUCH_ENABLED()
  TouchManager::instance()->init();  // init touch task
#endif

  RTOS_CREATE_MUTEX(audioMutex);
  RTOS_CREATE_MUTEX(mixerMutex);

  RTOS_CREATE_FLAG(openTxInitCompleteFlag);

  RTOS_START();
}
