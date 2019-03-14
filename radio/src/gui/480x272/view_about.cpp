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

enum AboutScreens {
  ABOUT_OPENTX,
  ABOUT_HARDWARE,
  ABOUT_BERTRAND,
  ABOUT_ANDRE,
  ABOUT_MIKE,
  ABOUT_KJELL,
  ABOUT_MARTIN,
  ABOUT_ROMOLO,
  ABOUT_ROB,
  ABOUT_PARENTS,
  ABOUT_END,
  ABOUT_COUNT,
};

#define ABOUT_X      5
#define ABOUT_INDENT 16
#include "tabsgroup.h"
#include "opentx.h"
#include "libwindows.h"
#include "view_about.h"

struct aboutLine {
  const  pm_char* text;
  const  LcdFlags flags;
};

const aboutLine aboutLines[] {
	{STR_ABOUT_OPENTX_1, SMLSIZE},
	{STR_ABOUT_OPENTX_2, SMLSIZE},
	{STR_ABOUT_OPENTX_3, SMLSIZE},
	{STR_ABOUT_OPENTX_4, SMLSIZE},
	{STR_ABOUT_OPENTX_5, SMLSIZE},
	{STR_ABOUT_HARDWARE_1, SMLSIZE | INVERS},
	{STR_ABOUT_HARDWARE_2, SMLSIZE},
	{STR_ABOUT_HARDWARE_3, SMLSIZE},
	{STR_ABOUT_BERTRAND_1, SMLSIZE | INVERS },
	{STR_ABOUT_BERTRAND_2, SMLSIZE},
	{STR_ABOUT_BERTRAND_3, SMLSIZE},
	{STR_ABOUT_ANDRE_1, SMLSIZE | INVERS },
	{STR_ABOUT_ANDRE_2, SMLSIZE},
	{STR_ABOUT_ANDRE_3, SMLSIZE},
	{STR_ABOUT_MIKE_1, SMLSIZE | INVERS },
	{STR_ABOUT_MIKE_2, SMLSIZE},
	{STR_ABOUT_MIKE_3, SMLSIZE},
	{STR_ABOUT_MIKE_4, SMLSIZE},
	{STR_ABOUT_KJELL_1, SMLSIZE | INVERS },
	{STR_ABOUT_KJELL_2, SMLSIZE},
	{STR_ABOUT_KJELL_3, SMLSIZE},
	{STR_ABOUT_KJELL_4, SMLSIZE},
	{STR_ABOUT_MARTIN_1, SMLSIZE  | INVERS},
	{STR_ABOUT_MARTIN_2, SMLSIZE},
	{STR_ABOUT_ROMOLO_1, SMLSIZE  | INVERS},
	{STR_ABOUT_ROMOLO_2, SMLSIZE},
	{STR_ABOUT_ROB_1, SMLSIZE | INVERS},
	{STR_ABOUT_ROB_2, SMLSIZE},
	{STR_ABOUT_PARENTS_1, SMLSIZE | INVERS},
	{STR_ABOUT_PARENTS_2, SMLSIZE},
	{STR_ABOUT_PARENTS_3, SMLSIZE},
	{STR_ABOUT_PARENTS_4, SMLSIZE},
};

void AboutBody::paint(BitmapBuffer * dc)
{
  dc->clear(TEXT_BGCOLOR);
  int index = 0;
  bool isInvers = false;

  for (auto &line : aboutLines) // access by reference to avoid copying
  {  
	isInvers = (line.flags&INVERS) != 0;
	if(isInvers) index++;
	dc->drawText(isInvers ? ((width() - getTextWidth(line.text, strlen(line.text), line.flags)) / 2) : ABOUT_X, 20 + (index++)*FH, line.text, line.flags);
	if(isInvers) index++;
  }
  //setHeight(20 + (index++)*FH);
  setInnerHeight(20 + (index++)*FH);
}