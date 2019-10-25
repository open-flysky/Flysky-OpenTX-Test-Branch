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
  coord_t scroll = window->getScrollPositionY();
  if (state == X_IDLE){
    window->clear();
    createControls(grid, 0);
  }
  window->setInnerHeight(grid.getWindowHeight());
  window->setScrollPositionY(scroll);
}


void CrossfireConfigPage::createControls(GridLayout& grid, uint8_t folder, uint8_t level) {
  grid.setMarginLeft(level ? 3 : 9);

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
                if(status == XFIRE_CONFIRMATION_NEEDED) p->save(XFIRE_CONFIRM);
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
            int16_t min = val->TEXT_SELECTION.minVal();
            int16_t max = val->TEXT_SELECTION.maxVal();
            //First item is always 0
            //it looks 0 can be only displayed...
            //according to docs first item is always 0
            Choice* choice = new Choice(window, grid.getFieldSlot(), p->getItemsList(),
              (int16_t) min > 0 ? 0 : min ,
              (int16_t) max,
              [=]() -> int16_t {return val->TEXT_SELECTION.selected();},
              [=](int16_t newValue) {
                state = X_SAVING;
                p->save(newValue);
              });
            //irregular list
            if(min > 0){
              choice->setAvailableHandler([=](int16_t value) {
                if(value < 0) return false;
                uint8_t v = (uint8_t) value;
                return v >= min && v <= max;
              });
            }

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



