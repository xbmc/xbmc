#include ".\musicdatabase.h"
#include "settings.h"
#include "StdString.h"
#include "crc32.h"
#include "LocalizeStrings.h"

CSong::CSong()
{
	Clear();
}

void CSong::Clear() 
{
	strFileName="";
	strTitle="";
	strArtist="";
	strAlbum="";
	strGenre="";
	iTrack=0;
	iDuration=0;
	iYear=0;
}

CMusicDatabase::CMusicDatabase(void)
{
}

CMusicDatabase::~CMusicDatabase(void)
{
}


void CMusicDatabase::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
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

void CMusicDatabase::RemoveInvalidChars(CStdString& strTxt)
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


bool CMusicDatabase::Open()
{

	Close();

	// test id dbs already exists, if not we need 2 create the tables
	bool bDatabaseExists=false;
	FILE* fd= fopen("Q:\\albums\\MyMusic4.db","rb");
	if (fd)
	{
		bDatabaseExists=true;
		fclose(fd);
	}

	m_pDB.reset(new SqliteDatabase() ) ;
  m_pDB->setDatabase("Q:\\albums\\MyMusic4.db");
	
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


void CMusicDatabase::Close()
{
	if (NULL==m_pDB.get() ) return;
	m_pDB->disconnect();
	m_pDB.reset();
}

bool CMusicDatabase::CreateTables()
{

  try 
	{
    m_pDS->exec("CREATE TABLE artist ( idArtist integer primary key, strArtist text)\n");
    m_pDS->exec("CREATE TABLE album ( idAlbum integer primary key, idArtist integer, strAlbum text)\n");
		m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");
		m_pDS->exec("CREATE TABLE path ( idPath integer primary key,  strPath text)\n");
		m_pDS->exec("CREATE TABLE song ( idSong integer primary key, idArtist integer, idAlbum integer, idGenre integer, idPath integer, strTitle text, iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, iTimesPlayed integer)\n");
		m_pDS->exec("CREATE TABLE albuminfo ( idAlbumInfo integer primary key, idAlbum integer, idArtist integer,iYear integer, idGenre integer, strTones text, strStyles text, strReview text, strImage text, iRating integer)\n");

		m_pDS->exec("CREATE INDEX idxAlbum ON album(strAlbum)");
		m_pDS->exec("CREATE INDEX idxGenre ON genre(strGenre)");
		m_pDS->exec("CREATE INDEX idxArtist ON artist(strArtist)");
		m_pDS->exec("CREATE INDEX idxPath ON path(strPath)");
		//m_pDS->exec("CREATE INDEX idxSong ON song(dwFileNameCRC)");

  }
  catch (...) 
	{ 
		return false;
	}

	return true;
}

void CMusicDatabase::AddSong(const CSong& song1, bool bCheck)
{
	CSong song=song1;
	RemoveInvalidChars(song.strAlbum);
	RemoveInvalidChars(song.strGenre);
	RemoveInvalidChars(song.strArtist);
	RemoveInvalidChars(song.strFileName);
	RemoveInvalidChars(song.strTitle);

	CStdString strPath, strFileName;
	Split(song.strFileName, strPath, strFileName); 

	if (NULL==m_pDB.get()) return ;
	if (NULL==m_pDS.get()) return ;
	long lGenreId  = AddGenre(song1.strGenre);
	long lArtistId = AddArtist(song1.strArtist);
	long lPathId   = AddPath(strPath);
	long lAlbumId  = AddAlbum(song1.strAlbum,lArtistId);
	DWORD dwCRC;
	Crc32 crc;
	crc.Reset();
	crc.Compute(song1.strFileName.c_str(),strlen(song1.strFileName.c_str()));
	dwCRC=crc;

	char szSQL[1024];
  if (bCheck)
  {
	  sprintf(szSQL,"select * from song where idAlbum=%i AND idGenre=%i AND idArtist=%i AND dwFileNameCRC='%ul' AND strTitle='%s'", 
			  lAlbumId,lGenreId,lArtistId,dwCRC,song.strTitle.c_str());
	  try
	  {
		  if (!m_pDS->query(szSQL)) return;
	  }
	  catch(...)
	  {
		  OutputDebugString("-------ERRORohoh!");
		  return;
	  }
	  int iRowsFound = m_pDS->num_rows();
	  if (iRowsFound!= 0) 
	  {
		  return ; // already exists
	  }
  }
	

	sprintf(szSQL,"insert into song (idSong,idArtist,idAlbum,idGenre,idPath,strTitle,iTrack,iDuration,iYear,dwFileNameCRC,strFileName,iTimesPlayed) values(NULL,%i,%i,%i,%i,'%s',%i,%i,%i,'%ul','%s',%i)",
								lArtistId,lAlbumId,lGenreId,lPathId,
								song.strTitle.c_str(),
								song.iTrack,song.iDuration,song.iYear,
								dwCRC,
								strFileName.c_str(), 0);
	try
	{
		m_pDS->exec(szSQL);
	}
	catch(...)
	{
		OutputDebugString("-------ERROR----------------");
	}
}


long CMusicDatabase::AddAlbum(const CStdString& strAlbum1, long lArtistId)
{
	CStdString strAlbum=strAlbum1;
	RemoveInvalidChars(strAlbum);

	if (NULL==m_pDB.get()) return -1;
	if (NULL==m_pDS.get()) return -1;

	VECSONGS songs;
	GetSongsByAlbum(strAlbum1, songs);
	if (songs.size()>1)
	{
		//	Are the artists of this album all the same
		for (int i=0; i < (int)songs.size()-1; i++)
		{
			CSong song=songs[i];
			CSong song1=songs[i+1];
			if (song.strArtist!=song1.strArtist)
			{
				CStdString strVariousArtists=g_localizeStrings.Get(340);
				lArtistId=AddArtist(strVariousArtists);
				break;
			}
		}
	}

	CStdString strSQL;
	strSQL.Format("select * from album where strAlbum like '%s'", strAlbum);
	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() == 0) 
	{
		// doesnt exists, add it
		char sSQL[1024];
		sprintf(sSQL,"insert into album (idAlbum, strAlbum,idArtist) values( NULL, '%s',%i)", strAlbum.c_str(),lArtistId);
		m_pDS->exec(sSQL);
		long lAlbumID=sqlite_last_insert_rowid(m_pDB->getHandle());
		return lAlbumID;
	}
	else
	{
		long lAlbumID=m_pDS->fv("idAlbum").get_asLong();
		long lArtistID=m_pDS->fv("idArtist").get_asLong();

		// Is this a "Various Artists" album which has another artist set?
		if (lArtistID!=lArtistId)
		{
			char sSQL[1024];
			sprintf(sSQL,"update album set idArtist=%i where idAlbum=%i", lArtistId, lAlbumID);
			m_pDS->exec(sSQL);
		}

		return lAlbumID;
	}


	return -1;
}

long CMusicDatabase::AddGenre(const CStdString& strGenre1)
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

long CMusicDatabase::AddArtist(const CStdString& strArtist1)
{
	CStdString strArtist=strArtist1;
	RemoveInvalidChars(strArtist);

	if (NULL==m_pDB.get()) return -1;
	if (NULL==m_pDS.get()) return -1;
	CStdString strSQL="select * from Artist where strArtist like '";
	strSQL += strArtist;
	strSQL += "'";
	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() == 0) 
	{
		// doesnt exists, add it
		strSQL = "insert into Artist (idArtist, strArtist) values( NULL, '" ;
		strSQL += strArtist;
		strSQL += "')";
		m_pDS->exec(strSQL.c_str());
		long lArtistId=sqlite_last_insert_rowid(m_pDB->getHandle());
		return lArtistId;
	}
	else
	{
		const field_value value = m_pDS->fv("idArtist");
		long lArtistId=value.get_asLong() ;
		return lArtistId;
	}


	return -1;
}

long CMusicDatabase::AddPath(const CStdString& strPath1)
{
	CStdString strPath=strPath1;
	//if (bRemoveChars) RemoveInvalidChars(strPath);

	if (NULL==m_pDB.get()) return -1;
	if (NULL==m_pDS.get()) return -1;
	CStdString strSQL="select * from Path where strPath like '";
	strSQL += strPath;
	strSQL += "'";
	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() == 0) 
	{
		// doesnt exists, add it
		strSQL = "insert into Path (idPath, strPath) values( NULL, '" ;
		strSQL += strPath;
		strSQL += "')";
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


bool CMusicDatabase::GetSongByFileName(const CStdString& strFileName1, CSong& song)
{
	song.Clear();
	CStdString strFileName=strFileName1;
	RemoveInvalidChars(strFileName);

	CStdString strPath, strFName;
	Split(strFileName, strPath, strFName);

	if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
	char szSQL[1024];
	DWORD dwCRC;
	Crc32 crc;
	crc.Reset();
  crc.Compute(strFileName1.c_str(),strlen(strFileName1.c_str()));
	dwCRC=crc;

	sprintf(szSQL,"select * from song,album,genre,artist,path where song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre and song.idArtist=artist.idArtist and dwFileNameCRC='%ul' and strPath='%s'",
									dwCRC,
									strPath.c_str());
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;

	song.strArtist    = m_pDS->fv("artist.strArtist").get_asString();
	song.strAlbum     = m_pDS->fv("album.strAlbum").get_asString();
	song.strGenre     = m_pDS->fv("genre.strGenre").get_asString();
	song.iTrack       = m_pDS->fv("song.iTrack").get_asLong() ;
	song.iDuration    = m_pDS->fv("song.iDuration").get_asLong() ;
	song.iYear        = m_pDS->fv("song.iYear").get_asLong() ;
	song.strTitle	    = m_pDS->fv("song.strTitle").get_asString() ;
	song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
	song.strFileName  = strFileName1;

	return true;
}

bool CMusicDatabase::GetSong(const CStdString& strTitle1, CSong& song)
{
	song.Clear();
	CStdString strTitle=strTitle1;
	RemoveInvalidChars(strTitle);

	if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
	char szSQL[1024];

	sprintf(szSQL,"select * from song,album,genre,artist,path where song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre and song.idArtist=artist.idArtist and strTitle='%s'",strTitle.c_str() );
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;

	song.strArtist = m_pDS->fv("artist.strArtist").get_asString();
	song.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
	song.strGenre  = m_pDS->fv("genre.strGenre").get_asString();
	song.iTrack    = m_pDS->fv("song.iTrack").get_asLong() ;
	song.iDuration = m_pDS->fv("song.iDuration").get_asLong() ;
	song.iYear     = m_pDS->fv("song.iYear").get_asLong() ;
	song.strTitle  = m_pDS->fv("song.strTitle").get_asString() ;
	song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();

	string strFileName = m_pDS->fv("path.strPath").get_asString() ;
	strFileName += m_pDS->fv("song.strFileName").get_asString() ;
	song.strFileName = strFileName;
	return true;
}

bool CMusicDatabase::GetSongsByArtist(const CStdString strArtist1, VECSONGS& songs)
{	
	CStdString strArtist=strArtist1;
	RemoveInvalidChars(strArtist);

	songs.erase(songs.begin(), songs.end());
	if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from song,album,genre,artist,path where song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre and song.idArtist=artist.idArtist and artist.strArtist like '%s'",strArtist.c_str() );
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;
	while (!m_pDS->eof()) 
	{
		CSong song;
		song.strArtist = m_pDS->fv("artist.strArtist").get_asString();
		song.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
		song.strGenre  = m_pDS->fv("genre.strGenre").get_asString();
		song.iTrack    = m_pDS->fv("song.iTrack").get_asLong() ;
		song.iDuration = m_pDS->fv("song.iDuration").get_asLong() ;
		song.iYear     = m_pDS->fv("song.iYear").get_asLong() ;
		song.strTitle  = m_pDS->fv("song.strTitle").get_asString();
		song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
			
		string strFileName = m_pDS->fv("path.strPath").get_asString() ;
		strFileName += m_pDS->fv("song.strFileName").get_asString() ;
		song.strFileName = strFileName;
		songs.push_back(song);
		m_pDS->next();
	}

	return true;
}

bool CMusicDatabase::GetSongsByAlbum(const CStdString& strAlbum1, VECSONGS& songs)
{
	CStdString strAlbum=strAlbum1;
	RemoveInvalidChars(strAlbum);

	songs.erase(songs.begin(), songs.end());
	if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from song,album,genre,artist,path where song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre and song.idArtist=artist.idArtist and album.strAlbum like '%s'",strAlbum.c_str() );
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;
	while (!m_pDS->eof()) 
	{
		CSong song;
		song.strArtist = m_pDS->fv("artist.strArtist").get_asString();
		song.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
		song.strGenre  = m_pDS->fv("genre.strGenre").get_asString();
		song.iTrack    = m_pDS->fv("song.iTrack").get_asLong() ;
		song.iDuration = m_pDS->fv("song.iDuration").get_asLong() ;
		song.iYear     = m_pDS->fv("song.iYear").get_asLong() ;
		song.strTitle  = m_pDS->fv("song.strTitle").get_asString();
		song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();

		string strFileName = m_pDS->fv("path.strPath").get_asString() ;
		strFileName += m_pDS->fv("song.strFileName").get_asString() ;
		song.strFileName = strFileName;

		songs.push_back(song);
		m_pDS->next();
	}

	return true;
}

bool CMusicDatabase::GetArtists(VECARTISTS& artists)
{

	artists.erase(artists.begin(), artists.end());
	if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
	// Exclude "Various Artists"
	CStdString strVariousArtists=g_localizeStrings.Get(340);
	long lVariousArtistId=AddArtist(strVariousArtists);
	char szSQL[1024];
	sprintf(szSQL,"select * from artist where idArtist <> %i ", lVariousArtistId );
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;
	while (!m_pDS->eof()) 
	{
		CStdString strArtist = m_pDS->fv("strArtist").get_asString();
		artists.push_back(strArtist);
		m_pDS->next();
	}

	return true;
}

bool CMusicDatabase::GetAlbums(VECALBUMS& albums)
{

	albums.erase(albums.begin(), albums.end());
	if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from album,artist where album.idArtist=artist.idArtist" );
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;
	while (!m_pDS->eof()) 
	{
		CAlbum album;
		album.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
		album.strArtist = m_pDS->fv("artist.strArtist").get_asString();
		albums.push_back(album);
		m_pDS->next();
	}

	return true;
}


long CMusicDatabase::AddAlbumInfo(const CAlbum& album1)
{
	CAlbum album;
	album=album1;
	RemoveInvalidChars(album.strAlbum);
	RemoveInvalidChars(album.strGenre);
	RemoveInvalidChars(album.strArtist);
	RemoveInvalidChars(album.strTones);
	RemoveInvalidChars(album.strStyles);
	RemoveInvalidChars(album.strReview);
	RemoveInvalidChars(album.strImage);

	if (NULL==m_pDB.get()) return -1;
	if (NULL==m_pDS.get()) return -1;
	long lGenreId  = AddGenre(album1.strGenre);
	long lArtistId = AddArtist(album1.strArtist);
	long lAlbumId  = AddAlbum(album1.strAlbum,lArtistId);


	CStdString strSQL;
	strSQL.Format("select * from albuminfo where idAlbum=%i AND idGenre=%i AND idArtist=%i ", lAlbumId,lGenreId,lArtistId);
	if (!m_pDS->query(strSQL.c_str())) return -1;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound!= 0) 
	{
		const field_value value = m_pDS->fv("idAlbumInfo");
		long lAlbumID=value.get_asLong() ;
		return lAlbumID;
	}

	strSQL.Format("insert into albuminfo (idAlbumInfo,idAlbum,idArtist,idGenre,strTones,strStyles,strReview,strImage,iRating,iYear) values(NULL,%i,%i,%i,'%s','%s','%s','%s',%i,%i)",
											lAlbumId,lArtistId,lGenreId,
											album.strTones.c_str(),
											album.strStyles.c_str(),
											album.strReview.c_str(),
											album.strImage.c_str(),
											album.iRating,
											album.iYear);
	m_pDS->exec(strSQL.c_str());
	long lAlbumInfoId=sqlite_last_insert_rowid(m_pDB->getHandle());
	return lAlbumInfoId;

	return -1;
}

bool CMusicDatabase::GetSongsByGenre(const CStdString& strGenre, VECSONGS& songs)
{
	CStdString strSQLGenre=strGenre;
	RemoveInvalidChars(strSQLGenre);

	songs.erase(songs.begin(), songs.end());
	if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from song,album,genre,artist,path where song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre and song.idArtist=artist.idArtist and genre.strGenre like '%s'",strSQLGenre.c_str() );
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;
	while (!m_pDS->eof()) 
	{
		CSong song;
		song.strArtist    = m_pDS->fv("artist.strArtist").get_asString();
		song.strAlbum     = m_pDS->fv("album.strAlbum").get_asString();
		song.strGenre     = m_pDS->fv("genre.strGenre").get_asString();
		song.iTrack       = m_pDS->fv("song.iTrack").get_asLong() ;
		song.iDuration    = m_pDS->fv("song.iDuration").get_asLong() ;
		song.iYear        = m_pDS->fv("song.iYear").get_asLong() ;
		song.strTitle     = m_pDS->fv("song.strTitle").get_asString();
		song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
		
		string strFileName = m_pDS->fv("path.strPath").get_asString() ;
		strFileName += m_pDS->fv("song.strFileName").get_asString() ;
		song.strFileName = strFileName;

		songs.push_back(song);
		m_pDS->next();
	}

	return true;
}

bool CMusicDatabase::GetGenres(VECGENRES& genres)
{

	genres.erase(genres.begin(), genres.end());
	if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from genre " );
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;
	while (!m_pDS->eof()) 
	{
		CStdString strGenre = m_pDS->fv("strGenre").get_asString();
		genres.push_back(strGenre);
		m_pDS->next();
	}

	return true;
}


bool CMusicDatabase::GetAlbumInfo(const CStdString& strAlbum1, CAlbum& album)
{
	CStdString strAlbum = strAlbum1;
	RemoveInvalidChars(strAlbum);
	char szSQL[1024];
	sprintf(szSQL,"select * from albuminfo,album,genre,artist where albuminfo.idAlbum=album.idAlbum and albuminfo.idGenre=genre.idGenre and albuminfo.idArtist=artist.idArtist and strAlbum like '%s'",strAlbum.c_str() );
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound!= 0) 
	{
		album.iRating		= m_pDS->fv("albuminfo.iRating").get_asLong() ;
		album.iYear			= m_pDS->fv("albuminfo.iYear").get_asLong() ;
		album.strAlbum	= m_pDS->fv("album.strAlbum").get_asString();
		album.strArtist	= m_pDS->fv("artist.strArtist").get_asString();
		album.strGenre	= m_pDS->fv("genre.strGenre").get_asString();
		album.strImage	= m_pDS->fv("albuminfo.strImage").get_asString();
		album.strReview	= m_pDS->fv("albuminfo.strReview").get_asString();
		album.strStyles	= m_pDS->fv("albuminfo.strStyles").get_asString();
		album.strTones	= m_pDS->fv("albuminfo.strTones").get_asString();
		return true;
	}
	return false;
}

bool CMusicDatabase::GetTop100(VECSONGS& songs)
{
	songs.erase(songs.begin(), songs.end());
	if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from song,album,genre,artist,path where song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre and song.idArtist=artist.idArtist and iTimesPlayed > 0 order by song.iTimesPlayed desc limit 100"  );
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;
	int iCount=1;
	while (!m_pDS->eof()) 
	{
		CSong song;
		song.strArtist    = m_pDS->fv("artist.strArtist").get_asString();
		song.strAlbum     = m_pDS->fv("album.strAlbum").get_asString();
		song.strGenre     = m_pDS->fv("genre.strGenre").get_asString();
		song.iTrack       = m_pDS->fv("song.iTrack").get_asLong() ;
		song.iDuration    = m_pDS->fv("song.iDuration").get_asLong() ;
		song.iYear        = m_pDS->fv("song.iYear").get_asLong() ;
		song.strTitle     = m_pDS->fv("song.strTitle").get_asString();
		song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
		song.iTrack       = iCount;
		
		string strFileName = m_pDS->fv("path.strPath").get_asString() ;
		strFileName += m_pDS->fv("song.strFileName").get_asString() ;
		song.strFileName = strFileName;

		songs.push_back(song);
		m_pDS->next();

		iCount++;
	}

	return true;
}

bool CMusicDatabase::IncrTop100CounterByFileName(const CStdString& strFileName1)
{
	CSong song;
	CStdString strFileName=strFileName1;
	RemoveInvalidChars(strFileName);

	CStdString strPath, strFName;
	Split(strFileName, strPath, strFName);

	if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
	char szSQL[1024];
	DWORD dwCRC;
	Crc32 crc;
	crc.Reset();
  crc.Compute(strFileName1.c_str(),strlen(strFileName1.c_str()));
	dwCRC=crc;

	sprintf(szSQL,"select * from song,path where song.idPath=path.idPath and dwFileNameCRC='%ul' and strPath='%s'",
									dwCRC,
									strPath.c_str());
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;

	int idSong        = m_pDS->fv("song.idSong").get_asLong();
	int iTimesPlayed  = m_pDS->fv("song.iTimesPlayed").get_asLong();

	sprintf(szSQL,"update song set iTimesPlayed=%i where idSong=%i",
								++iTimesPlayed, idSong);
	try
	{
		m_pDS->exec(szSQL);
	}
	catch(...)
	{
		OutputDebugString("-------ERROR----------------");
	}

	return true;
}

bool CMusicDatabase::GetSongsByPath(const CStdString& strPath1, VECSONGS& songs)
{
  songs.erase(songs.begin(),songs.end());
	CStdString strPath=strPath1;
	RemoveInvalidChars(strPath);
	songs.erase(songs.begin(), songs.end());
	if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from song,album,genre,artist,path where song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre and song.idArtist=artist.idArtist and path.strPath like '%s'",strPath.c_str() );
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;
	while (!m_pDS->eof()) 
	{
		CSong song;
		song.strArtist    = m_pDS->fv("artist.strArtist").get_asString();
		song.strAlbum     = m_pDS->fv("album.strAlbum").get_asString();
		song.strGenre     = m_pDS->fv("genre.strGenre").get_asString();
		song.iTrack       = m_pDS->fv("song.iTrack").get_asLong() ;
		song.iDuration    = m_pDS->fv("song.iDuration").get_asLong() ;
		song.iYear        = m_pDS->fv("song.iYear").get_asLong() ;
		song.strTitle     = m_pDS->fv("song.strTitle").get_asString();
		song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
		
		string strFileName = m_pDS->fv("path.strPath").get_asString() ;
		strFileName += m_pDS->fv("song.strFileName").get_asString() ;
		song.strFileName = strFileName;

		songs.push_back(song);
		m_pDS->next();
	}

	return true;
}

void CMusicDatabase::BeginTransaction()
{
	if (NULL!=m_pDB.get())
		m_pDB->start_transaction();
}

void CMusicDatabase::CommitTransaction()
{
	if (NULL!=m_pDB.get())
		m_pDB->commit_transaction();
}

void CMusicDatabase::RollbackTransaction()
{
	if (NULL!=m_pDB.get())
		m_pDB->rollback_transaction();
}

bool CMusicDatabase::InTransaction()
{
	if (NULL!=m_pDB.get()) return false;
	return m_pDB->in_transaction();
}
