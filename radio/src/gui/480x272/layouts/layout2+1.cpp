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

const uint8_t LBM_LAYOUT_2P1[] = {
#include "mask_layout2+1.lbm"
};

const ZoneOption OPTIONS_LAYOUT_2P1[] = {
  { Layout::TopBar, ZoneOption::Bool },
  { Layout::Navigation, ZoneOption::Bool },
  { Layout::FlightMode, ZoneOption::Bool },
  { Layout::Sliders, ZoneOption::Bool },
  { Layout::Trims, ZoneOption::Bool },
  { NULL, ZoneOption::Bool }
};

class Layout2P1: public Layout
{
  public:
    Layout2P1(const LayoutFactory * factory, Layout::PersistentData * persistentData):
      Layout(factory, persistentData, 3)
    {
    }
    Zone getZone(unsigned int index) const override
    {
      Zone zone = Layout::getZone(index);
      zone.w -= margin(); //additional margin
      zone.w /= 2; //half of space
      if(index == 0) zone.x += zone.w + margin(); //right column
      else {
        zone.h = zone.h/2 - margin()/2; // half of height on left
        if (index == 2) zone.y += zone.h + margin(); //offset top on left
      }
      return zone;
    }
};

BaseLayoutFactory<Layout2P1> layout2P1("Layout2P1", LBM_LAYOUT_2P1, OPTIONS_LAYOUT_2P1);
const LayoutFactory * defaultLayout = &layout2P1;
