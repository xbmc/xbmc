/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/LibraryGUIInfo.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/Directory.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "music/MusicDatabase.h"
#include "music/MusicLibraryQueue.h"
#include "profiles/ProfileManager.h"
#include "settings/SettingsComponent.h"
#include "utils/FileUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoLibraryQueue.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <optional>
#include <type_traits>
#include <utility>

using namespace KODI::GUILIB::GUIINFO;

namespace
{
//! Asks a database of its own; nullopt if it cannot be opened
template<typename Database, typename Question>
auto Ask(Question&& question) -> std::optional<std::invoke_result_t<Question, Database&>>
{
  Database db;
  if (!db.Open())
    return {};
  return std::forward<Question>(question)(db);
}

std::optional<bool> HasSongs()
{
  return Ask<CMusicDatabase>([](CMusicDatabase& db) { return db.GetSongsCount() > 0; });
}

std::optional<bool> HasSingles()
{
  return Ask<CMusicDatabase>([](CMusicDatabase& db) { return db.GetSinglesCount() > 0; });
}

std::optional<bool> HasCompilations()
{
  return Ask<CMusicDatabase>([](CMusicDatabase& db) { return db.GetCompilationAlbumsCount() > 0; });
}

std::optional<bool> HasBoxsets()
{
  return Ask<CMusicDatabase>([](CMusicDatabase& db) { return db.GetBoxsetsCount() > 0; });
}

std::optional<bool> HasMovieSets()
{
  return Ask<CVideoDatabase>([](CVideoDatabase& db) { return db.HasSets(); });
}

template<VideoDbContentType Content>
std::optional<bool> HasVideos()
{
  return Ask<CVideoDatabase>([](CVideoDatabase& db) { return db.HasContent(Content); });
}

//! A kind of content a skin can ask whether the library has, and how that is found out
struct LibraryContent
{
  int condition;
  std::optional<bool> (*ask)();
};

constexpr std::array CONTENTS{
    LibraryContent{LIBRARY_HAS_MUSIC, HasSongs},
    LibraryContent{LIBRARY_HAS_MOVIES, HasVideos<VideoDbContentType::MOVIES>},
    LibraryContent{LIBRARY_HAS_MOVIE_SETS, HasMovieSets},
    LibraryContent{LIBRARY_HAS_TVSHOWS, HasVideos<VideoDbContentType::TVSHOWS>},
    LibraryContent{LIBRARY_HAS_MUSICVIDEOS, HasVideos<VideoDbContentType::MUSICVIDEOS>},
    LibraryContent{LIBRARY_HAS_SINGLES, HasSingles},
    LibraryContent{LIBRARY_HAS_COMPILATIONS, HasCompilations},
    LibraryContent{LIBRARY_HAS_BOXSETS, HasBoxsets},
};

//! The kind of content a condition asks about; nullopt for another condition
std::optional<std::size_t> KindOf(int condition)
{
  const auto found{std::ranges::find(CONTENTS, condition, &LibraryContent::condition)};
  if (found == CONTENTS.end())
    return {};
  return static_cast<std::size_t>(std::distance(CONTENTS.begin(), found));
}
} // namespace

CLibraryGUIInfo::CLibraryGUIInfo()
{
  ResetLibraryBools();
}

bool CLibraryGUIInfo::GetLibraryBool(int condition) const
{
  bool value = false;
  GetBool(value, nullptr, 0, CGUIInfo(condition));
  return value;
}

void CLibraryGUIInfo::SetLibraryBool(int condition, bool value)
{
  if (const std::optional<std::size_t> kind{KindOf(condition)})
    m_libraryHas[*kind] = value ? 1 : 0;
}

void CLibraryGUIInfo::ResetLibraryBools()
{
  m_libraryHas.fill(-1);
  m_libraryRoleCounts.clear();
}

bool CLibraryGUIInfo::HasContent(std::size_t kind) const
{
  static_assert(CONTENTS.size() == std::tuple_size_v<decltype(m_libraryHas)>);

  int& has{m_libraryHas[kind]};
  // A database that cannot be opened is asked again next time
  if (has < 0)
  {
    if (const std::optional<bool> asked{CONTENTS[kind].ask()})
      has = *asked ? 1 : 0;
  }
  return has > 0;
}

bool CLibraryGUIInfo::InitCurrentItem(CFileItem* item)
{
  return false;
}

bool CLibraryGUIInfo::GetLabel(std::string& value,
                               const CFileItem* item,
                               int contextWindow,
                               const CGUIInfo& info,
                               std::string* fallback) const
{
  return false;
}

bool CLibraryGUIInfo::GetInt(int& value,
                             const CGUIListItem* gitem,
                             int contextWindow,
                             const CGUIInfo& info) const
{
  return false;
}

bool CLibraryGUIInfo::GetBool(bool& value,
                              const CGUIListItem* gitem,
                              int contextWindow,
                              const CGUIInfo& info) const
{
  if (const std::optional<std::size_t> kind{KindOf(info.GetInfo())})
  {
    value = HasContent(*kind);
    return true;
  }

  switch (info.GetInfo())
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // LIBRARY_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case LIBRARY_HAS_VIDEO:
    {
      return (GetBool(value, gitem, contextWindow, CGUIInfo(LIBRARY_HAS_MOVIES)) ||
              GetBool(value, gitem, contextWindow, CGUIInfo(LIBRARY_HAS_TVSHOWS)) ||
              GetBool(value, gitem, contextWindow, CGUIInfo(LIBRARY_HAS_MUSICVIDEOS)));
    }
    case LIBRARY_HAS_ROLE:
    {
      std::string strRole = info.GetData3();
      // Find value for role if already stored
      int artistcount = -1;
      for (const auto& [role, artists] : m_libraryRoleCounts)
      {
        if (StringUtils::EqualsNoCase(strRole, role))
        {
          artistcount = artists;
          break;
        }
      }
      // Otherwise get from DB and store
      if (artistcount < 0)
      {
        if (const std::optional<int> count{Ask<CMusicDatabase>(
                [&strRole](CMusicDatabase& db) { return db.GetArtistCountForRole(strRole); })})
        {
          artistcount = *count;
          m_libraryRoleCounts.emplace_back(strRole, artistcount);
        }
      }
      value = artistcount > 0;
      return true;
    }
    case LIBRARY_HAS_NODE:
    {
      const CURL url(info.GetData3());
      const std::shared_ptr<CProfileManager> profileManager =
          CServiceBroker::GetSettingsComponent()->GetProfileManager();
      CFileItemList items;

      std::string libDir = profileManager->GetLibraryFolder();
      XFILE::CDirectory::GetDirectory(libDir, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS);
      if (items.IsEmpty())
        libDir = "special://xbmc/system/library/";

      std::string nodePath = URIUtils::AddFileToFolder(libDir, url.GetHostName() + "/");
      nodePath = URIUtils::AddFileToFolder(nodePath, url.GetFileName());
      value = CFileUtils::Exists(nodePath);
      return true;
    }
    case LIBRARY_IS_SCANNING:
    {
      value = (CMusicLibraryQueue::GetInstance().IsScanningLibrary() ||
               CVideoLibraryQueue::GetInstance().IsScanningLibrary());
      return true;
    }
    case LIBRARY_IS_SCANNING_VIDEO:
    {
      value = CVideoLibraryQueue::GetInstance().IsScanningLibrary();
      return true;
    }
    case LIBRARY_IS_SCANNING_MUSIC:
    {
      value = CMusicLibraryQueue::GetInstance().IsScanningLibrary();
      return true;
    }
    default:
      break;
  }

  return false;
}
