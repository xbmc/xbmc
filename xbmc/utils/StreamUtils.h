/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>

static constexpr int MP4_BOX_HEADER_SIZE = 8;

class StreamUtils
{
public:
  static int GetCodecPriority(const std::string& codec);

  /*!
   * \brief Compare two audio streams on the technical quality of what they carry.
   *
   * \param codecA The codec name of the first stream, as GetCodecPriority() expects it
   * \param channelsA The channel count of the first stream, zero or negative when unknown
   * \param codecB The codec name of the second stream
   * \param channelsB The channel count of the second stream
   * \return A positive value when the first stream is better, a negative value when the second
   *         is, and zero when the two are equally good (which is not the same as interchangeable,
   *         as equally ranked codecs are different presentations)
   */
  static int CompareAudioQuality(const std::string& codecA,
                                 int channelsA,
                                 const std::string& codecB,
                                 int channelsB);

  /*!
   * \brief Make a FourCC code as unsigned integer value
   * \param c1 The first FourCC char
   * \param c2 The second FourCC char
   * \param c3 The third FourCC char
   * \param c4 The fourth FourCC char
   * \return The FourCC as unsigned integer value
   */
  static constexpr uint32_t MakeFourCC(char c1, char c2, char c3, char c4)
  {
    return ((static_cast<uint32_t>(c1) << 24) | (static_cast<uint32_t>(c2) << 16) |
            (static_cast<uint32_t>(c3) << 8) | (static_cast<uint32_t>(c4)));
  }

  /*!
   * \brief Get the codec name translated from ffmpeg codec id and profile
   * \param codecId The ffmpeg codec id
   * \param profile The ffmpeg codec profile
   * \return The codec name
   */
  static std::string GetCodecName(int codecId, int profile);

  /*!
   * \brief Normalise an externally supplied (eg. NFO) audio codec name to the name Kodi uses
   *
   * \param codec The audio codec name, lowercased
   * \return The equivalent Kodi codec name, or the codec name unchanged if nothing maps
   */
  static std::string NormalizeAudioCodecName(const std::string& codec);

  /*!
   * \brief Return a default channel layout in x.y.z form for a channel count.
   * \param[in] channels the count of channels
   * \return the default layout
   */
  static std::string GetDefaultLayout(unsigned int channels);

  /*!
   * \brief Return a default channel layout for a channel count or localized count of channels
   * when no default exists.
   * \param[in] channels the count of channels
   * \return the layout
   */
  static std::string GetLayout(unsigned int channels);

  /*!
   * \brief Determines if a codec support forced overlays (on image type subtitles).
   * \param codecId The ffmpeg codec id
   * \return True when support forced overlay, otherwise false
   */
  static bool IsCodecSupportForcedOverlay(int codecId);
};
