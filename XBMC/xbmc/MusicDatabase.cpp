#include ".\musicdatabase.h"
#include "settings.h"

CMusicDatabase::CMusicDatabase(void)
{
	m_pDB=NULL;
	m_pDS=NULL;
}

CMusicDatabase::~CMusicDatabase(void)
{
}

void CMusicDatabase::RemoveInvalidChars(string& strTxt)
{
	string strReturn="";
	for (int i=0; i < (int)strTxt.size(); ++i)
	{
		byte k=strTxt[i];
		if (k==0x27) 
		{
			strReturn += k;
		}
		strReturn += k;
	}
	strTxt=strReturn;
}


bool CMusicDatabase::Open()
{
#if 1
	Close();

	// test id dbs already exists, if not we need 2 create the tables
	string strDbs=g_stSettings.m_szAlbumDirectory;
	strDbs+="\\MyMusic.db";
	bool bDatabaseExists=false;
	FILE* fd= fopen(strDbs.c_str(),"rb");
	if (fd)
	{
		bDatabaseExists=true;
		fclose(fd);
	}

	m_pDB = new SqliteDatabase();
  m_pDB->setDatabase(strDbs.c_str());
	
  m_pDS = m_pDB->CreateDataset();
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
#endif
	return true;
}


void CMusicDatabase::Close()
{
#if 1
	if (!m_pDB) return;
	m_pDB->disconnect();
	delete m_pDB;
	m_pDB=NULL;
#endif
}

bool CMusicDatabase::CreateTables()
{
#if 1
  try 
	{
    m_pDB->start_transaction();
    m_pDS->exec("CREATE TABLE artist ( idArtist integer primary key, strArtist text)\n");
    m_pDS->exec("CREATE TABLE album ( idAlbum integer primary key, strAlbum text)\n");
		m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");
		m_pDS->exec("CREATE TABLE song ( idSong integer primary key, idArtist integer, idAlbum integer, idGenre integer, strTitle text, iTrack integer, iDuration integer, iYear integer, strFileName text)\n");
		m_pDS->exec("CREATE TABLE albuminfo ( idAlbumInfo integer primary key, idAlbum integer, idArtist integer,iYear integer, idGenre integer, strTones text, strStyles text, strReview text, strImage text, iRating integer)\n");

    m_pDB->commit_transaction();
  }
  catch (...) 
	{ 
		m_pDB->rollback_transaction(); 
	return false;
	}
#endif
	return true;
}

void CMusicDatabase::AddSong(const CSong& song1)
{
	CSong song=song1;
	RemoveInvalidChars(song.strAlbum);
	RemoveInvalidChars(song.strGenre);
	RemoveInvalidChars(song.strArtist);
	RemoveInvalidChars(song.strFileName);
	RemoveInvalidChars(song.strTitle);
#if 1
	if (NULL==m_pDB) return ;
	if (NULL==m_pDS) return ;
	long lAlbumId  = AddAlbum(song.strAlbum);
	long lGenreId  = AddGenre(song.strGenre);
	long lArtistId = AddArtist(song.strArtist);

	char szSQL[1024];
	sprintf(szSQL,"select * from song where idAlbum=%i AND idGenre=%i AND idArtist=%i AND strTitle='%s'", lAlbumId,lGenreId,lArtistId,song.strTitle.c_str());
	if (!m_pDS->query(szSQL)) return;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound!= 0) return ; // already exists

	sprintf(szSQL,"insert into song (idSong,idArtist,idAlbum,idGenre,strTitle,iTrack,iDuration,iYear,strFileName) values(NULL,%i,%i,%i,'%s',%i,%i,%i,'%s')",
				lArtistId,lAlbumId,lGenreId,song.strTitle.c_str(),song.iTrack,song.iDuration,song.iYear,song.strFileName.c_str());
	m_pDS->exec(szSQL);
	
#endif
}


long CMusicDatabase::AddAlbum(const string& strAlbum1)
{
	string strAlbum=strAlbum1;
	RemoveInvalidChars(strAlbum);
#if 1
	if (NULL==m_pDB) return -1;
	if (NULL==m_pDS) return -1;
	string strSQL="select * from album where strAlbum='";
	strSQL += strAlbum;
	strSQL += "'";
	m_pDS->query(strSQL.c_str());
	if (m_pDS->num_rows() == 0) 
	{
		// doesnt exists, add it
		strSQL = "insert into album (idAlbum, strAlbum) values( NULL, '" ;
		strSQL +=strAlbum;
		strSQL +="')";
		m_pDS->exec(strSQL.c_str());
		long lAlbumID=sqlite_last_insert_rowid(m_pDB->getHandle());
		return lAlbumID;
	}
	else
	{
		const field_value value = m_pDS->fv("idAlbum");
		long lAlbumID=value.get_asLong() ;
		return lAlbumID;
	}

#endif
	return -1;
}

long CMusicDatabase::AddGenre(const string& strGenre1)
{
	string strGenre=strGenre1;
	RemoveInvalidChars(strGenre);
#if 1
	if (NULL==m_pDB) return -1;
	if (NULL==m_pDS) return -1;
	string strSQL="select * from genre where strGenre='";
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
#endif
	return -1;
}

long CMusicDatabase::AddArtist(const string& strArtist1)
{
	string strArtist=strArtist1;
	RemoveInvalidChars(strArtist);
#if 1
	if (NULL==m_pDB) return -1;
	if (NULL==m_pDS) return -1;
	string strSQL="select * from Artist where strArtist='";
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

#endif
	return -1;
}

bool CMusicDatabase::GetSongByFileName(const string& strFileName1, CSong& song)
{
	string strFileName=strFileName1;
	RemoveInvalidChars(strFileName);
#if 1
	if (NULL==m_pDB) return false;
	if (NULL==m_pDS) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from song,album,genre,artist where song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre and song.idArtist=artist.idArtist and strFileName='%s'",strFileName.c_str() );
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
	song.strFileName= m_pDS->fv("song.strFileName").get_asString() ;

#endif
	return true;
}

bool CMusicDatabase::GetSong(const string& strTitle1, CSong& song)
{
	string strTitle=strTitle1;
	RemoveInvalidChars(strTitle);
#if 1
	if (NULL==m_pDB) return false;
	if (NULL==m_pDS) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from song,album,genre,artist where song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre and song.idArtist=artist.idArtist and strTitle='%s'",strTitle.c_str() );
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
	song.strFileName= m_pDS->fv("song.strFileName").get_asString() ;
#endif
	return true;
}

bool CMusicDatabase::GetSongsByArtist(const string strArtist1, VECSONGS& songs)
{	
	string strArtist=strArtist1;
	RemoveInvalidChars(strArtist);
#if 1
	songs.erase(songs.begin(), songs.end());
	if (NULL==m_pDB) return false;
	if (NULL==m_pDS) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from song,album,genre,artist where song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre and song.idArtist=artist.idArtist and artist.strArtist='%s'",strArtist.c_str() );
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
		song.strFileName= m_pDS->fv("song.strFileName").get_asString() ;
		songs.push_back(song);
		m_pDS->next();
	}
#endif
	return true;
}

bool CMusicDatabase::GetSongsByAlbum(const string& strAlbum1, VECSONGS& songs)
{
	string strAlbum=strAlbum1;
	RemoveInvalidChars(strAlbum);
#if 1
	songs.erase(songs.begin(), songs.end());
	if (NULL==m_pDB) return false;
	if (NULL==m_pDS) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from song,album,genre,artist where song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre and song.idArtist=artist.idArtist and album.strAlbum='%s'",strAlbum.c_str() );
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
		song.strFileName= m_pDS->fv("song.strFileName").get_asString() ;
		songs.push_back(song);
		m_pDS->next();
	}
#endif
	return true;
}

bool CMusicDatabase::GetArtists(VECARTISTS& artists)
{
#if 1
	artists.erase(artists.begin(), artists.end());
	if (NULL==m_pDB) return false;
	if (NULL==m_pDS) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from artist " );
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;
	while (!m_pDS->eof()) 
	{
		string strArtist = m_pDS->fv("strArtist").get_asString();
		artists.push_back(strArtist);
		m_pDS->next();
	}
#endif
	return true;
}

bool CMusicDatabase::GetAlbums(VECALBUMS& albums)
{
#if 1
	albums.erase(albums.begin(), albums.end());
	if (NULL==m_pDB) return false;
	if (NULL==m_pDS) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from album" );
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;
	while (!m_pDS->eof()) 
	{
		CAlbum album;
		album.strAlbum  = m_pDS->fv("strAlbum").get_asString();
		albums.push_back(album);
		m_pDS->next();
	}
#endif
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
#if 1
	if (NULL==m_pDB) return -1;
	if (NULL==m_pDS) return -1;
	long lAlbumId  = AddAlbum(album.strAlbum);
	long lGenreId  = AddGenre(album.strGenre);
	long lArtistId = AddArtist(album.strArtist);

	char szSQL[1024];
	sprintf(szSQL,"select * from albuminfo where idAlbum=%i AND idGenre=%i AND idArtist=%i ", lAlbumId,lGenreId,lArtistId);
	if (!m_pDS->query(szSQL)) return -1;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound!= 0) 
	{
		const field_value value = m_pDS->fv("idAlbumInfo");
		long lAlbumID=value.get_asLong() ;
		return lAlbumID;
	}

	sprintf(szSQL,"insert into albuminfo (idAlbumInfo,idAlbum,idArtist,idGenre,strTones,strStyles,strReview,strImage,iRating,iYear) values(NULL,%i,%i,%i,'%s','%s','%s','%s',%i,%i)",
											lAlbumId,lArtistId,lGenreId,
											album.strTones.c_str(),
											album.strStyles.c_str(),
											album.strReview.c_str(),
											album.strImage.c_str(),
											album.iRating,
											album.iYear);
	m_pDS->exec(szSQL);
	long lAlbumInfoId=sqlite_last_insert_rowid(m_pDB->getHandle());
	return lAlbumInfoId;
#endif
	return -1;
}

bool CMusicDatabase::GetSongsByGenre(const string& strGenre, VECSONGS& songs)
{
	string strSQLGenre=strGenre;
	RemoveInvalidChars(strSQLGenre);
#if 1
	songs.erase(songs.begin(), songs.end());
	if (NULL==m_pDB) return false;
	if (NULL==m_pDS) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from song,album,genre,artist where song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre and song.idArtist=artist.idArtist and genre.strGenre='%s'",strSQLGenre.c_str() );
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
		song.strFileName= m_pDS->fv("song.strFileName").get_asString() ;
		songs.push_back(song);
		m_pDS->next();
	}
#endif
	return true;
}

bool CMusicDatabase::GetGenres(VECGENRES& genres)
{
#if 1
	genres.erase(genres.begin(), genres.end());
	if (NULL==m_pDB) return false;
	if (NULL==m_pDS) return false;
	char szSQL[1024];
	sprintf(szSQL,"select * from genre " );
	if (!m_pDS->query(szSQL)) return false;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0) return false;
	while (!m_pDS->eof()) 
	{
		string strGenre = m_pDS->fv("strGenre").get_asString();
		genres.push_back(strGenre);
		m_pDS->next();
	}
#endif
	return true;
}
