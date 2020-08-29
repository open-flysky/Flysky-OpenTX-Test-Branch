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

#ifndef _MODULES_H_
#define _MODULES_H_

#include "myeeprom.h"
#include "modules_constants.h"
#define CROSSFIRE_CHANNELS_COUNT        16

#if defined(MULTIMODULE)
#include "multi.h"
#include "telemetry/multi.h"
// When using packed, the pointer in here end up not being aligned, which clang and gcc complain about
// Keep the order of the fields that the so that the size stays small
struct mm_options_strings {
  static const char* options[];
};

struct mm_protocol_definition {
  uint8_t protocol;
  uint8_t maxSubtype;
  bool failsafe;
  bool disable_ch_mapping;
  const char *subTypeString;
  const char *optionsstr;
};

const mm_protocol_definition *getMultiProtocolDefinition (uint8_t protocol);

inline uint8_t getMaxMultiSubtype(uint8_t moduleIdx)
{
  MultiModuleStatus &status = getMultiModuleStatus(moduleIdx);
  const mm_protocol_definition *pdef = getMultiProtocolDefinition(g_model.moduleData[moduleIdx].getMultiProtocol());

  if (g_model.moduleData[moduleIdx].getMultiProtocol() == MODULE_SUBTYPE_MULTI_FRSKY) {
    return 7;
  }

  if (g_model.moduleData[moduleIdx].getMultiProtocol() > MODULE_SUBTYPE_MULTI_LAST) {
    if (status.isValid())
      return (status.protocolSubNbr == 0 ? 0 : status.protocolSubNbr - 1);
    else
      return 7;
  }
  else {
    return max((uint8_t )(status.protocolSubNbr == 0 ? 0 : status.protocolSubNbr - 1), pdef->maxSubtype);
  }
}



inline bool isModuleMultimodule(uint8_t idx)
{
  return g_model.moduleData[idx].type == MODULE_TYPE_MULTIMODULE;
}

inline bool isModuleMultimoduleDSM2(uint8_t idx)
{
  return isModuleMultimodule(idx) && g_model.moduleData[idx].getMultiProtocol() == MODULE_SUBTYPE_MULTI_DSM2;
}
#else
inline bool isModuleMultimodule(uint8_t)
{
  return false;
}

inline bool isModuleMultimoduleDSM2(uint8_t)
{
  return false;
}

inline void resetMultiProtocolsOptions(uint8_t moduleIdx)
{
  if (!isModuleMultimodule(moduleIdx))
    return;

  // Sensible default for DSM2 (same as for ppm): 7ch@22ms + Autodetect settings enabled
  if (g_model.moduleData[moduleIdx].getMultiProtocol() == MODULE_SUBTYPE_MULTI_DSM2) {
    g_model.moduleData[moduleIdx].multi.autoBindMode = 1;
  }
  else {
    g_model.moduleData[moduleIdx].multi.autoBindMode = 0;
  }
  g_model.moduleData[moduleIdx].multi.optionValue = 0;
  g_model.moduleData[moduleIdx].multi.disableTelemetry = 0;
  g_model.moduleData[moduleIdx].multi.disableMapping = 0;
  g_model.moduleData[moduleIdx].multi.lowPowerMode = 0;
  g_model.moduleData[moduleIdx].failsafeMode = FAILSAFE_NOT_SET;
  g_model.header.modelId[moduleIdx] = 0;
}
#endif

#if defined(PCBFLYSKY)
inline bool isModuleFlysky(uint8_t idx)
{
  return g_model.moduleData[idx].type == MODULE_TYPE_FLYSKY;
}
#else
inline bool isModuleFlysky(uint8_t idx)
{
  return idx == EXTERNAL_MODULE && g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_FLYSKY;
}
#endif

inline bool isModuleAFHDS3(uint8_t idx)
{
  return g_model.moduleData[idx].type == MODULE_TYPE_AFHDS3;
}


#if defined(PCBFRSKY)
inline bool isModuleXJT(uint8_t idx)
{
  return g_model.moduleData[idx].type == MODULE_TYPE_XJT_PXX1;
}
#else
inline bool isModuleXJT(uint8_t idx)
{
  return idx == EXTERNAL_MODULE && g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_XJT_PXX1;
}
#endif

#if defined(CROSSFIRE)
inline bool isModuleCrossfire(uint8_t idx)
{
  return idx == EXTERNAL_MODULE && g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_CROSSFIRE;
}
#else
inline bool isModuleCrossfire(uint8_t idx)
{
  return false;
}
#endif

#if defined(EXTRA_MODULE)
inline bool isExtraModule(uint8_t idx)
{
  return idx == EXTRA_MODULE;
}
#else
inline bool isExtraModule(uint8_t)
{
  return false;
}
#endif

#if defined(TARANIS_INTERNAL_PPM)
inline bool isModulePPM(uint8_t idx)
{
  return idx == TRAINER_MODULE ||
         (idx == INTERNAL_MODULE && g_model.moduleData[INTERNAL_MODULE].type == MODULE_TYPE_PPM) ||
         (idx == EXTERNAL_MODULE && g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_PPM));
}
#else
inline bool isModulePPM(uint8_t idx)
{
  return idx == TRAINER_MODULE ||
         isExtraModule(idx) ||
         (idx == EXTERNAL_MODULE && g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_PPM);
}
#endif

inline bool isModuleTypeR9MNonAccess(uint8_t type)
{
  return type == MODULE_TYPE_R9M_PXX1 || type == MODULE_TYPE_R9M_LITE_PXX1 || type == MODULE_TYPE_R9M_LITE_PRO_PXX1;
}

inline bool isModuleR9MNonAccess(uint8_t idx)
{
  return isModuleTypeR9MNonAccess(g_model.moduleData[idx].type);
}

inline bool isModuleTypeR9MAccess(uint8_t type)
{
  return type == MODULE_TYPE_R9M_PXX2 || type == MODULE_TYPE_R9M_LITE_PXX2 || type == MODULE_TYPE_R9M_LITE_PRO_PXX2;
}

inline bool isModuleR9MAccess(uint8_t idx)
{
  return isModuleTypeR9MAccess(g_model.moduleData[idx].type);
}

inline bool isModuleTypeR9M(uint8_t type)
{
  return isModuleTypeR9MNonAccess(type) || isModuleTypeR9MAccess(type);
}

inline bool isModuleR9M(uint8_t idx)
{
  return isModuleTypeR9M(g_model.moduleData[idx].type);
}

inline bool isModuleTypeR9MLiteNonPro(uint8_t type)
{
  return type == MODULE_TYPE_R9M_LITE_PXX1 || type == MODULE_TYPE_R9M_LITE_PXX2;
}

inline bool isModuleR9MLiteNonPro(uint8_t idx)
{
  return isModuleTypeR9MLiteNonPro(g_model.moduleData[idx].type);
}

inline bool isModuleTypeR9MLitePro(uint8_t type)
{
  return type == MODULE_TYPE_R9M_LITE_PRO_PXX1 || type == MODULE_TYPE_R9M_LITE_PRO_PXX2;
}

inline bool isModuleTypeR9MLite(uint8_t type)
{
  return isModuleTypeR9MLiteNonPro(type) || isModuleTypeR9MLitePro(type);
}

inline bool isModuleR9MLite(uint8_t idx)
{
  return isModuleTypeR9MLite(g_model.moduleData[idx].type);
}

inline bool isModuleR9M_FCC(uint8_t idx)
{
  return isModuleR9MNonAccess(idx) && g_model.moduleData[idx].subType == MODULE_SUBTYPE_R9M_FCC;
}

inline bool isModuleTypeLite(uint8_t type)
{
  return isModuleTypeR9MLite(type) || type == MODULE_TYPE_XJT_LITE_PXX2;
}

inline bool isModuleR9M_LBT(uint8_t idx)
{
  return isModuleR9MNonAccess(idx) && g_model.moduleData[idx].subType == MODULE_SUBTYPE_R9M_EU;
}

inline bool isModuleR9M_FCC_VARIANT(uint8_t idx)
{
  return isModuleR9MNonAccess(idx) && g_model.moduleData[idx].subType != MODULE_SUBTYPE_R9M_EU;
}

inline bool isModuleR9M_EUPLUS(uint8_t idx)
{
  return isModuleR9MNonAccess(idx) && g_model.moduleData[idx].subType == MODULE_SUBTYPE_R9M_EUPLUS;
}

inline bool isModuleR9M_AU_PLUS(uint8_t idx)
{
  return isModuleR9MNonAccess(idx) && g_model.moduleData[idx].subType == MODULE_SUBTYPE_R9M_AUPLUS;
}

inline bool isModuleTypeXJT(uint8_t type)
{
  return type == MODULE_TYPE_XJT_PXX1 || type == MODULE_TYPE_XJT_LITE_PXX2;
}

inline bool isModulePXX(uint8_t idx)
{
  return isModuleXJT(idx) || isModuleR9M(idx);
}

inline bool isModuleISRM(uint8_t idx)
{
  return g_model.moduleData[idx].type == MODULE_TYPE_ISRM_PXX2;
}

inline bool isModuleTypePXX1(uint8_t type)
{
  return isModuleTypeXJT(type) || isModuleTypeR9MNonAccess(type);
}

inline bool isModulePXX1(uint8_t idx)
{
  return isModuleTypePXX1(g_model.moduleData[idx].type);
}

inline bool isModulePXX2(uint8_t idx)
{
  return isModuleISRM(idx) || isModuleR9MAccess(idx);
}

#if defined(DSM2)
inline bool isModuleDSM2(uint8_t idx)
{
  return idx == EXTERNAL_MODULE && g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_DSM2;
}

inline bool isModuleSBUS(uint8_t idx)
{
  return idx == EXTERNAL_MODULE && g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_SBUS;
}
#else
inline bool isModuleDSM2(uint8_t idx)
{
  return false;
}
#endif

// order is the same as in enum Protocols in myeeprom.h (none, ppm, pxx, flysky, dsm, crossfire, multi, r9m, sbus)
static const int8_t maxChannelsModules_M8[] = {
    0, //MODULE_TYPE_NONE
    8, //MODULE_TYPE_PPM
    8, //MODULE_TYPE_XJT
    6, //MODULE_TYPE_FLYSKY
    -2, //MODULE_TYPE_DSM2
    8, //MODULE_TYPE_CROSSFIRE
    4, //MODULE_TYPE_MULTIMODULE
    8, //MODULE_TYPE_R9M
    8, //MODULE_TYPE_SBUS
    10  //MODULE_TYPE_AFHDS3
}; // relative to 8!

static const int8_t maxChannelsXJT[] = {0, 8, 0, 4}; // relative to 8!

constexpr int8_t MAX_TRAINER_CHANNELS_M8 = MAX_TRAINER_CHANNELS - 8;
constexpr int8_t MAX_EXTRA_MODULE_CHANNELS_M8 = 8; // only 16ch PPM

inline int8_t maxModuleChannels_M8(uint8_t idx)
{
  if (isExtraModule(idx))
    return MAX_EXTRA_MODULE_CHANNELS_M8;
  else if (idx == TRAINER_MODULE)
    return MAX_TRAINER_CHANNELS_M8;
  else if (isModuleXJT(idx))
    return maxChannelsXJT[1 + g_model.moduleData[idx].rfProtocol];
  else
    return maxChannelsModules_M8[g_model.moduleData[idx].type];
}

inline int8_t maxModuleChannels(uint8_t idx)
{
  return 8 + maxModuleChannels_M8(idx);
}

inline int8_t minModuleChannels(uint8_t idx)
{
  if (isModuleCrossfire(idx))
    return 16;
  else
    return 1;
}

inline int8_t defaultModuleChannels_M8(uint8_t idx)
{
  if (isModulePPM(idx))
    return 0; // 8 channels
  else if (isModuleDSM2(idx))
    return 0; // 8 channels
  else if (isModuleMultimoduleDSM2(idx))
    return -1; // 7 channels
  else if (isModuleFlysky(idx))
    return 6; // 14 channels
  else if (isModuleAFHDS3(idx))
    return 10; // 18 channels
  else
    return 8; // 16 channels
}

inline uint8_t sentModulePXXChannels(uint8_t idx)
{
  return 8 + g_model.moduleData[idx].channelsCount;
}

inline int8_t sentModuleChannels(uint8_t idx)
{
  if (isModuleCrossfire(idx))
    return CROSSFIRE_CHANNELS_COUNT;
  else if (isModuleMultimodule(idx) && !isModuleMultimoduleDSM2(idx))
    return 16;
  else
    return 8 + g_model.moduleData[idx].channelsCount;
}

inline bool isModuleTypeAllowed(uint8_t idx, uint8_t type)
{
#if defined(PCBFLYSKY)
  if (idx == INTERNAL_MODULE) {
    return (type == MODULE_TYPE_NONE || type == MODULE_TYPE_FLYSKY);
  }
  else if (idx == EXTERNAL_MODULE) {
#if defined(MULTIMODULE)
    if(type == MODULE_TYPE_MULTIMODULE) return true;
#endif
    return (type == MODULE_TYPE_NONE || type == MODULE_TYPE_PPM
         || type == MODULE_TYPE_XJT_PXX1 || type == MODULE_TYPE_CROSSFIRE
         || type == MODULE_TYPE_R9M_PXX1 || type == MODULE_TYPE_AFHDS3);
  }
#endif

  return true;
}

//TBD
inline bool isModuleNeedingReceiverNumber(uint8_t idx)
{
  if(isModuleXJT(idx)) {
    return g_model.moduleData[idx].rfProtocol != RF_PROTO_D8;
  }
  return isModulePXX(idx) || isModuleDSM2(idx) || isModuleMultimodule(idx);
}

//TBD
inline bool isModuleNeedingBindRangeButtons(uint8_t idx)
{
  return isModulePXX(idx) || isModuleDSM2(idx) || isModuleMultimodule(idx) || isModuleFlysky(idx) || isModuleAFHDS3(idx);
}

//TBD
inline bool isModuleNeedingRangeButtons(uint8_t idx)
{
  return isModuleNeedingBindRangeButtons(idx) && !isModuleAFHDS3(idx);
}

//TBD
inline bool isModuleNeedingFailsafeButton(uint8_t idx)
{
  if(isModuleXJT(idx)){
    return g_model.moduleData[idx].rfProtocol == RF_PROTO_X16;
  }
  return isModulePXX(idx) || isModuleR9M(idx) || isModuleFlysky(idx) || isModuleAFHDS3(idx);
}


#endif // _MODULES_H_
