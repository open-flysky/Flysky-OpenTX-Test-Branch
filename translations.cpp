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
#include "translations.h"

#define ISTR(x) LEN_##x TR_##x
// The non-0-terminated-strings
const pm_char STR_OPEN9X[] PROGMEM =
    ISTR(OFFON)
    ISTR(MMMINV)
    ISTR(NCHANNELS)
#if !defined(GRAPHICS)
    ISTR(VBEEPLEN)
#endif
    ISTR(VBEEPMODE)
#if defined(ROTARY_ENCODERS)
    ISTR(VRENAVIG)
#endif
#if defined(ROTARY_ENCODER_NAVIGATION)
    ISTR(VRENCODERS)
#endif
    ISTR(TRNMODE)
    ISTR(TRNCHN)
#if defined(PCBFRSKY) || defined(PCBFLYSKY)
    ISTR(UART3MODES)
    ISTR(SWTYPES)
    ISTR(POTTYPES)
    ISTR(SLIDERTYPES)
#endif
    ISTR(VTRIMINC)
#if defined(CPUARM)
    ISTR(VDISPLAYTRIMS)
#endif
    ISTR(RETA123)
    ISTR(VPROTOS)
    ISTR(POSNEG)
#if defined(PCBSKY9X) && defined(REVX)
    ISTR(VOUTPUT_TYPE)
#endif
    ISTR(VBLMODE)
    ISTR(VCURVEFUNC)
    ISTR(VMLTPX)
    ISTR(VMLTPX2)
    ISTR(VMIXTRIMS)
    ISTR(VCSWFUNC)
    ISTR(VFSWFUNC)
    ISTR(VFSWRESET)
    ISTR(FUNCSOUNDS)
#if !defined(CPUARM)
    ISTR(VTELEMCHNS)
#endif
#if defined(TELEMETRY_FRSKY) || defined(CPUARM)
    ISTR(VTELEMUNIT)
    ISTR(VALARM)
    ISTR(VALARMFN)
    ISTR(VTELPROTO)
    ISTR(GPSFORMAT)
    ISTR(AMPSRC)
    ISTR(VARIOSRC)
    ISTR(VTELEMSCREENTYPE)
#endif
#if defined(TEMPLATES)
    ISTR(VTEMPLATES)
#endif
#if defined(HELI)
    ISTR(VSWASHTYPE)
#endif
    ISTR(VKEYS)
    ISTR(VSWITCHES)
    ISTR(VSRCRAW)
#if defined(TRANSLATIONS_CZ) && defined(CPUARM)
    ISTR(INPUTNAMES)
#endif
    ISTR(VTMRMODES)
#if defined(CPUM2560) || defined(CPUARM)
    ISTR(DATETIME)
    ISTR(VPERSISTENT)
#endif
#if defined(CPUARM)
    ISTR(VLCD)
    ISTR(VUNITSSYSTEM)
    ISTR(VBEEPCOUNTDOWN)
    ISTR(COUNTDOWNVALUES)
    ISTR(VVARIOCENTER)
#endif
#if defined(PXX) || defined(CPUARM)
    ISTR(COUNTRYCODES)
    ISTR(USBMODES)
    ISTR(USBMODESELECT)
    ISTR(VFAILSAFE)
#endif
#if defined(CPUARM)
    ISTR(VTRAINERMODES)
    ISTR(MODULE_PROTOCOLS)
    ISTR(R9M_MODES)
    ISTR(R9M_FCC_POWER_VALUES)
    ISTR(R9M_LBT_POWER_VALUES)
    ISTR(TELEMETRY_PROTOCOLS)
    ISTR(XJT_PROTOCOLS)
    ISTR(FLYSKY_PROTOCOLS)
    ISTR(DSM_PROTOCOLS)
#if defined(MULTIMODULE)
    ISTR(MULTI_PROTOCOLS)
#endif
    ISTR(VOLTSRC)
    ISTR(CURVE_TYPES)
    ISTR(VSENSORTYPES)
    ISTR(VFORMULAS)
    ISTR(VPREC)
    ISTR(VCELLINDEX)
#if defined(BLUETOOTH)
    ISTR(BLUETOOTH_MODES)
#endif
    ISTR(VANTENNATYPES)
#endif
#if defined(TELEMETRY_MAVLINK)
    ISTR(MAVLINK_BAUDS)
    ISTR(MAVLINK_AC_MODES)
    ISTR(MAVLINK_AP_MODES)
#endif
    ;

// The 0-terminated-strings
const pm_char STR_POPUPS[] PROGMEM = TR_POPUPS;
#if !defined(OFS_EXIT)
  const pm_char STR_EXIT[] PROGMEM = TR_EXIT;
#endif
#if !defined(PCBHORUS)
  const pm_char STR_MENUWHENDONE[] PROGMEM = TR_MENUWHENDONE;
#endif
const pm_char STR_FREE[] PROGMEM = TR_FREE;
const pm_char STR_DELETEMODEL[] PROGMEM = TR_DELETEMODEL;
const pm_char STR_COPYINGMODEL[] PROGMEM = TR_COPYINGMODEL;
const pm_char STR_MOVINGMODEL[] PROGMEM = TR_MOVINGMODEL;
const pm_char STR_LOADINGMODEL[] PROGMEM = TR_LOADINGMODEL;
const pm_char STR_NAME[] PROGMEM = TR_NAME;
const pm_char STR_BITMAP[] PROGMEM = TR_BITMAP;
const pm_char STR_TIMER[] PROGMEM = TR_TIMER;
const pm_char STR_ELIMITS[] PROGMEM = TR_ELIMITS;
const pm_char STR_ETRIMS[] PROGMEM = TR_ETRIMS;
const pm_char STR_TRIMINC[] PROGMEM = TR_TRIMINC;
const pm_char STR_DISPLAY_TRIMS[] PROGMEM = TR_DISPLAY_TRIMS;
const pm_char STR_TTRACE[] PROGMEM = TR_TTRACE;
const pm_char STR_TTRIM[] PROGMEM = TR_TTRIM;
const pm_char STR_BEEPCTR[] PROGMEM = TR_BEEPCTR;
const pm_char STR_USE_GLOBAL_FUNCS[] PROGMEM = TR_USE_GLOBAL_FUNCS;
const pm_char STR_FLYSKY_TELEMETRY[] PROGMEM = TR_FLYSKY_TELEM;
#if defined(PCBSKY9X) && defined(REVX)
  const pm_char STR_OUTPUT_TYPE[] PROGMEM = TR_OUTPUT_TYPE;
#endif
const pm_char STR_PROTO[] PROGMEM = TR_PROTO;
const pm_char STR_PPMFRAME[] PROGMEM = TR_PPMFRAME;
#if defined(CPUARM)
const pm_char STR_REFRESHRATE[] PROGMEM = TR_REFRESHRATE;
const pm_char SSTR_WARN_BATTVOLTAGE[] PROGMEM = STR_WARN_BATTVOLTAGE;
#endif
const pm_char STR_MS[] PROGMEM = TR_MS;
const pm_char STR_SWITCH[] PROGMEM = TR_SWITCH;
const pm_char STR_TRIMS[] PROGMEM = TR_TRIMS;
const pm_char STR_FADEIN[] PROGMEM = TR_FADEIN;
const pm_char STR_FADEOUT[] PROGMEM = TR_FADEOUT;
const pm_char STR_DEFAULT[] PROGMEM = TR_DEFAULT;
const pm_char STR_CHECKTRIMS[] PROGMEM = TR_CHECKTRIMS;
#ifdef HELI
const pm_char STR_SWASHTYPE[] PROGMEM = TR_SWASHTYPE;
const pm_char STR_COLLECTIVE[] PROGMEM = TR_COLLECTIVE;
const pm_char STR_AILERON[] PROGMEM = TR_AILERON;
const pm_char STR_ELEVATOR[] PROGMEM = TR_ELEVATOR;
const pm_char STR_SWASHRING[] PROGMEM = TR_SWASHRING;
const pm_char STR_ELEDIRECTION[] PROGMEM = TR_ELEDIRECTION;
const pm_char STR_AILDIRECTION[] PROGMEM = TR_AILDIRECTION;
const pm_char STR_COLDIRECTION[] PROGMEM = TR_COLDIRECTION;
#endif
const pm_char STR_MODE[] PROGMEM = TR_MODE;
#if defined(AUDIO) && defined(BUZZER)
const pm_char STR_SPEAKER[] PROGMEM = TR_SPEAKER;
const pm_char STR_BUZZER[] PROGMEM = TR_BUZZER;
#endif
const pm_char STR_NOFREEEXPO[] PROGMEM = TR_NOFREEEXPO;
const pm_char STR_NOFREEMIXER[] PROGMEM = TR_NOFREEMIXER;
const pm_char STR_SOURCE[] PROGMEM = TR_SOURCE;
const pm_char STR_WEIGHT[] PROGMEM = TR_WEIGHT;
const pm_char STR_EXPO[] PROGMEM = TR_EXPO;
const pm_char STR_SIDE[] PROGMEM = TR_SIDE;
const pm_char STR_DIFFERENTIAL[] PROGMEM = TR_DIFFERENTIAL;
const pm_char STR_OFFSET[] PROGMEM = TR_OFFSET;
const pm_char STR_TRIM[] PROGMEM = TR_TRIM;
const pm_char STR_DREX[] PROGMEM = TR_DREX;
const pm_char STR_CURVE[] PROGMEM = TR_CURVE;
const pm_char STR_FLMODE[] PROGMEM = TR_FLMODE;
const pm_char STR_MIXWARNING[] PROGMEM = TR_MIXWARNING;
const pm_char STR_OFF[] PROGMEM = TR_OFF;
const pm_char STR_MULTPX[] PROGMEM = TR_MULTPX;
const pm_char STR_DELAYDOWN[] PROGMEM = TR_DELAYDOWN;
const pm_char STR_DELAYUP[] PROGMEM = TR_DELAYUP;
const pm_char STR_SLOWDOWN[] PROGMEM = TR_SLOWDOWN;
const pm_char STR_SLOWUP[] PROGMEM = TR_SLOWUP;
const pm_char STR_MIXER[] PROGMEM = TR_MIXER;
const pm_char STR_CV[] PROGMEM = TR_CV;
const pm_char STR_GV[] PROGMEM = TR_GV;
const pm_char STR_ACHANNEL[] PROGMEM = TR_ACHANNEL;
const pm_char STR_RANGE[] PROGMEM = TR_RANGE;
const pm_char STR_CENTER[] PROGMEM = TR_CENTER;
const pm_char STR_BAR[] PROGMEM = TR_BAR;
const pm_char STR_ALARM[] PROGMEM = TR_ALARM;
const pm_char STR_USRDATA[] PROGMEM = TR_USRDATA;
const pm_char STR_BLADES[] PROGMEM = TR_BLADES;
const pm_char STR_SCREEN[] PROGMEM = TR_SCREEN;
const pm_char STR_SOUND_LABEL[] PROGMEM = TR_SOUND_LABEL;
const pm_char STR_LENGTH[] PROGMEM = TR_LENGTH;
#if defined(CPUARM)
const pm_char STR_BEEP_LENGTH[] PROGMEM = TR_BEEP_LENGTH;
#endif
#if defined(AUDIO)
const pm_char STR_SPKRPITCH[] PROGMEM = TR_SPKRPITCH;
#endif
#if defined(HAPTIC)
const pm_char STR_HAPTIC_LABEL[] PROGMEM = TR_HAPTIC_LABEL;
const pm_char STR_HAPTICSTRENGTH[] PROGMEM = TR_HAPTICSTRENGTH;
#endif
const pm_char STR_CONTRAST[] PROGMEM = TR_CONTRAST;
const pm_char STR_ALARMS_LABEL[] PROGMEM = TR_ALARMS_LABEL;
#if defined(BATTGRAPH) || defined(CPUARM)
const pm_char STR_BATTERY_RANGE[] PROGMEM = TR_BATTERY_RANGE;
#endif
const pm_char STR_BATTERYWARNING[] PROGMEM = TR_BATTERYWARNING;
const pm_char STR_BATTERYCHARGING[] PROGMEM = TR_BATTERYCHARGING;
const pm_char STR_BATTERYFULL[] PROGMEM = TR_BATTERYFULL;
const pm_char STR_BATTERYNONE[] PROGMEM = TR_BATTERYNONE;
const pm_char STR_INACTIVITYALARM[] PROGMEM = TR_INACTIVITYALARM;
const pm_char STR_MEMORYWARNING[] PROGMEM = TR_MEMORYWARNING;
const pm_char STR_ALARMWARNING[] PROGMEM = TR_ALARMWARNING;
const pm_char STR_RSSISHUTDOWNALARM[] PROGMEM = TR_RSSISHUTDOWNALARM;
const pm_char STR_MODEL_STILL_POWERED[] PROGMEM = TR_MODEL_STILL_POWERED;
const pm_char STR_PRESS_ENTER_TO_CONFIRM[] PROGMEM = TR_PRESS_ENTER_TO_CONFIRM;
#if defined(ROTARY_ENCODERS)
const pm_char STR_RENAVIG[] PROGMEM = TR_RENAVIG;
#endif
const pm_char STR_THROTTLEREVERSE[] PROGMEM = TR_THROTTLEREVERSE;
const pm_char STR_TIMER_NAME[] PROGMEM = TR_TIMER_NAME;
const pm_char STR_MINUTEBEEP[] PROGMEM = TR_MINUTEBEEP;
const pm_char STR_BEEPCOUNTDOWN[] PROGMEM = TR_BEEPCOUNTDOWN;
const pm_char STR_PERSISTENT[] PROGMEM = TR_PERSISTENT;
const pm_char STR_BACKLIGHT_LABEL[] PROGMEM = TR_BACKLIGHT_LABEL;
const pm_char STR_BLDELAY[] PROGMEM = TR_BLDELAY;

#if defined(PWM_BACKLIGHT) || defined(COLORLCD)
const pm_char STR_BLONBRIGHTNESS[] PROGMEM = TR_BLONBRIGHTNESS;
const pm_char STR_BLOFFBRIGHTNESS[] PROGMEM = TR_BLOFFBRIGHTNESS;
#endif

const pm_char STR_SPLASHSCREEN[] PROGMEM = TR_SPLASHSCREEN;
const pm_char STR_THROTTLEWARNING[] PROGMEM = TR_THROTTLEWARNING;
const pm_char STR_SWITCHWARNING[] PROGMEM = TR_SWITCHWARNING;
const pm_char STR_POTWARNINGSTATE[] PROGMEM = TR_POTWARNINGSTATE;
const pm_char STR_POTWARNING[] PROGMEM = TR_POTWARNING;
const pm_char STR_SLIDERWARNING[] PROGMEM = TR_SLIDERWARNING;
#ifdef TELEMETRY_FRSKY
const pm_char STR_TIMEZONE[] PROGMEM = TR_TIMEZONE;
const pm_char STR_ADJUST_RTC[] PROGMEM = TR_ADJUST_RTC;
const pm_char STR_GPS[] PROGMEM = TR_GPS;
const pm_char STR_GPSCOORD[] PROGMEM = TR_GPSCOORD;
const pm_char STR_VARIO[] PROGMEM = TR_VARIO;
const pm_char STR_PITCH_AT_ZERO[] PROGMEM = TR_PITCH_AT_ZERO;
const pm_char STR_PITCH_AT_MAX[] PROGMEM = TR_PITCH_AT_MAX;
const pm_char STR_REPEAT_AT_ZERO[] PROGMEM = TR_REPEAT_AT_ZERO;
#endif
const pm_char STR_RXCHANNELORD[] PROGMEM = TR_RXCHANNELORD;
const pm_char STR_STICKS[] PROGMEM = TR_STICKS;
const pm_char STR_POTS[] PROGMEM = TR_POTS;
const pm_char STR_SWITCHES[] PROGMEM = TR_SWITCHES;
const pm_char STR_SWITCHES_DELAY[] PROGMEM = TR_SWITCHES_DELAY;
const pm_char STR_SLAVE[] PROGMEM = TR_SLAVE;
const pm_char STR_MODESRC[] PROGMEM = TR_MODESRC;
const pm_char STR_MULTIPLIER[] PROGMEM = TR_MULTIPLIER;
const pm_char STR_CAL[] PROGMEM = TR_CAL;
const pm_char STR_VTRIM[] PROGMEM = TR_VTRIM;
const pm_char STR_BG[] PROGMEM = TR_BG;
const pm_char STR_MENUTOSTART[] PROGMEM = TR_MENUTOSTART;
const pm_char STR_SETMIDPOINT[] PROGMEM = TR_SETMIDPOINT;
const pm_char STR_MOVESTICKSPOTS[] PROGMEM = TR_MOVESTICKSPOTS;
const pm_char STR_RXBATT[] PROGMEM = TR_RXBATT;
const pm_char STR_TX[] PROGMEM = TR_TXnRX;
const pm_char STR_ACCEL[] PROGMEM = TR_ACCEL;
const pm_char STR_NODATA[] PROGMEM = TR_NODATA;
const pm_char STR_TOTTM1TM2THRTHP[] PROGMEM = TR_TOTTM1TM2THRTHP;
const pm_char STR_TMR1LATMAXUS[] PROGMEM = TR_TMR1LATMAXUS;
const pm_char STR_TMR1LATMINUS[] PROGMEM = TR_TMR1LATMINUS;
const pm_char STR_TMR1JITTERUS[] PROGMEM = TR_TMR1JITTERUS;
const pm_char STR_TMIXMAXMS[] PROGMEM = TR_TMIXMAXMS;
const pm_char STR_FREESTACKMINB[] PROGMEM = TR_FREESTACKMINB;
const pm_char STR_MENUTORESET[] PROGMEM = TR_MENUTORESET;
const pm_char STR_PPM_TRAINER[] PROGMEM = TR_PPM_TRAINER;
const pm_char STR_CH[] PROGMEM = TR_CH;
const pm_char STR_MODEL[] PROGMEM = TR_MODEL;
const pm_char STR_FP[] PROGMEM = TR_FP;
#if defined(CPUARM)
const pm_char STR_MIX[] PROGMEM = TR_MIX;
#endif
const pm_char STR_ALERT[] PROGMEM = TR_ALERT;
const pm_char STR_PRESSANYKEYTOSKIP[] PROGMEM = TR_PRESSANYKEYTOSKIP;
const pm_char STR_THROTTLENOTIDLE[] PROGMEM = TR_THROTTLENOTIDLE;
const pm_char STR_ALARMSDISABLED[] PROGMEM = TR_ALARMSDISABLED;
const pm_char STR_PRESSANYKEY[] PROGMEM = TR_PRESSANYKEY;
const pm_char STR_TRIMS2OFFSETS[] PROGMEM = TR_TRIMS2OFFSETS;
const pm_char STR_OUTPUTS2FAILSAFE[] PROGMEM = TR_OUTPUTS2FAILSAFE;
const pm_char STR_MENURADIOSETUP[] PROGMEM = TR_MENURADIOSETUP;

#if defined(EEPROM)
const pm_char STR_BAD_RADIO_DATA[] PROGMEM = TR_BADEEPROMDATA;
const pm_char STR_STORAGE_FORMAT[] PROGMEM = TR_EEPROMFORMATTING;
const pm_char STR_STORAGE_WARNING[] PROGMEM = TR_EEPROMWARN;
const pm_char STR_EEPROM_CONVERTING[] PROGMEM = TR_EEPROM_CONVERTING;
const pm_char STR_EEPROMLOWMEM[] PROGMEM = TR_EEPROMLOWMEM;
const pm_char STR_EEPROMOVERFLOW[] PROGMEM = TR_EEPROMOVERFLOW;
#else
const pm_char STR_BAD_RADIO_DATA[] PROGMEM = TR_BAD_RADIO_DATA;
const pm_char STR_STORAGE_WARNING[] PROGMEM = TR_STORAGE_WARNING;
const pm_char STR_STORAGE_FORMAT[] PROGMEM = TR_STORAGE_FORMAT;
#endif

#if defined(FAS_OFFSET) || !defined(CPUM64)
const pm_char STR_FAS_OFFSET[] PROGMEM = TR_FAS_OFFSET;
#endif

#if defined(CPUM2560) || defined(CPUARM)
const pm_char STR_MENUDATEANDTIME[] PROGMEM = TR_MENUDATEANDTIME;
#endif

const pm_char STR_MENUTRAINER[] PROGMEM = TR_MENUTRAINER;
const pm_char STR_MENUSPECIALFUNCS[] PROGMEM = TR_MENUSPECIALFUNCS;
const pm_char STR_MENUVERSION[] PROGMEM = TR_MENUVERSION;
const pm_char STR_MENU_RADIO_SWITCHES[] PROGMEM = TR_MENU_RADIO_SWITCHES;
const pm_char STR_MENU_RADIO_ANALOGS[] PROGMEM = TR_MENU_RADIO_ANALOGS;
const pm_char STR_MENUCALIBRATION[] PROGMEM = TR_MENUCALIBRATION;

const pm_char STR_MENUMODELSEL[] PROGMEM = TR_MENUMODELSEL;
const pm_char STR_MENUSETUP[] PROGMEM = TR_MENUSETUP;
const pm_char STR_MENUFLIGHTMODE[] PROGMEM = TR_MENUFLIGHTMODE;
const pm_char STR_MENUFLIGHTMODES[] PROGMEM = TR_MENUFLIGHTMODES;


#ifdef CROSSFIRE_NATIVE
const pm_char STR_CROSSFIRE_SETUP[] PROGMEM = TR_CROSSFIRE_SETUP;
#endif

#ifdef HELI
const pm_char STR_MENUHELISETUP[] PROGMEM = TR_MENUHELISETUP;
#endif

const pm_char STR_MENUINPUTS[] PROGMEM = TR_MENUINPUTS;
const pm_char STR_MENULIMITS[] PROGMEM = TR_MENULIMITS;
const pm_char STR_MENUCURVES[] PROGMEM = TR_MENUCURVES;
const pm_char STR_MENUCURVE[] PROGMEM = TR_MENUCURVE;
const pm_char STR_MENULOGICALSWITCH[] PROGMEM = TR_MENULOGICALSWITCH;
const pm_char STR_MENULOGICALSWITCHES[] PROGMEM = TR_MENULOGICALSWITCHES;
const pm_char STR_MENUCUSTOMFUNC[] PROGMEM = TR_MENUCUSTOMFUNC;

#if defined(LUA)
const pm_char STR_MENUCUSTOMSCRIPTS[] PROGMEM = TR_MENUCUSTOMSCRIPTS;
#endif

#if defined(TELEMETRY_FRSKY)
const pm_char STR_MENUTELEMETRY[] PROGMEM = TR_MENUTELEMETRY;
const pm_char STR_LIMIT[] PROGMEM = TR_LIMIT;
#endif

#if defined(TEMPLATES)
const pm_char STR_MENUTEMPLATES[] PROGMEM = TR_MENUTEMPLATES;
#endif

const pm_char STR_MENUSTAT[] PROGMEM = TR_MENUSTAT;
const pm_char STR_MENUDEBUG[] PROGMEM = TR_MENUDEBUG;
const char * const STR_MONITOR_CHANNELS[] = { TR_MONITOR_CHANNELS1, TR_MONITOR_CHANNELS2, TR_MONITOR_CHANNELS3, TR_MONITOR_CHANNELS4 };
const pm_char STR_MONITOR_SWITCHES[] PROGMEM = TR_MONITOR_SWITCHES;
const pm_char STR_MONITOR_OUTPUT_DESC[] PROGMEM = TR_MONITOR_OUTPUT_DESC;
const pm_char STR_MONITOR_MIXER_DESC[] PROGMEM = TR_MONITOR_MIXER_DESC;
const pm_char STR_MENUGLOBALVARS[] PROGMEM = TR_MENUGLOBALVARS;

#if defined(DSM2) || defined(PXX)
const pm_char STR_RECEIVER_NUM[] PROGMEM = TR_RECEIVER_NUM;
const pm_char STR_RECEIVER[] PROGMEM = TR_RECEIVER;
#endif

#if defined(PXX) || defined(CPUARM)
const pm_char STR_SYNCMENU[] PROGMEM = TR_SYNCMENU;
const pm_char STR_INTERNALRF[] PROGMEM = TR_INTERNALRF;
const pm_char STR_EXTERNALRF[] PROGMEM = TR_EXTERNALRF;
const pm_char STR_MODULE_TELEMETRY[] PROGMEM = TR_MODULE_TELEMETRY;
const pm_char STR_MODULE_TELEM_ON[] PROGMEM = TR_MODULE_TELEM_ON;
const pm_char STR_COUNTRYCODE[] PROGMEM = TR_COUNTRYCODE;
const pm_char STR_USBMODE[] PROGMEM = TR_USBMODE;
const pm_char STR_FAILSAFE[] PROGMEM = TR_FAILSAFE;
const pm_char STR_FAILSAFESET[] PROGMEM = TR_FAILSAFESET;
const pm_char STR_HOLD[] PROGMEM = TR_HOLD;
const pm_char STR_NONE[] PROGMEM = TR_NONE;
const pm_char STR_MENUSENSOR[] PROGMEM = TR_MENUSENSOR;
const pm_char STR_SENSOR[] PROGMEM = TR_SENSOR;
const pm_char STR_DISABLE_INTERNAL[] PROGMEM = TR_DISABLE_INTERNAL;
#endif

const pm_char STR_INVERT_THR[] PROGMEM = TR_INVERT_THR;
const pm_char STR_AND_SWITCH[] PROGMEM = TR_AND_SWITCH;
const pm_char STR_SF[] PROGMEM = TR_SF;
const pm_char STR_GF[] PROGMEM = TR_GF;

#if defined(FRSKY_HUB)
const pm_char STR_MINRSSI[] PROGMEM = TR_MINRSSI;
const pm_char STR_LATITUDE[] PROGMEM = TR_LATITUDE;
const pm_char STR_LONGITUDE[] PROGMEM = TR_LONGITUDE;
#endif

#if defined(CPUARM) || defined(CPUM2560)
const pm_char STR_SHUTDOWN[] PROGMEM = TR_SHUTDOWN;
const pm_char STR_SAVEMODEL[] PROGMEM = TR_SAVEMODEL;
#endif

#if defined(PCBX9E)
const pm_char STR_POWEROFF[] PROGMEM = TR_POWEROFF;
#endif

const pm_char STR_BATT_CALIB[] PROGMEM = TR_BATT_CALIB;

#if defined(CPUARM) || defined(TELEMETRY_FRSKY)
const pm_char STR_VOLTAGE[] PROGMEM = TR_VOLTAGE;
const pm_char STR_CURRENT[] PROGMEM = TR_CURRENT;
#endif

#if defined(CPUARM)
const pm_char STR_CURRENT_CALIB[] PROGMEM = TR_CURRENT_CALIB;
const pm_char STR_UNITSSYSTEM[]   PROGMEM = TR_UNITSSYSTEM;
const pm_char STR_VOICELANG[] PROGMEM = TR_VOICELANG;
const pm_char STR_MODELIDUSED[] PROGMEM = TR_MODELIDUSED;
const pm_char STR_BEEP_VOLUME[] PROGMEM = INDENT TR_BEEP_VOLUME;
const pm_char STR_WAV_VOLUME[] PROGMEM = INDENT TR_WAV_VOLUME;
const pm_char STR_BG_VOLUME[] PROGMEM = INDENT TR_BG_VOLUME;
const pm_char STR_PERSISTENT_MAH[] PROGMEM = TR_PERSISTENT_MAH;
#endif

#if defined(NAVIGATION_MENUS)
const pm_char STR_SELECT_MODEL[] PROGMEM = TR_SELECT_MODEL;
const pm_char STR_CREATE_CATEGORY[] PROGMEM = TR_CREATE_CATEGORY;
const pm_char STR_RENAME_CATEGORY[] PROGMEM = TR_RENAME_CATEGORY;
const pm_char STR_DELETE_CATEGORY[] PROGMEM = TR_DELETE_CATEGORY;
const pm_char STR_CREATE_MODEL[] PROGMEM = TR_CREATE_MODEL;
const pm_char STR_DUPLICATE_MODEL[] PROGMEM = TR_DUPLICATE_MODEL;
const pm_char STR_COPY_MODEL[] PROGMEM = TR_COPY_MODEL;
const pm_char STR_MOVE_MODEL[] PROGMEM = TR_MOVE_MODEL;
const pm_char STR_DELETE_MODEL[] PROGMEM = TR_DELETE_MODEL;
const pm_char STR_EDIT[] PROGMEM = TR_EDIT;
const pm_char STR_INSERT_BEFORE[] PROGMEM = TR_INSERT_BEFORE;
const pm_char STR_INSERT_AFTER[] PROGMEM = TR_INSERT_AFTER;
const pm_char STR_COPY[] PROGMEM = TR_COPY;
const pm_char STR_MOVE[] PROGMEM = TR_MOVE;
const pm_char STR_PASTE[] PROGMEM = TR_PASTE;
const pm_char STR_PASTE_AFTER[] PROGMEM = TR_PASTE_AFTER;
const pm_char STR_PASTE_BEFORE[] PROGMEM = TR_PASTE_BEFORE;
const pm_char STR_INSERT[] PROGMEM = TR_INSERT;
const pm_char STR_DELETE[] PROGMEM = TR_DELETE;
const pm_char STR_RESET_FLIGHT[] PROGMEM = TR_RESET_FLIGHT;
const pm_char STR_RESET_TIMER1[] PROGMEM = TR_RESET_TIMER1;
const pm_char STR_RESET_TIMER2[] PROGMEM = TR_RESET_TIMER2;
const pm_char STR_RESET_TIMER3[] PROGMEM = TR_RESET_TIMER3;
const pm_char STR_RESET_TELEMETRY[] PROGMEM = TR_RESET_TELEMETRY;
const pm_char STR_STATISTICS[] PROGMEM = TR_STATISTICS;
const pm_char STR_ABOUT_US[] PROGMEM = TR_ABOUT_US;
const pm_char STR_USB_JOYSTICK[] PROGMEM = TR_USB_JOYSTICK;
const pm_char STR_USB_MASS_STORAGE[] PROGMEM = TR_USB_MASS_STORAGE;
const pm_char STR_USB_SERIAL[] PROGMEM = TR_USB_SERIAL;
const pm_char STR_SETUP_SCREENS[] PROGMEM = TR_SETUP_SCREENS;
const pm_char STR_MONITOR_SCREENS[] PROGMEM = TR_MONITOR_SCREENS;
#endif

#if defined(MULTIMODULE)
const pm_char STR_MULTI_CUSTOM[] PROGMEM = TR_MULTI_CUSTOM;
const pm_char STR_MULTI_RFTUNE[] PROGMEM = TR_MULTI_RFTUNE;
const pm_char STR_MULTI_TELEMETRY[] PROGMEM = TR_MULTI_TELEMETRY;
const pm_char STR_MULTI_VIDFREQ[] PROGMEM = TR_MULTI_VIDFREQ;
const pm_char STR_MULTI_OPTION[] PROGMEM = TR_MULTI_OPTION;
const pm_char STR_MULTI_FIXEDID[] PROGMEM = TR_MULTI_FIXEDID;
const pm_char STR_MULTI_AUTOBIND[] PROGMEM = TR_MULTI_AUTOBIND;
const pm_char STR_MULTI_DSM_AUTODTECT[] PROGMEM = TR_MULTI_DSM_AUTODTECT;
const pm_char STR_MULTI_LOWPOWER[] PROGMEM = TR_MULTI_LOWPOWER;
const pm_char STR_MODULE_NO_SERIAL_MODE[] PROGMEM = TR_MODULE_NO_SERIAL_MODE;
const pm_char STR_MODULE_NO_INPUT[] PROGMEM = TR_MODULE_NO_INPUT;
const pm_char STR_MODULE_WAITFORBIND[] PROGMEM = TR_MODULE_WAITFORBIND;
const pm_char STR_MODULE_NO_TELEMETRY[] PROGMEM = TR_MODULE_NO_TELEMETRY;
const pm_char STR_MODULE_BINDING[] PROGMEM = TR_MODULE_BINDING;
const pm_char STR_PROTOCOL_INVALID[] PROGMEM = TR_PROTOCOL_INVALID;
const pm_char STR_MODULE_STATUS[] PROGMEM = TR_MODULE_STATUS;
const pm_char STR_MODULE_SYNC[] PROGMEM = TR_MODULE_SYNC;
const pm_char STR_MULTI_SERVOFREQ[] PROGMEM = TR_MULTI_SERVOFREQ;
#if LCD_W < 212
const pm_char STR_SUBTYPE[] PROGMEM = TR_SUBTYPE;
#endif
#endif

const pm_char STR_RESET_BTN[] PROGMEM = TR_RESET_BTN;

#if defined(SDCARD)
const pm_char STR_BACKUP_MODEL[] PROGMEM = TR_BACKUP_MODEL;
const pm_char STR_RESTORE_MODEL[] PROGMEM = TR_RESTORE_MODEL;
const pm_char STR_DELETE_ERROR[] PROGMEM = TR_DELETE_ERROR;
const pm_char STR_SDCARD_ERROR[] PROGMEM = TR_SDCARD_ERROR;
const pm_char STR_NO_SDCARD[] PROGMEM = TR_NO_SDCARD;
const pm_char STR_SDCARD_FULL[] PROGMEM = TR_SDCARD_FULL;
const pm_char STR_INCOMPATIBLE[] PROGMEM = TR_INCOMPATIBLE;
const pm_char STR_LOGS_PATH[] PROGMEM = LOGS_PATH;
const pm_char STR_LOGS_EXT[] PROGMEM = LOGS_EXT;
const pm_char STR_MODELS_PATH[] PROGMEM = MODELS_PATH;
const pm_char STR_MODELS_EXT[] PROGMEM = MODELS_EXT;
#endif

const pm_char STR_CAT_NOT_EMPTY[] PROGMEM = TR_CAT_NOT_EMPTY;
const pm_char STR_WARNING[] PROGMEM = TR_WARNING;
const pm_char STR_THROTTLEWARN[] PROGMEM = TR_THROTTLEWARN;
const pm_char STR_ALARMSWARN[] PROGMEM = TR_ALARMSWARN;
const pm_char STR_SWITCHWARN[] PROGMEM = TR_SWITCHWARN;
const pm_char STR_FAILSAFEWARN[] PROGMEM = TR_FAILSAFEWARN;
#if defined(NIGHTLY_BUILD_WARNING)
const pm_char STR_NIGHTLY_WARNING[] PROGMEM = TR_NIGHTLY_WARNING;
const pm_char STR_NIGHTLY_NOTSAFE[] PROGMEM = TR_NIGHTLY_NOTSAFE;
#endif
const pm_char STR_WRONG_SDCARDVERSION[] PROGMEM = TR_WRONG_SDCARDVERSION;
const pm_char STR_WRONG_PCBREV[] PROGMEM = TR_WRONG_PCBREV;
const pm_char STR_EMERGENCY_MODE[] PROGMEM = TR_EMERGENCY_MODE;
const pm_char STR_PCBREV_ERROR[] PROGMEM = TR_PCBREV_ERROR;
const pm_char STR_NO_FAILSAFE[] PROGMEM = TR_NO_FAILSAFE;
const pm_char STR_KEYSTUCK[] PROGMEM = TR_KEYSTUCK;

const pm_char STR_SPEAKER_VOLUME[] PROGMEM = TR_SPEAKER_VOLUME;
const pm_char STR_LCD[] PROGMEM = TR_LCD;
const pm_char STR_BRIGHTNESS[] PROGMEM = TR_BRIGHTNESS;
const pm_char STR_CPU_TEMP[] PROGMEM = TR_CPU_TEMP;
const pm_char STR_CPU_CURRENT[] PROGMEM = TR_CPU_CURRENT;
const pm_char STR_CPU_MAH[] PROGMEM = TR_CPU_MAH;
const pm_char STR_COPROC[] PROGMEM = TR_COPROC;
const pm_char STR_COPROC_TEMP[] PROGMEM = TR_COPROC_TEMP;
const pm_char STR_TEMPWARNING[] PROGMEM = TR_TEMPWARNING;
const pm_char STR_CAPAWARNING[] PROGMEM = TR_CAPAWARNING;
const pm_char STR_FUNC[] PROGMEM = TR_FUNC;
const pm_char STR_V1[] PROGMEM = TR_V1;
const pm_char STR_V2[] PROGMEM = TR_V2;
const pm_char STR_DURATION[] PROGMEM = TR_DURATION;
const pm_char STR_DELAY[] PROGMEM = TR_DELAY;
const pm_char STR_SD_CARD[] PROGMEM = TR_SD_CARD;
const pm_char STR_SDHC_CARD[] PROGMEM = TR_SDHC_CARD;
const pm_char STR_NO_SOUNDS_ON_SD[] PROGMEM = TR_NO_SOUNDS_ON_SD;
const pm_char STR_NO_MODELS_ON_SD[] PROGMEM = TR_NO_MODELS_ON_SD;
const pm_char STR_NO_BITMAPS_ON_SD[] PROGMEM = TR_NO_BITMAPS_ON_SD;
const pm_char STR_NO_SCRIPTS_ON_SD[] PROGMEM = TR_NO_SCRIPTS_ON_SD;
const pm_char STR_SCRIPT_SYNTAX_ERROR[] PROGMEM = TR_SCRIPT_SYNTAX_ERROR;
const pm_char STR_SCRIPT_PANIC[] PROGMEM = TR_SCRIPT_PANIC;
const pm_char STR_SCRIPT_KILLED[] PROGMEM = TR_SCRIPT_KILLED;
const pm_char STR_SCRIPT_ERROR[] PROGMEM = TR_SCRIPT_ERROR;
const pm_char STR_PLAY_FILE[] PROGMEM = TR_PLAY_FILE;
const pm_char STR_ASSIGN_BITMAP[] PROGMEM = TR_ASSIGN_BITMAP;
#if defined(COLORLCD)
const pm_char STR_ASSIGN_SPLASH[] PROGMEM = TR_ASSIGN_SPLASH;
const pm_char STR_DIALOG_ABORT[] PROGMEM = TR_DIALOG_ABORT;
const pm_char STR_DIALOG_NO[] PROGMEM = TR_DIALOG_NO;
const pm_char STR_DIALOG_YES[] PROGMEM = TR_DIALOG_YES;
const pm_char STR_DIALOG_OK[] PROGMEM = TR_DIALOG_OK;
const pm_char STR_DIALOG_CANCEL[] PROGMEM = TR_DIALOG_CANCEL;
#endif
const pm_char STR_EXECUTE_FILE[] PROGMEM = TR_EXECUTE_FILE;
const pm_char STR_DELETE_FILE[] PROGMEM = TR_DELETE_FILE;
const pm_char STR_COPY_FILE[] PROGMEM = TR_COPY_FILE;
const pm_char STR_RENAME_FILE[] PROGMEM = TR_RENAME_FILE;
const pm_char STR_SD_INFO[] PROGMEM = TR_SD_INFO;
const pm_char STR_SD_FORMAT[] PROGMEM = TR_SD_FORMAT;
const pm_char STR_REMOVED[] PROGMEM = TR_REMOVED;
const pm_char STR_NA[] PROGMEM = TR_NA;
const pm_char STR_HARDWARE[] PROGMEM = TR_HARDWARE;
const pm_char STR_FORMATTING[] PROGMEM = TR_FORMATTING;
const pm_char STR_TEMP_CALIB[] PROGMEM = TR_TEMP_CALIB;
const pm_char STR_TIME[] PROGMEM = TR_TIME;
const pm_char STR_MAXBAUDRATE[] PROGMEM = TR_MAXBAUDRATE;
const pm_char STR_BAUDRATE[] PROGMEM = TR_BAUDRATE;
const pm_char STR_SD_INFO_TITLE[] PROGMEM = TR_SD_INFO_TITLE;
const pm_char STR_SD_TYPE[] PROGMEM = TR_SD_TYPE;
const pm_char STR_SD_SPEED[] PROGMEM = TR_SD_SPEED;
const pm_char STR_SD_SECTORS[] PROGMEM = TR_SD_SECTORS;
const pm_char STR_SD_SIZE[] PROGMEM = TR_SD_SIZE;
const pm_char STR_TYPE[] PROGMEM = TR_TYPE;
const pm_char STR_GLOBAL_VARS[] PROGMEM = TR_GLOBAL_VARS;
const pm_char STR_GVARS[] PROGMEM = TR_GVARS;
const pm_char STR_GLOBAL_VAR[] PROGMEM = TR_GLOBAL_VAR;
const pm_char STR_OWN[] PROGMEM = TR_OWN;
const pm_char STR_ROTARY_ENCODER[] PROGMEM = TR_ROTARY_ENCODER;
const pm_char STR_DATE[] PROGMEM = TR_DATE;
const pm_char STR_CHANNELS_MONITOR[] PROGMEM = TR_CHANNELS_MONITOR;
const pm_char STR_MIXERS_MONITOR[] PROGMEM = TR_MIXERS_MONITOR;
const pm_char STR_PATH_TOO_LONG[] PROGMEM = TR_PATH_TOO_LONG;
const pm_char STR_VIEW_TEXT[] PROGMEM = TR_VIEW_TEXT;
const pm_char STR_FLASH_BOOTLOADER[] PROGMEM = TR_FLASH_BOOTLOADER;
const pm_char STR_FLASH_INTERNAL_MODULE[] PROGMEM = TR_FLASH_INTERNAL_MODULE;
const pm_char STR_FIRMWARE_UPDATE_ERROR[] PROGMEM = TR_FIRMWARE_UPDATE_ERROR;
const pm_char STR_FLASH_EXTERNAL_DEVICE[] PROGMEM = TR_FLASH_EXTERNAL_DEVICE;
const pm_char STR_WRITING[] PROGMEM = TR_WRITING;
const pm_char STR_CONFIRM_FORMAT[] PROGMEM = TR_CONFIRM_FORMAT;
const pm_char STR_EEBACKUP[] PROGMEM = TR_EEBACKUP;
const pm_char STR_FACTORYRESET[] PROGMEM = TR_FACTORYRESET;
const pm_char STR_FIRMWAREUPDATE[] PROGMEM = TR_FIRMWAREUPDATE;
const pm_char STR_CONFIRMRESET[] PROGMEM = TR_CONFIRMRESET;
const pm_char STR_FW_UPDATE_QUESTION[] PROGMEM = TR_FW_UPDATE_QUESTION;
const pm_char STR_TOO_MANY_LUA_SCRIPTS[] PROGMEM = TR_TO_MANY_LUA_SCRIPTS;
const pm_char STR_BLCOLOR[]  PROGMEM = TR_BLCOLOR;
const pm_char STR_QUICK_START_GUIDE[] PROGMEM = TR_QUICK_START_GUIDE;
const pm_char STR_USER_MANUAL[] = TR_USER_MANUAL;


#if defined(CPUARM)
  const pm_char STR_MODELNAME[] PROGMEM = TR_MODELNAME;
  const pm_char STR_PHASENAME[] PROGMEM = TR_PHASENAME;
  const pm_char STR_MIXNAME[] PROGMEM = TR_MIXNAME;
  const pm_char STR_INPUTNAME[] PROGMEM = TR_INPUTNAME;
  const pm_char STR_EXPONAME[] PROGMEM = TR_EXPONAME;
#endif

#if LCD_W >= 212
  const char * const STR_PHASES_HEADERS[] = TR_PHASES_HEADERS;
  const char * const STR_LIMITS_HEADERS[] = TR_LIMITS_HEADERS;
  const char * const STR_LSW_HEADERS[] = TR_LSW_HEADERS;
  const char * const STR_LSW_DESCRIPTIONS[] = TR_LSW_DESCRIPTIONS;
  const char * const STR_GVAR_HEADERS[] = TR_GVAR_HEADERS;
#endif

#if defined(CPUARM)
  const pm_char STR_TRAINER[] PROGMEM = TR_TRAINER;
  const pm_char STR_MODULE_BIND[] PROGMEM  = TR_MODULE_BIND;
  const pm_char STR_BINDING_1_8_TELEM_ON[] PROGMEM = TR_BINDING_CH1_8_TELEM_ON;
  const pm_char STR_BINDING_1_8_TELEM_OFF[] PROGMEM = TR_BINDING_CH1_8_TELEM_OFF;
  const pm_char STR_BINDING_9_16_TELEM_ON[] PROGMEM = TR_BINDING_CH9_16_TELEM_ON;
  const pm_char STR_BINDING_9_16_TELEM_OFF[] PROGMEM = TR_BINDING_CH9_16_TELEM_OFF;
  const pm_char STR_BINDING_25MW_CH1_8_TELEM_OFF[] PROGMEM = TR_BINDING_25MW_CH1_8_TELEM_OFF;
  const pm_char STR_BINDING_25MW_CH1_8_TELEM_ON[] PROGMEM = TR_BINDING_25MW_CH1_8_TELEM_ON;
  const pm_char STR_BINDING_500MW_CH1_8_TELEM_OFF[] PROGMEM = TR_BINDING_500MW_CH1_8_TELEM_OFF;
  const pm_char STR_BINDING_500MW_CH9_16_TELEM_OFF[] PROGMEM = TR_BINDING_500MW_CH9_16_TELEM_OFF;
  const pm_char STR_CHANNELRANGE[] PROGMEM = TR_CHANNELRANGE;
  const pm_char STR_RXFREQUENCY[] PROGMEM = TR_RXFREQUENCY;
  const pm_char STR_ANTENNASELECTION[] PROGMEM = TR_ANTENNASELECTION;
  const pm_char STR_ANTENNACONFIRM1[] PROGMEM = TR_ANTENNACONFIRM1;
  const pm_char STR_ANTENNACONFIRM2[] PROGMEM = TR_ANTENNACONFIRM2;
  const pm_char STR_SET[] PROGMEM = TR_SET;
  const pm_char STR_PREFLIGHT[] PROGMEM = TR_PREFLIGHT;
  const pm_char STR_CHECKLIST[] PROGMEM = TR_CHECKLIST;
  const pm_char STR_VIEW_NOTES[] PROGMEM = TR_VIEW_NOTES;
  const pm_char STR_MODEL_SELECT[] PROGMEM = TR_MODEL_SELECT;
  const pm_char STR_RESET_SUBMENU[] PROGMEM = TR_RESET_SUBMENU;
  const pm_char STR_LOWALARM[] PROGMEM = TR_LOWALARM;
  const pm_char STR_CRITICALALARM[] PROGMEM = TR_CRITICALALARM;
  const pm_char STR_RSSIALARM_WARN[] PROGMEM = TR_RSSIALARM_WARN;
  const pm_char STR_NO_RSSIALARM[] PROGMEM = TR_NO_RSSIALARM;
  const pm_char STR_DISABLE_ALARM[] PROGMEM = TR_DISABLE_ALARM;
  const pm_char STR_TELEMETRY_TYPE[] PROGMEM = TR_TELEMETRY_TYPE;
  const pm_char STR_TELEMETRY_SENSORS[] PROGMEM = TR_TELEMETRY_SENSORS;
  const pm_char STR_VALUE[] PROGMEM = TR_VALUE;
const pm_char STR_REPEAT[] PROGMEM = TR_REPEAT;
const pm_char STR_ENABLE[] PROGMEM = TR_ENABLE;
  const pm_char STR_TOPLCDTIMER[] PROGMEM = TR_TOPLCDTIMER;
  const pm_char STR_UNIT[] PROGMEM = TR_UNIT;
  const pm_char STR_TELEMETRY_NEWSENSOR[] PROGMEM = TR_TELEMETRY_NEWSENSOR;
  const pm_char STR_ID[] PROGMEM = TR_ID;
  const pm_char STR_PRECISION[] PROGMEM = TR_PRECISION;
  const pm_char STR_RATIO[] PROGMEM = TR_RATIO;
  const pm_char STR_FORMULA[] PROGMEM = TR_FORMULA;
  const pm_char STR_CELLINDEX[] PROGMEM = TR_CELLINDEX;
  const pm_char STR_LOGS[] PROGMEM = TR_LOGS;
  const pm_char STR_OPTIONS[] PROGMEM = TR_OPTIONS;
  const pm_char STR_ALTSENSOR[] PROGMEM = TR_ALTSENSOR;
  const pm_char STR_CELLSENSOR[] PROGMEM = TR_CELLSENSOR;
  const pm_char STR_GPSSENSOR[] PROGMEM = TR_GPSSENSOR;
  const pm_char STR_CURRENTSENSOR[] PROGMEM = TR_CURRENTSENSOR;
  const pm_char STR_AUTOOFFSET[] PROGMEM = TR_AUTOOFFSET;
  const pm_char STR_ONLYPOSITIVE[] PROGMEM = TR_ONLYPOSITIVE;
  const pm_char STR_FILTER[] PROGMEM = TR_FILTER;
  const pm_char STR_TELEMETRYFULL[] PROGMEM = TR_TELEMETRYFULL;
  const pm_char STR_SERVOS_OK[] PROGMEM = TR_SERVOS_OK;
  const pm_char STR_SERVOS_KO[] PROGMEM = TR_SERVOS_KO;
  const pm_char STR_INVERTED_SERIAL[] PROGMEM = TR_INVERTED_SERIAL;
  const pm_char STR_IGNORE_INSTANCE[] PROGMEM = TR_IGNORE_INSTANCE;
  const pm_char STR_DISCOVER_SENSORS[] PROGMEM = TR_DISCOVER_SENSORS;
  const pm_char STR_STOP_DISCOVER_SENSORS[] PROGMEM = TR_STOP_DISCOVER_SENSORS;
  const pm_char STR_DELETE_ALL_SENSORS[] PROGMEM = TR_DELETE_ALL_SENSORS;
  const pm_char STR_CONFIRMDELETE[] PROGMEM = TR_CONFIRMDELETE;
  const pm_char STR_SELECT_WIDGET[] PROGMEM = TR_SELECT_WIDGET;
  const pm_char STR_REMOVE_WIDGET[] PROGMEM = TR_REMOVE_WIDGET;
  const pm_char STR_WIDGET_SETTINGS[] PROGMEM = TR_WIDGET_SETTINGS;
  const pm_char STR_REMOVE_SCREEN[] PROGMEM = TR_REMOVE_SCREEN;
  const pm_char STR_SETUP_WIDGETS[] PROGMEM = TR_SETUP_WIDGETS;
  const pm_char STR_USER_INTERFACE[] PROGMEM = TR_USER_INTERFACE;
  const pm_char STR_THEME[] PROGMEM = TR_THEME;
  const pm_char STR_SETUP[] PROGMEM = TR_SETUP;
  const pm_char STR_MAINVIEWX[] PROGMEM = TR_MAINVIEWX;
  const pm_char STR_LAYOUT[] PROGMEM = TR_LAYOUT;
  const pm_char STR_ADDMAINVIEW[] PROGMEM = TR_ADDMAINVIEW;
  const pm_char STR_BACKGROUND_COLOR[] PROGMEM = TR_BACKGROUND_COLOR;
  const pm_char STR_MAIN_COLOR[] PROGMEM = TR_MAIN_COLOR;
  const pm_char STR_MULTI_RFPOWER[] PROGMEM = TR_MULTI_RFPOWER;
  const pm_char STR_RF_PROTOCOL[] PROGMEM = TR_RF_PROTOCOL;
  const pm_char STR_MODULE_OPTIONS[] PROGMEM = TR_MODULE_OPTIONS;
  const pm_char STR_POWER[] PROGMEM = TR_POWER;
#endif

#if defined(CPUARM)
  const pm_char STR_BYTES[] PROGMEM = TR_BYTES;
  const pm_char STR_ANTENNAPROBLEM[] PROGMEM = TR_ANTENNAPROBLEM;
  const pm_char STR_MODULE[] PROGMEM = TR_MODULE;
  const pm_char STR_ENABLE_POPUP[] PROGMEM = TR_ENABLE_POPUP;
  const pm_char STR_DISABLE_POPUP[] PROGMEM = TR_DISABLE_POPUP;
  const pm_char STR_POPUP[] PROGMEM = TR_POPUP;
  const pm_char STR_MIN[] PROGMEM = TR_MIN;
  const pm_char STR_MAX[] PROGMEM = TR_MAX;
  const pm_char STR_CURVE_PRESET[] PROGMEM = TR_CURVE_PRESET;
  const pm_char STR_PRESET[] PROGMEM = TR_PRESET;
  const pm_char STR_MIRROR[] PROGMEM = TR_MIRROR;
  const pm_char STR_CLEAR[] PROGMEM = TR_CLEAR;
  const pm_char STR_RESET[] PROGMEM = TR_RESET;
  const pm_char STR_COUNT[] PROGMEM = TR_COUNT;
  const pm_char STR_PT[] PROGMEM = TR_PT;
  const pm_char STR_PTS[] PROGMEM = TR_PTS;
  const pm_char STR_SMOOTH[] PROGMEM = TR_SMOOTH;
  const pm_char STR_COPY_STICKS_TO_OFS[] PROGMEM = TR_COPY_STICKS_TO_OFS;
  const pm_char STR_COPY_TRIMS_TO_OFS[] PROGMEM = TR_COPY_TRIMS_TO_OFS;
  const pm_char STR_INCDEC[] PROGMEM = TR_INCDEC;
  const pm_char STR_GLOBALVAR[] PROGMEM = TR_GLOBALVAR;
  const pm_char STR_MIXSOURCE[] PROGMEM = TR_MIXSOURCE;
  const pm_char STR_CONSTANT[] PROGMEM = TR_CONSTANT;
  const pm_char STR_TOP_BAR[] PROGMEM = TR_TOP_BAR;
  const pm_char STR_ALTITUDE[] PROGMEM = TR_ALTITUDE;
  const pm_char STR_SCALE[] PROGMEM = TR_SCALE;
  const pm_char STR_VIEW_CHANNELS[] PROGMEM = TR_VIEW_CHANNELS;
  const pm_char STR_UART3MODE[] PROGMEM = TR_UART3MODE;
  const pm_char STR_THROTTLE_LABEL[] PROGMEM = TR_THROTTLE_LABEL;
  const pm_char STR_SCRIPT[] PROGMEM = TR_SCRIPT;
  const pm_char STR_INPUTS[] PROGMEM = TR_INPUTS;
  const pm_char STR_OUTPUTS[] PROGMEM = TR_OUTPUTS;
  const pm_char STR_MENU_INPUTS[] PROGMEM = TR_MENU_INPUTS;
  const pm_char STR_MENU_LUA[] PROGMEM = TR_MENU_LUA;
  const pm_char STR_MENU_STICKS[] PROGMEM = TR_MENU_STICKS;
  const pm_char STR_MENU_POTS[] PROGMEM = TR_MENU_POTS;
  const pm_char STR_MENU_MAX[] PROGMEM = TR_MENU_MAX;
  const pm_char STR_MENU_HELI[] PROGMEM = TR_MENU_HELI;
  const pm_char STR_MENU_TRIMS[] PROGMEM = TR_MENU_TRIMS;
  const pm_char STR_MENU_SWITCHES[] PROGMEM = TR_MENU_SWITCHES;
  const pm_char STR_MENU_LOGICAL_SWITCHES[] PROGMEM = TR_MENU_LOGICAL_SWITCHES;
  const pm_char STR_MENU_TRAINER[] PROGMEM = TR_MENU_TRAINER;
  const pm_char STR_MENU_CHANNELS[] PROGMEM = TR_MENU_CHANNELS;
  const pm_char STR_MENU_GVARS[] PROGMEM = TR_MENU_GVARS;
  const pm_char STR_MENU_TELEMETRY[] PROGMEM = TR_MENU_TELEMETRY;
  const pm_char STR_MENU_DISPLAY[] PROGMEM = TR_MENU_DISPLAY;
  const pm_char STR_MENU_OTHER[] PROGMEM = TR_MENU_OTHER;
  const pm_char STR_MENU_INVERT[] PROGMEM = TR_MENU_INVERT;
  const pm_char STR_JITTER_FILTER[] PROGMEM = TR_JITTER_FILTER;
  const pm_char STR_DEAD_ZONE[] PROGMEM = TR_DEAD_ZONE;
#endif

#if MENUS_LOCK == 1
  const pm_char STR_UNLOCKED[] PROGMEM = TR_UNLOCKED;
  const pm_char STR_MODS_FORBIDDEN[] PROGMEM = TR_MODS_FORBIDDEN;
#endif

#if defined(PCBTARANIS) || defined(DSM2)
  const pm_char STR_MODULE_RANGE[] PROGMEM = TR_MODULE_RANGE;
#endif

#if defined(BLUETOOTH)
  const pm_char STR_BLUETOOTH[] PROGMEM = TR_BLUETOOTH;
  const pm_char STR_BLUETOOTH_DISC[] PROGMEM = TR_BLUETOOTH_DISC;
  const pm_char STR_BLUETOOTH_INIT[] PROGMEM = TR_BLUETOOTH_INIT;
  const pm_char STR_BLUETOOTH_DIST_ADDR[] PROGMEM = TR_BLUETOOTH_DIST_ADDR;
  const pm_char STR_BLUETOOTH_LOCAL_ADDR[] PROGMEM = TR_BLUETOOTH_LOCAL_ADDR;
  const pm_char STR_BLUETOOTH_PIN_CODE[] PROGMEM = TR_BLUETOOTH_PIN_CODE;
#endif

#if defined(TELEMETRY_MAVLINK)
  const pm_char STR_MAVLINK_RC_RSSI_SCALE_LABEL[] PROGMEM = TR_MAVLINK_RC_RSSI_SCALE_LABEL;
  const pm_char STR_MAVLINK_PC_RSSI_EN_LABEL[] PROGMEM = TR_MAVLINK_PC_RSSI_EN_LABEL;
  const pm_char STR_MAVMENUSETUP_TITLE[] PROGMEM = TR_MAVMENUSETUP_TITLE;
  const pm_char STR_MAVLINK_BAUD_LABEL[] PROGMEM = TR_MAVLINK_BAUD_LABEL;
  const pm_char STR_MAVLINK_INFOS[] PROGMEM = TR_MAVLINK_INFOS;
  const pm_char STR_MAVLINK_MODE[] PROGMEM = TR_MAVLINK_MODE;
  const pm_char STR_MAVLINK_CUR_MODE[] PROGMEM = TR_MAVLINK_CUR_MODE;
  const pm_char STR_MAVLINK_ARMED[] PROGMEM = TR_MAVLINK_ARMED;
  const pm_char STR_MAVLINK_BAT_MENU_TITLE[] PROGMEM = TR_MAVLINK_BAT_MENU_TITLE;
  const pm_char STR_MAVLINK_BATTERY_LABEL[] PROGMEM = TR_MAVLINK_BATTERY_LABEL;
  const pm_char STR_MAVLINK_RC_RSSI_LABEL[] PROGMEM = TR_MAVLINK_RC_RSSI_LABEL;
  const pm_char STR_MAVLINK_PC_RSSI_LABEL[] PROGMEM = TR_MAVLINK_PC_RSSI_LABEL;
  const pm_char STR_MAVLINK_NAV_MENU_TITLE[] PROGMEM = TR_MAVLINK_NAV_MENU_TITLE;
  const pm_char STR_MAVLINK_COURSE[] PROGMEM = TR_MAVLINK_COURSE;
  const pm_char STR_MAVLINK_HEADING[] PROGMEM = TR_MAVLINK_HEADING;
  const pm_char STR_MAVLINK_BEARING[] PROGMEM = TR_MAVLINK_BEARING;
  const pm_char STR_MAVLINK_ALTITUDE[] PROGMEM = TR_MAVLINK_ALTITUDE;
  const pm_char STR_MAVLINK_GPS[] PROGMEM = TR_MAVLINK_GPS;
  const pm_char STR_MAVLINK_NO_FIX[] PROGMEM = TR_MAVLINK_NO_FIX;
  const pm_char STR_MAVLINK_SAT[] PROGMEM = TR_MAVLINK_SAT;
  const pm_char STR_MAVLINK_HDOP[] PROGMEM = TR_MAVLINK_HDOP;
  const pm_char STR_MAVLINK_LAT[] PROGMEM = TR_MAVLINK_LAT;
  const pm_char STR_MAVLINK_LON[] PROGMEM = TR_MAVLINK_LON;
#endif

#if !defined(CPUM64)
  const pm_char STR_ABOUTUS[] PROGMEM = TR_ABOUTUS;
  const pm_char STR_ABOUT_OPENTX_1[] PROGMEM = TR_ABOUT_OPENTX_1;
  const pm_char STR_ABOUT_OPENTX_2[] PROGMEM = TR_ABOUT_OPENTX_2;
  const pm_char STR_ABOUT_OPENTX_3[] PROGMEM = TR_ABOUT_OPENTX_3;
  const pm_char STR_ABOUT_OPENTX_4[] PROGMEM = TR_ABOUT_OPENTX_4;
  const pm_char STR_ABOUT_OPENTX_5[] PROGMEM = TR_ABOUT_OPENTX_5;

  const pm_char STR_ABOUT_BERTRAND_1[] PROGMEM = TR_ABOUT_BERTRAND_1;
  const pm_char STR_ABOUT_BERTRAND_2[] PROGMEM = TR_ABOUT_BERTRAND_2;
  const pm_char STR_ABOUT_BERTRAND_3[] PROGMEM = TR_ABOUT_BERTRAND_3;

  const pm_char STR_ABOUT_MIKE_1[] PROGMEM = TR_ABOUT_MIKE_1;
  const pm_char STR_ABOUT_MIKE_2[] PROGMEM = TR_ABOUT_MIKE_2;
  const pm_char STR_ABOUT_MIKE_3[] PROGMEM = TR_ABOUT_MIKE_3;
  const pm_char STR_ABOUT_MIKE_4[] PROGMEM = TR_ABOUT_MIKE_4;

  const pm_char STR_ABOUT_ROMOLO_1[] PROGMEM = TR_ABOUT_ROMOLO_1;
  const pm_char STR_ABOUT_ROMOLO_2[] PROGMEM = TR_ABOUT_ROMOLO_2;
  const pm_char STR_ABOUT_ROMOLO_3[] PROGMEM = TR_ABOUT_ROMOLO_3;

  const pm_char STR_ABOUT_ANDRE_1[] PROGMEM = TR_ABOUT_ANDRE_1;
  const pm_char STR_ABOUT_ANDRE_2[] PROGMEM = TR_ABOUT_ANDRE_2;
  const pm_char STR_ABOUT_ANDRE_3[] PROGMEM = TR_ABOUT_ANDRE_3;

  const pm_char STR_ABOUT_ROB_1[] PROGMEM = TR_ABOUT_ROB_1;
  const pm_char STR_ABOUT_ROB_2[] PROGMEM = TR_ABOUT_ROB_2;

  const pm_char STR_ABOUT_MARTIN_1[] PROGMEM = TR_ABOUT_MARTIN_1;
  const pm_char STR_ABOUT_MARTIN_2[] PROGMEM = TR_ABOUT_MARTIN_2;

  const pm_char STR_ABOUT_KJELL_1[] PROGMEM = TR_ABOUT_KJELL_1;
  const pm_char STR_ABOUT_KJELL_2[] PROGMEM = TR_ABOUT_KJELL_2;
  const pm_char STR_ABOUT_KJELL_3[] PROGMEM = TR_ABOUT_KJELL_3;
  const pm_char STR_ABOUT_KJELL_4[] PROGMEM = TR_ABOUT_KJELL_4;

  const pm_char STR_ABOUT_HARDWARE_1[] PROGMEM = TR_ABOUT_HARDWARE_1;
  const pm_char STR_ABOUT_HARDWARE_2[] PROGMEM = TR_ABOUT_HARDWARE_2;
  const pm_char STR_ABOUT_HARDWARE_3[] PROGMEM = TR_ABOUT_HARDWARE_3;

  const pm_char STR_ABOUT_PARENTS_1[] PROGMEM = TR_ABOUT_PARENTS_1;
  const pm_char STR_ABOUT_PARENTS_2[] PROGMEM = TR_ABOUT_PARENTS_2;
  const pm_char STR_ABOUT_PARENTS_3[] PROGMEM = TR_ABOUT_PARENTS_3;
  const pm_char STR_ABOUT_PARENTS_4[] PROGMEM = TR_ABOUT_PARENTS_4;
#endif