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

extern uint8_t s_current_protocol[NUM_MODULES];
extern uint8_t s_pulses_paused;
extern uint16_t failsafeCounter[NUM_MODULES];

template<class T> struct PpmPulsesData {
  T pulses[20];
  T * ptr;
};

#if defined(PXX_FREQUENCY_HIGH)
#define EXTMODULE_USART_PXX_BAUDRATE 420000
#define INTMODULE_USART_PXX_BAUDRATE 450000

#define PXX_PERIOD 4 /*ms*/
#else
#define EXTMODULE_USART_PXX_BAUDRATE 115200
#define INTMODULE_USART_PXX_BAUDRATE 115200

#define FLYSKY_PERIOD 9 /*ms*/
#endif

#define HALF_US_MULTI 200
#define PERIOD_LENGHT 2000

#define PXX_PERIOD 9 /*ms*/
#define PXX_PERIOD_HALF_US (PXX_PERIOD * PERIOD_LENGHT)

#define PPM_PERIOD_HALF_US(module) (g_model.moduleData[module].ppm.frameLength * 5 + 225) * HALF_US_MULTI)
#define PPM_PERIOD(module) (PPM_PERIOD_HALF_US(module) / PERIOD_LENGHT)

#define DSM2_BAUDRATE 125000
#define DSM2_PERIOD 22 /*ms*/

#define SBUS_BAUDRATE 100000
#define SBUS_PERIOD_HALF_US ((g_model.moduleData[EXTERNAL_MODULE].sbus.refreshRate * 5 + 225) * HALF_US_MULTI)                                          /*half us*/
#define SBUS_PERIOD (SBUS_PERIOD_HALF_US / PERIOD_LENGHT)

#define MULTIMODULE_BAUDRATE 100000
#define MULTIMODULE_PERIOD 7 /*ms*/

#if defined(PPM_PIN_SERIAL)
PACK(struct PxxSerialPulsesData {
  uint8_t  pulses[64];
  uint8_t  * ptr;
  uint16_t pcmValue;
  uint16_t pcmCrc;
  uint32_t pcmOnesCount;
  uint16_t serialByte;
  uint16_t serialBitCount;
});

PACK(struct Dsm2SerialPulsesData {
  uint8_t  pulses[64];
  uint8_t * ptr;
  uint8_t  serialByte ;
  uint8_t  serialBitCount;
  uint16_t _alignment;
});
#endif

#if defined(INTMODULE_USART)
PACK(struct PxxUartPulsesData {
  uint8_t  pulses[64];
  uint8_t  * ptr;
  uint16_t pcmCrc;
  uint16_t _alignment;
});
PACK(struct FlySkySerialPulsesData {
  uint8_t  pulses[64];
  uint8_t  * ptr;
  uint8_t  frame_index;
  uint8_t  crc;
  uint8_t  state;
  uint8_t  state_index;
  uint8_t  esc_state;
  uint8_t  telemetry[64];
  uint8_t  telemetry_index;
});
#endif

#if defined(INTMODULE_PULSES) || defined(EXTMODULE_PULSES)
/* PXX uses 20 bytes (as of Rev 1.1 document) with 8 changes per byte + stop bit ~= 162 max pulses */
/* DSM2 uses 2 header + 12 channel bytes, with max 10 changes (8n2) per byte + 16 bits trailer ~= 156 max pulses */
/* Multimodule uses 3 bytes header + 22 channel bytes with max 11 changes per byte (8e2) + 16 bits trailer ~= 291 max pulses */
/* Multimodule reuses some of the DSM2 function and structs since the protocols are similar enough */
/* sbus is 1 byte header, 22 channel bytes (11bit * 16ch) + 1 byte flags */
PACK(struct PxxTimerPulsesData {
  pulse_duration_t pulses[200];
  pulse_duration_t * ptr;
  uint16_t rest;
  uint16_t pcmCrc;
  uint32_t pcmOnesCount;
});

#define MAX_PULSES_TRANSITIONS 300

PACK(struct Dsm2TimerPulsesData {
  pulse_duration_t pulses[MAX_PULSES_TRANSITIONS];
  pulse_duration_t * ptr;
  uint16_t rest;
  uint8_t index;
});
#endif

#define CROSSFIRE_FRAME_MAXLEN         256
#define CROSSFIRE_CHANNELS_COUNT       16
PACK(struct CrossfirePulsesData {
  uint8_t pulses[CROSSFIRE_FRAME_MAXLEN];
});

union ModulePulsesData {
#if defined(PPM_PIN_SERIAL)
  PxxSerialPulsesData pxx;
  Dsm2SerialPulsesData dsm2;
#endif
#if defined(INTMODULE_PULSES) || defined(EXTMODULE_PULSES)
  PxxTimerPulsesData pxx;
  Dsm2TimerPulsesData dsm2;
#endif
#if defined(INTMODULE_USART)
  PxxUartPulsesData pxx_uart;
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
extern ModulePulsesData modulePulsesData[NUM_MODULES];

union TrainerPulsesData {
  PpmPulsesData<trainer_pulse_duration_t> ppm;
};

extern TrainerPulsesData trainerPulsesData;
extern const uint16_t CRCTable[];
extern uint8_t tx_working_power;
void setupPulses(uint8_t port);
void setupPulsesDSM2(uint8_t port);
void setupPulsesMultimodule(uint8_t port);
void setupPulsesSbus(uint8_t port);
void setupPulsesPXX(uint8_t port);
void resetPulsesFlySky(uint8_t port);
void setupPulsesFlySky(uint8_t port);
void setupPulsesPPMModule(uint8_t port);
void setupPulsesPPMTrainer();
void sendByteDsm2(uint8_t b);
void putDsm2Flush();
void putDsm2SerialBit(uint8_t bit);
void sendByteSbus(uint8_t byte);
void setFlyskyState(uint8_t port, uint8_t state);
void onFlySkyBindReceiver(uint8_t port);
void onFlySkyModuleSetPower(uint8_t port, bool isPowerOn);
void intmoduleSendBufferDMA(uint8_t * data, uint8_t size);
void onFlySkyGetVersionInfoStart(uint8_t port, uint8_t isRfTransfer);
void usbDownloadTransmit(uint8_t *buffer, uint32_t size);

#if defined(HUBSAN)
void Hubsan_Init();
#endif

inline void startPulses()
{
  s_pulses_paused = false;

#if defined(INTMODULE)
  setupPulses(INTERNAL_MODULE);
#endif

  setupPulses(EXTERNAL_MODULE);

#if defined(HUBSAN)
  Hubsan_Init();
#endif
}

extern OS_FlagID pulseFlag;
inline bool pulsesStarted() { return s_current_protocol[0] != 255; }
inline void pausePulses() { s_pulses_paused = true; }
inline void resumePulses() { s_pulses_paused = false; }

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

#define LEN_R9M_MODES                  "\007"
#define TR_R9M_MODES                   "FCC\0   ""LBT(EU)"
#define LEN_R9M_FCC_POWER_VALUES       "\006"
#define LEN_R9M_LBT_POWER_VALUES       "\006"
#define TR_R9M_FCC_POWER_VALUES        "10 mW\0" "100 mW" "500 mW" "1 W\0"
#define TR_R9M_LBT_POWER_VALUES        "25 mW\0" "500 mW"

enum R9MFCCPowerValues {
  R9M_FCC_POWER_10 = 0,
  R9M_FCC_POWER_100,
  R9M_FCC_POWER_500,
  R9M_FCC_POWER_1000,
  R9M_FCC_POWER_MAX = R9M_FCC_POWER_1000
};

enum R9MLBTPowerValues {
  R9M_LBT_POWER_25 = 0,
  R9M_LBT_POWER_500,
  R9M_LBT_POWER_MAX = R9M_LBT_POWER_500
};

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
  STATE_DEFAULT = 17,
};

#define BIND_TELEM_ALLOWED(idx)      (!isModuleR9M_LBT(idx) || g_model.moduleData[idx].pxx.power == R9M_LBT_POWER_25)
#define BIND_CH9TO16_ALLOWED(idx)    (!isModuleR9M_LBT(idx) || g_model.moduleData[idx].pxx.power != R9M_LBT_POWER_25)

#endif // _PULSES_ARM_H_
