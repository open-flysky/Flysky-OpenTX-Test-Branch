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

#include "screen_theme.h"
#include "opentx.h"
#include "libwindows.h"
#include "widgets_setup.h"
#define SET_DIRTY() storageDirty(EE_GENERAL)

ScreenThemePage::ScreenThemePage() :
  PageTab(STR_USER_INTERFACE, ICON_THEME_SETUP)
{
}


static ZoneOptionValue ThemeValue[2];
ScreenThemePage::~ScreenThemePage()
{
    const ZoneOption * options = theme->getOptions();
    int optionsCount = getOptionsCount(options);
    for (int index = 0; index < optionsCount; index++)
    {
       const ZoneOption * option = &options[index];
       ZoneOptionValue * value = theme->getOptionValue(index);
       if(ThemeValue[index].unsignedValue != value->unsignedValue)
       {
           theme->updatecolor();
           break;
       }
    }
}

Window * createOptionEdit(Window * parent, const rect_t &rect, const ZoneOption * option, ZoneOptionValue * value)
{
  switch(option->type){
  case ZoneOption::Bool: return new CheckBox(parent, rect, GET_SET_DEFAULT(value->boolValue));
  case ZoneOption::Integer: return new NumberEdit(parent, rect, option->min.signedValue, option->max.signedValue, GET_SET_DEFAULT(value->signedValue));
  case ZoneOption::String: return new TextEdit(parent, rect, value->stringValue, sizeof(value->stringValue));
  case ZoneOption::File: return new FileChoice(parent, rect, BITMAPS_PATH, BITMAPS_EXT, sizeof(value->stringValue),
                          [=]() { return std::string(value->stringValue, ZLEN(value->stringValue)); },
                          [=](std::string newValue) { strncpy(value->stringValue, newValue.c_str(), sizeof(value->stringValue)); SET_DIRTY(); }, false);
  case ZoneOption::TextSize: return new Choice(parent, rect, "\010StandardTiny\0   Small\0  Mid\0    Double", 0, 4, GET_SET_DEFAULT(value->unsignedValue));
  case ZoneOption::Timer: {
    auto timerchoice = new Choice(parent, rect, nullptr, 0, TIMERS - 1, GET_SET_DEFAULT(value->unsignedValue));
    timerchoice->setTextHandler([](int32_t value) {
      return std::string(STR_TIMER) + std::to_string(value + 1);
    });
    return timerchoice;
  }
  case ZoneOption::Source: return new SourceChoice(parent, rect, 1, MIXSRC_LAST_TELEM, GET_SET_DEFAULT(value->unsignedValue));
  case ZoneOption::Color: {
    new ColorEdit(parent, rect, GET_SET_DEFAULT(value->unsignedValue));
  }
  default: return nullptr;
  }

  return nullptr;
}

void ScreenThemePage::build(Window * window)
{
  GridLayout grid;
  grid.setLabelWidth(LCD_W/2);
  grid.spacer(8);

#if 0
  // Theme choice
  new StaticText(window, grid.getLabelSlot(), STR_THEME);
  Theme * new_theme = editThemeChoice<Theme>(SCREENS_SETUP_2ND_COLUMN, y, getRegisteredThemes(), theme, needsOffsetCheck, attr, event);
  if (new_theme) {
    new_theme->init();
    loadTheme(new_theme);
    strncpy(g_eeGeneral.themeName, new_theme->getName(), sizeof(g_eeGeneral.themeName));
    killEvents(KEY_ENTER);
    storageDirty(EE_GENERAL);
  }
#endif

  // Theme options
  const ZoneOption * options = theme->getOptions();
  int optionsCount = getOptionsCount(options);
  for (int index = 0; index < optionsCount; index++) {
    const ZoneOption * option = &options[index];
    ZoneOptionValue * value = theme->getOptionValue(index);
    ThemeValue[index] = g_eeGeneral.themeData.options[index];
    new StaticText(window, grid.getLabelSlot(), option->name);
    Window* result = createOptionEdit(window, grid.getFieldSlot(), option, value);
    //theme->update();
    grid.nextLine();
  }
  // Topbar customization
  new TextButton(window, grid.getLineSlot(), STR_TOP_BAR, []() -> uint8_t {
                   new WidgetsSetupPage(static_cast<WidgetsContainerInterface*>(topbar), 0, MENU_TITLE_COLOR, 2, 1);
                   return 0;
  });
  grid.nextLine();

  window->setInnerHeight(grid.getWindowHeight());
}

