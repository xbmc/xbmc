/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

namespace XFILE
{

class CCDDADirectory :
      public IDirectory
{
public:
  CCDDADirectory(void);
  ~CCDDADirectory(void) override;
  bool GetDirectory(const CURL& url, CFileItemList &items) override;
};
}
