#
# Copyright (C) wir-sind-die-matrix.de
#
# License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

#!/usr/bin/env python

from __future__ import print_function
import os
import sys
import subprocess
import shutil
import time
from collections import OrderedDict

# some tools and colors
def error(str):
    print('\n' + '\033[41m' + str + '\033[0m' + '\n')
    sys.exit(1)

def header(str):
    print('\n' + '\033[44m' + str + '\033[0m' + '\n')
    
def info(str):
    print('\n' + '\033[30;43m' + str + '\033[0m' + '\n')

def mv(source, target):
    try:
        shutil.move(source, target)
    except:
        pass

def cp(source, target):
    try:
        shutil.copyfile(source, target)
    except:
        pass


# Environment checks
if not sys.version_info >= (3, 6):
    error("* Requires Python 3.6+.")

# Show a header
header('===========================  Nirvana NV14  ===========================')
header('* https://cloud.docker.com/repository/docker/derdoktor667/nv14-build *')

# Specify some paths for the build
build_dirty = "../"
build_dir_name = "/buildit"
output_dir_name = "/firmware_built"
output_extension = ".bin"
output_filename = "firmware"

base_dir = os.getcwd()
source_dir = base_dir
build_dir = (source_dir + build_dir_name)
output_dir = (source_dir + output_dir_name)

# Maximum size for the compiled firmware
nv14_max_size = ( 2 * 1024 * 1024 ) # 2MB - 2.097.152‬ byte

# Default NV14 cmake flags
nv14_default_options = OrderedDict([
    ("PCB", "NV14"),
    ("GUI", "YES"),
    ("GVARS", "NO"),
    ("HELI", "NO"),
    ("LCD_DUAL_BUFFER", "NO"),
    ("ALLOW_NIGHTLY_BUILDS", "YES"),
    ("LUA", "YES"),
    ("LUA_COMPILER", "YES"),
    ("SIMU_AUDIO", "NO"),
    ("SIMU_LUA_COMPILER", "NO"),
    ("USB_SERIAL", "YES"),
    ("MODULE_R9M_FULLSIZE", "YES"),
    ("MULTIMODULE", "YES"),
    ("AUTOSWITCH", "YES"),
    ("AUTOSOURCE", "YES"),
    ("PPM_CENTER_ADJUSTABLE", "YES"),
    ("RAS", "YES"),
    ("DISABLE_COMPANION", "YES"),
    ("BOOTLOADER", "NO"),
    ("CMAKE_BUILD_TYPE", "Release")
])

# Available languages
# , "FR", "SE", "IT", "CZ", "DE", "PT", "ES", "PL", "NL")
available_languages = ("EN")

# Ensure the source is valid
workbench_folder = build_dir

if not os.path.exists(source_dir + "/CMakeLists.txt"):
    error("* NV14 source not found. Where is the git repo?")

# Parse the extra options from the command line
extra_options = OrderedDict()

if "CMAKE_FLAGS" in os.environ:
    info("Additional CMAKE Flags: %s" % os.environ["CMAKE_FLAGS"])
    flags = os.environ["CMAKE_FLAGS"].split()
    for flag in flags:
        opt, val = flag.split("=")
        extra_options[opt] = val
else:
    info("No additional CMAKE flags specified.")

# If specified, get the PCB from the flags; default to the NV14
board = "NV14"
if "PCB" in extra_options:
    board = extra_options["PCB"].upper()

# Get the board defaults
if board == "NV14":
    default_options = nv14_default_options

else:
    error("* Invalid board (%s) specified. Valid board ONLY NV14." % board)

# If specified, validate the language from the flags
if "TRANSLATIONS" in extra_options:
    if not extra_options["TRANSLATIONS"].upper() in available_languages:
        error("ERROR: Invalid language (%s) specified. Valid languages are: %s." % (
            extra_options["TRANSLATIONS"], " ".join(available_languages)))

# Compare the extra options to the board's defaults
extra_command_options = OrderedDict()
for ext_opt, ext_value in extra_options.items():
    found = False
    for def_opt, def_value in default_options.items():
        if ext_opt == def_opt:
            found = True
            break

    if found:
        if ext_value != def_value:
            default_options[def_opt] = ext_value
            info("Overriding default flag: %s=%s => %s=%s" %
                 (def_opt, def_value, def_opt, ext_value))
        else:
            if def_opt != "PCB":
                info("Override for default flag matches default value: %s=%s" %
                     (def_opt, def_value))
    else:
        info("Adding additional flag: %s=%s" % (ext_opt, ext_value))
        extra_command_options[ext_opt] = ext_value

# Start the timer
start = time.time()

# create the build directory and enter
if not os.path.exists(build_dir):
    os.mkdir(build_dir)

os.chdir(build_dir)

#
info("Starting build...")

# Prepare the cmake command
buildCommand = ["cmake"]

# Append the default flags
for opt, value in default_options.items():
    buildCommand.append("-D%s=%s" % (opt, value))

# Append the extra flags
for opt, value in extra_command_options.items():
    buildCommand.append("-D%s=%s" % (opt, value))

# Append the source directory
buildCommand.append(build_dirty)

# Output the cmake command line
info("".join(buildCommand))

# Launch cmake
proc = subprocess.Popen(buildCommand)
proc.wait()

# Exit if cmake errored
if proc.returncode != 0:
    error("ERROR: cmake configuration failed.")

# Launch make
proc = subprocess.Popen(["make", "firmware"])
proc.wait()

# Exit if make errored
if proc.returncode != 0:
    error("ERROR: make compilation failed.")

# Stop the timer
end = time.time()

# Append the PCB type to the output file name
output_filename = output_filename + "-" + default_options["PCB"].lower()

# Get the firmware version
stampfile = "radio/src/stamp.h"

for line in open(stampfile):
    if "#define VERSION " in line:
        firmware_version = line.split()[2].replace('"', '')

# Append the version to the output file name
if firmware_version:
    output_filename = output_filename + "-" + firmware_version

# Append the language to the output file name if one is specified
if "TRANSLATIONS" in extra_command_options:
    output_filename = output_filename + "-" + \
        extra_command_options["TRANSLATIONS"].lower()

# Append the extension to the output file name
output_filename = output_filename + output_extension

# Assemble the output path
output_path = os.path.join(output_dir, output_filename)

# Move the new binary to the output path
mv("firmware.bin", output_path)

# Get the size of the binary
binsize = os.stat(output_path).st_size

# Print out the file name and size
info("Build completed in {0:.1f} seconds.".format((end-start)))
header("Firmware file: %s" % (output_path))

# Exit with an error if the firmware is too big
if binsize > nv14_max_size:
     error("ERROR: Firmware is too large for radio.")

# all went right ...well done
else:
    header("Built successfully!") 
    info("Firmware size: {0:.2f} KB ({1:.2f} %)".format(binsize / 1024, ((float(binsize) * 100) / float(nv14_max_size))))
    header('======================================================================')
