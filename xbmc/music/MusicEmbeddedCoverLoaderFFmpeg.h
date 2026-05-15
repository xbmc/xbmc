/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

struct AVFormatContext;
class EmbeddedArt;

namespace MUSIC_INFO
{
class CMusicInfoTag;

class CMusicEmbeddedCoverLoaderFFmpeg
{
public:
  /*!
 * \brief Searches the supplied AVFormatContext for the first stream containing a supported embedded
 *  picture.
 *  This function is called by CMusicInfoTagLoaderFFmpeg when browsing mka music in files view, and
 *   by CAudioBookFileDirectory when scanning mka files into the music library.
 *
 *  If art is found, it is added to CMusicInfoTag and EmbeddedArt (if available).
 *
 *  If there is no embedded picture or it is not of type jpg/png/bmp then no art is set.
 *
 * \param fctx An AVFormatContext containing one or more streams, one of which may contain art
 * \param tag MusicInfoTag holding information about the current music track to which found art is
 *  added
 * \param art Class containing embedded art details (if available)
 */
  static void GetEmbeddedCover(AVFormatContext* fctx,
                               CMusicInfoTag& tag,
                               EmbeddedArt* art = nullptr);

  /*!
   *  Convenience overload that opens the file via FFmpeg internally.
   *  Use when the caller does not already have an AVFormatContext (e.g. the
   *  Matroska TagLib loader). Returns false if the file could not be opened.
   */
  static bool GetEmbeddedCover(const std::string& strFileName,
                               CMusicInfoTag& tag,
                               EmbeddedArt* art = nullptr);
};
} // namespace MUSIC_INFO
