# -*- coding: utf-8 -*-

#   Copyright (C) 2021 Team Kodi
#   This file is part of Kodi - https://kodi.tv
#
#   SPDX-License-Identifier: GPL-2.0-or-later
#   See LICENSES/README.md for more information.

# Own includes
from code_generator import DEVKIT_DIR, KODI_DIR
from .generateCMake__CMAKE_TREEDATA_COMMON_addon_dev_kit_txt import *
from .generateCMake__XBMC_ADDONS_KODIDEVKIT_INCLUDE_KODI_allfiles import *
from .helper_Log import *

# Global includes
import importlib, os, subprocess

git_found = importlib.find_loader("git") is not None
if git_found:
    from git import Repo


def CommitChanges(options):
    """
    Do a git commit of the automatically changed locations of the dev-kit.
    """
    if not options.commit:
        return
    if not git_found:
        Log.PrintFatal(
            'Needed "GitPython" module not present! To make commits need them be installed.'
        )
        quit(1)

    Log.PrintBegin("Perform GIT update check")
    Log.PrintResult(Result.SEE_BELOW)

    contains_devkit_change = False
    contains_external_change = False
    changes_list = []

    # Hack place one to get also new added files
    subprocess.run(["git", "add", "-A"], check=True, stdout=subprocess.PIPE).stdout

    r = Repo(KODI_DIR)
    for x in r.index.diff("HEAD"):
        if GenerateCMake__XBMC_ADDONS_KODIDEVKIT_INCLUDE_KODI_all_files_RelatedCheck(
            x.b_path
        ):
            Log.PrintBegin(" - Changed file {}".format(x.b_path))
            contains_devkit_change = True
            changes_list.append(x.b_path)
            Log.PrintResult(Result.NEW if x.new_file else Result.UPDATE)
        elif GenerateCMake__CMAKE_TREEDATA_COMMON_addon_dev_kit_txt_RelatedCheck(
            x.b_path
        ):
            Log.PrintBegin(" - Changed file {}".format(x.b_path))
            contains_devkit_change = True
            changes_list.append(x.b_path)
            Log.PrintResult(Result.NEW if x.new_file else Result.UPDATE)
        else:
            Log.PrintBegin(" - Changed file {}".format(x.b_path))
            Log.PrintFollow(" (Not auto update related)")
            Log.PrintResult(Result.IGNORED)
            contains_external_change = True

    # Hack place two to reset about before
    subprocess.run(["git", "reset", "HEAD"], check=True, stdout=subprocess.PIPE).stdout

    Log.PrintBegin("Perform GIT commit")
    if contains_devkit_change:
        # Add changed or added files to git
        for x in changes_list:
            r.index.add(x)

        commit_msg = (
            "[auto][addons] devkit script update ({})\n"
            "\n"
            "This commit automatic generated by script '{}/tools/code-generator.py'.\n"
            "\n"
            "{}".format(
                datetime.utcnow().strftime("%d/%m/%Y %H:%M:%S"),
                DEVKIT_DIR,
                open(Log.log_file).read(),
            )
        )
        r.index.commit(commit_msg)
        Log.PrintFollow(" ( Commit SHA256: {})".format(str(r.head.reference.commit)))
        Log.PrintResult(Result.OK)
    else:
        Log.PrintResult(Result.ALREADY_DONE)
