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

#define FLYSKY_FIXED_RX_VOLTAGE (uint8_t)(FLYSKY_SENSOR_RX_VOLTAGE + (uint8_t)0xA0)

#define MIN_SNR 8
#define MAX_SNR 45

#define FIXED_PRECISION 15
#define FIXED(val) (val << FIXED_PRECISION)
#define DECIMAL(val) (val >> FIXED_PRECISION)
#define R_DIV_G_MUL_10_Q15 UINT64_C(9591506)
#define INV_LOG2_E_Q1DOT31 UINT64_C(0x58b90bfc) // Inverse log base 2 of e

struct FlyskyNv14Sensor {
  const uint16_t id;
  const uint8_t subId;
  const char * name;
  const TelemetryUnit unit;
  const uint8_t precision;
  const uint8_t offset;
  const uint8_t bytes;
  const bool issigned;
};


union nv14SensorData {
	uint8_t UINT8;
	uint16_t UINT16;
	int16_t INT16;
	uint32_t UINT32;
};



const FlyskyNv14Sensor Nv14Sensor[]=
{
    {FLYSKY_FIXED_RX_VOLTAGE,  0, ZSTR_BATT,        UNIT_VOLTS,         2, 0, 2, false},
    {FLYSKY_SENSOR_RX_SIGNAL,  0, ZSTR_SIG,         UNIT_RAW,           0, 0, 2, false},
    {FLYSKY_SENSOR_RX_RSSI,    0, ZSTR_RSSI,        UNIT_DB,           	0, 0, 2, true,},
    {FLYSKY_SENSOR_RX_NOISE,   0, ZSTR_NOISE,       UNIT_DB,            0, 0, 2, true},
    {FLYSKY_SENSOR_RX_SNR,     0, ZSTR_RX_SNR,      UNIT_DB,            0, 0, 2, false},
    {FLYSKY_SENSOR_RX_SNR,     1, ZSTR_RX_QUALITY,  UNIT_PERCENT,       0, 0, 2, false},
    {FLYSKY_SENSOR_TEMP,       0, ZSTR_TEMP1,       UNIT_CELSIUS,       1, 0, 2, true},
    {FLYSKY_SENSOR_EXT_VOLTAGE,0, ZSTR_EBATT,       UNIT_VOLTS,         2, 0, 2, false},
    {FLYSKY_SENSOR_MOTO_RPM,   0, ZSTR_RPM,         UNIT_RPMS,          0, 0, 2, false},
    {FLYSKY_SENSOR_PRESURRE,   0, ZSTR_AIRPRE,      UNIT_RAW,           1, 0, 2, false},
    {FLYSKY_SENSOR_PRESURRE,   1, ZSTR_ALT,         UNIT_METERS,        0, 0, 2, true},
    {FLYSKY_SENSOR_GPS,        1, ZSTR_SATELLITES,  UNIT_RAW,           0, 0, 1, false},
    {FLYSKY_SENSOR_GPS,        2, ZSTR_GPS,         UNIT_GPS_LATITUDE,  0, 1, 4, true},
    {FLYSKY_SENSOR_GPS,        3, ZSTR_GPS,         UNIT_GPS_LONGITUDE, 0, 5, 4, true},
    {FLYSKY_SENSOR_GPS,        4, ZSTR_ALT,         UNIT_METERS,        0, 8, 2, true},
    {FLYSKY_SENSOR_GPS,        5, ZSTR_GSPD,        UNIT_KMH,           1, 10, 2, false},
    {FLYSKY_SENSOR_GPS,        6, ZSTR_HDG,         UNIT_DEGREE,        3, 12, 2, false},

};

FlyskyNv14Sensor defaultNv14Sensor = {0, 0, "UNKNOWN", UNIT_RAW, 0, 0, 2, false};


const uint8_t snrToSig[] = {
    4, 15, 24, 33, 40, 47, 53, 58, 63, 67, 71, 74, 77, 80, 82, 84, 86, 88, 89,
	90, 92, 93, 94, 94, 95, 96, 96, 97, 98, 98, 98, 99, 99, 99, 99, 100, 100, 100
};

const FlyskyNv14Sensor* getFlyskyNv14Sensor(uint16_t id, uint8_t subId)
{
	for(unsigned index = 0; index < *(&Nv14Sensor + 1) - Nv14Sensor; index++ ){
		if (Nv14Sensor[index].id == id && Nv14Sensor[index].subId == subId) {
			return &Nv14Sensor[index];
		}
	}
    return &defaultNv14Sensor;
}

void flySkyNv14SetDefault(int index, uint8_t id, uint8_t subId, uint8_t instance)
{
    TelemetrySensor & telemetrySensor = g_model.telemetrySensors[index];
    telemetrySensor.id = id;
    telemetrySensor.subId = subId;
    telemetrySensor.instance = instance;
    const FlyskyNv14Sensor* sensor = getFlyskyNv14Sensor(id, subId);
    telemetrySensor.init(sensor->name, sensor->unit, sensor->precision);
    storageDirty(EE_MODEL);
}

uint16_t tempToK(int16_t temperture){
	return (uint16_t)(temperture) + 2731;
}

int32_t log2fix(uint32_t x){
	int32_t b = 1U << (FIXED_PRECISION - 1);
	int32_t y = 0;
	while (x < 1U << FIXED_PRECISION) {
		x <<= 1;
		y -= 1U << FIXED_PRECISION;
	}

	while (x >= 2U << FIXED_PRECISION) {
		x >>= 1;
		y += 1U << FIXED_PRECISION;
	}

	uint64_t z = x;
	for (size_t i = 0; i < FIXED_PRECISION; i++) {
		z = z*z >> FIXED_PRECISION;
		if (z >= 2U << FIXED_PRECISION) {
			z >>= 1;
			y += b;
		}
		b >>= 1;
	}
	return y;
}


int getALT(uint32_t pressurePa, uint16_t temperture){
	if(pressurePa == 0) return 0;
	static int32_t initPressure;
	static int32_t initTemperature;

    uint16_t temperatureK = tempToK(temperture);

    if (initPressure <= 0) {
    	initPressure = pressurePa;
    	initTemperature = temperatureK;
    }
    int temperature = (initTemperature + temperatureK) >> 1; //div 2
    bool tempNegative = temperature < 0;
    if (tempNegative)  temperature = temperature *-1;
    uint64_t helper = R_DIV_G_MUL_10_Q15;
    helper = helper *(uint64_t)temperature;
    helper = helper >> FIXED_PRECISION;
	uint32_t po_to_p = (uint32_t)(initPressure << (FIXED_PRECISION-1));
	po_to_p = po_to_p / pressurePa;
	//shift missing bit
	po_to_p = po_to_p << 1;
	if(po_to_p == 0) return 0;
	uint64_t t =  log2fix(po_to_p) * INV_LOG2_E_Q1DOT31;
	int32_t ln = t >> 31;
    bool neg = ln < 0;
    if (neg) ln = ln * -1;
    helper = helper * (uint64_t)ln;
    helper = helper >> FIXED_PRECISION;
    int result = (int)helper;
    if (neg ^ tempNegative) result = result * -1;
    return result;
}



int32_t GetSensorValueFlySkyNv14(const FlyskyNv14Sensor* sensor, const uint8_t * data){
  int32_t value = 0;
  const nv14SensorData* sensorData = reinterpret_cast<const nv14SensorData*>(data + sensor->offset);
  if(sensor->bytes == 1) value = sensorData->UINT8;
  else if(sensor->bytes == 2) value = sensor->issigned ? sensorData->INT16 : sensorData->UINT16;
  else if(sensor->bytes == 4) value = sensorData->UINT32;

  //special sensors
  if(sensor->id == FLYSKY_SENSOR_RX_SNR && sensor->subId != 0) {
    if(value < MIN_SNR) value = 0;
    if(value > MAX_SNR) value = 100;
    else value = snrToSig[value-MIN_SNR];
  }
  if(sensor->id == FLYSKY_SENSOR_PRESURRE && sensor->subId != 0){
    value = getALT(value, 250);
  }
  return value;
}

void flySkyNv14ProcessTelemetryPacket(const uint8_t * ptr, uint8_t sensorID )
{
  uint8_t instnace = *ptr++;
  if(sensorID == FLYSKY_SENSOR_RX_VOLTAGE) sensorID = FLYSKY_FIXED_RX_VOLTAGE;
  for (const FlyskyNv14Sensor sensor : Nv14Sensor) {
  if (sensor.id == sensorID) {
  int32_t value = GetSensorValueFlySkyNv14(&sensor, ptr);
  setTelemetryValue(TELEM_PROTO_FLYSKY_NV14, sensor.id, sensor.subId, instnace, value, sensor.unit, sensor.precision);
}
    }

    telemetryStreaming = TELEMETRY_TIMEOUT10ms;
}

