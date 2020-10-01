
#include "crc.h"
#include "hallStick_parser.h"
#include "hallStick_driver.h"


void hallStickParser::parse(STRUCT_HALL *buffer, unsigned char data) {
  if (parseState != 0) return;
  parseState = 1;
  switch (buffer->status)
  {
    case GET_START:
      if (HALL_PROTOLO_HEAD == data)
      {
        buffer->head = HALL_PROTOLO_HEAD;
        buffer->status = GET_ID;
        buffer->valid = 0;
      }
      break;
    case GET_ID:
      buffer->hallID.ID = data;
      buffer->status = GET_LENGTH;
      break;
    case GET_LENGTH:
      buffer->length = data;
      buffer->dataIndex = 0;
      buffer->status = GET_DATA;
      if (0 == buffer->length)
      {
        buffer->status = GET_CHECKSUM;
        buffer->checkSum=0;
      }
      break;
    case GET_DATA:
      buffer->data[buffer->dataIndex++] = data;
      if (buffer->dataIndex >= buffer->length)
      {
        buffer->checkSum = 0;
        buffer->dataIndex = 0;
        buffer->status = GET_CHECKSUM;
      }
      break;
    case GET_CHECKSUM:
      buffer->checkSum |= data << ((buffer->dataIndex++) * 8);
      if (buffer->dataIndex >= 2 )
      {
        buffer->dataIndex = 0;
        buffer->valid = (buffer->checkSum == crc16(CRC_1021, (const uint8_t*)&buffer->head, buffer->length + 3, 0xffff));
        buffer->status = GET_START;
      }
      break;
  }
  parseState = 0;
}
  