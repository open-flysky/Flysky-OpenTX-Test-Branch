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
#include "radio_spectrum_analyser.h"
#include "libwindows.h"

extern uint8_t g_moduleIdx;
#define FREQ_MHZ 1000000
#define FREQ_10_MHZ 10000000
enum SpectrumFields
{
  SPECTRUM_FREQUENCY,
  SPECTRUM_SPAN,
  SPECTRUM_TRACK,
  SPECTRUM_FIELDS_MAX
};

coord_t getAverage(uint8_t number, const uint8_t * value)
{
  uint16_t sum = 0;
  for (uint8_t i = 0; i < number; i++) {
    sum += value[i];
  }
  return sum / number;
}

#if defined(INTERNAL_MODULE_MULTI)
  #define SPECTRUM_ROW  (g_moduleIdx == INTERNAL_MODULE ? READONLY_ROW : isModuleMultimodule(g_moduleIdx) ? READONLY_ROW : (uint8_t)0)
#else
  #define SPECTRUM_ROW  (isModuleMultimodule(g_moduleIdx) ? READONLY_ROW : (uint8_t)0)
#endif

class SpectrumView : public Window {
  public:
    SpectrumView(Window * parent, const rect_t & rect) : Window(parent, rect, OPAQUE){}

    void checkEvents() override {
      Window::checkEvents();
      if (reusableBuffer.spectrumAnalyser.dirty) {
        reusableBuffer.spectrumAnalyser.dirty = false;
        invalidate();
      }
    }

    void paint(BitmapBuffer * dc) override {
      lcdSetColor(RGB(0xE0, 0xE0, 0xE0));
      dc->clear(CUSTOM_COLOR);
      const coord_t SCALE_HEIGHT = 12;
      const coord_t SCALE_TOP = rect.h - SCALE_HEIGHT;
      // Draw fixed part (scale,..)
      dc->drawSolidFilledRect(0, SCALE_TOP, rect.w, SCALE_HEIGHT, CURVE_AXIS_COLOR);
      for (uint32_t frequency = ((reusableBuffer.spectrumAnalyser.freq - reusableBuffer.spectrumAnalyser.span / 2) / FREQ_10_MHZ) * FREQ_10_MHZ + FREQ_10_MHZ; ; frequency += FREQ_10_MHZ) {
        int offset = frequency - (reusableBuffer.spectrumAnalyser.freq - reusableBuffer.spectrumAnalyser.span / 2);
        int x = offset / reusableBuffer.spectrumAnalyser.step;
        if (x >= rect.w - 1)
          break;
        dc->drawVerticalLine(x, 0, rect.h, STASHED, CURVE_AXIS_COLOR);

        if ((frequency / FREQ_MHZ) % 10 == 0) {
          lcdDrawNumber(x, SCALE_TOP - 1, frequency / FREQ_MHZ, TINSIZE | TEXT_COLOR | CENTERED);
        }
      }

      for (uint8_t power = 20;; power += 20) {
        int y = rect.h - 1 - limit<int>(0, power << 1, rect.h);
        if (y < 0)
          break;
        dc->drawHorizontalLine(0, y, rect.w, STASHED, CURVE_AXIS_COLOR);
      }

      // Draw tracker
      int offset = reusableBuffer.spectrumAnalyser.track - (reusableBuffer.spectrumAnalyser.freq - reusableBuffer.spectrumAnalyser.span / 2);
      int x = limit<int>(0, offset / reusableBuffer.spectrumAnalyser.step, rect.w - 1);
      dc->drawSolidVerticalLine(x, 0, rect.h - SCALE_HEIGHT, TEXT_COLOR);

      // // Draw spectrum data
      const uint8_t step = 4;

      for (coord_t xv = 0; xv < rect.w; xv += step) {
        auto avg = getAverage(step, &reusableBuffer.spectrumAnalyser.bars[xv]) << 1;
        coord_t yv = SCALE_TOP - 1 - limit<int>(0, avg, rect.h);
        coord_t max_yv = SCALE_TOP - 1 - limit<int>(0, getAverage(step, &reusableBuffer.spectrumAnalyser.max[xv]) << 1, rect.h);

        // Signal bar
        lcdSetColor(RGB(avg, 255 - avg, 0));
        dc->drawSolidFilledRect(xv, yv, step - 1, SCALE_TOP - yv, CUSTOM_COLOR);
        // lcdDrawSolidRect(xv, yv, step - 1, SCALE_TOP - yv, 1, TEXT_COLOR);

        // Signal max
        dc->drawSolidHorizontalLine(xv, max_yv, step - 1, TEXT_COLOR);

        // Decay max values
        if (max_yv < yv) { // Those value are INVERTED (MENU_FOOTER_TOP - value)
          for (uint8_t i = 0; i < step; i++) {
            reusableBuffer.spectrumAnalyser.max[xv + i] = max<int>(0, reusableBuffer.spectrumAnalyser.max[xv + i] - 1);
          }
        }
      }
    }
};

RadioSpectrumAnalyserPage::RadioSpectrumAnalyserPage():
  PageTab(STR_MENU_SPECTRUM_ANALYSER, ICON_RADIO_SPECTRUM_ANALYSER) { }

bool RadioSpectrumAnalyserPage::leave(std::function<void()> handler) {
  if (moduleState[moduleIndex].mode != MODULE_MODE_SPECTRUM_ANALYSER) {
    return true;
  }
  started = false;
  tmr10ms_t start = get_tmr10ms();
  auto mb = new MessageBox(WARNING_TYPE_INFO, (DialogResult)0, "", STR_STOPPING, 
    [=](DialogResult result) { 
      if (isModulePXX2(moduleIndex)) {
        moduleState[moduleIndex].readModuleInformation(&reusableBuffer.moduleSetup.pxx2.moduleInformation, PXX2_HW_INFO_TX_ID, PXX2_HW_INFO_TX_ID);
        //resetAccessAuthenticationCount();
      }
      if (isModuleMultimodule(moduleIndex)) {
        if (reusableBuffer.spectrumAnalyser.moduleOFF) {
          setModuleType(moduleIndex, MODULE_TYPE_NONE);
        }
        else {
          moduleState[moduleIndex].mode = MODULE_MODE_NORMAL;
        } 
      }
      if(handler) handler();
    });
    mb->setCloseCondition([=]() -> DialogResult {
      if (get_tmr10ms() >= start + 500) {
        return DialogResult::OK;
      }
      return (DialogResult) 0;
    });
    return false;
} 

bool RadioSpectrumAnalyserPage::prepare(Window * window) {
  if (moduleState[moduleIndex].mode != MODULE_MODE_SPECTRUM_ANALYSER) {
    if (TELEMETRY_STREAMING()) {
      new MessageBox(WARNING_TYPE_INFO, DialogResult::OK, STR_TURN_OFF_RECEIVER, STR_TURN_OFF_RECEIVER_MESSAGE, [=](DialogResult result) { 
        build(window);
      });
      return false;
    }
    memclear(&reusableBuffer.spectrumAnalyser, sizeof(reusableBuffer.spectrumAnalyser));

#if defined(INTERNAL_MODULE_MULTI)
    if (moduleIndex == INTERNAL_MODULE && g_model.moduleData[INTERNAL_MODULE].type == MODULE_TYPE_NONE) {
      reusableBuffer.spectrumAnalyser.moduleOFF = true;
      setModuleType(INTERNAL_MODULE, MODULE_TYPE_MULTIMODULE);
    }
#endif

    if (isModuleR9MAccess(moduleIndex)) {
      reusableBuffer.spectrumAnalyser.spanDefault = 20;
      reusableBuffer.spectrumAnalyser.spanMax = 40;
      reusableBuffer.spectrumAnalyser.freqDefault = 890;
      reusableBuffer.spectrumAnalyser.freqMin = 850;
      reusableBuffer.spectrumAnalyser.freqMax = 930;
    }
    else {
      if (isModuleMultimodule(moduleIndex))
        reusableBuffer.spectrumAnalyser.spanDefault = 80;  // 80MHz
      else
        reusableBuffer.spectrumAnalyser.spanDefault = 40;  // 40MHz
      reusableBuffer.spectrumAnalyser.spanMax = 80;
      reusableBuffer.spectrumAnalyser.freqDefault = 2440; // 2440MHz
      reusableBuffer.spectrumAnalyser.freqMin = 2400;
      reusableBuffer.spectrumAnalyser.freqMax = 2485;
    }

    reusableBuffer.spectrumAnalyser.span = reusableBuffer.spectrumAnalyser.spanDefault * FREQ_MHZ;
    reusableBuffer.spectrumAnalyser.freq = reusableBuffer.spectrumAnalyser.freqDefault * FREQ_MHZ;
    reusableBuffer.spectrumAnalyser.track = reusableBuffer.spectrumAnalyser.freq;
    reusableBuffer.spectrumAnalyser.step = reusableBuffer.spectrumAnalyser.span / LCD_W;
    reusableBuffer.spectrumAnalyser.dirty = true;
    moduleState[moduleIndex].mode = MODULE_MODE_SPECTRUM_ANALYSER;
  }
  return true;
}

void RadioSpectrumAnalyserPage::build(Window * window)
{
  GridLayout grid;
  if (!started) {
    grid.spacer(10);
    auto startScan = new TextButton(window, grid.getLineSlot(), "Start");
    startScan->setPressHandler([=]() {
        started = true;
        window->clear();
        build(window);
        return 0;
    });
    return;
  }
  moduleIndex = EXTERNAL_MODULE;
  if(!prepare(window)) {
    return;
  }
  auto spectrum = new SpectrumView(window, { 0, 0, LCD_W, LCD_W - 40});
  grid.setMarginLeft(5);
  grid.setMarginRight(5);
  grid.setLabelWidth(5);
  grid.spacer(spectrum->height() + 5);

  new StaticText(window, grid.getFieldSlot(3,0), "Frequency");
  new StaticText(window, grid.getFieldSlot(3,1), "Span");
  new StaticText(window, grid.getFieldSlot(3,2), "Track");

  grid.nextLine();
  auto freq = new NumberEdit(window, grid.getFieldSlot(3,0), reusableBuffer.spectrumAnalyser.freqMin, reusableBuffer.spectrumAnalyser.freqMax, 
    [=] { return reusableBuffer.spectrumAnalyser.freq / FREQ_MHZ; },
    [=](int32_t newValue) { 
      reusableBuffer.spectrumAnalyser.freq = newValue * FREQ_MHZ; 
      reusableBuffer.spectrumAnalyser.dirty = true; 
      });
  freq->setSuffix("MHz");
  auto span = new NumberEdit(window, grid.getFieldSlot(3,1), 1, reusableBuffer.spectrumAnalyser.spanMax, 
    [=] { return reusableBuffer.spectrumAnalyser.span / FREQ_MHZ; },
    [=](int32_t newValue) { 
      reusableBuffer.spectrumAnalyser.span = newValue * FREQ_MHZ; 
      reusableBuffer.spectrumAnalyser.step = reusableBuffer.spectrumAnalyser.span / spectrum->width();
      reusableBuffer.spectrumAnalyser.dirty = true; 
      });
  span->setSuffix("MHz");
  auto track = new NumberEdit(window, grid.getFieldSlot(3,2), 
    (reusableBuffer.spectrumAnalyser.freq - reusableBuffer.spectrumAnalyser.span / 2) / FREQ_MHZ, 
    (reusableBuffer.spectrumAnalyser.freq + reusableBuffer.spectrumAnalyser.span / 2) / FREQ_MHZ, 
    [=] { return reusableBuffer.spectrumAnalyser.track / FREQ_MHZ; },
    [=](int32_t newValue) { 
      reusableBuffer.spectrumAnalyser.track = newValue * FREQ_MHZ; 
      reusableBuffer.spectrumAnalyser.dirty = true; 
      });
  track->setSuffix("MHz");
  grid.nextLine();
  window->setInnerHeight(grid.getWindowHeight());
}
