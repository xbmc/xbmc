#include ".\videodatabase.h"

//********************************************************************************************************************************
CVideoDatabase::CVideoDatabase(void)
{
}

//********************************************************************************************************************************
CVideoDatabase::~CVideoDatabase(void)
{
}

//********************************************************************************************************************************
void CVideoDatabase::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
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
void CVideoDatabase::RemoveInvalidChars(CStdString& strTxt)
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
bool CVideoDatabase::Open()
{
	Close();

	// test id dbs already exists, if not we need 2 create the tables
	bool bDatabaseExists=false;
	FILE* fd= fopen("Q:\\albums\\MyVideos.db","rb");
	if (fd)
	{
		bDatabaseExists=true;
		fclose(fd);
	}

	m_pDB.reset(new SqliteDatabase() ) ;
  m_pDB->setDatabase("Q:\\albums\\MyVideos.db");
	
  m_pDS.reset(m_pDB->CreateDataset());
	if ( m_pDB->connect() != DB_CONNECTION_OK) 
	{
		Close();
		return false;
	}

	if (!bDatabaseExists) 
	{
		if (!CreateTables()) 
		{
			Close();
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
void CVideoDatabase::Close()
{
	if (NULL==m_pDB.get() ) return;
	m_pDB->disconnect();
	m_pDB.reset();
}

//********************************************************************************************************************************
bool CVideoDatabase::CreateTables()
{

  try 
	{
    m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idMovie integer, fPercentage text)\n");
		m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");
    m_pDS->exec("CREATE TABLE genrelinkmovie ( idGenre integer, idMovie integer)\n");
    m_pDS->exec("CREATE TABLE movie ( idMovie integer, idPath integer, hasSubtitles integer)\n");
    m_pDS->exec("CREATE TABLE path ( idPath integer, strPath text, strFilename text, cdlabel text )\n");
    m_pDS->exec("CREATE TABLE movieinfo ( idMovie integer, idDirector integer, strPlotOutline text, strPlot text, strTagLine text, iVotes int, fRating text )\n");
    m_pDS->exec("CREATE TABLE actorlinkmovie ( idActor integer, idMovie integer )\n");
    m_pDS->exec("CREATE TABLE actors ( idActor integer, strActor text )\n");
  }
  catch (...) 
	{ 
		return false;
	}

	return true;
}

//********************************************************************************************************************************
void CVideoDatabase::AddMovie(const CStdString& strFilenameAndPath, const CStdString& strcdLabel, bool bHassubtitles)
{
  if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
	CStdString strPath, strFileName, strCDLabel=strcdLabel;
	Split(strFilenameAndPath, strPath, strFileName); 
  RemoveInvalidChars(strPath);
  RemoveInvalidChars(strFileName);
  RemoveInvalidChars(strCDLabel);

  long lPathId = AddPath(strPath,strFileName,strCDLabel);
  if (lPathId>=0) return;
	
  CStdString strSQL;
  strSQL.Format("insert into movie (idMovie, idPath, hasSubtitles) values( NULL, %i, %i)",
	                      lPathId,bHassubtitles);
	m_pDS->exec(strSQL.c_str());
}

//********************************************************************************************************************************
long CVideoDatabase::AddPath(const CStdString& strPath, const CStdString& strFilename, const CStdString& strCdLabel)
{
  if (NULL==m_pDB.get()) return -1;
	if (NULL==m_pDS.get()) return -1;
	CStdString strSQL;
  strSQL.Format("select * from path where strPath like '%s' and strFilename like '%s' and cdlabel like '%s'",
	                  strPath,strFilename,strCdLabel);

	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() == 0) 
	{
		// doesnt exists, add it
		strSQL.Format("insert into Path (idPath, strPath,strFilename,cdlabel) values( NULL, '%s', '%s', '%s')",
	                  strPath,strFilename,strCdLabel);
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
	return -1;
}