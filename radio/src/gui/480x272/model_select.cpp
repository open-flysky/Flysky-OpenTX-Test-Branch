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

#include <algorithm>
#include "model_select.h"
#include "opentx.h"
#include "libwindows.h"
#include "storage/modelslist.h"

#define CATEGORIES_WIDTH               120
#define MODELS_LEFT                    123
#define MODELS_COLUMN_WIDTH            174

enum ModelSelectMode {
  MODE_SELECT_MODEL,
  MODE_RENAME_CATEGORY,
  MODE_MOVE_MODEL,
};

enum ModelDeleteMode {
  MODE_DELETE_MODEL,
  MODE_DELETE_CATEGORY,
};

uint8_t selectMode, deleteMode;
ModelsList modelslist;

ModelsCategory * currentCategory;
int currentCategoryIndex;
ModelCell * currentModel;

void drawCategory(coord_t y, const char * name, bool selected)
{
  if (selected) {
    lcdDrawSolidFilledRect(1, y-INVERT_VERT_MARGIN, CATEGORIES_WIDTH-10, INVERT_LINE_HEIGHT+2, TEXT_INVERTED_BGCOLOR);
    lcdDrawText(6, y, name, TEXT_COLOR | INVERS);
  }
  else {
    lcdDrawText(6, y, name, TEXT_COLOR);
  }
}

void drawModel(coord_t x, coord_t y, ModelCell * model, bool current, bool selected)
{
  lcd->drawBitmap(x+1, y+1, model->getBuffer());
  if (current) {
    lcd->drawBitmapPattern(x+66, y+43, LBM_ACTIVE_MODEL, TITLE_BGCOLOR);
  }
  if (selected) {
    lcdDrawSolidRect(x, y, MODELCELL_WIDTH+2, MODELCELL_HEIGHT+2, 1, TITLE_BGCOLOR);
    drawShadow(x, y, MODELCELL_WIDTH+2, MODELCELL_HEIGHT+2);
    if (selectMode == MODE_MOVE_MODEL) {
      lcd->drawMask(x+MODELCELL_WIDTH+2-modelselModelMoveBackground->getWidth(), y, modelselModelMoveBackground, TITLE_BGCOLOR);
      lcd->drawMask(x+MODELCELL_WIDTH+2-modelselModelMoveBackground->getWidth()+12, y+5, modelselModelMoveIcon, TEXT_BGCOLOR);
    }
  }
}

uint16_t categoriesVerticalOffset = 0;
uint16_t categoriesVerticalPosition = 0;
#define MODEL_INDEX()       (menuVerticalPosition*2+menuHorizontalPosition)

#if 0
void setCurrentModel(unsigned int index)
{
  auto it = currentCategory->begin();
  std::advance(it, index);
  currentModel = *it;
}
#endif


void setCurrentCategory(unsigned int index)
{
  currentCategoryIndex = index;
  std::list<ModelsCategory *>::iterator it = modelslist.categories.begin();
  std::advance(it, index);
  currentCategory = *it;
  categoriesVerticalPosition = index;
  categoriesVerticalOffset = limit<int>(categoriesVerticalPosition-4, categoriesVerticalOffset, min<int>(categoriesVerticalPosition, max<int>(0, modelslist.categories.size()-5)));
  /*if (currentCategory->size() > 0)
    setCurrentModel(0);
  else
    currentModel = NULL;*/
}

void initModelsList()
{
  modelslist.load();

  categoriesVerticalOffset = 0;
  bool found = false;
  int index = 0;
  for (std::list<ModelsCategory *>::iterator it = modelslist.categories.begin(); it != modelslist.categories.end(); ++it, ++index) {
    if (*it == modelslist.currentCategory) {
      setCurrentCategory(index);
      found = true;
      break;
    }
  }
  if (!found) {
    setCurrentCategory(0);
  }

  menuVerticalOffset = 0;
  found = false;
  index = 0;
  for (ModelsCategory::iterator it = currentCategory->begin(); it != currentCategory->end(); ++it, ++index) {
    if (*it == modelslist.currentModel) {
      // setCurrentModel(index);
      found = true;
      break;
    }
  }
  if (!found) {
    // setCurrentModel(0);
  }
}
class ModelselectPage;

class ModelselectButton: public Button {
  public:
    ModelselectButton(ModelselectPage* page, Window * parent, const rect_t & rect, ModelCell * modelCell, Window * footer);

    virtual void paint(BitmapBuffer * dc) override {
      drawSolidRect(dc, 0, 0, rect.w, rect.h, 2, hasFocus() ? SCROLLBOX_COLOR : CURVE_AXIS_COLOR);
      dc->drawBitmap(10, 2, modelCell->getBuffer());
      if (modelCell == modelslist.currentModel) {
        dc->drawBitmapPattern(112, 71, LBM_ACTIVE_MODEL, TITLE_BGCOLOR);
      }
    }

    const char * modelFilename() {
      return modelCell->modelFilename;
    }

  protected:
    ModelCell * modelCell;
};

#if defined(LUA)

#define MAX_WIZARD_NAME_LEN            (sizeof(WIZARD_PATH)+20)
#define WIZARD_ICON_H                  130
#define WIZARD_ICON_W                  85
#define WIZARD_ICON_SPACE              10

uint8_t getWizardCount()
{
  uint8_t wizCnt=0;
  DIR dir;
  static FILINFO fno;

  FRESULT res = f_opendir(&dir, WIZARD_PATH);
  if (res == FR_OK) {
    for (;;) {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) {
        break;
      }
      if (fno.fattrib & AM_DIR) {
        wizCnt++;
      }
    }
  }
  f_closedir(&dir);
  return wizCnt;
}

class WizardButton : public Window {
public:
  WizardButton(Window * parent, const rect_t & rect, const char* dirPath) :
    Window(parent, rect)
  {
    strcpy(wizardDirPath, dirPath);
    strcpy(&wizardDirPath[strlen(wizardDirPath)], "/icon.png");
    if(isFileAvailable(wizardDirPath)){
      background = BitmapBuffer::load(wizardDirPath);
    }
    strcpy(wizardDirPath, dirPath);
  }
  virtual bool onTouchEnd(coord_t x, coord_t y) override
  {
    char script[MAX_WIZARD_NAME_LEN];
    size_t len = strlen(wizardDirPath);
    strcpy(script, wizardDirPath);
    script[len] = '/';
    strcpy(&script[len + 1], WIZARD_NAME);
    if (isFileAvailable(script)) {
      f_chdir(wizardDirPath);
      luaExec(WIZARD_NAME);
    }
    return true;
  }

  virtual void paint(BitmapBuffer * dc) override {
    if(background!=nullptr) dc->drawBitmap(0, 0, background);
    dc->drawRect(0,0, WIZARD_ICON_W, WIZARD_ICON_H, 2, SOLID, hasFocus() ? SCROLLBOX_COLOR : CURVE_AXIS_COLOR);
  }
protected:
  BitmapBuffer * background;
  char wizardDirPath[MAX_WIZARD_NAME_LEN];
};

class WizardsList: public Window {
  public:
  WizardsList(Window* window) : Window(window, {window->width() / 2 - 100, window->height() / 2 - 30, 200, 0}, OPAQUE)
  {
    DIR dir;
    static FILINFO fno;
    char wizpath[MAX_WIZARD_NAME_LEN];
    strcpy(wizpath, WIZARD_PATH);
    strcpy(&wizpath[sizeof(WIZARD_PATH) - 1], "/");

    int index = 0;
    coord_t pos_x;
    coord_t pos_y;

    int itemsPerRow = (rect.w - WIZARD_ICON_SPACE) / (WIZARD_ICON_W + WIZARD_ICON_SPACE);
    int column = 0;
    int row = 0;
    FRESULT res = f_opendir(&dir, WIZARD_PATH);
    if (res == FR_OK) {
      for (uint8_t wizidx = 0;; wizidx++) {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0) {
          break;
        }
        if (fno.fattrib & AM_DIR) {
          strcpy(&wizpath[sizeof(WIZARD_PATH)], fno.fname);
          column = index % itemsPerRow;
          row = index / itemsPerRow;
          pos_x = WIZARD_ICON_SPACE * (column + 1) + WIZARD_ICON_W * column;
          pos_y = WIZARD_ICON_SPACE + (row * (WIZARD_ICON_H + WIZARD_ICON_SPACE));
          index++;
          new WizardButton(this, { pos_x, pos_y, WIZARD_ICON_W, WIZARD_ICON_H }, wizpath);
        }
      }
    }
    row = ((row + 1) * (WIZARD_ICON_H + WIZARD_ICON_SPACE)) + WIZARD_ICON_SPACE;
    this->adjustInnerHeight();
    this->setHeight(row);
    this->setTop((window->height() - row) / 2);
  }


  virtual bool onTouchEnd(coord_t x, coord_t y) override
  {
    if(Window::onTouchEnd(x,y)) return false;
    return true;
  }
  virtual void paint(BitmapBuffer * dc) override {
    dc->clear(TEXT_BGCOLOR);
  }
};

class WizardSelectWindow: public Window {
public:
  WizardSelectWindow() :
      Window(&mainWindow, { 0, 0, LCD_W, LCD_H }, TRANSPARENT) {
    new WizardsList(this);
  }

  bool onTouchEnd(coord_t x, coord_t y) override {
    bool result = Window::onTouchEnd(x, y);
    clear();
    deleteLater();
    return result;
  }

  bool onTouchStart(coord_t x, coord_t y) override { return true; }

  bool onTouchSlide(coord_t x, coord_t y, coord_t startX, coord_t startY,
      coord_t slideX, coord_t slideY) override {
    Window::onTouchSlide(x, y, startX, startY, slideX, slideY);
    return true;
  }
};
#endif
class ModelselectFooter: public Window {
  public:
    ModelselectFooter(Window * parent, const rect_t & rect):
      Window(parent, rect)
    {
    }

    virtual void paint(BitmapBuffer * dc) override {
      dc->drawSolidFilledRect(0, 5, rect.w, 2, CURVE_AXIS_COLOR);
      dc->drawBitmap(7, 12, modelselSdFreeBitmap);
      uint32_t size = sdGetSize() / 100;
      drawNumber(dc, 24, 11, size, PREC1|SMLSIZE, 0, NULL, "GB");
      dc->drawBitmap(77, 12, modelselModelQtyBitmap);
      drawNumber(dc, 99, 11, modelslist.modelsCount, SMLSIZE);
      ModelselectButton * selectedModel = dynamic_cast<ModelselectButton *>(focusWindow);
      if (selectedModel) {
        dc->drawBitmap(7, 37, modelselModelNameBitmap);
        dc->drawText(24, 32, selectedModel->modelFilename(), SMLSIZE | TEXT_COLOR);
      }
    }
};

class ModelselectPage: public PageTab {
  private:
    Window* body;
    Window* footer;
    uint8_t showMenuAddModel() {
      auto menu = new Menu();
      menu->addLine(STR_CREATE_MODEL, [&]() {
        storageCheck(true);
        modelslist.setCurrentModel(modelslist.addModel(currentCategory, createModel()));
#if defined(LUA)
        if(getWizardCount()) {
          new WizardSelectWindow();
        }
#endif
        updateModels(currentCategory->size() - 1);
      });
      return 1;
    }
  public:
    ModelselectPage() :
      PageTab(STR_MODEL_SELECT, ICON_MODEL_SELECT)
    {
    }

    void updateModels(int selected=-1) {
    
      this->body->clear();
      int index = 0;
      for (auto it = currentCategory->begin(); it != currentCategory->end(); ++it, ++index) {
        Button * button = new ModelselectButton(this, this->body, {10, 10 + index * 104, LCD_W - 20, 94}, *it, footer);
        if (selected == index) {
          button->setFocus();
        }
      }
      auto addButton = new Button(this->body, {10, 10 + index * 104, LCD_W - 20, 94});
      addButton->setPressHandler(std::bind(&ModelselectPage::showMenuAddModel, this));
      body->adjustInnerHeight();
    }
    virtual void build(Window * window) override
    {
      initModelsList();
      this->body = new Window(window, {0, 0, LCD_W, window->height() - 55});
      this->footer = new ModelselectFooter(window, {0, window->height() - 55, LCD_W, 55});
      updateModels();
    }
};

ModelselectMenu::ModelselectMenu():
  TabsGroup()
{
  addTab(new ModelselectPage());
}

ModelselectButton::ModelselectButton(ModelselectPage* page, Window * parent, const rect_t & rect, ModelCell * modelCell, Window * footer):

  Button(parent, rect,
         [=]() -> uint8_t {
           if (hasFocus()) {
             Menu * menu = new Menu();
             if (modelCell && modelCell != modelslist.currentModel) {
               menu->addLine(STR_SELECT_MODEL, [=]() {
                 // we store the latest changes if any
                 storageFlushCurrentModel();
                 storageCheck(true);
                 memcpy(g_eeGeneral.currModelFilename, modelCell->modelFilename, LEN_MODEL_FILENAME);
                 loadModel(g_eeGeneral.currModelFilename, false);
                 storageDirty(EE_GENERAL);
                 storageCheck(true);
                 // chainMenu(menuMainView);
                 postModelLoad(true);
                 modelslist.setCurrentModel(modelCell);
                 page->updateModels(modelslist.getModelIndex(modelCell));
               });
             }
             menu->addLine(STR_CREATE_MODEL, [=]() {
               storageCheck(true);
               modelslist.setCurrentModel(modelslist.addModel(currentCategory, createModel()));
#if defined(LUA)
               if(getWizardCount()) {
                 new WizardSelectWindow();
               }
#endif
               page->updateModels(currentCategory->size() - 1);
             });
             if (modelCell) {
               menu->addLine(STR_DUPLICATE_MODEL, [=]() {
                 char duplicatedFilename[LEN_MODEL_FILENAME + 1];
                 memcpy(duplicatedFilename, modelCell->modelFilename, sizeof(duplicatedFilename));
                 if (findNextFileIndex(duplicatedFilename, LEN_MODEL_FILENAME, MODELS_PATH)) {
                   sdCopyFile(modelCell->modelFilename, MODELS_PATH, duplicatedFilename, MODELS_PATH);
                   modelslist.addModel(currentCategory, duplicatedFilename);
                   page->updateModels(currentCategory->size() - 1);
                 }
                 else {
                   POPUP_WARNING("Invalid File");
                 }
               });
             }
             // menu->addLine(STR_MOVE_MODEL);
             if (modelCell && modelCell != modelslist.currentModel) {
               menu->addLine(STR_DELETE_MODEL, [=]() {
                 // POPUP_CONFIRMATION(STR_DELETEMODEL);
                 // SET_WARNING_INFO(modelCell->modelName, LEN_MODEL_NAME, 0);
                 unsigned int index = modelslist.getModelIndex(modelCell);
                 if (index > 0)
                   --index;
                 modelslist.removeModel(currentCategory, modelCell);
                 page->updateModels(index);
               });
             }
           }
           else {
             setFocus();
             footer->invalidate();
           }
           return 1;
         }, BUTTON_CHECKED_ON_FOCUS),
  modelCell(modelCell)
{
}
