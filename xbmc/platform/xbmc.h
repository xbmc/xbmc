/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once
#include "XBApplicationEx.h"

class CAppParamParser;

extern "C" void XBMC_EnqueuePlayList(const CFileItemList &playlist, EnqueueOperation op);
extern "C" int XBMC_Run(bool renderGUI, const CAppParamParser &params);

