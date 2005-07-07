//********************************************************************************************************************************
//**
//** see docs/videodatabase.png for a diagram of the database
//**
//********************************************************************************************************************************

#include "stdafx.h"
#include ".\programdatabase.h"
#include "utils/fstrcmp.h"
#include "util.h"

//********************************************************************************************************************************
CProgramDatabase::CProgramDatabase(void)
{}

//********************************************************************************************************************************
CProgramDatabase::~CProgramDatabase(void)
{}

//********************************************************************************************************************************
void CProgramDatabase::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
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

//********************************************************************************************************************************
void CProgramDatabase::RemoveInvalidChars(CStdString& strTxt)
{
  CStdString strReturn = "";
  for (int i = 0; i < (int)strTxt.size(); ++i)
  {
    byte k = strTxt[i];
    if (k == 0x27)
    {
      strReturn += k;
    }
    strReturn += k;
  }
  if (strReturn == "")
    strReturn = "unknown";
  strTxt = strReturn;
}



//********************************************************************************************************************************
bool CProgramDatabase::Open()
{
  CStdString programDatabase = g_stSettings.m_szAlbumDirectory;
  programDatabase += PROGRAM_DATABASE_NAME;

  Close();

  // test id dbs already exists, if not we need 2 create the tables
  bool bDatabaseExists = false;
  FILE* fd = fopen(programDatabase.c_str(), "rb");
  if (fd)
  {
    bDatabaseExists = true;
    fclose(fd);
  }

  m_pDB.reset(new SqliteDatabase() ) ;
  m_pDB->setDatabase(programDatabase.c_str());

  m_pDS.reset(m_pDB->CreateDataset());
  if ( m_pDB->connect() != DB_CONNECTION_OK)
  {
    CLog::Log(LOGINFO, "programdatabase::unable to open %s", programDatabase.c_str());
    Close();
    ::DeleteFile(programDatabase.c_str());
    return false;
  }

  if (!bDatabaseExists)
  {
    if (!CreateTables())
    {
      CLog::Log(LOGINFO, "programdatabase::unable to create %s", programDatabase.c_str());
      Close();
      ::DeleteFile(programDatabase.c_str());
      return false;
    }
  }

  m_pDS->exec("PRAGMA cache_size=16384\n");
  m_pDS->exec("PRAGMA synchronous='NORMAL'\n");
  m_pDS->exec("PRAGMA count_changes='OFF'\n");
  // m_pDS->exec("PRAGMA temp_store='MEMORY'\n");
  return true;
}


//********************************************************************************************************************************
void CProgramDatabase::Close()
{
  if (NULL == m_pDB.get() ) return ;
  if (NULL != m_pDS.get()) m_pDS->close();
  m_pDB->disconnect();
  m_pDB.reset();
  m_pDS.reset();
}

//********************************************************************************************************************************
bool CProgramDatabase::CreateTables()
{

  try
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

    CLog::Log(LOGINFO, "create program table");
    m_pDS->exec("CREATE TABLE program ( idProgram integer primary key, idPath integer, idBookmark integer)\n");

    CLog::Log(LOGINFO, "create bookmark table");
    m_pDS->exec("CREATE TABLE bookmark (idBookmark integer primary key, bookmarkName text)\n");

    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text)\n");

    CLog::Log(LOGINFO, "create files table");
    m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, strFilename text, titleId text, xbedescription text, iTimesPlayed integer, lastAccessed integer)\n");

    CLog::Log(LOGINFO, "create bookmark index");
    m_pDS->exec("CREATE INDEX idxBookMark ON bookmark(bookmarkName)");
    CLog::Log(LOGINFO, "create path index");
    m_pDS->exec("CREATE INDEX idxPath ON path(strPath)");
    CLog::Log(LOGINFO, "create files index");
    m_pDS->exec("CREATE INDEX idxFiles ON files(strFilename)");
    CLog::Log(LOGINFO, "create files - titleid index");
    m_pDS->exec("CREATE INDEX idxTitleIdFiles ON files(titleId)");


  }
  catch (...)
  {
    CLog::Log(LOGERROR, "programdatabase::unable to create tables:%i", GetLastError());
    return false;
  }

  return true;
}

//********************************************************************************************************************************

long CProgramDatabase::GetFile(const CStdString& strFilenameAndPath, CFileItemList& programs)
{
  try
  {
    FILETIME localTime;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath, strFileName;
    Split(strFilenameAndPath, strPath, strFileName);
    RemoveInvalidChars(strPath);
    RemoveInvalidChars(strFileName);
    strPath.Replace("\\", "/");
    strFileName.Replace("\\", "/");
    CStdString strSQL = "";
    strSQL.Format("select * from path,files where path.idPath=files.idPath and path.strPath like '%s' and files.strFileName like '%s'", strPath.c_str(), strFileName.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;
      CStdString strPath, strFile, strPathandFile;
      strPath = m_pDS->fv("path.strPath").get_asString();
      strFile = m_pDS->fv("files.strFilename").get_asString();
      long lFileId = m_pDS->fv("files.idFile").get_asLong();
      strPathandFile = strPath + strFile;
      strPathandFile.Replace("/", "\\");
      if (CUtil::FileExists(strPathandFile))
      {
        CFileItem *pItem = new CFileItem(m_pDS->fv("files.xbedescription").get_asString());
        pItem->m_iprogramCount = m_pDS->fv("files.iTimesPlayed").get_asLong();
        pItem->m_strPath = strPathandFile;
        pItem->m_bIsFolder = false;
        GetFileAttributesEx(pItem->m_strPath, GetFileExInfoStandard, &FileAttributeData);
        FileTimeToLocalFileTime(&FileAttributeData.ftLastWriteTime, &localTime);
        FileTimeToSystemTime(&localTime, &pItem->m_stTime);
        pItem->m_dwSize = FileAttributeData.nFileSizeLow;
        programs.Add(pItem);
        m_pDS->close();
        return lFileId;
      }
      else
      {
        m_pDS->close();
        DeleteFile(lFileId);
        return -1;
      }
      m_pDS->close();
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetFile(%s)", strFilenameAndPath.c_str());
  }
  return -1;
}

bool CProgramDatabase::GetXBEPathByTitleId(const DWORD titleId, CStdString& strPathAndFilename)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    CStdString strSQL;
    strSQL.Format("select path.strPath, files.strFilename from files join path on files.idPath=path.idPath where files.titleId='%x'", titleId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      strPathAndFilename = m_pDS->fv("path.strPath").get_asString();
      strPathAndFilename += m_pDS->fv("files.strFilename").get_asString();
      strPathAndFilename.Replace('/', '\\');
      m_pDS->close();
      return true;
    }
    m_pDS->close();
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetXBEPathByTitleId(%i) failed", titleId);
  }
  return false;
}

long CProgramDatabase::AddFile(long lPathId, const CStdString& strFileName , DWORD titleId, const CStdString& strDescription)
{
  CStdString strSQL = "";
  try
  {
    long lFileId;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    strSQL.Format("select * from files where idPath=%i and strFileName like '%s'", lPathId, strFileName.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      lFileId = m_pDS->fv("idFile").get_asLong() ;
      m_pDS->close();
      return lFileId;
    }
    m_pDS->close();
    SYSTEMTIME localTime;
    GetLocalTime(&localTime);
    unsigned __int64 lastAccessed = LocalTimeToTimeStamp(localTime);
    strSQL.Format ("insert into files (idFile, idPath, strFileName, titleId, xbedescription, iTimesPlayed, lastAccessed) values(NULL, %i, '%s', '%x','%s',%i,%I64u)", lPathId, strFileName.c_str(), titleId, strDescription.c_str(), 0, lastAccessed);
    m_pDS->exec(strSQL.c_str());
    lFileId = (long)sqlite3_last_insert_rowid( m_pDB->getHandle() );
    return lFileId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "programdatabase:unable to addfile (%s)", strSQL.c_str());
  }
  return -1;
}

//********************************************************************************************************************************

long CProgramDatabase::AddBookMark(const CStdString& strBookmark)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strSQL;
    strSQL.Format("select * from bookmark where bookmarkName='%s'", strBookmark.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      strSQL.Format("insert into bookmark (idBookmark, bookMarkname) values (NULL, '%s')", strBookmark.c_str());
      m_pDS->exec(strSQL.c_str());
      long lBookmarkId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      return lBookmarkId;
    }
    else
    {
      const field_value value = m_pDS->fv("idBookmark");
      long lBookmarkId = value.get_asLong();
      m_pDS->close();
      return lBookmarkId;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::AddBookMark(%s) failed", strBookmark.c_str());
  }
  return -1;
}


//********************************************************************************************************************************
long CProgramDatabase::AddPath(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strSQL;
    strSQL.Format("select * from path where strPath like '%s'", strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL.Format("insert into Path (idPath, strPath) values( NULL, '%s')",
                    strPath.c_str());
      m_pDS->exec(strSQL.c_str());
      long lPathId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      return lPathId;
    }
    else
    {
      const field_value value = m_pDS->fv("idPath");
      long lPathId = value.get_asLong() ;
      m_pDS->close();
      return lPathId;
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::AddPath(%s) failed", strPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************

bool CProgramDatabase::EntryExists(const CStdString& strPath, const CStdString& strBookmark)
{
  try
  {
    CStdString strPath1 = strPath;
    RemoveInvalidChars(strPath1);
    strPath1.Replace("\\", "/");
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lPathId = GetPath(strPath1);
    long lBookmarkId = AddBookMark(strBookmark);
    CStdString strSQL;
    strSQL.Format("select * from program where idPath=%i and idBookmark=%i", lPathId, lBookmarkId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::EntryExists(%s,%s) failed", strPath.c_str(), strBookmark.c_str());
  }
  return false;
}

void CProgramDatabase::GetPathsByBookmark(const CStdString& strBookmark, vector <CStdString>& vecPaths)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;
    long lBookMarkId = AddBookMark(strBookmark);
    CStdString strSQL;
    strSQL.Format("select strPath from path,program where program.idBookmark=%i and program.idPath=path.idPath", lBookMarkId);
    m_pDS->query(strSQL.c_str());
    while (!m_pDS->eof())
    {
      CStdString strPath = m_pDS->fv("strPath").get_asString();
      strPath.Replace("/", "\\");
      vecPaths.push_back(strPath);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetPathsByBookmark(%s) failed", strBookmark.c_str());
  }
}


long CProgramDatabase::GetPath(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strSQL;
    strSQL.Format("select * from path where strPath like '%s' ", strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      long lPathId = m_pDS->fv("idPath").get_asLong();
      m_pDS->close();
      return lPathId;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetPath(%s) failed", strPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
long CProgramDatabase::GetProgram(long lPathId)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strSQL;
    strSQL.Format("select idProgram from program where idPath=%i", lPathId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      long lProgramId = m_pDS->fv("idProgram").get_asLong();
      m_pDS->close();
      return lProgramId;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetProgram(%i) failed", lPathId);
  }
  return -1;
}

//********************************************************************************************************************************
long CProgramDatabase::AddProgram(const CStdString& strFilenameAndPath, DWORD titleId, const CStdString& strDescription, const CStdString& strBookmark)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    CStdString strPath, strFileName, strDes = strDescription;
    Split(strFilenameAndPath, strPath, strFileName);
    RemoveInvalidChars(strPath);
    RemoveInvalidChars(strFileName);
    RemoveInvalidChars(strDes);
    strPath.Replace("\\", "/");
    strFileName.Replace("\\", "/");

    long lPathId = GetPath(strPath);

    if (!EntryExists(strPath, strBookmark))
    {

      CStdString strSQL;

      lPathId = AddPath(strPath);
      if (lPathId < 0) return -1;
      long lBookMarkId = AddBookMark(strBookmark);
      if (lBookMarkId < 0) return -1;
      strSQL.Format("insert into program (idProgram, idPath, idBookmark) values( NULL, %i, %i)",
                    lPathId, lBookMarkId);
      m_pDS->exec(strSQL.c_str());
      long lProgramId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      AddFile(lPathId, strFileName, titleId, strDes);
    }
    else
    {
      long lProgramId = GetProgram(lPathId);
      AddFile(lPathId, strFileName, titleId, strDes);
      return lProgramId;
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::AddProgram(%s,%s) failed", strFilenameAndPath.c_str() , strDescription.c_str() );
  }
  return -1;
}

unsigned __int64 CProgramDatabase::LocalTimeToTimeStamp( const SYSTEMTIME & localTime )
{

  unsigned __int64 localFileTime;
  unsigned __int64 systemFileTime;

  ::SystemTimeToFileTime( &localTime, (FILETIME*)&localFileTime );
  ::LocalFileTimeToFileTime( (const FILETIME*)&localFileTime,(FILETIME*)&systemFileTime );

  systemFileTime += Date_1601;
  return systemFileTime;
}

SYSTEMTIME CProgramDatabase::TimeStampToLocalTime( unsigned __int64 timeStamp )
{
  SYSTEMTIME localTime;
  unsigned __int64 fileTime;

  timeStamp -= Date_1601;
  ::FileTimeToLocalFileTime( (const FILETIME *)&timeStamp, (FILETIME*)&fileTime);
  ::FileTimeToSystemTime( (const FILETIME *)&fileTime, &localTime );
  return localTime;
} 


//********************************************************************************************************************************
void CProgramDatabase::GetProgramsByBookmark(CStdString& strBookmark, CFileItemList& programs, int iDepth, bool bOnlyDefaultXBE)
{
  try
  {
    VECPROGRAMPATHS todelete;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lBookmarkId = AddBookMark(strBookmark);
    CStdString strSQL;
    if (bOnlyDefaultXBE)
    {
      strSQL.Format("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path,program where program.idBookmark=%i and program.idPath=path.idPath and files.idPath=program.idPath and files.strFilename like '/default.xbe'", lBookmarkId);
    }
    else
    {
      strSQL.Format("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path,program where program.idBookmark=%i and program.idPath=path.idPath and files.idPath=program.idPath", lBookmarkId);
    }
    m_pDS->query(strSQL.c_str());
    while (!m_pDS->eof())
    {
      CStdString strPath, strFile, strPathandFile;
      strPath = m_pDS->fv("strPath").get_asString();
      strFile = m_pDS->fv("strFilename").get_asString();
      strPathandFile = strPath + strFile;
      strPathandFile.Replace("/", "\\");
      CFileItem *pItem = new CFileItem(m_pDS->fv("xbedescription").get_asString());
      pItem->m_iprogramCount = m_pDS->fv("iTimesPlayed").get_asLong();
      pItem->m_strPath = strPathandFile;
      pItem->m_bIsFolder = false;
      CStdString timestampstr = m_pDS->fv("lastAccessed").get_asString();
      unsigned __int64 timestamp = _atoi64(timestampstr.c_str());
      pItem->m_stTime=TimeStampToLocalTime(timestamp);
      programs.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetProgamsByBookmark() failed");
  }

}

void CProgramDatabase::GetProgramsByPath(const CStdString& strPath, CFileItemList& programs, int iDepth, bool bOnlyDefaultXBE)
{
  try
  {
    VECPROGRAMPATHS todelete;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CStdString strSQL;
    CStdString strPath1 = strPath;
    strPath1.Replace("\\", "/");
    CStdString strShortCutsDir = g_stSettings.m_szShortcutDirectory;
    strShortCutsDir.Replace("\\", "/");
    if (bOnlyDefaultXBE)
    {
      strSQL.Format("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path where files.idPath=path.idPath and path.strPath like '%s/%%' and files.strFilename like '/default.xbe'", strPath1.c_str());
    }
    else
    {
      if (strPath1.c_str() == strShortCutsDir)
      {
        strSQL.Format("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path where files.idPath=path.idPath and path.strPath like '%s'", strPath1.c_str());
      }
      else
      {
        strSQL.Format("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path where files.idPath=path.idPath and path.strPath like '%s/%%'", strPath1.c_str());
      }
    }
    m_pDS->query(strSQL.c_str());
    while (!m_pDS->eof())
    {
      CStdString strPath, strFile, strPathandFile;
      strPath = m_pDS->fv("strPath").get_asString();
      strFile = m_pDS->fv("strFilename").get_asString();
      int depth = StringUtils::FindNumber(strPath, "/") - StringUtils::FindNumber(strPath1, "/");
      strPathandFile = strPath + strFile;
      strPathandFile.Replace("/", "\\");
      if (depth <= iDepth)
      {
        CFileItem *pItem = new CFileItem(m_pDS->fv("xbedescription").get_asString());
        pItem->m_iprogramCount = m_pDS->fv("iTimesPlayed").get_asLong();
        pItem->m_strPath = strPathandFile;
        pItem->m_bIsFolder = false;
        CStdString timestampstr = m_pDS->fv("lastAccessed").get_asString();
        unsigned __int64 timestamp = _atoi64(timestampstr.c_str());
        pItem->m_stTime=TimeStampToLocalTime(timestamp);
        programs.Add(pItem);
      }
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetProgramsByPath() failed");
  }
}

//********************************************************************************************************************************

void CProgramDatabase::DeleteFile(long lFileId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString strSQL;
    strSQL.Format("delete from files where idFile=%i", lFileId);
    m_pDS->exec(strSQL.c_str());
  }

  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::DeleteFile() failed");
  }
}



void CProgramDatabase::DeleteProgram(const CStdString& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    long lPathId = GetPath(strPath);
    if (lPathId < 0)
    {
      return ;
    }


    CStdString strSQL;
    strSQL.Format("delete from files where idpath=%i", lPathId);
    m_pDS->exec(strSQL.c_str());

    strSQL.Format("delete from program where idpath=%i", lPathId);
    m_pDS->exec(strSQL.c_str());

    strSQL.Format("delete from path where idpath=%i", lPathId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::DeleteProgram() failed");
  }
}

bool CProgramDatabase::IncTimesPlayed(const CStdString& strFileName1)
{
  try
  {
    CStdString strFileName = strFileName1;
    RemoveInvalidChars(strFileName);

    CStdString strPath;
    CUtil::GetDirectory(strFileName, strPath);
    strPath.Replace("\\", "/");

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    CStdString strSQL;

    strSQL.Format("select * from files,path where files.idPath=path.idPath and path.strPath='%s'", strPath.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int idFile = m_pDS->fv("files.idFile").get_asLong();
    int iTimesPlayed = m_pDS->fv("files.iTimesPlayed").get_asLong();
    m_pDS->close();

    CLog::Log(LOGDEBUG, "CProgramDatabase::IncTimesPlayed(%s), idFile=%i, iTimesPlayed=%i",
              strFileName1.c_str(), idFile, iTimesPlayed);

    strSQL.Format("update files set iTimesPlayed=%i where idFile=%i",
                  ++iTimesPlayed, idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:IncTimesPlayed(%s) failed", strFileName1.c_str());
  }

  return false;
}

