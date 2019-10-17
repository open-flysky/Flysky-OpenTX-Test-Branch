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

  CrossfireDevice(uint8_t *data, uint16_t now);
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
    int index = 0;
    result[index] = strtok(buffer, ";");
    while (result[index]) {
      count++;
      result[++index] = strtok(NULL, ";");
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
  MessageBox* mb;
  uint8_t telemetryBuffer[64] = { };

  CrossfireParameter(uint8_t number, uint8_t devAddress);
  ~CrossfireParameter();

  uint8_t getFolder();
  bool isVisible();
  crossfire_state getState();
  void setState(crossfire_state state);
  crossfire_data_type dataType();
  uint8_t* getDataOffset(uint8_t* dataPtr);
  char* getEditableTextBuffer();
  const xfire_data* getValue();
  const char** getItems(size_t& itemsCount);
  //return items in format [len][item1][item2]..[itemN] no null termination
  const char* getItemsList();
  //valid for UINT8, INT8, UINT16, INT16, FLOAT
  const char* getUnit();
  void reset(crossfire_state targetState);
  //data pointer to payload
  void parse(uint8_t *payload, size_t length, uint16_t now);
  void load();
  void save(uint8_t* newData, size_t length);
  void pool();
  void setText(const char* text);
  void checkCommand();

  template<typename Type>
  void save(Type data)
  {
    uint8_t* a = reinterpret_cast<uint8_t*>(&data);
    save(a, sizeof(data));
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


class CrossfireMenu: public TabsGroup {
public:
  CrossfireMenu();
  ~CrossfireMenu();
  void removePage(PageTab * page);
  void pingDevices();
  void checkEvents() override {
    CrossfireConfigPage* page = dynamic_cast<CrossfireConfigPage*>(currentTab);
    if (page == NULL || !page->isIoActive())
      update();
    else
      TabsGroup::checkEvents();
  }
  void setTitle(const char * value) {
    header.setTitle(value);
    invalidate();
  }

protected:
  uint8_t telemetryBuffer[64] = { };
  uint8_t pingCommand[3] = { PING_DEVICES_ID, 0, RADIO_ADDRESS };
  uint16_t timeout = 0;
  std::list<CrossfireDevice*> devices;
  void update();
};

#endif
