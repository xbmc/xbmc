#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#   Copyright (C) 2021 Team Kodi
#   This file is part of Kodi - https://kodi.tv
#
#   SPDX-License-Identifier: GPL-2.0-or-later
#   See LICENSES/README.md for more information.

KODI_DIR = "../../../../../"
DEVKIT_DIR = "xbmc/addons/kodi-dev-kit"

# Global includes
from optparse import OptionParser

# Own includes
from src.commitChanges import *
from src.generateCMake__CMAKE_TREEDATA_COMMON_addon_dev_kit_txt import *
from src.generateCMake__XBMC_ADDONS_KODIDEVKIT_INCLUDE_KODI_allfiles import *
from src.helper_Log import *

# ===============================================================================
def GenerateCMakeParts(options):
    Log.PrintGroupStart("Generate cmake parts")

    # Generate Kodi's cmake system related include files to find related parts
    GenerateCMake__XBMC_ADDONS_KODIDEVKIT_INCLUDE_KODI_all_files(options)
    GenerateCMake__CMAKE_TREEDATA_COMMON_addon_dev_kit_txt(options)


# ===============================================================================
if __name__ == "__main__":
    # parse command-line options
    disc = """\
This utility autogenerate the interface between Kodi and addon.
It is currently still in the initial phase and will be expanded in the future.
"""
    parser = OptionParser(description=disc)
    parser.add_option(
        "-f",
        "--force",
        action="store_true",
        dest="force",
        default=False,
        help="Force the generation of auto code",
    )
    parser.add_option(
        "-d",
        "--debug",
        action="store_true",
        dest="debug",
        default=False,
        help="Add debug identifiers to generated files",
    )
    parser.add_option(
        "-c",
        "--commit",
        action="store_true",
        dest="commit",
        default=False,
        help="Create automatic a git commit about API changes (WARNING: No hand edits should be present before!)",
    )
    (options, args) = parser.parse_args()

    Log.Init(options)
    Log.PrintMainStart(options)

    ##----------------------------------------------------------------------------
    # CMake generation
    GenerateCMakeParts(options)

    ##----------------------------------------------------------------------------
    # Commit GIT changes generation (only makes work if '-c' option is used)
    if options.commit:
        Log.PrintGroupStart("Git update")
        CommitChanges(options)
