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

#ifndef _BOARD_H_
#define _BOARD_H_

#include "stddef.h"
#include "stdbool.h"

#if defined(__cplusplus) && !defined(SIMU)
extern "C" {
#endif

#if __clang__
// clang is very picky about the use of "register"
// tell it to ignore for the STM32 includes instead of modyfing the orginal files
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#endif

#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_rcc.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_gpio.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_tim.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_adc.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_spi.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_i2c.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_rtc.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_pwr.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_dma.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_usart.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_flash.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_sdio.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_dbgmcu.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_exti.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/stm32f4xx_syscfg.h"
#include "STM32F4xx_DSP_StdPeriph_Lib_V1.4.0/Libraries/STM32F4xx_StdPeriph_Driver/inc/misc.h"

#if __clang__
// Restore warnings about registers
#pragma clang diagnostic pop
#endif

#include "usb_driver.h"
#if !defined(SIMU)
  #include "usbd_cdc_core.h"
  #include "usbd_msc_core.h"
  #include "usbd_hid_core.h"
  #include "usbd_usr.h"
  #include "usbd_desc.h"
  #include "usb_conf.h"
  #include "usbd_conf.h"
#endif

#include "hal.h"

#if defined(__cplusplus) && !defined(SIMU)
}
#endif

#define FLASHSIZE          0x80000
#define BOOTLOADER_SIZE    0xC000
#define FIRMWARE_ADDRESS   0x08000000

#define LUA_MEM_MAX        (0)    // max allowed memory usage for complete Lua  (in bytes), 0 means unlimited

#define PERI1_FREQUENCY    42000000
#define PERI2_FREQUENCY    84000000

#define TIMER_MULT_APB1    2
#define TIMER_MULT_APB2    2

#define strcpy_P strcpy

extern uint16_t sessionTimer;

#define SLAVE_MODE()                   (g_model.trainerMode == TRAINER_MODE_SLAVE)

// TODO
#define TRAINER_CONNECTED()            (true)

// Board driver
void boardInit(void);
void boardOff(void);

// Delays driver
#ifdef __cplusplus
extern "C" {
#endif
void delaysInit(void);
void delay_01us(uint16_t nb);
void delay_us(uint16_t nb);
void delay_ms(uint32_t ms);
#ifdef __cplusplus
}
#endif

// CPU Unique ID
#define LEN_CPU_UID                    (3*8+2)
void getCPUUniqueID(char * s);

// SD driver
#define BLOCK_SIZE                     512 /* Block Size in Bytes */
#if !defined(SIMU) || defined(SIMU_DISKIO)
uint32_t sdIsHC(void);
uint32_t sdGetSpeed(void);
#define SD_IS_HC()                     (sdIsHC())
#define SD_GET_SPEED()                 (sdGetSpeed())
#define SD_GET_FREE_BLOCKNR()          (sdGetFreeSectors())
#define SD_CARD_PRESENT()              true
void sdInit(void);
void sdMount(void);
void sdDone(void);
#define sdPoll10ms()
uint32_t sdMounted(void);
#else
#define SD_IS_HC()                     (0)
#define SD_GET_SPEED()                 (0)
#define sdInit()
#define sdMount()
#define sdDone()
#define SD_CARD_PRESENT()              true
#endif

#if defined(DISK_CACHE)
#include "diskio.h"
DRESULT __disk_read(BYTE drv, BYTE * buff, DWORD sector, UINT count);
DRESULT __disk_write(BYTE drv, const BYTE * buff, DWORD sector, UINT count);
#else
#define __disk_read                    disk_read
#define __disk_write                   disk_write
#endif

// Flash Write driver
#define FLASH_PAGESIZE 256
void flashUnlock(void);
void flashLock(void);
void flashWrite(uint32_t * address, uint32_t * buffer);
uint32_t isFirmwareStart(const uint8_t * buffer);
uint32_t isBootloaderStart(const uint8_t * buffer);

// Pulses driver
#define IS_UART_MODULE(port)      (port == INTERNAL_MODULE)
#define INTERNAL_MODULE_ON()      do {} while (0)
#define INTERNAL_MODULE_OFF()     do {} while (0)
#define EXTERNAL_MODULE_ON()      GPIO_SetBits(EXTMODULE_PWR_GPIO, EXTMODULE_PWR_GPIO_PIN)
#define EXTERNAL_MODULE_OFF()     GPIO_ResetBits(EXTMODULE_PWR_GPIO, EXTMODULE_PWR_GPIO_PIN)
#define IS_INTERNAL_MODULE_ON()   true
#define IS_EXTERNAL_MODULE_ON()   (GPIO_ReadInputDataBit(EXTMODULE_PWR_GPIO, EXTMODULE_PWR_GPIO_PIN) == Bit_SET)
void init_no_pulses(uint32_t port);
void disable_no_pulses(uint32_t port);
void init_ppm( uint32_t module_index );
void disable_ppm( uint32_t module_index );
void init_pxx( uint32_t module_index );
void disable_pxx( uint32_t module_index );
void init_dsm2( uint32_t module_index );
void disable_dsm2( uint32_t module_index );
void init_crossfire( uint32_t module_index );
void disable_crossfire( uint32_t module_index );
void init_sbusOut(uint32_t module_index);
void disable_sbusOut(uint32_t module_index);

// Trainer driver
void init_trainer_ppm(void);
void stop_trainer_ppm(void);
void init_trainer_capture(void);
void stop_trainer_capture(void);
void init_cppm_on_heartbeat_capture(void);
void stop_cppm_on_heartbeat_capture(void);
void init_sbus_on_heartbeat_capture(void);
void stop_sbus_on_heartbeat_capture(void);
int sbusGetByte(uint8_t * byte);

// Gimbals driver
void gimbalsInit(void);
void gimbalsRead(uint16_t * values);

// Enable touch
#if defined(TOUCH_SCREEN)
  #define IS_TOUCH_ENABLED()           (1)  // numeric value so can be used in macros
#else
  #define IS_TOUCH_ENABLED()           (0)  // numeric value so can be used in macros
#endif

// Keys driver
enum EnumKeys
{
  KEY_ENTER,
  KEY_MENU = KEY_ENTER,
  KEY_EXIT,
  KEY_DOWN,
  KEY_MINUS = KEY_DOWN,
  KEY_UP,
  KEY_PLUS = KEY_UP,
  KEY_RIGHT,
  KEY_LEFT,

  TRM_BASE,
  TRM_LH_DWN = TRM_BASE,
  TRM_LH_UP,
  TRM_LV_DWN,
  TRM_LV_UP,
  TRM_RV_DWN,
  TRM_RV_UP,
  TRM_RH_DWN,
  TRM_RH_UP,
  TRM_LAST = TRM_RH_UP,

  NUM_KEYS
};

enum EnumSwitches
{
  SW_SA,
  SW_SB,
  SW_SC,
  SW_SD,
  SW_SE,
  SW_SF
};

#define IS_3POS(x)                     (false)

enum EnumSwitchesPositions
{
  SW_SA0,
  SW_SA1,
  SW_SA2,
  SW_SB0,
  SW_SB1,
  SW_SB2,
  SW_SC0,
  SW_SC1,
  SW_SC2,
  SW_SD0,
  SW_SD1,
  SW_SD2,
  SW_SE0,
  SW_SE1,
  SW_SE2,
  SW_SF0,
  SW_SF1,
  SW_SF2,
};

#define NUM_SWITCHES                 6

void keysInit(void);
uint8_t keyState(uint8_t index);
uint32_t switchState(uint8_t index);
uint32_t readKeys(void);
uint32_t readTrims(void);
#define TRIMS_PRESSED()            (readTrims())
#define KEYS_PRESSED()             (readKeys())

// WDT driver
#define WDTO_500MS                            500
#if defined(WATCHDOG_DISABLED) || defined(SIMU)
  #define wdt_enable(x)
  #define wdt_reset()
#else
  #define wdt_enable(x)                       watchdogInit(x)
  #define wdt_reset()                         IWDG->KR = 0xAAAA
#endif
#define wdt_disable()
void watchdogInit(unsigned int duration);
#define WAS_RESET_BY_SOFTWARE()               (RCC->CSR & RCC_CSR_SFTRSTF)
#define WAS_RESET_BY_WATCHDOG()               (RCC->CSR & (RCC_CSR_WDGRSTF | RCC_CSR_WWDGRSTF))
#define WAS_RESET_BY_WATCHDOG_OR_SOFTWARE()   (RCC->CSR & (RCC_CSR_WDGRSTF | RCC_CSR_WWDGRSTF | RCC_CSR_SFTRSTF))

// ADC driver
enum Analogs {
  STICK1,
  STICK2,
  STICK3,
  STICK4,
  POT_FIRST,
  POT1 = POT_FIRST,
  POT2,
  POT_LAST = POT2,
  SWITCH_SA,
  SWITCH_SB,
  SWITCH_SC,
  SWITCH_SD,
  SWITCH_SE,
  SWITCH_SF,
  TX_VOLTAGE,
  LIBATT_VOLTAGE = TX_VOLTAGE,
  DRYBATT_VOLTAGE,
  NUM_ANALOGS
};

#define NUM_POTS                       2
#define NUM_XPOTS                      NUM_POTS
#define NUM_SLIDERS                    0
#define NUM_PWMANALOGS                 4

enum CalibratedAnalogs {
  CALIBRATED_STICK1,
  CALIBRATED_STICK2,
  CALIBRATED_STICK3,
  CALIBRATED_STICK4,
  CALIBRATED_POT_FIRST,
  CALIBRATED_POT_LAST = CALIBRATED_POT_FIRST + NUM_POTS - 1,
  CALIBRATED_SLIDER_FIRST,
  CALIBRATED_SLIDER_LAST = CALIBRATED_SLIDER_FIRST + NUM_SLIDERS - 1,
  NUM_CALIBRATED_ANALOGS
};

#define IS_POT(x)                      ((x)>=POT_FIRST && (x)<=POT_LAST)
#define IS_SLIDER(x)                   false
void adcInit(void);
void adcRead(void);
extern uint16_t adcValues[NUM_ANALOGS];
uint16_t getAnalogValue(uint8_t index);
uint16_t getBatteryVoltage();   // returns current battery voltage in 10mV steps

#define BATT_SCALE    150

#if defined(__cplusplus) && !defined(SIMU)
extern "C" {
#endif

// Power driver
#define SOFT_PWR_CTRL
void pwrInit(void);
uint32_t pwrCheck(void);
void pwrOn(void);
void pwrOff(void);
uint32_t pwrPressed(void);
#if defined(PWR_PRESS_BUTTON)
uint32_t pwrPressedDuration(void);
#endif

#if defined(SIMU)
#define UNEXPECTED_SHUTDOWN()          false
#else
#define UNEXPECTED_SHUTDOWN()          (WAS_RESET_BY_WATCHDOG() || g_eeGeneral.unexpectedShutdown)
#endif

// Backlight driver
void backlightInit(void);
void backlightDisable(void);
#define BACKLIGHT_DISABLE()            backlightDisable()
uint8_t isBacklightEnabled(void);
void backlightEnable(void);
#define BACKLIGHT_ENABLE()             backlightEnable()

#if !defined(SIMU)
  void usbJoystickUpdate();
#endif
#define USBD_MANUFACTURER_STRING       "FlySky"
#define USB_NAME                       "FlySky I8"
#define USB_MANUFACTURER               'F', 'l', 'y', 'S', 'k', 'y', ' ', ' '  /* 8 bytes */
#define USB_PRODUCT                    'I', '8', ' ', ' ', ' ', ' ', ' ', ' '  /* 8 Bytes */

#if defined(__cplusplus) && !defined(SIMU)
}
#endif

// Debug driver
void debugPutc(const char c);

// Telemetry driver
void telemetryPortInit(uint32_t baudrate, uint8_t mode);
void telemetryPortSetDirectionOutput(void);
void sportSendBuffer(uint8_t * buffer, uint32_t count);
uint8_t telemetryGetByte(uint8_t * byte);
extern uint32_t telemetryErrors;

// Sport update driver
#define SPORT_UPDATE_POWER_ON()        EXTERNAL_MODULE_ON()
#define SPORT_UPDATE_POWER_OFF()       EXTERNAL_MODULE_OFF()

// Audio driver
void audioInit(void) ;
void audioEnd(void) ;
void dacStart(void);
void dacStop(void);
void setSampleRate(uint32_t frequency);
#define VOLUME_LEVEL_MAX  23
#define VOLUME_LEVEL_DEF  12
#if !defined(SOFTWARE_VOLUME)
void setScaledVolume(uint8_t volume);
void setVolume(uint8_t volume);
int32_t getVolume(void);
#endif
void audioConsumeCurrentBuffer();
#define audioDisableIrq()       __disable_irq()
#define audioEnableIrq()        __enable_irq()

// Haptic driver
void hapticInit(void);
void hapticOff(void);
#if defined(PCBX9E) || defined(PCBX9DP)
  void hapticOn(uint32_t pwmPercent);
#else
  void hapticOn(void);
#endif

// Second serial port driver
#define DEBUG_BAUDRATE                 115200
#if !defined(PCBX7)
#define AUX_SERIAL
extern uint8_t auxSerialMode;
void auxSerialInit(unsigned int mode, unsigned int protocol);
void auxSerialPutc(char c);
#define auxSerialTelemetryInit(protocol) auxSerialInit(UART_MODE_TELEMETRY, protocol)
void auxSerialSbusInit(void);
void auxSerialStop(void);
#endif

// BT driver
#define BLUETOOTH_DEFAULT_BAUDRATE     115200
#if defined(PCBX9E) && !defined(USEHORUSBT)
#define BLUETOOTH_FACTORY_BAUDRATE     9600
#else
#define BLUETOOTH_FACTORY_BAUDRATE     57600
#endif
void bluetoothInit(uint32_t baudrate);
void bluetoothWriteWakeup(void);
uint8_t bluetoothIsWriting(void);
void bluetoothDone(void);

// LED driver
void ledInit(void);
void ledOff(void);
void ledRed(void);
void ledGreen(void);
void ledBlue(void);

// LCD driver
#define LCD_W                          128
#define LCD_H                          64
#define LCD_DEPTH                      1
#define IS_LCD_RESET_NEEDED()          true
#define LCD_CONTRAST_MIN               10
#define LCD_CONTRAST_MAX               30
#define LCD_CONTRAST_DEFAULT           20
void lcdInit(void);
void lcdInitFinish(void);
void lcdOff(void);

// TODO lcdRefreshWait() stub in simpgmspace and remove LCD_DUAL_BUFFER
#if !defined(LCD_DUAL_BUFFER) && !defined(SIMU)
void lcdRefreshWait();
#else
#define lcdRefreshWait()
#endif
#if defined(SIMU) || !defined(__cplusplus)
void lcdRefresh(void);
#else
void lcdRefresh(bool wait=true); // TODO uint8_t wait to simplify this
#endif
void lcdSetRefVolt(unsigned char val);
void lcdSetContrast(void);

#define USART_FLAG_ERRORS (USART_FLAG_ORE | USART_FLAG_NE | USART_FLAG_FE | USART_FLAG_PE)

extern uint8_t currentTrainerMode;
void checkTrainerSettings(void);

#if defined(__cplusplus)
#include "fifo.h"
#include "dmafifo.h"

#if defined(CROSSFIRE)
#define TELEMETRY_FIFO_SIZE 128
#else
#define TELEMETRY_FIFO_SIZE 64
#endif

extern Fifo<uint8_t, TELEMETRY_FIFO_SIZE> telemetryFifo;
extern Fifo<uint8_t, 32> auxSerialRxFifo;
#endif

#endif // _BOARD_H_
