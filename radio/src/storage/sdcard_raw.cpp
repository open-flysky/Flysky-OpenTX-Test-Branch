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

void getModelPath(char * path, const char * filename)
{
  strcpy(path, STR_MODELS_PATH);
  path[sizeof(MODELS_PATH)-1] = '/';
  strcpy(&path[sizeof(MODELS_PATH)], filename);
}

const char * writeFile(const char * filename, const uint8_t * data, uint16_t size)
{
  TRACE("writeFile(%s)", filename);
  
  FIL file;
  unsigned char buf[8];
  UINT written;

  FRESULT result = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
  if (result != FR_OK) {
    return SDCARD_ERROR(result);
  }

  *(uint32_t*)&buf[0] = OTX_FOURCC;
  buf[4] = EEPROM_VER;
  buf[5] = 'M';
  *(uint16_t*)&buf[6] = size;

  result = f_write(&file, buf, 8, &written);
  if (result != FR_OK || written != 8) {
    f_close(&file);
    return SDCARD_ERROR(result);
  }

  result = f_write(&file, data, size, &written);
  if (result != FR_OK || written != size) {
    f_close(&file);
    return SDCARD_ERROR(result);
  }

  f_close(&file);
  return NULL;
}

const char * writeModel()
{
  char path[256];
  getModelPath(path, g_eeGeneral.currModelFilename);
  return writeFile(path, (uint8_t *)&g_model, sizeof(g_model));
}

const char * loadFile(const char * filename, uint8_t * data, uint16_t maxsize)
{
  TRACE("loadFile(%s)", filename);
  
  FIL file;
  char buf[8];
  UINT read;

  FRESULT result = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
  if (result != FR_OK) {
    return SDCARD_ERROR(result);
  }

  if (f_size(&file) < 8) {
    f_close(&file);
    return STR_INCOMPATIBLE;
  }

  result = f_read(&file, (uint8_t *)buf, 8, &read);
  if (result != FR_OK || read != 8) {
    f_close(&file);
    return SDCARD_ERROR(result);
  }

  uint8_t version = (uint8_t)buf[4];
  if ((*(uint32_t*)&buf[0] != OTX_FOURCC && *(uint32_t*)&buf[0] != O9X_FOURCC) || version < FIRST_CONV_EEPROM_VER || version > EEPROM_VER || buf[5] != 'M') {
    f_close(&file);
    return STR_INCOMPATIBLE;
  }

  uint16_t size = min<uint16_t>(maxsize, *(uint16_t*)&buf[6]);
  result = f_read(&file, data, size, &read);
  if (result != FR_OK || read != size) {
    f_close(&file);
    return SDCARD_ERROR(result);
  }

  f_close(&file);
  return NULL;
}

const char * readModel(const char * filename, uint8_t * buffer, uint32_t size)
{
  char path[256];
  getModelPath(path, filename);
  return loadFile(path, buffer, size);
}

const char * loadModel(const char * filename, bool alarms)
{
  preModelLoad();

  const char * error = readModel(filename, (uint8_t *)&g_model, sizeof(g_model));
  if (error) {
    TRACE("loadModel error=%s", error);
  }
  
  if (error) {
    modelDefault(0) ;
    storageCheck(true);
    alarms = false;
  }

  postModelLoad(alarms);

  return error;
}

const char * loadRadioSettingsSettings()
{
  const char * error = loadFile(RADIO_SETTINGS_PATH, (uint8_t *)&g_eeGeneral, sizeof(g_eeGeneral));
  if (error) {
    TRACE("loadRadioSettingsSettings error=%s", error);
  }
  // TODO this is temporary, we only have one model for now
  return error;
}

const char * writeGeneralSettings()
{
  return writeFile(RADIO_SETTINGS_PATH, (uint8_t *)&g_eeGeneral, sizeof(g_eeGeneral));
}

void storageCheck(bool immediately)
{
  if (storageDirtyMsk & EE_GENERAL) {
    TRACE("eeprom write general");
    storageDirtyMsk -= EE_GENERAL;
    const char * error = writeGeneralSettings();
    if (error) {
      TRACE("writeGeneralSettings error=%s", error);
    }
  }

  if (storageDirtyMsk & EE_MODEL) {
    TRACE("eeprom write model");
    storageDirtyMsk -= EE_MODEL;
    const char * error = writeModel();
    if (error) {
      TRACE("writeModel error=%s", error);
    }
  }
}

void storageReadAll()
{
  TRACE("storageReadAll");

  if (loadRadioSettingsSettings() != NULL) {
    storageEraseAll(true);
  }

#if defined(CPUARM)
  for (uint8_t i=0; languagePacks[i]!=NULL; i++) {
    if (!strncmp(g_eeGeneral.ttsLanguage, languagePacks[i]->id, 2)) {
      currentLanguagePackIdx = i;
      currentLanguagePack = languagePacks[i];
    }
  }
#endif

  if (loadModel(g_eeGeneral.currModelFilename, false) != NULL) {
    sdCheckAndCreateDirectory(MODELS_PATH);
    createModel();
  }
}

void storageCreateModelsList()
{
  FIL file;

  FRESULT result = f_open(&file, RADIO_MODELSLIST_PATH, FA_CREATE_ALWAYS | FA_WRITE);
  if (result == FR_OK) {
    f_puts("[" DEFAULT_CATEGORY "]\n" DEFAULT_MODEL_FILENAME "\n", &file);
    f_close(&file);
  }
}

void storageFormat()
{
  sdCheckAndCreateDirectory(RADIO_PATH);
  sdCheckAndCreateDirectory(MODELS_PATH);
  storageCreateModelsList();
}

const char * createModel()
{
  preModelLoad();

  char filename[LEN_MODEL_FILENAME+1];
  memset(filename, 0, sizeof(filename));
  strcpy(filename, "model.bin");

  int index = findNextFileIndex(filename, LEN_MODEL_FILENAME, MODELS_PATH);
  if (index > 0) {
    modelDefault(index);
    memcpy(g_eeGeneral.currModelFilename, filename, sizeof(g_eeGeneral.currModelFilename));
    storageDirty(EE_GENERAL);
    storageDirty(EE_MODEL);
    storageCheck(true);
  }
  postModelLoad(false);

  return g_eeGeneral.currModelFilename;
}

void storageEraseModels()
{
  DIR dir;
  FILINFO fno;
  char fullFileName[255];
  size_t len = strlen(MODELS_PATH);
  strcpy(fullFileName, MODELS_PATH);
  fullFileName[len++] = '/';

  FRESULT res = f_opendir(&dir, MODELS_PATH);
  if (res == FR_OK) {
    for (;;) {
      res = f_readdir(&dir, &fno);
      if (res != FR_OK || fno.fname[0] == 0) {
        break;
      }
      if (fno.fattrib & (AM_RDO | AM_DIR | AM_HID | AM_SYS)) {
        continue;
      }
      strcpy(fullFileName + len, fno.fname);
      f_unlink(fullFileName);
    }
  }
}
void storageEraseAll(bool warn)
{
  TRACE("storageEraseAll");

#if defined(COLORLCD)
  // the theme has not been loaded before
  theme->load();
#endif

  generalDefault();
  modelDefault(1);

  if (warn) {
    ALERT(STR_STORAGE_WARNING, STR_BAD_RADIO_DATA, AU_BAD_RADIODATA);
  }

  RAISE_ALERT(STR_STORAGE_WARNING, STR_STORAGE_FORMAT, NULL, AU_NONE);
  storageEraseModels();
  storageFormat();
  storageDirty(EE_GENERAL|EE_MODEL);
  storageCheck(true);
}
