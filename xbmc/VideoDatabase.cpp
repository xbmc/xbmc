//********************************************************************************************************************************
//**
//** see docs/videodatabase.png for a diagram of the database
//**
//********************************************************************************************************************************
#include ".\videodatabase.h"
#include "utils/fstrcmp.h"

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
	FILE* fd= fopen("Q:\\albums\\MyVideos1.db","rb");
	if (fd)
	{
		bDatabaseExists=true;
		fclose(fd);
	}

	m_pDB.reset(new SqliteDatabase() ) ;
  m_pDB->setDatabase("Q:\\albums\\MyVideos1.db");
	
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
    m_pDS->exec("CREATE TABLE bookmark ( idBookmark integer primary key, idFile integer, fPercentage text)\n");
		m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");
    m_pDS->exec("CREATE TABLE genrelinkmovie ( idGenre integer, idMovie integer)\n");
    m_pDS->exec("CREATE TABLE movie ( idMovie integer primary key, idPath integer, hasSubtitles integer)\n");
    m_pDS->exec("CREATE TABLE movieinfo ( idMovie integer, idDirector integer, strPlotOutline text, strPlot text, strTagLine text, strVotes text, fRating text,strCast text,strCredits text, iYear integer, strGenre text, strPictureURL text, strTitle text)\n");
    m_pDS->exec("CREATE TABLE actorlinkmovie ( idActor integer, idMovie integer )\n");
    m_pDS->exec("CREATE TABLE actors ( idActor integer primary key, strActor text )\n");

    m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, cdlabel text )\n");
    m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, idMovie integer,strFilename text)\n");
  }
  catch (...) 
	{ 
		return false;
	}

	return true;
}

//********************************************************************************************************************************
long CVideoDatabase::AddFile(long lMovieId, long lPathId, const CStdString& strFileName)
{
  long lFileId;
  if (NULL==m_pDB.get()) return -1;
	if (NULL==m_pDS.get()) return -1;
	CStdString strSQL;
  strSQL.Format("select * from files where idmovie=%i and idpath=%i and strFileName like '%s'",lMovieId,lPathId,strFileName.c_str());
 m_pDS->query(strSQL.c_str());
  if (m_pDS->num_rows() > 0) 
  {
    lFileId=m_pDS->fv("idFile").get_asLong() ;
    return lFileId;
  }
	strSQL.Format ("insert into files (idFile, idMovie,idPath, strFileName) values(NULL, %i,%i,'%s')", lMovieId,lPathId, strFileName.c_str() );
	m_pDS->exec(strSQL.c_str());
  lFileId=sqlite_last_insert_rowid( m_pDB->getHandle() );
  return lFileId;
}


//********************************************************************************************************************************
long CVideoDatabase::GetFile(const CStdString& strFilenameAndPath, long &lPathId, long& lMovieId, bool bExact)
{
  lPathId=-1;
  lMovieId=-1;
  if (NULL==m_pDB.get()) return -1;
	if (NULL==m_pDS.get()) return -1;
	CStdString strPath, strFileName ;
	Split(strFilenameAndPath, strPath, strFileName); 
  RemoveInvalidChars(strPath);
  
  lPathId = GetPath(strPath);
  if (lPathId < 0) return -1;

	CStdString strSQL;
  strSQL.Format("select * from files where idpath=%i",lPathId);
  m_pDS->query(strSQL.c_str());
  if (m_pDS->num_rows() > 0) 
  {
    while (!m_pDS->eof()) 
    {
      CStdString strFname= m_pDS->fv("strFilename").get_asString() ;
      if (bExact)
      {
        if (strFname==strFileName) return true;
      }
      else
      {
        double fPercentage=fstrcmp(strFname.c_str(),strFileName.c_str(), COMPARE_PERCENTAGE_MIN );
        if ( fPercentage >=COMPARE_PERCENTAGE)
        {
          long lFileId=m_pDS->fv("idFile").get_asLong() ;
          lMovieId=m_pDS->fv("idMovie").get_asLong() ;
          return lFileId;
        }
      }
      m_pDS->next();
    }
  }
  return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::AddPath(const CStdString& strPath, const CStdString& strCdLabel)
{
  if (NULL==m_pDB.get()) return -1;
	if (NULL==m_pDS.get()) return -1;
	CStdString strSQL;
  strSQL.Format("select * from path where strPath like '%s'",strPath.c_str());
	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() == 0) 
	{
		// doesnt exists, add it
		strSQL.Format("insert into Path (idPath, strPath,cdlabel) values( NULL, '%s', '%s')",
                      strPath.c_str(),strCdLabel.c_str());
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

//********************************************************************************************************************************
long CVideoDatabase::GetPath(const CStdString& strPath)
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
	return -1;
}

//********************************************************************************************************************************
long CVideoDatabase::GetMovie(const CStdString& strFilenameAndPath)
{
  long lPathId;
  long lMovieId;
  if (GetFile(strFilenameAndPath, lPathId, lMovieId) < 0)
  {
    return -1;
  }
  return lMovieId;
}

//********************************************************************************************************************************
long CVideoDatabase::AddMovie(const CStdString& strFilenameAndPath, const CStdString& strcdLabel, bool bHassubtitles)
{
  if (NULL==m_pDB.get()) return -1;
	if (NULL==m_pDS.get()) return -1;
	CStdString strPath, strFileName, strCDLabel=strcdLabel;
	Split(strFilenameAndPath, strPath, strFileName); 
  RemoveInvalidChars(strPath);
  RemoveInvalidChars(strFileName);
  RemoveInvalidChars(strCDLabel);
  
  long lMovieId = GetMovie(strFilenameAndPath);
  if (lMovieId < 0)
  {
	  CStdString strSQL;

    long lPathId = AddPath(strPath,strCDLabel);
  
    strSQL.Format("insert into movie (idMovie, idPath, hasSubtitles) values( NULL, %i, %i)",
	                                    lPathId,bHassubtitles);
	  m_pDS->exec(strSQL.c_str());
    lMovieId=sqlite_last_insert_rowid(m_pDB->getHandle());

    AddFile(lMovieId,lPathId,strFileName);
  }
  else
  {
    long lPathId = GetPath(strPath);
    AddFile(lMovieId,lPathId,strFileName);
  }
  return lMovieId;
}


//********************************************************************************************************************************
long CVideoDatabase::AddGenre(const CStdString& strGenre1)
{
	CStdString strGenre=strGenre1;
	RemoveInvalidChars(strGenre);

	if (NULL==m_pDB.get()) return -1;
	if (NULL==m_pDS.get()) return -1;
	CStdString strSQL="select * from genre where strGenre like '";
	strSQL += strGenre;
	strSQL += "'";
	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() == 0) 
	{
		// doesnt exists, add it
		strSQL = "insert into genre (idGenre, strGenre) values( NULL, '" ;
		strSQL += strGenre;
		strSQL += "')";
		m_pDS->exec(strSQL.c_str());
		long lGenreId=sqlite_last_insert_rowid(m_pDB->getHandle());
		return lGenreId;
	}
	else
	{
		const field_value value = m_pDS->fv("idGenre");
		long lGenreId=value.get_asLong() ;
		return lGenreId;
	}

	return -1;
}


//********************************************************************************************************************************
long CVideoDatabase::AddActor(const CStdString& strActor1)
{
	CStdString strActor=strActor1;
	RemoveInvalidChars(strActor);

	if (NULL==m_pDB.get()) return -1;
	if (NULL==m_pDS.get()) return -1;
	CStdString strSQL="select * from Actors where strActor like '";
	strSQL += strActor;
	strSQL += "'";
	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() == 0) 
	{
		// doesnt exists, add it
		strSQL = "insert into Actors (idActor, strActor) values( NULL, '" ;
		strSQL += strActor;
		strSQL += "')";
		m_pDS->exec(strSQL.c_str());
		long lActorId=sqlite_last_insert_rowid(m_pDB->getHandle());
		return lActorId;
	}
	else
	{
		const field_value value = m_pDS->fv("idActor");
		long lActorId=value.get_asLong() ;
		return lActorId;
	}

	return -1;
}
//********************************************************************************************************************************
void CVideoDatabase::AddGenreToMovie(long lMovieId, long lGenreId)
{

	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
	CStdString strSQL;
  strSQL.Format("select * from genrelinkmovie where idGenre=%i and idMovie=%i",lGenreId,lMovieId);
	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() == 0) 
	{
		// doesnt exists, add it
		strSQL.Format ("insert into genrelinkmovie (idGenre, idMovie) values( %i,%i)",lGenreId,lMovieId);
		m_pDS->exec(strSQL.c_str());
	}
}

//********************************************************************************************************************************
void CVideoDatabase::AddActorToMovie(long lMovieId, long lActorId)
{
	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
	CStdString strSQL;
  strSQL.Format("select * from actorlinkmovie where idActor=%i and idMovie=%i",lActorId,lMovieId);
	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() == 0) 
	{
		// doesnt exists, add it
		strSQL.Format ("insert into actorlinkmovie (idActor, idMovie) values( %i,%i)",lActorId,lMovieId);
		m_pDS->exec(strSQL.c_str());
	}
}

//********************************************************************************************************************************
void CVideoDatabase::GetGenres(VECMOVIEGENRES& genres)
{
  genres.erase(genres.begin(),genres.end());
	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
  m_pDS->query("select * from genre order by strGenre");
  if (m_pDS->num_rows() == 0)  return;
  while (!m_pDS->eof()) 
  {
    genres.push_back( m_pDS->fv("strGenre").get_asString() );
    m_pDS->next();
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetActors(VECMOVIEACTORS& actors)
{
  actors.erase(actors.begin(),actors.end());
	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
  m_pDS->query("select * from actors");
  if (m_pDS->num_rows() == 0)  return;
  while (!m_pDS->eof()) 
  {
    actors.push_back( m_pDS->fv("strActor").get_asString() );
    m_pDS->next();
  }
}

//********************************************************************************************************************************
bool CVideoDatabase::HasMovieInfo(const CStdString& strFilenameAndPath)
{
  if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
  long lMovieId=GetMovie(strFilenameAndPath);
  if ( lMovieId < 0) return false;
  CStdString strSQL;
  strSQL.Format("select * from movieinfo where movieinfo.idmovie=%i",lMovieId);
	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() == 0) 
  {
    return false;
  }
  return true;
}

//********************************************************************************************************************************
bool CVideoDatabase::HasSubtitle(const CStdString& strFilenameAndPath)
{
  //todo
  if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
 
  long lMovieId=GetMovie(strFilenameAndPath);
  if ( lMovieId < 0) return false;

	CStdString strSQL;
  strSQL.Format("select * from movie where movie.idMovie=%i",lMovieId);
	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() == 0) 
  {
    return false;
  }
	long lHasSubs = m_pDS->fv("hasSubtitles").get_asLong() ;
  return (lHasSubs!=0);
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteMovieInfo(const CStdString& strFileNameAndPath)
{
  if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
  
  long lMovieId=GetMovie(strFileNameAndPath);
  if ( lMovieId < 0) return ;
  
  CStdString strSQL;
  strSQL.Format("delete from genrelinkmovie where idmovie=%i", lMovieId);
  m_pDS->exec(strSQL.c_str());

  strSQL.Format("delete from actorlinkmovie where idmovie=%i", lMovieId);
  m_pDS->exec(strSQL.c_str());

  strSQL.Format("delete from movieinfo where idmovie=%i", lMovieId);
  m_pDS->exec(strSQL.c_str());  
}



//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByGenre(CStdString& strGenre1, VECMOVIES& movies)
{
 	CStdString strGenre=strGenre1;
	RemoveInvalidChars(strGenre);

  movies.erase(movies.begin(),movies.end());
	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
  CStdString strSQL;
  strSQL.Format("select * from genrelinkmovie,genre,movie,movieinfo,actors where genrelinkmovie.idGenre=genre.idGenre and genrelinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and genre.strGenre='%s' and movieinfo.iddirector=actors.idActor", strGenre.c_str());

  m_pDS->query( strSQL.c_str() );
  if (m_pDS->num_rows() == 0)  return;
  while (!m_pDS->eof()) 
  {
    CIMDBMovie details;
    details.m_fRating=(float)atof(m_pDS->fv("movieinfo.fRating").get_asString().c_str()) ;
	  details.m_strDirector=m_pDS->fv("actors.strActor").get_asString();
	  details.m_strWritingCredits=m_pDS->fv("movieinfo.strCredits").get_asString();
	  details.m_strTagLine=m_pDS->fv("movieinfo.strTagLine").get_asString();
	  details.m_strPlotOutline=m_pDS->fv("movieinfo.strPlotOutline").get_asString();
	  details.m_strPlot=m_pDS->fv("movieinfo.strPlot").get_asString();
	  details.m_strVotes=m_pDS->fv("movieinfo.strVotes").get_asString();
	  details.m_strCast=m_pDS->fv("movieinfo.strCast").get_asString();
	  details.m_iYear=m_pDS->fv("movieinfo.iYear").get_asLong();
    details.m_strGenre=m_pDS->fv("movieinfo.strGenre").get_asString();
    details.m_strPictureURL=m_pDS->fv("movieinfo.strPictureURL").get_asString();
    details.m_strTitle=m_pDS->fv("movieinfo.strTitle").get_asString();
    long lMovieId=m_pDS->fv("movieinfo.idMovie").get_asLong();
    details.m_strSearchString.Format("%i", lMovieId);

    movies.push_back(details);
    m_pDS->next();
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByActor(CStdString& strActor1, VECMOVIES& movies)
{
  //todo
  CStdString strActor=strActor1;
	RemoveInvalidChars(strActor);

  movies.erase(movies.begin(),movies.end());
	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
  CStdString strSQL;
  strSQL.Format("select * from actorlinkmovie,actors,movie,movieinfo where actors.idActor=actorlinkmovie.idActor and actorlinkmovie.idmovie=movie.idmovie and movieinfo.idmovie=movie.idmovie and actors.stractor='%s'", strActor.c_str());
  m_pDS->query( strSQL.c_str() );
  if (m_pDS->num_rows() == 0)  return;
  while (!m_pDS->eof()) 
  {
    CIMDBMovie details;
    details.m_fRating=(float)atof(m_pDS->fv("movieinfo.fRating").get_asString().c_str()) ;
	  details.m_strDirector=m_pDS->fv("actors.strActor").get_asString();
	  details.m_strWritingCredits=m_pDS->fv("movieinfo.strCredits").get_asString();
	  details.m_strTagLine=m_pDS->fv("movieinfo.strTagLine").get_asString();
	  details.m_strPlotOutline=m_pDS->fv("movieinfo.strPlotOutline").get_asString();
	  details.m_strPlot=m_pDS->fv("movieinfo.strPlot").get_asString();
	  details.m_strVotes=m_pDS->fv("movieinfo.strVotes").get_asString();
	  details.m_strCast=m_pDS->fv("movieinfo.strCast").get_asString();
	  details.m_iYear=m_pDS->fv("movieinfo.iYear").get_asLong();
    details.m_strGenre=m_pDS->fv("movieinfo.strGenre").get_asString();
    details.m_strPictureURL=m_pDS->fv("movieinfo.strPictureURL").get_asString();
    details.m_strTitle=m_pDS->fv("movieinfo.strTitle").get_asString();
    long lMovieId=m_pDS->fv("movieinfo.idMovie").get_asLong();
    details.m_strSearchString.Format("%i", lMovieId);

    movies.push_back(details);
    m_pDS->next();
  }
}


//********************************************************************************************************************************
void CVideoDatabase::GetMovieInfo(const CStdString& strFilenameAndPath,CIMDBMovie& details)
{
  long lMovieId=GetMovie(strFilenameAndPath);
  if (lMovieId<0) return;

	CStdString strSQL;
  strSQL.Format("select * from movieinfo,actors where idmovie=%i and idDirector=idActor", lMovieId);
	m_pDS->query(strSQL.c_str());
  if (m_pDS->num_rows() == 0) 
  {
    return ;
  }
  details.m_fRating=(float)atof(m_pDS->fv("movieinfo.fRating").get_asString().c_str()) ;
	details.m_strDirector=m_pDS->fv("actors.strActor").get_asString();
	details.m_strWritingCredits=m_pDS->fv("movieinfo.strCredits").get_asString();
	details.m_strTagLine=m_pDS->fv("movieinfo.strTagLine").get_asString();
	details.m_strPlotOutline=m_pDS->fv("movieinfo.strPlotOutline").get_asString();
	details.m_strPlot=m_pDS->fv("movieinfo.strPlot").get_asString();
	details.m_strVotes=m_pDS->fv("movieinfo.strVotes").get_asString();
	details.m_strCast=m_pDS->fv("movieinfo.strCast").get_asString();
	details.m_iYear=m_pDS->fv("movieinfo.iYear").get_asLong();
  details.m_strGenre=m_pDS->fv("movieinfo.strGenre").get_asString();
  details.m_strPictureURL=m_pDS->fv("movieinfo.strPictureURL").get_asString();
  details.m_strTitle=m_pDS->fv("movieinfo.strTitle").get_asString();
  details.m_strSearchString.Format("%i", lMovieId);
}

//********************************************************************************************************************************
void CVideoDatabase::SetMovieInfo(const CStdString& strFilenameAndPath, CIMDBMovie& details)
{
  long lMovieId=GetMovie(strFilenameAndPath);
  if (lMovieId< 0) return;
  details.m_strSearchString.Format("%i", lMovieId);

  CIMDBMovie details1=details;
  RemoveInvalidChars(details1.m_strCast);
  RemoveInvalidChars(details1.m_strDirector);  
  RemoveInvalidChars(details1.m_strPlot);  
  RemoveInvalidChars(details1.m_strPlotOutline);  
  RemoveInvalidChars(details1.m_strTagLine);  
  RemoveInvalidChars(details1.m_strPictureURL);  
  RemoveInvalidChars(details1.m_strSearchString);  
  RemoveInvalidChars(details1.m_strTitle);  
  RemoveInvalidChars(details1.m_strVotes);  
  RemoveInvalidChars(details1.m_strWritingCredits);  
  RemoveInvalidChars(details1.m_strGenre);  

  // add director
  long lDirector=AddActor(details.m_strDirector);
  
  // add all genres
  char szGenres[1024];
  strcpy(szGenres,details.m_strGenre.c_str());
  vector<long> vecGenres;
  if (strstr(szGenres,"/"))
  {
     char *pToken=strtok(szGenres,"/");
     while( pToken != NULL )
     {
       CStdString strGenre=pToken; 
       strGenre.Trim();
       long lGenreId=AddGenre(strGenre);
       vecGenres.push_back(lGenreId);
       pToken = strtok( NULL, "/" );
     }
  }
  else
  {
    CStdString strGenre=details.m_strGenre; 
    strGenre.Trim();
    long lGenreId=AddGenre(strGenre);
    vecGenres.push_back(lGenreId);
  }

  // add cast...
  vector<long> vecActors;
  CStdString strCast;
  int ipos=0;
  strCast=details.m_strCast.c_str();
  ipos=strCast.Find(" as ");
  while (ipos > 0)
  {
    CStdString strActor;
    int x=ipos;
    while (x > 0)
    {
      if (strCast[x] != '\r'&& strCast[x]!='\n') x--;
      else 
      {
        x++;
        break;
      }
    }
    strActor=strCast.Mid(x,ipos-x);          
    long lActorId=AddActor(strActor);
    vecActors.push_back(lActorId);
    ipos=strCast.Find(" as ",ipos+3);
  }

  for (int i=0; i < (int)vecGenres.size(); ++i)
  {
    AddGenreToMovie(lMovieId,vecGenres[i]);
  }
  
  for (i=0; i < (int)vecActors.size(); ++i)
  {
    AddActorToMovie(lMovieId,vecActors[i]);
  }

	CStdString strSQL;
  CStdString strRating;
  strRating.Format("%3.3f", details1.m_fRating);
  if (strRating=="") strRating="0.0";
  strSQL.Format("select * from movieinfo where idmovie=%i", lMovieId);
	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() == 0) 
  {
    CStdString strTmp="";
    strSQL.Format("insert into movieinfo ( idMovie,idDirector,strPlotOutline,strPlot,strTagLine,strVotes,fRating,strCast,strCredits , iYear  ,strGenre, strPictureURL, strTitle) values(%i,%i,'%s','%s','%s','%s','%s','%s','%s',%i,'%s','%s','%s')",
            lMovieId,lDirector, details1.m_strPlotOutline.c_str(),
            details1.m_strPlot.c_str(),details1.m_strTagLine.c_str(),
            details1.m_strVotes.c_str(),strRating.c_str(),
            details1.m_strCast.c_str(),details1.m_strWritingCredits.c_str(),
            
            details1.m_iYear, details1.m_strGenre.c_str(),
            details1.m_strPictureURL.c_str(),details1.m_strTitle.c_str() );

	  m_pDS->exec(strSQL.c_str());
              
  }
  else
  {
    strSQL.Format("update movieinfo set idDirector=%i, strPlotOutline='%s', strPlot='%s', strTagLine='%s', strVotes='%s', fRating='%s', strCast='%s',strCredits='%s', iYear=%i, strGenre='%s' strPictureURL='%s', strTitle='%s' where idMovie=%i",
            lDirector,details1.m_strPlotOutline.c_str(),
            details1.m_strPlot.c_str(),details1.m_strTagLine.c_str(),
            details1.m_strVotes.c_str(),strRating.c_str(),
            details1.m_strCast.c_str(),details1.m_strWritingCredits.c_str(),
            details1.m_iYear,details1.m_strGenre.c_str(),
            details1.m_strPictureURL.c_str(),details1.m_strTitle.c_str(),lMovieId);
	  m_pDS->exec(strSQL.c_str());
  }
}


//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByPath(CStdString& strPath1, VECMOVIES& movies)
{
 	CStdString strPath=strPath1;
	RemoveInvalidChars(strPath);

  movies.erase(movies.begin(),movies.end());
	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
  long lPathId = GetPath(strPath);
  if (lPathId< 0) return;

  CStdString strSQL;
  strSQL.Format("select * from files where files.idpath=%i", lPathId);

  m_pDS->query( strSQL.c_str() );
  if (m_pDS->num_rows() == 0)  return;
  while (!m_pDS->eof()) 
  {
    CIMDBMovie details;
    long lMovieId=m_pDS->fv("idMovie").get_asLong();
    details.m_strSearchString.Format("%i", lMovieId);
    details.m_strFile=m_pDS->fv("strFilename").get_asString();
    movies.push_back(details);
    m_pDS->next();
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetFiles(long lMovieId, VECMOVIESFILES& movies)
{
  movies.erase(movies.begin(),movies.end());
	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;

	//long lMovieId=GetMovie(strFile);
	if (lMovieId < 0) return;
	
	CStdString strSQL;
  strSQL.Format("select * from path,files where path.idPath=files.idPath and files.idmovie=%i order by strFilename", lMovieId );
  m_pDS->query( strSQL.c_str() );
  if (m_pDS->num_rows() == 0)  return;
  while (!m_pDS->eof()) 
  {
    CStdString strPath,strFile;
    strFile=m_pDS->fv("files.strFilename").get_asString();
    strPath=m_pDS->fv("path.strPath").get_asString();
    strFile=strPath+strFile;
    movies.push_back(strFile);
    m_pDS->next();
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetYears(VECMOVIEYEARS& years)
{
  years.erase(years.begin(),years.end());
	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
  m_pDS->query("select * from movieinfo");
  if (m_pDS->num_rows() == 0)  return;
  while (!m_pDS->eof()) 
  {
    CStdString strYear=m_pDS->fv("iYear").get_asString();
    bool bAdd=true;
    for (int i=0; i < (int)years.size();++i)
    {
      if (strYear == years[i]) bAdd=false;
    }
    if (bAdd) years.push_back( strYear);
    m_pDS->next();
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByYear(CStdString& strYear, VECMOVIES& movies)
{
	int iYear = atoi(strYear.c_str());
  movies.erase(movies.begin(),movies.end());
	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
  CStdString strSQL;
  strSQL.Format("select * from movie,movieinfo,actors where movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor and movieinfo.iYear=%i",iYear);

  m_pDS->query( strSQL.c_str() );
  if (m_pDS->num_rows() == 0)  return;
  while (!m_pDS->eof()) 
  {
    CIMDBMovie details;
    details.m_fRating=(float)atof(m_pDS->fv("movieinfo.fRating").get_asString().c_str()) ;
	  details.m_strDirector=m_pDS->fv("actors.strActor").get_asString();
	  details.m_strWritingCredits=m_pDS->fv("movieinfo.strCredits").get_asString();
	  details.m_strTagLine=m_pDS->fv("movieinfo.strTagLine").get_asString();
	  details.m_strPlotOutline=m_pDS->fv("movieinfo.strPlotOutline").get_asString();
	  details.m_strPlot=m_pDS->fv("movieinfo.strPlot").get_asString();
	  details.m_strVotes=m_pDS->fv("movieinfo.strVotes").get_asString();
	  details.m_strCast=m_pDS->fv("movieinfo.strCast").get_asString();
	  details.m_iYear=m_pDS->fv("movieinfo.iYear").get_asLong();
    details.m_strGenre=m_pDS->fv("movieinfo.strGenre").get_asString();
    details.m_strPictureURL=m_pDS->fv("movieinfo.strPictureURL").get_asString();
    details.m_strTitle=m_pDS->fv("movieinfo.strTitle").get_asString();
    long lMovieId=m_pDS->fv("movieinfo.idMovie").get_asLong();
    details.m_strSearchString.Format("%i", lMovieId);

    movies.push_back(details);
    m_pDS->next();
  }
}

//********************************************************************************************************************************
void CVideoDatabase::GetBookMarksForMovie(const CStdString& strFilenameAndPath, VECBOOKMARKS& bookmarks)
{
  long lPathId, lMovieId;
  long lFileId=GetFile(strFilenameAndPath, lPathId, lMovieId, true);
  if (lFileId < 0) return;
  bookmarks.erase(bookmarks.begin(),bookmarks.end());
	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;

  CStdString strSQL;
  strSQL.Format("select * from bookmark where idFile=%i order by fPercentage", lFileId);
  m_pDS->query( strSQL.c_str() );
  if (m_pDS->num_rows() == 0)  return;
  while (!m_pDS->eof()) 
  {
    float fPercentage=m_pDS->fv("fPercentage").get_asFloat();
    bookmarks.push_back(fPercentage);
    m_pDS->next();
  }
}

//********************************************************************************************************************************
void CVideoDatabase::AddBookMarkToMovie(const CStdString& strFilenameAndPath, float fPercentage)
{
  long lPathId, lMovieId;
  long lFileId=GetFile(strFilenameAndPath, lPathId, lMovieId, true);
  if (lFileId < 0) return;
	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
  CStdString strSQL;
  strSQL.Format("select * from bookmark where idFile=%i and fPercentage=%03.3f",lFileId,fPercentage);
  m_pDS->query( strSQL.c_str() );
  if (m_pDS->num_rows() != 0)  return;

	strSQL.Format ("insert into bookmark (idBookmark, idFile, fPercentage) values(NULL,%i,%03.3f)", lFileId,fPercentage);
	m_pDS->exec(strSQL.c_str());
}

//********************************************************************************************************************************
void CVideoDatabase::ClearBookMarksOfMovie(const CStdString& strFilenameAndPath)
{
  long lPathId, lMovieId;
  long lFileId=GetFile(strFilenameAndPath, lPathId, lMovieId, true);
  if (lFileId < 0) return;
	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
  CStdString strSQL;
  strSQL.Format("delete from bookmark where idFile=%i",lFileId);
	m_pDS->exec(strSQL.c_str());
}

void  CVideoDatabase::GetMovies(VECMOVIES& movies)
{	
  movies.erase(movies.begin(),movies.end());
	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
  CStdString strSQL;
  strSQL.Format("select * from movie,movieinfo,actors where movieinfo.idmovie=movie.idmovie and movieinfo.iddirector=actors.idActor");

  m_pDS->query( strSQL.c_str() );
  if (m_pDS->num_rows() == 0)  return;
  while (!m_pDS->eof()) 
  {
    CIMDBMovie details;
    details.m_fRating=(float)atof(m_pDS->fv("movieinfo.fRating").get_asString().c_str()) ;
	  details.m_strDirector=m_pDS->fv("actors.strActor").get_asString();
	  details.m_strWritingCredits=m_pDS->fv("movieinfo.strCredits").get_asString();
	  details.m_strTagLine=m_pDS->fv("movieinfo.strTagLine").get_asString();
	  details.m_strPlotOutline=m_pDS->fv("movieinfo.strPlotOutline").get_asString();
	  details.m_strPlot=m_pDS->fv("movieinfo.strPlot").get_asString();
	  details.m_strVotes=m_pDS->fv("movieinfo.strVotes").get_asString();
	  details.m_strCast=m_pDS->fv("movieinfo.strCast").get_asString();
	  details.m_iYear=m_pDS->fv("movieinfo.iYear").get_asLong();
    details.m_strGenre=m_pDS->fv("movieinfo.strGenre").get_asString();
    details.m_strPictureURL=m_pDS->fv("movieinfo.strPictureURL").get_asString();
    details.m_strTitle=m_pDS->fv("movieinfo.strTitle").get_asString();
    long lMovieId=m_pDS->fv("movieinfo.idMovie").get_asLong();
    details.m_strSearchString.Format("%i", lMovieId);
    movies.push_back(details);
    m_pDS->next();
  }
}
