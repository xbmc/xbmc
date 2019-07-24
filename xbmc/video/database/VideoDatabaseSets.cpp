/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabase.h"

#include <map>
#include <string>

#include "video/VideoInfoTag.h"
#include "dbwrappers/dataset.h"
#include "FileItem.h"
#include "utils/GroupUtils.h"
#include "utils/log.h"

using namespace dbiplus;

int CVideoDatabase::AddSet(const std::string& strSet, const std::string& strOverview /* = "" */)
{
  if (strSet.empty())
    return -1;

  try
  {
    if (m_pDB.get() == nullptr || m_pDS.get() == nullptr)
      return -1;

    std::string strSQL = PrepareSQL("SELECT idSet FROM sets WHERE strSet LIKE '%s'", strSet.c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      strSQL = PrepareSQL("INSERT INTO sets (idSet, strSet, strOverview) VALUES(NULL, '%s', '%s')", strSet.c_str(), strOverview.c_str());
      m_pDS->exec(strSQL);
      int id = static_cast<int>(m_pDS->lastinsertid());
      return id;
    }
    else
    {
      int id = m_pDS->fv("idSet").get_asInt();
      m_pDS->close();
      return id;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSet.c_str());
  }

  return -1;
}

bool CVideoDatabase::GetSetInfo(int idSet, CVideoInfoTag& details)
{
  try
  {
    if (idSet < 0)
      return false;

    Filter filter;
    filter.where = PrepareSQL("sets.idSet=%d", idSet);
    CFileItemList items;
    if (!GetSetsByWhere("videodb://movies/sets/", filter, items) ||
        items.Size() != 1 ||
        !items[0]->HasVideoInfoTag())
      return false;

    details = *(items[0]->GetVideoInfoTag());
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%d) failed", __FUNCTION__, idSet);
  }
  return false;
}

int CVideoDatabase::SetDetailsForMovieSet(const CVideoInfoTag& details, const std::map<std::string, std::string> &artwork, int idSet /* = -1 */)
{
  if (details.m_strTitle.empty())
    return -1;

  try
  {
    BeginTransaction();
    if (idSet < 0)
    {
      idSet = AddSet(details.m_strTitle, details.m_strPlot);
      if (idSet < 0)
      {
        RollbackTransaction();
        return -1;
      }
    }

    SetArtForItem(idSet, MediaTypeVideoCollection, artwork);

    // and insert the new row
    std::string sql = PrepareSQL("UPDATE sets SET strSet='%s', strOverview='%s' WHERE idSet=%i", details.m_strTitle.c_str(), details.m_strPlot.c_str(), idSet);
    m_pDS->exec(sql);
    CommitTransaction();

    return idSet;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idSet);
  }
  RollbackTransaction();
  return -1;
}

void CVideoDatabase::DeleteSet(int idSet)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    std::string strSQL;
    strSQL=PrepareSQL("delete from sets where idSet = %i", idSet);
    m_pDS->exec(strSQL);
    strSQL=PrepareSQL("update movie set idSet = null where idSet = %i", idSet);
    m_pDS->exec(strSQL);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idSet);
  }
}

void CVideoDatabase::ClearMovieSet(int idMovie)
{
  SetMovieSet(idMovie, -1);
}

void CVideoDatabase::SetMovieSet(int idMovie, int idSet)
{
  if (idSet >= 0)
    ExecuteQuery(PrepareSQL("update movie set idSet = %i where idMovie = %i", idSet, idMovie));
  else
    ExecuteQuery(PrepareSQL("update movie set idSet = null where idMovie = %i", idMovie));
}

bool CVideoDatabase::GetSetsNav(const std::string& strBaseDir, CFileItemList& items, int idContent /* = -1 */, const Filter &filter /* = Filter() */, bool ignoreSingleMovieSets /* = false */)
{
  if (idContent != VIDEODB_CONTENT_MOVIES)
    return false;

  return GetSetsByWhere(strBaseDir, filter, items, ignoreSingleMovieSets);
}

bool CVideoDatabase::GetSetsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, bool ignoreSingleMovieSets /* = false */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CVideoDbUrl videoUrl;
    if (!videoUrl.FromString(strBaseDir))
      return false;

    Filter setFilter = filter;
    setFilter.join += " JOIN sets ON movie_view.idSet = sets.idSet";
    if (!setFilter.order.empty())
      setFilter.order += ",";
    setFilter.order += "sets.idSet";

    if (!GetMoviesByWhere(strBaseDir, setFilter, items))
      return false;

    CFileItemList sets;
    GroupAttribute groupingAttributes = ignoreSingleMovieSets ? GroupAttributeIgnoreSingleItems : GroupAttributeNone;
    if (!GroupUtils::Group(GroupBySet, strBaseDir, items, sets, groupingAttributes))
      return false;

    items.ClearItems();
    items.Append(sets);

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

std::string CVideoDatabase::GetSetById(int id)
{
  return GetSingleValue("sets", "strSet", PrepareSQL("idSet=%i", id));
}

bool CVideoDatabase::HasSets() const
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    m_pDS->query("SELECT movie_view.idSet,COUNT(1) AS c FROM movie_view "
                 "JOIN sets ON sets.idSet = movie_view.idSet "
                 "GROUP BY movie_view.idSet HAVING c>1");

    bool bResult = (m_pDS->num_rows() > 0);
    m_pDS->close();
    return bResult;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}
