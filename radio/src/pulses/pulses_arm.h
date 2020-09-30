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

#ifndef _PULSES_ARM_H_
#define _PULSES_ARM_H_

#include "CoOS.h"
#include "afhds3.h"
#include "multi.h"
#include "pxx1.h"
#include "pxx2.h"


#if NUM_MODULES == 2
  #define MODULES_INIT(...)            __VA_ARGS__, __VA_ARGS__
#else
  #define MODULES_INIT(...)            __VA_ARGS__
#endif

#if defined(PCBX12S) && PCBREV < 13
  #define pulse_duration_t             uint32_t
  #define trainer_pulse_duration_t     uint16_t
#else
  #define pulse_duration_t             uint16_t
  #define trainer_pulse_duration_t     uint16_t
#endif


extern uint8_t s_pulses_paused;
extern uint16_t failsafeCounter[NUM_MODULES];



PACK(struct PXX2Version {
  uint8_t major;
  uint8_t revision:4;
  uint8_t minor:4;
});

PACK(struct PXX2HardwareInformation {
  uint8_t modelID;
  PXX2Version hwVersion;
  PXX2Version swVersion;
  uint8_t variant;
  uint32_t capabilities; // variable length
  uint8_t capabilityNotSupported;
});

PACK(struct ModuleInformation {
  int8_t current;
  int8_t maximum;
  uint8_t timeout;
  PXX2HardwareInformation information;
  struct {
    PXX2HardwareInformation information;
    tmr10ms_t timestamp;
  } receivers[PXX2_MAX_RECEIVERS_PER_MODULE];
});

class ModuleSettings {
  public:
    uint8_t state;  // 0x00 = READ 0x40 = WRITE
    tmr10ms_t timeout;
    uint8_t externalAntenna;
    int8_t txPower;
    uint8_t dirty;
};

class ReceiverSettings {
  public:
    uint8_t state;  // 0x00 = READ 0x40 = WRITE
    tmr10ms_t timeout;
    uint8_t receiverId;
    uint8_t dirty;
    uint8_t telemetryDisabled;
    uint8_t telemetry25mw;
    uint8_t pwmRate;
    uint8_t fport;
    uint8_t enablePwmCh5Ch6;
    uint8_t outputsCount;
    uint8_t outputsMapping[24];
};

class BindInformation {
  public:
    int8_t step;
    uint32_t timeout;
    char candidateReceiversNames[PXX2_MAX_RECEIVERS_PER_MODULE][PXX2_LEN_RX_NAME + 1];
    uint8_t candidateReceiversCount;
    uint8_t selectedReceiverIndex;
    uint8_t rxUid;
    uint8_t lbtMode;
    uint8_t flexMode;
    PXX2HardwareInformation receiverInformation;
};

class OtaUpdateInformation: public BindInformation {
  public:
    char filename[_MAX_LFN + 1];
    uint32_t address;
    uint32_t module;
};

typedef void (* ModuleCallback)();

PACK(struct ModuleState {
  uint8_t protocol:4;
  uint8_t mode:4;
  uint8_t paused:1;
  uint8_t spare:7;
  uint16_t counter;
  union
  {
    ModuleInformation * moduleInformation;
    ModuleSettings * moduleSettings;
    ReceiverSettings * receiverSettings;
    BindInformation * bindInformation;
    OtaUpdateInformation * otaUpdateInformation;
  };
  ModuleCallback callback;

  void startBind(BindInformation * destination, ModuleCallback bindCallback = nullptr);

  void readModuleInformation(ModuleInformation * destination, int8_t first, int8_t last)
  {
    moduleInformation = destination;
    moduleInformation->current = first;
    moduleInformation->maximum = last;
    mode = MODULE_MODE_GET_HARDWARE_INFO;
  }

  void readModuleSettings(ModuleSettings * destination)
  {
    moduleSettings = destination;
    moduleSettings->state = PXX2_SETTINGS_READ;
    mode = MODULE_MODE_MODULE_SETTINGS;
  }

  void writeModuleSettings(ModuleSettings * source)
  {
    moduleSettings = source;
    moduleSettings->state = PXX2_SETTINGS_WRITE;
    moduleSettings->timeout = 0;
    mode = MODULE_MODE_MODULE_SETTINGS;
  }

  void readReceiverSettings(ReceiverSettings * destination)
  {
    receiverSettings = destination;
    receiverSettings->state = PXX2_SETTINGS_READ;
    mode = MODULE_MODE_RECEIVER_SETTINGS;
  }

  void writeReceiverSettings(ReceiverSettings * source)
  {
    receiverSettings = source;
    receiverSettings->state = PXX2_SETTINGS_WRITE;
    receiverSettings->timeout = 0;
    mode = MODULE_MODE_RECEIVER_SETTINGS;
  }
});

extern ModuleState moduleState[NUM_MODULES];

template<class T> struct PpmPulsesData {
  T pulses[20];
  T * ptr;
};

#if defined(PXX_FREQUENCY_HIGH)
#define EXTMODULE_USART_PXX_BAUDRATE 420000
#define INTMODULE_USART_PXX_BAUDRATE 450000

#define PXX_PERIOD                   4000 /*us*/
#else
#define EXTMODULE_USART_PXX_BAUDRATE 115200
#define INTMODULE_USART_PXX_BAUDRATE 115200

#endif

#define HALF_US_MULTI 200

#define PXX_PERIOD                  9000/*us*/
#define PXX_PERIOD_HALF_US          (PXX_PERIOD * 2)
#define AFHDS2_PERIOD               3850 /*us*/
#define AFHDS2_PERIOD_HALF_US       (AFHDS2_PERIOD * 2)

#define AFHDS3_PERIOD               4000 /*us*/
#define AFHDS3_PERIOD_HALF_US       (AFHDS3_PERIOD * 2)


#define PPM_PERIOD_HALF_US(module)   ((g_model.moduleData[module].ppm.frameLength * 5 + 225) * 200) /*half us*/
#define PPM_PERIOD(module)           (PPM_PERIOD_HALF_US(module) / 2) /*us*/

#define DSM2_BAUDRATE 125000
#define DSM2_PERIOD                  22000 /*us*/

#define SBUS_BAUDRATE 100000
#define SBUS_PERIOD_HALF_US ((g_model.moduleData[EXTERNAL_MODULE].sbus.refreshRate * 5 + 225) * HALF_US_MULTI)                                          /*half us*/
#define SBUS_PERIOD                  (SBUS_PERIOD_HALF_US / 2) /*us*/

#define MULTIMODULE_BAUDRATE         100000
#define MULTIMODULE_PERIOD           7000 /*us*/

#if defined(PPM_PIN_SERIAL)
PACK(struct Dsm2SerialPulsesData {
  uint8_t  pulses[64];
  uint8_t * ptr;
  uint8_t  serialByte ;
  uint8_t  serialBitCount;
  uint16_t _alignment;
});
typedef Dsm2SerialPulsesData Dsm2PulsesData;
#else
#define MAX_PULSES_TRANSITIONS 300
PACK(struct Dsm2TimerPulsesData {
  pulse_duration_t pulses[MAX_PULSES_TRANSITIONS];
  pulse_duration_t * ptr;
  uint8_t index;
});
typedef Dsm2TimerPulsesData Dsm2PulsesData;
#endif


#define MAX_PULSES_TRANSITIONS 300
#define CROSSFIRE_FRAME_MAXLEN         256
#define CROSSFIRE_CHANNELS_COUNT       16
PACK(struct CrossfirePulsesData {
  uint8_t pulses[CROSSFIRE_FRAME_MAXLEN];
  uint8_t length;
});

union InternalModulePulsesData {

#if defined(INTMODULE_USART)
  UartPxx1Pulses pxx_uart;
#else
  PwmPxx1Pulses pxx;
#endif

#if defined(INTMODULE_USART) && defined(AFHDS2)
  FlySkySerialPulsesData flysky;
#endif

#if defined(MULTIMODULE) //&& defined(INTMODULE_USART)
  UartMultiPulses multi;
#endif

#if defined(INTERNAL_MODULE_PPM)
  PpmPulsesData<pulse_duration_t> ppm;
#endif
} __ALIGNED(4);


union ExternalModulePulsesData {
#if defined(PXX1)
#if defined(EXTMODULE_USART)
  UartPxx1Pulses pxx_uart;
#endif
#if defined(PPM_PIN_SERIAL)
  SerialPxx1Pulses pxx;
#else
  PwmPxx1Pulses pxx;
#endif
#endif

#if defined(DSM2) || defined(MULTIMODULE) || defined(SBUS)
  Dsm2PulsesData dsm2;
#endif

#if defined(AFHDS3)
  FlySkySerialPulsesData flysky;
#endif

  PpmPulsesData<pulse_duration_t> ppm;

  CrossfirePulsesData crossfire;
} __ALIGNED(4);

/* The __ALIGNED keyword is required to align the struct inside the modulePulsesData below,
 * which is also defined to be __DMA  (which includes __ALIGNED) aligned.
 * Arrays in C/C++ are always defined to be *contiguously*. The first byte of the second element is therefore always
 * sizeof(ModulePulsesData). __ALIGNED is required for sizeof(ModulePulsesData) to be a multiple of the alignment.
 */

/* TODO: internal pulsedata only needs 200 bytes vs 300 bytes for external, both use 300 byte since we have a common struct */
extern InternalModulePulsesData intmodulePulsesData;
extern ExternalModulePulsesData extmodulePulsesData;

union TrainerPulsesData {
  PpmPulsesData<trainer_pulse_duration_t> ppm;
};

extern TrainerPulsesData trainerPulsesData;
extern const uint16_t CRCTable[];
#if defined(INTMODULE) || (HARDWARE_INTERNAL_MODULE)
extern bool internalModuleUpdate;
bool setupPulsesInternalModule();
#endif
bool setupPulsesExternalModule();
void setupPulsesDSM2();
void setupPulsesCrossfire();
void setupPulsesMultiExternalModule();
void setupPulsesMultiInternalModule();
void setupPulsesSbus();
void setupPulsesPPMInternalModule();
void setupPulsesPPMExternalModule();
void setupPulsesPPMTrainer();

void sendByteDsm2(uint8_t b);
void putDsm2Flush();
void putDsm2SerialBit(uint8_t bit);
void sendByteSbus(uint8_t byte);
void intmodulePpmStart();
void intmodulePxx1PulsesStart();
void intmodulePxx1SerialStart();
void extmodulePxx1PulsesStart();
void extmodulePxx1SerialStart();
void extmodulePpmStart();
void intmoduleStop();
void extmoduleStop();
#if defined(HARDWARE_EXTRA_MODULE)
void extramodulePpmStart();
#endif
#if defined AFHDS2
void setupPulsesAFHDS2();
void resetPulsesAFHDS2();
void setFlyskyState(uint8_t state);
void onFlySkyBindReceiver();
void onFlySkyModuleSetPower(bool isPowerOn);
void afhds2Command(uint8_t type, uint8_t cmd);
void intmoduleSendBufferDMA(uint8_t * data, uint16_t size);
void onFlySkyGetVersionInfoStart(uint8_t isRfTransfer);
void usbDownloadTransmit(uint8_t *buffer, uint32_t size);

#define END                             0xC0
#define ESC                             0xDB
#define ESC_END                         0xDC
#define ESC_ESC                         0xDD

#define FRAME_TYPE_REQUEST_ACK          0x01
#define FRAME_TYPE_REQUEST_NACK         0x02
#define FRAME_TYPE_ANSWER               0x10

enum FlySkyModuleCommandID {
  CMD_NONE,
  CMD_RF_INIT,
  CMD_BIND,
  CMD_SET_RECEIVER_ID,
  CMD_RF_GET_CONFIG,
  CMD_SEND_CHANNEL_DATA,
  CMD_RX_SENSOR_DATA,
  CMD_SET_RX_PWM_PPM,
  CMD_SET_RX_SERVO_FREQ,
  CMD_GET_VERSION_INFO,
  CMD_SET_RX_IBUS_SBUS,
  CMD_SET_RX_IBUS_SERVO_EXT,
  CMD_UPDATE_RF_FIRMWARE = 0x0C,
  CMD_SET_TX_POWER = 0x0D,
  CMD_SET_RF_PROTOCOL,
  CMD_TEST_RANGE,
  CMD_TEST_RF_RESERVED,
  CMD_UPDATE_RX_FIRMWARE = 0x20,
  CMD_LAST
};
#endif

#if defined(AFHDS3)
extern afhds3::afhds3 afhds3uart;
#endif

inline void startPulses()
{
  s_pulses_paused = false;

#if defined(INTMODULE) || (HARDWARE_INTERNAL_MODULE)
  setupPulsesInternalModule();
#endif

  setupPulsesExternalModule();

#if defined(HARDWARE_EXTRA_MODULE)
  extramodulePpmStart();
#endif
}

enum ChannelsProtocols {
  PROTOCOL_CHANNELS_UNINITIALIZED,
  PROTOCOL_CHANNELS_NONE,
  PROTOCOL_CHANNELS_PPM,
  PROTOCOL_CHANNELS_PXX1_PULSES,
  PROTOCOL_CHANNELS_PXX1_SERIAL,
  PROTOCOL_CHANNELS_DSM2_LP45,
  PROTOCOL_CHANNELS_DSM2_DSM2,
  PROTOCOL_CHANNELS_DSM2_DSMX,
  PROTOCOL_CHANNELS_CROSSFIRE,
  PROTOCOL_CHANNELS_MULTIMODULE,
  PROTOCOL_CHANNELS_SBUS,
  PROTOCOL_CHANNELS_PXX2_LOWSPEED,
  PROTOCOL_CHANNELS_PXX2_HIGHSPEED,
  PROTOCOL_CHANNELS_AFHDS3,
  PROTOCOL_CHANNELS_AFHDS2
};

inline void stopPulses()
{
  s_pulses_paused = true;
  moduleState[0].protocol = PROTOCOL_CHANNELS_UNINITIALIZED;
}

// extern OS_FlagID pulseFlag;

inline bool pulsesStarted()
{
  return moduleState[0].protocol != PROTOCOL_CHANNELS_UNINITIALIZED;
}
inline void pausePulses()
{
  s_pulses_paused = true;
}
inline void resumePulses()
{
  s_pulses_paused = false;
}

#define SEND_FAILSAFE_NOW(idx) failsafeCounter[idx] = 1

inline void SEND_FAILSAFE_1S()
{
  for (int i=0; i<NUM_MODULES; i++) {
    failsafeCounter[i] = 100;
  }
}

// Assign failsafe values using the current channel outputs
// for channels not set previously to HOLD or NOPULSE
void setCustomFailsafe(uint8_t moduleIndex);

inline bool isModuleInRangeCheckMode()
{
  if (moduleState[0].mode == MODULE_MODE_RANGECHECK)
    return true;

#if NUM_MODULES > 1
  if (moduleState[1].mode == MODULE_MODE_RANGECHECK)
    return true;
#endif

  return false;
}

inline bool isModuleInBeepMode()
{
  if (moduleState[0].mode >= MODULE_MODE_BEEP_FIRST)
    return true;

#if NUM_MODULES > 1
  if (moduleState[1].mode >= MODULE_MODE_BEEP_FIRST)
    return true;
#endif

  return false;
}

#define LEN_R9M_MODES                  "\007"
#define TR_R9M_MODES                   "FCC\0   ""LBT(EU)"
#define LEN_R9M_FCC_POWER_VALUES       "\006"
#define LEN_R9M_LBT_POWER_VALUES       "\006"
#define TR_R9M_FCC_POWER_VALUES        "10 mW\0" "100 mW" "500 mW" "1 W\0"
#define TR_R9M_LBT_POWER_VALUES        "25 mW\0" "500 mW"

enum FlySkyModuleState_E {
  STATE_SET_TX_POWER = 0,
  STATE_INIT = 1,
  STATE_BIND = 2,
  STATE_SET_RECEIVER_ID = 3,
  STATE_SET_RX_PWM_PPM = 4,
  STATE_SET_RX_IBUS_SBUS = 5,
  STATE_SET_RX_FREQUENCY = 6,
  STATE_UPDATE_RF_FIRMWARE = 7,
  STATE_UPDATE_RX_FIRMWARE = 8,
  STATE_UPDATE_HALL_FIRMWARE = 9,
  STATE_UPDATE_RF_PROTOCOL = 10,
  STATE_GET_RECEIVER_CONFIG = 11,
  STATE_GET_RX_VERSION_INFO = 12,
  STATE_GET_RF_VERSION_INFO = 13,
  STATE_SET_RANGE_TEST = 14,
  STATE_RANGE_TEST_RUNNING = 15,
  STATE_IDLE = 16,
  STATE_SEND_CHANNELS = 17,
};

#define BIND_TELEM_ALLOWED(idx)      (!isModuleR9M_LBT(idx) || g_model.moduleData[idx].pxx.power == R9M_LBT_POWER_25)
#define BIND_CH9TO16_ALLOWED(idx)    (!isModuleR9M_LBT(idx) || g_model.moduleData[idx].pxx.power != R9M_LBT_POWER_25)

#endif // _PULSES_ARM_H_
