/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ICodec.h"

class CachingCodec : public ICodec
{
public:
  virtual ~CachingCodec() {}
  virtual int GetCacheLevel() const { return -1; }
};
