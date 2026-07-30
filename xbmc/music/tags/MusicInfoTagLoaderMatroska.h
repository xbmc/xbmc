/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ImusicInfoTagLoader.h"
#include "MatroskaTagLibStream.h"
#include "MusicInfoTag.h"
#include "utils/EmbeddedArt.h"

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include <taglib/taglib.h>

// TagLib's Matroska API needs 2.3.1 - earlier 2.x crash on an invalid seek head and drop
// chapters carrying no UID.
#if (TAGLIB_MAJOR_VERSION > 2) ||                                                                  \
    (TAGLIB_MAJOR_VERSION == 2 &&                                                                  \
     (TAGLIB_MINOR_VERSION > 3 || (TAGLIB_MINOR_VERSION == 3 && TAGLIB_PATCH_VERSION >= 1)))

namespace MUSIC_INFO
{
enum class MatroskaTrackTagRoute
{
  BindFirstChapter, //!< Single-chapter file: bind to the only chapter
  BindChapterUid, //!< Multi-chapter: bind by TagChapterUID (caller falls back if missing)
  BindFileTags //!< Multi-chapter with no ChapterUID, or unknown-UID fallback target
};

class CMusicInfoTagLoaderMatroska : public IMusicInfoTagLoader
{
public:
  using ChapterOrder =
      std::vector<std::tuple<unsigned long long, std::string, double, double, unsigned long long>>;

  CMusicInfoTagLoaderMatroska() = default;
  ~CMusicInfoTagLoaderMatroska() override = default;

  bool Load(const std::string& strFileName,
            CMusicInfoTag& tag,
            EmbeddedArt* art = nullptr) override;

  static void ParseTag(const std::string& key,
                       const std::string& value,
                       std::vector<std::string>& separators,
                       const std::string& musicsep,
                       CMusicInfoTag& tag);

  // Static overload for external callers (e.g. AudioBookFileDirectory) —
  // opens its own MatroskaTagLibStream internally.
  // If coverTag is non-null, embedded cover art info is set on it.
  static void GetMatroskaMusicTags(
      const std::string& fileName,
      std::map<std::string, std::string>& fileTags,
      std::map<unsigned long long, std::map<std::string, std::string>>& chapterTags,
      ChapterOrder& chapterOrder,
      CMusicInfoTag* coverTag = nullptr);

  // Placeholder UID used when TagLib finds no Chapters element, so song-level
  // tags with targetTypeValue 30 still have somewhere to land. Not a real track.
  static constexpr unsigned long long DummyChapterUid = 999000999000999ULL;

  //! True unless both album-level tags and the chapter list are empty.
  static bool HasMatroskaDirectoryContent(const std::map<std::string, std::string>& fileTags,
                                          const ChapterOrder& chapterOrder);

  //! False for the synthetic DummyChapterUid placeholder only.
  static constexpr bool IsRealChapterUid(unsigned long long uid)
  {
    return uid != DummyChapterUid;
  }

  static size_t CountRealChapters(const ChapterOrder& chapterOrder);

  //! Where a targetTypeValue 30 simple tag should bind.
  static MatroskaTrackTagRoute ResolveTargetType30Route(int chapterCount,
                                                        unsigned long long chapterUid);

private:
  // Internal overload used by Load() — reuses an already-open stream
  static void GetMatroskaMusicTags(
      const std::string& fileName,
      MatroskaTagLibStream& matroskaStream,
      std::map<std::string, std::string>& fileTags,
      std::map<unsigned long long, std::map<std::string, std::string>>& chapterTags,
      ChapterOrder& chapterOrder,
      CMusicInfoTag* coverTag = nullptr,
      EmbeddedArt* art = nullptr);

  static void AddRole(const std::vector<std::string>& data,
                      const std::vector<std::string>& separators,
                      CMusicInfoTag& musictag);
  static void AddCommaDelimitedString(const std::vector<std::string>& data,
                                      const std::vector<std::string>& separators,
                                      CMusicInfoTag& musictag);
};
} // namespace MUSIC_INFO

#endif // TagLib >= 2.3.1
