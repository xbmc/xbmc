/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "BlurayDirectory.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "DllLibbluray.h"
#include "FileItem.h"
#include "video/VideoInfoTag.h"
#include "addons/Scraper.h"

namespace XFILE
{

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
    m_dll = NULL;
  }
  delete m_dll;
  m_dll = NULL;
}

CFileItemPtr CBlurayDirectory::GetTitle(const BLURAY_TITLE_INFO* title, const CStdString label)
{
  CStdString buf;
  CFileItemPtr item(new CFileItem("", false));
  CURL path(m_url);
  buf.Format("BDMV/PLAYLIST/%05d.mpls", title->playlist);
  path.SetFileName(buf);
  item->SetPath(path.Get());
  item->GetVideoInfoTag()->m_strRuntime.Format("%d",title->duration / 90000);
  item->GetVideoInfoTag()->m_iTrack = title->playlist;
  buf.Format("%s %d", label.c_str(), title->playlist);
  item->m_strTitle = buf;
  item->SetLabel(buf);
  item->m_dwSize = 0;
  for(unsigned int i = 0; i < title->clip_count; ++i)
    item->m_dwSize += title->clips[i].pkt_count * 192;

  return item;
}

void CBlurayDirectory::GetTitles(bool main, CFileItemList &items)
{
  unsigned titles = m_dll->bd_get_titles(m_bd, TITLES_RELEVANT, 0);
  CStdString buf;

  std::vector<BLURAY_TITLE_INFO*> buffer;

  uint64_t duration = 0;

  for(unsigned i=0; i < titles; i++)
  {
    BLURAY_TITLE_INFO *t = m_dll->bd_get_title_info(m_bd, i, 0);
    if(!t)
    {
      CLog::Log(LOGDEBUG, "CBlurayDirectory - unable to get title %d", i);
      continue;
    }
    if(t->duration > duration)
      duration = t->duration;

    buffer.push_back(t);
  }

  if(main)
    duration = duration * 2 /3;
  else
    duration = 0;

  for(std::vector<BLURAY_TITLE_INFO*>::iterator it = buffer.begin(); it != buffer.end(); ++it)
  {
    if((*it)->duration < duration)
      continue;
    items.Add(GetTitle(*it, main ? "Main Title" : "Title"));
  }


  for(std::vector<BLURAY_TITLE_INFO*>::iterator it = buffer.begin(); it != buffer.end(); ++it)
    m_dll->bd_free_title_info(*it);
}
bool CBlurayDirectory::GetOriginal(const CStdString& path, CFileItemList &items)
{
  CDirectory::CHints hints;
  hints.mask          = IDirectory::m_strFileMask;
  hints.flags         = IDirectory::m_flags;
  hints.scanning_type = IDirectory::m_scanningType;
  return CDirectory::GetDirectory(path, items
                                , hints
                                , false);
}
bool CBlurayDirectory::GetDirectory(const CStdString& path, CFileItemList &items)
{
  Dispose();
  m_url.Parse(path);
  CStdString root = m_url.GetHostName();
  CStdString file = m_url.GetFileName();
  URIUtils::RemoveSlashAtEnd(file);

  m_dll = new DllLibbluray();
  if (!m_dll->Load())
  {
    CLog::Log(LOGERROR, "CBlurayDirectory::GetDirectory - failed to load dll");
    return GetOriginal(root, items);
  }

  m_dll->bd_register_dir(DllLibbluray::dir_open);
  m_dll->bd_register_file(DllLibbluray::file_open);
  m_dll->bd_set_debug_handler(DllLibbluray::bluray_logger);
  m_dll->bd_set_debug_mask(DBG_CRIT | DBG_BLURAY | DBG_NAV);

  m_bd = m_dll->bd_open(root.c_str(), NULL);

  if(!m_bd)
  {
    CLog::Log(LOGERROR, "CBlurayDirectory::GetDirectory - failed to open %s", root.c_str());
    return GetOriginal(root, items);
  }

  if(file == "") {
    if(m_scanningType == CONTENT_MOVIES
    || m_scanningType == CONTENT_TVSHOWS) {
      /* only return main titles */
      GetTitles(true, items);
    } else {
      GetTitles(true, items);

      CURL path(m_url);
      CFileItemPtr item;

      path.SetFileName(URIUtils::AddFileToFolder(m_url.GetFileName(), "titles"));
      item.reset(new CFileItem());
      item->SetPath(path.Get());
      item->m_bIsFolder = true;
      item->SetLabel("All Titles");
      items.Add(item);

      path.SetFileName("BDMV/MovieObject.bdmv");
      item.reset(new CFileItem());
      item->SetPath(path.Get());
      item->m_bIsFolder = false;
      item->SetLabel("Menus");
      items.Add(item);
    }
  } else if(file == "titles") {
    GetTitles(false, items);
  } else {
    return false;
  }

  items.AddSortMethod(SORT_METHOD_TRACKNUM , 554, LABEL_MASKS("%L", "%D", "%L", ""));    // FileName, Duration | Foldername, empty
  items.AddSortMethod(SORT_METHOD_SIZE     , 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // FileName, Size | Foldername, Size

  return true;
}


} /* namespace XFILE */
