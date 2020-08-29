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

#include "model_monitor.h"
#include "opentx.h"
#include "libwindows.h"

#define SET_DIRTY()     storageDirty(EE_MODEL)
#define STR_4BIND(v)    ((moduleState[moduleIndex].mode == MODULE_MODE_BIND) ? STR_MODULE_BINDING : (v))

class ChannelMonitorBargraph: public Window {
  public:
    ChannelMonitorBargraph(Window * parent, const rect_t & rect, uint8_t moduleIndex, uint8_t channel):
      Window(parent, rect),
      moduleIndex(moduleIndex),
      channel(channel)
    {
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

class MonitorBody : public Window {
  public:
    MonitorBody(Window * parent, const rect_t &rect, uint8_t moduleIndex) :
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

      for (int ch=0; ch < maxModuleChannels(moduleIndex); ch++) {
        // Channel name
        // TODO if (g_model.limitData[ch].name[0] != '\0') { <= add channel name
        new StaticText(this, grid.getLabelSlot(), getSourceString(MIXSRC_CH1 + ch));

        // Channel numeric value
        new NumberEdit(this, grid.getFieldSlot(3, 0), -lim, lim,
                       GET_DEFAULT(g_model.moduleData[moduleIndex].failsafeChannels[ch]),
                       //GET_DEFAULT(calcRESXto1000(g_model.moduleData[moduleIndex].failsafeChannels[ch])),
                       SET_VALUE(g_model.moduleData[moduleIndex].failsafeChannels[ch], newValue), PREC1);

        // Channel bargraph
        new ChannelMonitorBargraph(this, {150, grid.getWindowHeight(), 150, lineHeight}, moduleIndex, ch);
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


ModelMonitorPage::ModelMonitorPage() :
  PageTab(STR_MENUSETUP, ICON_MODEL_SETUP)
{
}


void ModelMonitorPage::build(Window * window)
{
  //new MonitorBody(window, {0, 0, LCD_W, window->height() - footerHeight}, moduleIndex );
  new MonitorBody(window, {0, 0, LCD_W, window->height() - footerHeight}, moduleIndex );
  //new FailSafeFooter(window, {0, window->height() - footerHeight, LCD_W, footerHeight});
}

