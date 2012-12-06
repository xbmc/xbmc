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

#include "Database.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/Crc32.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/File.h"
#include "utils/AutoPtrHandle.h"
#include "utils/log.h"
#include "utils/SortUtils.h"
#include "utils/URIUtils.h"
#include "sqlitedataset.h"
#include "DatabaseManager.h"
#include "DbUrl.h"

#ifdef HAS_MYSQL
#include "mysqldataset.h"
#endif

using namespace AUTOPTR;
using namespace dbiplus;

#define MAX_COMPRESS_COUNT 20

void CDatabase::Filter::AppendField(const std::string &strField)
{
  if (strField.empty())
    return;

  if (fields.empty() || fields == "*")
    fields = strField;
  else
    fields += ", " + strField;
}

void CDatabase::Filter::AppendJoin(const std::string &strJoin)
{
  if (strJoin.empty())
    return;

  if (join.empty())
    join = strJoin;
  else
    join += " " + strJoin;
}

void CDatabase::Filter::AppendWhere(const std::string &strWhere, bool combineWithAnd /* = true */)
{
  if (strWhere.empty())
    return;

  if (where.empty())
    where = strWhere;
  else
  {
    where = "(" + where + ") ";
    where += combineWithAnd ? "AND" : "OR";
    where += " (" + strWhere + ")";
  }
}

void CDatabase::Filter::AppendOrder(const std::string &strOrder)
{
  if (strOrder.empty())
    return;

  if (order.empty())
    order = strOrder;
  else
    order += ", " + strOrder;
}

void CDatabase::Filter::AppendGroup(const std::string &strGroup)
{
  if (strGroup.empty())
    return;

  if (group.empty())
    group = strGroup;
  else
    group += ", " + strGroup;
}

CDatabase::CDatabase(void)
{
  m_openCount = 0;
  m_sqlite = true;
  m_bMultiWrite = false;
}

CDatabase::~CDatabase(void)
{
  Close();
}

void CDatabase::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
  strFileName = "";
  strPath = "";
  int i = strFileNameAndPath.size() - 1;
  while (i > 0)
  {
    char ch = strFileNameAndPath[i];
    if (ch == ':' || ch == '/' || ch == '\\') break;
    else i--;
  }
  strPath = strFileNameAndPath.Left(i);
  strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i);
}

uint32_t CDatabase::ComputeCRC(const CStdString &text)
{
  Crc32 crc;
  crc.ComputeFromLowerCase(text);
  return crc;
}


CStdString CDatabase::FormatSQL(CStdString strStmt, ...)
{
  //  %q is the sqlite format string for %s.
  //  Any bad character, like "'", will be replaced with a proper one
  strStmt.Replace("%s", "%q");
  //  the %I64 enhancement is not supported by sqlite3_vmprintf
  //  must be %ll instead
  strStmt.Replace("%I64", "%ll");

  va_list args;
  va_start(args, strStmt);
  char *szSql = sqlite3_vmprintf(strStmt.c_str(), args);
  va_end(args);

  CStdString strResult;
  if (szSql) {
    strResult = szSql;
    sqlite3_free(szSql);
  }

  return strResult;
}

CStdString CDatabase::PrepareSQL(CStdString strStmt, ...) const
{
  CStdString strResult = "";

  if (NULL != m_pDB.get())
  {
    va_list args;
    va_start(args, strStmt);
    strResult = m_pDB->vprepare(strStmt.c_str(), args);
    va_end(args);
  }

  return strResult;
}

std::string CDatabase::GetSingleValue(const std::string &query, std::auto_ptr<Dataset> &ds)
{
  std::string ret;
  try
  {
    if (!m_pDB.get() || !ds.get())
      return ret;

    if (ds->query(query.c_str()) && ds->num_rows() > 0)
      ret = ds->fv(0).get_asString();

    ds->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - failed on query '%s'", __FUNCTION__, query.c_str());
  }
  return ret;
}

CStdString CDatabase::GetSingleValue(const CStdString &strTable, const CStdString &strColumn, const CStdString &strWhereClause /* = CStdString() */, const CStdString &strOrderBy /* = CStdString() */)
{
  CStdString query = PrepareSQL("SELECT %s FROM %s", strColumn.c_str(), strTable.c_str());
  if (!strWhereClause.empty())
    query += " WHERE " + strWhereClause;
  if (!strOrderBy.empty())
    query += " ORDER BY " + strOrderBy;
  query += " LIMIT 1";
  return GetSingleValue(query, m_pDS);
}

CStdString CDatabase::GetSingleValue(const CStdString &query)
{
  return GetSingleValue(query, m_pDS);
}

bool CDatabase::DeleteValues(const CStdString &strTable, const CStdString &strWhereClause /* = CStdString() */)
{
  bool bReturn = true;

  CStdString strQueryBase = "DELETE FROM %s";
  if (!strWhereClause.IsEmpty())
    strQueryBase.AppendFormat(" WHERE %s", strWhereClause.c_str());

  CStdString strQuery = FormatSQL(strQueryBase, strTable.c_str());

  bReturn = ExecuteQuery(strQuery);

  return bReturn;
}

bool CDatabase::ExecuteQuery(const CStdString &strQuery)
{
  bool bReturn = false;

  try
  {
    if (NULL == m_pDB.get()) return bReturn;
    if (NULL == m_pDS.get()) return bReturn;
    m_pDS->exec(strQuery.c_str());
    bReturn = true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - failed to execute query '%s'",
        __FUNCTION__, strQuery.c_str());
  }

  return bReturn;
}

bool CDatabase::ResultQuery(const CStdString &strQuery)
{
  bool bReturn = false;

  try
  {
    if (NULL == m_pDB.get()) return bReturn;
    if (NULL == m_pDS.get()) return bReturn;

    CStdString strPreparedQuery = PrepareSQL(strQuery.c_str());

    bReturn = m_pDS->query(strPreparedQuery.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - failed to execute query '%s'",
        __FUNCTION__, strQuery.c_str());
  }

  return bReturn;
}

bool CDatabase::QueueInsertQuery(const CStdString &strQuery)
{
  if (strQuery.IsEmpty())
    return false;

  if (!m_bMultiWrite)
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    m_bMultiWrite = true;
    m_pDS2->insert();
  }

  m_pDS2->add_insert_sql(strQuery);

  return true;
}

bool CDatabase::CommitInsertQueries()
{
  bool bReturn = true;

  if (m_bMultiWrite)
  {
    try
    {
      m_bMultiWrite = false;
      m_pDS2->post();
      m_pDS2->clear_insert_sql();
    }
    catch(...)
    {
      bReturn = false;
      CLog::Log(LOGERROR, "%s - failed to execute queries",
          __FUNCTION__);
    }
  }

  return bReturn;
}

bool CDatabase::Open()
{
  DatabaseSettings db_fallback;
  return Open(db_fallback);
}

bool CDatabase::Open(const DatabaseSettings &settings)
{
  if (IsOpen())
  {
    m_openCount++;
    return true;
  }

  // check our database manager to see if this database can be opened
  if (!CDatabaseManager::Get().CanOpen(GetBaseDBName()))
    return false;

  DatabaseSettings dbSettings = settings;
  InitSettings(dbSettings);

  CStdString dbName = dbSettings.name;
  dbName.AppendFormat("%d", GetMinVersion());
  return Connect(dbName, dbSettings, false);
}

void CDatabase::InitSettings(DatabaseSettings &dbSettings)
{
  m_sqlite = true;

#ifdef HAS_MYSQL
  if ( dbSettings.type.Equals("mysql") )
  {
    // check we have all information before we cancel the fallback
    if ( ! (dbSettings.host.IsEmpty() ||
            dbSettings.user.IsEmpty() || dbSettings.pass.IsEmpty()) )
      m_sqlite = false;
    else
      CLog::Log(LOGINFO, "Essential mysql database information is missing. Require at least host, user and pass defined.");
  }
  else
#else
  if ( dbSettings.type.Equals("mysql") )
    CLog::Log(LOGERROR, "MySQL library requested but MySQL support is not compiled in. Falling back to sqlite3.");
#endif
  {
    dbSettings.type = "sqlite3";
    if (dbSettings.host.IsEmpty())
      dbSettings.host = CSpecialProtocol::TranslatePath(g_settings.GetDatabaseFolder());
  }

  // use separate, versioned database
  if (dbSettings.name.IsEmpty())
    dbSettings.name = GetBaseDBName();
}

bool CDatabase::Update(const DatabaseSettings &settings)
{
  DatabaseSettings dbSettings = settings;
  InitSettings(dbSettings);

  int version = GetMinVersion();
  CStdString latestDb = dbSettings.name;
  latestDb.AppendFormat("%d", version);

  while (version >= 0)
  {
    CStdString dbName = dbSettings.name;
    if (version)
      dbName.AppendFormat("%d", version);

    if (Connect(dbName, dbSettings, false))
    {
      // Database exists, take a copy for our current version (if needed) and reopen that one
      if (version < GetMinVersion())
      {
        CLog::Log(LOGNOTICE, "Old database found - updating from version %i to %i", version, GetMinVersion());

        bool copy_fail = false;

        try
        {
          m_pDB->copy(latestDb);
        }
        catch(...)
        {
          CLog::Log(LOGERROR, "Unable to copy old database %s to new version %s", dbName.c_str(), latestDb.c_str());
          copy_fail = true;
        }

        Close();

        if ( copy_fail )
          return false;

        if (!Connect(latestDb, dbSettings, false))
        {
          CLog::Log(LOGERROR, "Unable to open freshly copied database %s", latestDb.c_str());
          return false;
        }
      }

      // yay - we have a copy of our db, now do our worst with it
      if (UpdateVersion(latestDb))
        return true;

      // update failed - loop around and see if we have another one available
      Close();
    }

    // drop back to the previous version and try that
    version--;
  }
  // try creating a new one
  if (Connect(latestDb, dbSettings, true))
    return true;

  // failed to update or open the database
  Close();
  CLog::Log(LOGERROR, "Unable to create new database");
  return false;
}

bool CDatabase::Connect(const CStdString &dbName, const DatabaseSettings &dbSettings, bool create)
{
  // create the appropriate database structure
  if (dbSettings.type.Equals("sqlite3"))
  {
    m_pDB.reset( new SqliteDatabase() ) ;
  }
#ifdef HAS_MYSQL
  else if (dbSettings.type.Equals("mysql"))
  {
    m_pDB.reset( new MysqlDatabase() ) ;
  }
#endif
  else
  {
    CLog::Log(LOGERROR, "Unable to determine database type: %s", dbSettings.type.c_str());
    return false;
  }

  // host name is always required
  m_pDB->setHostName(dbSettings.host.c_str());

  if (!dbSettings.port.IsEmpty())
    m_pDB->setPort(dbSettings.port.c_str());

  if (!dbSettings.user.IsEmpty())
    m_pDB->setLogin(dbSettings.user.c_str());

  if (!dbSettings.pass.IsEmpty())
    m_pDB->setPasswd(dbSettings.pass.c_str());

  // database name is always required
  m_pDB->setDatabase(dbName.c_str());

  // create the datasets
  m_pDS.reset(m_pDB->CreateDataset());
  m_pDS2.reset(m_pDB->CreateDataset());

  if (m_pDB->connect(create) != DB_CONNECTION_OK)
    return false;

  try
  {
    // test if db already exists, if not we need to create the tables
    if (!m_pDB->exists() && create)
    {
      if (dbSettings.type.Equals("sqlite3"))
      {
        //  Modern file systems have a cluster/block size of 4k.
        //  To gain better performance when performing write
        //  operations to the database, set the page size of the
        //  database file to 4k.
        //  This needs to be done before any table is created.
        m_pDS->exec("PRAGMA page_size=4096\n");

        //  Also set the memory cache size to 16k
        m_pDS->exec("PRAGMA default_cache_size=4096\n");
      }
      CreateTables();
    }

    // sqlite3 post connection operations
    if (dbSettings.type.Equals("sqlite3"))
    {
      m_pDS->exec("PRAGMA cache_size=4096\n");
      m_pDS->exec("PRAGMA synchronous='NORMAL'\n");
      m_pDS->exec("PRAGMA count_changes='OFF'\n");
    }
  }
  catch (DbErrors &error)
  {
    CLog::Log(LOGERROR, "%s failed with '%s'", __FUNCTION__, error.getMsg());
    m_openCount = 1; // set to open so we can execute Close()
    Close();
    return false;
  }

  m_openCount = 1; // our database is open
  return true;
}

int CDatabase::GetDBVersion()
{
  m_pDS->query("SELECT idVersion FROM version\n");
  if (m_pDS->num_rows() > 0)
    return m_pDS->fv("idVersion").get_asInt();
  return 0;
}

bool CDatabase::UpdateVersion(const CStdString &dbName)
{
  int version = GetDBVersion();
  if (version < GetMinVersion())
  {
    CLog::Log(LOGNOTICE, "Attempting to update the database %s from version %i to %i", dbName.c_str(), version, GetMinVersion());
    bool success = false;
    BeginTransaction();
    try
    {
      success = UpdateOldVersion(version);
      if (success)
        success = UpdateVersionNumber();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "Exception updating database %s from version %i to %i", dbName.c_str(), version, GetMinVersion());
      success = false;
    }
    if (!success)
    {
      CLog::Log(LOGERROR, "Error updating database %s from version %i to %i", dbName.c_str(), version, GetMinVersion());
      RollbackTransaction();
      return false;
    }
    CommitTransaction();
    CLog::Log(LOGINFO, "Update to version %i successful", GetMinVersion());
  }
  else if (version > GetMinVersion())
  {
    CLog::Log(LOGERROR, "Can't open the database %s as it is a NEWER version than what we were expecting?", dbName.c_str());
    return false;
  }
  else 
    CLog::Log(LOGNOTICE, "Running database version %s", dbName.c_str());
  return true;
}

bool CDatabase::IsOpen()
{
  return m_openCount > 0;
}

void CDatabase::Close()
{
  if (m_openCount == 0)
    return;

  if (m_openCount > 1)
  {
    m_openCount--;
    return;
  }

  m_openCount = 0;

  if (NULL == m_pDB.get() ) return ;
  if (NULL != m_pDS.get()) m_pDS->close();
  m_pDB->disconnect();
  m_pDB.reset();
  m_pDS.reset();
  m_pDS2.reset();
}

bool CDatabase::Compress(bool bForce /* =true */)
{
  if (!m_sqlite)
    return true;

  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    if (!bForce)
    {
      m_pDS->query("select iCompressCount from version");
      if (!m_pDS->eof())
      {
        int iCount = m_pDS->fv(0).get_asInt();
        if (iCount > MAX_COMPRESS_COUNT)
          iCount = -1;
        m_pDS->close();
        CStdString strSQL=PrepareSQL("update version set iCompressCount=%i\n",++iCount);
        m_pDS->exec(strSQL.c_str());
        if (iCount != 0)
          return true;
      }
    }

    if (!m_pDS->exec("vacuum\n"))
      return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Compressing the database failed", __FUNCTION__);
    return false;
  }
  return true;
}

void CDatabase::Interupt()
{
  m_pDS->interrupt();
}

void CDatabase::BeginTransaction()
{
  try
  {
    if (NULL != m_pDB.get())
      m_pDB->start_transaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "database:begintransaction failed");
  }
}

bool CDatabase::CommitTransaction()
{
  try
  {
    if (NULL != m_pDB.get())
      m_pDB->commit_transaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "database:committransaction failed");
    return false;
  }
  return true;
}

void CDatabase::RollbackTransaction()
{
  try
  {
    if (NULL != m_pDB.get())
      m_pDB->rollback_transaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "database:rollbacktransaction failed");
  }
}

bool CDatabase::InTransaction()
{
  if (NULL != m_pDB.get()) return false;
  return m_pDB->in_transaction();
}

bool CDatabase::CreateTables()
{

    CLog::Log(LOGINFO, "creating version table");
    m_pDS->exec("CREATE TABLE version (idVersion integer, iCompressCount integer)\n");
    CStdString strSQL=PrepareSQL("INSERT INTO version (idVersion,iCompressCount) values(%i,0)\n", GetMinVersion());
    m_pDS->exec(strSQL.c_str());

    return true;
}

bool CDatabase::UpdateVersionNumber()
{
  CStdString strSQL=PrepareSQL("UPDATE version SET idVersion=%i\n", GetMinVersion());
  m_pDS->exec(strSQL.c_str());
  return true;
}

bool CDatabase::BuildSQL(const CStdString &strQuery, const Filter &filter, CStdString &strSQL)
{
  strSQL = strQuery;

  if (!filter.join.empty())
    strSQL += filter.join;
  if (!filter.where.empty())
    strSQL += " WHERE " + filter.where;
  if (!filter.group.empty())
    strSQL += " GROUP BY " + filter.group;
  if (!filter.order.empty())
    strSQL += " ORDER BY " + filter.order;
  if (!filter.limit.empty())
    strSQL += " LIMIT " + filter.limit;

  return true;
}

bool CDatabase::BuildSQL(const CStdString &strBaseDir, const CStdString &strQuery, Filter &filter, CStdString &strSQL, CDbUrl &dbUrl)
{
  SortDescription sorting;
  return BuildSQL(strBaseDir, strQuery, filter, strSQL, dbUrl, sorting);
}

bool CDatabase::BuildSQL(const CStdString &strBaseDir, const CStdString &strQuery, Filter &filter, CStdString &strSQL, CDbUrl &dbUrl, SortDescription &sorting /* = SortDescription() */)
{
  // parse the base path to get additional filters
  dbUrl.Reset();
  if (!dbUrl.FromString(strBaseDir) || !GetFilter(dbUrl, filter, sorting))
    return false;

  return BuildSQL(strQuery, filter, strSQL);
}
