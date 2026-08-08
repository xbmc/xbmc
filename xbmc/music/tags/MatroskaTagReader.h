/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>
#include <vector>

class CURL;
struct AVFormatContext;

namespace MUSIC_INFO
{
//! Tags of one chapter and its play range, both from the reader that read the file.
struct ChapterTags
{
  std::map<std::string, std::string> tags;
  double start = 0.0; //!< seconds
  double end = 0.0;
};

/*!
 * \brief Matroska tags of a whole file: album level, plus one entry per chapter in file order.
 *
 * Every chapter the file declares is here, with a play range both readers fill. Whether a chapter
 * is short enough not to be a track is the library's call and is made by the caller, so that the
 * two readers describe the same file the same way.
 */
struct MatroskaAlbum
{
  std::map<std::string, std::string> fileTags;
  std::vector<ChapterTags> chapters;

  //! Whether the file carries album level tags, which is what makes it worth expanding.
  bool hasAlbumTags() const { return !fileTags.empty(); }
};

/*!
 * \brief Read a Matroska file's album level tags and its chapters.
 *
 * A build has one reader, TagLib's Matroska API or FFmpeg's demuxer, picked in the .cpp from
 * HAS_TAGLIB_MATROSKA - see TagLibVersion.h. Whichever it is reads a file whole: callers must not
 * take some of an album from here and the rest from elsewhere, because the two readers need not
 * agree on how many chapters a file has nor on their order. The tag names both produce mean the
 * same thing; MatroskaTagMapping is where that is settled.
 *
 * \param url The file. Supply it whichever reader this build has.
 * \param fctx A demuxer context already opened on the file. Supply it too: one reader reaches the
 *             file through it, the other through the URL, and which one this is is not the
 *             caller's business.
 * \return What the file holds. Both members are empty for a file the reader could not read, so
 *         what counts as an album is the caller's to decide.
 */
MatroskaAlbum ReadMatroskaTags(const CURL& url, const AVFormatContext* fctx);

/*!
 * \brief Read with FFmpeg's demuxer, whether or not it is the reader this build uses.
 * \param fctx A demuxer context already opened on the file. This reader has no other way to it.
 */
MatroskaAlbum ReadMatroskaTagsWithFFmpeg(const CURL& url, const AVFormatContext* fctx);

/*!
 * \brief Read with TagLib's Matroska API. Defined only where HAS_TAGLIB_MATROSKA is.
 * \param url The file. This reader opens it itself.
 * \param fctx Unused, and named only to match ReadMatroskaTagsWithFFmpeg so that
 *             ReadMatroskaTags() calls either the same way.
 */
MatroskaAlbum ReadMatroskaTagsWithTagLib(const CURL& url, const AVFormatContext* fctx);
} // namespace MUSIC_INFO
