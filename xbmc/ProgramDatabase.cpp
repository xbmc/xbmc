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

#include "stdafx.h"
#include "ProgramDatabase.h"
#include "FileSystem/MultiPathDirectory.h"
#include "utils/fstrcmp.h"
#include "Util.h"
#include "GUIWindowFileManager.h"
#include "VideoInfoScanner.h"
#include "FileItem.h"
#include "GUISettings.h"
#include "Settings.h"

using namespace XFILE;

#define PROGRAM_DATABASE_OLD_VERSION 0.9f
#define PROGRAM_DATABASE_VERSION 3

//********************************************************************************************************************************
CProgramDatabase::CProgramDatabase(void)
{
  m_preV2version=PROGRAM_DATABASE_OLD_VERSION;
  m_version=PROGRAM_DATABASE_VERSION;
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

    CLog::Log(LOGINFO, "create files table");
    m_pDS->exec("CREATE TABLE files ( idFile integer primary key, strFilename text, titleId integer, xbedescription text, iTimesPlayed integer, lastAccessed integer, iRegion integer, iSize integer)\n");
    CLog::Log(LOGINFO, "create files index");
    m_pDS->exec("CREATE INDEX idxFiles ON files(strFilename)");
    CLog::Log(LOGINFO, "create files - titleid index");
    m_pDS->exec("CREATE INDEX idxTitleIdFiles ON files(titleId)");
    CLog::Log(LOGINFO, "create titles table");

    m_pDS->exec("CREATE TABLE titles ( idTitle integer primary key, strTitle text, strDescription text, iType integer, strGenre text, strStyle text, strPublisher text, strDateOfRelease text, strYear text, thumbURL text, fanartURL text )\n");
    CLog::Log(LOGINFO, "create titles index");
    m_pDS->exec("CREATE INDEX idxTitles ON titles(idTitle)");

    CLog::Log(LOGINFO, "create path table");
    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, strContent text, strScraper text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX idxPath ON path ( strPath )\n");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "programdatabase::unable to create tables:%u",
              GetLastError());
    return false;
  }

  return true;
}

bool CProgramDatabase::UpdateOldVersion(int version)
{
  if (NULL == m_pDB.get()) return false;
  if (NULL == m_pDS.get()) return false;
  if (NULL == m_pDS2.get()) return false;

  try
  {
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Error attempting to update the database version!");
    return false;
  }
  return true;
}

DWORD CProgramDatabase::GetTitleId(const CStdString& strFilenameAndPath)
{
  if (NULL == m_pDB.get()) return 0;
  if (NULL == m_pDS.get()) return 0;

  try
  {
    CStdString strSQL = FormatSQL("select * from files where files.strFileName like '%s'", strFilenameAndPath.c_str());
    if (!m_pDS->query(strSQL.c_str()))
      return 0;

    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return 0;
    }
    DWORD dwTitleId = m_pDS->fv("files.TitleId").get_asLong();
    m_pDS->close();
    return dwTitleId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:GetTitleId(%s) failed", strFilenameAndPath.c_str());
  }
  return 0;
}

bool CProgramDatabase::SetTitleId(const CStdString& strFileName, DWORD dwTitleId)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = FormatSQL("select * from files where files.strFileName like '%s'", strFileName.c_str());
    if (!m_pDS->query(strSQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false;
    }
    int idFile = m_pDS->fv("files.idFile").get_asLong();
    m_pDS->close();

    CLog::Log(LOGDEBUG, "CProgramDatabase::SetTitle(%s), idFile=%i, region=%u",
              strFileName.c_str(), idFile,dwTitleId);

    strSQL=FormatSQL("update files set titleId=%u where idFile=%i",
                  dwTitleId, idFile);
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase:SetDescription(%s) failed", strFileName.c_str());
  }

  return false;
}

bool CProgramDatabase::GetXBEPathByTitleId(const DWORD titleId, CStdString& strPathAndFilename)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("select files.strFilename from files where files.titleId=%u", titleId);
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0)
    {
      strPathAndFilename = m_pDS->fv("files.strFilename").get_asString();
      strPathAndFilename.Replace('/', '\\');
      m_pDS->close();
      return true;
    }
    m_pDS->close();
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetXBEPathByTitleId(%u) failed",
              titleId);
  }
  return false;
}

DWORD CProgramDatabase::GetProgramInfo(CFileItem *item)
{
  DWORD titleID = 0;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = FormatSQL("select xbedescription,iTimesPlayed,lastAccessed,titleId,iSize from files where strFileName like '%s'", item->m_strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
    { // get info - only set the label if not preformatted
      if (!item->IsLabelPreformated())
        item->SetLabel(m_pDS->fv("xbedescription").get_asString());
      item->m_iprogramCount = m_pDS->fv("iTimesPlayed").get_asLong();
      item->m_strTitle = item->GetLabel();  // is this needed?
      item->m_dateTime = TimeStampToLocalTime(_atoi64(m_pDS->fv("lastAccessed").get_asString().c_str()));
      item->m_dwSize = _atoi64(m_pDS->fv("iSize").get_asString().c_str());
      titleID = m_pDS->fv("titleId").get_asLong();
      if (item->m_dwSize == -1)
      {
        CStdString strPath;
        CUtil::GetDirectory(item->m_strPath,strPath);
        __int64 iSize = CGUIWindowFileManager::CalculateFolderSize(strPath);
        CStdString strSQL=FormatSQL("update files set iSize=%I64u where strFileName like '%s'",iSize,item->m_strPath.c_str());
        m_pDS->exec(strSQL.c_str());
      }
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::GetProgramInfo(%s) failed", item->m_strPath.c_str());
  }
  return titleID;
}

bool CProgramDatabase::AddProgramInfo(CFileItem *item, unsigned int titleID)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int iRegion = -1;
    if (g_guiSettings.GetBool("myprograms.gameautoregion"))
    {
        iRegion = 0;
    }
    FILETIME time;
    item->m_dateTime=CDateTime::GetCurrentDateTime();
    item->m_dateTime.GetAsTimeStamp(time);

    ULARGE_INTEGER lastAccessed;
    lastAccessed.u.LowPart = time.dwLowDateTime; 
    lastAccessed.u.HighPart = time.dwHighDateTime;

    CStdString strPath, strParent;
    CUtil::GetDirectory(item->m_strPath,strPath);
    // special case - programs in root of sources
    bool bIsShare=false;
    CUtil::GetMatchingSource(strPath,g_settings.m_programSources,bIsShare);
    __int64 iSize=0;
    if (bIsShare || !item->IsDefaultXBE())
    {
      struct __stat64 stat;
      if (CFile::Stat(item->m_strPath,&stat) == 0)
        iSize = stat.st_size;
    }
    else
      iSize = CGUIWindowFileManager::CalculateFolderSize(strPath);
    if (titleID == 0)
      titleID = (unsigned int) -1;
    CStdString strSQL=FormatSQL("insert into files (idFile, strFileName, titleId, xbedescription, iTimesPlayed, lastAccessed, iRegion, iSize) values(NULL, '%s', %u, '%s', %i, %I64u, %i, %I64u)", item->m_strPath.c_str(), titleID, item->GetLabel().c_str(), 0, lastAccessed.QuadPart, iRegion, iSize);
    m_pDS->exec(strSQL.c_str());
    item->m_dwSize = iSize;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "CProgramDatabase::AddProgramInfo(%s) failed", item->m_strPath.c_str());
  }
  return true;
}

FILETIME CProgramDatabase::TimeStampToLocalTime( unsigned __int64 timeStamp )
{
  FILETIME fileTime;
  ::FileTimeToLocalFileTime( (const FILETIME *)&timeStamp, &fileTime);
  return fileTime;
}

bool CProgramDatabase::IncTimesPlayed(const CStdString& strFileName)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = FormatSQL("select * from files where files.strFileName like '%s'", strFileName.c_str());
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
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL = FormatSQL("select * from files where files.strFileName like '%s'", strFileName.c_str());
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

long CProgramDatabase::AddPath(const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    long lPathId;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strPath1(strPath);
    if (CUtil::IsStack(strPath) || strPath.Mid(0,6).Equals("rar://") || strPath.Mid(0,6).Equals("zip://"))
      CUtil::GetParentPath(strPath,strPath1);

    CUtil::AddSlashAtEnd(strPath1);

    strSQL=FormatSQL("insert into path (idPath, strPath, strContent, strScraper) values (NULL,'%s','','')", strPath1.c_str());
    m_pDS->exec(strSQL.c_str());
    lPathId = (long)sqlite3_last_insert_rowid( m_pDB->getHandle() );
    return lPathId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to addpath (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

long CProgramDatabase::GetPathId(const CStdString& strPath)
{
  CStdString strSQL;
  try
  {
    long lPathId=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString strPath1(strPath);
    if (CUtil::IsStack(strPath) || strPath.Mid(0,6).Equals("rar://") || strPath.Mid(0,6).Equals("zip://"))
      CUtil::GetParentPath(strPath,strPath1);

    CUtil::AddSlashAtEnd(strPath1);

    strSQL=FormatSQL("select idPath from path where strPath like '%s'",strPath1.c_str());
    m_pDS->query(strSQL.c_str());
    if (!m_pDS->eof())
      lPathId = m_pDS->fv("path.idPath").get_asLong();

    m_pDS->close();
    return lPathId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to getpath (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

void CProgramDatabase::SetScraperForPath(const CStdString& filePath, const SScraperInfo& info)
{
  // if we have a multipath, set scraper for all contained paths too
  if(CUtil::IsMultiPath(filePath))
  {
    std::vector<CStdString> paths;
    DIRECTORY::CMultiPathDirectory::GetPaths(filePath, paths);

    for(unsigned i=0;i<paths.size();i++)
      SetScraperForPath(paths[i],info);
  }

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    long lPathId = GetPathId(filePath);
    if (lPathId < 0)
    { // no path found - we have to add one
      lPathId = AddPath(filePath);
      if (lPathId < 0) return ;
    }

    // Update
    CStdString strSQL=FormatSQL("update path set strContent='%s',strScraper='%s' where idPath=%u", info.strContent.c_str(), info.strPath.c_str(), lPathId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filePath.c_str());
  }
}

bool CProgramDatabase::GetScraperForPath(const CStdString& strPath, SScraperInfo& info)
{
  try
  {
    info.Reset();
    if (strPath.IsEmpty()) return false;
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strPath1;
    CUtil::GetDirectory(strPath,strPath1);
    long lPathId = GetPathId(strPath1);

    // search wheter some of the parent folders has scraper info
    while (lPathId == -1)
    {
      CStdString lastPath = strPath1;
      CUtil::GetDirectory(strPath1.substr(0, strPath1.length()-1), strPath1);
      if (strPath1.Equals(lastPath)) break;     
      lPathId = GetPathId(strPath1);
    }

    if (lPathId > -1)
    {
      CStdString strSQL=FormatSQL("select path.strContent,path.strScraper from path where path.idPath=%u",lPathId);
      m_pDS->query( strSQL.c_str() );
    }
    else
      return false;

    if (!m_pDS->eof())
    {
      info.strContent = m_pDS->fv("path.strContent").get_asString();
      info.strPath = m_pDS->fv("path.strScraper").get_asString();
      CScraperParser parser;
      parser.Load("special://xbmc/system/scrapers/programs/" + info.strPath);
      info.strLanguage = parser.GetLanguage();
      info.strTitle = parser.GetName();
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

DWORD CProgramDatabase::AddTitle(CProgramInfoTag program)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;
    
    // insert the new program info
    CStdString strSQL=FormatSQL("insert into titles(idTitle, iType, strTitle, strDescription, strGenre, strStyle, strPublisher, strDateOfRelease, strYear, thumbURL, fanartURL) values (NULL,'%i', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')", 
      program.m_iType, program.m_strTitle.c_str(), program.m_strDescription.c_str(), program.m_strGenre.c_str(), program.m_strStyle.c_str(), program.m_strPublisher.c_str(), program.m_strDateOfRelease.c_str(), program.m_strYear.c_str(), program.m_thumbURL.m_xml.c_str(), program.m_fanart.m_xml.c_str());
    m_pDS->exec(strSQL.c_str());

    // return the generated id of it
    return (DWORD) sqlite3_last_insert_rowid( m_pDB->getHandle() );
   }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, program.m_strTitle.c_str());
    return 0;
  }
}

CProgramInfoTag CProgramDatabase::GetTitle(DWORD titleId)
{
  CProgramInfoTag result;
  try
  {
    if (NULL == m_pDB.get()) return result;
    if (NULL == m_pDS.get()) return result;
    

    // if doesn't, insert a new one
    CStdString strSQL=FormatSQL("select idTitle, iType, strTitle, strDescription, strGenre, strStyle, strPublisher, strDateOfRelease, strYear, thumbURL, fanartURL from titles where idTitle=%i", titleId);
    m_pDS->query(strSQL.c_str());

    if (!m_pDS->eof())
    {
      result.m_idProgram = m_pDS->fv("titles.idTitle").get_asLong();
      result.m_iType = m_pDS->fv("titles.iType").get_asInteger();
      result.m_strTitle = m_pDS->fv("titles.strTitle").get_asString();
      result.m_strDescription = m_pDS->fv("titles.strDescription").get_asString();
      result.m_strGenre = m_pDS->fv("titles.strGenre").get_asString();
      result.m_strStyle = m_pDS->fv("titles.strStyle").get_asString();
      result.m_strPublisher = m_pDS->fv("titles.strPublisher").get_asString();
      result.m_strDateOfRelease = m_pDS->fv("titles.strDateOfRelease").get_asString();
      result.m_strYear = m_pDS->fv("titles.strYear").get_asString();
      result.m_thumbURL = CScraperUrl(m_pDS->fv("titles.thumbURL").get_asString());
      result.m_fanart.m_xml = m_pDS->fv("titles.fanartURL").get_asString();
      result.m_fanart.Unpack();
    }
 }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%d) failed", __FUNCTION__, titleId);
  }
  return result;
}

void CProgramDatabase::RemoveTitle(DWORD titleId)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;
    
    // if doesn't, insert a new one
    CStdString strSQL=FormatSQL("delete from titles where idTitle=%i", titleId);
    m_pDS->exec(strSQL.c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%d) failed", __FUNCTION__, titleId);
  }
}

int CProgramDatabase::GetProgramsCount()
{
  return GetProgramsCount((CStdString)"");
}

int CProgramDatabase::GetProgramsCount(const CStdString& strWhere)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString strSQL = "select count(idTitle) as NumProgs from titles " + strWhere;
    if (!m_pDS->query(strSQL.c_str())) return false;
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      return 0;
    }

    int iNumSongs = m_pDS->fv(0).get_asLong();
    // cleanup
    m_pDS->close();
    return iNumSongs;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, strWhere.c_str());
  }
  return 0;
}

