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

/* Full telemetry 
packet[0] = TX RSSI value
packet[1] = TX LQI value
packet[2] = frame number
packet[3-7] telemetry data

The frame number takes the following values: 0x00, 0x11, 0x12, ..., 0x18. The frames can be present or not, they also do not have to follow each others.
Here is a description of the telemetry data for each frame number:
- frame 0x00
data byte 0 -> 0x00				unknown
data byte 1 -> 0x00				unknown
data byte 2 -> 0x00				unknown
data byte 3 -> RX Batt Volt_H
data byte 4 -> RX Batt Volt_L => RX Batt=(Volt_H*256+Volt_L)/28
- frame 0x11
data byte 0 -> 0xAF				start of frame
data byte 1 -> 0x00				unknown
data byte 2 -> 0x2D				frame type but constant here
data byte 3 -> Volt1_H
data byte 4 -> Volt1_L			RX Batt=(Volt1_H*256+Volt1_L)/28 V
- frame 0x12
data byte 0 -> Lat_sec_H		GPS : latitude second
data byte 1 -> Lat_sec_L		signed int : 1/100 of second
data byte 2 -> Lat_deg_min_H	GPS : latitude degree.minute
data byte 3 -> Lat_deg_min_L	signed int : +=North, - = south
data byte 4 -> Time_second		GPS Time
- frame 0x13
data byte 0 -> 					GPS Longitude second
data byte 1 -> 					signed int : 1/100 of second
data byte 2 -> 					GPS Longitude degree.minute
data byte 3 -> 					signed int : +=Est, - = west
data byte 4 -> Temp2			Temperature2=Temp2-40°C
- frame 0x14
data byte 0 -> Speed_H
data byte 1 -> Speed_L			Speed=Speed_H*256+Speed_L km/h
data byte 2 -> Alti_sea_H
data byte 3 -> Alti_sea_L		Altitude sea=Alti_sea_H*256+Alti_sea_L m
data byte 4 -> Temp1			Temperature1=Temp1-40°C
- frame 0x15
data byte 0 -> FUEL
data byte 1 -> RPM1_L
data byte 2 -> RPM1_H			RPM1=RPM1_H*256+RPM1_L
data byte 3 -> RPM2_L
data byte 4 -> RPM2_H			RPM2=RPM2_H*256+RPM2_L
- frame 0x16
data byte 0 -> Date_year		GPS Date
data byte 1 -> Date_month
data byte 2 -> Date_day
data byte 3 -> Time_hour		GPS Time
data byte 4 -> Time_min
- frame 0x17
data byte 0 -> 0x00	COURSEH
data byte 1 -> 0x00	COURSEL		GPS Course = COURSEH*256+COURSEL
data byte 2 -> 0x00				GPS count
data byte 3 -> Temp3			Temperature3=Temp2-40°C
data byte 4 -> Temp4			Temperature4=Temp3-40°C
- frame 0x18
data byte 1 -> Volt2_H
data byte 2 -> Volt2_L			Volt2=(Volt2_H*256+Volt2_L)/10 V
data byte 3 -> AMP1_L
data byte 4 -> AMP1_H			Amp=(AMP1_H*256+AMP1_L -180)/14 in signed A
*/

#define HITEC_TELEMETRY_LENGTH 8

struct HitecSensor {
  const uint16_t id;
  const char *name;
  const TelemetryUnit unit;
  const uint8_t precision;
};

// telemetry frames
enum{
    HITEC_FRAME_00     = 0x00,
    HITEC_FRAME_11     = 0x11,
    HITEC_FRAME_12     = 0x12,
    HITEC_FRAME_13     = 0x13,
    HITEC_FRAME_14     = 0x14,
    HITEC_FRAME_15     = 0x15,
    HITEC_FRAME_16     = 0x16,
    HITEC_FRAME_17     = 0x17,
    HITEC_FRAME_18     = 0x18
};

// telemetry sensors ID
enum{
    HITEC_ID_RX_VOLTAGE     = 0x0003,    // RX_Batt Voltage
    HITEC_ID_VOLTAGE1       = 0x1103,    // Voltage sensor 1
    HITEC_ID_GPS_LAT_SEC    = 0x1200,    // GPS latitude sec
    HITEC_ID_GPS_LAT_DEG    = 0x1202,    // GPS latitude deg
    HITEC_ID_GPS_TIME_SEC   = 0x1204,    // GPS time sec
    HITEC_ID_GPS_LON_SEC    = 0x1300,    // GPS longitude sec
    HITEC_ID_GPS_LON_DEG    = 0x1302,    // GPS longitude deg
    HITEC_ID_TEMP2          = 0x1304,    // Temperature sensor 2
    HITEC_ID_SPEED          = 0x1400,    // Speed
    HITEC_ID_ALTITUDE       = 0x1402,    // Altitude sea level
    HITEC_ID_TEMP1          = 0x1404,    // Temperature sensor 1
    HITEC_ID_FUEL           = 0x1500,    // Fuel
    HITEC_ID_RPM1           = 0x1501,    // RPM1
    HITEC_ID_RPM2           = 0x1503,    // RPM2
    HITEC_ID_GPS_DATE_YEAR  = 0x1600,    // GPS date year
    HITEC_ID_GPS_DATE_MONTH = 0x1601,    // GPS date month
    HITEC_ID_GPS_DATE_DAY   = 0x1602,    // GPS date day
    HITEC_ID_GPS_TIME_HOUR  = 0x1603,    // GPS time hour
    HITEC_ID_GPS_TIME_MIN   = 0x1604,    // GPS time minute
    HITEC_ID_GPS_COURSE     = 0x1700,    // GPS course
    HITEC_ID_GPS_COUNT      = 0x1702,    // GPS count
    HITEC_ID_TEMP3          = 0x1703,    // Temperature sensor 3
    HITEC_ID_TEMP4          = 0x1704,    // Temperature sensor 4
    HITEC_ID_VOLTAGE2       = 0x1800,    // Voltage sensor 2
    HITEC_ID_AMP            = 0x1803,    // Amp sensor
    TX_RSSI_ID              = 0xFF00,    // Pseudo id outside 1 byte range of Hitec sensors
    TX_LQI_ID               = 0xFF01,    // Pseudo id outside 1 byte range of Hitec sensors
};

const HitecSensor hitecSensors[] = {
  //frame 00
  {HITEC_ID_RX_VOLTAGE,       ZSTR_BATT,              UNIT_VOLTS,                  2},  // RX_Batt Voltage
  //frame 11
  {HITEC_ID_VOLTAGE1,         ZSTR_A1,                UNIT_VOLTS,                  2},  // Voltage sensor 1
  //frame 12
  {HITEC_ID_GPS_LAT_SEC,      ZSTR_GPS,               UNIT_RAW,                    0},  // GPS latitude sec
  {HITEC_ID_GPS_LAT_DEG,      ZSTR_GPS,               UNIT_RAW,                    0},  // GPS latitude deg
  {HITEC_ID_GPS_TIME_SEC,     ZSTR_GPS,               UNIT_RAW,                    0},  // GPS time sec
  //frame 13
  {HITEC_ID_GPS_LON_SEC,      ZSTR_GPS,               UNIT_RAW,                    0},  // GPS longitude sec
  {HITEC_ID_GPS_LON_DEG,      ZSTR_GPS,               UNIT_RAW,                    0},  // GPS longitude deg
  {HITEC_ID_TEMP2,            ZSTR_TEMP2,             UNIT_CELSIUS,                0},  // Temperature sensor 2
  //frame 14
  {HITEC_ID_SPEED,            ZSTR_ASPD,              UNIT_KMH,                    0},  // Speed
  {HITEC_ID_ALTITUDE,         ZSTR_ALT,               UNIT_METERS,                 0},  // Altitude sea level
  {HITEC_ID_TEMP1,            ZSTR_TEMP1,             UNIT_CELSIUS,                0},  // Temperature sensor 1
  //frame 15
  {HITEC_ID_FUEL,             ZSTR_FUEL,              UNIT_RAW,                    0},  // Fuel
  {HITEC_ID_RPM1,             ZSTR_RPM,               UNIT_RPMS,                   0},  // RPM1
  {HITEC_ID_RPM2,             ZSTR_RPM2,              UNIT_RPMS,                   0},  // RPM2
  //frame 16
  {HITEC_ID_GPS_DATE_YEAR,    ZSTR_GPS,               UNIT_RAW,                    0},  // GPS date year
  {HITEC_ID_GPS_DATE_MONTH,   ZSTR_GPS,               UNIT_RAW,                    0},  // GPS date month
  {HITEC_ID_GPS_DATE_DAY,     ZSTR_GPS,               UNIT_RAW,                    0},  // GPS date day
  {HITEC_ID_GPS_TIME_HOUR,    ZSTR_GPS,               UNIT_RAW,                    0},  // GPS time hour
  {HITEC_ID_GPS_TIME_MIN,     ZSTR_GPS,               UNIT_RAW,                    0},  // GPS time minute
  //frame 17
  {HITEC_ID_GPS_COURSE,       ZSTR_GPS,               UNIT_RAW,                    0},  // GPS course
  {HITEC_ID_GPS_COUNT,        ZSTR_SATELLITES,        UNIT_RAW,                    0},  // GPS count
  {HITEC_ID_TEMP3,            ZSTR_TEMP3,             UNIT_CELSIUS,                0},  // Temperature sensor 3
  {HITEC_ID_TEMP4,            ZSTR_TEMP4,             UNIT_CELSIUS,                0},  // Temperature sensor 4
  //frame 18
  {HITEC_ID_VOLTAGE2,         ZSTR_A2,                UNIT_VOLTS,                  1},  // Voltage sensor 2
  {HITEC_ID_AMP,              ZSTR_CURR,              UNIT_AMPS,                   2},  // Amp sensor

  {TX_RSSI_ID,                ZSTR_TX_RSSI,           UNIT_RAW,                    0},  // Pseudo id outside 1 byte range of Hitec sensors
  {TX_LQI_ID,                 ZSTR_TX_QUALITY,        UNIT_RAW,                    0},  // Pseudo id outside 1 byte range of Hitec sensors// Pseudo sensor for TLQI
  {0x00,                      NULL,                   UNIT_RAW,                    0},  // sentinel
};

void processHitecPacket(const uint8_t *packet)
{
  // Set TX RSSI Value, reverse MULTIs scaling
  setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, TX_RSSI_ID, 0, 0, packet[0], UNIT_RAW, 0);
  telemetryData.rssi.set(packet[0]);
  if(packet[0]>0) telemetryStreaming = TELEMETRY_TIMEOUT10ms;
  // Set TX LQI  Value, reverse MULTIs scaling
  setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, TX_LQI_ID , 0, 0, packet[1], UNIT_RAW, 0);

  const HitecSensor *sensor;
  int32_t value;
  
  switch(packet[2])
  {
    case HITEC_FRAME_00:
      value=(((packet[6]<<8)|packet[7])*100)/28;
      sensor = getHitecSensor(HITEC_ID_RX_VOLTAGE);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_RX_VOLTAGE, 0, 0, value, sensor->unit, sensor->precision);
      return;
    case HITEC_FRAME_11:
      value=(((packet[6]<<8)|packet[7])*100)/28;
      sensor = getHitecSensor(HITEC_ID_VOLTAGE1);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_VOLTAGE1, 0, 0, value, sensor->unit, sensor->precision);
      return;
    case HITEC_FRAME_12:
      //TO DO GPS
      return;
    case HITEC_FRAME_13:
      //TO DO GPS
      value=packet[7]-40;
      sensor = getHitecSensor(HITEC_ID_TEMP2);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_TEMP2, 0, 0, value, sensor->unit, sensor->precision);
      return;
    case HITEC_FRAME_14:
      value=(packet[3]<<8)|packet[4];
      sensor = getHitecSensor(HITEC_ID_SPEED);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_SPEED, 0, 0, value, sensor->unit, sensor->precision);
      value=(packet[5]<<8)|packet[6];
      sensor = getHitecSensor(HITEC_ID_ALTITUDE);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_ALTITUDE, 0, 0, value, sensor->unit, sensor->precision);
      value=packet[7]-40;
      sensor = getHitecSensor(HITEC_ID_TEMP1);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_TEMP1, 0, 0, value, sensor->unit, sensor->precision);
      return;
    case HITEC_FRAME_15:
      value=packet[3];
      sensor = getHitecSensor(HITEC_ID_FUEL);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_FUEL, 0, 0, value, sensor->unit, sensor->precision);
      value=(packet[5]<<8)|packet[4];
      sensor = getHitecSensor(HITEC_ID_RPM1);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_RPM1, 0, 0, value, sensor->unit, sensor->precision);
      value=(packet[7]<<8)|packet[6];
      sensor = getHitecSensor(HITEC_ID_RPM2);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_RPM2, 0, 0, value, sensor->unit, sensor->precision);
      return;
    case HITEC_FRAME_16:
      //TO DO GPS
      return;
    case HITEC_FRAME_17:
      value=(packet[3]<<8)|packet[4];
      sensor = getHitecSensor(HITEC_ID_GPS_COURSE);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_GPS_COURSE, 0, 0, value, sensor->unit, sensor->precision);
      value=packet[5];
      sensor = getHitecSensor(HITEC_ID_GPS_COUNT);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_GPS_COUNT, 0, 0, value, sensor->unit, sensor->precision);
      value=packet[6]-40;
      sensor = getHitecSensor(HITEC_ID_TEMP3);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_TEMP3, 0, 0, value, sensor->unit, sensor->precision);
      value=packet[7]-40;
      sensor = getHitecSensor(HITEC_ID_TEMP4);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_TEMP4, 0, 0, value, sensor->unit, sensor->precision);
      return;
    case HITEC_FRAME_18:
      value=(packet[4]<<8)|packet[5];
      sensor = getHitecSensor(HITEC_ID_VOLTAGE2);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_VOLTAGE2, 0, 0, value, sensor->unit, sensor->precision);
      value=(((packet[6]<<8)|packet[7])*100)/14;
      sensor = getHitecSensor(HITEC_ID_AMP);
      setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, HITEC_ID_AMP, 0, 0, value, sensor->unit, sensor->precision);
      return;
  }
  //unknown
  value = (packet[6] << 24) | (packet[5] << 16) | (packet[4] << 8) | packet[3];
  setTelemetryValue(PROTOCOL_TELEMETRY_HITEC, packet[2], 0, 0, value, UNIT_RAW, 0);
}

void processHitecTelemetryData(uint8_t data, uint8_t* rxBuffer, uint8_t* rxBufferCount)
{
  if ((*rxBufferCount) == 0 && (data != 0x00 || data < 0xAC)) {
    TRACE("[HITEC] invalid start byte 0x%02X", data);
    return;
  }

  if ((*rxBufferCount) < TELEMETRY_RX_PACKET_SIZE) {
    rxBuffer[(*rxBufferCount)++] = data;
  }
  else {
    TRACE("[HITEC] array size %d error", (*rxBufferCount));
    (*rxBufferCount) = 0;
  }


  if ((*rxBufferCount) >= HITEC_TELEMETRY_LENGTH) {
    // debug print the content of the packets
#if 0
    debugPrintf("[HITEC] Packet rssi 0x%02X lqi 0x%02X frame 0x%02X: ",
                rxBuffer[0], rxBuffer[1], rxBuffer[2]);
    for (int i=0; i<5; i++) {
      debugPrintf("%02X ", rxBuffer[3+i]);
    }
    debugPrintf("\r\n");
#endif
	processHitecPacket(rxBuffer);
    (*rxBufferCount) = 0;
  }
}

const HitecSensor *getHitecSensor(uint16_t id)
{
  for (const HitecSensor * sensor = hitecSensors; sensor->id; sensor++) {
    if (id == sensor->id)
      return sensor;
  }
  return nullptr;
}

void hitecSetDefault(int index, uint16_t id, uint8_t subId, uint8_t instance)
{
  TelemetrySensor &telemetrySensor = g_model.telemetrySensors[index];
  telemetrySensor.id = id;
  telemetrySensor.subId = subId;
  telemetrySensor.instance = instance;

  const HitecSensor *sensor = getHitecSensor(id);
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
