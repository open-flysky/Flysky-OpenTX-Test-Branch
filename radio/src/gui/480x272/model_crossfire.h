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

#ifndef MODEL_CROSSFIRE_H_
#define MODEL_CROSSFIRE_H_

#include "tabsgroup.h"
#include "window.h"
#include "libwindows.h"
#include <list>
#include "crossfire.h"
#include "opentx.h"
#include "telemetry.h"
#include "translations.h"

class CrossfirePage;
class CrossfireConfigPage;
class CrossfireMenu;

#if defined(SIMU)
  #define bswapu16 __builtin_bswap16
  #define bswaps16 __builtin_bswap16
  #define bswapu32 __builtin_bswap32
#else
  #define bswapu16 __REV16
  #define bswaps16 __REVSH
  #define bswapu32 __REV
#endif


#define CRSF_ALL_DEVICES 0x00
#define FRAME_TYPE_OFFSET 0
#define FRAME_PARAM_NUM_OFFSET 3
#define RETRY_COUNT 5

enum crossfire_data_type : uint8_t {
  UINT8 = 0,
  INT8 = 1,
  UINT16 = 2,
  INT16 = 3,
  FLOAT  = 8,
  TEXT_SELECTION = 9,
  STRING = 10,
  FOLDER = 11,
  INFO = 12,
  COMMAND = 13,
  OUT_OF_RANGE = 127
};

enum crossfire_state : uint8_t {
  X_IDLE = 0,
  X_WAITING_FOR_LOAD = 1,
  X_RX_ERROR = 2,
  X_TX_ERROR = 3,
  X_LOADING = 4,
  X_SAVING = 5
};

enum crossfire_cmd_status : uint8_t {
  XFIRE_READY = 0,
  XFIRE_START = 1,
  XFIRE_PROGRESS = 2,
  XFIRE_CONFIRMATION_NEEDED = 3,
  XFIRE_CONFIRM = 4,
  XFIRE_CANCEL = 5,
  XFIRE_POLL = 6
};



class CrossfireDevice {
public:
  uint8_t destAddress;
  uint8_t devAddress;
  std::string devName;
  uint32_t serial;
  uint32_t hwID;
  uint32_t fwID;
  uint8_t paramsCount;
  uint8_t paramVersion;
  uint16_t timeout;
  CrossfireConfigPage* configPage;

  CrossfireDevice(uint8_t *data, uint16_t now){
    this->timeout = now + 300;
    data++; //skip type
    this->destAddress = *(data++);
    this->devAddress = *(data++);
    const char* str = reinterpret_cast<const char*>(data);
    this->devName= std::string(str);
    data += strlen(str)+1;
    this->serial = *reinterpret_cast<uint32_t*>(data);
    data+=4;
    this->hwID = *reinterpret_cast<uint32_t*>(data);
    data+=4;
    this->fwID = *reinterpret_cast<uint32_t*>(data);
    data+=4;
    this->paramsCount = *(data++);
    this->paramVersion = *(data++);
  }
};





template <typename T>
struct xfire_value {
  T value;
  T minVal;
  T maxVal;
  T defVal;
};

struct xfire_float : xfire_value<int32_t>  {
  uint8_t decimalPoint;
  int32_t step;
};

struct xfire_comand {
  crossfire_cmd_status status;
  uint8_t timout; //ms*100
  char info[]; //Null-terminated string
};

struct xfire_text {
  char firstChar;

  const char* text() const {
    return &firstChar;
  }
  const char* defaultText() const {
    return text() + strlen(text()) + 1;
  }
  const uint8_t maxLength() const {
    const char* def = defaultText();
    def += strlen(def) + 1;
    const uint8_t length = *reinterpret_cast<const uint8_t*>(def);
    if(length > 128) return 128;
    if(length == 0) return strlen(text());
    return length;
  }
};

struct xfire_text_select {
  char firstChar;

  const char* text() const {
    return &firstChar;
  }
  const uint8_t* data() const {
    return reinterpret_cast<const uint8_t*>(text() + strlen(text()) + 1) ;
  }
  void getItems(char* buffer, char** result, size_t& count) const {
    strcpy(buffer, text());
    count = 0;
    result[count++] = strtok(buffer, ";"); //first item must be there

    while (count < minVal()) {
      result[count++] = 0; //fill the gap
    }

    while(true) {
      char *token = strtok(NULL, ";");
      if(token) result[count++] = token;
      else break;
    }
  }
  uint8_t* selectedPtr() const {
    return const_cast<uint8_t*>(data());
  }
  uint8_t selected() const {
    return *data();
  }
  uint8_t minVal() const {
    return *(data() + 1);
  }
  uint8_t maxVal() const {
    return *(data() + 2);
  }
  uint8_t defVal() const {
    return *(data() + 3);
  }

};

union xfire_data {
  xfire_value<uint8_t>   UINT8;
  xfire_value<int8_t>   INT8;
  xfire_value<uint16_t>   UINT16;
  xfire_value<int16_t>   INT16;
  xfire_float       FLOAT;
  xfire_text        STRING;
  xfire_comand      COMMAND;
  xfire_text_select    TEXT_SELECTION;


};

/*
class CrosfireData {
  public:
  CrosfireData(){}
protected:
  uint8_t telemetryBuffer[64] = {};
    bool get(uint8_t* buffer, uint8_t& dataSize);
    void send(uint8_t* payload, size_t size);
};
*/


#define X_FIRE_HEADER_LEN  6
#define X_FIRE_CHUNKS_OFFSET 3
#define X_FIRE_FOLDER_OFFSET 4
#define X_FIRE_NEXT_CHUNK_DATA 4
#define X_FIRE_TYPE_OFFSET 5
#define X_FIRE_FRAME_LEN 64

class CrossfireParameter {
protected:
  //[header][payload chunk 0][payload chunk 1]..[payload chunk n]
  uint8_t* data;
  uint8_t* dataPtr;
  uint8_t devAddress;
  crossfire_state state = X_WAITING_FOR_LOAD;
  friend class CrossfireCommand;

public:

  uint8_t number;
  uint8_t chunkActual;
  uint16_t timeout;
  uint8_t tries;
  uint8_t chunksRemaining;
  Window* control;

  std::string name;
  //copy of text - max length set - so it can be edited
  char* text_buffer;
  char** items;
  int items_count;
  char* itemsList;

  CrossfireParameter(uint8_t number, uint8_t devAddress) {
    this->state = X_WAITING_FOR_LOAD;
    this->devAddress = devAddress;
    this->number = number;
    this->data = NULL;
    this->dataPtr = NULL;
    this->chunkActual = 0;
    this->timeout = 0;
    this->tries = 0;
    this->chunksRemaining = 1;
    this->items_count = 0;
    this->text_buffer = NULL;
    this->itemsList = NULL;
    this->items = NULL;
    this->control = NULL;
  }

  ~CrossfireParameter() {
    if (itemsList != NULL) {
      delete[] itemsList;
      itemsList = NULL;
    }
    if (text_buffer != NULL) {
      delete[] text_buffer;
      text_buffer = NULL;
    }
    if (items != NULL) {
      for (int i = 0; i < items_count; i++) {
        delete[] items[i];
        items[i] = NULL;
      }
      delete[] items;
      items = NULL;
      items_count = 0;
    }
  }

  uint8_t getFolder(){
    if(!data) return 0;
    return data[X_FIRE_FOLDER_OFFSET];
  }

  bool isVisible(){
    return data && (data[X_FIRE_TYPE_OFFSET] & 0x80) == 0;
  }

  crossfire_state getState(){
    return this->state;
  }
  void setState(crossfire_state state){
#if defined(DEBUG)
    if(this->state == X_LOADING && state == X_IDLE) {
      char buffer[256];
      sprintf(buffer, "XFIRE DONE [%d] size = %d ", number, dataPtr - data);
      debug(buffer, data, dataPtr - data);
    }
#endif
    this->state = state;
  }
  crossfire_data_type dataType(){
    if(!data) return OUT_OF_RANGE;
    return static_cast<crossfire_data_type>(data[X_FIRE_TYPE_OFFSET] & 0x7F);
  }

  uint8_t* getDataOffset(uint8_t* dataPtr){
    uint8_t* result = dataPtr + X_FIRE_HEADER_LEN;
    result += strlen(reinterpret_cast<char*>(result)) + 1;
    return result;
  }

  char* getEditableTextBuffer(){
    if(!data) return 0;
    const xfire_data* xdata = getValue();
    if(text_buffer == NULL) {
      size_t size = strlen(xdata->STRING.text());
      if(dataType() == STRING) size = xdata->STRING.maxLength();
      text_buffer = new char[size];
    }
    strcpy(text_buffer, xdata->STRING.text());
    return text_buffer;
  }

  const xfire_data* getValue(){
    return reinterpret_cast<const xfire_data*>(getDataOffset(data));
  }

  const char** getItems(size_t& itemsCount) {
    const xfire_data* xdata = getValue();
    if (dataType() == TEXT_SELECTION) {
      items_count = 0;
      size_t len = strlen(xdata->STRING.text());
      for (size_t i = 0; i < len+1; i++)
        if (xdata->TEXT_SELECTION.text()[i] == ';' || xdata->TEXT_SELECTION.text()[i] == 0)
          items_count++;
      if (text_buffer == NULL) {
        text_buffer = new char[len + 1];
        items = new char*[items_count];
      }
      strcpy(text_buffer, xdata->TEXT_SELECTION.text());
      for(size_t i= 0; i < len; i++){
        if(text_buffer[i] < ' ' || text_buffer[i] > '~') text_buffer[i] = ' ';
      }
      xdata->TEXT_SELECTION.getItems(this->text_buffer, this->items, itemsCount);
      return const_cast<const char**>(items);
    }
    return NULL;
  }

  //return items in format [len][item1][item2]..[itemN] no null termination
  const char* getItemsList() {
    //if (itemsList != NULL)
    //  return const_cast<const char*>(itemsList);
    size_t count = 0;
    size_t maxSize = 0;
    const char** list = getItems(count);

    for (size_t index = 0; index < count; index++) {
      size_t s = strlen(list[index]);
      if (s > maxSize)
        maxSize = s;
    }
    if (itemsList != NULL) delete [] itemsList;
    itemsList = new char[count * maxSize + 2];

    char* pos = itemsList;
    *pos++ = (char)(maxSize);

    for (size_t index = 0; index < count; index++) {
      size_t length = strlen(list[index]);
      size_t s = maxSize - length;
      strcpy(pos, list[index]);
      pos += length;
      while(s-- > 0){
        *(pos++) = ' ';
      }
    }
    return itemsList;

  }

  //valid for UINT8, INT8, UINT16, INT16, FLOAT
  const char* getUnit(){
    uint8_t* unit = getDataOffset(data);
    switch(dataType()){
    case UINT8:
    case INT8:
      unit += sizeof(xfire_value<uint8_t>);
      break;
    case UINT16:
    case INT16:
      unit += sizeof(xfire_value<uint16_t>);
      break;
    case FLOAT:
      unit += sizeof(xfire_float);
      break;
    }
    return reinterpret_cast<const char*>(getDataOffset(data));
  }
  void reset(crossfire_state targetState) {
    tries = 0;
    chunksRemaining = 1;
    chunkActual = 0;
    state = targetState;
    /*
    if (data != NULL) {
      delete[] data;
      data = NULL;
      dataPtr = NULL;
    }
    */
  }
  //data pointer to payload
  void parse(uint8_t *payload, size_t length, uint16_t now){
#if defined(DEBUG)
    char buffer[64];
    sprintf(buffer, "[%d] RESPONSE %s size [%d] ", number, (state == X_SAVING ? "SAVING" : "LOADING"), length);
    debug(buffer, payload, length);
#endif
    //skip type,
    payload += 1;
    //skip crc
    length -= 2;
    tries = 0;
    timeout = now + 200;
    chunksRemaining = payload[X_FIRE_CHUNKS_OFFSET];

    if (data == NULL) {
      int sizeTotal = X_FIRE_FRAME_LEN * (chunksRemaining + 1);
      data = new uint8_t[sizeTotal];
      memset(data, 0, sizeTotal);
      dataPtr = data;
    }

    if(!chunkActual) dataPtr = data;
    else {
      length -= X_FIRE_NEXT_CHUNK_DATA;
      payload += X_FIRE_NEXT_CHUNK_DATA;
    }

    chunkActual++;

    memcpy(dataPtr, payload, length);
    dataPtr += length;

    if (state == X_LOADING) {
      const char* nameptr = reinterpret_cast<const char*>(data + X_FIRE_HEADER_LEN);
      if (name.length() == 0) name = std::string(nameptr);
      if (chunksRemaining == 0) setState(X_IDLE);
    }
    if(state == X_SAVING) {
      if (dataType() == COMMAND) {
        xfire_data* currentData = reinterpret_cast<xfire_data*>(getDataOffset(data));
        const xfire_data* xdata = reinterpret_cast<const xfire_data*>(getDataOffset(payload));
        currentData->COMMAND.status = xdata->COMMAND.status;
        switch (xdata->COMMAND.status) {
        case XFIRE_PROGRESS:
          if (strlen(xdata->COMMAND.info) > 0) setText(xdata->COMMAND.info);
          save(XFIRE_POLL);
          break;
        case XFIRE_CONFIRMATION_NEEDED:
          if (strlen(xdata->COMMAND.info) > 0) setText(xdata->COMMAND.info);
          break;
        case XFIRE_READY:
          if(strlen(xdata->COMMAND.info) > 0) setText(xdata->COMMAND.info);
          else setText(name.c_str());
          setState(X_IDLE);
          break;
        }
      }
      else if(chunksRemaining) setState(X_LOADING);
      else setState(X_IDLE);
    }
  }

  void load(){
    if(chunksRemaining <= 0) {
      setState(X_IDLE);
    }
    else if(tries >= RETRY_COUNT) setState(X_RX_ERROR);
    //else if(timeout >= get_tmr10ms()) setState(X_TX_ERROR);
    else {
      setState(X_LOADING);
      tries++;
#if defined(DEBUG)
      char buffer[64];
      sprintf(buffer, "[%d] LOAD ", number);
      uint8_t payload[] = { READ_SETTINGS_ID, devAddress, RADIO_ADDRESS, number, chunkActual };
      debug(buffer, payload, sizeof(payload));
#endif
      crossfireSend(payload, sizeof(payload));
    }
  }
#if defined(DEBUG)
  void debug(char* step, uint8_t* data, int length) {
    char buffer[256];
    TRACE("%s", step);
    int count = 0;
    for (int c =0; c < length; c++){
      count += sprintf(buffer + count, "%02X", data[c]);
      if(count >= 32) {
        TRACE("%s", buffer);
        count = 0;
      }
    }
    if(count > 0) TRACE("%s", buffer);
  }
#endif
  template<typename Type>
  void save(Type data)
  {
    uint8_t* a = reinterpret_cast<uint8_t*>(&data);
    save(a, sizeof(data));
  }

  void save(uint8_t* newData, size_t length){
    size_t totalLength = 4 + length;
    uint8_t payload[64] = { WRITE_SETTINGS_ID, devAddress, RADIO_ADDRESS, number };
    memcpy(payload + 4, newData, length);
    if(dataType() <= FLOAT || dataType() == COMMAND){
      memcpy(getDataOffset(data), newData, length);
    }
    else if(dataType() == TEXT_SELECTION){
      memcpy(const_cast<uint8_t*>(getValue()->TEXT_SELECTION.selectedPtr()), newData, length);
    }
    timeout = get_tmr10ms() + (dataType() == COMMAND ? getValue()->COMMAND.timout / 10 : 200);
    reset(X_SAVING);
#if defined(DEBUG)
    char buffer[64];
    sprintf(buffer, "[%d] SAVE ", number);
    debug(buffer, payload, totalLength);
#endif
    crossfireSend(payload, totalLength);
  }

  void pool() {
    if (state == X_SAVING) {
      if (dataType() == COMMAND) {
        const xfire_data* val = getValue();
        if (val->COMMAND.status == XFIRE_START || val->COMMAND.status == XFIRE_PROGRESS) {
          save(XFIRE_POLL);
        }
      }
    }
  }

  void setText(const char* text){
    if(strlen(text) == 0) return;
    if(control == NULL) return;
    TextButton* t = dynamic_cast<TextButton*>(control);
    if(t == NULL) return;
    t->setText(std::string(text));
  }
};


class CrossfireConfigPage: public PageTab {
  public:
  std::list<CrossfireParameter*> parameters;
  uint16_t timoutSettings = 0;

  CrossfireConfigPage(CrossfireDevice* device, CrossfireMenu* menu);
  ~CrossfireConfigPage();
  void update();
  void updateHeaderStatus();

  void build(Window * window) override {
      this->window = window;
      rebuildPage();
    }
    void checkEvents() override {
        PageTab::checkEvents();
        update();
    }
    bool isIoActive(){
      auto param = parameters.begin();
      while (param != parameters.end()) {
        if((*param)->getState() >= X_LOADING) return true;
        param++;
      }
      return  state >= X_LOADING;
    }
  protected:
    crossfire_state state = X_LOADING;
    uint8_t telemetryBuffer[64] = {};
    Window * window = nullptr;
    CrossfireDevice* device;
    CrossfireMenu* xmenu;


    void rebuildPage();

    void createControls(GridLayout& grid, uint8_t folder, uint8_t level = 0);
};


class CrossfireMenu : public TabsGroup {
public:
  CrossfireMenu();
  ~CrossfireMenu();
  void removePage(PageTab * page);
  void pingDevices();
  void checkEvents() override {
    CrossfireConfigPage* page =  dynamic_cast<CrossfireConfigPage*>(currentTab);
    if (page == NULL || !page->isIoActive()) update();
    else TabsGroup::checkEvents();
  }
    void setTitle(const char * value)
    {
      header.setTitle(value);
      invalidate();
    }

protected:
    uint8_t telemetryBuffer[64] = {};
    uint8_t pingCommand[3] = { PING_DEVICES_ID, 0, RADIO_ADDRESS };
    uint16_t timeout = 0;
    std::list<CrossfireDevice*> devices;
    void update();
};

#endif
