/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Database.h"

#include "DatabaseManager.h"
#include "DbUrl.h"
#include "ServiceBroker.h"
#include "filesystem/SpecialProtocol.h"
#if defined(HAS_MYSQL) || defined(HAS_MARIADB)
#include "mysqldataset.h"
#endif
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "sqlitedataset.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#ifdef TARGET_POSIX
#include "platform/posix/ConvUtils.h"
#endif

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>

using namespace dbiplus;

namespace
{
constexpr int MAX_COMPRESS_COUNT = 20;
} // unnamed namespace

CDatabase::Filter::Filter() = default;

void CDatabase::Filter::AppendField(const std::string& strField)
{
  if (strField.empty())
    return;

  if (fields.empty() || fields == "*")
    fields = strField;
  else
    fields += ", " + strField;
}

void CDatabase::Filter::AppendJoin(const std::string& strJoin)
{
  if (strJoin.empty())
    return;

  if (join.empty())
    join = strJoin;
  else
    join += " " + strJoin;
}

void CDatabase::Filter::AppendWhere(const std::string& strWhere, bool combineWithAnd /* = true */)
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

void CDatabase::Filter::AppendOrder(const std::string& strOrder)
{
  if (strOrder.empty())
    return;

  if (order.empty())
    order = strOrder;
  else
    order += ", " + strOrder;
}

void CDatabase::Filter::AppendGroup(const std::string& strGroup)
{
  if (strGroup.empty())
    return;

  if (group.empty())
    group = strGroup;
  else
    group += ", " + strGroup;
}

void CDatabase::ExistsSubQuery::AppendJoin(const std::string& strJoin)
{
  if (strJoin.empty())
    return;

  if (join.empty())
    join = strJoin;
  else
    join += " " + strJoin;
}

void CDatabase::ExistsSubQuery::AppendWhere(std::string_view strWhere,
                                            bool combineWithAnd /* = true */)
{
  if (strWhere.empty())
    return;

  if (where.empty())
    where = strWhere;
  else
  {
    where += combineWithAnd ? " AND " : " OR ";
    where += strWhere;
  }
}

bool CDatabase::ExistsSubQuery::BuildSQL(std::string& strSQL) const
{
  if (tablename.empty())
    return false;
  strSQL = "EXISTS (SELECT 1 FROM " + tablename;
  if (!join.empty())
    strSQL += " " + join;
  std::string strWhere;
  if (!param.empty())
    strWhere = param;
  if (!where.empty())
  {
    if (!strWhere.empty())
      strWhere += " AND ";
    strWhere += where;
  }
  if (!strWhere.empty())
    strSQL += " WHERE " + strWhere;

  strSQL += ")";
  return true;
}

CDatabase::DatasetLayout::DatasetLayout(size_t totalfields)
{
  m_fields.resize(totalfields, DatasetFieldInfo(false, false, -1));
}

void CDatabase::DatasetLayout::SetField(int fieldNo,
                                        std::string_view strField,
                                        bool bOutput /*= false*/)
{
  if (fieldNo >= 0 && fieldNo < static_cast<int>(m_fields.size()))
  {
    m_fields[fieldNo].strField = strField;
    m_fields[fieldNo].fetch = true;
    m_fields[fieldNo].output = bOutput;
  }
}

void CDatabase::DatasetLayout::AdjustRecordNumbers(int offset)
{
  int recno = 0;
  for (auto& field : m_fields)
  {
    if (field.fetch)
    {
      field.recno = recno + offset;
      ++recno;
    }
  }
}

bool CDatabase::DatasetLayout::GetFetch(int fieldno)
{
  if (fieldno >= 0 && fieldno < static_cast<int>(m_fields.size()))
    return m_fields[fieldno].fetch;
  return false;
}

void CDatabase::DatasetLayout::SetFetch(int fieldno, bool bFetch /*= true*/)
{
  if (fieldno >= 0 && fieldno < static_cast<int>(m_fields.size()))
    m_fields[fieldno].fetch = bFetch;
}

bool CDatabase::DatasetLayout::GetOutput(int fieldno)
{
  if (fieldno >= 0 && fieldno < static_cast<int>(m_fields.size()))
    return m_fields[fieldno].output;
  return false;
}

int CDatabase::DatasetLayout::GetRecNo(int fieldno)
{
  if (fieldno >= 0 && fieldno < static_cast<int>(m_fields.size()))
    return m_fields[fieldno].recno;
  return -1;
}

std::string CDatabase::DatasetLayout::GetFields() const
{
  std::string strSQL;
  for (const auto& field : m_fields)
  {
    if (!field.strField.empty() && field.fetch)
    {
      if (strSQL.empty())
        strSQL = field.strField;
      else
        strSQL += ", " + field.strField;
    }
  }

  return strSQL;
}

bool CDatabase::DatasetLayout::HasFilterFields() const
{
  return std::ranges::any_of(m_fields, [](const auto& field) { return field.fetch; });
}

CDatabase::CDatabase()
  : m_profileManager(*CServiceBroker::GetSettingsComponent()->GetProfileManager())
{
}

CDatabase::~CDatabase(void)
{
  Close();
}

void CDatabase::Split(const std::string& strFileNameAndPath,
                      std::string& strPath,
                      std::string& strFileName) const
{
  strFileName = "";
  strPath = "";
  int i = strFileNameAndPath.size() - 1;
  while (i > 0)
  {
    char ch = strFileNameAndPath[i];
    if (ch == ':' || ch == '/' || ch == '\\')
      break;
    else
      i--;
  }
  strPath = strFileNameAndPath.substr(0, i);
  strFileName = strFileNameAndPath.substr(i);
}

std::string CDatabase::PrepareSQL(std::string_view sqlFormat, ...) const
{
  std::string strResult;

  if (nullptr != m_pDB)
  {
    va_list args;
    va_start(args, sqlFormat);
    strResult = m_pDB->vprepare(sqlFormat, args);
    va_end(args);
  }

  return strResult;
}

std::string CDatabase::GetSingleValue(const std::string& query, Dataset& ds) const
{
  std::string ret;
  try
  {
    if (!m_pDB)
      return ret;

    if (ds.query(query) && ds.num_rows() > 0)
      ret = ds.fv(0).get_asString();

    ds.close();
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed on query '{}'", query);
  }
  return ret;
}

std::string CDatabase::GetSingleValue(const std::string& strTable,
                                      const std::string& strColumn,
                                      const std::string& strWhereClause /* = std::string() */,
                                      const std::string& strOrderBy /* = std::string() */) const
{
  if (!m_pDS)
    return {};

  std::string query = PrepareSQL("SELECT %s FROM %s", strColumn.c_str(), strTable.c_str());
  if (!strWhereClause.empty())
    query += " WHERE " + strWhereClause;
  if (!strOrderBy.empty())
    query += " ORDER BY " + strOrderBy;
  query += " LIMIT 1";
  return GetSingleValue(query, *m_pDS);
}

std::string CDatabase::GetSingleValue(const std::string& query) const
{
  if (!m_pDS)
    return {};

  return GetSingleValue(query, *m_pDS);
}

int CDatabase::GetSingleValueInt(const std::string& query, Dataset& ds) const
{
  int ret = 0;
  try
  {
    if (!m_pDB)
      return ret;

    if (ds.query(query) && ds.num_rows() > 0)
      ret = ds.fv(0).get_asInt();

    ds.close();
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed on query '{}'", query);
  }
  return ret;
}

int CDatabase::GetSingleValueInt(const std::string& strTable,
                                 const std::string& strColumn,
                                 const std::string& strWhereClause /* = std::string() */,
                                 const std::string& strOrderBy /* = std::string() */) const
{
  std::string strResult = GetSingleValue(strTable, strColumn, strWhereClause, strOrderBy);
  return static_cast<int>(std::strtol(strResult.c_str(), nullptr, 10));
}

int CDatabase::GetSingleValueInt(const std::string& query) const
{
  if (!m_pDS)
    return 0;

  return GetSingleValueInt(query, *m_pDS);
}

bool CDatabase::DeleteValues(const std::string& strTable, const Filter& filter /* = Filter() */)
{
  std::string strQuery;
  BuildSQL(PrepareSQL("DELETE FROM %s ", strTable.c_str()), filter, strQuery);
  return ExecuteQuery(strQuery);
}

bool CDatabase::BeginMultipleExecute()
{
  m_multipleExecute = true;
  m_multipleQueries.clear();
  return true;
}

bool CDatabase::CommitMultipleExecute()
{
  m_multipleExecute = false;
  BeginTransaction();
  for (const auto& i : m_multipleQueries)
  {
    if (!ExecuteQuery(i))
    {
      RollbackTransaction();
      return false;
    }
  }
  m_multipleQueries.clear();
  return CommitTransaction();
}

bool CDatabase::ExecuteQuery(const std::string& strQuery)
{
  if (m_multipleExecute)
  {
    m_multipleQueries.push_back(strQuery);
    return true;
  }

  bool bReturn = false;

  try
  {
    if (nullptr == m_pDB)
      return bReturn;
    if (nullptr == m_pDS)
      return bReturn;
    m_pDS->exec(strQuery);
    bReturn = true;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to execute query '{}'", strQuery);
  }

  return bReturn;
}

bool CDatabase::ResultQuery(const std::string& strQuery) const
{
  bool bReturn = false;

  try
  {
    if (nullptr == m_pDB)
      return bReturn;
    if (nullptr == m_pDS)
      return bReturn;

    std::string strPreparedQuery = PrepareSQL(strQuery);

    bReturn = m_pDS->query(strPreparedQuery);
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Failed to execute query '{}'", strQuery);
  }

  return bReturn;
}

bool CDatabase::QueueInsertQuery(const std::string& strQuery)
{
  if (strQuery.empty())
    return false;

  if (!m_bMultiInsert)
  {
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS2)
      return false;

    m_bMultiInsert = true;
    m_pDS2->insert();
  }

  m_pDS2->add_insert_sql(strQuery);

  return true;
}

bool CDatabase::CommitInsertQueries()
{
  bool bReturn = true;

  if (m_bMultiInsert)
  {
    try
    {
      m_bMultiInsert = false;
      m_pDS2->post();
      m_pDS2->clear_insert_sql();
    }
    catch (...)
    {
      bReturn = false;
      CLog::LogF(LOGERROR, "Failed to execute queries");
    }
  }

  return bReturn;
}

size_t CDatabase::GetInsertQueriesCount() const
{
  return m_pDS2->insert_sql_count();
}

bool CDatabase::QueueDeleteQuery(const std::string& strQuery)
{
  if (strQuery.empty() || !m_pDB || !m_pDS)
    return false;

  m_bMultiDelete = true;
  m_pDS->del();
  m_pDS->add_delete_sql(strQuery);
  return true;
}

bool CDatabase::CommitDeleteQueries()
{
  bool bReturn = true;

  if (m_bMultiDelete)
  {
    try
    {
      m_bMultiDelete = false;
      m_pDS->deletion();
      m_pDS->clear_delete_sql();
    }
    catch (...)
    {
      bReturn = false;
      CLog::LogF(LOGERROR, "Failed to execute queries");
    }
  }

  return bReturn;
}

size_t CDatabase::GetDeleteQueriesCount() const
{
  return m_pDS->delete_sql_count();
}

bool CDatabase::Open()
{
  DatabaseSettings db_fallback;
  return Open(db_fallback);
}

bool CDatabase::Open(const DatabaseSettings& settings)
{
  if (IsOpen())
  {
    m_openCount++;
    return true;
  }

  // check our database manager to see if this database can be opened
  if (!CServiceBroker::GetDatabaseManager().CanOpen(GetBaseDBName()))
    return false;

  DatabaseSettings dbSettings = settings;
  InitSettings(dbSettings);

  std::string dbName = dbSettings.name;
  dbName += std::to_string(GetSchemaVersion());
  return Connect(dbName, dbSettings, false) == CDatabase::ConnectionState::STATE_CONNECTED;
}

void CDatabase::InitSettings(DatabaseSettings& dbSettings)
{
  m_sqlite = true;

#if defined(HAS_MYSQL) || defined(HAS_MARIADB)
  if (dbSettings.type == "mysql")
  {
    // check we have all information before we cancel the fallback
    if (!(dbSettings.host.empty() || dbSettings.user.empty() || dbSettings.pass.empty()))
      m_sqlite = false;
    else
      CLog::Log(LOGINFO, "Essential mysql database information is missing. Require at least host, "
                         "user and pass defined.");
  }
  else
#else
  if (dbSettings.type == "mysql")
    CLog::Log(
        LOGERROR,
        "MySQL library requested but MySQL support is not compiled in. Falling back to sqlite3.");
#endif
  {
    dbSettings.type = "sqlite3";
    if (dbSettings.host.empty())
      dbSettings.host = CSpecialProtocol::TranslatePath(m_profileManager.GetDatabaseFolder());
  }

  // use separate, versioned database unless user has elected to share default in profile
  if (dbSettings.name.empty() || !m_profileManager.GetCurrentProfile().hasDatabases())
    dbSettings.name = GetBaseDBName();
}

void CDatabase::CopyDB(const std::string& latestDb)
{
  m_pDB->copy(latestDb.c_str());
}

void CDatabase::DropAnalytics()
{
  m_pDB->drop_analytics();
}

CDatabase::ConnectionState CDatabase::Connect(const std::string& dbName,
                                              const DatabaseSettings& dbSettings,
                                              bool create)
{
  // create the appropriate database structure
  if (dbSettings.type == "sqlite3")
  {
    m_pDB = std::make_unique<SqliteDatabase>();
  }
#if defined(HAS_MYSQL) || defined(HAS_MARIADB)
  else if (dbSettings.type == "mysql")
  {
    m_pDB = std::make_unique<MysqlDatabase>();
  }
#endif
  else
  {
    CLog::Log(LOGERROR, "Unable to determine database type: {}", dbSettings.type);
    return ConnectionState::STATE_ERROR;
  }

  // host name is always required
  m_pDB->setHostName(dbSettings.host.c_str());

  if (!dbSettings.port.empty())
    m_pDB->setPort(dbSettings.port.c_str());

  if (!dbSettings.user.empty())
    m_pDB->setLogin(dbSettings.user.c_str());

  if (!dbSettings.pass.empty())
    m_pDB->setPasswd(dbSettings.pass.c_str());

  // database name is always required
  m_pDB->setDatabase(dbName.c_str());

  // set configuration regardless if any are empty
  m_pDB->setConfig(dbSettings.key.c_str(), dbSettings.cert.c_str(), dbSettings.ca.c_str(),
                   dbSettings.capath.c_str(), dbSettings.ciphers.c_str(), dbSettings.connecttimeout,
                   dbSettings.compression);

  // create the datasets
  m_pDS.reset(m_pDB->CreateDataset());
  m_pDS2.reset(m_pDB->CreateDataset());

  const int state{m_pDB->connect(create)};
  switch (state)
  {
    using enum ConnectionState;
    case DB_CONNECTION_OK:
      break;
    case DB_CONNECTION_DATABASE_NOT_FOUND:
      return STATE_DATABASE_NOT_FOUND;
    case DB_CONNECTION_NONE:
      return STATE_ERROR;
    default:
      CLog::LogF(LOGERROR, "Unhandled connection status: {}", state);
      return STATE_ERROR;
  }

  try
  {
    // test if db already exists, if not we need to create the tables
    if (!m_pDB->exists() && create)
    {
      if (dbSettings.type == "sqlite3")
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
      CreateDatabase();
    }
    m_pDB->postconnect();
  }
  catch (DbErrors& error)
  {
    CLog::LogF(LOGERROR, "Failed with '{}'", error.getMsg());
    m_openCount = 1; // set to open so we can execute Close()
    Close();
    return ConnectionState::STATE_ERROR;
  }

  m_openCount = 1; // our database is open
  return ConnectionState::STATE_CONNECTED;
}

int CDatabase::GetDBVersion()
{
  m_pDS->query("SELECT idVersion FROM version\n");
  if (m_pDS->num_rows() > 0)
    return m_pDS->fv("idVersion").get_asInt();
  return 0;
}

bool CDatabase::IsOpen() const
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
  m_multipleExecute = false;

  if (nullptr == m_pDB)
    return;
  if (nullptr != m_pDS)
    m_pDS->close();
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
    if (nullptr == m_pDB)
      return false;
    if (nullptr == m_pDS)
      return false;
    if (!bForce)
    {
      m_pDS->query("select iCompressCount from version");
      if (!m_pDS->eof())
      {
        int iCount = m_pDS->fv(0).get_asInt();
        if (iCount > MAX_COMPRESS_COUNT)
          iCount = -1;
        m_pDS->close();
        std::string strSQL = PrepareSQL("update version set iCompressCount=%i\n", ++iCount);
        m_pDS->exec(strSQL);
        if (iCount != 0)
          return true;
      }
    }

    if (!m_pDS->exec("vacuum\n"))
      return false;
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Compressing the database failed");
    return false;
  }
  return true;
}

void CDatabase::Interrupt()
{
  m_pDS->interrupt();
}

void CDatabase::BeginTransaction()
{
  try
  {
    if (nullptr != m_pDB)
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
    if (nullptr != m_pDB)
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
    if (nullptr != m_pDB)
      m_pDB->rollback_transaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "database:rollbacktransaction failed");
  }
}

bool CDatabase::CreateDatabase()
{
  BeginTransaction();
  try
  {
    CLog::Log(LOGINFO, "creating version table");
    m_pDS->exec("CREATE TABLE version (idVersion integer, iCompressCount integer)\n");
    std::string strSQL = PrepareSQL("INSERT INTO version (idVersion,iCompressCount) values(%i,0)\n",
                                    GetSchemaVersion());
    m_pDS->exec(strSQL);

    CreateTables();
    CreateAnalytics();
  }
  catch (...)
  {
    CLog::LogF(LOGERROR, "Unable to create database:{}", static_cast<int>(GetLastError()));
    RollbackTransaction();
    return false;
  }

  return CommitTransaction();
}

void CDatabase::UpdateVersionNumber()
{
  std::string strSQL = PrepareSQL("UPDATE version SET idVersion=%i\n", GetSchemaVersion());
  m_pDS->exec(strSQL);
}

bool CDatabase::BuildSQL(std::string_view strQuery, const Filter& filter, std::string& strSQL) const
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

bool CDatabase::BuildSQL(const std::string& strBaseDir,
                         const std::string& strQuery,
                         Filter& filter,
                         std::string& strSQL,
                         CDbUrl& dbUrl)
{
  SortDescription sorting;
  return BuildSQL(strBaseDir, strQuery, filter, strSQL, dbUrl, sorting);
}

bool CDatabase::BuildSQL(const std::string& strBaseDir,
                         const std::string& strQuery,
                         Filter& filter,
                         std::string& strSQL,
                         CDbUrl& dbUrl,
                         SortDescription& sorting /* = SortDescription() */)
{
  // parse the base path to get additional filters
  dbUrl.Reset();
  if (!dbUrl.FromString(strBaseDir) || !GetFilter(dbUrl, filter, sorting))
    return false;

  return BuildSQL(strQuery, filter, strSQL);
}
