#include "stdafx.h"
#include "database.h"

CDatabase::CDatabase(void)
{
  m_bOpen = false;
  m_iRefCount = 0;
}

CDatabase::~CDatabase(void)
{

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
  auto_aptr<char> szStmt(sqlite3_vmprintf(strStmt.c_str(), args));
  va_end(args);

  return szStmt.get();
}

bool CDatabase::Open()
{
  if (IsOpen())
  {
    m_iRefCount++;
    return true;
  }

  CStdString strDatabase = g_stSettings.m_szAlbumDirectory;
  strDatabase+="\\";
  strDatabase+=m_strDatabaseFile;

  // test id dbs already exists, if not we need 2 create the tables
  bool bDatabaseExists = false;
  FILE* fd = fopen(strDatabase.c_str(), "rb");
  if (fd)
  {
    bDatabaseExists = true;
    fclose(fd);
  }

  m_pDB.reset(new SqliteDatabase() ) ;
  m_pDB->setDatabase(strDatabase.c_str());

  m_pDS.reset(m_pDB->CreateDataset());
  m_pDS2.reset(m_pDB->CreateDataset());

  if ( m_pDB->connect() != DB_CONNECTION_OK)
  {
    CLog::Log(LOGERROR, "Unable to open %s (old version?)", m_strDatabaseFile.c_str());
    Close();
    ::DeleteFile(strDatabase.c_str());
    return false;
  }

  if (!bDatabaseExists)
  {
    if (!CreateTables())
    {
      CLog::Log(LOGERROR, "Unable to create %s", m_strDatabaseFile.c_str());
      Close();
      ::DeleteFile(strDatabase.c_str());
      return false;
    }
  }
  else
  { // Database exists, check the version number
    m_pDS->query("SELECT * FROM sqlite_master WHERE type = 'table' AND name = 'version'\n");
    float fVersion = 0.0f;
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      m_pDS->query("SELECT idVersion FROM version\n");
      if (m_pDS->num_rows() > 0)
        fVersion = m_pDS->fv("idVersion").get_asFloat();
    }
    if (fVersion < m_fVersion)
    {
      CLog::Log(LOGNOTICE, "Attempting to update the database %s from version %.2f to %.2f", m_strDatabaseFile.c_str(), fVersion, m_fVersion);
      if (UpdateOldVersion(fVersion) && UpdateVersionNumber())
        CLog::Log(LOGINFO, "Update to version %.2f successfull", m_fVersion);
      else
      {
        CLog::Log(LOGERROR, "Can't update the database %s from version %.2f to %.2f", m_strDatabaseFile.c_str(), fVersion, m_fVersion);
        Close();
        return false;
      }
    }
    else if (fVersion > m_fVersion)
    {
      CLog::Log(LOGERROR, "Can't open the database %s as it is a NEWER version than what we were expecting!", m_strDatabaseFile.c_str());
      Close();
      return false;
    }
  }


  m_pDS->exec("PRAGMA cache_size=16384\n");
  m_pDS->exec("PRAGMA synchronous='NORMAL'\n");
  m_pDS->exec("PRAGMA count_changes='OFF'\n");
  m_bOpen = true;
  m_iRefCount++;
  return true;
}

bool CDatabase::IsOpen()
{
  return m_bOpen;
}

void CDatabase::Close()
{
  if (!m_bOpen)
    return ;

  if (m_iRefCount > 1)
  {
    m_iRefCount--;
    return ;
  }

  m_iRefCount--;
  m_bOpen = false;

  if (NULL == m_pDB.get() ) return ;
  if (NULL != m_pDS.get()) m_pDS->close();
  m_pDB->disconnect();
  m_pDB.reset();
  m_pDS.reset();
  m_pDS2.reset();
}

bool CDatabase::Compress()
{
  // compress database
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    if (!m_pDS->exec("vacuum\n"))
      return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Compressing the database %s failed", m_strDatabaseFile.c_str());
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
    //  all fatx formatted partitions, except the utility drive, 
    //  have a cluster size of 16k. To gain better performance 
    //  when performing write operations to the database, set 
    //  the page size of the database file to 16k. 
    //  This needs to be done before any table is created.
    CLog::Log(LOGINFO, "Set page size");
    m_pDS->exec("PRAGMA page_size=16384\n");
    //  Also set the memory cache size to 16k
    CLog::Log(LOGINFO, "Set default cache size");
    m_pDS->exec("PRAGMA default_cache_size=16384\n");

    CLog::Log(LOGINFO, "creating version table");
    m_pDS->exec("CREATE TABLE version (idVersion float)\n");
    CStdString strSQL=FormatSQL("INSERT INTO version (idVersion) values(%f)\n", m_fVersion);
    m_pDS->exec(strSQL.c_str());

    return true;
}

bool CDatabase::UpdateOldVersion(float fVersion)
{
  return false;
}

bool CDatabase::UpdateVersionNumber()
{
  try
  {
    CStdString strSQL=FormatSQL("UPDATE version SET idVersion=%f\n", m_fVersion);
    m_pDS->exec(strSQL.c_str());
  }
  catch(...)
  {
    return false;
  }

  return true;
}
