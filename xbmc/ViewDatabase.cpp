/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ViewDatabase.h"
#include "utils/URIUtils.h"
#include "settings/Settings.h"
#include "ViewState.h"
#include "utils/log.h"
#ifdef _LINUX
#include "linux/ConvUtils.h" // GetLastError()
#endif
#include "dbwrappers/dataset.h"


//********************************************************************************************************************************
CViewDatabase::CViewDatabase(void)
{
}

//********************************************************************************************************************************
CViewDatabase::~CViewDatabase(void)
{

}

//********************************************************************************************************************************
bool CViewDatabase::Open()
{
  return CDatabase::Open();
}

bool CViewDatabase::CreateTables()
{
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create view table");
    m_pDS->exec("CREATE TABLE view ( idView integer primary key, window integer, path text, viewMode integer, sortMethod integer, sortOrder integer, skin text)\n");
    CLog::Log(LOGINFO, "create view index");
    m_pDS->exec("CREATE INDEX idxViews ON view(path)");
    CLog::Log(LOGINFO, "create view - window index");
    m_pDS->exec("CREATE INDEX idxViewsWindow ON view(window)");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to create tables:%u",
              __FUNCTION__, GetLastError());
    return false;
  }

  return true;
}

bool CViewDatabase::UpdateOldVersion(int version)
{
  if (version < 4)
    m_pDS->exec("alter table view add skin text");
  return true;
}

bool CViewDatabase::GetViewState(const CStdString &path, int window, CViewState &state, const CStdString &skin)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString path1(path);
    URIUtils::AddSlashAtEnd(path1);
    if (path1.IsEmpty()) path1 = "root://";

    CStdString sql;
    if (skin.IsEmpty())
      sql = PrepareSQL("select * from view where window = %i and path='%s'", window, path1.c_str());
    else
      sql = PrepareSQL("select * from view where window = %i and path='%s' and skin='%s'", window, path1.c_str(), skin.c_str());
    m_pDS->query(sql.c_str());

    if (!m_pDS->eof())
    { // have some information
      state.m_viewMode = m_pDS->fv("viewMode").get_asInt();
      state.m_sortMethod = (SORT_METHOD)m_pDS->fv("sortMethod").get_asInt();
      state.m_sortOrder = (SortOrder)m_pDS->fv("sortOrder").get_asInt();
      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s, failed on path '%s'", __FUNCTION__, path.c_str());
  }
  return false;
}

bool CViewDatabase::SetViewState(const CStdString &path, int window, const CViewState &state, const CStdString &skin)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString path1(path);
    URIUtils::AddSlashAtEnd(path1);
    if (path1.IsEmpty()) path1 = "root://";

    CStdString sql = PrepareSQL("select idView from view where window = %i and path='%s' and skin='%s'", window, path1.c_str(), skin.c_str());
    m_pDS->query(sql.c_str());
    if (!m_pDS->eof())
    { // update the view
      int idView = m_pDS->fv("idView").get_asInt();
      m_pDS->close();
      sql = PrepareSQL("update view set viewMode=%i,sortMethod=%i,sortOrder=%i where idView=%i", state.m_viewMode, (int)state.m_sortMethod, (int)state.m_sortOrder, idView);
      m_pDS->exec(sql.c_str());
    }
    else
    { // add the view
      m_pDS->close();
      sql = PrepareSQL("insert into view (idView, path, window, viewMode, sortMethod, sortOrder, skin) values(NULL, '%s', %i, %i, %i, %i, '%s')", path1.c_str(), window, state.m_viewMode, (int)state.m_sortMethod, (int)state.m_sortOrder, skin.c_str());
      m_pDS->exec(sql.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on path '%s'", __FUNCTION__, path.c_str());
  }
  return true;
}

bool CViewDatabase::ClearViewStates(int windowID)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString sql = PrepareSQL("delete from view where window = %i", windowID);
    m_pDS->exec(sql.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed on window '%i'", __FUNCTION__, windowID);
  }
  return true;
}
