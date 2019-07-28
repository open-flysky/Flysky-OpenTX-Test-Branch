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

#include "model_mixes.h"
#include "opentx.h"
#include "libwindows.h"

#define SET_DIRTY()     storageDirty(EE_MODEL)

#define PASTE_BEFORE    -2
#define PASTE_AFTER     -1

uint8_t getMixesCount()
{
  uint8_t count = 0;
  uint8_t ch;

  for (int i = MAX_MIXERS - 1; i >= 0; i--) {
    ch = mixAddress(i)->srcRaw;
    if (ch != 0) {
      count++;
    }
  }
  return count;
}

bool reachMixesLimit()
{
  if (getMixesCount() >= MAX_MIXERS) {
    POPUP_WARNING(STR_NOFREEMIXER);
    return true;
  }
  return false;
}


class MixEditWindow : public Page {
  public:
    MixEditWindow(int8_t channel, uint8_t mixIndex) :
      Page(),
      channel(channel),
      mixIndex(mixIndex)
    {
      buildBody(&body);
      buildHeader(&header);
    }

  protected:
    uint8_t channel;
    uint8_t mixIndex;
    Window * updateCurvesWindow = nullptr;
    Choice * curveTypeChoice = nullptr;

    void buildHeader(Window * window)
    {
      new StaticText(window, {70, 4, 100, 20}, STR_MIXER, MENU_TITLE_COLOR);
      new StaticText(window, {70, 28, 100, 20}, getSourceString(MIXSRC_CH1 + channel), MENU_TITLE_COLOR);
    }

    void updateCurves()
    {
      GridLayout grid;
      updateCurvesWindow->clear();

      MixData * line = mixAddress(mixIndex);

      new StaticText(updateCurvesWindow, grid.getLabelSlot(), STR_CURVE);
      curveTypeChoice = new Choice(updateCurvesWindow, grid.getFieldSlot(2, 0), "\004DiffExpoFuncCstm", 0, CURVE_REF_CUSTOM,
                                   GET_DEFAULT(line->curve.type),
                                   [=](int32_t newValue) {
                                     line->curve.type = newValue;
                                     line->curve.value = 0;
                                     SET_DIRTY();
                                     updateCurves();
                                     curveTypeChoice->setFocus();
                                   });

      switch (line->curve.type) {
        case CURVE_REF_DIFF:
        case CURVE_REF_EXPO: {
          // TODO GVAR
          NumberEdit * edit = new NumberEdit(updateCurvesWindow, grid.getFieldSlot(2, 1), -100, 100,
                                             GET_SET_DEFAULT(line->curve.value));
          edit->setSuffix("%");
          break;
        }
        case CURVE_REF_FUNC:
          new Choice(updateCurvesWindow, grid.getFieldSlot(2, 1), STR_VCURVEFUNC, 0, CURVE_BASE - 1, GET_SET_DEFAULT(line->curve.value));
          break;
        case CURVE_REF_CUSTOM:
          new CustomCurveChoice(updateCurvesWindow, grid.getFieldSlot(2, 1), -MAX_CURVES, MAX_CURVES, GET_SET_DEFAULT(line->curve.value));
          break;
      }
    }

    void buildBody(Window * window)
    {
      GridLayout grid;
      grid.spacer(8);

      MixData * mix = mixAddress(mixIndex);

      // Mix name
      new StaticText(window, grid.getLabelSlot(), STR_MIXNAME);
      new TextEdit(window, grid.getFieldSlot(), mix->name, sizeof(mix->name));
      grid.nextLine();

      // Source
      new StaticText(window, grid.getLabelSlot(), STR_SOURCE);
      new SourceChoice(window, grid.getFieldSlot(), 0, MIXSRC_LAST, GET_SET_DEFAULT(mix->srcRaw));
      grid.nextLine();

      // Weight
      new StaticText(window, grid.getLabelSlot(), STR_WEIGHT);
      // TODO GVAR ?
      NumberEdit * edit = new NumberEdit(window, grid.getFieldSlot(), -500, 500, GET_SET_DEFAULT(mix->weight));
      edit->setSuffix("%");
      grid.nextLine();

      // Offset
      new StaticText(window, grid.getLabelSlot(), STR_OFFSET);
      edit = new NumberEdit(window, grid.getFieldSlot(), GV_RANGELARGE_OFFSET_NEG, GV_RANGELARGE_OFFSET, GET_SET_DEFAULT(mix->offset));
      edit->setSuffix("%");
      grid.nextLine();

      // Trim
      new StaticText(window, grid.getLabelSlot(), STR_TRIM);
      new CheckBox(window, grid.getFieldSlot(), GET_SET_INVERTED(mix->carryTrim));
      grid.nextLine();

      // Curve
      updateCurvesWindow = new Window(window, {0, grid.getWindowHeight(), LCD_W, 0});
      updateCurves();
      grid.addWindow(updateCurvesWindow);

      // Flight modes
      new StaticText(window, grid.getLabelSlot(), STR_FLMODE);
      for (uint8_t i = 0; i < MAX_FLIGHT_MODES; i++) {
        char fm[2] = {char('0' + i), '\0'};
        if (i > 0 && (i % 4) == 0)
          grid.nextLine();
        new TextButton(window, grid.getFieldSlot(4, i % 4), fm,
                       [=]() -> uint8_t {
                         BF_BIT_FLIP(mix->flightModes, BF_BIT(i));
                         SET_DIRTY();
                         return !(BF_SINGLE_BIT_GET(mix->flightModes, i));
                       },
                       BF_SINGLE_BIT_GET(mix->flightModes, i) ? 0 : BUTTON_CHECKED);
      }
      grid.nextLine();

      // Switch
      new StaticText(window, grid.getLabelSlot(), STR_SWITCH);
      new SwitchChoice(window, grid.getFieldSlot(), SWSRC_FIRST_IN_MIXES, SWSRC_LAST_IN_MIXES, GET_SET_DEFAULT(mix->swtch));
      grid.nextLine();

      // Warning
      new StaticText(window, grid.getLabelSlot(), STR_MIXWARNING);
      edit = new NumberEdit(window, grid.getFieldSlot(2, 0), 0, 3, GET_SET_DEFAULT(mix->mixWarn));
      edit->setZeroText(STR_OFF);
      grid.nextLine();

      // Multiplex
      new StaticText(window, grid.getLabelSlot(), STR_MULTPX);
      new Choice(window, grid.getFieldSlot(), STR_VMLTPX, 0, 2, GET_SET_DEFAULT(mix->mltpx));
      grid.nextLine();

      // Delay up
      new StaticText(window, grid.getLabelSlot(), STR_DELAYUP);
      edit = new NumberEdit(window, grid.getFieldSlot(2, 0), 0, DELAY_MAX,
                            GET_DEFAULT(mix->delayUp),
                            SET_VALUE(mix->delayUp, newValue),
                            PREC1);
      edit->setStep(10 / DELAY_STEP);
      edit->setSuffix("s");
      grid.nextLine();

      // Delay down
      new StaticText(window, grid.getLabelSlot(), STR_DELAYDOWN);
      edit = new NumberEdit(window, grid.getFieldSlot(2, 0), 0, DELAY_MAX,
                            GET_DEFAULT(mix->delayDown),
                            SET_VALUE(mix->delayDown, newValue),
                            PREC1);
      edit->setStep(10 / DELAY_STEP);
      edit->setSuffix("s");
      grid.nextLine();

      // Slow up
      new StaticText(window, grid.getLabelSlot(), STR_SLOWUP);
      edit = new NumberEdit(window, grid.getFieldSlot(2, 0), 0, DELAY_MAX,
                            GET_DEFAULT(mix->speedUp),
                            SET_VALUE(mix->speedUp, newValue),
                            PREC1);
      edit->setStep(10 / DELAY_STEP);
      edit->setSuffix("s");
      grid.nextLine();

      // Slow down
      new StaticText(window, grid.getLabelSlot(), STR_SLOWDOWN);
      edit = new NumberEdit(window, grid.getFieldSlot(2, 0), 0, DELAY_MAX,
                            GET_DEFAULT(mix->speedDown),
                            SET_VALUE(mix->speedDown, newValue),
                            PREC1);
      edit->setStep(10 / DELAY_STEP);
      edit->setSuffix("s");
      grid.nextLine();

      grid.nextLine();

      window->setInnerHeight(grid.getWindowHeight());
    }
};

static constexpr coord_t line1 = 2;
static constexpr coord_t line2 = 22;

class MixLineButton : public Button {
  public:
    MixLineButton(Window * parent, const rect_t &rect, uint8_t mixIndex) :
      Button(parent, rect),
      mixIndex(mixIndex)
    {
      const MixData & mix = g_model.mixData[mixIndex];
      if (mix.swtch || mix.curve.value != 0 || mix.flightModes) {
        setHeight(getHeight() + 20);
      }
    }

    bool isActive()
    {
      return isMixActive(mixIndex);
    }

    void checkEvents() override
    {
      if (active != isActive()) {
        invalidate();
        active = !active;
      }
    }

    void paintFlightModes(BitmapBuffer * dc, FlightModesType value)
    {
      dc->drawBitmap(146, line2 + 2, mixerSetupFlightmodeBitmap);
      coord_t x = 166;
      for (int i = 0; i < MAX_FLIGHT_MODES; i++) {
        char s[] = " ";
        s[0] = '0' + i;
        if (value & (1 << i)) {
          dc->drawText(x, line2 + 1, s, SMLSIZE | TEXT_DISABLE_COLOR);
        }
        else {
          dc->drawSolidFilledRect(x, 40, 8, 3, SCROLLBOX_COLOR);
          dc->drawText(x, line2 + 1, s, SMLSIZE);
        }
        x += 8;
      }
    }

    void paintMixLine(BitmapBuffer * dc)
    {
      const MixData &mix = g_model.mixData[mixIndex];

      // first line ...
      drawNumber(dc, 3, line1, mix.weight, 0, 0, nullptr, "%");
      // TODO gvarWeightItem(MIX_LINE_WEIGHT_POS, y, md, RIGHT | attr | (isMixActive(i) ? BOLD : 0), event);

      drawSource(dc, 60, line1, mix.srcRaw);

      if (mix.name[0]) {
        dc->drawBitmap(146, line1 + 2, mixerSetupLabelBitmap);
        dc->drawSizedText(166, line1, mix.name, sizeof(mix.name), ZCHAR);
      }

      // second line ...
      if (mix.swtch) {
        dc->drawBitmap(3, line2 + 2, mixerSetupSwitchBitmap);
        drawSwitch(21, line2, mix.swtch);
      }

      if (mix.curve.value) {
        dc->drawBitmap(60, line2 + 2, mixerSetupCurveBitmap);
        drawCurveRef(dc, 80, line2, mix.curve);
      }

      if (mix.flightModes) {
        paintFlightModes(dc, mix.flightModes);
      }
    }

    virtual void paint(BitmapBuffer * dc) override
    {
      if (active)
        dc->drawSolidFilledRect(2, 2, rect.w - 4, rect.h - 4, WARNING_COLOR);
      paintMixLine(dc);
      drawSolidRect(dc, 0, 0, rect.w, rect.h, 2, hasFocus() ? SCROLLBOX_COLOR : CURVE_AXIS_COLOR);
    }

  protected:
    uint8_t mixIndex;
    bool active = false;
};

void insertMix(uint8_t idx, uint8_t channel)
{
  pauseMixerCalculations();
  MixData * mix = mixAddress(idx);
  memmove(mix + 1, mix, (MAX_MIXERS - (idx + 1)) * sizeof(MixData));
  memclear(mix, sizeof(MixData));
  mix->destCh = channel;
  mix->srcRaw = channel + 1;
  if (!isSourceAvailable(mix->srcRaw)) {
    mix->srcRaw = (channel > 3 ? MIXSRC_Rud - 1 + channel : MIXSRC_Rud - 1 + channel_order(channel));
    while (!isSourceAvailable(mix->srcRaw)) {
      mix->srcRaw += 1;
    }
  }
  mix->weight = 100;
  resumeMixerCalculations();
  storageDirty(EE_MODEL);
}

ModelMixesPage::ModelMixesPage() :
  PageTab(STR_MIXER, ICON_MODEL_MIXER)
{
}

void ModelMixesPage::rebuild(Window * window, int8_t focusMixIndex)
{
  coord_t scrollPosition = window->getScrollPositionY();
  window->clear();
  build(window, focusMixIndex);
  window->setScrollPositionY(scrollPosition);
}

void ModelMixesPage::editMix(Window * window, uint8_t channel, uint8_t mixIndex)
{
  Window * editWindow = new MixEditWindow(channel, mixIndex);
  editWindow->setCloseHandler([=]() {
    rebuild(window, mixIndex);
  });
}

void ModelMixesPage::build(Window * window, int8_t focusMixIndex)
{
  GridLayout grid;
  grid.spacer(8);
  grid.setLabelWidth(66);

  Window::clearFocus();

  const BitmapBuffer * const mixerMultiplexBitmap[] = {
    mixerSetupAddBitmap,
    mixerSetupMultiBitmap,
    mixerSetupReplaceBitmap
  };

  int mixIndex = 0;
  MixData * mix = g_model.mixData;
  for (uint8_t ch = 0; ch < MAX_OUTPUT_CHANNELS; ch++) {
    if (mixIndex < MAX_MIXERS && mix->srcRaw >= 0 && mix->destCh == ch) {
      new TextButton(window, grid.getLabelSlot(), getSourceString(MIXSRC_CH1 + ch));
      uint8_t count = 0;
      while (mixIndex < MAX_MIXERS && mix->srcRaw >= 0 && mix->destCh == ch) {
        Button * button = new MixLineButton(window, grid.getFieldSlot(), mixIndex);
        if (focusMixIndex == mixIndex)
          button->setFocus();
        button->setPressHandler([=]() -> uint8_t {
          button->bringToTop();
          Menu * menu = new Menu();
          menu->addLine(STR_EDIT, [=]() {
            editMix(window, ch, mixIndex);
          });
          if (!reachMixesLimit()) {
            menu->addLine(STR_INSERT_BEFORE, [=]() {
              insertMix(mixIndex, ch);
              editMix(window, ch, mixIndex);
            });
            menu->addLine(STR_INSERT_AFTER, [=]() {
              insertMix(mixIndex + 1, ch);
              editMix(window, ch, mixIndex + 1);
            });
            menu->addLine(STR_COPY, [=]() {
              s_copyMode = COPY_MODE;
              s_copySrcIdx =mixIndex;
            });
            if (s_copyMode != 0) {
              menu->addLine(STR_PASTE_BEFORE, [=]() {
                copyMix(s_copySrcIdx, mixIndex, PASTE_BEFORE);
                if(s_copyMode == MOVE_MODE) {
                  deleteMix((s_copySrcIdx > mixIndex) ? s_copySrcIdx+1 : s_copySrcIdx);
                  s_copyMode = 0;
                }
                rebuild(window, mixIndex);
              });
              menu->addLine(STR_PASTE_AFTER, [=]() {
                copyMix(s_copySrcIdx, mixIndex, PASTE_AFTER);
                if(s_copyMode == MOVE_MODE) {
                  deleteMix((s_copySrcIdx > mixIndex) ? s_copySrcIdx+1 : s_copySrcIdx);
                  s_copyMode = 0;
                }
                rebuild(window, mixIndex+1);
              });
            }
          }
          menu->addLine(STR_MOVE, [=]() {
            s_copyMode = MOVE_MODE;
            s_copySrcIdx = mixIndex;
          });
          menu->addLine(STR_DELETE, [=]() {
            deleteMix(mixIndex);
            rebuild(window, -1);
          });
          return 0;
        });

        if (count++ > 0) {
          new StaticBitmap(window, {35, button->top() + (button->height() - 18) / 2, 25, 17}, mixerMultiplexBitmap[mix->mltpx]);
        }

        grid.spacer(button->height() - 2);
        ++mixIndex;
        ++mix;
      }

      grid.spacer(7);
    }
    else {
      auto button = new TextButton(window, grid.getLabelSlot(), getSourceString(MIXSRC_CH1 + ch));
      if (focusMixIndex == mixIndex)
        button->setFocus();
      button->setPressHandler([=]() -> uint8_t {
        button->bringToTop();
        Menu * menu = new Menu();
        menu->addLine(STR_EDIT, [=]() {
          insertMix(mixIndex, ch);
          editMix(window, ch, mixIndex);
          return 0;
        });
        if (!reachMixesLimit()) {
          if (s_copyMode != 0) {
            menu->addLine(STR_PASTE, [=]() {
              copyMix(s_copySrcIdx, mixIndex, ch);
              if(s_copyMode == MOVE_MODE) {
                deleteMix((s_copySrcIdx >= mixIndex) ? s_copySrcIdx+1 : s_copySrcIdx);
                s_copyMode = 0;
              }
              rebuild(window, -1);
              return 0;
            });
          }
        }
        // TODO STR_MOVE
        return 0;
      });
      grid.nextLine();
    }
  }

  Window * focus = Window::getFocus();
  if (focus) {
    focus->bringToTop();
  }

  grid.nextLine();

  window->setInnerHeight(grid.getWindowHeight());
}

void deleteMix(uint8_t idx)
{
  pauseMixerCalculations();
  MixData * mix = mixAddress(idx);
  memmove(mix, mix + 1, (MAX_MIXERS - (idx + 1)) * sizeof(MixData));
  memclear(&g_model.mixData[MAX_MIXERS - 1], sizeof(MixData));
  resumeMixerCalculations();
  storageDirty(EE_MODEL);
}

void insertMix(uint8_t idx)
{
  insertMix(idx, s_currCh + 1);
}

void copyMix(uint8_t source, uint8_t dest, int8_t ch)
{
  pauseMixerCalculations();
  MixData sourceMix;
  memcpy(&sourceMix, mixAddress(source), sizeof(MixData));
  MixData * mix = mixAddress(dest);
  if(ch == PASTE_AFTER) {
    memmove(mix+2, mix+1, (MAX_MIXERS-(source+1))*sizeof(MixData));
    memcpy(mix+1, &sourceMix, sizeof(MixData));
    (mix+1)->destCh = (mix)->destCh;
  }
  else if(ch == PASTE_BEFORE) {
    memmove(mix+1, mix, (MAX_MIXERS-(source+1))*sizeof(MixData));
    memcpy(mix, &sourceMix, sizeof(MixData));
    mix->destCh = (mix+1)->destCh;
  }
  else {
    memmove(mix+1, mix, (MAX_MIXERS-(source+1))*sizeof(MixData));
    memcpy(mix, &sourceMix, sizeof(MixData));
    mix->destCh  = ch;
  }

  //memmove(mix + 1, mix, (MAX_MIXERS - (dest + 1)) * sizeof(MixData));
  resumeMixerCalculations();
  storageDirty(EE_MODEL);
}

bool swapMixes(uint8_t &idx, uint8_t up)
{
  MixData * x, * y;
  int8_t tgt_idx = (up ? idx - 1 : idx + 1);

  x = mixAddress(idx);

  if (tgt_idx < 0) {
    if (x->destCh == 0)
      return false;
    x->destCh--;
    return true;
  }

  if (tgt_idx == MAX_MIXERS) {
    if (x->destCh == MAX_OUTPUT_CHANNELS - 1)
      return false;
    x->destCh++;
    return true;
  }

  y = mixAddress(tgt_idx);
  uint8_t destCh = x->destCh;
  if (!y->srcRaw || destCh != y->destCh) {
    if (up) {
      if (destCh > 0) x->destCh--;
      else return false;
    }
    else {
      if (destCh < MAX_OUTPUT_CHANNELS - 1) x->destCh++;
      else return false;
    }
    return true;
  }

  pauseMixerCalculations();
  memswap(x, y, sizeof(MixData));
  resumeMixerCalculations();

  idx = tgt_idx;
  return true;
}
