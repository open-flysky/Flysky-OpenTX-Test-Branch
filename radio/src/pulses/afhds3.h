#ifndef PULSES_AFHDS3_H_
#define PULSES_AFHDS3_H_

#include "pulses_arm.h"
#include "afhds2.h"
#include <functional>
#include <map>
#include <list>

namespace afhds3 {

enum DeviceAddress {
  TRANSMITTER = 0x01,
  MODULE = 0x03,
};

enum FRAME_TYPE {
  REQUEST_GET_DATA                  = 0x01,  //Get data response: ACK + DATA
  REQUEST_SET_EXPECT_DATA           = 0x02,  //Set data response: ACK + DATA
  REQUEST_SET_EXPECT_ACK            = 0x03,  //Set data response: ACK
  REQUEST_SET_NO_RESP               = 0x05,  //Set data response: none
  RESPONSE_DATA                     = 0x10,  //Response ACK + DATA
  RESPONSE_ACK                      = 0x20,  //Response ACK
  NOT_USED                          = 0xff
};

enum COMMAND {
  MODULE_READY                      = 0x01,
  MODULE_STATE                      = 0x02,
  MODULE_MODE                       = 0x03,
  MODULE_SET_CONFIG                 = 0x04,
  MODULE_GET_CONFIG                 = 0x06,
  CHANNELS_FAILSAFE_DATA            = 0x07,
  TELEMETRY_DATA                    = 0x09,
  SEND_COMMAND                      = 0x0C,
  COMMAND_RESULT                    = 0x0D,
  MODULE_POWER_STATUS               = 0x0F,
  MODULE_VERSION                    = 0x1F,
};

enum COMMAND_DIRECTION {
  RADIO_TO_MODULE = 0,
  MODULE_TO_RADIO = 1
};

enum DATA_TYPE {
  READY_DT,  // 8 bytes 0x01 Not ready 0x02 Ready
  STATE_DT,  // See MODULE_STATE
  MODE_DT,
  MOD_CONFIG_DT,
  CHANNELS_DT,
  TELEMETRY_DT,
  MODULE_POWER_DT,
  MODULE_VERSION_DT,
  EMPTY_DT,
};

enum MODULE_STATE {
  STATE_HW_ERROR = 0x01,
  STATE_BINDING = 0x02,
  STATE_SYNC_RUNNING = 0x03,
  STATE_SYNC_DONE = 0x04,
  STATE_STANDBY = 0x05,
  STATE_UPDATING_WAIT = 0x06,
  STATE_UPDATING_MOD = 0x07,
  STATE_UPDATING_RX = 0x08,
  STATE_UPDATING_RX_FAILED = 0x09,
  STATE_RF_TESTING = 0x0a,
  STATE_HW_TEST = 0xff,
};

enum MODULE_MODE {
  STANDBY   = 0x01,
  BIND      = 0x02,  //after bind module will enter run mode
  RUN       = 0x03,
  RX_UPDATE = 0x04,  //after successful update module will enter standby mode, otherwise hw error will be rised
};


#define MIN_FREQ 50
#define MAX_FREQ 400
#define MAX_CHANNELS 18
#define FAILSAFE_KEEP_LAST 0x8000
#define FAILSAFE_MIN -1500
#define FAILSAFE_MAX -1500

enum BIND_POWER {
  MIN_16bBm = 0x00,
  MIN_5bBm = 0x01,
  MIN_0dbm = 0x02,
  PLUS_5dBm = 0x03,
  PLUS_14dBm = 0x04
};

enum RUN_POWER {
  PLUS_15bBm = 0x00,
  PLUS_20bBm = 0x01,
  PLUS_27dbm = 0x02,
  PLUS_30dBm = 0x03,
  PLUS_33dBm = 0x04
};

enum EMI_STANDARD {
  FCC = 0x00,
  CE = 0x01
};

enum DIRECTION {
  ONE_WAY = 0x00,
  TWO_WAYS = 0x01
};

enum PULSE_MODE {
  PWM = 0x00,
  PPM = 0x01,
};

enum SERIAL_MODE {
  IBUS = 0x01,
  SBUS = 0x02
};

struct Config {
  uint8_t bindPower;
  uint8_t runPower;
  uint8_t emiStandard;
  uint8_t direction;
  uint16_t pwmFreq;
  uint8_t pulseMode;
  uint8_t serialMode;
  uint8_t channelCount;
  uint16_t failSafeTimout;
  int16_t failSafeMode;
};

union Config_u {
  Config config;
  uint8_t buffer[sizeof(Config)];
};

enum CHANNELS_DATA_MODE {
  CHANNELS = 0x01,
  FAIL_SAFE = 0x02,
};

struct ChannelsData {
  uint8_t mode;
  uint8_t channelsNumber;
  int16_t data[MAX_CHANNELS];
};

union ChannelsData_u {
  ChannelsData data;
  uint8_t buffer[sizeof(ChannelsData)];
};

struct TelemetryData {
  uint8_t sensorType;
  uint8_t length;
  uint8_t type;
  uint8_t semsorID;
  uint8_t data[8];
};

enum MODULE_POWER_SOURCE {
  INTERNAL = 0x01,
  EXTERNAL = 0x02,
};

struct ModuleVersion {
  uint32_t productNumber;
  uint32_t hardwereVersion;
  uint32_t bootloaderVersion;
  uint32_t firmwareVersion;
  uint32_t rfVersion;
};

struct CmdDesc {
  COMMAND     Command;
  FRAME_TYPE  RequestType;
  DATA_TYPE   RequestDataType;
  FRAME_TYPE  ResponseType;
  DATA_TYPE   ResponseDataType;
};

std::list<CmdDesc> Commands = {
    {   //4.1
        COMMAND::MODULE_READY,
        FRAME_TYPE::REQUEST_GET_DATA,
        DATA_TYPE::EMPTY_DT,
        FRAME_TYPE::RESPONSE_DATA,
        DATA_TYPE::READY_DT,
    },
    {   //4.2
        COMMAND::MODULE_STATE,
        FRAME_TYPE::REQUEST_GET_DATA,
        DATA_TYPE::EMPTY_DT,
        FRAME_TYPE::RESPONSE_DATA,
        DATA_TYPE::STATE_DT,
    },
    {   //4.2
        COMMAND::MODULE_STATE,
        FRAME_TYPE::REQUEST_SET_EXPECT_ACK,
        DATA_TYPE::STATE_DT,
        FRAME_TYPE::RESPONSE_DATA,
        DATA_TYPE::EMPTY_DT,
    },
    {   //4.3
        COMMAND::MODULE_MODE,
        FRAME_TYPE::REQUEST_SET_EXPECT_DATA,
        DATA_TYPE::MODE_DT,
        FRAME_TYPE::RESPONSE_DATA,
        DATA_TYPE::READY_DT,
    },
    {   //4.4
        COMMAND::MODULE_SET_CONFIG,
        FRAME_TYPE::REQUEST_SET_EXPECT_DATA,
        DATA_TYPE::MOD_CONFIG_DT,
        FRAME_TYPE::RESPONSE_DATA,
        DATA_TYPE::READY_DT,
    },
    {   //4.5
        COMMAND::MODULE_GET_CONFIG,
        FRAME_TYPE::REQUEST_GET_DATA,
        DATA_TYPE::EMPTY_DT,
        FRAME_TYPE::RESPONSE_DATA,
        DATA_TYPE::MOD_CONFIG_DT,
    },
    {   //4.6
        COMMAND::CHANNELS_FAILSAFE_DATA,
        FRAME_TYPE::REQUEST_SET_NO_RESP,
        DATA_TYPE::CHANNELS_DT,
        FRAME_TYPE::NOT_USED,
        DATA_TYPE::EMPTY_DT,
    },
    {   //4.7
        COMMAND::TELEMETRY_DATA,
        FRAME_TYPE::REQUEST_SET_NO_RESP,
        DATA_TYPE::TELEMETRY_DT,
        FRAME_TYPE::NOT_USED,
        DATA_TYPE::EMPTY_DT,
    },
    {   //4.9
        COMMAND::MODULE_POWER_STATUS,
        FRAME_TYPE::REQUEST_GET_DATA,
        DATA_TYPE::EMPTY_DT,
        FRAME_TYPE::RESPONSE_DATA,
        DATA_TYPE::MODULE_POWER_DT,
    },
    {   //4.10
        COMMAND::MODULE_VERSION,
        FRAME_TYPE::REQUEST_GET_DATA,
        DATA_TYPE::EMPTY_DT,
        FRAME_TYPE::RESPONSE_DATA,
        DATA_TYPE::MODULE_POWER_DT,
    },
};
class afhds3 {
public:
  afhds3(FlySkySerialPulsesData* data);
  virtual ~afhds3();
  const uint32_t baudrate = 1500;
  const uint16_t parity = 0;
  const uint16_t stopBits;
  const uint16_t wordLength;

private:
  FlySkySerialPulsesData data;
};

}
#endif /* PULSES_AFHDS3_H_ */
