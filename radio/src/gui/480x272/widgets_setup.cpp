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

#include "widgets_setup.h"
//defined in screen theme - far from OO design

Window * createOptionEdit(Window * parent, const rect_t &rect, const ZoneOption * option, ZoneOptionValue * value);

WidgetConfigPage::WidgetConfigPage(Widget* widget) :
PageTab(STR_WIDGET_SETTINGS, ICON_THEME_SETUP),
widget(widget)
{
}

void WidgetConfigPage::build(Window * window) {
  GridLayout grid;
  grid.spacer(8);
  grid.setLabelWidth(LCD_W/2);
  const ZoneOption * options = widget->getOptions();
  for (int option=0; ; option++) {
    if(!options || !options[option].name) break;
    addOption(window, grid, options[option], widget->getOptionValue(option));
  }
}

void WidgetConfigPage::addOption(Window * window, GridLayout& grid, const ZoneOption& option, ZoneOptionValue * value) {
  new StaticText(window, grid.getLabelSlot(), std::string(option.name));
  createOptionEdit(window, grid.getFieldSlot(), &option, value);
  grid.nextLine();
}

WidgetsSetupPage::WidgetsSetupPage(WidgetsContainerInterface* container, uint8_t index, LcdFlags color, int padding, int thickness):
  ViewMain(),
  container(container),
  index(index),
  color(color),
  padding(padding),
  thickness(thickness)
{
}

void WidgetsSetupPage::setWidget(unsigned int zone, Widget* widget){
  Widget* old = container->getWidget(zone);
  if(old) delete old;
  container->setWidget(zone, widget);
  storageDirty(EE_MODEL);
}
void WidgetsSetupPage::displayWidgetConfig(Widget* widget)
{
  if(widget && widget->getFactory()->getOptions()) {
    TabsGroup* tabsGroup = new TabsGroup();
    tabsGroup->addTab(new WidgetConfigPage(widget));
  }
}

void WidgetsSetupPage::showSelectWidgetMenu(unsigned int zone)
{
  Menu* menu = new Menu ();
  menu->addLine (TR_NONE, [=]() { setWidget(zone, NULL); });
  for (auto it = getRegisteredWidgets().cbegin(); it != getRegisteredWidgets().cend(); ++it) {
    menu->addLine ((*it)->getName(), [=]() {
      Widget* widget = loadWidget((*it)->getName(), container->getZone(zone), new Widget::PersistentData());
      setWidget(zone, widget);
      displayWidgetConfig(widget);
    });
  }

}
void WidgetsSetupPage::showWidgetMenu(unsigned int zone)
{
  Widget* widget = container->getWidget(zone);
  if(widget) {
    Menu * menu = new Menu ();
    menu->addLine (STR_SELECT_WIDGET, [=]() { showSelectWidgetMenu(zone); });
    if (widget->getFactory()->getOptions()) menu->addLine (STR_WIDGET_SETTINGS, [=]() { displayWidgetConfig (widget); });
    menu->addLine (STR_REMOVE_WIDGET, [=]() { setWidget(zone, NULL); });
  }
  else showSelectWidgetMenu(zone);
}

bool WidgetsSetupPage::onTouchEnd(coord_t x, coord_t y)
{
  for (unsigned int index=0; index < container->getZonesCount(); index++) {
    Zone zone = container->getZone(index);
    if(x >= zone.x && x <= (zone.x + zone.w) && y >= zone.y && y <= (zone.y + zone.h)){
      showWidgetMenu(index);
    }
  }
  return true;
}

bool WidgetsSetupPage::onTouchSlide(coord_t x, coord_t y, coord_t startX, coord_t startY, coord_t slideX, coord_t slideY)
{
  if(slideDirection == SlideDirection::None) {
    if (startX < x && (x - startX > LCD_W / 3)){
      slideDirection = SlideDirection::Right;
      this->detach();
      return true;
    }
  }
  return true;
}

uint8_t WidgetsSetupPage::currentView() {
  return this->index;
}

void WidgetsSetupPage::paint(BitmapBuffer * dc)
{
  ViewMain::paint(dc);
  for (int i=container->getZonesCount()-1; i>=0; i--) {
    Zone zone = container->getZone(i);
    lcdDrawRect(zone.x-padding, zone.y-padding, zone.w+2*padding, zone.h+2*padding, thickness, 0x3F, color);
  }
}

