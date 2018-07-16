/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ImusicInfoTagLoader.h"

class CFileItem;  // forward

namespace MUSIC_INFO
{
  class CMusicInfoTagLoaderFactory
  {
    public:
      CMusicInfoTagLoaderFactory(void);
      virtual ~CMusicInfoTagLoaderFactory();

      static IMusicInfoTagLoader* CreateLoader(const CFileItem& item);
  };
}

