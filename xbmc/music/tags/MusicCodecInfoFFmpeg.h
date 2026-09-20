/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

struct AVFormatContext;

struct musicCodecInfo
{
public:
  int bitsPerSample = 0;
  int sampleRate = 0;
  int bitRate = 0;
  int channels = 0;
  int duration = 0;
  std::string codecName;
};

class CMusicCodecInfoFFmpeg
{
public:
  /*!
   * \brief Fill codec_info from a context the caller already opened.
   * \param fctx A context on which avformat_find_stream_info() has been called.
   * \param codec_info Filled from the default audio stream, or the first one.
   * \return Whether an audio stream was found.
   */
  static bool GetMusicCodecInfo(AVFormatContext* fctx, musicCodecInfo& codec_info);

  //! Convenience overload for a caller with no context of its own; opens the file itself.
  static bool GetMusicCodecInfo(const std::string& strFileName, musicCodecInfo& codec_info);
};
