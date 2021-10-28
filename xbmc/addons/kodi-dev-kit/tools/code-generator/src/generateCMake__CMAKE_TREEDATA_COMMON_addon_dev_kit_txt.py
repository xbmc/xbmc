# -*- coding: utf-8 -*-

#   Copyright (C) 2021 Team Kodi
#   This file is part of Kodi - https://kodi.tv
#
#   SPDX-License-Identifier: GPL-2.0-or-later
#   See LICENSES/README.md for more information.

# Own includes
from code_generator import DEVKIT_DIR, KODI_DIR
from .helper_Log import *

# Global includes
import glob, os, re


def GenerateCMake__CMAKE_TREEDATA_COMMON_addon_dev_kit_txt_RelatedCheck(filename):
    """
    This function is called by git update to be able to assign changed files to the dev kit.
    """
    return True if filename == "cmake/treedata/common/addon_dev_kit.txt" else False


def GenerateCMake__CMAKE_TREEDATA_COMMON_addon_dev_kit_txt(options):
    """
    This function generate the "cmake/treedata/common/addon_dev_kit.txt"
    by scan of related directories to use for addon interface build.
    """
    gen_file = "cmake/treedata/common/addon_dev_kit.txt"

    Log.PrintBegin("Check for {}".format(gen_file))

    scan_dir = "{}{}/include/kodi/**/CMakeLists.txt".format(KODI_DIR, DEVKIT_DIR)
    parts = "# Auto generated {}.\n" "# See {}/tools/code-generator.py.\n\n".format(
        gen_file, DEVKIT_DIR
    )

    for entry in glob.glob(scan_dir, recursive=True):
        cmake_dir = entry.replace(KODI_DIR, "").replace("/CMakeLists.txt", "")
        with open(entry) as search:
            for line in search:
                line = line.rstrip()  # remove '\n' at end of line
                m = re.search("^ *core_add_devkit_header\((.*)\)", line)
                if m:
                    parts += "{} addons_kodi-dev-kit_include_{}\n".format(
                        cmake_dir, m.group(1)
                    )
                    break
    file = "{}{}".format(KODI_DIR, gen_file)
    present = os.path.isfile(file)
    if not present or parts != open(file).read() or options.force:
        with open(file, "w") as f:
            f.write(parts)
        Log.PrintResult(Result.NEW if not present else Result.UPDATE)
    else:
        Log.PrintResult(Result.ALREADY_DONE)
