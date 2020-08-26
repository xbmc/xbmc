/*
*  Copyright (C) 2010-2020 Team Kodi
*  This file is part of Kodi - https://kodi.tv
*
*  SPDX-License-Identifier: GPL-2.0-or-later
*  See LICENSES/README.md for more information.
*/

#pragma once

class CDarwinEmbedUtils
{
public:
  static const char* GetAppRootFolder(void);
  static bool IsIosSandboxed(void);
};
