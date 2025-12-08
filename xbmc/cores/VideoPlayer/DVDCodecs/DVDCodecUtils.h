/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Video/DVDVideoCodec.h"

class AVStream;
struct AVFormatContext;

class CDVDCodecUtils
{
public:
  static bool IsVP3CompatibleWidth(int width);
  static double NormalizeFrameduration(double frameduration, bool *match = nullptr);
  static bool IsH264AnnexB(std::string format, AVStream *avstream);
  static bool ProcessH264MVCExtradata(uint8_t *extradata, uint32_t extradata_size, uint8_t **mvc_extradata = nullptr, uint32_t *mvc_extradata_size = nullptr);
  static bool GetH264MvcStreamIndex(AVFormatContext *fmt, int *mvcIndex);
};

