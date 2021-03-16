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

#include "model_setup.h"
#include "opentx.h"
#include "libwindows.h"
#include "model_crossfire.h"
#include "radio_ghost_menu.h"

#define SET_DIRTY()     storageDirty(EE_MODEL)
#define STR_4BIND(v)    ((moduleState[moduleIndex].mode == MODULE_MODE_BIND) ? STR_MODULE_BINDING : (v))

void resetModuleSettings(uint8_t module)
{
  g_model.moduleData[module].channelsStart = 0;
  g_model.moduleData[module].channelsCount = defaultModuleChannels_M8(module);
  g_model.moduleData[module].rfProtocol = 0;
  g_model.moduleData[module].subType = 0;
  g_model.moduleData[module].failsafeMode = FAILSAFE_NOT_SET;
  if (isModulePPM(module)) {
    SET_DEFAULT_PPM_FRAME_LENGTH(EXTERNAL_MODULE);
  }
#if defined(AFHDS3)
  else if (isModuleAFHDS3(module)) {
    g_model.moduleData[module].afhds3.setDefault();
  }
#endif
  setModuleFlag(module, MODULE_MODE_RESET_SETTINGS);
  setModuleFlag(module, MODULE_MODE_NORMAL);
}

#if defined(MULTIMODULE)
inline void resetMultiProtocolsOptions(uint8_t moduleIdx)
{
  if (!isModuleMultimodule(moduleIdx))
    return;

  // Sensible default for DSM2 (same as for ppm): 7ch@22ms + Autodetect settings enabled
  if (g_model.moduleData[moduleIdx].getMultiProtocol() == MODULE_SUBTYPE_MULTI_DSM2) {
    g_model.moduleData[moduleIdx].multi.autoBindMode = 1;
  }
  else {
    g_model.moduleData[moduleIdx].multi.autoBindMode = 0;
  }
  g_model.moduleData[moduleIdx].multi.optionValue = 0;
  g_model.moduleData[moduleIdx].multi.disableTelemetry = 0;
  g_model.moduleData[moduleIdx].multi.disableMapping = 0;
  g_model.moduleData[moduleIdx].multi.lowPowerMode = 0;
  g_model.moduleData[moduleIdx].failsafeMode = FAILSAFE_NOT_SET;
  g_model.header.modelId[moduleIdx] = 0;
}

#endif
class ChannelFailsafeBargraph: public Window {
  public:
    ChannelFailsafeBargraph(Window * parent, const rect_t & rect, uint8_t moduleIndex, uint8_t channel):
      Window(parent, rect),
      moduleIndex(moduleIndex),
      channel(channel)
    {
    }

    void checkEvents() override
    {
      invalidate();
    }


    void paint(BitmapBuffer * dc) override
    {
      int32_t failsafeValue = g_model.moduleData[moduleIndex].failsafeChannels[channel];
      int32_t channelValue = channelOutputs[channel];

      const int lim = (g_model.extendedLimits ? (512 * LIMIT_EXT_PERCENT / 100) : 512) * 2;

      coord_t x = 0;
      lcdDrawRect(x, 0, width(), height());
      const coord_t lenChannel = limit((uint8_t) 1, uint8_t((abs(channelValue) * width() / 2 + lim / 2) / lim),
                                       uint8_t(width() / 2));
      const coord_t lenFailsafe = limit((uint8_t) 1, uint8_t((abs(failsafeValue) * width() / 2 + lim / 2) / lim),
                                        uint8_t(width() / 2));
      x += width() / 2;
      const coord_t xChannel = (channelValue > 0) ? x : x + 1 - lenChannel;
      const coord_t xFailsafe = (failsafeValue > 0) ? x : x + 1 - lenFailsafe;
      lcdDrawSolidFilledRect(xChannel, + 2, lenChannel, (height() / 2) - 3, TEXT_COLOR);
      lcdDrawSolidFilledRect(xFailsafe, (height() / 2) + 1, lenFailsafe, (height() / 2) - 3, ALARM_COLOR);
    }

  protected:
    uint8_t moduleIndex;
    uint8_t channel;
};

class FailSafeBody : public Window {
  public:
    FailSafeBody(Window * parent, const rect_t &rect, uint8_t moduleIndex) :
      Window(parent, rect),
      moduleIndex(moduleIndex)
    {
      build();
    }

    void checkEvents() override
    {
      invalidate();
    }

    void build()
    {
      GridLayout grid;
      grid.setLabelWidth(60);
      grid.spacer(8);

      const int lim = (g_model.extendedLimits ? (512 * LIMIT_EXT_PERCENT / 100) : 512) * 2;
      int minChannel = g_model.moduleData[moduleIndex].channelsStart;
      int maxChannel = minChannel + 8 + g_model.moduleData[moduleIndex].channelsCount;

      //MAX_OUTPUT_CHANNELS maxModuleChannels(moduleIndex)
      for (int ch = minChannel; ch < maxChannel; ch++) {
        // Channel name
        // TODO if (g_model.limitData[ch].name[0] != '\0') { <= add channel name
        new StaticText(this, grid.getLabelSlot(), getSourceString(MIXSRC_CH1 + ch));

        // Channel numeric value
        new NumberEdit(this, grid.getFieldSlot(3, 0), -lim, lim,
                       GET_DEFAULT(g_model.moduleData[moduleIndex].failsafeChannels[ch]),
                       //GET_DEFAULT(calcRESXto1000(g_model.moduleData[moduleIndex].failsafeChannels[ch])),
                       SET_VALUE(g_model.moduleData[moduleIndex].failsafeChannels[ch], newValue), PREC1);

        // Channel bargraph
        new ChannelFailsafeBargraph(this, {150, grid.getWindowHeight(), 150, lineHeight}, moduleIndex, ch);
        grid.nextLine();
      }

      auto out2fail = new TextButton(this, grid.getLineSlot(), STR_OUTPUTS2FAILSAFE);
      out2fail->setPressHandler([=]() {
        setCustomFailsafe(moduleIndex);
        storageDirty(EE_MODEL);
        return 0;
      });

      grid.nextLine();
      setInnerHeight(grid.getWindowHeight());
    }

  protected:
    uint8_t moduleIndex;
};

class FailSafeFooter : public Window {
  public:
    FailSafeFooter(Window * parent, const rect_t &rect) :
      Window(parent, rect)
    {
    }

    void paint(BitmapBuffer * dc) override
    {
    }
};

class FailSafePage : public PageTab {
  public:
    FailSafePage(uint8_t moduleIndex) :
      PageTab("FAILSAFE", ICON_STATS_ANALOGS),
      moduleIndex(moduleIndex)
    {
    }

    void build(Window * window) override
    {
      new FailSafeBody(window, {0, 0, LCD_W, window->height() - footerHeight}, moduleIndex );
      new FailSafeFooter(window, {0, window->height() - footerHeight, LCD_W, footerHeight});
    }

  protected:
    static constexpr coord_t footerHeight = 30;
    uint8_t moduleIndex;
};

FailSafeMenu::FailSafeMenu(uint8_t moduleIndex) :
  TabsGroup()
{
  addTab(new FailSafePage(moduleIndex));
}
void onBindMenu(const char * result, uint8_t moduleIndex);




class ModuleWindow : public Window {
  public:
    ModuleWindow(Window * parent, const rect_t &rect, uint8_t moduleIndex) :
      Window(parent, rect),
      moduleIndex(moduleIndex)
    {
      update();
    }

    ~ModuleWindow()
    {
      deleteChildren();
    }

  protected:
    uint8_t moduleIndex;
    Choice * moduleChoice = nullptr;
    TextButton * bindButton = nullptr;
    TextButton * rangeButton = nullptr;
    Choice * failSafeChoice = nullptr;
    TextButton * failSafeSetButton = nullptr;
    NumberEdit * channelStart = nullptr;
    NumberEdit * channelEnd = nullptr;
    Choice* afhds3RxPower = nullptr;

    bool isPPM(uint8_t moduleIndex){
      if(moduleIndex == TRAINER_MODULE) return g_model.trainerMode == TRAINER_MODE_SLAVE;
      return isModulePPM(moduleIndex);
    }
    void addChannelRange(GridLayout &grid, uint8_t moduleIndex)
    {
      new StaticText(this, grid.getLabelSlot(true), STR_CHANNELRANGE);
      channelStart = new NumberEdit(this, grid.getFieldSlot(2, 0), 1,
                                         MAX_OUTPUT_CHANNELS - sentModuleChannels(moduleIndex) + 1,
                                         GET_DEFAULT(1 + g_model.moduleData[moduleIndex].channelsStart));
      channelEnd = new NumberEdit(this, grid.getFieldSlot(2, 1),
                                       g_model.moduleData[moduleIndex].channelsStart + minModuleChannels(moduleIndex),
                                       min<int8_t>(MAX_OUTPUT_CHANNELS, g_model.moduleData[moduleIndex].channelsStart + maxModuleChannels(moduleIndex)),
                                       GET_DEFAULT(g_model.moduleData[moduleIndex].channelsStart + 8 + g_model.moduleData[moduleIndex].channelsCount));

      channelStart->setPrefix(STR_CH);
      channelEnd->setPrefix(STR_CH);
      channelStart->setSetValueHandler([=](int32_t newValue) {
        g_model.moduleData[moduleIndex].channelsStart = newValue - 1;
        if(g_model.moduleData[moduleIndex].failsafeMode == FAILSAFE_CUSTOM) {
          //reset fail safe on channel range set - in custom mode it was set only for actual selection
          g_model.moduleData[moduleIndex].failsafeMode = FAILSAFE_NOT_SET;
          if(failSafeChoice) failSafeChoice->invalidate();
          if(failSafeSetButton) failSafeSetButton->enable(false);
        }
        SET_DIRTY();
        channelEnd->setMin(g_model.moduleData[moduleIndex].channelsStart + minModuleChannels(moduleIndex));
        channelEnd->setMax(min<int8_t>(MAX_OUTPUT_CHANNELS, g_model.moduleData[moduleIndex].channelsStart + maxModuleChannels(moduleIndex)));
        channelEnd->invalidate();
      });
      channelEnd->setSetValueHandler([=](int32_t newValue) {
        g_model.moduleData[moduleIndex].channelsCount = newValue - g_model.moduleData[moduleIndex].channelsStart - 8;
        if(g_model.moduleData[moduleIndex].failsafeMode == FAILSAFE_CUSTOM) {
          //reset fail safe on channel range set - in custom mode it was set only for actual selection
          g_model.moduleData[moduleIndex].failsafeMode = FAILSAFE_NOT_SET;
          if(failSafeChoice) failSafeChoice->invalidate();
          if(failSafeSetButton) failSafeSetButton->enable(false);
        }
        SET_DIRTY();
        channelStart->setMax(MAX_OUTPUT_CHANNELS - sentModuleChannels(moduleIndex) + 1);
      });
      channelEnd->enable(minModuleChannels(moduleIndex) < maxModuleChannels(moduleIndex));
    }

    void update()
    {
      GridLayout grid;
      uint8_t moduleType = g_model.moduleData[moduleIndex].type;

      clear();
      moduleChoice = nullptr;
      bindButton = nullptr;
      rangeButton = nullptr;
      failSafeChoice = nullptr;
      failSafeSetButton = nullptr;
      channelStart = nullptr;
      channelEnd = nullptr;
      afhds3RxPower = nullptr;
      // Module Type
      new StaticText(this, grid.getLabelSlot(true), STR_MODE);
      if (moduleIndex == TRAINER_MODULE) {
        moduleChoice = new Choice(this, grid.getFieldSlot(), STR_VTRAINERMODES, 0, TRAINER_MODE_MAX(), GET_DEFAULT(g_model.trainerMode), [=](int32_t newValue) {
          g_model.trainerMode = newValue;
          g_model.moduleData[moduleIndex].type = newValue == TRAINER_MODE_SLAVE ? MODULE_TYPE_PPM : MODULE_TYPE_NONE;
          update();
          SET_DIRTY();
          moduleChoice->setFocus();
        });
        moduleChoice->setAvailableHandler([=](int8_t moduleType) { return isTrainerModeAvailable(moduleType);});
      }
      else {
        moduleChoice = new Choice(this, grid.getFieldSlot(),
            STR_MODULE_PROTOCOLS, MODULE_TYPE_NONE, MODULE_TYPE_COUNT - 1,
            GET_DEFAULT(g_model.moduleData[moduleIndex].type),
            [=](int32_t newValue) {
              g_model.moduleData[moduleIndex].type = newValue;
              SET_DIRTY();
              resetModuleSettings(moduleIndex);
              if(moduleIndex == INTERNAL_MODULE) {
                onFlySkyModuleSetPower(newValue == MODULE_TYPE_FLYSKY);
              }
              update();
              moduleChoice->setFocus();
            });
        moduleChoice->setAvailableHandler([=](int8_t moduleType) {
          return isModuleTypeAllowed(moduleIndex, moduleType);
        });
      }
#if defined(DEBUG)
      if (moduleIndex == EXTERNAL_MODULE) {
         grid.nextLine();
         new StaticText(this, grid.getLabelSlot(true), TR_POWER);
         new CheckBox(this, grid.getFieldSlot(), [=] { return IS_EXTERNAL_MODULE_ON(); }, [=](int32_t newValue) {
          if(IS_EXTERNAL_MODULE_ON()) {
            EXTERNAL_MODULE_OFF();
          } else {
            EXTERNAL_MODULE_ON();
          }
        });
      }
#endif
#if defined(AFHDS2)
      if (isModuleFlysky(moduleIndex) && NV14internalModuleFwVersion) {
        grid.nextLine();
        new StaticText(this, grid.getLabelSlot(true), STR_FW_VERSION);
        sprintf(reusableBuffer.moduleSetup.msg, "%d.%d.%d", (int)((NV14internalModuleFwVersion >> 16) & 0xFF), (int)((NV14internalModuleFwVersion >> 8) & 0xFF), (int)(NV14internalModuleFwVersion & 0xFF));
        new StaticText(this, grid.getFieldSlot(), reusableBuffer.moduleSetup.msg);

      }
#endif
      // Module parameters
#if defined(DEBUG)
      if (isModuleFlysky(moduleIndex)) {
        grid.nextLine();
        new Choice(this, grid.getFieldSlot(), STR_FLYSKY_MODES, 0, 2,
                   GET_DEFAULT(g_model.moduleData[moduleIndex].subType),
                   [=](int32_t newValue) -> void {
                     g_model.moduleData[moduleIndex].subType = newValue;
                     SET_DIRTY();
                     setModuleFlag(moduleIndex, MODULE_MODE_NORMAL);
                     setFlyskyState(STATE_UPDATE_RF_PROTOCOL);
                   });
      }
#endif
      if (isModuleFlysky(moduleIndex) || isModuleAFHDS3(moduleIndex)) {
        grid.nextLine();
        new Choice(this, grid.getFieldSlot(), STR_FLYSKY_PROTOCOLS, 0, 3,
                   GET_DEFAULT(isModuleFlysky(moduleIndex) ? g_model.moduleData[moduleIndex].romData.mode : g_model.moduleData[moduleIndex].afhds3.mode),
                   [=](int32_t newValue) -> void {
                     if(isModuleFlysky(moduleIndex)) g_model.moduleData[moduleIndex].romData.mode = newValue;
                     else g_model.moduleData[moduleIndex].afhds3.mode = newValue;
                     SET_DIRTY();
                     setModuleFlag(moduleIndex, MODULE_MODE_NORMAL);
                     setFlyskyState(STATE_SET_RX_PWM_PPM);
                   });
        
      }
#if defined(AFHDS3)
      if(isModuleAFHDS3(moduleIndex)){
        grid.nextLine();
        new StaticText(this, grid.getLabelSlot(true), STR_MODULE_STATUS);
        StaticText* status = new StaticText(this, grid.getFieldSlot());
        status->setCheckHandler([=]() {
          afhds3uart.getState(reusableBuffer.moduleSetup.msg);
          if(!status->isTextEqual(reusableBuffer.moduleSetup.msg)) {
            status->setText(std::string(reusableBuffer.moduleSetup.msg));
          }
        });
        grid.nextLine();
        new StaticText(this, grid.getLabelSlot(true), STR_POWER_SOURCE);
        StaticText* powerSource = new StaticText(this, grid.getFieldSlot());
        powerSource->setCheckHandler([=]() {
          afhds3uart.getPowerSource(reusableBuffer.moduleSetup.msg);
          if(!powerSource->isTextEqual(reusableBuffer.moduleSetup.msg)) {
            powerSource->setText(std::string(reusableBuffer.moduleSetup.msg));
          }
        });
        grid.nextLine();
        new StaticText(this, grid.getLabelSlot(true), TR_MODE);
        StaticText* afhds3Mode = new StaticText(this, grid.getFieldSlot());
        afhds3Mode->setCheckHandler([=]() {
          afhds3uart.getOpMode(reusableBuffer.moduleSetup.msg);
          if(!afhds3Mode->isTextEqual(reusableBuffer.moduleSetup.msg)) {
            afhds3Mode->setText(std::string(reusableBuffer.moduleSetup.msg));
          }
        });
      }
#endif
#if defined(MULTIMODULE)
      if (isModuleMultimodule(moduleIndex)) {
        grid.nextLine();
        int count = LCD_W < LCD_H ? 1 : 2;
        new StaticText(this, grid.getLabelSlot(true), STR_RF_PROTOCOL);
        // Multi type (CUSTOM, brand A, brand B,...)
        int multiRfProto = g_model.moduleData[moduleIndex].getMultiProtocol();
        //g_model.moduleData[moduleIndex].multi.customProto ? 3 : 2
        auto rfProt = new Choice(this, grid.getFieldSlot(count, 0), STR_MULTI_PROTOCOLS, MODULE_SUBTYPE_MULTI_FIRST, MODULE_SUBTYPE_MULTI_LAST,
                              GET_DEFAULT(multiRfProto),
                              [=](int32_t newValue) {
                                g_model.moduleData[moduleIndex].setMultiProtocol(newValue);
                                resetMultiProtocolsOptions(moduleIndex);
                                SET_DIRTY();
                                update();
                              });
        rfProt->setAvailableHandler([=](int value) { return value != MODULE_SUBTYPE_MULTI_SCANNER; });
        const uint8_t multi_proto = g_model.moduleData[moduleIndex].getMultiProtocol();
        const mm_protocol_definition *pdef = getMultiProtocolDefinition(multi_proto);
        MultiModuleStatus &multiStatus = getMultiModuleStatus(moduleIndex);

        if (pdef->maxSubtype > 0) {
          int index = 1;
          if (count == 1) {
            grid.nextLine();
            index = 0;
          }
          new Choice(this, grid.getFieldSlot(count, index), pdef->subTypeString, 0, pdef->maxSubtype, GET_SET_DEFAULT(g_model.moduleData[moduleIndex].subType));
        }
          
        grid.nextLine();
        // Multimodule status
        new StaticText(this, grid.getLabelSlot(true), STR_MODULE_STATUS);
        StaticText* status = new StaticText(this, grid.getFieldSlot());
        status->setCheckHandler([=]() {
          char statusText[64];
          getMultiModuleStatus(moduleIndex).getStatusString(statusText);
          if(!status->isTextEqual(statusText)){
            status->setText(std::string(statusText));
          }
        });
        // Multimodule sync
        grid.nextLine();
        new StaticText(this, grid.getLabelSlot(true), STR_MODULE_SYNC);
        StaticText* syncStatus = new StaticText(this, grid.getFieldSlot());
        syncStatus->setCheckHandler([=]() {
          char buffer[64];
          getModuleSyncStatus(moduleIndex).getRefreshString(buffer);
          if(!syncStatus->isTextEqual(buffer)){
            syncStatus->setText(std::string(buffer));
          }
        });
        
  
        if ((multiStatus.isValid() && multiStatus.optionDisp == 2) || (pdef->optionsstr && pdef->optionsstr == STR_MULTI_RFTUNE)) {
          grid.nextLine();
          if (multiStatus.optionDisp == 2) {
            new StaticText(this, grid.getLabelSlot(true), mm_options_strings::options[multiStatus.optionDisp]);
          }
          auto rssi = new StaticText(this, grid.getFieldSlot(), STR_MODULE_NO_TELEMETRY);
          rssi->setCheckHandler([=]() {
            char buffer[64];
            sprintf(buffer, "RSSI(%d)", TELEMETRY_RSSI());
            if(!rssi->isTextEqual(buffer)){
              rssi->setText(std::string(buffer));
            }
          });
        }
        // Multi optional feature row
        if (pdef->optionsstr) {
          grid.nextLine();
          int8_t min, max;
          getMultiOptionValues(multi_proto, min, max);
          new StaticText(this, grid.getLabelSlot(true), pdef->optionsstr);
          
          if (multi_proto == MODULE_SUBTYPE_MULTI_FS_AFHDS2A) {
            auto edit = new NumberEdit(this, grid.getFieldSlot(2,0), 50 + 5 * min, 50 + 5 * max,
                           GET_DEFAULT(50 + 5 * g_model.moduleData[moduleIndex].multi.optionValue),
                           SET_VALUE(g_model.moduleData[moduleIndex].multi.optionValue, (newValue- 50) / 5));
            edit->setStep(5);
          }
          else if (multi_proto == MODULE_SUBTYPE_MULTI_FRSKY_R9) {
            new Choice(this, grid.getFieldSlot(), STR_MULTI_POWER, min, max, GET_SET_DEFAULT(g_model.moduleData[moduleIndex].multi.optionValue));
          }
          else if (multi_proto == MODULE_SUBTYPE_MULTI_DSM2) {  
            new CheckBox(this, grid.getFieldSlot(), GET_DEFAULT(g_model.moduleData[moduleIndex].multi.optionValue & 0x01), 
                        [=](int32_t x) { g_model.moduleData[moduleIndex].multi.optionValue = x & 0x1; SET_DIRTY(); });
          }
          else {
            if (min == 0 && max == 1)
              new CheckBox(this, grid.getFieldSlot(), GET_SET_DEFAULT(g_model.moduleData[moduleIndex].multi.optionValue));
            else
              new NumberEdit(this, grid.getFieldSlot(), min, max, GET_SET_DEFAULT(g_model.moduleData[moduleIndex].multi.optionValue));
          }
        }
        // Disable mapping
        grid.nextLine();
        new StaticText(this, grid.getLabelSlot(true), STR_DISABLE_CH_MAP);
        new CheckBox(this, grid.getFieldSlot(), GET_SET_DEFAULT(g_model.moduleData[moduleIndex].multi.disableMapping));

        // Disable telemetry
        grid.nextLine();
        new StaticText(this, grid.getLabelSlot(true), STR_DISABLE_TELEM);
        new CheckBox(this, grid.getFieldSlot(), GET_SET_DEFAULT(g_model.moduleData[moduleIndex].multi.disableTelemetry));

        // Bind on power up
        grid.nextLine();
        new StaticText(this, grid.getLabelSlot(true),g_model.moduleData[moduleIndex].getMultiProtocol() == MODULE_SUBTYPE_MULTI_DSM2 ? STR_MULTI_DSM_AUTODTECT : STR_MULTI_AUTOBIND);
        new CheckBox(this, grid.getFieldSlot(), GET_SET_DEFAULT(g_model.moduleData[moduleIndex].multi.autoBindMode));

        // Low power mode
        grid.nextLine();
        new StaticText(this, grid.getLabelSlot(true), STR_MULTI_LOWPOWER);
        new CheckBox(this, grid.getFieldSlot(), GET_SET_DEFAULT(g_model.moduleData[moduleIndex].multi.lowPowerMode));
      }
#endif

      if (isModuleXJT(moduleIndex)) {
        grid.nextLine();
        auto xjtChoice = new Choice(this, grid.getFieldSlot(), STR_XJT_PROTOCOLS, RF_PROTO_OFF, RF_PROTO_LAST,
                                    GET_DEFAULT(g_model.moduleData[moduleIndex].subType),
                                    [=](int32_t newValue) {
                                        //workaround for eeprom conversion
                                        g_model.moduleData[moduleIndex].subType = (int8_t)newValue;
                                        g_model.moduleData[moduleIndex].rfProtocol = (int8_t)newValue;
                                        int8_t chStart = g_model.moduleData[moduleIndex].channelsStart;
                                        int8_t lastChannel = min<int8_t>(MAX_OUTPUT_CHANNELS, chStart + maxModuleChannels(moduleIndex));
                                        if (chStart + g_model.moduleData[moduleIndex].channelsCount + 8 > lastChannel) {
                                          g_model.moduleData[moduleIndex].channelsCount = lastChannel - chStart - 8;
                                        }
                                        if (channelEnd) {
                                          channelEnd->setMax(lastChannel);
                                          channelEnd->invalidate();
                                        }
                                        SET_DIRTY();
                                        update();//reload fail safe and RX number
                                    });
        xjtChoice->setAvailableHandler([](int index) {
          return index != RF_PROTO_OFF;
        });

      }

      if (isModuleDSM2(moduleIndex)) {
        grid.nextLine();
        new Choice(this, grid.getFieldSlot(), STR_DSM_PROTOCOLS, DSM2_PROTO_LP45, DSM2_PROTO_DSMX, 
                   GET_DEFAULT(g_model.moduleData[moduleIndex].subType),
                   [=](int32_t newValue) {
                      //workaround for eeprom conversion
                      g_model.moduleData[moduleIndex].subType = (int8_t)newValue;
                      g_model.moduleData[moduleIndex].rfProtocol = (int8_t)newValue;
                   });
      }

      if (isModuleR9M(moduleIndex)) {
        grid.nextLine();
        auto r9Choice = new Choice(this, grid.getFieldSlot(), STR_R9M_MODES, MODULE_SUBTYPE_R9M_FCC,
                   MODULE_SUBTYPE_R9M_LAST,
                   GET_DEFAULT(g_model.moduleData[moduleIndex].subType),
                   [=](int32_t newValue) {
                     g_model.moduleData[moduleIndex].subType = newValue;
                     SET_DIRTY();
                     update();
                   });
        r9Choice->setAvailableHandler(isR9MModeAvailable);
      }

      if (isPPM(moduleIndex)) {
        grid.nextLine();
        new Choice(this, grid.getFieldSlot(), "\006""     +""     -", 0,
                   1,
                   GET_DEFAULT(g_model.moduleData[moduleIndex].ppm.pulsePol),
                   [=](int32_t newValue) {
                     g_model.moduleData[moduleIndex].ppm.pulsePol = newValue;
                     SET_DIRTY();
                     update();
                   });
      }

      grid.nextLine();

      // Channel Range
      if (g_model.moduleData[moduleIndex].type != MODULE_TYPE_NONE) {
        addChannelRange(grid, moduleIndex);
        grid.nextLine();
      }

      // PPM modules
      if (isPPM(moduleIndex)) {
        // PPM frame
        new StaticText(this, grid.getLabelSlot(true), STR_PPMFRAME);

        // PPM frame length
        auto edit = new NumberEdit(this, grid.getFieldSlot(2, 0), 125, 35 * 5 + 225,
                                   GET_DEFAULT(g_model.moduleData[moduleIndex].ppm.frameLength * 5 + 225),
                                   SET_VALUE(g_model.moduleData[moduleIndex].ppm.frameLength, (newValue - 225) / 5),
                                   PREC1);
        edit->setStep(5);
        edit->setSuffix(STR_MS);

        // PPM frame delay
        edit = new NumberEdit(this, grid.getFieldSlot(2, 1), 100, 800,
                              GET_DEFAULT(g_model.moduleData[moduleIndex].ppm.delay * 50 + 300),
                              SET_VALUE(g_model.moduleData[moduleIndex].ppm.delay, (newValue - 300) / 50));
        edit->setStep(50);
        edit->setSuffix("us");
        grid.nextLine();
      }

      // Receiver number
      if (isModuleNeedingReceiverNumber(moduleIndex)) {
        new StaticText(this, grid.getLabelSlot(true), STR_RECEIVER_NUM);
        new NumberEdit(this, grid.getFieldSlot(2, 0), 0, MAX_RX_NUM(moduleIndex), GET_SET_DEFAULT(g_model.header.modelId[moduleIndex]));
        grid.nextLine();
      }

      // Module parameters
      if (isModuleFlysky(moduleIndex)) {
        new StaticText(this, grid.getLabelSlot(true), STR_RXFREQUENCY);
        new NumberEdit(this, grid.getFieldSlot(), 50, 400,
                       GET_DEFAULT(g_model.moduleData[moduleIndex].romData.rx_freq[0] +
                                   g_model.moduleData[moduleIndex].romData.rx_freq[1] * 256),
                       [=](int32_t newValue) -> void {
                         g_model.moduleData[moduleIndex].romData.rx_freq[0] = newValue & 0xFF;
                         g_model.moduleData[moduleIndex].romData.rx_freq[1] = newValue >> 8;
                         SET_DIRTY();
                         setModuleFlag(moduleIndex, MODULE_MODE_NORMAL);
                         setFlyskyState(STATE_SET_RX_FREQUENCY);
                       });
        grid.nextLine();
      }
      if (isModuleAFHDS3(moduleIndex)) {
             new StaticText(this, grid.getLabelSlot(true), STR_RXFREQUENCY);
             new NumberEdit(this, grid.getFieldSlot(), 50, 400, GET_SET_DEFAULT(g_model.moduleData[moduleIndex].afhds3.rxFreq));
             grid.nextLine();
           }

      // Failsafe
      if (isModuleNeedingFailsafeButton(moduleIndex)) {
        new StaticText(this, grid.getLabelSlot(true), STR_FAILSAFE);
        failSafeChoice = new Choice(this, grid.getFieldSlot(2, 0), STR_VFAILSAFE, 0, FAILSAFE_LAST,
                                    GET_DEFAULT(g_model.moduleData[moduleIndex].failsafeMode),
                                    [=](int32_t newValue) {
                                      g_model.moduleData[moduleIndex].failsafeMode = newValue;
                                      SET_DIRTY();
                                      update();
                                      failSafeChoice->setFocus();
                                      setModuleFlag(moduleIndex, MODULE_MODE_NORMAL);
                                      SEND_FAILSAFE_NOW(moduleIndex);
                                    });
        failSafeChoice->setAvailableHandler([=](int8_t newValue) {
          if(isModuleFlysky(moduleIndex) || isModuleAFHDS3(moduleIndex)){
            failSafeChoice->setAvailableHandler([=](int8_t newValue) {
                return newValue != FAILSAFE_RECEIVER;
            });
          }
          return true;
        });
        if (g_model.moduleData[moduleIndex].failsafeMode == FAILSAFE_CUSTOM) {
          failSafeSetButton = new TextButton(this, grid.getFieldSlot(2, 1), STR_SET,
                         [=]() -> uint8_t {
                           new FailSafeMenu(moduleIndex);
                           return 1;
                         });
        }
        grid.nextLine();
      }

      // Bind and Range buttons
      if (isModuleNeedingBindRangeButtons(moduleIndex)) {
        bindButton = new TextButton(this, grid.getFieldSlot(2, 0), STR_4BIND(STR_MODULE_BIND));
        bindButton->setPressHandler([=]() -> uint8_t {
          if (moduleState[moduleIndex].mode == MODULE_MODE_RANGECHECK) {
            rangeButton->check(false);
          }
          if (moduleState[moduleIndex].mode == MODULE_MODE_BIND) {
            bindButton->setText(STR_MODULE_BIND);
            setModuleFlag(moduleIndex, MODULE_MODE_NORMAL);
            if (isModuleFlysky(moduleIndex)) resetPulsesAFHDS2();
            return 0;
          }
          else {

            if (isModuleR9M(moduleIndex) || (isModuleXJT(moduleIndex) && g_model.moduleData[moduleIndex].subType == RF_PROTO_X16)) {
                  Menu * menu = new Menu();
                  //use global handler
                  menu->setSelectHandler([=](const char* selected) {
                    onBindMenu(selected, moduleIndex);
                    bindButton->setText(STR_MODULE_BINDING);
                    setModuleFlag(moduleIndex, MODULE_MODE_BIND);
                  });
                  if (!(IS_TELEMETRY_INTERNAL_MODULE() && moduleIndex == EXTERNAL_MODULE)) {
                    menu->addLine(STR_BINDING_1_8_TELEM_ON);
                  }
                  menu->addLine(STR_BINDING_1_8_TELEM_OFF);
                  if (!(IS_TELEMETRY_INTERNAL_MODULE() && moduleIndex == EXTERNAL_MODULE)) {
                    menu->addLine(STR_BINDING_9_16_TELEM_ON);
                  }
                  menu->addLine(STR_BINDING_9_16_TELEM_OFF);
                  return 1;
            }
            else if(isModuleAFHDS3(moduleIndex)) {
              Menu * menu = new Menu();
              menu->setSelectHandler([=](const char* selected) {
                g_model.moduleData[moduleIndex].afhds3.telemetry = strcmp(selected, STR_AFHDS3_ONE_TO_ONE_TELEMETRY) == 0;
                bindButton->setText(STR_MODULE_BINDING);
                setModuleFlag(moduleIndex, MODULE_MODE_BIND);
              });
              menu->addLine(STR_AFHDS3_ONE_TO_ONE_TELEMETRY);
              menu->addLine(STR_AFHDS3_ONE_TO_MANY);
            }
            else {
              bindButton->setText(STR_MODULE_BINDING);
              setModuleFlag(moduleIndex, MODULE_MODE_BIND);
#if defined(MULTIMODULE)
              //why not use moduleFlag directly
              setMultiBindStatus(moduleIndex, MULTI_BIND_INITIATED);
#endif
            }
            if (isModuleFlysky(moduleIndex)) {
              resetPulsesAFHDS2();
              setFlyskyState(STATE_INIT);
            }
            return 1;
            }
        });
        bindButton->setCheckHandler([=]() {
#if defined(MULTIMODULE)
          if (getMultiBindStatus(moduleIndex)  == MULTI_BIND_FINISHED) {
            setMultiBindStatus(moduleIndex, MULTI_NORMAL_OPERATION);
            setModuleFlag(moduleIndex, MODULE_MODE_NORMAL);
          }
#endif
          if (moduleState[moduleIndex].mode != MODULE_MODE_BIND) {
            bindButton->setText(STR_MODULE_BIND);
            bindButton->check(false);
          }
        });
      }
      if(isModuleNeedingRangeButtons(moduleIndex)) {
        rangeButton = new TextButton(this, grid.getFieldSlot(2, 1), STR_MODULE_RANGE);
        rangeButton->setPressHandler([=]() -> uint8_t {
          if (moduleState[moduleIndex].mode == MODULE_MODE_BIND) {
            bindButton->setText(STR_MODULE_BIND);
            bindButton->check(false);
          }
          if (moduleState[moduleIndex].mode != MODULE_MODE_RANGECHECK) {
            setModuleFlag(moduleIndex, MODULE_MODE_RANGECHECK);
            if(isModuleFlysky(moduleIndex)) resetPulsesAFHDS2();
            MessageBox* mb = new MessageBox(WARNING_TYPE_INFO, DialogResult::Cancel, "Range check", "",
              [=](DialogResult result) {
                setModuleFlag(moduleIndex, MODULE_MODE_NORMAL);
                rangeButton->check(false);
              }
            );

            char* messageBuffer = new char[32];
            uint8_t lastTelemetry = 255;
            mb->setUpdateMethod([=](){
              if(lastTelemetry != TELEMETRY_RSSI()) {
                sprintf(messageBuffer, "RSSI: %d", TELEMETRY_RSSI());
                mb->setMessage(std::string(messageBuffer));
              }
            });
            return 1;
          }
          else {
            setModuleFlag(moduleIndex, MODULE_MODE_NORMAL);
            return 0;
          }
        });
        rangeButton->setCheckHandler([=]() {
          if (moduleState[moduleIndex].mode != MODULE_MODE_RANGECHECK) {
            rangeButton->check(false);
          }
        });
      }
      if(isModuleNeedingBindRangeButtons(moduleIndex) || isModuleNeedingRangeButtons(moduleIndex)){
        grid.nextLine();
      }

      // R9M Power
      if(isModuleR9MNonAccess(moduleIndex)) {
        if (isModuleR9M_FCC(moduleIndex)) {
          new StaticText(this, grid.getLabelSlot(true), STR_MULTI_RFPOWER);
          new Choice(this, grid.getFieldSlot(), STR_R9M_FCC_POWER_VALUES, 0, R9M_FCC_POWER_MAX,
                   GET_SET_DEFAULT(g_model.moduleData[moduleIndex].pxx.power));
        }
        else {
          new StaticText(this, grid.getLabelSlot(true), STR_MULTI_RFPOWER);
          new Choice(this, grid.getFieldSlot(), STR_R9M_LBT_POWER_VALUES, 0, R9M_LBT_POWER_MAX,
                   GET_DEFAULT(min<uint8_t>(g_model.moduleData[moduleIndex].pxx.power, R9M_LBT_POWER_MAX)),
                   SET_DEFAULT(g_model.moduleData[moduleIndex].pxx.power));
        }
      }
      if (isModuleFlysky(moduleIndex)) {
        new StaticText(this, grid.getLabelSlot(true), STR_MULTI_RFPOWER);
        new Choice(this, grid.getFieldSlot(), STR_RFPOWER_AFHDS2, 0, 1,
            GET_DEFAULT(g_model.moduleData[moduleIndex].romData.rfPower),
            [=](int32_t newValue) -> void {
          g_model.moduleData[moduleIndex].romData.rfPower = newValue;
          SET_DIRTY();
          onFlySkyModuleSetPower(true);
        });
      }
      if (isModuleAFHDS3(moduleIndex)) {
        new StaticText(this, grid.getLabelSlot(true), STR_MULTI_RFPOWER);
        new Choice(this, grid.getFieldSlot(), STR_RFPOWER_AFHDS3, 0, 4, GET_SET_DEFAULT(g_model.moduleData[moduleIndex].afhds3.runPower));
        grid.nextLine();
        new StaticText(this, grid.getLabelSlot(true), STR_ACTUAL_RF_POWER);
        auto actualRFPower = new Choice(this, grid.getFieldSlot(), STR_RFPOWER_AFHDS3, 0, 4, [=] { return afhds3uart.actualRunPower(); }, [=](int32_t newValue) { } );
        actualRFPower->setReadOnly(true);
        grid.nextLine();
      }
#if defined (CROSSFIRE_NATIVE)
      if(isModuleCrossfire(moduleIndex)){
          new TextButton(this, grid.getFieldSlot(), STR_SETUP, [=]() -> uint8_t {
              new CrossfireMenu();
              return 1;
          });
      }
#endif
#if defined (GHOST)
      if(isModuleGhost(moduleIndex)){
          new TextButton(this, grid.getFieldSlot(), STR_SETUP, [=]() -> uint8_t {
              new GhostMenu();
              return 1;
          });
      }
#endif

      getParent()->moveWindowsTop(top(), adjustHeight());
    }
};

ModelSetupPage::ModelSetupPage() :
  PageTab(STR_MENUSETUP, ICON_MODEL_SETUP)
{
}

uint8_t g_moduleIndex;

void onBindMenu(const char * result, uint8_t moduleIndex)
{
  if (strcmp(result, STR_BINDING_1_8_TELEM_ON) == 0) {
    g_model.moduleData[moduleIndex].pxx.receiverTelemetryOff = false;
    g_model.moduleData[moduleIndex].pxx.receiverHigherChannels = false;
  }
  else if (strcmp(result, STR_BINDING_1_8_TELEM_OFF) == 0) {
    g_model.moduleData[moduleIndex].pxx.receiverTelemetryOff = true;
    g_model.moduleData[moduleIndex].pxx.receiverHigherChannels = false;
  }
  else if (strcmp(result, STR_BINDING_9_16_TELEM_ON) == 0) {
    g_model.moduleData[moduleIndex].pxx.receiverTelemetryOff = false;
    g_model.moduleData[moduleIndex].pxx.receiverHigherChannels = true;
  }
  else if (strcmp(result, STR_BINDING_9_16_TELEM_OFF) == 0) {
    g_model.moduleData[moduleIndex].pxx.receiverTelemetryOff = true;
    g_model.moduleData[moduleIndex].pxx.receiverHigherChannels = true;
  }
  else {
    return;
  }

  moduleState[moduleIndex].mode = MODULE_MODE_BIND;
}
void ModelSetupPage::build(Window * window)
{
  GridLayout grid;
  grid.spacer(8);

  // Model name
  new StaticText(window, grid.getLabelSlot(), STR_MODELNAME);
  new TextEdit(window, grid.getFieldSlot(), g_model.header.name, sizeof(g_model.header.name));
  grid.nextLine();

  // Bitmap
  new StaticText(window, grid.getLabelSlot(), STR_BITMAP);
  new FileChoice(window, grid.getFieldSlot(), BITMAPS_PATH, BITMAPS_EXT, sizeof(g_model.header.bitmap),
                 [=]() {
                   return std::string(g_model.header.bitmap, ZLEN(g_model.header.bitmap));
                 },
                 [=](std::string newValue) {
                   strncpy(g_model.header.bitmap, newValue.c_str(), sizeof(g_model.header.bitmap));
                   SET_DIRTY();
                 }, false);
  grid.nextLine();

  for (uint8_t i = 0; i < TIMERS; i++) {
    TimerData * timer = &g_model.timers[i];

    // Timer label
    char timerLabel[8];
    strAppendStringWithIndex(timerLabel, STR_TIMER, i + 1);
    new Subtitle(window, grid.getLineSlot(), timerLabel);
    grid.nextLine();

    // Timer name
    new StaticText(window, grid.getLabelSlot(true), STR_TIMER_NAME);
    new TextEdit(window, grid.getFieldSlot(), timer->name, LEN_TIMER_NAME);
    grid.nextLine();

    // Timer mode
    new StaticText(window, grid.getLabelSlot(true), STR_MODE);
    new SwitchChoice(window, grid.getFieldSlot(2, 0), SWSRC_FIRST, SWSRC_LAST, GET_SET_DEFAULT(timer->swtch));
    new Choice(window, grid.getFieldSlot(2, 1), "\006""Simple""Thr.\0 ""Thr.%", 0, TMRMODE_COUNT, GET_SET_DEFAULT(timer->mode));
    grid.nextLine();

    // Timer start value
    new StaticText(window, grid.getLabelSlot(true), "Start");
    new TimeEdit(window, grid.getFieldSlot(), 0, TIMER_MAX, GET_SET_DEFAULT(timer->start));
    grid.nextLine();

    // Timer minute beep
    new StaticText(window, grid.getLabelSlot(true), STR_MINUTEBEEP);
    new CheckBox(window, grid.getFieldSlot(), GET_SET_DEFAULT(timer->minuteBeep));
    grid.nextLine();

    // Timer countdown
    new StaticText(window, grid.getLabelSlot(true), STR_BEEPCOUNTDOWN);
    new Choice(window, grid.getFieldSlot(2, 0), STR_VBEEPCOUNTDOWN, COUNTDOWN_SILENT, COUNTDOWN_COUNT - 1, GET_SET_DEFAULT(timer->countdownBeep));
    new Choice(window, grid.getFieldSlot(2, 1), STR_COUNTDOWNVALUES, 0, 3, GET_SET_WITH_OFFSET(timer->countdownStart, 2));
    grid.nextLine();

    // Timer persistent
    new StaticText(window, grid.getLabelSlot(true), STR_PERSISTENT);
    new Choice(window, grid.getFieldSlot(), STR_VPERSISTENT, 0, 2, GET_SET_DEFAULT(timer->persistent));
    grid.nextLine();
  }

  // Extended limits
  new StaticText(window, grid.getLabelSlot(), STR_ELIMITS);
  new CheckBox(window, grid.getFieldSlot(), GET_SET_DEFAULT(g_model.extendedLimits));
  grid.nextLine();

  // Extended trims
  new StaticText(window, grid.getLabelSlot(), STR_ETRIMS);
  new CheckBox(window, grid.getFieldSlot(2, 0), GET_SET_DEFAULT(g_model.extendedTrims));
  new TextButton(window, grid.getFieldSlot(2, 1), STR_RESET_BTN,
                 []() -> uint8_t {
                   START_NO_HIGHLIGHT();
                   for (auto &flightModeData : g_model.flightModeData) {
                     memclear(&flightModeData, TRIMS_ARRAY_SIZE);
                   }
                   storageDirty(EE_MODEL);
                   AUDIO_WARNING1();
                   return 0;
                 });
  grid.nextLine();

  // Display trims
  new StaticText(window, grid.getLabelSlot(), STR_DISPLAY_TRIMS);
  new Choice(window, grid.getFieldSlot(), "\006No\0   ChangeYes", 0, 2, GET_SET_DEFAULT(g_model.displayTrims));
  grid.nextLine();

  // Trim step
  new StaticText(window, grid.getLabelSlot(), STR_TRIMINC);
  new Choice(window, grid.getFieldSlot(), STR_VTRIMINC, -2, 2, GET_SET_DEFAULT(g_model.trimInc));
  grid.nextLine();

  // Throttle parameters
  {
    new Subtitle(window, grid.getLineSlot(), STR_THROTTLE_LABEL);
    grid.nextLine();

    // Throttle reversed
    new StaticText(window, grid.getLabelSlot(true), STR_THROTTLEREVERSE);
    new CheckBox(window, grid.getFieldSlot(), GET_SET_DEFAULT(g_model.throttleReversed));
    grid.nextLine();

    // Throttle source
    new StaticText(window, grid.getLabelSlot(true), STR_TTRACE);
    new SourceChoice(window, grid.getFieldSlot(), 0, MIXSRC_LAST_CH, GET_SET_DEFAULT(g_model.thrTraceSrc));
    grid.nextLine();

    // Throttle trim
    new StaticText(window, grid.getLabelSlot(true), STR_TTRIM);
    new CheckBox(window, grid.getFieldSlot(), GET_SET_DEFAULT(g_model.thrTrim));
    grid.nextLine();
  }

  // Preflight parameters
  {
    new Subtitle(window, grid.getLineSlot(), STR_PREFLIGHT);
    grid.nextLine();

    // Display checklist
    new StaticText(window, grid.getLabelSlot(true), STR_CHECKLIST);
    new CheckBox(window, grid.getFieldSlot(), GET_SET_DEFAULT(g_model.displayChecklist));
    grid.nextLine();

    // Throttle warning
    new StaticText(window, grid.getLabelSlot(true), STR_THROTTLEWARNING);
    new CheckBox(window, grid.getFieldSlot(), GET_SET_INVERTED(g_model.disableThrottleWarning));
    grid.nextLine();

    // Switches warning
    new StaticText(window, grid.getLabelSlot(true), STR_SWITCHWARNING);
    for (int i = 0; i < NUM_SWITCHES; i++) {
      char s[3];
      if (i > 0 && (i % 3) == 0)
        grid.nextLine();

      auto button = new TextButton(window, grid.getFieldSlot(3, i % 3), getSwitchWarningString(s, i), nullptr,
                                   (BF_GET(g_model.switchWarningState, 3 * i, 3) == 0 ? 0 : BUTTON_CHECKED));
      button->setPressHandler([button, i] {
        uint8_t newstate = BF_GET(g_model.switchWarningState, 3 * i, 3);
        if (newstate == 1 && SWITCH_CONFIG(i) != SWITCH_3POS)
          newstate = 3;
        else
          newstate = (newstate + 1) % 4;
        BF_SET(g_model.switchWarningState, newstate, 3 * i, 3);
        SET_DIRTY();
        button->setText(getSwitchWarningString(i));
        return newstate > 0;
      });
    }
    grid.nextLine();
  }

  // Center beeps
  {
    new StaticText(window, grid.getLabelSlot(), STR_BEEPCTR);
    for (int i = 0; i < NUM_STICKS + NUM_POTS + NUM_SLIDERS; i++) {
      char s[2];
      if (i > 0 && (i % 3) == 0)
        grid.nextLine();

      new TextButton(window, grid.getFieldSlot(3, i % 3), getStringAtIndex(s, STR_RETA123, i),
                     [=]() -> uint8_t {
                       BF_BIT_FLIP(g_model.beepANACenter, BF_BIT(i));
                       SET_DIRTY();
                       return BF_SINGLE_BIT_GET(g_model.beepANACenter, i);
                     },
                     BF_SINGLE_BIT_GET(g_model.beepANACenter, i) ? BUTTON_CHECKED : 0);
    }
    grid.nextLine();
  }

  // Global functions
  {
    new StaticText(window, grid.getLabelSlot(), STR_USE_GLOBAL_FUNCS);
    new CheckBox(window, grid.getFieldSlot(), GET_SET_INVERTED(g_model.noGlobalFunctions));
    grid.nextLine();
  }

  // Internal module
  {
    new Subtitle(window, grid.getLineSlot(), STR_INTERNALRF);
    grid.nextLine();
    grid.addWindow(new ModuleWindow(window, {0, grid.getWindowHeight(), LCD_W, 0}, INTERNAL_MODULE));
  }

  // External module
  {
    new Subtitle(window, grid.getLineSlot(), STR_EXTERNALRF);
    grid.nextLine();
    grid.addWindow(new ModuleWindow(window, {0, grid.getWindowHeight(), LCD_W, 0}, EXTERNAL_MODULE));
  }

  // Trainer Mode
#if defined(TRAINERMODULE)
  {
    new Subtitle(window, grid.getLineSlot(), STR_TRAINER);
    grid.nextLine();
    grid.addWindow(new ModuleWindow(window, {0, grid.getWindowHeight(), LCD_W, 0}, TRAINER_MODULE));
  }
#endif

  grid.nextLine();

  window->setInnerHeight(grid.getWindowHeight());
}

// Switch to external antenna confirmation
//  bool newAntennaSel;
//  if (warningResult) {
//    warningResult = 0;
//    g_model.moduleData[INTERNAL_MODULE].pxx.external_antenna = XJT_EXTERNAL_ANTENNA;
//  }

#if 0

case ITEM_MODEL_SLIDPOT_WARNING_STATE:
  drawText(window,MENUS_MARGIN_LEFT, y,STR_POTWARNINGSTATE);
  lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, PSTR("\004""OFF\0""Man\0""Auto"), g_model.potsWarnMode, attr);
  if (attr) {
    CHECK_INCDEC_MODELVAR(event, g_model.potsWarnMode, POTS_WARN_OFF, POTS_WARN_AUTO);
    storageDirty(EE_MODEL);
  }
  break;

case ITEM_MODEL_POTS_WARNING:
{
  drawText(window,MENUS_MARGIN_LEFT, y, STR_POTWARNING);
  if (attr) {
    if (!READ_ONLY() && menuHorizontalPosition >= 0 && event==EVT_KEY_LONG(KEY_ENTER)) {
      killEvents(event);
      if (g_model.potsWarnMode == POTS_WARN_MANUAL) {
        SAVE_POT_POSITION(menuHorizontalPosition);
        AUDIO_WARNING1();
        storageDirty(EE_MODEL);
      }
    }

    if (!READ_ONLY() &&  menuHorizontalPosition >= 0 && s_editMode && event==EVT_KEY_BREAK(KEY_ENTER)) {
      s_editMode = 0;
      g_model.potsWarnEnabled ^= (1 << (menuHorizontalPosition));
      storageDirty(EE_MODEL);
    }
  }

  if (attr && menuHorizontalPosition < 0) {
    lcdDrawSolidFilledRect(MODEL_SETUP_2ND_COLUMN-INVERT_HORZ_MARGIN, y-INVERT_VERT_MARGIN+1, NUM_POTS*MODEL_SETUP_SLIDPOT_SPACING+INVERT_HORZ_MARGIN, INVERT_LINE_HEIGHT, TEXT_INVERTED_BGCOLOR);
  }

  if (g_model.potsWarnMode) {
    coord_t x = MODEL_SETUP_2ND_COLUMN;
    for (int i=0; i<NUM_POTS; ++i) {
      LcdFlags flags = (((menuHorizontalPosition==i) && attr) ? INVERS : 0);
      flags |= (g_model.potsWarnEnabled & (1 << i)) ? TEXT_DISABLE_COLOR : TEXT_COLOR;
      if (attr && menuHorizontalPosition < 0) {
        flags |= INVERS;
      }
      lcdDrawTextAtIndex(x, y, STR_VSRCRAW, NUM_STICKS+1+i, flags);
      x += MODEL_SETUP_SLIDPOT_SPACING;
    }
  }
  break;
}

case ITEM_MODEL_SLIDERS_WARNING:
  drawText(window,MENUS_MARGIN_LEFT, y, STR_SLIDERWARNING);
  if (attr) {
    if (!READ_ONLY() && menuHorizontalPosition+1 && event==EVT_KEY_LONG(KEY_ENTER)) {
      killEvents(event);
      if (g_model.potsWarnMode == POTS_WARN_MANUAL) {
        SAVE_POT_POSITION(menuHorizontalPosition+NUM_POTS);
        AUDIO_WARNING1();
        storageDirty(EE_MODEL);
      }
    }

    if (!READ_ONLY() && menuHorizontalPosition+1 && s_editMode && event==EVT_KEY_BREAK(KEY_ENTER)) {
      s_editMode = 0;
      g_model.potsWarnEnabled ^= (1 << (menuHorizontalPosition+NUM_POTS));
      storageDirty(EE_MODEL);
    }
  }

  if (attr && menuHorizontalPosition < 0) {
    lcdDrawSolidFilledRect(MODEL_SETUP_2ND_COLUMN-INVERT_HORZ_MARGIN, y-INVERT_VERT_MARGIN+1, NUM_SLIDERS*MODEL_SETUP_SLIDPOT_SPACING+INVERT_HORZ_MARGIN, INVERT_LINE_HEIGHT, TEXT_INVERTED_BGCOLOR);
  }

  if (g_model.potsWarnMode) {
    coord_t x = MODEL_SETUP_2ND_COLUMN;
    for (int i=NUM_POTS; i<NUM_POTS+NUM_SLIDERS; ++i) {
      LcdFlags flags = (((menuHorizontalPosition==i-NUM_POTS) && attr) ? INVERS : 0);
      flags |= (g_model.potsWarnEnabled & (1 << i)) ? TEXT_DISABLE_COLOR : TEXT_COLOR;
      if (attr && menuHorizontalPosition < 0) {
        flags |= INVERS;
      }
      lcdDrawTextAtIndex(x, y, STR_VSRCRAW, NUM_STICKS+1+i, flags);
      x += MODEL_SETUP_SLIDPOT_SPACING;
    }
  }
  break;

case ITEM_MODEL_USE_GLOBAL_FUNCTIONS:
  drawText(window,MENUS_MARGIN_LEFT, y, STR_USE_GLOBAL_FUNCS);
  drawCheckBox(MODEL_SETUP_2ND_COLUMN, y, !g_model.noGlobalFunctions, attr);
  if (attr) g_model.noGlobalFunctions = !checkIncDecModel(event, !g_model.noGlobalFunctions, 0, 1);
  break;

case ITEM_MODEL_INTERNAL_MODULE_LABEL:
  drawText(window,MENUS_MARGIN_LEFT, y, TR_INTERNALRF);
  break;

case ITEM_MODEL_INTERNAL_MODULE_MODE:
  drawText(window,MENUS_MARGIN_LEFT, y, STR_MODE);
  lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_XJT_PROTOCOLS, 1+g_model.moduleData[0].rfProtocol, attr);
  if (attr) {
    g_model.moduleData[INTERNAL_MODULE].rfProtocol = checkIncDec(event, g_model.moduleData[INTERNAL_MODULE].rfProtocol, RF_PROTO_OFF, RF_PROTO_LAST, EE_MODEL, isRfProtocolAvailable);
    if (checkIncDec_Ret) {
      g_model.moduleData[0].type = MODULE_TYPE_XJT_PXX1;
      g_model.moduleData[0].channelsStart = 0;
      g_model.moduleData[0].channelsCount = DEFAULT_CHANNELS(INTERNAL_MODULE);
      if (g_model.moduleData[INTERNAL_MODULE].rfProtocol == RF_PROTO_OFF)
        g_model.moduleData[INTERNAL_MODULE].type = MODULE_TYPE_NONE;
    }

  }
  break;

case ITEM_MODEL_INTERNAL_MODULE_ANTENNA:
  CHOICE(MENUS_MARGIN_LEFT + INDENT_WIDTH, y, STR_ANTENNASELECTION, STR_VANTENNATYPES, g_model.moduleData[INTERNAL_MODULE].pxx.external_antenna, 0, 1);
#if 0
  if (newAntennaSel != g_model.moduleData[INTERNAL_MODULE].pxx.external_antenna && newAntennaSel == XJT_EXTERNAL_ANTENNA) {
    POPUP_CONFIRMATION(STR_ANTENNACONFIRM1);
    const char * w = STR_ANTENNACONFIRM2;
    SET_WARNING_INFO(w, strlen(w), 0);
  }
  else {
    g_model.moduleData[INTERNAL_MODULE].pxx.external_antenna = newAntennaSel;
  }
#endif
  break;

case ITEM_MODEL_TRAINER_MODE:
  CHOICE(MENUS_MARGIN_LEFT + INDENT_WIDTH, y, STR_MODE, STR_VTRAINERMODES, g_model.trainerMode, 0, TRAINER_MODE_MAX());
#if defined(BLUETOOTH)
  if (attr && checkIncDec_Ret) {
    bluetoothState = BLUETOOTH_STATE_OFF;
    bluetoothDistantAddr[0] = 0;
  }
#endif
  break;

case ITEM_MODEL_EXTERNAL_MODULE_LABEL:
  drawText(window,MENUS_MARGIN_LEFT, y, TR_EXTERNALRF);
  break;

case ITEM_MODEL_EXTERNAL_MODULE_MODE:
  drawText(window,MENUS_MARGIN_LEFT, y, STR_MODE);
  lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_MODULE_PROTOCOLS, g_model.moduleData[EXTERNAL_MODULE].type, menuHorizontalPosition==0 ? attr : 0);
  if (isModuleXJT(EXTERNAL_MODULE))
    lcdDrawTextAtIndex(MODEL_SETUP_3RD_COLUMN, y, STR_XJT_PROTOCOLS, 1+g_model.moduleData[EXTERNAL_MODULE].rfProtocol, (menuHorizontalPosition==1 ? attr : 0));
  else if (isModuleDSM2(EXTERNAL_MODULE))
    lcdDrawTextAtIndex(MODEL_SETUP_3RD_COLUMN, y, STR_DSM_PROTOCOLS, g_model.moduleData[EXTERNAL_MODULE].rfProtocol, (menuHorizontalPosition==1 ? attr : 0));
  else if (isModuleR9M(EXTERNAL_MODULE))
    lcdDrawTextAtIndex(MODEL_SETUP_3RD_COLUMN, y, STR_R9M_MODES, g_model.moduleData[EXTERNAL_MODULE].subType, (menuHorizontalPosition==1 ? attr : 0));
#if defined(MULTIMODULE)
  else if (isModuleMultimodule(EXTERNAL_MODULE)) {
    int multi_rfProto = g_model.moduleData[EXTERNAL_MODULE].getMultiProtocol();
    if (g_model.moduleData[EXTERNAL_MODULE].multi.customProto) {
      drawText(window,MODEL_SETUP_3RD_COLUMN, y, STR_MULTI_CUSTOM, menuHorizontalPosition == 1 ? attr : 0);
      lcdDrawNumber(MODEL_SETUP_4TH_COLUMN, y, multi_rfProto, menuHorizontalPosition==2 ? attr : 0, 2);
      lcdDrawNumber(MODEL_SETUP_4TH_COLUMN + MODEL_SETUP_BIND_OFS, y, g_model.moduleData[EXTERNAL_MODULE].subType, menuHorizontalPosition==3 ? attr : 0, 2);
    }
    else {
      const mm_protocol_definition * pdef = getMultiProtocolDefinition(multi_rfProto);
      lcdDrawTextAtIndex(MODEL_SETUP_3RD_COLUMN, y, STR_MULTI_PROTOCOLS, multi_rfProto, menuHorizontalPosition == 1 ? attr : 0);
      if (pdef->subTypeString != nullptr)
        lcdDrawTextAtIndex(MODEL_SETUP_4TH_COLUMN, y, pdef->subTypeString, g_model.moduleData[EXTERNAL_MODULE].subType, menuHorizontalPosition==2 ? attr : 0);
    }
  }
#endif
  if (attr && s_editMode>0) {
    switch (menuHorizontalPosition) {
      case 0:
        g_model.moduleData[EXTERNAL_MODULE].type = checkIncDec(event, g_model.moduleData[EXTERNAL_MODULE].type, MODULE_TYPE_NONE, MODULE_TYPE_COUNT-1, EE_MODEL, isModuleAvailable);
        if (checkIncDec_Ret) {
          g_model.moduleData[EXTERNAL_MODULE].rfProtocol = 0;
          g_model.moduleData[EXTERNAL_MODULE].channelsStart = 0;
          g_model.moduleData[EXTERNAL_MODULE].channelsCount = DEFAULT_CHANNELS(EXTERNAL_MODULE);
          if (IS_MODULE_SBUS(EXTERNAL_MODULE))
            g_model.moduleData[EXTERNAL_MODULE].sbus.refreshRate = -31;
        }
        break;
      case 1:
        if (isModuleDSM2(EXTERNAL_MODULE))
          CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].rfProtocol, DSM2_PROTO_LP45, DSM2_PROTO_DSMX);
        else if (isModuleR9M(EXTERNAL_MODULE))
          CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].subType, MODULE_SUBTYPE_R9M_FCC, MODULE_SUBTYPE_R9M_LAST);
#if defined(MULTIMODULE)
        else if (isModuleMultimodule(EXTERNAL_MODULE)) {
          int multiRfProto = g_model.moduleData[EXTERNAL_MODULE].multi.customProto == 1 ? MM_RF_CUSTOM_SELECTED : g_model.moduleData[EXTERNAL_MODULE].getMultiProtocol();
          CHECK_INCDEC_MODELVAR(event, multiRfProto, MODULE_SUBTYPE_MULTI_FIRST, MODULE_SUBTYPE_MULTI_LAST);
          if (checkIncDec_Ret) {
            g_model.moduleData[EXTERNAL_MODULE].multi.customProto = (multiRfProto == MM_RF_CUSTOM_SELECTED);
            if (!g_model.moduleData[EXTERNAL_MODULE].multi.customProto)
              g_model.moduleData[EXTERNAL_MODULE].setMultiProtocol(multiRfProto);
            g_model.moduleData[EXTERNAL_MODULE].subType = 0;
            // Sensible default for DSM2 (same as for ppm): 7ch@22ms + Autodetect settings enabled
            if (g_model.moduleData[EXTERNAL_MODULE].getMultiProtocol() == MODULE_SUBTYPE_MULTI_DSM2) {
              g_model.moduleData[EXTERNAL_MODULE].multi.autoBindMode = 1;
            }
            else {
              g_model.moduleData[EXTERNAL_MODULE].multi.autoBindMode = 0;
            }
            g_model.moduleData[EXTERNAL_MODULE].multi.optionValue = 0;
          }
        }
#endif
        else {
          CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].rfProtocol, RF_PROTO_X16, RF_PROTO_LAST);
        }
        if (checkIncDec_Ret) {
          g_model.moduleData[EXTERNAL_MODULE].channelsStart = 0;
          g_model.moduleData[EXTERNAL_MODULE].channelsCount = DEFAULT_CHANNELS(EXTERNAL_MODULE);
        }
        break;
#if defined(MULTIMODULE)
      case 2:
        if (g_model.moduleData[EXTERNAL_MODULE].multi.customProto) {
          g_model.moduleData[EXTERNAL_MODULE].setMultiProtocol(checkIncDec(event, g_model.moduleData[EXTERNAL_MODULE].getMultiProtocol(), 0, 63, EE_MODEL));
          break;
        } else {
          const mm_protocol_definition *pdef = getMultiProtocolDefinition(g_model.moduleData[EXTERNAL_MODULE].getMultiProtocol());
          if (pdef->maxSubtype > 0)
            CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].subType, 0, pdef->maxSubtype);
        }
        break;
      case 3:
        // Custom protocol, third column is subtype
        CHECK_INCDEC_MODELVAR(event, g_model.moduleData[EXTERNAL_MODULE].subType, 0, 7);
        break;
#endif
    }
  }
  break;

case ITEM_MODEL_TRAINER_LABEL:
  drawText(window,MENUS_MARGIN_LEFT, y, STR_TRAINER);
  break;

case ITEM_MODEL_TRAINER_LINE1:
#if defined(BLUETOOTH)
  if (g_model.trainerMode == TRAINER_MODE_MASTER_BLUETOOTH) {
    if (attr) {
      s_editMode = 0;
    }
    if (bluetoothDistantAddr[0]) {
      drawText(window,MENUS_MARGIN_LEFT + INDENT_WIDTH, y, bluetoothDistantAddr);
      if (bluetoothState != BLUETOOTH_STATE_CONNECTED) {
        drawButton(MODEL_SETUP_2ND_COLUMN, y, "Bind", menuHorizontalPosition == 0 ? attr : 0);
        drawButton(MODEL_SETUP_2ND_COLUMN+60, y, "Clear", menuHorizontalPosition == 1 ? attr : 0);
      }
      else {
        drawButton(MODEL_SETUP_2ND_COLUMN, y, "Clear", attr);
      }
      if (attr && event == EVT_KEY_FIRST(KEY_ENTER)) {
        if (bluetoothState == BLUETOOTH_STATE_CONNECTED || menuHorizontalPosition == 1) {
          bluetoothState = BLUETOOTH_STATE_OFF;
          bluetoothDistantAddr[0] = 0;
        }
        else {
          bluetoothState = BLUETOOTH_STATE_BIND_REQUESTED;
        }
      }
    }
    else {
      drawText(window,MENUS_MARGIN_LEFT + INDENT_WIDTH, y, "---");
      if (bluetoothState < BLUETOOTH_STATE_IDLE)
        drawButton(MODEL_SETUP_2ND_COLUMN, y, STR_BLUETOOTH_INIT, attr);
      else
        drawButton(MODEL_SETUP_2ND_COLUMN, y, STR_BLUETOOTH_DISC, attr);
      if (attr && event == EVT_KEY_FIRST(KEY_ENTER)) {
        if (bluetoothState < BLUETOOTH_STATE_IDLE)
          bluetoothState = BLUETOOTH_STATE_OFF;
        else
          bluetoothState = BLUETOOTH_STATE_DISCOVER_REQUESTED;
      }
    }
    break;
  }
  // no break
#endif

case ITEM_MODEL_INTERNAL_MODULE_CHANNELS:
case ITEM_MODEL_EXTERNAL_MODULE_CHANNELS:
{
  uint8_t moduleIndex = CURRENT_MODULE_EDITED(i);
  ModuleData & moduleData = g_model.moduleData[moduleIndex];
  drawText(window,MENUS_MARGIN_LEFT, y, STR_CHANNELRANGE);
  if ((int8_t)PORT_CHANNELS_ROWS(moduleIndex) >= 0) {
    drawStringWithIndex(MODEL_SETUP_2ND_COLUMN, y, STR_CH, moduleData.channelsStart+1, menuHorizontalPosition==0 ? attr : 0);
    drawText(window,lcdNextPos+5, y, "-");
    drawStringWithIndex(lcdNextPos+5, y, STR_CH, moduleData.channelsStart+sentModuleChannels(moduleIndex), menuHorizontalPosition==1 ? attr : 0);
    if (IS_R9M_OR_XJTD16(moduleIndex)) {
      if (sentModuleChannels(moduleIndex) > 8)
        drawText(window,lcdNextPos + 15, y, "(18ms)");
      else
        drawText(window,lcdNextPos + 15, y, "(9ms)");
    }
    if (attr && s_editMode>0) {
      switch (menuHorizontalPosition) {
        case 0:
          CHECK_INCDEC_MODELVAR_ZERO(event, moduleData.channelsStart, 32-8-moduleData.channelsCount);
          break;
        case 1:
          CHECK_INCDEC_MODELVAR(event, moduleData.channelsCount, -4, min<int8_t>(MAX_CHANNELS_M8(moduleIndex), 32-8-moduleData.channelsStart));
          if ((i == ITEM_MODEL_EXTERNAL_MODULE_CHANNELS && g_model.moduleData[EXTERNAL_MODULE].type == MODULE_TYPE_PPM)
              || (i == ITEM_MODEL_TRAINER_LINE1)
              )
            SET_DEFAULT_PPM_FRAME_LENGTH(moduleIndex);
          break;
      }
    }
  }
  break;
}

case ITEM_MODEL_INTERNAL_MODULE_BIND:
case ITEM_MODEL_EXTERNAL_MODULE_BIND:
case ITEM_MODEL_TRAINER_LINE2:
{
  uint8_t moduleIndex = CURRENT_MODULE_EDITED(i);
  ModuleData & moduleData = g_model.moduleData[moduleIndex];
  if (isModulePPM(moduleIndex)) {
    drawText(window,MENUS_MARGIN_LEFT, y, STR_PPMFRAME);
    lcdDrawNumber(MODEL_SETUP_2ND_COLUMN, y, (int16_t)moduleData.ppm.frameLength*5 + 225, (menuHorizontalPosition<=0 ? attr : 0) | PREC1|LEFT, 0, NULL, STR_MS);
    lcdDrawNumber(MODEL_SETUP_2ND_COLUMN+80, y, (moduleData.ppm.delay*50)+300, (CURSOR_ON_LINE() || menuHorizontalPosition==1) ? attr|LEFT : LEFT, 0, NULL, "us");
    drawText(window,MODEL_SETUP_2ND_COLUMN+160, y, moduleData.ppm.pulsePol ? "+" : "-", (CURSOR_ON_LINE() || menuHorizontalPosition==2) ? attr : 0);
    if (attr && s_editMode>0) {
      switch (menuHorizontalPosition) {
        case 0:
          CHECK_INCDEC_MODELVAR(event, moduleData.ppm.frameLength, -20, 35);
          break;
        case 1:
          CHECK_INCDEC_MODELVAR(event, moduleData.ppm.delay, -4, 10);
          break;
        case 2:
          CHECK_INCDEC_MODELVAR_ZERO(event, moduleData.ppm.pulsePol, 1);
          break;
      }
    }
  }
  else if (IS_MODULE_SBUS(moduleIndex)) {
    drawText(window,MENUS_MARGIN_LEFT, y, STR_REFRESHRATE);
    lcdDrawNumber(MODEL_SETUP_2ND_COLUMN, y, (int16_t)moduleData.ppm.frameLength*5 + 225, (menuHorizontalPosition<=0 ? attr : 0) | PREC1|LEFT, 0, NULL, STR_MS);
    drawText(window,MODEL_SETUP_3RD_COLUMN, y, moduleData.sbus.noninverted ? "not inverted" : "normal", (CURSOR_ON_LINE() || menuHorizontalPosition==1) ? attr : 0);

    if (attr && s_editMode>0) {
      switch (menuHorizontalPosition) {
        case 0:
          CHECK_INCDEC_MODELVAR(event, moduleData.ppm.frameLength, -33, 35);
          break;
        case 1:
          CHECK_INCDEC_MODELVAR_ZERO(event, moduleData.sbus.noninverted, 1);
          break;
      }
    }
  }
  else {
    int l_posHorz = menuHorizontalPosition;
    coord_t xOffsetBind = MODEL_SETUP_BIND_OFS;
    if (isModuleXJT(moduleIndex) && IS_D8_RX(moduleIndex)) {
      xOffsetBind = 0;
      drawText(window,MENUS_MARGIN_LEFT, y, STR_RECEIVER);
      if (attr) l_posHorz += 1;
    }
    else {
      drawText(window,MENUS_MARGIN_LEFT, y, STR_RECEIVER_NUM);
    }
    if (isModulePXX(moduleIndex) || isModuleDSM2(moduleIndex) || isModuleMultimodule(moduleIndex)) {
      if (xOffsetBind)
        lcdDrawNumber(MODEL_SETUP_2ND_COLUMN, y, g_model.header.modelId[moduleIndex], (l_posHorz==0 ? attr : 0) | LEADING0 | LEFT, 2);
      if (attr && l_posHorz==0 && s_editMode>0)
        CHECK_INCDEC_MODELVAR_ZERO(event, g_model.header.modelId[moduleIndex], MAX_RX_NUM(moduleIndex));
      drawButton(MODEL_SETUP_2ND_COLUMN+xOffsetBind, y, STR_MODULE_BIND, (moduleState[moduleIndex].mode == MODULE_MODE_BIND ? BUTTON_ON : BUTTON_OFF) | (l_posHorz==1 ? attr : 0));
      drawButton(MODEL_SETUP_2ND_COLUMN+MODEL_SETUP_RANGE_OFS+xOffsetBind, y, STR_MODULE_RANGE, (moduleState[moduleIndex].mode == MODULE_MODE_RANGECHECK ? BUTTON_ON : BUTTON_OFF) | (l_posHorz==2 ? attr : 0));
      uint8_t newFlag = 0;
#if defined(MULTIMODULE)
      if (getMultiBindStatus(moduleIdx) == MULTI_BIND_FINISHED) {
        setMultiBindStatus(moduleIdx, MULTI_NORMAL_OPERATION);
        s_editMode = 0;
      }
#endif
      if (attr && l_posHorz>0) {
        if (s_editMode>0) {
          if (l_posHorz == 1) {
            if (isModuleR9M(moduleIndex) || (isModuleXJT(moduleIndex) && g_model.moduleData[moduleIndex].rfProtocol == RF_PROTO_X16)) {
              if (event == EVT_KEY_BREAK(KEY_ENTER)) {
                uint8_t default_selection;
                if (isModuleR9M_LBT(moduleIndex)) {
                  if (!IS_TELEMETRY_INTERNAL_MODULE())
                    POPUP_MENU_ADD_ITEM(STR_BINDING_25MW_CH1_8_TELEM_ON);
                  POPUP_MENU_ADD_ITEM(STR_BINDING_25MW_CH1_8_TELEM_OFF);
                  POPUP_MENU_ADD_ITEM(STR_BINDING_500MW_CH1_8_TELEM_OFF);
                  POPUP_MENU_ADD_ITEM(STR_BINDING_500MW_CH9_16_TELEM_OFF);
                  default_selection = 2;
                }
                else {
                  if (!(IS_TELEMETRY_INTERNAL_MODULE() && moduleIndex == EXTERNAL_MODULE))
                    POPUP_MENU_ADD_ITEM(STR_BINDING_1_8_TELEM_ON);
                  POPUP_MENU_ADD_ITEM(STR_BINDING_1_8_TELEM_OFF);
                  if (!(IS_TELEMETRY_INTERNAL_MODULE() && moduleIndex == EXTERNAL_MODULE))
                    POPUP_MENU_ADD_ITEM(STR_BINDING_9_16_TELEM_ON);
                  POPUP_MENU_ADD_ITEM(STR_BINDING_9_16_TELEM_OFF);
                  default_selection = g_model.moduleData[moduleIndex].pxx.receiverTelemetryOff + (g_model.moduleData[moduleIndex].pxx.receiverHigherChannels << 1);
                }
                POPUP_MENU_SELECT_ITEM(default_selection);
                POPUP_MENU_START(onBindMenu);
                continue;
              }
              if (moduleState[moduleIndex].mode == MODULE_MODE_BIND) {
                newFlag = MODULE_MODE_BIND;
              }
              else {
                if (!popupMenuNoItems) {
                  s_editMode = 0;  // this is when popup is exited before a choice is made
                }
              }
            }
            else {
              newFlag = MODULE_MODE_BIND;
            }
          }
          else if (l_posHorz == 2) {
            newFlag = MODULE_MODE_RANGECHECK;
          }
        }
      }
      moduleState[moduleIndex].mode = newFlag;
#if defined(MULTIMODULE)
      if (newFlag == MODULE_MODE_BIND)
        setMultiBindStatus(moduleIdx, MULTI_BIND_INITIATED);
#endif
    }
  }
  break;
}

case ITEM_MODEL_INTERNAL_MODULE_FAILSAFE:
case ITEM_MODEL_EXTERNAL_MODULE_FAILSAFE:
{
  uint8_t moduleIndex = CURRENT_MODULE_EDITED(i);
  ModuleData & moduleData = g_model.moduleData[moduleIndex];
  drawText(window,MENUS_MARGIN_LEFT, y, STR_FAILSAFE);
  lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_VFAILSAFE, moduleData.failsafeMode, menuHorizontalPosition==0 ? attr : 0);
  if (moduleData.failsafeMode == FAILSAFE_CUSTOM) {
    drawButton(MODEL_SETUP_2ND_COLUMN + MODEL_SETUP_SET_FAILSAFE_OFS, y, STR_SET, menuHorizontalPosition==1 ? attr : 0);
  }
  if (attr) {
    if (moduleData.failsafeMode != FAILSAFE_CUSTOM)
      menuHorizontalPosition = 0;
    if (menuHorizontalPosition==0) {
      if (s_editMode>0) {
        CHECK_INCDEC_MODELVAR_ZERO(event, moduleData.failsafeMode, FAILSAFE_LAST);
        if (checkIncDec_Ret) SEND_FAILSAFE_NOW(moduleIndex);
      }
    }
    else if (menuHorizontalPosition==1) {
      s_editMode = 0;
      if (moduleData.failsafeMode==FAILSAFE_CUSTOM && event==EVT_KEY_FIRST(KEY_ENTER)) {
        g_moduleIndex = moduleIndex;
        pushMenu(menuModelFailsafe);
      }
    }
    else {
      lcdDrawSolidFilledRect(MODEL_SETUP_2ND_COLUMN, y, LCD_W - MODEL_SETUP_2ND_COLUMN - 2, 8, TEXT_COLOR);
    }
  }
  break;
}

case ITEM_MODEL_EXTERNAL_MODULE_OPTIONS:
{
  uint8_t moduleIndex = CURRENT_MODULE_EDITED(i);
#if defined(MULTIMODULE)
  if (isModuleMultimodule(moduleIndex)) {
    int optionValue = g_model.moduleData[moduleIndex].multi.optionValue;

    const uint8_t multi_proto = g_model.moduleData[EXTERNAL_MODULE].getMultiProtocol();
    const mm_protocol_definition *pdef = getMultiProtocolDefinition(multi_proto);
    if (pdef->optionsstr)
      drawText(window,MENUS_MARGIN_LEFT, y, pdef->optionsstr);

    if (multi_proto == MODULE_SUBTYPE_MULTI_FS_AFHDS2A)
      optionValue = 50 + 5 * optionValue;

    lcdDrawNumber(MODEL_SETUP_2ND_COLUMN, y, optionValue, LEFT | attr);
    if (attr) {
      if (multi_proto == MODULE_SUBTYPE_MULTI_FS_AFHDS2A) {
        CHECK_INCDEC_MODELVAR(event, g_model.moduleData[moduleIndex].multi.optionValue, 0, 70);
      }
      else if (multi_proto == MODULE_SUBTYPE_MULTI_OLRS) {
        CHECK_INCDEC_MODELVAR(event, g_model.moduleData[moduleIndex].multi.optionValue, -1, 7);
      }
      else {
        CHECK_INCDEC_MODELVAR(event, g_model.moduleData[moduleIndex].multi.optionValue, -128, 127);
      }
    }
  }
#endif
  if (isModuleR9M(moduleIndex)) {
    drawText(window,MENUS_MARGIN_LEFT, y, STR_MODULE_TELEMETRY);
    if (IS_TELEMETRY_INTERNAL_MODULE()) {
      drawText(window,MODEL_SETUP_2ND_COLUMN, y, STR_DISABLE_INTERNAL);
    }
    else {
      drawText(window,MODEL_SETUP_2ND_COLUMN, y, STR_MODULE_TELEM_ON);
    }
  }
  else if (IS_MODULE_SBUS(moduleIndex)) {
    drawText(window,MENUS_MARGIN_LEFT, y, STR_WARN_BATTVOLTAGE);
    drawValueWithUnit(MODEL_SETUP_4TH_COLUMN, y, getBatteryVoltage(), UNIT_VOLTS, attr|PREC2|LEFT);
  }
}
break;

case ITEM_MODEL_EXTERNAL_MODULE_BIND_OPTIONS:
{
  uint8_t moduleIndex = CURRENT_MODULE_EDITED(i);

  drawText(window,MENUS_MARGIN_LEFT+ INDENT_WIDTH, y, "Bind mode");
  if (g_model.moduleData[moduleIndex].pxx.power == R9M_LBT_POWER_25_8CH) {
    if(g_model.moduleData[moduleIndex].pxx.receiverTelemetryOff == true)
      drawText(window,MODEL_SETUP_2ND_COLUMN, y, STR_BINDING_25MW_CH1_8_TELEM_OFF);
    else
      drawText(window,MODEL_SETUP_2ND_COLUMN, y, STR_BINDING_25MW_CH1_8_TELEM_ON);
  }
  else {
    if(g_model.moduleData[moduleIndex].pxx.receiverHigherChannels == true)
      drawText(window,MODEL_SETUP_2ND_COLUMN, y, STR_BINDING_500MW_CH9_16_TELEM_OFF);
    else
      drawText(window,MODEL_SETUP_2ND_COLUMN, y, STR_BINDING_500MW_CH1_8_TELEM_OFF);
  }
  while (menuVerticalPosition==i && menuHorizontalPosition > 0) {
    REPEAT_LAST_CURSOR_MOVE(ITEM_MODEL_SETUP_MAX, true);
  }
}

case ITEM_MODEL_EXTERNAL_MODULE_POWER:
{
  uint8_t moduleIndex = CURRENT_MODULE_EDITED(i);
  if (isModuleR9M_FCC(moduleIndex)) {
    // Power selection is only available on R9M FCC
    drawText(window,MENUS_MARGIN_LEFT, y, STR_MULTI_RFPOWER);
    lcdDrawTextAtIndex(MODEL_SETUP_2ND_COLUMN, y, STR_R9M_FCC_POWER_VALUES, g_model.moduleData[moduleIndex].pxx.power, LEFT | attr);
    if (attr)
      CHECK_INCDEC_MODELVAR(event, g_model.moduleData[moduleIndex].pxx.power, 0, R9M_FCC_POWER_MAX);
  }
#if defined(MULTIMODULE)
  else if (isModuleMultimodule(moduleIndex)) {
    drawText(window,MENUS_MARGIN_LEFT, y, STR_MULTI_LOWPOWER);
    g_model.moduleData[EXTERNAL_MODULE].multi.lowPowerMode = editCheckBox(window, g_model.moduleData[EXTERNAL_MODULE].multi.lowPowerMode, MODEL_SETUP_2ND_COLUMN, y, attr, event);
  }
#endif
}
break;

#if defined(MULTIMODULE)
case ITEM_MODEL_EXTERNAL_MODULE_AUTOBIND:
if (g_model.moduleData[EXTERNAL_MODULE].getMultiProtocol() == MODULE_SUBTYPE_MULTI_DSM2)
  drawText(window,MENUS_MARGIN_LEFT, y, STR_MULTI_DSM_AUTODTECT);
else
  drawText(window,MENUS_MARGIN_LEFT, y, STR_MULTI_AUTOBIND);
g_model.moduleData[EXTERNAL_MODULE].multi.autoBindMode = editCheckBox(window, g_model.moduleData[EXTERNAL_MODULE].multi.autoBindMode, MODEL_SETUP_2ND_COLUMN, y, attr, event);
break;
case ITEM_MODEL_EXTERNAL_MODULE_STATUS: {
drawText(window,MENUS_MARGIN_LEFT, y, STR_MODULE_STATUS);

char statusText[64];
getMultiModuleStatus(EXTERNAL_MODULE).getStatusString(statusText);
drawText(window,MODEL_SETUP_2ND_COLUMN, y, statusText);
break;
case ITEM_MODEL_EXTERNAL_MODULE_SYNCSTATUS: {
drawText(window,MENUS_MARGIN_LEFT, y, STR_MODULE_SYNC);

char statusText[64];
getModuleSyncStatus(EXTERNAL_MODULE).getRefreshString(statusText);
drawText(window,MODEL_SETUP_2ND_COLUMN, y, statusText);
break;
}
}
#endif
}

y += 40;
}

if (IS_RANGECHECK_ENABLE()) {
theme->drawMessageBox("RSSI :", NULL, NULL, WARNING_TYPE_INFO);
lcdDrawNumber(WARNING_LINE_X, WARNING_INFOLINE_Y, TELEMETRY_RSSI(), DBLSIZE|LEFT);
}
#endif

#if 0
bool menuModelFailsafe(event_t event)
{
  uint8_t ch = 0;
  const uint8_t channelStart = g_model.moduleData[g_moduleIndex].channelsStart;
  const int lim = (g_model.extendedLimits ? (512 * LIMIT_EXT_PERCENT / 100) : 512) * 2;
  const uint8_t SLIDER_W = 128;
  const uint8_t cols = sentModuleChannels(g_moduleIndex) > 8 ? 2 : 1;

  if (event == EVT_KEY_LONG(KEY_ENTER)) {
    killEvents(event);
    event = 0;
    if (s_editMode) {
      g_model.moduleData[g_moduleIndex].failsafeChannels[menuVerticalPosition] = channelOutputs[menuVerticalPosition +
                                                                                              channelStart];
      storageDirty(EE_MODEL);
      AUDIO_WARNING1();
      s_editMode = 0;
      SEND_FAILSAFE_NOW(g_moduleIndex);
    } else {
      int16_t &failsafe = g_model.moduleData[g_moduleIndex].failsafeChannels[menuVerticalPosition];
      if (failsafe < FAILSAFE_CHANNEL_HOLD)
        failsafe = FAILSAFE_CHANNEL_HOLD;
      else if (failsafe == FAILSAFE_CHANNEL_HOLD)
        failsafe = FAILSAFE_CHANNEL_NOPULSE;
      else
        failsafe = 0;
      storageDirty(EE_MODEL);
      AUDIO_WARNING1();
      SEND_FAILSAFE_NOW(g_moduleIndex);
    }
  }

  SIMPLE_SUBMENU_WITH_OPTIONS("FAILSAFE", ICON_STATS_ANALOGS, NUM_CHANNELS(g_moduleIndex), OPTION_MENU_NO_SCROLLBAR);
  drawStringWithIndex(50, 3 + FH, "Module", g_moduleIndex + 1, MENU_TITLE_COLOR);

  for (uint8_t col = 0; col < cols; col++) {
    for (uint8_t line = 0; line < 8; line++) {
      coord_t x = col * (LCD_W / 2);
      const coord_t y = MENU_CONTENT_TOP - FH + line * (FH + 4);
      const int32_t channelValue = channelOutputs[ch + channelStart];
      int32_t failsafeValue = g_model.moduleData[g_moduleIndex].failsafeChannels[8 * col + line];

      // Channel name if present, number if not
      if (g_model.limitData[ch + channelStart].name[0] != '\0') {
        putsChn(x + MENUS_MARGIN_LEFT, y - 3, ch + 1, TINSIZE);
        lcdDrawSizedText(x + MENUS_MARGIN_LEFT, y + 5, g_model.limitData[ch + channelStart].name,
                         sizeof(g_model.limitData[ch + channelStart].name), ZCHAR | SMLSIZE);
      } else {
        putsChn(x + MENUS_MARGIN_LEFT, y, ch + 1, 0);
      }

      // Value
      LcdFlags flags = RIGHT;
      if (menuVerticalPosition == ch) {
        flags |= INVERS;
        if (s_editMode) {
          if (failsafeValue == FAILSAFE_CHANNEL_HOLD || failsafeValue == FAILSAFE_CHANNEL_NOPULSE) {
            s_editMode = 0;
          } else {
            flags |= BLINK;
            CHECK_INCDEC_MODELVAR(event, g_model.moduleData[g_moduleIndex].failsafeChannels[8 * col + line], -lim, +lim);
          }
        }
      }

      x += (LCD_W / 2) - 4 - MENUS_MARGIN_LEFT - SLIDER_W;

      if (failsafeValue == FAILSAFE_CHANNEL_HOLD) {
        lcd->drawText(x, y + 2, "HOLD", flags | SMLSIZE);
        failsafeValue = 0;
      } else if (failsafeValue == FAILSAFE_CHANNEL_NOPULSE) {
        lcd->drawText(x, y + 2, "NONE", flags | SMLSIZE);
        failsafeValue = 0;
      } else {
#if defined(PPM_UNIT_US)
        lcdDrawNumber(x, y, PPM_CH_CENTER(ch)+failsafeValue/2, flags);
#elif defined(PPM_UNIT_PERCENT_PREC1)
        lcdDrawNumber(x, y, calcRESXto1000(failsafeValue), PREC1 | flags);
#else
        lcdDrawNumber(x, y, calcRESXto1000(failsafeValue)/10, flags);
#endif
      }

      // Gauge
      x += 4;
      lcdDrawRect(x, y + 3, SLIDER_W + 1, 12);
      const coord_t lenChannel = limit((uint8_t) 1, uint8_t((abs(channelValue) * SLIDER_W / 2 + lim / 2) / lim),
                                       uint8_t(SLIDER_W / 2));
      const coord_t lenFailsafe = limit((uint8_t) 1, uint8_t((abs(failsafeValue) * SLIDER_W / 2 + lim / 2) / lim),
                                        uint8_t(SLIDER_W / 2));
      x += SLIDER_W / 2;
      const coord_t xChannel = (channelValue > 0) ? x : x + 1 - lenChannel;
      const coord_t xFailsafe = (failsafeValue > 0) ? x : x + 1 - lenFailsafe;
      lcdDrawSolidFilledRect(xChannel, y + 4, lenChannel, 5, TEXT_COLOR);
      lcdDrawSolidFilledRect(xFailsafe, y + 9, lenFailsafe, 5, ALARM_COLOR);

      if (++ch >= NUM_CHANNELS(g_moduleIndex))
        break;

    }
  }

  return true;
}

#endif
