/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "iimage.h"
#include "threads/CriticalSection.h"
#include "URL.h"

class ImageFactory
{
public:
  ImageFactory() = default;
  virtual ~ImageFactory() = default;

  static IImage* CreateLoader(const std::string& strFileName);
  static IImage* CreateLoader(const CURL& url);
  static IImage* CreateLoaderFromMimeType(const std::string& strMimeType);

private:
  static CCriticalSection m_createSec; //!< Critical section for add-on creation.
};
