#include "opentx.h"
#include "../../common/arm/stm32/bootloader/boot.h"
#include "../../common/arm/stm32/bootloader/bin_files.h"

#define SELECTED_COLOR (INVERS | TEXT_COLOR)
#define DEFAULT_PADDING 28
#define DOUBLE_PADDING  56
#define MESSAGE_TOP     (LCD_H - (2*DOUBLE_PADDING))
//#include "bmp_plug_usb.lbm"
//#include "bmp_usb_plugged.lbm"

const uint8_t LBM_FLASH[] = {
#include "icon_flash.lbm"
};

const uint8_t LBM_EXIT[] = {
#include "icon_exit.lbm"
};

const uint8_t LBM_SD[] = {
#include "icon_sd.lbm"
};

const uint8_t LBM_FILE[] = {
#include "icon_file.lbm"
};

const uint8_t LBM_ERROR[] = {
#include "icon_error.lbm"
};

const uint8_t LBM_OK[] = {
#include "icon_ok.lbm"
};

void bootloaderInitScreen()
{
  lcdColorTable[TEXT_COLOR_INDEX]      = BLACK;
  lcdColorTable[TEXT_BGCOLOR_INDEX]    = WHITE;
  lcdColorTable[LINE_COLOR_INDEX]      = RED;
  lcdColorTable[BARGRAPH1_COLOR_INDEX] = RED;
  lcdColorTable[BARGRAPH2_COLOR_INDEX] = RGB(73, 219, 62); // green
 
  backlightEnable(BACKLIGHT_LEVEL_MAX);
}

static void bootloaderDrawTitle(const char* text)
{
    lcdDrawText(LCD_W/2, DEFAULT_PADDING, text, CENTERED | TEXT_COLOR);
    lcdDrawSolidFilledRect(DEFAULT_PADDING, DOUBLE_PADDING, LCD_W - DOUBLE_PADDING, 2, TEXT_COLOR);
}

static void bootloaderDrawFooter()
{
    lcdDrawSolidFilledRect(DEFAULT_PADDING, LCD_H - (DOUBLE_PADDING + 4), LCD_W - DOUBLE_PADDING, 2, TEXT_COLOR);
}

void bootloaderDrawScreen(BootloaderState st, int opt, const char* str)
{
    // clear screen
    lcdDrawSolidFilledRect(0, 0, LCD_W, LCD_H, TEXT_BGCOLOR);
    int center = LCD_W/2;
    if (st == ST_START) {

        bootloaderDrawTitle("NV14 BOOTLOADER");
        
        lcdDrawBitmapPattern(50, 72, LBM_FLASH, TEXT_COLOR);
        lcdDrawText(84,  75, "Write Firmware");

        lcdDrawBitmapPattern(50, 107, LBM_EXIT, TEXT_COLOR);
        lcdDrawText(84, 110, "Exit");

        lcdDrawSolidRect(79, (opt == 0) ? 72 : 107, LCD_W - 79 - 28, 26, 2, LINE_COLOR);
        
        //lcd->drawBitmap(60, 166, &BMP_PLUG_USB);
        lcdDrawText(center, 175, "Or plug in a USB cable", CENTERED | TEXT_COLOR);
        lcdDrawText(center, 200, "for mass storage", CENTERED | TEXT_COLOR);

        bootloaderDrawFooter();
        lcdDrawText(center, LCD_H - DOUBLE_PADDING, "Current Firmware:", CENTERED | TEXT_COLOR);
        lcdDrawText(center, LCD_H - DEFAULT_PADDING, getOtherVersion(nullptr), CENTERED | TEXT_COLOR);
    }
    else if (st == ST_USB) {
        //lcd->drawBitmap(136, 98, &BMP_USB_PLUGGED);
        lcdDrawText(center, 128, "USB Connected", CENTERED | TEXT_COLOR);
    }
    else if (st == ST_FILE_LIST || st == ST_DIR_CHECK || st == ST_FLASH_CHECK ||
             st == ST_FLASHING || st == ST_FLASH_DONE) {

        bootloaderDrawTitle("SD>FIRMWARE");
        lcdDrawBitmapPattern(DEFAULT_PADDING, 16, LBM_SD, TEXT_COLOR);

        if (st == ST_FLASHING || st == ST_FLASH_DONE) {

            LcdFlags color = BARGRAPH1_COLOR; // red

            if (st == ST_FLASH_DONE) {
                color = BARGRAPH2_COLOR/* green */;
                opt   = 100; // Completed > 100%
            }

            lcdDrawRect(DEFAULT_PADDING, 120, LCD_W - DOUBLE_PADDING, 31, 2);
            lcdDrawSolidFilledRect(DEFAULT_PADDING+4, 124, ((LCD_W - DOUBLE_PADDING - 8) * opt) / 100, 23, color);
        }
        else if (st == ST_DIR_CHECK) {

            if (opt == FR_NO_PATH) {
                lcdDrawText(20, MESSAGE_TOP, "Directory is missing");
            }
            else {
                lcdDrawText(20, MESSAGE_TOP, "Directory is empty");
            }

            lcdDrawBitmapPattern(LCD_W - DOUBLE_PADDING, MESSAGE_TOP-10, LBM_ERROR, BARGRAPH1_COLOR);
        }
        else if (st == ST_FLASH_CHECK) {

            bootloaderDrawFilename(str, 0, true);

            if (opt == FC_ERROR) {
                lcdDrawText(20, MESSAGE_TOP, STR_INVALID_FIRMWARE);
                lcdDrawBitmapPattern(LCD_W - DOUBLE_PADDING, MESSAGE_TOP-10, LBM_ERROR, BARGRAPH1_COLOR);
            }
            else if (opt == FC_OK) {
                VersionTag tag;
                extractFirmwareVersion(&tag);

                lcdDrawText(LCD_W/4 + DEFAULT_PADDING, MESSAGE_TOP, "Version:", RIGHT);
                lcdDrawText(LCD_W/4 + 6 + DEFAULT_PADDING, MESSAGE_TOP, tag.version);
                
                lcdDrawText(LCD_W/4 + DEFAULT_PADDING, MESSAGE_TOP + DEFAULT_PADDING, "Radio:", RIGHT);
                lcdDrawText(LCD_W/4 + 6 + DEFAULT_PADDING, MESSAGE_TOP + DEFAULT_PADDING, tag.flavour);
                
                lcdDrawBitmapPattern(LCD_W - DOUBLE_PADDING, MESSAGE_TOP-10, LBM_OK, BARGRAPH2_COLOR);
            }
        }
        
        bootloaderDrawFooter();

        if ( st != ST_DIR_CHECK && (st != ST_FLASH_CHECK || opt == FC_OK)) {

            lcdDrawBitmapPattern(DEFAULT_PADDING, LCD_H - DOUBLE_PADDING - 2, LBM_FLASH, TEXT_COLOR);

            if (st == ST_FILE_LIST) {
                lcdDrawText(DOUBLE_PADDING, LCD_H - DOUBLE_PADDING, "[R TRIM] to select file");
            }
            else if (st == ST_FLASH_CHECK && opt == FC_OK) {
                lcdDrawText(DOUBLE_PADDING, LCD_H - DOUBLE_PADDING, "Hold [R TRIM] long to flash");
            }
            else if (st == ST_FLASHING) {
                lcdDrawText(DOUBLE_PADDING, LCD_H - DOUBLE_PADDING, "Writing Firmware ...");
            }
            else if (st == ST_FLASH_DONE) {
                lcdDrawText(DOUBLE_PADDING, LCD_H - DOUBLE_PADDING, "Writing Completed");
            }
        }

        if (st != ST_FLASHING) {
            lcdDrawBitmapPattern(DEFAULT_PADDING, LCD_H - DEFAULT_PADDING - 2, LBM_EXIT, TEXT_COLOR);
            lcdDrawText(DOUBLE_PADDING, LCD_H - DEFAULT_PADDING, "[L TRIM] to exit");
        }        
    }
}

void bootloaderDrawFilename(const char* str, uint8_t line, bool selected)
{
    lcdDrawBitmapPattern(DEFAULT_PADDING, 76 + (line * 25), LBM_FILE, TEXT_COLOR);
    lcdDrawText(DEFAULT_PADDING + 30, 75 + (line * 25), str);

    if (selected) {
        lcdDrawSolidRect(DEFAULT_PADDING + 25, 72 + (line * 25), LCD_W - (DEFAULT_PADDING + 25) - 28, 26, 2, LINE_COLOR);
    }
}
