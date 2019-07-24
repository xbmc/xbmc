/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabase.h"

#include <algorithm>
#include <string>
#include <vector>

#include "video/VideoInfoTag.h"
#include "dbwrappers/dataset.h"
#include "filesystem/MultiPathDirectory.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "video/VideoInfoScanner.h"

using namespace ADDON;
using namespace dbiplus;
using namespace VIDEO;
using namespace XFILE;

int CVideoDatabase::GetPathId(const std::string& strPath)
{
  std::string strSQL;
  try
  {
    int idPath=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string strPath1(strPath);
    if (URIUtils::IsStack(strPath) || StringUtils::StartsWithNoCase(strPath, "rar://") || StringUtils::StartsWithNoCase(strPath, "zip://"))
      URIUtils::GetParentPath(strPath,strPath1);

    URIUtils::AddSlashAtEnd(strPath1);

    strSQL=PrepareSQL("select idPath from path where strPath='%s'",strPath1.c_str());
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
      idPath = m_pDS->fv("path.idPath").get_asInt();

    m_pDS->close();
    return idPath;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to getpath (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

bool CVideoDatabase::GetPaths(std::set<std::string> &paths)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    paths.clear();

    // grab all paths with movie content set
    if (!m_pDS->query("select strPath,noUpdate from path"
                      " where (strContent = 'movies' or strContent = 'musicvideos')"
                      " and strPath NOT like 'multipath://%%'"
                      " order by strPath"))
      return false;

    while (!m_pDS->eof())
    {
      if (!m_pDS->fv("noUpdate").get_asBool())
        paths.insert(m_pDS->fv("strPath").get_asString());
      m_pDS->next();
    }
    m_pDS->close();

    // then grab all tvshow paths
    if (!m_pDS->query("select strPath,noUpdate from path"
                      " where ( strContent = 'tvshows'"
                      "       or idPath in (select idPath from tvshowlinkpath))"
                      " and strPath NOT like 'multipath://%%'"
                      " order by strPath"))
      return false;

    while (!m_pDS->eof())
    {
      if (!m_pDS->fv("noUpdate").get_asBool())
        paths.insert(m_pDS->fv("strPath").get_asString());
      m_pDS->next();
    }
    m_pDS->close();

    // finally grab all other paths holding a movie which is not a stack or a rar archive
    // - this isnt perfect but it should do fine in most situations.
    // reason we need it to hold a movie is stacks from different directories (cdx folders for instance)
    // not making mistakes must take priority
    if (!m_pDS->query("select strPath,noUpdate from path"
                       " where idPath in (select idPath from files join movie on movie.idFile=files.idFile)"
                       " and idPath NOT in (select idPath from tvshowlinkpath)"
                       " and idPath NOT in (select idPath from files where strFileName like 'video_ts.ifo')" // dvd folders get stacked to a single item in parent folder
                       " and idPath NOT in (select idPath from files where strFileName like 'index.bdmv')" // bluray folders get stacked to a single item in parent folder
                       " and strPath NOT like 'multipath://%%'"
                       " and strContent NOT in ('movies', 'tvshows', 'None')" // these have been added above
                       " order by strPath"))

      return false;
    while (!m_pDS->eof())
    {
      if (!m_pDS->fv("noUpdate").get_asBool())
        paths.insert(m_pDS->fv("strPath").get_asString());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetSubPaths(const std::string &basepath, std::vector<std::pair<int, std::string>>& subpaths)
{
  std::string sql;
  try
  {
    if (!m_pDB.get() || !m_pDS.get())
      return false;

    std::string path(basepath);
    URIUtils::AddSlashAtEnd(path);
    sql = PrepareSQL("SELECT idPath,strPath FROM path WHERE SUBSTR(strPath,1,%i)='%s'"
                     " AND idPath NOT IN (SELECT idPath FROM files WHERE strFileName LIKE 'video_ts.ifo')"
                     " AND idPath NOT IN (SELECT idPath FROM files WHERE strFileName LIKE 'index.bdmv')"
                     , StringUtils::utf8_strlen(path.c_str()), path.c_str());

    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      subpaths.emplace_back(m_pDS->fv(0).get_asInt(), m_pDS->fv(1).get_asString());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s error during query: %s",__FUNCTION__, sql.c_str());
  }
  return false;
}

int CVideoDatabase::AddPath(const std::string& strPath, const std::string &parentPath /*= "" */, const CDateTime& dateAdded /* = CDateTime() */)
{
  std::string strSQL;
  try
  {
    int idPath = GetPathId(strPath);
    if (idPath >= 0)
      return idPath; // already have the path

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string strPath1(strPath);
    if (URIUtils::IsStack(strPath) || StringUtils::StartsWithNoCase(strPath, "rar://") || StringUtils::StartsWithNoCase(strPath, "zip://"))
      URIUtils::GetParentPath(strPath,strPath1);

    URIUtils::AddSlashAtEnd(strPath1);

    int idParentPath = GetPathId(parentPath.empty() ? URIUtils::GetParentPath(strPath1) : parentPath);

    // add the path
    if (idParentPath < 0)
    {
      if (dateAdded.IsValid())
        strSQL=PrepareSQL("insert into path (idPath, strPath, dateAdded) values (NULL, '%s', '%s')", strPath1.c_str(), dateAdded.GetAsDBDateTime().c_str());
      else
        strSQL=PrepareSQL("insert into path (idPath, strPath) values (NULL, '%s')", strPath1.c_str());
    }
    else
    {
      if (dateAdded.IsValid())
        strSQL = PrepareSQL("insert into path (idPath, strPath, dateAdded, idParentPath) values (NULL, '%s', '%s', %i)", strPath1.c_str(), dateAdded.GetAsDBDateTime().c_str(), idParentPath);
      else
        strSQL=PrepareSQL("insert into path (idPath, strPath, idParentPath) values (NULL, '%s', %i)", strPath1.c_str(), idParentPath);
    }
    m_pDS->exec(strSQL);
    idPath = (int)m_pDS->lastinsertid();
    return idPath;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to addpath (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

bool CVideoDatabase::GetPathHash(const std::string &path, std::string &hash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL=PrepareSQL("select strHash from path where strPath='%s'", path.c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() == 0)
      return false;
    hash = m_pDS->fv("strHash").get_asString();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, path.c_str());
  }

  return false;
}

bool CVideoDatabase::GetSourcePath(const std::string &path, std::string &sourcePath)
{
  SScanSettings dummy;
  return GetSourcePath(path, sourcePath, dummy);
}

bool CVideoDatabase::GetSourcePath(const std::string &path, std::string &sourcePath, SScanSettings& settings)
{
  try
  {
    if (path.empty() ||
        m_pDB.get() == NULL || m_pDS.get() == NULL)
      return false;

    std::string strPath2;

    if (URIUtils::IsMultiPath(path))
      strPath2 = CMultiPathDirectory::GetFirstPath(path);
    else
      strPath2 = path;

    std::string strPath1 = URIUtils::GetDirectory(strPath2);
    int idPath = GetPathId(strPath1);

    if (idPath > -1)
    {
      // check if the given path already is a source itself
      std::string strSQL = PrepareSQL("SELECT path.useFolderNames, path.scanRecursive, path.noUpdate, path.exclude FROM path WHERE "
                                        "path.idPath = %i AND "
                                        "path.strContent IS NOT NULL AND path.strContent != '' AND "
                                        "path.strScraper IS NOT NULL AND path.strScraper != ''", idPath);
      if (m_pDS->query(strSQL) && !m_pDS->eof())
      {
        settings.parent_name_root = settings.parent_name = m_pDS->fv(0).get_asBool();
        settings.recurse = m_pDS->fv(1).get_asInt();
        settings.noupdate = m_pDS->fv(2).get_asBool();
        settings.exclude = m_pDS->fv(3).get_asBool();

        m_pDS->close();
        sourcePath = path;
        return true;
      }
    }

    // look for parent paths until there is one which is a source
    std::string strParent;
    bool found = false;
    while (URIUtils::GetParentPath(strPath1, strParent))
    {
      std::string strSQL = PrepareSQL("SELECT path.strContent, path.strScraper, path.scanRecursive, path.useFolderNames, path.noUpdate, path.exclude FROM path WHERE strPath = '%s'", strParent.c_str());
      if (m_pDS->query(strSQL) && !m_pDS->eof())
      {
        std::string strContent = m_pDS->fv(0).get_asString();
        std::string strScraper = m_pDS->fv(1).get_asString();
        if (!strContent.empty() && !strScraper.empty())
        {
          settings.parent_name_root = settings.parent_name = m_pDS->fv(2).get_asBool();
          settings.recurse = m_pDS->fv(3).get_asInt();
          settings.noupdate = m_pDS->fv(4).get_asBool();
          settings.exclude = m_pDS->fv(5).get_asBool();
          found = true;
          break;
        }
      }

      strPath1 = strParent;
    }
    m_pDS->close();

    if (found)
    {
      sourcePath = strParent;
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::SetPathHash(const std::string &path, const std::string &hash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int idPath = AddPath(path);
    if (idPath < 0) return false;

    std::string strSQL=PrepareSQL("update path set strHash='%s' where idPath=%ld", hash.c_str(), idPath);
    m_pDS->exec(strSQL);

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s, %s) failed", __FUNCTION__, path.c_str(), hash.c_str());
  }

  return false;
}

void CVideoDatabase::ConstructPath(std::string& strDest, const std::string& strPath, const std::string& strFileName)
{
  if (URIUtils::IsStack(strFileName) ||
      URIUtils::IsInArchive(strFileName) || URIUtils::IsPlugin(strPath))
    strDest = strFileName;
  else
    strDest = URIUtils::AddFileToFolder(strPath, strFileName);
}

void CVideoDatabase::SplitPath(const std::string& strFileNameAndPath, std::string& strPath, std::string& strFileName)
{
  if (URIUtils::IsStack(strFileNameAndPath) || StringUtils::StartsWithNoCase(strFileNameAndPath, "rar://") || StringUtils::StartsWithNoCase(strFileNameAndPath, "zip://"))
  {
    URIUtils::GetParentPath(strFileNameAndPath,strPath);
    strFileName = strFileNameAndPath;
  }
  else if (URIUtils::IsPlugin(strFileNameAndPath))
  {
    CURL url(strFileNameAndPath);
    strPath = url.GetOptions().empty() ? url.GetWithoutFilename() : url.GetWithoutOptions();
    strFileName = strFileNameAndPath;
  }
  else
  {
    URIUtils::Split(strFileNameAndPath, strPath, strFileName);
    // Keep protocol options as part of the path
    if (URIUtils::IsURL(strFileNameAndPath))
    {
      CURL url(strFileNameAndPath);
      if (!url.GetProtocolOptions().empty())
        strPath += "|" + url.GetProtocolOptions();
    }
  }
}

void CVideoDatabase::InvalidatePathHash(const std::string& strPath)
{
  SScanSettings settings;
  bool foundDirectly;
  ScraperPtr info = GetScraperForPath(strPath,settings,foundDirectly);
  SetPathHash(strPath,"");
  if (!info)
    return;
  if (info->Content() == CONTENT_TVSHOWS || (info->Content() == CONTENT_MOVIES && !foundDirectly)) // if we scan by folder name we need to invalidate parent as well
  {
    if (info->Content() == CONTENT_TVSHOWS || settings.parent_name_root)
    {
      std::string strParent;
      if (URIUtils::GetParentPath(strPath, strParent) && (!URIUtils::IsPlugin(strPath) || !CURL(strParent).GetHostName().empty()))
        SetPathHash(strParent, "");
    }
  }
}

bool CVideoDatabase::GetItemsForPath(const std::string &content, const std::string &strPath, CFileItemList &items)
{
  std::string path(strPath);

  if(URIUtils::IsMultiPath(path))
  {
    std::vector<std::string> paths;
    CMultiPathDirectory::GetPaths(path, paths);

    for(unsigned i=0;i<paths.size();i++)
      GetItemsForPath(content, paths[i], items);

    return items.Size() > 0;
  }

  int pathID = GetPathId(path);
  if (pathID < 0)
    return false;

  if (content == "movies")
  {
    Filter filter(PrepareSQL("c%02d=%d", VIDEODB_ID_PARENTPATHID, pathID));
    GetMoviesByWhere("videodb://movies/titles/", filter, items);
  }
  else if (content == "episodes")
  {
    Filter filter(PrepareSQL("c%02d=%d", VIDEODB_ID_EPISODE_PARENTPATHID, pathID));
    GetEpisodesByWhere("videodb://tvshows/titles/", filter, items);
  }
  else if (content == "tvshows")
  {
    Filter filter(PrepareSQL("idParentPath=%d", pathID));
    GetTvShowsByWhere("videodb://tvshows/titles/", filter, items);
  }
  else if (content == "musicvideos")
  {
    Filter filter(PrepareSQL("c%02d=%d", VIDEODB_ID_MUSICVIDEO_PARENTPATHID, pathID));
    GetMusicVideosByWhere("videodb://musicvideos/titles/", filter, items);
  }
  for (int i = 0; i < items.Size(); i++)
    items[i]->SetPath(items[i]->GetVideoInfoTag()->m_basePath);
  return items.Size() > 0;
}
