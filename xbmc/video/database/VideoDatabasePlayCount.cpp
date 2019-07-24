/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabase.h"

#include <string>
#include <vector>

#include "Application.h"
#include "ServiceBroker.h"
#include "dbwrappers/dataset.h"
#include "FileItem.h"
#include "filesystem/MultiPathDirectory.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace dbiplus;
using namespace XFILE;

bool CVideoDatabase::GetPlayCounts(const std::string &strPath, CFileItemList &items)
{
  if(URIUtils::IsMultiPath(strPath))
  {
    std::vector<std::string> paths;
    CMultiPathDirectory::GetPaths(strPath, paths);

    bool ret = false;
    for(unsigned i=0;i<paths.size();i++)
      ret |= GetPlayCounts(paths[i], items);

    return ret;
  }
  int pathID = -1;
  if (!URIUtils::IsPlugin(strPath))
  {
    pathID = GetPathId(strPath);
    if (pathID < 0)
      return false; // path (and thus files) aren't in the database
  }

  try
  {
    // error!
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql =
      "SELECT"
      "  files.strFilename, files.playCount,"
      "  bookmark.timeInSeconds, bookmark.totalTimeInSeconds "
      "FROM files"
      "  LEFT JOIN bookmark ON"
      "    files.idFile = bookmark.idFile AND bookmark.type = %i ";

    if (URIUtils::IsPlugin(strPath))
    {
      for (auto& item : items)
      {
        if (!item || item->m_bIsFolder || !item->GetProperty("IsPlayable").asBoolean())
          continue;

        std::string path, filename;
        SplitPath(item->GetPath(), path, filename);
        m_pDS->query(PrepareSQL(sql +
          "INNER JOIN path ON files.idPath = path.idPath "
          "WHERE files.strFilename='%s' AND path.strPath='%s'",
          (int)CBookmark::RESUME, filename.c_str(), path.c_str()));

        if (!m_pDS->eof())
        {
          if (!item->GetVideoInfoTag()->IsPlayCountSet())
            item->GetVideoInfoTag()->SetPlayCount(m_pDS->fv(1).get_asInt());
          if (!item->GetVideoInfoTag()->GetResumePoint().IsSet())
            item->GetVideoInfoTag()->SetResumePoint(m_pDS->fv(2).get_asInt(), m_pDS->fv(3).get_asInt(), "");
        }
        m_pDS->close();
      }
    }
    else
    {
      //! @todo also test a single query for the above and below
      sql = PrepareSQL(sql + "WHERE files.idPath=%i", (int)CBookmark::RESUME, pathID);

      if (RunQuery(sql) <= 0)
        return false;

      items.SetFastLookup(true); // note: it's possibly quicker the other way around (map on db returned items)?
      while (!m_pDS->eof())
      {
        std::string path;
        ConstructPath(path, strPath, m_pDS->fv(0).get_asString());
        CFileItemPtr item = items.Get(path);
        if (item)
        {
          if (!items.IsPlugin() || !item->GetVideoInfoTag()->IsPlayCountSet())
            item->GetVideoInfoTag()->SetPlayCount(m_pDS->fv(1).get_asInt());

          if (!item->GetVideoInfoTag()->GetResumePoint().IsSet())
          {
            item->GetVideoInfoTag()->SetResumePoint(m_pDS->fv(2).get_asInt(), m_pDS->fv(3).get_asInt(), "");
          }
        }
        m_pDS->next();
      }
    }

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

int CVideoDatabase::GetPlayCount(int iFileId)
{
  if (iFileId < 0)
    return 0;  // not in db, so not watched

  try
  {
    // error!
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string strSQL = PrepareSQL("select playCount from files WHERE idFile=%i", iFileId);
    int count = 0;
    if (m_pDS->query(strSQL))
    {
      // there should only ever be one row returned
      if (m_pDS->num_rows() == 1)
        count = m_pDS->fv(0).get_asInt();
      m_pDS->close();
    }
    return count;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return -1;
}

int CVideoDatabase::GetPlayCount(const std::string& strFilenameAndPath)
{
  return GetPlayCount(GetFileId(strFilenameAndPath));
}

int CVideoDatabase::GetPlayCount(const CFileItem &item)
{
  return GetPlayCount(GetFileId(item));
}

void CVideoDatabase::SetPlayCount(const CFileItem &item, int count, const CDateTime &date)
{
  int id;
  if (item.HasProperty("original_listitem_url") &&
      URIUtils::IsPlugin(item.GetProperty("original_listitem_url").asString()))
  {
    CFileItem item2(item);
    item2.SetPath(item.GetProperty("original_listitem_url").asString());
    id = AddFile(item2);
  }
  else
    id = AddFile(item);
  if (id < 0)
    return;

  // and mark as watched
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    std::string strSQL;
    if (count)
    {
      if (!date.IsValid())
        strSQL = PrepareSQL("update files set playCount=%i,lastPlayed='%s' where idFile=%i", count, CDateTime::GetCurrentDateTime().GetAsDBDateTime().c_str(), id);
      else
        strSQL = PrepareSQL("update files set playCount=%i,lastPlayed='%s' where idFile=%i", count, date.GetAsDBDateTime().c_str(), id);
    }
    else
    {
      if (!date.IsValid())
        strSQL = PrepareSQL("update files set playCount=NULL,lastPlayed=NULL where idFile=%i", id);
      else
        strSQL = PrepareSQL("update files set playCount=NULL,lastPlayed='%s' where idFile=%i", date.GetAsDBDateTime().c_str(), id);
    }

    m_pDS->exec(strSQL);

    // We only need to announce changes to video items in the library
    if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->m_iDbId > 0)
    {
      CVariant data;
      if (g_application.IsVideoScanning())
        data["transaction"] = true;
      // Only provide the "playcount" value if it has actually changed
      if (item.GetVideoInfoTag()->GetPlayCount() != count)
        data["playcount"] = count;
      CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::VideoLibrary, "xbmc", "OnUpdate", CFileItemPtr(new CFileItem(item)), data);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

void CVideoDatabase::IncrementPlayCount(const CFileItem &item)
{
  SetPlayCount(item, GetPlayCount(item) + 1);
}

void CVideoDatabase::UpdateLastPlayed(const CFileItem &item)
{
  SetPlayCount(item, GetPlayCount(item), CDateTime::GetCurrentDateTime());
}
