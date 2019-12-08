#ifndef PULSES_AFHDS3_H_
#define PULSES_AFHDS3_H_

#include "afhds2.h"
#include "../moduledata.h"

#define AFHDS3_BAUDRATE 1500000
namespace afhds3 {

typedef void (*bindCallback_t) (bool);

enum DeviceAddress {
  TRANSMITTER = 0x01, MODULE = 0x03,
};

enum FRAME_TYPE {
  REQUEST_GET_DATA = 0x01,  //Get data response: ACK + DATA
  REQUEST_SET_EXPECT_DATA = 0x02,  //Set data response: ACK + DATA
  REQUEST_SET_EXPECT_ACK = 0x03,  //Set data response: ACK
  REQUEST_SET_NO_RESP = 0x05,  //Set data response: none
  RESPONSE_DATA = 0x10,  //Response ACK + DATA
  RESPONSE_ACK = 0x20,  //Response ACK
  NOT_USED = 0xff
};

enum COMMAND {
  MODULE_READY = 0x01,
  MODULE_STATE = 0x02,
  MODULE_MODE = 0x03,
  MODULE_SET_CONFIG = 0x04,
  MODULE_GET_CONFIG = 0x06,
  CHANNELS_FAILSAFE_DATA = 0x07,
  TELEMETRY_DATA = 0x09,
  SEND_COMMAND = 0x0C,
  COMMAND_RESULT = 0x0D,
  MODULE_POWER_STATUS = 0x0F,
  MODULE_VERSION = 0x1F,
};

enum COMMAND_DIRECTION {
  RADIO_TO_MODULE = 0, MODULE_TO_RADIO = 1
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
enum MODULE_READY_E {
  MR_UNKNOWN = 0x00,
  MR_NOT_READY = 0x01,
  MR_READY = 0x02
};

enum ModuleState {
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
  STATE_NOT_READY = 0xe0, //virtual
  STATE_READY = 0xe1, //virtual
  STATE_LOAD_INFO = 0xe2, //virtual
  STATE_LOAD_INFO2 = 0xe3, //virtual
  STATE_REQUEST = 0xe4, //virtual
  STATE_BIND = 0xe5, //virtual
  STATE_SET_BIND_CONFIG = 0xe6, //virtual
  STATE_HW_TEST = 0xff,
};

//used for set command
enum MODULE_MODE_E {
  STANDBY = 0x01,
  BIND = 0x02,  //after bind module will enter run mode
  RUN = 0x03,
  RX_UPDATE = 0x04, //after successful update module will enter standby mode, otherwise hw error will be raised
};

enum CMD_RESULT {
  FAILURE = 0x01,
  SUCCESS = 0x02,
};


#define MIN_FREQ 50
#define MAX_FREQ 400
#define MAX_CHANNELS 18
#define FAILSAFE_KEEP_LAST 0x8000
#define FAILSAFE_MIN -15000
#define FAILSAFE_MAX 15000

enum BIND_POWER {
  MIN_16dBm = 0x00,
  MIN_5dBm = 0x01,
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
  FCC = 0x00, CE = 0x01
};

enum DIRECTION {
  ONE_WAY = 0x00, TWO_WAYS = 0x01
};

enum PULSE_MODE {
  PWM = 0x00, PPM = 0x01,
};

enum SERIAL_MODE {
  IBUS = 0x01, SBUS = 0x02
};

struct Config_s {
  uint8_t bindPower;
  uint8_t runPower;
  uint8_t emiStandard;
  uint8_t direction;
  uint16_t pwmFreq;
  uint8_t pulseMode;
  uint8_t serialMode;
  uint8_t channelCount;
  uint16_t failSafeTimout;
  int16_t failSafeMode[MAX_CHANNELS];
};

union Config_u {
  Config_s config;
  uint8_t buffer[sizeof(Config_s)];
};

enum CHANNELS_DATA_MODE {
  CHANNELS = 0x01, FAIL_SAFE = 0x02,
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
  INTERNAL = 0x01, EXTERNAL = 0x02,
};

struct ModuleVersion {
  uint32_t productNumber;
  uint32_t hardwereVersion;
  uint32_t bootloaderVersion;
  uint32_t firmwareVersion;
  uint32_t rfVersion;
};


/*
template <typename T>
struct xfire_value {
  T value;
  T minVal;
  T maxVal;
  T defVal;
};
*/

union AfhdsFrameData {
  uint8_t Result;
  Config_s Config;
  ChannelsData Channels;
  TelemetryData Telemetry;
  ModuleVersion Version;
};

struct AfhdsFrame {
  uint8_t startByte;
  uint8_t address;
  uint8_t frameNumber;
  uint8_t frameType;
  uint8_t command;
  AfhdsFrameData data;
};



struct CmdDesc {
  COMMAND Command;
  FRAME_TYPE RequestType;
  DATA_TYPE RequestDataType;
  FRAME_TYPE ResponseType;
  DATA_TYPE ResponseDataType;
};

enum State {
  UNKNOWN = 0,
  SENDING_COMMAND,
  AWAITING_RESPONSE,
  IDLE
};

class afhds3 {
public:
  afhds3(FlySkySerialPulsesData* data, ModuleData* moduleData, int16_t* channelOutputs) {
    this->data = data;
    this->moduleData = moduleData;
    this->channelOutputs = channelOutputs;
    reset();
  }

  virtual ~afhds3() {

  }
  const uint32_t baudrate = AFHDS3_BAUDRATE;
  const uint16_t parity = ((uint16_t)0x0000); //USART_Parity_No
  const uint16_t stopBits = ((uint16_t)0x0000); //USART_StopBits_1
  const uint16_t wordLength = ((uint16_t)0x0000); //USART_WordLength_8b
  const uint16_t commandTimout = 5; //ms
  const uint16_t commandRepeatCount = 5;

  const uint8_t FrameAddress = DeviceAddress::TRANSMITTER | (DeviceAddress::MODULE << 4);

  //Equivalent to setupPulses
  void setupPulses();
  void onDataReceived(uint8_t data, uint8_t* rxBuffer, uint8_t& rxBufferCount, uint8_t maxSize);
  void reset();
  void bind(bindCallback_t callback);
  void cancelBind();
private:
  void putByte(uint8_t byte);
  void putBytes(uint8_t* data, int length);
  void putHeader(COMMAND command, FRAME_TYPE frameType);
  void putFooter();
  void putFrame(COMMAND command, FRAME_TYPE frameType, uint8_t* data, uint8_t dataLength);
  void parseData(uint8_t* rxBuffer, uint8_t rxBufferCount);
  void onModuleSetModeResponse(bool success, uint8_t mode);
  void onModuleStateReponse(uint8_t state);
  void onModelSwitch();
  void setConfigToDefault();
  void setConfigFromModelData();
  void sendChannelsData();

  State operationState;
  uint16_t repeatCount;
  FlySkySerialPulsesData* data;
  ModuleData* moduleData;
  //buffer where the channels are
  int16_t* channelOutputs;
  bindCallback_t bindCallback;
  Config_u config;
  ModuleVersion version;
  enum MODULE_POWER_SOURCE powerSource;
};

}
#endif /* PULSES_AFHDS3_H_ */
