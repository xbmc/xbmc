/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#include "BlurayDirectory.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "DllLibbluray.h"
#include "FileItem.h"
#include "video/VideoInfoTag.h"
#include "guilib/LocalizeStrings.h"

#include <cassert>
#include <array>
#include <climits>
#include <stdlib.h>
#include <string>
#include "utils/LangCodeExpander.h"
#include "File.h"
#include "utils/RegExp.h"

namespace XFILE
{

#define MAIN_TITLE_LENGTH_PERCENT 70 /** Minimum length of main titles, based on longest title */

CBlurayDirectory::CBlurayDirectory()
  : m_dll(NULL)
  , m_bd(NULL)
{
}

CBlurayDirectory::~CBlurayDirectory()
{
  Dispose();
}

void CBlurayDirectory::Dispose()
{
  if(m_bd)
  {
    m_dll->bd_close(m_bd);
    m_bd = NULL;
  }
  delete m_dll;
  m_dll = NULL;
}

std::string CBlurayDirectory::GetBlurayTitle()
{
  return GetDiscInfoString(DiscInfo::TITLE);
}

std::string CBlurayDirectory::GetBlurayID()
{
  return GetDiscInfoString(DiscInfo::ID);
}

std::string CBlurayDirectory::GetDiscInfoString(DiscInfo info)
{
  switch (info)
  {
  case XFILE::CBlurayDirectory::DiscInfo::TITLE:
  {
    if (!m_blurayInitialized)
      return "";
    const BLURAY_DISC_INFO* disc_info = m_dll->bd_get_disc_info(m_bd);
    if (!disc_info || !disc_info->bluray_detected)
      return "";

    std::string title = "";

#if (BLURAY_VERSION > BLURAY_VERSION_CODE(1,0,0))
    title = disc_info->disc_name ? disc_info->disc_name : "";
#endif

    return title;
  }
  case XFILE::CBlurayDirectory::DiscInfo::ID:
  {
    if (!m_blurayInitialized)
      return "";

    const BLURAY_DISC_INFO* disc_info = m_dll->bd_get_disc_info(m_bd);
    if (!disc_info || !disc_info->bluray_detected)
      return "";

    std::string id = "";

#if (BLURAY_VERSION > BLURAY_VERSION_CODE(1,0,0))
    id = disc_info->udf_volume_id ? disc_info->udf_volume_id : "";

    if (id.empty())
    {
      id = HexToString(disc_info->disc_id, 20);
    }
#endif

    return id;
  }
  default:
    break;
  }

  return "";
}

CFileItemPtr CBlurayDirectory::GetTitle(const BLURAY_TITLE_INFO* title, const std::string& label)
{
  std::string buf;
  std::string chap;
  CFileItemPtr item(new CFileItem("", false));
  CURL path(m_url);
  buf = StringUtils::Format("BDMV/PLAYLIST/%05d.mpls", title->playlist);
  path.SetFileName(buf);
  item->SetPath(path.Get());
  int duration = (int)(title->duration / 90000);
  item->GetVideoInfoTag()->SetDuration(duration);
  item->GetVideoInfoTag()->m_iTrack = title->playlist;
  buf = StringUtils::Format(label.c_str(), title->playlist);
  item->m_strTitle = buf;
  item->SetLabel(buf);
  chap = StringUtils::Format(g_localizeStrings.Get(25007).c_str(), title->chapter_count, StringUtils::SecondsToTimeString(duration).c_str());
  item->SetLabel2(chap);
  item->m_dwSize = 0;
  item->SetIconImage("DefaultVideo.png");
  for(unsigned int i = 0; i < title->clip_count; ++i)
    item->m_dwSize += title->clips[i].pkt_count * 192;

  return item;
}

void CBlurayDirectory::GetTitles(bool main, CFileItemList &items)
{
  std::vector<BLURAY_TITLE_INFO*> titleList;
  uint64_t minDuration = 0;

  // Searching for a user provided list of playlists. 
  if (main)
    titleList = GetUserPlaylists();

  if (!main || titleList.empty())
  {
    uint32_t numTitles = m_dll->bd_get_titles(m_bd, TITLES_RELEVANT, 0);

    for (uint32_t i = 0; i < numTitles; i++)
    {
      BLURAY_TITLE_INFO* t = m_dll->bd_get_title_info(m_bd, i, 0);

      if (!t)
      {
        CLog::Log(LOGDEBUG, "CBlurayDirectory - unable to get title %d", i);
        continue;
      }

      if (main && t->duration > minDuration)
          minDuration = t->duration;

      titleList.emplace_back(t);
    }
  }

  minDuration = minDuration * MAIN_TITLE_LENGTH_PERCENT / 100;

  for (auto& title : titleList)
  {
    if (title->duration < minDuration)
      continue;

    items.Add(GetTitle(title, main ? g_localizeStrings.Get(25004) /* Main Title */ : g_localizeStrings.Get(25005) /* Title */));
    m_dll->bd_free_title_info(title);
  }
}

void CBlurayDirectory::GetRoot(CFileItemList &items)
{
    GetTitles(true, items);

    CURL path(m_url);
    CFileItemPtr item;

    path.SetFileName(URIUtils::AddFileToFolder(m_url.GetFileName(), "titles"));
    item.reset(new CFileItem());
    item->SetPath(path.Get());
    item->m_bIsFolder = true;
    item->SetLabel(g_localizeStrings.Get(25002) /* All titles */);
    item->SetIconImage("DefaultVideoPlaylists.png");
    items.Add(item);

    path.SetFileName("menu");
    item.reset(new CFileItem());
    item->SetPath(path.Get());
    item->m_bIsFolder = false;
    item->SetLabel(g_localizeStrings.Get(25003) /* Menus */);
    item->SetIconImage("DefaultProgram.png");
    items.Add(item);
}

bool CBlurayDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  Dispose();
  m_url = url;
  std::string root = m_url.GetHostName();
  std::string file = m_url.GetFileName();
  URIUtils::RemoveSlashAtEnd(file);
  URIUtils::RemoveSlashAtEnd(root);

  if (!InitializeBluray(root))
    return false;

  if(file == "root")
    GetRoot(items);
  else if(file == "root/titles")
    GetTitles(false, items);
  else
  {
    CURL url2 = GetUnderlyingCURL(url);
    CDirectory::CHints hints;
    hints.flags = m_flags;
    if (!CDirectory::GetDirectory(url2, items, hints))
      return false;
  }

  items.AddSortMethod(SortByTrackNumber,  554, LABEL_MASKS("%L", "%D", "%L", ""));    // FileName, Duration | Foldername, empty
  items.AddSortMethod(SortBySize,         553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // FileName, Size | Foldername, Size

  return true;
}

CURL CBlurayDirectory::GetUnderlyingCURL(const CURL& url)
{
  assert(url.IsProtocol("bluray"));
  std::string host = url.GetHostName();
  std::string filename = url.GetFileName();
  return CURL(host.append(filename));
}

bool CBlurayDirectory::InitializeBluray(const std::string &root)
{
  m_dll = new DllLibbluray();
  if (!m_dll->Load())
  {
    CLog::Log(LOGERROR, "CBlurayDirectory::InitializeBluray - failed to load dll");
    return false;
  }

  m_dll->bd_set_debug_handler(DllLibbluray::bluray_logger);
  m_dll->bd_set_debug_mask(DBG_CRIT | DBG_BLURAY | DBG_NAV);

  m_bd = m_dll->bd_init();

  if (!m_bd)
  {
    CLog::Log(LOGERROR, "CBlurayDirectory::InitializeBluray - failed to initialize libbluray");
    return false;
  }

  std::string langCode;
  g_LangCodeExpander.ConvertToISO6392T(g_langInfo.GetDVDMenuLanguage(), langCode);
  m_dll->bd_set_player_setting_str(m_bd, BLURAY_PLAYER_SETTING_MENU_LANG, langCode.c_str());
  
  if (!m_dll->bd_open_files(m_bd, const_cast<std::string*>(&root), DllLibbluray::dir_open, DllLibbluray::file_open))
  {
    CLog::Log(LOGERROR, "CBlurayDirectory::InitializeBluray - failed to open %s", CURL::GetRedacted(root).c_str());
    return false;
  }
  m_blurayInitialized = true;

  return true;
}

std::string CBlurayDirectory::HexToString(const uint8_t *buf, int count)
{
  std::array<char, 42> tmp;

  for (int i = 0; i < count; i++)
  {
    sprintf(tmp.data() + (i * 2), "%02x", buf[i]);
  }

  return std::string(std::begin(tmp), std::end(tmp));
}

std::vector<BLURAY_TITLE_INFO*> CBlurayDirectory::GetUserPlaylists()
{
  std::string root = m_url.GetHostName();
  std::string discInfPath = URIUtils::AddFileToFolder(root, "disc.inf");
  std::vector<BLURAY_TITLE_INFO*> userTitles;
  CFile file;
  char buffer[1025];

  if (file.Open(discInfPath))
  {
    CLog::Log(LOGDEBUG, "CBlurayDirectory::GetTitles - disc.inf found");

    CRegExp pl(true);
    if (!pl.RegComp("(\\d+)"))
    {
      file.Close();
      return userTitles;
    }

    uint8_t maxLines = 100;
    while ((maxLines > 0) && file.ReadString(buffer, 1024))
    {
      maxLines--;
      if (StringUtils::StartsWithNoCase(buffer, "playlists"))
      {
        int pos = 0;
        while ((pos = pl.RegFind(buffer, static_cast<unsigned int>(pos))) >= 0)
        {
          std::string playlist = pl.GetMatch(0);
          uint32_t len = static_cast<uint32_t>(playlist.length());

          if (len <= 5)
          {
            unsigned long int plNum = strtoul(playlist.c_str(), nullptr, 10);

            BLURAY_TITLE_INFO* t = m_dll->bd_get_playlist_info(m_bd, static_cast<uint32_t>(plNum), 0);
            if (t)
              userTitles.emplace_back(t);
          }

          if (static_cast<int64_t>(pos) + static_cast<int64_t>(len) > INT_MAX)
            break;
          else
            pos += len;
        }
      }
    }
    file.Close();
  }
  return userTitles;
}

} /* namespace XFILE */
