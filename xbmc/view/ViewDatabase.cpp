/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ViewDatabase.h"

#include "DatabaseTypes.h"
#include "SortFileItem.h"
#include "dbwrappers/dataset.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "view/ViewState.h"

#include <utility>

#ifdef TARGET_POSIX
#include "platform/posix/ConvUtils.h"
#endif

CViewDatabase::CViewDatabase() : CDatabase(KODI::DATABASE::TYPE_VIEWMODES)
{
}

CViewDatabase::~CViewDatabase(void) = default;

bool CViewDatabase::Open()
{
  return CDatabase::Open();
}

void CViewDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create view table");
  m_pDS->exec("CREATE TABLE view ("
              "idView integer primary key,"
              "window integer,"
              "path text,"
              "viewMode integer,"
              "sortMethod integer,"
              "sortOrder integer,"
              "sortAttributes integer,"
              "skin text)");
}

void CViewDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "{} - creating indices", __FUNCTION__);
  m_pDS->exec("CREATE INDEX idxViews ON view(path)");
  m_pDS->exec("CREATE INDEX idxViewsWindow ON view(window)");
}

void CViewDatabase::UpdateTables(int version)
{
  if (version < 4)
    m_pDS->exec("alter table view add skin text");
  if (version < 5)
  {
    // translate legacy videodb:// and musicdb:// paths
    std::vector< std::pair<int, std::string> > paths;
    if (m_pDS->query("SELECT idView, path FROM view"))
    {
      while (!m_pDS->eof())
      {
        std::string originalPath = m_pDS->fv(1).get_asString();
        std::string path = originalPath;
        if (StringUtils::StartsWithNoCase(path, "musicdb://"))
          path = CLegacyPathTranslation::TranslateMusicDbPath(path);
        else if (StringUtils::StartsWithNoCase(path, "videodb://"))
          path = CLegacyPathTranslation::TranslateVideoDbPath(path);

        if (!StringUtils::EqualsNoCase(path, originalPath))
          paths.emplace_back(m_pDS->fv(0).get_asInt(), path);
        m_pDS->next();
      }
      m_pDS->close();

      for (std::vector< std::pair<int, std::string> >::const_iterator it = paths.begin(); it != paths.end(); ++it)
        m_pDS->exec(PrepareSQL("UPDATE view SET path='%s' WHERE idView=%d", it->second.c_str(), it->first));
    }
  }
  if (version < 6)
  {
    // convert the "path" table
    m_pDS->exec("ALTER TABLE view RENAME TO tmp_view");

    m_pDS->exec("CREATE TABLE view ("
                "idView integer primary key,"
                "window integer,"
                "path text,"
                "viewMode integer,"
                "sortMethod integer,"
                "sortOrder integer,"
                "sortAttributes integer,"
                "skin text)\n");

    m_pDS->query("SELECT * FROM tmp_view");
    while (!m_pDS->eof())
    {
      SortDescription sorting =
          SortUtils::TranslateOldSortMethod(static_cast<SortMethod>(m_pDS->fv(4).get_asInt()));

      std::string sql = PrepareSQL("INSERT INTO view (idView, window, path, viewMode, sortMethod, sortOrder, sortAttributes, skin) VALUES (%i, %i, '%s', %i, %i, %i, %i, '%s')",
        m_pDS->fv(0).get_asInt(), m_pDS->fv(1).get_asInt(), m_pDS->fv(2).get_asString().c_str(), m_pDS->fv(3).get_asInt(),
        (int)sorting.sortBy, m_pDS->fv(5).get_asInt(), (int)sorting.sortAttributes, m_pDS->fv(6).get_asString().c_str());
      m_pDS2->exec(sql);

      m_pDS->next();
    }
    m_pDS->exec("DROP TABLE tmp_view");
  }
}

bool CViewDatabase::GetViewState(const std::string &path, int window, CViewState &state, const std::string &skin)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    std::string path1(path);
    URIUtils::AddSlashAtEnd(path1);
    if (path1.empty()) path1 = "root://";

    std::string sql;
    if (skin.empty())
      sql = PrepareSQL("select * from view where window = %i and path='%s'", window, path1.c_str());
    else
      sql = PrepareSQL("select * from view where window = %i and path='%s' and skin='%s'", window, path1.c_str(), skin.c_str());
    m_pDS->query(sql);

    if (!m_pDS->eof())
    { // have some information
      state.m_viewMode = m_pDS->fv("viewMode").get_asInt();
      state.m_sortDescription.sortBy = (SortBy)m_pDS->fv("sortMethod").get_asInt();
      state.m_sortDescription.sortOrder = (SortOrder)m_pDS->fv("sortOrder").get_asInt();
      state.m_sortDescription.sortAttributes = (SortAttribute)m_pDS->fv("sortAttributes").get_asInt();
      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{}, failed on path '{}'", __FUNCTION__, path);
  }
  return false;
}

bool CViewDatabase::SetViewState(const std::string &path, int window, const CViewState &state, const std::string &skin)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    std::string path1(path);
    URIUtils::AddSlashAtEnd(path1);
    if (path1.empty()) path1 = "root://";

    std::string sql = PrepareSQL("select idView from view where window = %i and path='%s' and skin='%s'", window, path1.c_str(), skin.c_str());
    m_pDS->query(sql);
    if (!m_pDS->eof())
    { // update the view
      int idView = m_pDS->fv("idView").get_asInt();
      m_pDS->close();
      sql = PrepareSQL("update view set viewMode=%i,sortMethod=%i,sortOrder=%i,sortAttributes=%i where idView=%i",
        state.m_viewMode, (int)state.m_sortDescription.sortBy, (int)state.m_sortDescription.sortOrder, (int)state.m_sortDescription.sortAttributes, idView);
      m_pDS->exec(sql);
    }
    else
    { // add the view
      m_pDS->close();
      sql = PrepareSQL("insert into view (idView, path, window, viewMode, sortMethod, sortOrder, sortAttributes, skin) values(NULL, '%s', %i, %i, %i, %i, %i, '%s')",
        path1.c_str(), window, state.m_viewMode, (int)state.m_sortDescription.sortBy, (int)state.m_sortDescription.sortOrder, (int)state.m_sortDescription.sortAttributes, skin.c_str());
      m_pDS->exec(sql);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on path '{}'", __FUNCTION__, path);
  }
  return true;
}

bool CViewDatabase::ClearViewStates(int windowID)
{
  try
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;

    std::string sql = PrepareSQL("delete from view where window = %i", windowID);
    m_pDS->exec(sql);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "{} failed on window '{}'", __FUNCTION__, windowID);
  }
  return true;
}
