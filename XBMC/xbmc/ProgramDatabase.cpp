//********************************************************************************************************************************
//**
//** see docs/videodatabase.png for a diagram of the database
//**
//********************************************************************************************************************************

#include "stdafx.h"
#include ".\programdatabase.h"
#include "settings.h"
#include "utils/fstrcmp.h"
#include "utils/log.h"
#include "util.h"

//********************************************************************************************************************************
CProgramDatabase::CProgramDatabase(void)
{
}

//********************************************************************************************************************************
CProgramDatabase::~CProgramDatabase(void)
{
}

//********************************************************************************************************************************
void CProgramDatabase::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
	strFileName="";
	strPath="";
	int i=strFileNameAndPath.size()-1;
	while (i > 0)
	{
		char ch=strFileNameAndPath[i];
		if (ch==':' || ch=='/' || ch=='\\') break;
		else i--;
	}
	strPath     = strFileNameAndPath.Left(i);
	strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i);
}

//********************************************************************************************************************************
void CProgramDatabase::RemoveInvalidChars(CStdString& strTxt)
{
	CStdString strReturn="";
	for (int i=0; i < (int)strTxt.size(); ++i)
	{
		byte k=strTxt[i];
		if (k==0x27) 
		{
			strReturn += k;
		}
		strReturn += k;
	}
	if (strReturn=="") 
		strReturn="unknown";
	strTxt=strReturn;
}



//********************************************************************************************************************************
bool CProgramDatabase::Open()
{
	CStdString programDatabase=g_stSettings.m_szAlbumDirectory;
	programDatabase+="\\MyPrograms3.db";

	Close();

	// test id dbs already exists, if not we need 2 create the tables
	bool bDatabaseExists=false;
	FILE* fd= fopen(programDatabase.c_str(),"rb");
	if (fd)
	{
		bDatabaseExists=true;
		fclose(fd);
	}

	m_pDB.reset(new SqliteDatabase() ) ;
	m_pDB->setDatabase(programDatabase.c_str());

	m_pDS.reset(m_pDB->CreateDataset());
	if ( m_pDB->connect() != DB_CONNECTION_OK) 
	{
		CLog::Log("programdatabase::unable to open %s",programDatabase.c_str());
		Close();
		::DeleteFile(programDatabase.c_str());
		return false;
	}

	if (!bDatabaseExists) 
	{
		if (!CreateTables()) 
		{
			CLog::Log("programdatabase::unable to create %s",programDatabase.c_str());
			Close();
			::DeleteFile(programDatabase.c_str());
			return false;
		}
	}

	m_pDS->exec("PRAGMA cache_size=8192\n");
	m_pDS->exec("PRAGMA synchronous='OFF'\n");
	m_pDS->exec("PRAGMA count_changes='OFF'\n");
	//	m_pDS->exec("PRAGMA temp_store='MEMORY'\n");
	return true;
}


//********************************************************************************************************************************
void CProgramDatabase::Close()
{
	if (NULL==m_pDB.get() ) return;
	m_pDB->disconnect();
	m_pDB.reset();
}

//********************************************************************************************************************************
bool CProgramDatabase::CreateTables()
{

	try 
	{
		CLog::Log("create program table");
		m_pDS->exec("CREATE TABLE program ( idProgram integer primary key, idPath integer, idBookmark integer)\n");

		CLog::Log("create bookmark table");
		m_pDS->exec("CREATE TABLE bookmark (idBookmark integer primary key, bookmarkName text)\n");

		CLog::Log("create path table");
		m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, strBookmarkDir text)\n");

		CLog::Log("create files table");
		m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, idProgram integer,strFilename text, xbedescription text, iTimesPlayed integer)\n");
	
		CLog::Log("create bookmark index");
		m_pDS->exec("CREATE INDEX idxBookMark ON bookmark(bookmarkName)");
		CLog::Log("create path index");
		m_pDS->exec("CREATE INDEX idxPath ON path(strPath)");
		CLog::Log("create files index");
		m_pDS->exec("CREATE INDEX idxFiles ON files(strFilename)");
	
	}
	catch (...) 
	{ 
		CLog::Log("programdatabase::unable to create tables:%i",GetLastError());
		return false;
	}

	return true;
}

//********************************************************************************************************************************
long CProgramDatabase::AddFile(long lProgramId, long lPathId, const CStdString& strFileName, const CStdString& strDescription)
{
	CStdString strSQL="";
  try
  {
    long lFileId;
    if (NULL==m_pDB.get()) return -1;
	  if (NULL==m_pDS.get()) return -1;

    strSQL.Format("select * from files where idProgram=%i and idPath=%i and strFileName like '%s'",lProgramId,lPathId,strFileName.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() > 0) 
    {
      lFileId=m_pDS->fv("idFile").get_asLong() ;
      return lFileId;
    }
	  strSQL.Format ("insert into files (idFile, idProgram,idPath, strFileName, xbedescription, iTimesPlayed) values(NULL, %i,%i,'%s',\"%s\",%i)", lProgramId,lPathId, strFileName.c_str(),strDescription.c_str(),0);
	  m_pDS->exec(strSQL.c_str());
    lFileId=sqlite_last_insert_rowid( m_pDB->getHandle() );
    return lFileId;
  }
  catch(...)
  {
    CLog::Log("programdatabase:unable to addfile (%s)", strSQL.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
long CProgramDatabase::AddBookMark(const CStdString& strBookmark)
{
	try
	{
		if (NULL==m_pDB.get()) return -1;
		if (NULL==m_pDS.get()) return -1;
		CStdString strSQL;
		strSQL.Format("select * from bookmark where bookmarkName='%s'", strBookmark.c_str());
		m_pDS->query(strSQL.c_str());
		if (m_pDS->num_rows() == 0)
		{
			strSQL.Format("insert into bookmark (idBookmark, bookMarkname) values (NULL, '%s')", strBookmark.c_str());
			m_pDS->exec(strSQL.c_str());
			long lBookmarkId=sqlite_last_insert_rowid(m_pDB->getHandle());
			return lBookmarkId;
		}
		else
		{
			const field_value value = m_pDS->fv("idBookmark");
			long lBookmarkId=value.get_asLong();
			return lBookmarkId;
		}
	}
	catch(...)
	{
		CLog::Log("CProgramDatabase::AddBookMark(%s) failed",strBookmark.c_str());
	}
	return -1;
}


//********************************************************************************************************************************
long CProgramDatabase::AddPath(const CStdString& strPath, const CStdString& strBookmarkDir)
{
  try
  {
    if (NULL==m_pDB.get()) return -1;
    if (NULL==m_pDS.get()) return -1;
    CStdString strSQL;
    strSQL.Format("select * from path where strPath like '%s'",strPath.c_str());
    m_pDS->query(strSQL.c_str());
    if (m_pDS->num_rows() == 0) 
    {
	  // doesnt exists, add it
	  strSQL.Format("insert into Path (idPath, strPath, strBookmarkDir) values( NULL, '%s', '%s')",
                    strPath.c_str(), strBookmarkDir.c_str());
	  m_pDS->exec(strSQL.c_str());
	  long lPathId=sqlite_last_insert_rowid(m_pDB->getHandle());
	  return lPathId;
    }
    else
    {
 	  const field_value value = m_pDS->fv("idPath");
	  long lPathId=value.get_asLong() ;
	  return lPathId;
    }

  }
  catch(...)
  {
    CLog::Log("CProgramDatabase::AddPath(%s) failed",strPath.c_str());
  }
	return -1;
}

//********************************************************************************************************************************
long CProgramDatabase::GetPath(const CStdString& strPath)
{
  try
  {
	if (NULL==m_pDB.get()) return -1;
	if (NULL==m_pDS.get()) return -1;
	CStdString strSQL;
    strSQL.Format("select * from path where strPath like '%s' ",strPath.c_str());
	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() > 0) 
	{
		long lPathId = m_pDS->fv("idPath").get_asLong();
		return lPathId;
	}

  }
  catch(...)
  {
    CLog::Log("CProgramDatabase::GetPath(%s) failed",strPath.c_str());
  }
	return -1;
}

//********************************************************************************************************************************
long CProgramDatabase::GetProgram(long lPathId)
{
	try
	{
		if (NULL==m_pDB.get()) return -1;
		if (NULL==m_pDS.get()) return -1;
		CStdString strSQL;
		strSQL.Format("select idProgram from program where idPath=%i",lPathId);
		m_pDS->query(strSQL.c_str());
		if (m_pDS->num_rows() > 0) {
			long lProgramId=m_pDS->fv("idProgram").get_asLong();
			return lProgramId;
		}
		
	}
	catch(...)
	{
		CLog::Log("CProgramDatabase::GetProgram(%i) failed",lPathId);
	}
	return -1;
}

//********************************************************************************************************************************
long CProgramDatabase::AddProgram(const CStdString& strFilenameAndPath, const CStdString& strBookmarkDir, const CStdString& strDescription, const CStdString& strBookmark)
{
  try
  {
    if (NULL==m_pDB.get()) return -1;
	if (NULL==m_pDS.get()) return -1;
	CStdString strPath, strFileName, strDes=strDescription, strBookmarkPath=strBookmarkDir;
	Split(strFilenameAndPath, strPath, strFileName); 
    RemoveInvalidChars(strPath);
    RemoveInvalidChars(strFileName);
    RemoveInvalidChars(strDes);
	RemoveInvalidChars(strBookmarkPath);
	strPath.Replace("\\","/");
	strFileName.Replace("\\","/");
	strBookmarkPath.Replace("\\","/");

	
	long lPathId=GetPath(strPath);
	
    if (lPathId < 0)
    {
	  
	  CStdString strSQL;

      lPathId = AddPath(strPath, strBookmarkPath);
      if (lPathId < 0) return -1;
	  long lBookMarkId = AddBookMark(strBookmark);
	  if (lBookMarkId < 0) return -1;
      strSQL.Format("insert into program (idProgram, idPath, idBookmark) values( NULL, %i, %i)",
                    lPathId, lBookMarkId);
	  m_pDS->exec(strSQL.c_str());
      long lProgramId=sqlite_last_insert_rowid(m_pDB->getHandle());
      AddFile(lProgramId,lPathId,strFileName,strDes);
    }
    else
    {
	  long lProgramId=GetProgram(lPathId);
      AddFile(lProgramId,lPathId,strFileName,strDes);
	  return lProgramId;
    }
    
  }
  catch(...)
  {
    CLog::Log("CProgramDatabase::AddProgram(%s,%s) failed",strFilenameAndPath.c_str() , strDescription.c_str() );
  }
  return -1;
}

//********************************************************************************************************************************
void CProgramDatabase::GetProgramsByBookmark(CStdString& strBookmark, VECFILEITEMS& programs, CStdString& strBookmarkDir, bool bOnlyDefaultXBE, bool bOnlyOnePath)
{
	try {	
		VECPROGRAMPATHS todelete;
		FILETIME localTime;
		programs.erase(programs.begin(),programs.end());
		if (NULL==m_pDB.get()) return ;
		if (NULL==m_pDS.get()) return ;
		long lBookmarkId = AddBookMark(strBookmark);
		CStdString strSQL;
		CStdString strBookmarkPath=strBookmarkDir;
		strBookmarkPath.Replace("\\", "/");
		if (bOnlyDefaultXBE)
		{
			if (!bOnlyOnePath)
				strSQL.Format("select * from program,files,path where program.idBookmark=%i and program.idPath=path.idPath and files.idPath=program.idPath and files.strFilename like '/default.xbe'",lBookmarkId);
			else
				strSQL.Format("select * from program,files,path where program.idBookmark=%i and program.idPath=path.idPath and files.idPath=program.idPath and path.strBookmarkDir like '%s' and files.strFilename like '/default.xbe'",lBookmarkId, strBookmarkPath.c_str());
		} 
		else
		{
			if (!bOnlyOnePath)
				strSQL.Format("select * from program,files,path where program.idBookmark=%i and program.idPath=path.idPath and files.idPath=program.idPath",lBookmarkId);
			else
				strSQL.Format("select * from program,files,path where program.idBookmark=%i and program.idPath=path.idPath and files.idPath=program.idPath and path.strBookmarkDir like '%s'",lBookmarkId, strBookmarkPath.c_str());
		}
		m_pDS->query(strSQL.c_str());
		if (m_pDS->num_rows() == 0) return;
		while (!m_pDS->eof())
		{
			WIN32_FILE_ATTRIBUTE_DATA FileAttributeData;
			CStdString strPath,strFile,strPathandFile;
			strPath=m_pDS->fv("path.strPath").get_asString();
			strFile=m_pDS->fv("files.strFilename").get_asString();
			strPathandFile=strPath+strFile;
			strPathandFile.Replace("/","\\");
			if (CUtil::FileExists(strPathandFile))
			{
				CFileItem *pItem = new CFileItem(m_pDS->fv("files.xbedescription").get_asString());
				pItem->m_strPath=strPathandFile;
				pItem->m_bIsFolder=false;
				GetFileAttributesEx(pItem->m_strPath, GetFileExInfoStandard, &FileAttributeData);
				FileTimeToLocalFileTime(&FileAttributeData.ftLastWriteTime,&localTime);
				FileTimeToSystemTime(&localTime, &pItem->m_stTime);
				pItem->m_dwSize=FileAttributeData.nFileSizeLow;
				programs.push_back(pItem);
			}
			else
			{
				todelete.push_back(strPath); // push the paths that we need to delete and delete later
			}
			m_pDS->next();
		}
	
		// let's now delete the program from the database since it no longer exists.. better way to do this?

		for (int i=0; i < (int)todelete.size(); ++i)
		{
			CStdString& pathtodelete = todelete[i];
			DeleteProgram(pathtodelete);
		}

	}
	catch(...)
	{
		CLog::Log("CProgramDatabase::GetProgamsByBookmark() failed");
	}

}

//********************************************************************************************************************************
void CProgramDatabase::DeleteProgram(const CStdString& strPath)
{
  try
  {
	  if (NULL==m_pDB.get()) return ;
	  if (NULL==m_pDS.get()) return ;
   
	  long lPathId=GetPath(strPath);
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
  catch(...)
  {
    CLog::Log("CProgramDatabase::DeleteProgram() failed");
  }
}

