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
#define FREQ_MULT 1000000
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
    void paint(BitmapBuffer * dc) override {
      lcdSetColor(RGB(0xE0, 0xE0, 0xE0));
      dc->clear(CUSTOM_COLOR);
    }
};

RadioSpectrumAnalyserPage::RadioSpectrumAnalyserPage():
  PageTab(STR_MENU_SPECTRUM_ANALYSER, ICON_RADIO_SPECTRUM_ANALYSER) { }

void RadioSpectrumAnalyserPage::leave() {
  if (!started) return;
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
          //setModuleType(INTERNAL_MODULE, MODULE_TYPE_NONE);
        }
        else {
          moduleState[moduleIndex].mode = MODULE_MODE_NORMAL;
        } 
      }
      /* wait 1s to resume normal operation before leaving */
      watchdogSuspend(500 /*5s*/);
      RTOS_WAIT_MS(1000);
    });
    mb->setCloseCondition([=]() -> DialogResult {
      if (get_tmr10ms() >= start + 500) {
        return DialogResult::OK;
      }
      return (DialogResult) 0;
    });
} 

bool RadioSpectrumAnalyserPage::prepare(Window * window) {
  if (moduleState[moduleIndex].mode != MODULE_MODE_SPECTRUM_ANALYSER) {
    if (TELEMETRY_STREAMING()) {
      TRACE("SpectrumAnalyser RX active");
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

    reusableBuffer.spectrumAnalyser.span = reusableBuffer.spectrumAnalyser.spanDefault * FREQ_MULT;
    reusableBuffer.spectrumAnalyser.freq = reusableBuffer.spectrumAnalyser.freqDefault * FREQ_MULT;
    reusableBuffer.spectrumAnalyser.track = reusableBuffer.spectrumAnalyser.freq;
    reusableBuffer.spectrumAnalyser.step = reusableBuffer.spectrumAnalyser.span / LCD_W;
    reusableBuffer.spectrumAnalyser.dirty = true;
    moduleState[moduleIndex].mode = MODULE_MODE_SPECTRUM_ANALYSER;
  }
}

void RadioSpectrumAnalyserPage::build(Window * window)
{
  moduleIndex = EXTERNAL_MODULE;
  if(!prepare(window)) {
    return;
  }
  started = true;
  auto spectrum = new SpectrumView(window, { 20, 5, LCD_W - 40, LCD_W - 40});

  GridLayout grid;
  grid.setMarginLeft(20);
  grid.setMarginRight(20);
  grid.setLabelWidth(160);
  grid.spacer(spectrum->height() + 15);

  new StaticText(window, grid.getLabelSlot(true), "Frequency");
  auto freq = new NumberEdit(window, grid.getFieldSlot(), reusableBuffer.spectrumAnalyser.freqMin, reusableBuffer.spectrumAnalyser.freqMax, 
    [=] { return reusableBuffer.spectrumAnalyser.freq / FREQ_MULT; },
    [=](int32_t newValue) { 
      reusableBuffer.spectrumAnalyser.freq = newValue * FREQ_MULT; 
      reusableBuffer.spectrumAnalyser.dirty = true; 
      });
  freq->setSuffix("MHz");
  grid.nextLine();

  
  new StaticText(window, grid.getLabelSlot(true), "Span");
  auto span = new NumberEdit(window, grid.getFieldSlot(), 1, reusableBuffer.spectrumAnalyser.spanMax, 
    [=] { return reusableBuffer.spectrumAnalyser.span / FREQ_MULT; },
    [=](int32_t newValue) { 
      reusableBuffer.spectrumAnalyser.span = newValue * FREQ_MULT; 
      reusableBuffer.spectrumAnalyser.step = reusableBuffer.spectrumAnalyser.span / spectrum->width();
      reusableBuffer.spectrumAnalyser.dirty = true; 
      });
  span->setSuffix("MHz");
  grid.nextLine();

  new StaticText(window, grid.getLabelSlot(true), "Track");

  auto track = new NumberEdit(window, grid.getFieldSlot(), 
    (reusableBuffer.spectrumAnalyser.freq - reusableBuffer.spectrumAnalyser.span / 2) / FREQ_MULT, 
    (reusableBuffer.spectrumAnalyser.freq + reusableBuffer.spectrumAnalyser.span / 2) / FREQ_MULT, 
    [=] { return reusableBuffer.spectrumAnalyser.span / FREQ_MULT; },
    [=](int32_t newValue) { 
      reusableBuffer.spectrumAnalyser.track = newValue * FREQ_MULT; 
      reusableBuffer.spectrumAnalyser.dirty = true; 
      });
  track->setSuffix("MHz");
  grid.nextLine();
}

//   constexpr coord_t SCALE_HEIGHT = 12;
//   constexpr coord_t SCALE_TOP = MENU_FOOTER_TOP - SCALE_HEIGHT;
//   constexpr coord_t BARGRAPH_HEIGHT = SCALE_TOP - MENU_HEADER_HEIGHT;

//   // Draw fixed part (scale,..)
//   lcdDrawSolidFilledRect(0, SCALE_TOP, LCD_W, SCALE_HEIGHT, CURVE_AXIS_COLOR);
//   for (uint32_t frequency = ((reusableBuffer.spectrumAnalyser.freq - reusableBuffer.spectrumAnalyser.span / 2) / 10000000) * 10000000 + 10000000; ; frequency += 10000000) {
//     int offset = frequency - (reusableBuffer.spectrumAnalyser.freq - reusableBuffer.spectrumAnalyser.span / 2);
//     int x = offset / reusableBuffer.spectrumAnalyser.step;
//     if (x >= LCD_W - 1)
//       break;
//     lcdDrawVerticalLine(x, MENU_HEADER_HEIGHT, LCD_H - MENU_HEADER_HEIGHT - MENU_FOOTER_HEIGHT, STASHED, CURVE_AXIS_COLOR);

//     if ((frequency / 1000000) % 2 == 0) {
//       lcdDrawNumber(x, SCALE_TOP - 1, frequency / 1000000, TINSIZE | TEXT_COLOR | CENTERED);
//     }
//   }

//   for (uint8_t power = 20;; power += 20) {
//     int y = MENU_FOOTER_TOP - 1 - limit<int>(0, power << 1, LCD_H - MENU_HEADER_HEIGHT - MENU_FOOTER_HEIGHT);
//     if (y <= MENU_HEADER_HEIGHT)
//       break;
//     lcdDrawHorizontalLine(0, y, LCD_W, STASHED, CURVE_AXIS_COLOR);
//   }

//   // Draw tracker
//   int offset = reusableBuffer.spectrumAnalyser.track - (reusableBuffer.spectrumAnalyser.freq - reusableBuffer.spectrumAnalyser.span / 2);
//   int x = limit<int>(0, offset / reusableBuffer.spectrumAnalyser.step, LCD_W - 1);
//   lcdDrawSolidVerticalLine(x, MENU_HEADER_HEIGHT, BARGRAPH_HEIGHT, TEXT_COLOR);

//   // Draw spectrum data
//   constexpr uint8_t step = 4;

//   for (coord_t xv = 0; xv < LCD_W; xv += step) {
//     coord_t yv = SCALE_TOP - 1 - limit<int>(0, getAverage(step, &reusableBuffer.spectrumAnalyser.bars[xv]) << 1, BARGRAPH_HEIGHT);
//     coord_t max_yv = SCALE_TOP - 1 - limit<int>(0, getAverage(step, &reusableBuffer.spectrumAnalyser.max[xv]) << 1, BARGRAPH_HEIGHT);

//     // Signal bar
//     lcdDrawSolidFilledRect(xv, yv, step - 1, SCALE_TOP - yv, TEXT_INVERTED_BGCOLOR);
//     // lcdDrawSolidRect(xv, yv, step - 1, SCALE_TOP - yv, 1, TEXT_COLOR);

//     // Signal max
//     lcdDrawSolidHorizontalLine(xv, max_yv, step - 1, TEXT_COLOR);

//     // Decay max values
//     if (max_yv < yv) { // Those value are INVERTED (MENU_FOOTER_TOP - value)
//       for (uint8_t i = 0; i < step; i++) {
//         reusableBuffer.spectrumAnalyser.max[xv + i] = max<int>(0, reusableBuffer.spectrumAnalyser.max[xv + i] - 1);
//       }
//     }
//   }

//   return true;
// }
