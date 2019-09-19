/*
 * Copyright (C) OpenTX
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

/*
 *  TXID + RXID are already skipped in MULTI module to save memory+transmission time, format from Multi is:
 *  AA or AC | TX_RSSI | sensor ...
 */


#define FLYSKY_TELEMETRY_LENGTH (2+7*4)	// Should it be 2+7*6???

struct FlySkySensor {
  const uint16_t id;
  const char *name;
  const TelemetryUnit unit;
  const uint8_t precision;
};

// telemetry sensors ID
enum{
    AFHDS2A_ID_VOLTAGE        = 0x00,    // Internal Voltage
    AFHDS2A_ID_TEMPERATURE    = 0x01,    // Temperature
    AFHDS2A_ID_MOT            = 0x02,    // RPM
    AFHDS2A_ID_EXTV           = 0x03,    // External Voltage
    AFHDS2A_ID_CELL_VOLTAGE   = 0x04,    // Avg Cell voltage
    AFHDS2A_ID_BAT_CURR       = 0x05,    // battery current A * 100
    AFHDS2A_ID_FUEL           = 0x06,    // remaining battery percentage / mah drawn otherwise or fuel level no unit!
    AFHDS2A_ID_RPM            = 0x07,    // throttle value / battery capacity
    AFHDS2A_ID_CMP_HEAD       = 0x08,    // Heading  0..360 deg, 0=north 2bytes
    AFHDS2A_ID_CLIMB_RATE     = 0x09,    // 2 bytes m/s *100
    AFHDS2A_ID_COG            = 0x0A,    // 2 bytes  Course over ground(NOT heading, but direction of movement) in degrees * 100, 0.0..359.99 degrees. unknown max uint
    AFHDS2A_ID_GPS_STATUS     = 0x0B,    // 2 bytes
    AFHDS2A_ID_ACC_X          = 0x0C,    // 2 bytes m/s *100 signed
    AFHDS2A_ID_ACC_Y          = 0x0D,    // 2 bytes m/s *100 signed
    AFHDS2A_ID_ACC_Z          = 0x0E,    // 2 bytes m/s *100 signed
    AFHDS2A_ID_ROLL           = 0x0F,    // 2 bytes deg *100 signed
    AFHDS2A_ID_PITCH          = 0x10,    // 2 bytes deg *100 signed
    AFHDS2A_ID_YAW            = 0x11,    // 2 bytes deg *100 signed
    AFHDS2A_ID_VERTICAL_SPEED = 0x12,    // 2 bytes m/s *100
    AFHDS2A_ID_GROUND_SPEED   = 0x13,    // 2 bytes m/s *100 different unit than build-in sensor
    AFHDS2A_ID_GPS_DIST       = 0x14,    // 2 bytes dist from home m unsigned
    AFHDS2A_ID_ARMED          = 0x15,    // 2 bytes
    AFHDS2A_ID_FLIGHT_MODE    = 0x16,    // 2 bytes simple index listed below

    AFHDS2A_ID_PRES           = 0x41,    // Pressure
    AFHDS2A_ID_ODO1           = 0x7C,    // Odometer1
    AFHDS2A_ID_ODO2           = 0x7D,    // Odometer2
    AFHDS2A_ID_SPE            = 0x7E,    // Speed            //2byte km/h
    AFHDS2A_ID_TX_V           = 0x7F,    // TX Voltage

    AFHDS2A_ID_GPS_LAT        = 0x80,    // 4bytes signed WGS84 in degrees * 1E7
    AFHDS2A_ID_GPS_LON        = 0x81,    // 4bytes signed WGS84 in degrees * 1E7
    AFHDS2A_ID_GPS_ALT        = 0x82,    // 4bytes signed!!! GPS alt m*100
    AFHDS2A_ID_ALT            = 0x83,    // 4bytes signed!!! Alt m*100
    AFHDS2A_ID_S84            = 0x84,
    AFHDS2A_ID_S85            = 0x85,
    AFHDS2A_ID_S86            = 0x86,
    AFHDS2A_ID_S87            = 0x87,
    AFHDS2A_ID_S88            = 0x88,
    AFHDS2A_ID_S89            = 0x89,
    AFHDS2A_ID_S8a            = 0x8A,

    AFHDS2A_ID_ALT_FLYSKY     = 0xF9,    // Altitude         //2 bytes signed in m
    AFHDS2A_ID_RX_SNR         = 0xFA,    // SNR
    AFHDS2A_ID_RX_NOISE       = 0xFB,    // Noise
    AFHDS2A_ID_RX_RSSI        = 0xFC,    // RSSI
    AFHDS2A_ID_RX_ERR_RATE    = 0xFE,    // Error rate
    AFHDS2A_ID_END            = 0xFF,

// AC type telemetry with multiple values in one packet
    AFHDS2A_ID_GPS_FULL       = 0xFD,
    AFHDS2A_ID_VOLT_FULL      = 0xF0,
    AFHDS2A_ID_ACC_FULL       = 0xEF,
	TX_RSSI_ID                = 0x200,     // Pseudo id outside 1 byte range of FlySky sensors
};

const FlySkySensor flySkySensors[] = {
  {AFHDS2A_ID_VOLTAGE|0x100,    ZSTR_A1,                UNIT_VOLTS,                  2},  // RX Voltage (remapped, really 0x0)
  {AFHDS2A_ID_TEMPERATURE,      ZSTR_TEMP1,             UNIT_CELSIUS,                1},  // Temperature
  {AFHDS2A_ID_MOT,              ZSTR_RPM,               UNIT_RAW,                    0},  // RPM
  {AFHDS2A_ID_EXTV,             ZSTR_A3,                UNIT_VOLTS,                  2},  // External voltage
  {AFHDS2A_ID_CELL_VOLTAGE,     ZSTR_CELLS,             UNIT_VOLTS,                  2},  // Avg Cell voltage
  {AFHDS2A_ID_BAT_CURR,         ZSTR_CURR,              UNIT_AMPS,                   2},  // battery current A * 100
  {AFHDS2A_ID_FUEL,             ZSTR_CAPACITY,          UNIT_RAW,                    2},  // remaining battery percentage / mah drawn otherwise or fuel level no unit!
  {AFHDS2A_ID_RPM,              ZSTR_RPM,               UNIT_RPMS,                   2},  // throttle value / battery capacity
  {AFHDS2A_ID_CMP_HEAD,         ZSTR_HDG,               UNIT_DEGREE,                 0},  // Heading  0..360 deg, 0=north 2bytes
  {AFHDS2A_ID_CLIMB_RATE,       ZSTR_VSPD,              UNIT_METERS_PER_SECOND,      2},  // 2 bytes m/s *100
  {AFHDS2A_ID_COG,              ZSTR_HDG,               UNIT_DEGREE,                 2},  // 2 bytes  Course over ground(NOT heading, but direction of movement) in degrees * 100, 0.0..359.99 degrees. unknown max uint
  {AFHDS2A_ID_GPS_STATUS,       ZSTR_SATELLITES,        UNIT_RAW,                    0},  // 2 bytes
  {AFHDS2A_ID_ACC_X,            ZSTR_ACCX,              UNIT_METERS_PER_SECOND,      2},  // 2 bytes m/s *100 signed
  {AFHDS2A_ID_ACC_Y,            ZSTR_ACCY,              UNIT_METERS_PER_SECOND,      2},  // 2 bytes m/s *100 signed
  {AFHDS2A_ID_ACC_Z,            ZSTR_ACCZ,              UNIT_METERS_PER_SECOND,      2},  // 2 bytes m/s *100 signed
  {AFHDS2A_ID_ROLL,             ZSTR_ROLL,              UNIT_DEGREE,                 2},  // 2 bytes deg *100 signed
  {AFHDS2A_ID_PITCH,            ZSTR_PITCH,             UNIT_DEGREE,                 2},  // 2 bytes deg *100 signed
  {AFHDS2A_ID_YAW,              ZSTR_YAW,               UNIT_DEGREE,                 2},  // 2 bytes deg *100 signed
  {AFHDS2A_ID_VERTICAL_SPEED,   ZSTR_VSPD,              UNIT_METERS_PER_SECOND,      2},  // 2 bytes m/s *100
  {AFHDS2A_ID_GROUND_SPEED,     ZSTR_GSPD,              UNIT_METERS_PER_SECOND,      2},  // 2 bytes m/s *100 different unit than build-in sensor
  {AFHDS2A_ID_GPS_DIST,         ZSTR_GPS,               UNIT_METERS,                 0},  // 2 bytes dist from home m unsigned
  {AFHDS2A_ID_ARMED,            ZSTR_FLIGHT_MODE,       UNIT_RAW,                    0},  // 2 bytes
  {AFHDS2A_ID_FLIGHT_MODE,      ZSTR_FLIGHT_MODE,       UNIT_RAW,                    0},  // 2 bytes index
  {AFHDS2A_ID_PRES,             ZSTR_ALT,               UNIT_METERS,                 2},  // In fact Temperature + Pressure -> Altitude
  {AFHDS2A_ID_PRES|0x100,       ZSTR_TEMP2,             UNIT_CELSIUS,                1},  // Temperature
  {AFHDS2A_ID_ODO1,             ZSTR_ODO1,              UNIT_METERS,                 2},  // Odometer1
  {AFHDS2A_ID_ODO2,             ZSTR_ODO2,              UNIT_METERS,                 2},  // Odometer2
  {AFHDS2A_ID_SPE,              ZSTR_ASPD,              UNIT_KMH,                    2},  // Speed            //2byte km/h
  {AFHDS2A_ID_TX_V,             ZSTR_TXV,               UNIT_VOLTS,                  2},  // TX Voltage
  {AFHDS2A_ID_GPS_LAT,          ZSTR_GPS,               UNIT_GPS_LATITUDE,           2},  // 4bytes signed WGS84 in degrees * 1E7
  {AFHDS2A_ID_GPS_LON,          ZSTR_GPS,               UNIT_GPS_LONGITUDE,          2},  // 4bytes signed WGS84 in degrees * 1E7
  {AFHDS2A_ID_GPS_ALT,          ZSTR_GPS,               UNIT_METERS,                 2},  // 4bytes signed!!! GPS alt m*100
  {AFHDS2A_ID_ALT,              ZSTR_ALT,               UNIT_METERS,                 2},  // 4bytes signed!!! Alt m*100
//{AFHDS2A_ID_S84,              ZSTR_,                  UNIT_RAW,                    0},
//{AFHDS2A_ID_S85,              ZSTR_,                  UNIT_RAW,                    0},
//{AFHDS2A_ID_S86,              ZSTR_,                  UNIT_RAW,                    0},
//{AFHDS2A_ID_S87,              ZSTR_,                  UNIT_RAW,                    0},
//{AFHDS2A_ID_S88,              ZSTR_,                  UNIT_RAW,                    0},
//{AFHDS2A_ID_S89,              ZSTR_,                  UNIT_RAW,                    0},
//{AFHDS2A_ID_S8a,              ZSTR_,                  UNIT_RAW,                    0},
//{AFHDS2A_ID_GPS_FULL,         ZSTR_GPS,               UNIT_RAW,                    0},
//{AFHDS2A_ID_VOLT_FULL,        ZSTR_,                  UNIT_RAW,                    0},
//{AFHDS2A_ID_ACC_FULL,         ZSTR_,                  UNIT_RAW,                    0},
  {AFHDS2A_ID_ALT_FLYSKY,       ZSTR_ALT,               UNIT_METERS,                 0},  // 2 bytes signed in m

// Com
  {AFHDS2A_ID_RX_SNR,           ZSTR_RX_SNR,            UNIT_DB,                     0},  // RX SNR
  {AFHDS2A_ID_RX_NOISE,         ZSTR_RX_NOISE,          UNIT_DB,                     0},  // RX Noise
  {AFHDS2A_ID_RX_RSSI,          ZSTR_RSSI,              UNIT_DB,                     0},  // RX RSSI (0xfc)
  {AFHDS2A_ID_RX_ERR_RATE,      ZSTR_RX_QUALITY,        UNIT_RAW,                    0},  // RX error rate
  {TX_RSSI_ID,                  ZSTR_TX_RSSI,           UNIT_RAW,                    0},  // Pseudo sensor for TRSSI

  {0x00,                        NULL,                   UNIT_RAW,                    0},  // sentinel
};

int32_t getALT(uint32_t value);

static void processFlySkySensor(const uint8_t *packet, uint8_t type)
{
  uint16_t id = packet[0];
  const uint8_t instance = packet[1];
  int32_t value;

  //Load most likely value
  if(type==0xAA)
	  value = (packet[3] << 8) | packet[2];
  else
	  value = (packet[6] << 24) | (packet[5] << 16) | (packet[4] << 8) | packet[3];
  
  if (id == 0) id = 0x100;   // Some part of OpenTX does not like sensor with id and instance 0, remap to 0x100

  switch(id)
  {
    case AFHDS2A_ID_RX_NOISE:
    case AFHDS2A_ID_RX_RSSI:
      value  = - value;
      break;
    case AFHDS2A_ID_RX_ERR_RATE:
      value = 100 - value;
      telemetryData.rssi.set(value);
      if(value>0) telemetryStreaming = TELEMETRY_TIMEOUT10ms;
      break;
    case AFHDS2A_ID_PRES:
      setTelemetryValue(PROTOCOL_TELEMETRY_FLYSKY_IBUS, id|0x100, 0, instance, ((value >> 19) - 400), UNIT_CELSIUS, 1); // Extract temperature to a new sensor
      value = getALT(value);
	  break;
    case AFHDS2A_ID_ACC_X:
    case AFHDS2A_ID_ACC_Y:
    case AFHDS2A_ID_ACC_Z:
    case AFHDS2A_ID_ROLL:
    case AFHDS2A_ID_PITCH:
    case AFHDS2A_ID_YAW:
	case AFHDS2A_ID_ALT_FLYSKY:
	  value=(int16_t)value;	// Signed value
	  break;
  }
  for (const FlySkySensor * sensor = flySkySensors; sensor->id; sensor++)
  {
    if (sensor->id == id) {
      if (sensor->unit == UNIT_CELSIUS)
        value -= 400; // Temperature sensors have 40 degree offset
      else if (sensor->unit == UNIT_VOLTS)
        value = (int16_t)value; // Voltage types are signed 16bit integers
      setTelemetryValue(PROTOCOL_TELEMETRY_FLYSKY_IBUS, id, 0, instance, value, sensor->unit, sensor->precision);
      return;
    }
  }
  //unknown
  setTelemetryValue(PROTOCOL_TELEMETRY_FLYSKY_IBUS, id, 0, instance, value, UNIT_RAW, 0);
}

void processFlySkyPacket(const uint8_t *packet)
{
  // Set TX RSSI Value, reverse MULTIs scaling
  setTelemetryValue(PROTOCOL_TELEMETRY_FLYSKY_IBUS, TX_RSSI_ID, 0, 0, packet[0], UNIT_RAW, 0);

  uint8_t index=1;
  while(packet[index] != AFHDS2A_ID_END && (index + 4) <= 29)
  {
    processFlySkySensor(packet+index, 0xAA);
	index += 4;
  }
}

void processFlySkyPacketAC(const uint8_t *packet)
{
  // Set TX RSSI Value, reverse MULTIs scaling
  setTelemetryValue(PROTOCOL_TELEMETRY_FLYSKY_IBUS, TX_RSSI_ID, 0, 0, packet[0], UNIT_RAW, 0);

  uint8_t index=1;
  while(packet[index] != AFHDS2A_ID_END && (index+packet[index+2]+3) <= 29)
  {
    processFlySkySensor(packet+index, 0xAC);
	index+=packet[index+2]+3;
  }
}

void processFlySkyTelemetryData(uint8_t data, uint8_t* rxBuffer, uint8_t& rxBufferCount)
{
  if (rxBufferCount == 0 && (data != 0xAA || data != 0xAC)) {
    TRACE("[IBUS] invalid start byte 0x%02X", data);
    return;
  }

  if (rxBufferCount < TELEMETRY_RX_PACKET_SIZE) {
    rxBuffer[rxBufferCount++] = data;
  }
  else {
    TRACE("[IBUS] array size %d error", rxBufferCount);
    rxBufferCount = 0;
  }


  if (rxBufferCount >= FLYSKY_TELEMETRY_LENGTH) {
    // debug print the content of the packets
#if 0
    debugPrintf("[IBUS] Packet 0x%02X rssi 0x%02X: ",
                rxBuffer[0], rxBuffer[1]);
    for (int i=0; i<7; i++) {
      debugPrintf("[%02X %02X %02X%02X] ", rxBuffer[i*4+2], rxBuffer[i*4 + 3],
                  rxBuffer[i*4 + 4], rxBuffer[i*4 + 5]);
    }
    debugPrintf("\r\n");
#endif
    if( data == 0xAA )
		processFlySkyPacket(rxBuffer+1);
	else
		processFlySkyPacketAC(rxBuffer+1);
    rxBufferCount = 0;
  }
}

const FlySkySensor *getFlySkySensor(uint16_t id)
{
  for (const FlySkySensor * sensor = flySkySensors; sensor->id; sensor++) {
    if (id == sensor->id)
      return sensor;
  }
  return nullptr;
}

void flySkySetDefault(int index, uint16_t id, uint8_t subId, uint8_t instance)
{
  TelemetrySensor &telemetrySensor = g_model.telemetrySensors[index];
  telemetrySensor.id = id;
  telemetrySensor.subId = subId;
  telemetrySensor.instance = instance;

  const FlySkySensor *sensor = getFlySkySensor(id);
  if (sensor) {
    TelemetryUnit unit = sensor->unit;
    uint8_t prec = min<uint8_t>(2, sensor->precision);
    telemetrySensor.init(sensor->name, unit, prec);

    if (unit == UNIT_RPMS) {
      telemetrySensor.custom.ratio = 1;
      telemetrySensor.custom.offset = 1;
    }
  }
  else {
    telemetrySensor.init(id);
  }

  storageDirty(EE_MODEL);
}

#define ALT_PRECISION 15
#define R_DIV_G_MUL_10_Q15 UINT64_C(9591506)
#define INV_LOG2_E_Q1DOT31 UINT64_C(0x58b90bfc) // Inverse log base 2 of e
#define ALT_MASK 0x7FFFF

uint16_t ibusTempToK(int16_t tempertureIbus){
	return (uint16_t)(tempertureIbus - 400) + 2731;
}

int32_t log2fix(uint32_t x){
	int32_t b = 1U << (ALT_PRECISION - 1);
	int32_t y = 0;
	while (x < 1U << ALT_PRECISION) {
			x <<= 1;
			y -= 1U << ALT_PRECISION;
	}

	while (x >= 2U << ALT_PRECISION) {
			x >>= 1;
			y += 1U << ALT_PRECISION;
	}

	uint64_t z = x;
	for (size_t i = 0; i < ALT_PRECISION; i++) {
			z = (z * z) >> ALT_PRECISION;
			if (z >= 2U << ALT_PRECISION) {
					z >>= 1;
					y += b;
			}
			b >>= 1;
	}
	return y;
}

int32_t getALT(uint32_t value){
	uint32_t pressurePa = value & ALT_MASK;
	if(pressurePa == 0) return 0;
    uint16_t temperatureK = ibusTempToK((uint16_t)(value >> 19));
    static uint32_t initPressure = 0;
	static uint16_t initTemperature = 0;
	if (initPressure <= 0) // use current pressure for ground altitude -> 0
	{
			initPressure = pressurePa;
			initTemperature = temperatureK;
    }
    int temperature = (initTemperature + temperatureK) >> 1; //div 2
    bool tempNegative = temperature < 0;
    if (tempNegative)  temperature = temperature *-1;
    uint64_t helper = R_DIV_G_MUL_10_Q15;
    helper = helper*(uint64_t)temperature;
    helper = helper >> ALT_PRECISION;

	uint32_t po_to_p = (uint32_t)(initPressure << (ALT_PRECISION-1));
	po_to_p = po_to_p/pressurePa;
	//shift missing bit
	po_to_p = po_to_p << 1;
	if(po_to_p == 0) return 0;
	uint64_t t =  log2fix(po_to_p)*INV_LOG2_E_Q1DOT31;
	int32_t ln = t >> 31;
	
    bool neg = ln < 0;
    if (neg) ln = ln * -1;
    helper = helper*(uint64_t)ln;
    helper = helper >> ALT_PRECISION;
    int result = (int)helper;

    if (neg ^ tempNegative) result = result * -1;
    return result;
}
