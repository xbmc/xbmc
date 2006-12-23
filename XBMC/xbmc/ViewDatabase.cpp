#include "stdafx.h"
#include "viewdatabase.h"
#include "util.h"

#define VIEW_DATABASE_VERSION 3

//********************************************************************************************************************************
CViewDatabase::CViewDatabase(void)
{
  m_preV2version = 0;
  m_version = VIEW_DATABASE_VERSION;
  m_strDatabaseFile = VIEW_DATABASE_NAME;
}

//********************************************************************************************************************************
CViewDatabase::~CViewDatabase(void)
{

}

//********************************************************************************************************************************
bool CViewDatabase::CreateTables()
{
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create view table");
    m_pDS->exec("CREATE TABLE view ( idView integer primary key, window integer, path text, viewMode integer, sortMethod integer, sortOrder integer)\n");
    CLog::Log(LOGINFO, "create view index");
    m_pDS->exec("CREATE INDEX idxViews ON view(path)");
    CLog::Log(LOGINFO, "create view - window index");
    m_pDS->exec("CREATE INDEX idxViewsWindow ON view(window)");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" unable to create tables:%i", GetLastError());
    return false;
  }

  return true;
}

bool CViewDatabase::UpdateOldVersion(int version)
{
  return true;
}

bool CViewDatabase::GetViewState(const CStdString &path, int window, CViewState &state)
{
  DWORD titleID = 0;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString path1(path);
    CUtil::AddSlashAtEnd(path1);
    if (path1.IsEmpty()) path1 = "root://";

    CStdString sql = FormatSQL("select * from view where window = %i and path like '%s'", window, path1.c_str());
    m_pDS->query(sql.c_str());

    if (!m_pDS->eof())
    { // have some information
      state.m_viewMode = m_pDS->fv("viewMode").get_asInteger();
      state.m_sortMethod = (SORT_METHOD)m_pDS->fv("sortMethod").get_asInteger();
      state.m_sortOrder = (SORT_ORDER)m_pDS->fv("sortOrder").get_asInteger();
      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__"failed on path '%s'", path.c_str());
  }
  return false;
}

bool CViewDatabase::SetViewState(const CStdString &path, int window, const CViewState &state)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString path1(path);
    CUtil::AddSlashAtEnd(path1);
    if (path1.IsEmpty()) path1 = "root://";

    CStdString sql = FormatSQL("select idView from view where window = %i and path like '%s'", window, path1.c_str());
    m_pDS->query(sql.c_str());
    if (!m_pDS->eof())
    { // update the view
      long idView = m_pDS->fv("idView").get_asLong();
      m_pDS->close();
      sql = FormatSQL("update view set viewMode=%i,sortMethod=%i,sortOrder=%i where idView=%i", state.m_viewMode, (int)state.m_sortMethod, (int)state.m_sortOrder, idView);
      m_pDS->exec(sql.c_str());
    }
    else
    { // add the view
      m_pDS->close();
      sql = FormatSQL("insert into view (idView, path, window, viewMode, sortMethod, sortOrder) values(NULL, '%s', %i, %i, %i, %i)", path1.c_str(), window, state.m_viewMode, (int)state.m_sortMethod, (int)state.m_sortOrder);
      m_pDS->exec(sql.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, __FUNCTION__" failed on path '%s'", path.c_str());
  }
  return true;
}

