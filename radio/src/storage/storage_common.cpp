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
#include "CoOS.h"

uint8_t   storageDirtyMsk;
tmr10ms_t storageDirtyTime10ms;

#if defined(RAMBACKUP)
uint8_t   rambackupDirtyMsk;
tmr10ms_t rambackupDirtyTime10ms;
#endif

void storageDirty(uint8_t msk)
{
  storageDirtyMsk |= msk;
  storageDirtyTime10ms = get_tmr10ms();

#if defined(RAMBACKUP)
  rambackupDirtyMsk = storageDirtyMsk;
  rambackupDirtyTime10ms = storageDirtyTime10ms;
#endif
}

void preModelLoad()
{
#if defined(CPUARM)
  watchdogSuspend(500/*5s*/);
#endif

#if defined(SDCARD)
  logsClose();
#endif

  if (pulsesStarted()) {
    disconnectModel();
  }

  pauseMixerCalculations();
}
#if defined(PCBTARANIS) || defined(PCBHORUS) || defined(PCBNV14)
static void fixUpModel()
{
  //ensure rfProtocol is equal subType for PXX1 and DSM2
  for (int i=0; i<NUM_MODULES+1; i++) {
    if ((g_model.moduleData[i].type == MODULE_TYPE_XJT_PXX1 || g_model.moduleData[i].type == MODULE_TYPE_DSM2) && g_model.moduleData[i].subType != g_model.moduleData[i].rfProtocol) {
      g_model.moduleData[i].subType = g_model.moduleData[i].rfProtocol;
    }
    // Ensure that when subType is out of protocl range the type of the module is MODULE_TYPE_NONE
    if (g_model.moduleData[i].type == MODULE_TYPE_XJT_PXX1 && g_model.moduleData[i].subType > RF_PROTO_LAST)
      g_model.moduleData[i].type = MODULE_TYPE_NONE;
  }

  if (g_model.moduleData[INTERNAL_MODULE].failsafeMode > FAILSAFE_LAST) g_model.moduleData[INTERNAL_MODULE].failsafeMode = FAILSAFE_NOT_SET;
  if (g_model.moduleData[EXTERNAL_MODULE].failsafeMode > FAILSAFE_LAST) g_model.moduleData[EXTERNAL_MODULE].failsafeMode = FAILSAFE_NOT_SET;



}
#endif

void postModelLoad(bool alarms)
{
#if defined(PXX2)
  if (is_memclear(g_model.modelRegistrationID, PXX2_LEN_REGISTRATION_ID)) {
    memcpy(g_model.modelRegistrationID, g_eeGeneral.ownerRegistrationID, PXX2_LEN_REGISTRATION_ID);
  }
#endif
#if defined(HARDWARE_INTERNAL_MODULE)
  if (!isInternalModuleAvailable(g_model.moduleData[INTERNAL_MODULE].type)) {
    memclear(&g_model.moduleData[INTERNAL_MODULE], sizeof(ModuleData));
  }
#if defined(MULTIMODULE)
  else if (isModuleMultimodule(INTERNAL_MODULE))
    multiPatchCustom(INTERNAL_MODULE);
#endif
  setModuleFlag(INTERNAL_MODULE, MODULE_MODE_RESET_SETTINGS);
  setModuleFlag(INTERNAL_MODULE, MODULE_MODE_NORMAL);
#endif

if (!isExternalModuleAvailable(g_model.moduleData[EXTERNAL_MODULE].type)) {
    memclear(&g_model.moduleData[EXTERNAL_MODULE], sizeof(ModuleData));
  }
#if defined(MULTIMODULE)
  else if (isModuleMultimodule(EXTERNAL_MODULE))
    multiPatchCustom(EXTERNAL_MODULE);
#endif
  setModuleFlag(EXTERNAL_MODULE, MODULE_MODE_RESET_SETTINGS);
  setModuleFlag(EXTERNAL_MODULE, MODULE_MODE_NORMAL);
#if defined(PCBTARANIS) || defined(PCBHORUS) || defined(PCBNV14)
  fixUpModel();
#endif


  AUDIO_FLUSH();
  flightReset(false);

  customFunctionsReset();

  restoreTimers();

#if defined(CPUARM)
  for (int i=0; i<MAX_TELEMETRY_SENSORS; i++) {
    TelemetrySensor & sensor = g_model.telemetrySensors[i];
    if (sensor.type == TELEM_TYPE_CALCULATED && sensor.persistent) {
      telemetryItems[i].value = sensor.persistentValue;
      telemetryItems[i].lastReceived = TELEMETRY_VALUE_OLD;   // #3595: make value visible even before the first new value is received)
    }
  }
#endif

  LOAD_MODEL_CURVES();

  resumeMixerCalculations();
  if (pulsesStarted()) {
#if defined(GUI)
    if (alarms) {
      checkAll();
      PLAY_MODEL_NAME();
    }
#endif
    resumePulses();
  }

#if defined(TELEMETRY_FRSKY)
  frskySendAlarms();
#endif

#if defined(CPUARM) && defined(SDCARD)
  referenceModelAudioFiles();
#endif

#if defined(COLORLCD)
  loadCustomScreens();
#endif

  LOAD_MODEL_BITMAP();
  LUA_LOAD_MODEL_SCRIPTS();
  SEND_FAILSAFE_1S();
}

void storageFlushCurrentModel()
{
  saveTimers();

#if defined(CPUARM)
  for (int i=0; i<MAX_TELEMETRY_SENSORS; i++) {
    TelemetrySensor & sensor = g_model.telemetrySensors[i];
    if (sensor.type == TELEM_TYPE_CALCULATED && sensor.persistent && sensor.persistentValue != telemetryItems[i].value) {
      sensor.persistentValue = telemetryItems[i].value;
      storageDirty(EE_MODEL);
    }
  }
#endif

#if defined(CPUARM)
  if (g_model.potsWarnMode == POTS_WARN_AUTO) {
    for (int i=0; i<NUM_POTS+NUM_SLIDERS; i++) {
      if (!(g_model.potsWarnEnabled & (1 << i))) {
        SAVE_POT_POSITION(i);
      }
    }
    storageDirty(EE_MODEL);
  }
#endif
}
