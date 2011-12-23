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

#include "Database.h"
#include "Util.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"
#include "utils/Crc32.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/File.h"
#include "utils/AutoPtrHandle.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "mysqldataset.h"
#include "sqlitedataset.h"


using namespace AUTOPTR;
using namespace dbiplus;

#define MAX_COMPRESS_COUNT 20

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

CStdString CDatabase::GetSingleValue(const CStdString &strTable, const CStdString &strColumn, const CStdString &strWhereClause /* = CStdString() */, const CStdString &strOrderBy /* = CStdString() */)
{
  CStdString strReturn;

  try
  {
    if (NULL == m_pDB.get()) return strReturn;
    if (NULL == m_pDS.get()) return strReturn;

    CStdString strQueryBase = "SELECT %s FROM %s";
    if (!strWhereClause.IsEmpty())
      strQueryBase.AppendFormat(" WHERE %s", strWhereClause.c_str());
    if (!strOrderBy.IsEmpty())
      strQueryBase.AppendFormat(" ORDER BY %s", strOrderBy.c_str());
    strQueryBase.append(" LIMIT 1");

    CStdString strQuery = PrepareSQL(strQueryBase,
        strColumn.c_str(), strTable.c_str());

    if (!m_pDS->query(strQuery.c_str())) return strReturn;

    if (m_pDS->num_rows() > 0)
    {
      strReturn = m_pDS->fv(0).get_asString();
    }

    m_pDS->close();
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - failed to get value '%s' from table '%s'",
        __FUNCTION__, strColumn.c_str(), strTable.c_str());
  }

  return strReturn;
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
  bool bReturn = false;

  if (m_bMultiWrite)
  {
    try
    {
      m_bMultiWrite = false;
      m_pDS2->post();
      m_pDS2->clear_insert_sql();
      bReturn = true;
    }
    catch(...)
    {
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
  // take a copy - we're gonna be messing with it and we don't want to touch the original
  DatabaseSettings dbSettings = settings;

  if (IsOpen())
  {
    m_openCount++;
    return true;
  }

  m_sqlite = true;

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
  {
    dbSettings.type = "sqlite3";
    dbSettings.host = _P(g_settings.GetDatabaseFolder());
    dbSettings.name = GetBaseDBName();
  }

  // use separate, versioned database
  int version = GetMinVersion();
  CStdString baseDBName = (dbSettings.name.IsEmpty() ? GetBaseDBName() : dbSettings.name.c_str());
  CStdString latestDb;
  latestDb.Format("%s%d", baseDBName, version);

  while (version >= 0)
  {
    if (version)
      dbSettings.name.Format("%s%d", baseDBName, version);
    else
      dbSettings.name.Format("%s", baseDBName);

    if (Connect(dbSettings, false))
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
          CLog::Log(LOGERROR, "Unable to copy old database %s to new version %s", dbSettings.name.c_str(), latestDb.c_str());
          copy_fail = true;
        }

        Close();

        if ( copy_fail )
          return false;

        dbSettings.name = latestDb;
        if (!Connect(dbSettings, false))
        {
          CLog::Log(LOGERROR, "Unable to open freshly copied database %s", dbSettings.name.c_str());
          return false;
        }
      }

      // yay - we have a copy of our db, now do our worst with it
      if (UpdateVersion(dbSettings.name))
        return true;

      // update failed - loop around and see if we have another one available
      Close();
    }

    // drop back to the previous version and try that
    version--;
  }


  // unable to open any version fall through to create a new one
  dbSettings.name = latestDb;

  if (Connect(dbSettings, true) && UpdateVersion(dbSettings.name))
  {
    return true;
  }
  // safely fall back to sqlite as appropriate
  else if ( ! m_sqlite )
  {
    CLog::Log(LOGDEBUG, "Falling back to sqlite.");
    dbSettings = settings;
    dbSettings.type = "sqlite3";
    return Open(dbSettings);
  }

  // failed to update or open the database
  Close();
  CLog::Log(LOGERROR, "Unable to open database %s", dbSettings.name.c_str());
  return false;
}

bool CDatabase::Connect(const DatabaseSettings &dbSettings, bool create)
{
  // create the appropriate database structure
  if (dbSettings.type.Equals("sqlite3"))
  {
    m_pDB.reset( new SqliteDatabase() ) ;
  }
  else if (dbSettings.type.Equals("mysql"))
  {
    m_pDB.reset( new MysqlDatabase() ) ;
  }
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
  m_pDB->setDatabase(dbSettings.name.c_str());

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

bool CDatabase::UpdateVersion(const CStdString &dbName)
{
  int version = 0;
  m_pDS->query("SELECT idVersion FROM version\n");
  if (m_pDS->num_rows() > 0)
    version = m_pDS->fv("idVersion").get_asInt();

  if (version < GetMinVersion())
  {
    CLog::Log(LOGNOTICE, "Attempting to update the database %s from version %i to %i", dbName.c_str(), version, GetMinVersion());
    if (UpdateOldVersion(version) && UpdateVersionNumber())
      CLog::Log(LOGINFO, "Update to version %i successfull", GetMinVersion());
    else
    {
      CLog::Log(LOGERROR, "Can't update the database %s from version %i to %i", dbName.c_str(), version, GetMinVersion());
      return false;
    }
  }
  else if (version > GetMinVersion())
  {
    CLog::Log(LOGERROR, "Can't open the database %s as it is a NEWER version than what we were expecting?", dbName.c_str());
    return false;
  }
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
  try
  {
    CStdString strSQL=PrepareSQL("UPDATE version SET idVersion=%i\n", GetMinVersion());
    m_pDS->exec(strSQL.c_str());

    CommitTransaction();
  }
  catch(...)
  {
    return false;
  }

  return true;
}

