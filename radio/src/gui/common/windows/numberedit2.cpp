#include "numberedit2.h"
#include "strhelpers.h"

NumberEdit2::NumberEdit2(Window * parent, const rect_t & rect,
    int32_t vmin, int32_t vmax, int32_t minChoice, int32_t maxChoice, const char* label, const char * values, int buttonWidth,
    std::function<int32_t()> getValue, std::function<void(int32_t)> setValue, LcdFlags flags) :
    NumberEdit(parent, { rect.x, rect.y, rect.w - buttonWidth, rect.h }, vmin, vmax, getValue, setValue, flags),
    Choice(parent, { rect.x, rect.y, rect.w - buttonWidth, rect.h }, values, minChoice, maxChoice, getValue, setValue, flags) {

  textButton = new TextButton(parent, { rect.x + rect.w - buttonWidth, rect.y, buttonWidth, rect.h }, std::string(label));
  textButton->setPressHandler(std::bind(&NumberEdit2::onButtonCheck, this));
  int32_t value = getValue();
  //set checked if value out of numedit range
  checked = (value > NumberEdit::vmax) || (value < NumberEdit::vmin);
  textButton->check(checked);
  if (checked) Choice::bringToTop();
  else NumberEdit::bringToTop();
}

bool NumberEdit2::onTouchEnd(coord_t x, coord_t y) {
  if(checked) return Choice::onTouchEnd(x,y);
  return NumberEdit::onTouchEnd(x,y);
}

void NumberEdit2::paint(BitmapBuffer * dc) {
  if(checked) Choice::paint(dc);
  else NumberEdit::paint(dc);
}

void NumberEdit2::checkEvents() {
  if(checked) Choice::checkEvents();
  else NumberEdit::checkEvents();
}

uint8_t NumberEdit2::onButtonCheck() {
  checked = !textButton->checked();
  setOutputType();
  return checked;
}

void NumberEdit2::setOutputType() {
  if (checked) {
    Choice::setValue(Choice::vmin);
    Choice::invalidate();
    Choice::bringToTop();
  }
  else{
    NumberEdit::setValue((NumberEdit::vmin + NumberEdit::vmax)/2);
    NumberEdit::invalidate();
    NumberEdit::bringToTop();
  }
}

#define GVARS
#if defined(GVARS)
GvarNumberEdit::GvarNumberEdit(Window * parent, const rect_t & rect, int32_t vmin, int32_t vmax,
    std::function<int32_t()> getValue, std::function<void(int32_t)> setValue, LcdFlags flags) :
    NumberEdit2(parent, rect, vmin, vmax, vmin - MAX_GVARS, vmax + MAX_GVARS, TR_GV2, nullptr, 40, getValue, setValue, flags),
    flags(flags)
    {
    Choice::setAvailableHandler(std::bind(&GvarNumberEdit::isGvarValue, this, std::placeholders::_1));
    Choice::setTextHandler(std::bind(&GvarNumberEdit::getGVarName, this, std::placeholders::_1));
}
bool GvarNumberEdit::isGvarValue(int value) {
  return (value > NumberEdit::vmax) || (value < NumberEdit::vmin);
}

std::string GvarNumberEdit::getGVarName(int32_t value) {
  char buffer[16];
  char* pos = buffer;
  int idx = value - NumberEdit::vmax - 1;
  if (value <= NumberEdit::vmin) {
    idx = value - NumberEdit::vmin;
  }
  if (idx < 0) {
    *pos++ = '-';
    idx = -idx-1;
  }

  if (ZEXIST(g_model.gvars[idx].name))
    zchar2str(pos, g_model.gvars[idx].name, LEN_GVAR_NAME);
  else
    strAppendStringWithIndex(pos, TR_GV2, idx+1);

  return std::string(buffer);
}

void GvarNumberEdit::setOutputType() {

  if (checked) {
    Choice::setValue(NumberEdit::vmax+1);
    Choice::invalidate();
    Choice::bringToTop();
  }
  else{
    int value = Choice::getValue();
    int idx = value - NumberEdit::vmax - 1;
    int mul = 1;
    if (value <= NumberEdit::vmin) {
       idx = value - NumberEdit::vmin;
       mul = -1;
    }

    if (flags & PREC1) {
      mul *= 10;
    }
    value = getGVarValue(idx, mixerCurrentFlightMode);
    TRACE("GVAR value %d = %d", value*mul);
    NumberEdit::setValue(value*mul);
    NumberEdit::invalidate();
    NumberEdit::bringToTop();
  }
}
#endif

