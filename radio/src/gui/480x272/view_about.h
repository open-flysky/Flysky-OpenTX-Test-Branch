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
#ifndef _VIEW_ABOUT_H_
#define _VIEW_ABOUT_H_

#include "tabsgroup.h"
#include "opentx.h"
#include "libwindows.h"


class AboutBody : public Window {
	public:
	 AboutBody(Window * parent, const rect_t &rect) :
      Window(parent, rect)
    {
      
    }
	void paint(BitmapBuffer * dc) override;
};

class AboutPage : public PageTab {
  public:
    AboutPage() :
      PageTab(TR_ABOUTUS, ICON_RADIO_VERSION)
    {
    }
    void build(Window * window) override
    {
      new AboutBody(window, {0, 0, LCD_W, window->height()});
    }
};


class AboutMenu: public TabsGroup {
  public:
    AboutMenu():TabsGroup()
  {
    addTab(new AboutPage());
  }
};

#endif // _VIEW_ABOUT_H_