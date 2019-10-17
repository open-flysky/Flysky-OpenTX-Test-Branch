/*
 * Copyright (C) OpenTX
 *
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

#include "model_crossfire.h"

CrossfireDevice::CrossfireDevice(uint8_t *data, uint16_t now){
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

CrossfireParameter::CrossfireParameter(uint8_t number, uint8_t devAddress) {
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
CrossfireParameter::~CrossfireParameter() {
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


uint8_t CrossfireParameter::getFolder(){
    if(!data) return 0;
    return data[X_FIRE_FOLDER_OFFSET];
  }

  bool CrossfireParameter::isVisible(){
    return data && (data[X_FIRE_TYPE_OFFSET] & 0x80) == 0;
  }

  crossfire_state CrossfireParameter::getState(){
    return this->state;
  }
  void CrossfireParameter::setState(crossfire_state state){
    this->state = state;
  }
  crossfire_data_type CrossfireParameter::dataType(){
    if(!data) return OUT_OF_RANGE;
    return static_cast<crossfire_data_type>(data[X_FIRE_TYPE_OFFSET] & 0x7F);
  }

  uint8_t* CrossfireParameter::getDataOffset(uint8_t* dataPtr){
    uint8_t* result = dataPtr + X_FIRE_HEADER_LEN;
    result += strlen(reinterpret_cast<char*>(result)) + 1;
    return result;
  }

  char* CrossfireParameter::getEditableTextBuffer(){
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

  const xfire_data* CrossfireParameter::getValue(){
    return reinterpret_cast<const xfire_data*>(getDataOffset(data));
  }

  const char** CrossfireParameter::getItems(size_t& itemsCount) {
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
  const char* CrossfireParameter::getItemsList() {
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

  const char* CrossfireParameter::getUnit(){
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
  void CrossfireParameter::reset(crossfire_state targetState) {
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
  void CrossfireParameter::parse(uint8_t *payload, size_t length, uint16_t now){
    //skip type,
    payload += 1;
    //skip crc
    length -= 2;
    tries = 0;
    timeout = now + 200;
    chunksRemaining = payload[X_FIRE_CHUNKS_OFFSET];
    if (state == X_LOADING) {
      if (data == NULL) {
        data = new uint8_t[X_FIRE_FRAME_LEN * (chunksRemaining + 1)];
        dataPtr = data;
      } else {
        length -= X_FIRE_NEXT_CHUNK_DATA;
        payload += X_FIRE_NEXT_CHUNK_DATA;
      }
      memcpy(dataPtr, payload, length);
      dataPtr += length;
      const char* nameptr = reinterpret_cast<const char*>(data + X_FIRE_HEADER_LEN);
      if (!chunkActual && name.length() == 0) name = std::string(nameptr);
      chunkActual++;
      if (chunksRemaining == 0) setState(X_IDLE);
    }
    if(state == X_SAVING) {
      if (dataType() == COMMAND) {
        xfire_data* currentData = reinterpret_cast<xfire_data*>(getDataOffset(data));
        const xfire_data* xdata = reinterpret_cast<const xfire_data*>(getDataOffset(payload));
        currentData->COMMAND.status = xdata->COMMAND.status;
        switch (xdata->COMMAND.status) {
        case XFIRE_PROGRESS:
          TRACE("XFIRE_PROGRESS");
          if (mb == nullptr) {
            mb = new MessageBox(WARNING_TYPE_INFO, DialogResult::Cancel, "Progress...", xdata->COMMAND.info, [=](DialogResult result) {
                  TRACE("XFIRE_PROGRESS result");
                  if(result ==  DialogResult::Cancel){
                    save(XFIRE_CANCEL);
                  }
                  mb = nullptr;
            });
            mb->setCloseCondition([=]() {

              if(state == X_IDLE) {
                TRACE("CLOSE CONDITION DETECTED");
                return DialogResult::OK;
              }
              return (DialogResult)0;
            });
          }
          mb->setMessage(xdata->COMMAND.info);
          TRACE("XFIRE_POLL");
          //if (strlen(xdata->COMMAND.info) > 0) mb->setMessage(xdata->COMMAND.info);
          save(XFIRE_POLL);
          break;
        case XFIRE_CONFIRMATION_NEEDED:
          TRACE("XFIRE_CONFIRMATION_NEEDED");
          if(mb!=nullptr) delete mb;
          mb = new MessageBox(WARNING_TYPE_ASTERISK, (DialogResult)(DialogResult::OK|DialogResult::Cancel), "Confirmation", xdata->COMMAND.info,
            [=](DialogResult result) {
                TRACE("XFIRE_CONFIRMATION_NEEDED result");
                save(result == DialogResult::OK ? XFIRE_CONFIRM : XFIRE_CANCEL);
                mb = nullptr;
            }
          );
          mb->setUpdateMethod(std::bind(&CrossfireParameter::checkCommand, this));
          break;
        case XFIRE_READY:
          TRACE("XFIRE_READY");
          setState(X_IDLE);
          break;
        }
      }
      else if (chunksRemaining == 0) setState(X_IDLE);
    }
  }
  void CrossfireParameter::checkCommand() {
    uint8_t dataSize = 0;
    uint16_t now = get_tmr10ms();
    if (crossfireGet(telemetryBuffer, dataSize)) {
      if (telemetryBuffer[FRAME_TYPE_OFFSET] == ENTRY_SETTINGS_ID) {
        if (number == telemetryBuffer[FRAME_PARAM_NUM_OFFSET]) {
          parse(telemetryBuffer, dataSize, now);
        }
      }
    }
  }

  void CrossfireParameter::load(){
    if(chunksRemaining <= 0) setState(X_IDLE);
    else if(tries >= RETRY_COUNT) setState(X_RX_ERROR);
    //else if(timeout >= get_tmr10ms()) setState(X_TX_ERROR);
    else {
      setState(X_LOADING);
      tries++;
      uint8_t payload[] = { READ_SETTINGS_ID, devAddress, RADIO_ADDRESS, number, chunkActual };
      crossfireSend(payload, sizeof(payload));
    }
  }


  void CrossfireParameter::save(uint8_t* newData, size_t length){
    size_t totalLength = 4 + length;
    uint8_t payload[64] = { WRITE_SETTINGS_ID, devAddress, RADIO_ADDRESS, number };
    memcpy(payload + 4, newData, length);
    if(dataType() <= FLOAT || dataType() == COMMAND){
      memcpy(getDataOffset(data), newData, length);
    }
    else if(dataType() == TEXT_SELECTION){
      memcpy(const_cast<uint8_t*>(getValue()->TEXT_SELECTION.selectedPtr()), newData, length);
    }
    timeout = get_tmr10ms() + (dataType() == COMMAND ? getValue()->COMMAND.timout / 10: 200);
    reset(X_SAVING);
    crossfireSend(payload, totalLength);
  }

  void CrossfireParameter::pool(){
    if(state == X_SAVING && dataType() == COMMAND){
      const xfire_data* val = getValue();
      if(val->COMMAND.status == XFIRE_START || val->COMMAND.status == XFIRE_PROGRESS){
        save(XFIRE_POLL);
      }
    }
  }

  void CrossfireParameter::setText(const char* text){
    if(strlen(text) == 0) return;
    if(control == NULL) return;
    TextButton* t = dynamic_cast<TextButton*>(control);
    if(t == NULL) return;
    t->setText(std::string(text));
  }


CrossfireConfigPage::CrossfireConfigPage(CrossfireDevice* device, CrossfireMenu* menu) : PageTab(device->devName, ICON_RADIO_HARDWARE), xmenu(menu){
  this->device = device;
  for(uint8_t index = 1; index <= device->paramsCount; index++){
    parameters.push_back(new CrossfireParameter(index, device->devAddress));
  }
  this->device->configPage = this;
}
CrossfireConfigPage::~CrossfireConfigPage() {
  /*
  auto itr = parameters.begin();
  while (itr != parameters.end()) {
    CrossfireParameter* params = *itr;
    delete params;
    itr++;
  }
  parameters.clear();
  */
}



void CrossfireConfigPage::rebuildPage() {
  GridLayout grid;
  grid.spacer(16);
  if (state == X_IDLE){
    window->clear();
    createControls(grid, 0);
  }
  window->setInnerHeight(grid.getWindowHeight());
}


void CrossfireConfigPage::createControls(GridLayout& grid, uint8_t folder, uint8_t level) {
  grid.setMarginLeft(level ? 5 : 1);

  for (auto param = parameters.begin(); param != parameters.end(); param++) {
    CrossfireParameter* p = *param;
    if (p->isVisible() && p->number && p->name.length()) {
      uint8_t currentFolder = p->getFolder();
      if (currentFolder == folder && p->isVisible()) {
        crossfire_data_type dt = p->dataType();
        const xfire_data* val = p->getValue();
        if(dt <= INFO) new StaticText(window, grid.getLabelSlot(), p->name);
        switch (dt) {
          case FOLDER:
          {
            grid.nextLine();
            createControls(grid, p->number, level + 1);
          }
          break;
          case INFO:
          {
            new StaticText(window, grid.getFieldSlot(), std::string(p->getEditableTextBuffer()));
          }
          break;
          case COMMAND:
          {
            TextButton* btn = new TextButton(window, grid.getLineSlot(), p->name);
            p->control = btn;
            btn->setPressHandler(
              [=]() -> uint8_t {
                state = X_SAVING;
                crossfire_cmd_status status = val->COMMAND.status;
                if(status == XFIRE_READY) p->save(XFIRE_START);
                return status == XFIRE_READY;
              });
            }
          break;
          case UINT8:
          {
            new NumberEdit(window, grid.getFieldSlot(),
              val->UINT8.minVal, val->UINT8.maxVal,
              [=]() -> int32_t {
                return val->UINT8.value;
              }, [=](int32_t x) {
                state = X_SAVING;
                p->save(static_cast<uint8_t>(x));
              });
          }
          break;
          case INT8:
          {
            new NumberEdit(window, grid.getFieldSlot(),
              val->INT8.minVal, val->INT8.maxVal,
              [=]() -> int32_t {
                return val->INT8.value;
              }, [=](int32_t newValue) {
                state = X_SAVING;
                p->save(static_cast<int8_t>(newValue));
              });
          }
          break;
          case UINT16:
          {
            new NumberEdit(window, grid.getFieldSlot(),
              static_cast<uint16_t>(bswapu16(val->UINT16.minVal)),
              static_cast<uint16_t>(bswapu16(val->UINT16.maxVal)),
              [=]() -> int32_t {
                return static_cast<uint16_t>(bswapu16(val->UINT16.value));
              },
              [=](int32_t newValue) {
                state = X_SAVING;
                p->save(static_cast<uint16_t>(bswapu16(static_cast<uint16_t>(newValue))));
              });
          }
          break;
          case INT16:
          {
            new NumberEdit(window, grid.getFieldSlot(),
              static_cast<int16_t>(bswaps16(val->INT16.minVal)),
              static_cast<int16_t>(bswaps16(val->INT16.maxVal)),
              [=]() -> int32_t {
                return static_cast<int16_t>(bswaps16(val->INT16.value));
              },
              [=](int32_t newValue) {
                state = X_SAVING;
                p->save(static_cast<int16_t>(bswaps16(static_cast<int16_t>(newValue))));
              });
          }
          break;
          case STRING:
          {
            new TextEdit(window, grid.getFieldSlot(),
              p->getEditableTextBuffer(),
              val->STRING.maxLength(), 0, [=](char* newValue) {
              state = X_SAVING;
              p->save(reinterpret_cast<uint8_t*>(newValue), strlen(newValue)+1);
              }, false);

          }
          break;
          case TEXT_SELECTION:
          {
            new Choice(window, grid.getFieldSlot(), p->getItemsList(),
              (int16_t) val->TEXT_SELECTION.minVal(),
              (int16_t) val->TEXT_SELECTION.maxVal(),
              [=]() -> int16_t {return val->TEXT_SELECTION.selected();},
              [=](int16_t newValue) {
                state = X_SAVING;
                p->save(newValue);
              });
          }
          break;
          case FLOAT:
          {
            LcdFlags lcdFlag = 0;
            if (val->FLOAT.decimalPoint == 1) lcdFlag = PREC1;
            if (val->FLOAT.decimalPoint == 2) lcdFlag = PREC2;

            new NumberEdit(window, grid.getFieldSlot(),
              static_cast<int32_t>(bswapu32(val->FLOAT.minVal)),
              static_cast<int32_t>(bswapu32(val->FLOAT.maxVal)),
              [=]() -> int32_t {
                return static_cast<int32_t>(bswapu32(val->FLOAT.value));
              },
              [=](int32_t newValue) {
                state = X_SAVING;
                p->save(static_cast<int32_t>(bswapu32(static_cast<int32_t>(newValue))));
              }, lcdFlag);
          }
          break;
        default:
          break;
        }
        if (dt != FOLDER) {
          grid.spacer(8);
          grid.nextLine();
        }
      }
    }
  }
  grid.setMarginLeft(6);
}



CrossfireMenu::CrossfireMenu() : TabsGroup(){

}

void CrossfireMenu::removePage(PageTab * page){
  if(page == NULL) return;
  auto it = tabs.begin();
  int index = 0;
  while (it != tabs.end()) {
    if((*it) == page){
      removeTab(index);
      break;
    }
    index++;
    it++;
  }
}


void CrossfireConfigPage::updateHeaderStatus() {
  char buff[100];
  auto param = parameters.begin();
  if(state == X_LOADING) {
    int total = 0;
    int loaded = 0;
    while (param != parameters.end()) {
      total++;
      if((*param)->chunksRemaining == 0) loaded++;
      param++;
    }
    sprintf(buff, "%s Loading %d/%d", device->devName.c_str(), loaded, total);
    title = std::move(std::string(buff));
  }
  else if(state == X_SAVING){
    title = std::move(std::string("Saving..."));
  }
  else title = std::move(std::string(device->devName));
  if (xmenu != NULL) xmenu->setTitle(title.c_str());
}

void CrossfireConfigPage::update() {
  uint16_t now = get_tmr10ms();
  auto param = parameters.begin();
  uint8_t dataSize = 0;
  if (crossfireGet(telemetryBuffer, dataSize)) {
    if (telemetryBuffer[FRAME_TYPE_OFFSET] == ENTRY_SETTINGS_ID) {
      auto param = parameters.begin();
      while (param != parameters.end()) {
        if ((*param)->number == telemetryBuffer[FRAME_PARAM_NUM_OFFSET]) {
          (*param)->parse(telemetryBuffer, dataSize, now);
          break;
        }
        param++;
      }
      timoutSettings = 0; //load next
    }
  }
  if (now > timoutSettings) {
    timoutSettings = get_tmr10ms() + 200;
    crossfire_state currentState = X_IDLE;
    //Loading or saving
    if (state >= X_LOADING) {
      while (param != parameters.end()) {
        if (state == X_LOADING) (*param)->load();
        if (state == X_SAVING) (*param)->pool();
        if ((*param)->getState() >= X_LOADING) {
          currentState = (*param)->getState();
          break;
        }
        param++;
      }
      if (currentState != state) {
        crossfire_state prevState = state;
        //load saved values
        state = currentState;
        if (state == X_IDLE && prevState == X_LOADING) rebuildPage();
      }
    }
    updateHeaderStatus();
  }
  /*
  std::list<CrossfireDevice*>::iterator itr = devices.begin();
  uint8_t dataSize = 0;
  CrossfireDevice* dev = 0;
  if (get(telemetryBuffer, dataSize) && telemetryBuffer[0] == DEVICE_INFO_ID) {
    if (dataSize < 18) return;
    dev = new CrossfireDevice(telemetryBuffer, now);
  } else if (timout < now) {
    luaInputTelemetryFifo->clear();
    timout = get_tmr10ms() + 100;
    uint8_t payload[] = { PING_DEVICES_ID, 0x00, RADIO_ADDRESS };
    send(payload, sizeof(payload));
  }
  bool changed = false;
  bool found = false;
  while (itr != devices.end()) {
    CrossfireDevice* device = *itr;
    if(dev!=0 && device->serial == dev->serial) device->timeout = now + 300;
    if (device->timeout <= now) {
      //erase invalidates existing iterators,
      //but it returns a new iterator pointing
      //to the element after the one that was removed
      itr = devices.erase(itr);
      delete device;
      changed = true;
    }
    else{
      itr++;
    }

  }

  if(dev!=0 && !found) {
    devices.push_back(dev);
    changed = true;
  }

  */
}

CrossfireMenu::~CrossfireMenu(){
  /*
  auto itr = devices.begin();
  while (itr != devices.end()) {
    CrossfireDevice* device = *itr;
    delete device;
    itr++;
  }
  this->devices.clear();
  */
}

void CrossfireMenu::update() {
  uint16_t now = get_tmr10ms();
  uint8_t dataSize = 0;
  while (crossfireGet(telemetryBuffer, dataSize)) {
    if (telemetryBuffer[FRAME_TYPE_OFFSET] == DEVICE_INFO_ID) {
      bool found = false;
      for (auto itr = devices.begin(); itr != devices.end(); itr++) {
        CrossfireDevice* device = *itr;
        if (device->devAddress == telemetryBuffer[2]) {
          device->timeout = now + 1000; //+ 10sek
          found = true;
          break;
        }
      }
      if (!found) {
        CrossfireDevice* dev = new CrossfireDevice(telemetryBuffer,  now);
        devices.push_back(dev);
        addTab(new CrossfireConfigPage(dev, this));
        invalidate();
      }
    }
  }
  if(now > timeout){
    timeout = now + 500;
    crossfireSend(pingCommand, sizeof(pingCommand));
  }
}



