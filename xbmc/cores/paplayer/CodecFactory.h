/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ICodec.h"

class CFileItem;

class CodecFactory
{
public:
  CodecFactory() = default;
  virtual ~CodecFactory() = default;
  static ICodec* CreateCodec(const std::string &type);
  static ICodec* CreateCodecDemux(const CFileItem& file, unsigned int filecache);
};

