/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "OSScreenSaverTVOS.h"

#import "platform/darwin/tvos/XBMCController.h"

void COSScreenSaverTVOS::Inhibit()
{
  [g_xbmcController disableScreenSaver];
}

void COSScreenSaverTVOS::Uninhibit()
{
  [g_xbmcController enableScreenSaver];
}
