#include "stdafx.h"
#include ".\programdatabase.h"
#include "utils/fstrcmp.h"
#include "util.h"
#include "xbox/xbeheader.h"

#define PROGRAM_DATABASE_VERSION 0.7f

//********************************************************************************************************************************
CProgramDatabase::CProgramDatabase(void)
{
  m_fVersion=PROGRAM_DATABASE_VERSION;
  m_strDatabaseFile=PROGRAM_DATABASE_NAME;
}

//********************************************************************************************************************************
CProgramDatabase::~CProgramDatabase(void)
{

}

//********************************************************************************************************************************
bool CProgramDatabase::CreateTables()
{

  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "create program table");
    m_pDS->exec("CREATE TABLE program ( idProgram integer primary key, idPath integer, idBookmark integer)\n");
    CLog::Log(LOGINFO, "create bookmark table");
    m_pDS->exec("CREATE TABLE bookmark (idBookmark integer primary key, bookmarkName text)\n");
    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text)\n");
    CLog::Log(LOGINFO, "create files table");
    m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, strFilename text, titleId text, xbedescription text, iTimesPlayed integer, lastAccessed integer, iRegion integer)\n");
    CLog::Log(LOGINFO, "create trainers table");
    m_pDS->exec("CREATE TABLE trainers (idKey integer auto_increment primary key, idCRC integer, idTitle integer, strTrainerPath text, strSettings text, Active integer)\n");
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

bool CProgramDatabase::UpdateOldVersion(float fVersion)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  if (NULL == m_pDS2.get()) return false;

  try
  {
    if (fVersion < 0.5f)
    { // Version 0 to 0.5 upgrade - we need to add the version table
      CLog::Log(LOGINFO, "creating versions table");
      m_pDS->exec("CREATE TABLE version (idVersion float)\n");
    }

    if (fVersion < 0.6f)
    { // Version 0.5 to 0.6 upgrade - we need to add the region entry
      CGUIDialogProgress *dialog = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      if (dialog)
      {
        dialog->SetHeading("Updating old database version");
        dialog->SetLine(0, L"");
        dialog->SetLine(1, L"");
        dialog->SetLine(2, L"");
        dialog->StartModal(m_gWindowManager.GetActiveWindow());
        dialog->SetLine(1, L"Adding table entries");
        dialog->Progress();
        dialog->Progress();
      }
      BeginTransaction();
      CLog::Log(LOGINFO, "Creating temporary files table");
      m_pDS->exec("CREATE TABLE tempfiles ( idFile integer primary key, idPath integer, strFilename text, titleId text, xbedescription text, iTimesPlayed integer, lastAccessed integer)\n");
      CLog::Log(LOGINFO, "Copying files into temporary files table");
      m_pDS->exec("INSERT INTO tempfiles SELECT idFile,idPath,strFilename,titleId,xbedescription,iTimesPlayed,lastAccessed FROM files");
      CLog::Log(LOGINFO, "Destroying old files table");
      m_pDS->exec("DROP TABLE files");
      CLog::Log(LOGINFO, "Creating new files table");
      m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, strFilename text, titleId text, xbedescription text, iTimesPlayed integer, lastAccessed integer, iRegion integer)\n");
      CLog::Log(LOGINFO, "Copying files into new files table");
      m_pDS->exec("INSERT INTO files(idFile,idPath,strFilename,titleId,xbedescription,iTimesPlayed,lastAccessed) SELECT * FROM tempfiles");
      CLog::Log(LOGINFO, "Deleting temporary files table");
      m_pDS->exec("DROP TABLE tempfiles");

      CStdString strSQL=FormatSQL("update files set iRegion=%i",-1);
      m_pDS->exec(strSQL.c_str());
      
      CommitTransaction();
      if (dialog) dialog->Close();
    }
    if (fVersion < 0.7f)
    { // Version 0.6 to 0.7 update - need to create the trainers table
      CLog::Log(LOGINFO,"Creating trainers table");    
      m_pDS->exec("CREATE TABLE trainers (idKey integer auto_increment primary key, idCRC integer, idTitle integer, strTrainerPath text, strSettings text, Active integer)\n");
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Error attempting to update the database version!");
    return false;
  }
  return true;
}

int CProgramDatabase::GetRegion(const CStdString& strFilenameAndPath)
{
  CStdString strPath;
  CUtil::GetDirectory(strFilenameAndPath, strPath);
  strPath.Replace("\\", "/");

  if (NULL == m_pDB.get()) return 0;
  if (NULL == m_pDS.get()) return 0;

  CStdString strSQL=FormatSQL("select * from files,path where files.idPath=path.idPath and path.strPath='%s'", strPath.c_str());
  if (!m_pDS->query(strSQL.c_str())) 
    return 0;

  int iRowsFound = m_pDS->num_rows();
  if (iRowsFound == 0)
  {
    m_pDS->close();
    return 0;
  }
  int iRegion = m_pDS->fv("files.iRegion").get_asLong();
  m_pDS->close();

  return iRegion;
}

bool CProgramDatabase::SetRegion(const CStdString& strFileName, int iRegion)
{
  try
  {
    CStdString strPath;
    CUtil::GetDirectory(strFileName, strPath);
    strPath.Replace("\\", "/");

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from files,path where files.idPath=path.idPath and path.strPath='%s'", strPath.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int idFile = m_pDS->fv("files.idFile").get_asLong();
    m_pDS->close();

    CLog::Log(LOGDEBUG, "CProgramDatabase::SetRegion(%s), idFile=%i, region=%i",
              strFileName.c_str(), idFile,iRegion);

    strSQL=FormatSQL("update files set iRegion=%i where idFile=%i",
                  iRegion, idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:SetDescription(%s) failed", strFileName.c_str());
  }

  return false;
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
    strPath.Replace("\\", "/");
    strFileName.Replace("\\", "/");
    CStdString strSQL=FormatSQL("select * from path,files where path.idPath=files.idPath and path.strPath like '%s' and files.strFileName like '%s'", strPath.c_str(), strFileName.c_str());
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
      if (CFile::Exists(strPathandFile))
      {
        CFileItem *pItem = new CFileItem(m_pDS->fv("files.xbedescription").get_asString());
        pItem->m_iprogramCount = m_pDS->fv("files.iTimesPlayed").get_asLong();
        pItem->m_strPath = strPathandFile;
        pItem->m_strTitle=m_pDS->fv("files.xbedescription").get_asString();
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

    CStdString strSQL=FormatSQL("select path.strPath, files.strFilename from files join path on files.idPath=path.idPath where files.titleId='%x'", titleId);
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

long CProgramDatabase::AddFile(long lPathId, const CStdString& strFileName , DWORD titleId, const CStdString& strDescription, int iRegion)
{
  CStdString strSQL = "";
  try
  {
    long lFileId;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    strSQL=FormatSQL("select * from files where idPath=%i and strFileName like '%s'", lPathId, strFileName.c_str());
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
    strSQL=FormatSQL("insert into files (idFile, idPath, strFileName, titleId, xbedescription, iTimesPlayed, lastAccessed, iRegion) values(NULL, %i, '%s', '%x','%s',%i,%I64u,%i)", lPathId, strFileName.c_str(), titleId, strDescription.c_str(), 0, lastAccessed, iRegion);
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

bool CProgramDatabase::ItemHasTrainer(unsigned int iTitleId)
{
  CStdString strSQL;
  try
  {
    strSQL = FormatSQL("select * from trainers where idTitle=%u",iTitleId);
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    if (m_pDS->num_rows())
      return true;

    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"error checking for title's trainers (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::HasTrainer(const CStdString& strTrainerPath)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    strSQL = FormatSQL("select * from trainers where idCRC=%u",crc);
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    if (m_pDS->num_rows())
      return true;

    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"error checking for trainer existance (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::AddTrainer(int iTitleId, const CStdString& strTrainerPath)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    char temp[101];
    for( int i=0;i<100;++i)
      temp[i] = '0';
    temp[100] = '\0';
    strSQL=FormatSQL("insert into trainers (idKey,idCRC,idTitle,strTrainerPath,strSettings,Active) values(NULL,%u,%u,'%s','%s',%i)",crc,iTitleId,strTrainerPath.c_str(),temp,0);
    if (!m_pDS->exec(strSQL.c_str()))
      return false;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: unable to add trainer (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::RemoveTrainer(const CStdString& strTrainerPath)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    strSQL=FormatSQL("delete from trainers where idCRC=%u",crc);
    if (!m_pDS->exec(strSQL.c_str()))
      return false;

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: unable to remove trainer (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::GetTrainers(unsigned int iTitleId, std::vector<CStdString>& vecTrainers)
{
  vecTrainers.clear();
  CStdString strSQL;
  try 
  {
    strSQL = FormatSQL("select * from trainers where idTitle=%u",iTitleId);
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    
    while (!m_pDS->eof())
    {
      vecTrainers.push_back(m_pDS->fv("strTrainerPath").get_asString());
      m_pDS->next();
    }
    
    
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: error reading trainers for %i (%s)",iTitleId,strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::GetAllTrainers(std::vector<CStdString>& vecTrainers)
{
  vecTrainers.clear();
  CStdString strSQL;
  try 
  {
    strSQL = FormatSQL("select * from trainers");
    if (!m_pDS->query(strSQL.c_str()))
      return false;
    
    while (!m_pDS->eof())
    {
      vecTrainers.push_back(m_pDS->fv("strTrainerPath").get_asString());
      m_pDS->next();
    }
    
    
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: error reading trainers (%s)",strSQL.c_str());
  }
  return false;
}

bool CProgramDatabase::SetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    char temp[101];
    for (int i=0;i<100;++i)
    {
      if (data[i] == 1)
        temp[i] = '1';
      else
        temp[i] = '0';
    }
    temp[100] = '\0';

    strSQL = FormatSQL("update trainers set strSettings='%s' where idCRC=%u and idTitle=%u",temp,crc,iTitleId);
    if (m_pDS->exec(strSQL.c_str()))
      return true;

    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"CProgramDatabase::SetTrainerOptions failed (%s)",strSQL);
  }
  
  return false;
}

void CProgramDatabase::SetTrainerActive(const CStdString& strTrainerPath, unsigned int iTitleId, bool bActive)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    strSQL = FormatSQL("update trainers set Active=%u where idCRC=%u and idTitle=%u",bActive?1:0,crc,iTitleId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"CProgramDatabase::SetTrainerOptions failed (%s)",strSQL);
  }
}

CStdString CProgramDatabase::GetActiveTrainer(unsigned int iTitleId)
{
  CStdString strSQL;
  try 
  {
    strSQL = FormatSQL("select * from trainers where idTitle=%u and Active=1",iTitleId);
    if (!m_pDS->query(strSQL.c_str()))
      return "";
    
    if (!m_pDS->eof())
      return m_pDS->fv("strTrainerPath").get_asString();
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"programdatabase: error finding active trainer for %i (%s)",iTitleId,strSQL.c_str());
  }
  
  return "";
}

bool CProgramDatabase::GetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data)
{
  CStdString strSQL;
  Crc32 crc; crc.ComputeFromLowerCase(strTrainerPath);
  try
  {
    strSQL = FormatSQL("select * from trainers where idCRC=%u and idTitle=%u",crc,iTitleId);
    if (m_pDS->query(strSQL.c_str()))
    {
      CStdString strSettings = m_pDS->fv("strSettings").get_asString();
      for (int i=0;i<100;++i)
        data[i] = strSettings[i]=='1'?1:0;

      return true;
    }
    
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR,"CProgramDatabase::GetTrainerOptions failed (%s)",strSQL);
  }
  
  return false;
}

//********************************************************************************************************************************
long CProgramDatabase::AddBookMark(const CStdString& strBookmark)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strSQL=FormatSQL("select * from bookmark where bookmarkName='%s'", strBookmark.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      strSQL=FormatSQL("insert into bookmark (idBookmark, bookMarkname) values (NULL, '%s')", strBookmark.c_str());
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
    CStdString strSQL=FormatSQL("select * from path where strPath like '%s'", strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL=FormatSQL("insert into Path (idPath, strPath) values( NULL, '%s')",
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
bool CProgramDatabase::EntryExists(const CStdString& strPath1, const CStdString& strBookmark)
{
  try
  {
    CStdString strPath = strPath1;
    strPath.Replace("\\", "/");
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    long lPathId = GetPath(strPath);
    long lBookmarkId = AddBookMark(strBookmark);
    CStdString strSQL=FormatSQL("select * from program where idPath=%i and idBookmark=%i", lPathId, lBookmarkId);
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
    CLog::Log(LOGERROR, "CProgramDatabase::EntryExists(%s,%s) failed", strPath1.c_str(), strBookmark.c_str());
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
    CStdString strSQL=FormatSQL("select strPath from path,program where program.idBookmark=%i and program.idPath=path.idPath", lBookMarkId);
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

    CStdString strSQL=FormatSQL("select * from path where strPath like '%s' ", strPath.c_str());
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

    CStdString strSQL=FormatSQL("select idProgram from program where idPath=%i", lPathId);
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
    CStdString strPath, strFileName;
    Split(strFilenameAndPath, strPath, strFileName);
    strPath.Replace("\\", "/");
    strFileName.Replace("\\", "/");

    long lPathId = GetPath(strPath);

    int iRegion = 0;
    if (g_guiSettings.GetBool("MyPrograms.GameAutoRegion"))
    {
      CXBE xbe;
      iRegion = xbe.ExtractGameRegion(strFilenameAndPath);
      if (iRegion < 1 || iRegion > 7)
        iRegion = 0;
    }

    if (!EntryExists(strPath, strBookmark))
    {
      lPathId = AddPath(strPath);
      if (lPathId < 0) return -1;
      long lBookMarkId = AddBookMark(strBookmark);
      if (lBookMarkId < 0) return -1;
      CStdString strSQL=FormatSQL("insert into program (idProgram, idPath, idBookmark) values( NULL, %i, %i)",
                    lPathId, lBookMarkId);
      m_pDS->exec(strSQL.c_str());
      long lProgramId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      AddFile(lPathId, strFileName, titleId, strDescription, iRegion);
    }
    else
    {
      long lProgramId = GetProgram(lPathId);
      AddFile(lPathId, strFileName, titleId, strDescription,iRegion);
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
      strSQL=FormatSQL("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path,program where program.idBookmark=%i and program.idPath=path.idPath and files.idPath=program.idPath and files.strFilename like '/default.xbe'", lBookmarkId);
    else
      strSQL=FormatSQL("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path,program where program.idBookmark=%i and program.idPath=path.idPath and files.idPath=program.idPath", lBookmarkId);
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
      pItem->m_strTitle=m_pDS->fv("files.xbedescription").get_asString();
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

void CProgramDatabase::GetProgramsByPath(const CStdString& strPath1, CFileItemList& programs, int iDepth, bool bOnlyDefaultXBE)
{
  try
  {
    VECPROGRAMPATHS todelete;
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    CStdString strSQL;
    CStdString strPath = strPath1;
    strPath.Replace("\\", "/");
    CStdString strShortCutsDir = g_stSettings.m_szShortcutDirectory;
    strShortCutsDir.Replace("\\", "/");
    if (bOnlyDefaultXBE)
    {
      strSQL=FormatSQL("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path where files.idPath=path.idPath and path.strPath like '%s/%%' and files.strFilename like '/default.xbe'", strPath.c_str());
    }
    else
    {
      if (strPath.c_str() == strShortCutsDir)
        strSQL=FormatSQL("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path where files.idPath=path.idPath and path.strPath like '%s'", strPath.c_str());
      else
        strSQL=FormatSQL("select strPath,strFilename,xbedescription,iTimesPlayed,lastAccessed from files,path where files.idPath=path.idPath and path.strPath like '%s/%%'", strPath.c_str());
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
        pItem->m_strTitle=m_pDS->fv("files.xbedescription").get_asString();
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

    CStdString strSQL=FormatSQL("delete from files where idFile=%i", lFileId);
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


    CStdString strSQL=FormatSQL("delete from files where idpath=%i", lPathId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from program where idpath=%i", lPathId);
    m_pDS->exec(strSQL.c_str());

    strSQL=FormatSQL("delete from path where idpath=%i", lPathId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::DeleteProgram() failed");
  }
}

bool CProgramDatabase::IncTimesPlayed(const CStdString& strFileName)
{
  try
  {
    CStdString strPath;
    CUtil::GetDirectory(strFileName, strPath);
    strPath.Replace("\\", "/");

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from files,path where files.idPath=path.idPath and path.strPath='%s'", strPath.c_str());
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
              strFileName.c_str(), idFile, iTimesPlayed);

    strSQL=FormatSQL("update files set iTimesPlayed=%i where idFile=%i",
                  ++iTimesPlayed, idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:IncTimesPlayed(%s) failed", strFileName.c_str());
  }

  return false;
}

bool CProgramDatabase::SetDescription(const CStdString& strFileName, const CStdString& strDescription)
{
  try
  {
    CStdString strPath;
    CUtil::GetDirectory(strFileName, strPath);
    strPath.Replace("\\", "/");

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select * from files,path where files.idPath=path.idPath and path.strPath='%s'", strPath.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int idFile = m_pDS->fv("files.idFile").get_asLong();
    m_pDS->close();

    CLog::Log(LOGDEBUG, "CProgramDatabase::SetDescription(%s), idFile=%i, description=%s",
              strFileName.c_str(), idFile,strDescription.c_str());

    strSQL=FormatSQL("update files set xbedescription='%s' where idFile=%i",
                  strDescription.c_str(), idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:SetDescription(%s) failed", strFileName.c_str());
  }

  return false;
}
