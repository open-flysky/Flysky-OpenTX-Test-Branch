---- #########################################################################
---- #                                                                       #
---- # Copyright (C) OpenTX                                                  #
-----#                                                                       #
---- # License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html               #
---- #                                                                       #
---- # This program is free software; you can redistribute it and/or modify  #
---- # it under the terms of the GNU General Public License version 2 as     #
---- # published by the Free Software Foundation.                            #
---- #                                                                       #
---- # This program is distributed in the hope that it will be useful        #
---- # but WITHOUT ANY WARRANTY; without even the implied warranty of        #
---- # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
---- # GNU General Public License for more details.                          #
---- #                                                                       #
---- #########################################################################
local VALUE = 0
local COMBO = 1
local BUTTON = 2
local NAV_BUTTON_TOP = 235
local NAV_BUTTON_BOTTOM = 300
local NAV_BUTTON_NEXT_LEFT = 285
local NAV_BUTTON_PREV_RIGHT = 35
local FIELD_HEIGHT = 30
local TEXT_LEFT = 10
local FIELD_LEFT = 20

local edit = false
local page = 1
local current = 1
local pages = {}
local fields = {}

-- load common Bitmaps
local ImgMarkBg = Bitmap.open("img/mark_bg.png")
local BackgroundImg = Bitmap.open("img/background.png")
local ImgPlane = Bitmap.open("img/plane.png")
local ImgPrev = Bitmap.open("img/pageup.png")
local ImgNext = Bitmap.open("img/pagedn.png")


-- Change display attribute to current field
local function addField(step)
  local field = fields[current]
  local min, max
  if field[3] == VALUE then
    min = field[6]
    max = field[7]
  elseif field[3] == COMBO then
    min = 0
    max = #(field[6]) - 1
  end
  if (step < 0 and field[5] > min) or (step > 0 and field[5] < max) then
    field[5] = field[5] + step
  end
end
local function setMinMin(setMin)
  local field = fields[current]
  local min, max
  if field[3] == VALUE then
    min = field[6]
    max = field[7]
  elseif field[3] == COMBO then
    min = 0
    max = #(field[6]) - 1
  end
  if setMin == true then
    field[5] = min
  else
    field[5] = max
  end
end

-- Select the next or previous page
local function selectPage(step)
  page = 1 + ((page + step - 1 + #pages) % #pages)
  edit = false
  current = 1
end

-- Select the next or previous editable field
local function selectField(step)
  repeat
    current = 1 + ((current + step - 1 + #fields) % #fields)
  until fields[current][4]==1
end

-- Redraw the current page
local function redrawFieldsPage(event)

  for index = 1, 10, 1 do
    local field = fields[index]
    if field == nil then
      break
    end

    local attr = current == (index) and ((edit == true and BLINK or 0) + INVERS) or 0
    attr = attr + TEXT_COLOR
     lcd.setColor(CUSTOM_COLOR, lcd.RGB(255, 255, 255))
    if field[4] == 1 then
      if field[3] == VALUE then
        lcd.drawFilledRectangle(field[1]-10, field[2]-5, field[7], FIELD_HEIGHT, CUSTOM_COLOR)
        lcd.drawNumber(field[1], field[2], field[5], LEFT + attr)
      elseif field[3] == COMBO then
        lcd.drawFilledRectangle(field[1]-10, field[2]-5, field[7], FIELD_HEIGHT, CUSTOM_COLOR)
        if field[5] >= 0 and field[5] < #(field[6]) then
          lcd.drawText(field[1],field[2], field[6][1+field[5]], attr)
        end
      elseif field[3] == BUTTON then
        lcd.drawFilledRectangle(TEXT_LEFT, field[2] - 5, field[7], FIELD_HEIGHT, CURVE_AXIS_COLOR)
        if current == (index) then
          lcd.drawRectangle(TEXT_LEFT, field[2] - 5, field[7], FIELD_HEIGHT, SCROLLBOX_COLOR, 2)
        end
        lcd.drawText((field[1] + field[7]) / 2, field[2], field[5], field[6])
      end
    end
  end
end

local function updateField(field)
  local value = field[5]
end
--  {FIELD_LEFT, 50, COMBO, 1, 2, { "None", "One, or two with Y cable", "Two"}, 300 },
local function getField(x, y)
   for i = 1, #fields do
    local field = fields[i]
    local field_left = field[1] - 10
    local field_top = field[2] - 5
    local field_right = field_left + field[7]
    local field_bottom = field_top + FIELD_HEIGHT
    -- visible
    if field[4] == 1 and x >= field_left and x <= field_right and y >= field_top and y <= field_bottom then
      return i
    end
  end
  return -1
end
-- Main
local function runFieldsPage(event, x, y)
  if event == EVT_EXIT_BREAK then -- exit script
    return 2
  elseif event == EVT_TOUCH_UP then
    local index = getField(x,y)
    if (index ~= -1) then
      if fields[index][3] ~= BUTTON then
        edit = true
        current = index
        lcd.showKeyboard(KEYBOARD_NUM_INC_DEC)
      else
        return fields[index][8]()
      end
    else
      edit = false
      lcd.showKeyboard(KEYBOARD_NONE)
    end
  elseif event == EVT_ENTER_BREAK or event == EVT_ROT_BREAK then -- toggle editing/selecting current field
    if fields[current][5] ~= nil then
      edit = not edit
      if edit == false then
        updateField(fields[current])
      end
    end
  elseif edit then
    if event == EVT_PLUS_FIRST or event == EVT_ROT_RIGHT or event == EVT_PLUS_REPT or event == EVT_VK_INC or event == EVT_VK_INC_LARGE then
      addField(1)
    elseif event == EVT_MINUS_FIRST or event == EVT_ROT_LEFT or event == EVT_MINUS_REPT or event == EVT_VK_DEC or event == EVT_VK_DEC_LARGE then
      addField(-1)
    elseif event == EVT_VK_MIN then
      setMinMin(true)
    elseif event == EVT_VK_MAX then
      setMinMin(false)
    elseif event == EVT_VK_DEFAULT then
      setMinMin(true)
    end
  else
    if event == EVT_MINUS_FIRST or event == EVT_ROT_RIGHT then
      selectField(1)
    elseif event == EVT_PLUS_FIRST or event == EVT_ROT_LEFT then
      selectField(-1)
    end
  end
  redrawFieldsPage(event)
  return 0
end
-- set visibility flags starting with SECOND field of fields
local function setFieldsVisible(...)
  local arg={...}
  local cnt = 2
  for i,v in ipairs(arg) do
    fields[cnt][4] = v
    cnt = cnt + 1
  end
end

-- draws one letter mark
local function drawMark(x, y, name)
  lcd.drawBitmap(ImgMarkBg, x, y)
  lcd.drawText(x+8, y+3, name, TEXT_COLOR)
end


local MotorFields = {
  {FIELD_LEFT, 50, COMBO, 1, 1, { "No", "Yes"}, 100},
  {FIELD_LEFT, 107, COMBO, 1, 2, { "CH1", "CH2", "CH3", "CH4", "CH5", "CH6", "CH7", "CH8" }, 100},
}

local function drawNavButtons()
  lcd.drawBitmap(BackgroundImg, 0, 0)
  lcd.drawBitmap(ImgNext, NAV_BUTTON_NEXT_LEFT, NAV_BUTTON_TOP)
  lcd.drawBitmap(ImgPrev, 0, NAV_BUTTON_TOP)
end
local ImgEngine

local function runMotorConfig(event, x, y)
  lcd.clear()
  if ImgEngine == nil then
   ImgEngine = Bitmap.open("img/prop.png")
  end
  lcd.drawBitmap(BackgroundImg, 0, 0)
  lcd.drawBitmap(ImgNext, NAV_BUTTON_NEXT_LEFT, NAV_BUTTON_TOP)
  lcd.drawBitmap(ImgEngine, 80, 180)
  lcd.setColor(CUSTOM_COLOR, lcd.RGB(255, 255, 255))
  fields = MotorFields
  lcd.drawText(TEXT_LEFT, 20, "Does your model have a motor ?", TEXT_COLOR)
  --lcd.drawFilledRectangle(TEXT_LEFT, 45, 200, FIELD_HEIGHT, CUSTOM_COLOR)
  fields[2][4]=0
  if fields[1][5] == 1 then
    lcd.drawText(TEXT_LEFT, 80, "What channel is it on ?", TEXT_COLOR)
    --lcd.drawFilledRectangle(TEXT_LEFT, 122, 100, FIELD_HEIGHT, CUSTOM_COLOR)
    fields[2][4]=1
  end
  local result = runFieldsPage(event, x, y)
  return result
end

-- fields format : {[1]x, [2]y, [3]COMBO, [4]visible, [5]default, [6]{values}}
-- fields format : {[1]x, [2]y, [3]VALUE, [4]visible, [5]default, [6]min, [7]max}
-- fields format : {[1]x, [2]y, [3]TEXT,  [4]visible, [5]text}
local AilFields = {
  {FIELD_LEFT, 50, COMBO, 1, 2, { "None", "One, or two with Y cable", "Two"}, 280 },
  {FIELD_LEFT, 107, COMBO, 1, 0, { "CH1", "CH2", "CH3", "CH4", "CH5", "CH6", "CH7", "CH8" }, 100}, -- Ail1 chan
  {FIELD_LEFT, 147, COMBO, 1, 4, { "CH1", "CH2", "CH3", "CH4", "CH5", "CH6", "CH7", "CH8" }, 100 }, -- Ail2 chan
}

local ImgAilR
local ImgAilL

local function runAilConfig(event, x, y)
  lcd.clear()
  if ImgAilR == nil then
    ImgAilR = Bitmap.open("img/rail.png")
    ImgAilL = Bitmap.open("img/lail.png")
  end
  drawNavButtons()
  lcd.drawBitmap(ImgPlane, 52, 200)
  fields = AilFields
  if fields[1][5] == 1 then
    lcd.drawBitmap(ImgAilR, 124, 223)
    lcd.drawBitmap(ImgAilL, 75, 315)
    drawMark(163, 232, "A")
    drawMark(102, 327, "A")
    --lcd.drawFilledRectangle(TEXT_LEFT, 122, 100, FIELD_HEIGHT, CUSTOM_COLOR)
    drawMark(132, 104, "A")
    setFieldsVisible(1, 0)
  elseif fields[1][5] == 2 then
    lcd.drawBitmap(ImgAilR, 124, 223)
    lcd.drawBitmap(ImgAilL, 75, 315)
    drawMark(163, 232, "A")
    drawMark(102, 327, "B")
    --lcd.drawFilledRectangle(TEXT_LEFT, 122, 100, FIELD_HEIGHT, CUSTOM_COLOR)
    drawMark(132, 104, "A")
    --lcd.drawFilledRectangle(TEXT_LEFT, 162, 100, FIELD_HEIGHT, CUSTOM_COLOR)
    drawMark(132, 144, "B")
    setFieldsVisible(1, 1)
  else
    setFieldsVisible(0, 0)
  end
  lcd.drawText(TEXT_LEFT, 20, "Number of ailerons on your model ?", TEXT_COLOR)
  --lcd.drawFilledRectangle(TEXT_LEFT, 45, 300, FIELD_HEIGHT, CUSTOM_COLOR)
  local result = runFieldsPage(event, x, y)
  return result
end

local FlapsFields = {
  {FIELD_LEFT, 50, COMBO, 1, 0, { "No", "Yes, on one channel", "Yes, on two channels"}, 280 },
  {FIELD_LEFT, 107, COMBO, 1, 6, { "CH1", "CH2", "CH3", "CH4", "CH5", "CH6", "CH7", "CH8" }, 100 },
  {FIELD_LEFT, 147, COMBO, 1, 7, { "CH1", "CH2", "CH3", "CH4", "CH5", "CH6", "CH7", "CH8" }, 100 },
}

local ImgFlp

local function runFlapsConfig(event, x, y)
  lcd.clear()
  if ImgFlp == nil then
    ImgFlp = Bitmap.open("img/flap.png")
  end
  drawNavButtons()
  lcd.drawBitmap(ImgPlane, 52, 200)
  fields = FlapsFields
  if fields[1][5] == 1 then
    lcd.drawBitmap(ImgFlp, 115, 240)
    lcd.drawBitmap(ImgFlp, 85, 296)
    drawMark(163, 232, "A")
    drawMark(102, 327, "A")
    --lcd.drawFilledRectangle(TEXT_LEFT, 122, 100, FIELD_HEIGHT, CUSTOM_COLOR)
    drawMark(152, 104, "A")
    setFieldsVisible(1, 0)
  elseif fields[1][5] == 2 then
    lcd.drawBitmap(ImgFlp, 115, 240)
    lcd.drawBitmap(ImgFlp, 85, 296)
    drawMark(163, 232, "A")
    drawMark(102, 327, "B")
    --lcd.drawFilledRectangle(TEXT_LEFT, 122, 100, FIELD_HEIGHT, CUSTOM_COLOR)
    drawMark(152, 104, "A")
    --lcd.drawFilledRectangle(TEXT_LEFT, 162, 100, FIELD_HEIGHT, CUSTOM_COLOR)
    drawMark(152, 144, "B")
    setFieldsVisible(1, 1)
  else
    setFieldsVisible(0, 0)
  end
  lcd.drawText(TEXT_LEFT, 20, "Does your model have flaps ?", TEXT_COLOR)
  --lcd.drawFilledRectangle(TEXT_LEFT, 45, 300, FIELD_HEIGHT, CUSTOM_COLOR)
  local result = runFieldsPage(event, x, y)
  return result
end

local TailFields = {
  {FIELD_LEFT, 50, COMBO, 1, 1, { "1 CH Elevator, no Rudder", "1 CH Elevator, 1 CH Rudder", "2 CHs for Elevator, 1 CH Rudder", "V Tail"}, 280 },
  {FIELD_LEFT, 107, COMBO, 1, 1, { "CH1", "CH2", "CH3", "CH4", "CH5", "CH6", "CH7", "CH8" }, 100 }, --ele
  {FIELD_LEFT, 147, COMBO, 1, 3, { "CH1", "CH2", "CH3", "CH4", "CH5", "CH6", "CH7", "CH8" }, 100 }, --rud
  {FIELD_LEFT, 187, COMBO, 0, 5, { "CH1", "CH2", "CH3", "CH4", "CH5", "CH6", "CH7", "CH8" }, 100 }, --ele2
}

local ImgTail
local ImgVTail
local ImgTailRud

local function runTailConfig(event, x, y)
  lcd.clear()
  if ImgTail == nil then
    ImgTail = Bitmap.open("img/tail.png")
    ImgVTail = Bitmap.open("img/vtail.png")
    ImgTailRud = Bitmap.open("img/tailrud.png")
  end
  drawNavButtons()
  fields = TailFields
  if fields[1][5] == 0 then
    lcd.drawBitmap(ImgTail, 52, 220)
    drawMark(88, 240, "A")
    drawMark(195, 275, "A")
    drawMark(152, 104, "A")
    setFieldsVisible(1, 0, 0)
  end
  if fields[1][5] == 1 then
    lcd.drawBitmap(ImgTail, 52, 220)
    lcd.drawBitmap(ImgTailRud, 140, 220)
    drawMark(88, 240, "A")
    drawMark(195, 275, "A")
    drawMark(170, 230, "B")
    drawMark(152, 104, "A")
    drawMark(152, 144, "B")
    setFieldsVisible(1, 1, 0)
  end
  if fields[1][5] == 2 then
    lcd.drawBitmap(ImgTail, 52, 220)
    lcd.drawBitmap(ImgTailRud, 140, 220)
    drawMark(88, 240, "A")
    drawMark(195, 275, "C")
    drawMark(170, 230, "B")
    drawMark(152, 104, "A")
    drawMark(152, 144, "B")
    drawMark(152, 184, "C")
    setFieldsVisible(1, 1, 1)
  end
  if fields[1][5] == 3 then
    lcd.drawBitmap(ImgVTail, 52, 220)
    drawMark(115, 230, "A")
    drawMark(182, 240, "B")
    drawMark(152, 104, "A")
    drawMark(152, 144, "B")
    setFieldsVisible(1, 1, 0)
  end
  lcd.drawText(TEXT_LEFT, 20, "Pick the tail config of your model", TEXT_COLOR)
  --lcd.drawFilledRectangle(TEXT_LEFT, 45, 300, FIELD_HEIGHT, CUSTOM_COLOR)
  local result = runFieldsPage(event, x, y)
  return result
end

local lineIndex
local function drawNextLine(text, text2)
  lcd.drawText(TEXT_LEFT, lineIndex, text, TEXT_COLOR)
  lcd.drawText(TEXT_LEFT + 170, lineIndex, text2 + 1, TEXT_COLOR)
  lineIndex = lineIndex + 20
end
local function selectFirstPage()
  page = 1
  edit = false
  current = 1
  return 0
end

local function addMix(channel, input, name, weight, index)
  local mix = { source=input, name=name }
  if weight ~= nil then
    mix.weight = weight
  end
  if index == nil then
    index = 0
  end
  model.insertMix(channel, index, mix)
end

local function saveModel()
  model.defaultInputs()
  model.deleteMixes()
  -- motor
  if(MotorFields[1][5] == 1) then
    addMix(MotorFields[2][5], MIXSRC_FIRST_INPUT+defaultChannel(2), "Motor")
  end
  -- Ailerons
  if(AilFields[1][5] == 1) then
    addMix(AilFields[2][5], MIXSRC_FIRST_INPUT+defaultChannel(3), "Ail")
  elseif (AilFields[1][5] == 2) then
    addMix(AilFields[2][5], MIXSRC_FIRST_INPUT+defaultChannel(3), "AilL")
    addMix(AilFields[3][5], MIXSRC_FIRST_INPUT+defaultChannel(3), "AilR", -100)
  end
  -- Flaps
  if(FlapsFields[1][5] == 1) then
    addMix(FlapsFields[2][5], MIXSRC_SA, "Flaps")
  elseif (FlapsFields[1][5] == 2) then
    addMix(FlapsFields[2][5], MIXSRC_SA, "FlapsL")
    addMix(FlapsFields[3][5], MIXSRC_SA, "FlapsR")
  end
  -- Tail
  if(TailFields[1][5] == 0) then
    addMix(TailFields[2][5], MIXSRC_FIRST_INPUT+defaultChannel(1), "Elev")
  elseif (TailFields[1][5] == 1) then
    addMix(TailFields[2][5], MIXSRC_FIRST_INPUT+defaultChannel(1), "Elev")
    addMix(TailFields[3][5], MIXSRC_FIRST_INPUT+defaultChannel(0), "Rudder")
  elseif (TailFields[1][5] == 2) then
    addMix(TailFields[2][5], MIXSRC_FIRST_INPUT+defaultChannel(1), "ElevL")
    addMix(TailFields[3][5], MIXSRC_FIRST_INPUT+defaultChannel(0), "Rudder")
    addMix(TailFields[4][5], MIXSRC_FIRST_INPUT+defaultChannel(1), "ElevR")
  elseif (TailFields[1][5] == 3) then
    addMix(TailFields[2][5], MIXSRC_FIRST_INPUT+defaultChannel(1), "V-EleL", 50)
    addMix(TailFields[2][5], MIXSRC_FIRST_INPUT+defaultChannel(0), "V-RudL", 50, 1)
    addMix(TailFields[3][5], MIXSRC_FIRST_INPUT+defaultChannel(1), "V-EleR", 50)
    addMix(TailFields[3][5], MIXSRC_FIRST_INPUT+defaultChannel(0), "V-RudR", -50, 1)
  end
  selectPage(1)
  return 0
end
local ConfigSummaryFields = {
  {FIELD_LEFT, 395, BUTTON, 1, "No, I need to change something", 4, 300, selectFirstPage },
  {FIELD_LEFT, 435, BUTTON, 1,  "Yes, all is well, create the plane !", 4, 300, saveModel },
}

local ImgSummary

local function runConfigSummary(event, x, y)
  lcd.clear()
  if ImgSummary == nil then
    ImgSummary = Bitmap.open("img/summary.png")
  end
  fields = ConfigSummaryFields
  lcd.drawBitmap(BackgroundImg, 0, 0)
  lcd.drawBitmap(ImgPrev, 0, NAV_BUTTON_TOP)
  lcd.drawBitmap(ImgSummary, 160, 200)
  lineIndex = 40
  -- motors
  if(MotorFields[1][5] == 1) then
    drawNextLine("Motor channel:", MotorFields[2][5])
  end
  -- ail
  if(AilFields[1][5] == 1) then
    drawNextLine("Aileron channel:",AilFields[2][5])
  elseif (AilFields[1][5] == 2) then
    drawNextLine("Aileron 1 channel:",AilFields[2][5])
    drawNextLine("Aileron 2 channel:",AilFields[3][5])
  end
  -- flaps
  if(FlapsFields[1][5] == 1) then
    drawNextLine("Flaps channel:",FlapsFields[2][5])
  elseif (FlapsFields[1][5] == 2) then
    drawNextLine("Flaps 1 channel:",FlapsFields[2][5])
    drawNextLine("Flaps 2 channel:",FlapsFields[3][5])
  end
  -- tail
  if(TailFields[1][5] == 0) then
    drawNextLine("Elevator channel:",TailFields[2][5])
  elseif (TailFields[1][5] == 1) then
    drawNextLine("Elevator channel:",TailFields[2][5])
    drawNextLine("Rudder channel:",TailFields[3][5])
  elseif (TailFields[1][5] == 2) then
    drawNextLine("Elevator 1 channel:",TailFields[2][5])
    drawNextLine("Rudder channel:",TailFields[3][5])
    drawNextLine("Elevator 2 channel:",TailFields[4][5])
  elseif (TailFields[1][5] == 3) then
    drawNextLine("V-Tail elevator:", TailFields[2][5])
    drawNextLine("V-Tail rudder:", TailFields[3][5])
  end
  local result = runFieldsPage(event, x, y)
  return result
end

local function closeScript()
  return 2
end

local DoneFields = {
  {FIELD_LEFT, 400, BUTTON, 1, "Press to exit", 4, 300, closeScript }
}

local function createModel(event, x, y)
  fields = DoneFields
  lcd.clear()
  lcd.drawBitmap(BackgroundImg, 0, 0)
  lcd.drawBitmap(ImgSummary, 120, 60)
  lcd.drawText(TEXT_LEFT, 245, "Model successfully created !", TEXT_COLOR)
  local result = runFieldsPage(event, x, y)
  return result
end

-- Init
local function init()
  current, edit = 1, false
  pages = {
    runMotorConfig,
    runAilConfig,
    runFlapsConfig,
    runTailConfig,
    runConfigSummary,
    createModel,
  }
end

-- Main
local function run(event, x, y)
  if event == nil then
    error("Cannot be run as a model script!")
    return 2
  elseif (event == EVT_PAGE_BREAK or event == EVT_PAGEDN_FIRST or event == EVT_SLIDE_LEFT or (event == EVT_TOUCH_UP and x >= NAV_BUTTON_NEXT_LEFT  and y >= NAV_BUTTON_TOP  and y <= NAV_BUTTON_BOTTOM )) and page < #pages-1 then
    selectPage(1)
  elseif (event == EVT_PAGE_LONG or event == EVT_PAGEUP_FIRST or event == EVT_SLIDE_RIGHT or (event == EVT_TOUCH_UP and x <= NAV_BUTTON_PREV_RIGHT  and y >= NAV_BUTTON_TOP  and y <= NAV_BUTTON_BOTTOM )) and page > 1 then
    killEvents(event);
    selectPage(-1)
  end

  local result = pages[page](event, x, y)
  return result
end

return { init=init, run=run }
