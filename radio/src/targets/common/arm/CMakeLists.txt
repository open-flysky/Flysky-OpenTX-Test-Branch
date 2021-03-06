set(TIMERS 3 CACHE STRING "Timers count (1 or 2 or 3)")
set_property(CACHE TIMERS PROPERTY STRINGS 1 2 3)
option(CLI "Command Line Interface" OFF)
option(DEBUG "Debug mode" OFF)
option(LOG_TELEMETRY "Telemetry Logs on SD card" OFF)
option(TRACE_SD_CARD "Traces SD enabled" OFF)
option(TRACE_FATFS "Traces FatFS enabled" OFF)
option(TRACE_AUDIO "Traces audio enabled" OFF)
option(DEBUG_TRACE_BUFFER "Debug Trace Screen" OFF)
option(MULTIMODULE "DIY Multiprotocol TX Module (https://github.com/pascallanger/DIY-Multiprotocol-TX-Module)" OFF)
option(AFHDS3 "Support for AFHDS3" YES)
option(PPM "PPM TX Module" ON)
option(DSM2 "DSM2 TX Module" ON)
option(SBUS "SBUS TX Module" ON)
option(SBUS_TRAINER "SBUS TRAINER" ON)
option(XJT "XJT TX Module" ON)
option(MODULE_SIZE_STD "Standard size TX Module" ON)
option(PXX1 "PXX1 protocol support" ON)
option(PXX2 "PXX2 protocol support" OFF)
option(GHOST "Ghost TX Module" ON)
option(MODULE_PROTOCOL_FCC "Add support for FCC modules" ON)
option(MODULE_PROTOCOL_LBT "Add support for EU/LBT modules" ON)
option(MODULE_PROTOCOL_FLEX "Add support for non certified FLEX modules" OFF)
option(SUPPORT_D16_EU_ONLY "XJT module only supports D16-EU and LR12-EU" OFF) # TODO rename to XJT_EU_ONLY
option(DEBUG_INTERRUPTS "Count interrupts" OFF)
option(DEBUG_WINDOWS "Debug Windows" OFF)
option(DEBUG_USB_INTERRUPTS "Count individual USB interrupts" OFF)
option(DEBUG_TASKS "Task switching statistics" OFF)
option(DEBUG_TIMERS "Time critical parts of the code" OFF)

if(TIMERS EQUAL 3)
  add_definitions(-DTIMERS=3)
elseif(TIMERS EQUAL 1)
  add_definitions(-DTIMERS=1)
else()
  add_definitions(-DTIMERS=2)
endif()

if(LOG_TELEMETRY)
  add_definitions(-DLOG_TELEMETRY)
endif()
if(TRACE_SD_CARD)
  add_definitions(-DTRACE_SD_CARD)
  set(DEBUG ON)
  set(DEBUG_TRACE_BUFFER ON)
endif()
if(TRACE_FATFS)
  add_definitions(-DTRACE_FATFS)
  set(DEBUG ON)
  set(DEBUG_TRACE_BUFFER ON)
endif()
if(TRACE_AUDIO)
  add_definitions(-DTRACE_AUDIO)
  set(DEBUG ON)
  set(DEBUG_TRACE_BUFFER ON)
endif()
if(DEBUG_TRACE_BUFFER)
  add_definitions(-DDEBUG_TRACE_BUFFER)
endif()
if(DEBUG_INTERRUPTS)
  add_definitions(-DDEBUG_INTERRUPTS)
  set(DEBUG ON)
  if(DEBUG_USB_INTERRUPTS)
    add_definitions(-DDEBUG_USB_INTERRUPTS)
  endif()
endif()
if(DEBUG_TASKS)
  add_definitions(-DDEBUG_TASKS)
  set(DEBUG ON)
endif()
if(DEBUG_WINDOWS)
  add_definitions(-DDEBUG_WINDOWS)
  set(DEBUG ON)
endif()
if(DEBUG_TIMERS)
  add_definitions(-DDEBUG_TIMERS)
  set(DEBUG ON)
endif()
if(CLI)
  add_definitions(-DCLI)
  set(FIRMWARE_SRC ${FIRMWARE_SRC} cli.cpp)
  set(SRC ${SRC} dump.cpp)
elseif(DEBUG)
  set(SRC ${SRC} dump.cpp)
endif()
if(DEBUG)
  add_definitions(-DDEBUG)
endif()
if(SUPPORT_D16_EU_ONLY)
  add_definitions(-DMODULE_D16_EU_ONLY_SUPPORT)
endif()
if(HAPTIC)
  add_definitions(-DHAPTIC)
  set(SRC ${SRC} haptic.cpp)
  set(TARGET_SRC ${TARGET_SRC} haptic_driver.cpp)
endif()
if(MULTIMODULE)
  add_definitions(-DMULTIMODULE)
  set(SRC ${SRC} pulses/multi_arm.cpp telemetry/spektrum.cpp telemetry/hitec.cpp telemetry/hott.cpp telemetry/flysky_ibus.cpp telemetry/multi.cpp)
  set(SRC ${SRC} io/multi_firmware_update.cpp)
endif()
if(AFHDS2)
  set(SRC ${SRC} io/hallStick_parser.cpp io/hallStick_parser.cpp)
endif()
if(XJT OR PXX1 OR PXX2) 
  set(SRC ${SRC} io/frsky_firmware_update.cpp)
endif()
if(MULTIMODULE OR AFHDS3)
  set(SRC ${SRC} telemetry/flysky_ibus.cpp)
endif()

if(MODULE_PROTOCOL_FCC)
  add_definitions(-DMODULE_PROTOCOL_FCC)
endif()

if(MODULE_PROTOCOL_LBT)
  add_definitions(-DMODULE_PROTOCOL_LBT)
endif()

if(MODULE_PROTOCOL_FLEX)
  add_definitions(-DMODULE_PROTOCOL_FLEX)
endif()

if(AFHDS3)
add_definitions(-DAFHDS3)
endif()

if(PPM)
  add_definitions(-DPPM)
endif()
if(DSM2)
  add_definitions(-DDSM2)
endif()

if(SBUS)
  add_definitions(-DSBUS)
endif()

if(PXX1)
  add_definitions(-DPXX -DPXX1)
  set(PULSES_SRC
    ${PULSES_SRC}
    pxx1.cpp
    )
endif()

if(GHOST)
  add_definitions(-DGHOST)
  set(PULSES_SRC
    ${PULSES_SRC}
    ghost.cpp
    )
  set(SRC
    ${SRC}
    telemetry/ghost.cpp
    )
endif()

if(XJT)
  add_definitions(-DXJT)
endif()

if(MODULE_SIZE_STD)
  add_definitions(-DHARDWARE_EXTERNAL_MODULE_SIZE_STD)
endif()


add_definitions(-DCPUARM -DVIRTUAL_INPUTS)
add_definitions(-DTELEMETRY_FRSKY -DTELEMETRY_FRSKY_SPORT -DFRSKY_HUB -DGPS)
add_definitions(-DBOLD_FONT -DBATTGRAPH -DTHRTRACE -DGAUGES)
add_definitions(-DREQUIRED_SDCARD_VERSION="${SDCARD_VERSION}")
include_directories(${COOS_DIR} ${COOS_DIR}/kernel ${COOS_DIR}/portable)
foreach(LANGUAGE ${TTS_LANGUAGES})
  string(TOLOWER ${LANGUAGE} lang_lower)
  set(SRC ${SRC} translations/tts_${lang_lower}.cpp)
endforeach()

set(SRC
  ${SRC}
  main_arm.cpp
  tasks_arm.cpp
  audio_arm.cpp
  io/frsky_sport.cpp
  telemetry/telemetry.cpp
  telemetry/telemetry_holders.cpp
  telemetry/telemetry_sensors.cpp
  telemetry/frsky.cpp
  telemetry/frsky_d_arm.cpp
  telemetry/frsky_sport.cpp
  telemetry/flysky_nv14.cpp
  crc.cpp
  vario.cpp
  )


if(NOT MSVC)
  set(FIRMWARE_SRC
    ${FIRMWARE_SRC}
    ${THIRDPARTY_DIR}/DSP_Lib/Source/CommonTables/arm_common_tables.c
    ${THIRDPARTY_DIR}/DSP_Lib/Source/FastMathFunctions/arm_cos_f32.c
    ${THIRDPARTY_DIR}/DSP_Lib/Source/FastMathFunctions/arm_sin_f32.c
    ${THIRDPARTY_DIR}/DSP_Lib/Source/FastMathFunctions/arm_sin_cos_f32.c
    )
endif()

set(FIRMWARE_TARGET_SRC
  ${FIRMWARE_TARGET_SRC}
  keys_driver.cpp
  diskio.cpp
)
set(TARGET_SRC
  ${TARGET_SRC}
  pulses_driver.cpp
  telemetry_driver.cpp
  )
set(FIRMWARE_SRC
  ${FIRMWARE_SRC}
  ${COOS_DIR}/kernel/core.c
  ${COOS_DIR}/kernel/hook.c
  ${COOS_DIR}/kernel/task.c
  ${COOS_DIR}/kernel/event.c
  ${COOS_DIR}/kernel/time.c
  ${COOS_DIR}/kernel/timer.c
  ${COOS_DIR}/kernel/flag.c
  ${COOS_DIR}/kernel/mutex.c
  ${COOS_DIR}/kernel/sem.c
  ${COOS_DIR}/kernel/serviceReq.c
  ${COOS_DIR}/portable/GCC/port.c
  ${COOS_DIR}/portable/arch.c
  )
set(PULSES_SRC
  ${PULSES_SRC}
  pulses_arm.cpp
  ppm_arm.cpp
)


if(DSM2)
  set(PULSES_SRC
    ${PULSES_SRC}
    dsm2_arm.cpp
    )
endif()

if(SBUS)
  set(PULSES_SRC
    ${PULSES_SRC}
    sbus.cpp
    )
endif()
