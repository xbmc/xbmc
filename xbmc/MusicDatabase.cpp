
#include "stdafx.h"
#include ".\musicdatabase.h"
#include "settings.h"
#include "StdString.h"
#include "crc32.h"
#include "LocalizeStrings.h"
#include "util.h"
#include "utils/log.h"
#include "musicInfoTag.h"

CSong::CSong(CMusicInfoTag& tag)
{
	SYSTEMTIME stTime;
	tag.GetReleaseDate(stTime);
	strTitle		= tag.GetTitle();
	strGenre		= tag.GetGenre();
	strFileName	= tag.GetURL();
	strArtist		= tag.GetArtist();
	strAlbum		= tag.GetAlbum();
	iYear				=	stTime.wYear;
	iTrack			= tag.GetTrackNumber();
	iDuration		= tag.GetDuration();
	iStartOffset	= 0;
	iEndOffset		= 0;
}

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
	iStartOffset=0;
	iEndOffset=0;
}

CMusicDatabase	g_musicDatabase;

CMusicDatabase::CMusicDatabase(void)
{
	m_bOpen=false;
	m_iRefCount=0;
}

CMusicDatabase::~CMusicDatabase(void)
{
	m_iRefCount=1;
	Close();
	EmptyCache();
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

	CStdString musicDatabase=g_stSettings.m_szAlbumDirectory;
	musicDatabase+="\\MyMusic55.db";

	if (IsOpen())
	{
		m_iRefCount++;
		return true;
	}

	// test id dbs already exists, if not we need 2 create the tables
	bool bDatabaseExists=false;
	FILE* fd= fopen(musicDatabase.c_str(),"rb");
	if (fd)
	{
		bDatabaseExists=true;
		fclose(fd);
	}

	m_pDB.reset(new SqliteDatabase() ) ;
	m_pDB->setDatabase(musicDatabase.c_str());
	
	m_pDS.reset(m_pDB->CreateDataset());
	if ( m_pDB->connect() != DB_CONNECTION_OK) 
	{
    CLog::Log(LOGERROR, "musicdatabase::unable to open %s (old version?)",musicDatabase.c_str());
		Close();
    ::DeleteFile(musicDatabase.c_str());
		return false;
	}

	if (!bDatabaseExists) 
	{
		if (!CreateTables()) 
		{
      CLog::Log(LOGERROR, "musicdatabase::unable to create %s ",musicDatabase.c_str());
			Close();
      ::DeleteFile(musicDatabase.c_str());
			return false;
		}
	}

	m_pDS->exec("PRAGMA cache_size=8192\n");
	m_pDS->exec("PRAGMA synchronous='OFF'\n");
	m_pDS->exec("PRAGMA count_changes='OFF'\n");
//	m_pDS->exec("PRAGMA temp_store='MEMORY'\n");
	m_bOpen=true;
	m_iRefCount++;
	return true;
}

bool CMusicDatabase::IsOpen()
{
	return m_bOpen;
}

void CMusicDatabase::Close()
{
	if (!m_bOpen)
		return;

	if (m_iRefCount>1)
	{
		m_iRefCount--;
		return;
	}

	m_iRefCount--;
	m_bOpen=false;
	if (NULL==m_pDB.get() ) return;
	m_pDB->disconnect();
	m_pDB.reset();
	//EmptyCache();
}

bool CMusicDatabase::CreateTables()
{
	try 
	{
    CLog::Log(LOGINFO, "create artist table");
		m_pDS->exec("CREATE TABLE artist ( idArtist integer primary key, strArtist text)\n");	
    CLog::Log(LOGINFO, "create album table");
		m_pDS->exec("CREATE TABLE album ( idAlbum integer primary key, strAlbum text, strArtist integer, idPath integer)\n");
    CLog::Log(LOGINFO, "create genre table");
		m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");
    CLog::Log(LOGINFO, "create path table");
		m_pDS->exec("CREATE TABLE path ( idPath integer primary key,  strPath text)\n");
    CLog::Log(LOGINFO,"create song table");
		m_pDS->exec("CREATE TABLE song ( idSong integer primary key, idAlbum integer, idPath integer, strArtist text, strGenre text, strTitle text, iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer)\n");
    CLog::Log(LOGINFO,"create albuminfo table");
		m_pDS->exec("CREATE TABLE albuminfo ( idAlbumInfo integer primary key, idAlbum integer, strArtist text,iYear integer, strGenre text, strTones text, strStyles text, strReview text, strImage text, iRating integer)\n");

	CLog::Log(LOGINFO,"create song2artist table");
		m_pDS->exec("CREATE TABLE song2artist ( idSong2Artist integer primary key, idSong integer, idArtist integer)\n");
	CLog::Log(LOGINFO,"create song2genre table");
		m_pDS->exec("CREATE TABLE song2genre ( idSong2Genre integer primary key, idSong integer, idGenre integer)\n");
	CLog::Log(LOGINFO,"create album2artist table");
		m_pDS->exec("CREATE TABLE album2artist ( idAlbum2Artist integer primary key, idAlbum integer, idArtist integer)\n");
	CLog::Log(LOGINFO,"create album2genre table");
		m_pDS->exec("CREATE TABLE album2genre ( idAlbum2Genre integer primary key, idAlbum integer, idGenre integer)\n");

	CLog::Log(LOGINFO,"create song2artist index");
		m_pDS->exec("CREATE INDEX idxSong2Artist ON song2artist(idSong)");
	CLog::Log(LOGINFO,"create song2genre index");
		m_pDS->exec("CREATE INDEX idxSong2Genre ON song2genre(idSong)");
	CLog::Log(LOGINFO,"create album2artist index");
		m_pDS->exec("CREATE INDEX idxAlbum2Artist ON album2artist(idAlbum)");
	CLog::Log(LOGINFO,"create album2genre index");
		m_pDS->exec("CREATE INDEX idxAlbum2Genre ON album2genre(idAlbum)");

	CLog::Log(LOGINFO, "create album index");
		m_pDS->exec("CREATE INDEX idxAlbum ON album(strAlbum)");
    CLog::Log(LOGINFO, "create genre index");
		m_pDS->exec("CREATE INDEX idxGenre ON genre(strGenre)");
    CLog::Log(LOGINFO, "create artist index");
		m_pDS->exec("CREATE INDEX idxArtist ON artist(strArtist)");
    CLog::Log(LOGINFO, "create path index");
		m_pDS->exec("CREATE INDEX idxPath ON path(strPath)");
		//m_pDS->exec("CREATE INDEX idxSong ON song(dwFileNameCRC)");
	}
	catch (...) 
	{ 
    CLog::Log(LOGERROR, "musicbase::unable to create tables:%i",GetLastError());
		return false;
	}

	return true;
}

void CMusicDatabase::AddSong(const CSong& song1, bool bCheck)
{
	CStdString strSQL;
	try
	{
		CSong song=song1;
		RemoveInvalidChars(song.strAlbum);
		RemoveInvalidChars(song.strGenre);
		RemoveInvalidChars(song.strArtist);
		RemoveInvalidChars(song.strTitle);

		CStdString strPath, strFileName;
		CUtil::Split(song1.strFileName, strPath, strFileName); 
		RemoveInvalidChars(strFileName);

		if (NULL==m_pDB.get()) return;
		if (NULL==m_pDS.get()) return;
		long lPathId   = AddPath(strPath);
		long lAlbumId  = AddAlbum(song1.strAlbum,song1.strArtist,lPathId,strPath);

		DWORD dwCRC;
		Crc32 crc;
		crc.Reset();
		crc.Compute(song1.strFileName.c_str(),strlen(song1.strFileName.c_str()));
		dwCRC=crc;

		bool bInsert = true;
		int lSongId = -1;
		if (bCheck)
		{
			strSQL.Format("select * from song where idAlbum=%i AND strGenre='%s' AND strArtist='%s' AND dwFileNameCRC='%ul' AND strTitle='%s'", 
					lAlbumId,song.strGenre.c_str(),song.strArtist.c_str(),dwCRC,song.strTitle.c_str());
			if (!m_pDS->query(strSQL.c_str())) return;
			if (m_pDS->num_rows() != 0)
			{
				lSongId = m_pDS->fv("idSong").get_asLong();
				bInsert = false;
			}
		}
		if (bInsert)
		{
			strSQL.Format("insert into song (idSong,idAlbum,idPath,strArtist,strGenre,strTitle,iTrack,iDuration,iYear,dwFileNameCRC,strFileName,iTimesPlayed,iStartOffset,iEndOffset) values(NULL,%i,%i,'%s','%s','%s',%i,%i,%i,'%ul','%s',%i,%i,%i)",
										lAlbumId,lPathId,song.strArtist.c_str(),song.strGenre.c_str(),
										song.strTitle.c_str(),
										song.iTrack,song.iDuration,song.iYear,
										dwCRC,
										strFileName.c_str(), 0, song.iStartOffset, song.iEndOffset);

			m_pDS->exec(strSQL.c_str());
			lSongId = sqlite_last_insert_rowid(m_pDB->getHandle());
		}
		
		// add artists and genres
		AddArtists(song.strArtist, lSongId, lAlbumId, bCheck);
		AddGenres(song.strGenre, lSongId, lAlbumId, bCheck);
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:unable to addsong (%s)", strSQL.c_str());
	}
}

long CMusicDatabase::AddAlbum(const CStdString& strAlbum1, const CStdString &strArtist1, long lPathId, const CStdString& strPath)
{
	CStdString strSQL;
	try
	{
		CStdString strAlbum=strAlbum1;
		CStdString strArtist=strArtist1;
		RemoveInvalidChars(strAlbum);
		RemoveInvalidChars(strArtist);

		if (NULL==m_pDB.get()) return -1;
		if (NULL==m_pDS.get()) return -1;

		map <CStdString, CAlbumCache>::const_iterator it;

		it=m_albumCache.find(strAlbum1);
		if (it!=m_albumCache.end())
			return it->second.idAlbum;

		strSQL.Format("select * from album where idPath=%i and strAlbum like '%s'", lPathId, strAlbum);
		m_pDS->query(strSQL.c_str());

		if (m_pDS->num_rows() == 0) 
		{
			// doesnt exists, add it
			strSQL.Format("insert into album (idAlbum, strAlbum, strArtist,idPath) values( NULL, '%s', '%s', %i)", strAlbum, strArtist, lPathId);
			m_pDS->exec(strSQL.c_str());

			CAlbumCache album;
			album.idAlbum  = sqlite_last_insert_rowid(m_pDB->getHandle());
			album.strPath  = strPath;
			album.idPath   = lPathId;
			album.strAlbum = strAlbum1;
			album.strArtist = strArtist1;
			m_albumCache.insert(pair<CStdString, CAlbumCache>(album.strAlbum+album.strPath, album));
			return album.idAlbum;
		}
		else
		{
			CAlbumCache album;
			album.idAlbum  = m_pDS->fv("idAlbum").get_asLong();
			album.strPath  = strPath;
			album.idPath   = lPathId;
			album.strAlbum = strAlbum1;
			album.strArtist = strArtist1;
			m_albumCache.insert(pair<CStdString, CAlbumCache>(album.strAlbum+album.strPath, album));
			return album.idAlbum;
		}
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:unable to addalbum (%s)", strSQL.c_str());
	}

	return -1;
}

void CMusicDatabase::CheckVariousArtistsAndCoverArt()
{
	if (m_albumCache.size()<=0)
		return;

	map <CStdString, CAlbumCache>::const_iterator it;

	VECSONGS songs;
	for (it=m_albumCache.begin(); it!=m_albumCache.end(); ++it)
	{
		CAlbumCache album=it->second;
		long lAlbumId=album.idAlbum;
		bool bVarious=false;
		CStdString strVariousArtists;
		GetSongsByAlbum(album.strAlbum, album.strPath, songs);
		if (songs.size()>1)
		{
			//	Are the artists of this album all the same
			for (int i=0; i < (int)songs.size()-1; i++)
			{
				CSong song=songs[i];
				CSong song1=songs[i+1];
				if (song.strArtist!=song1.strArtist)
				{
					strVariousArtists=g_localizeStrings.Get(340);
//					AddArtist(strVariousArtists);
					bVarious=true;
					break;
				}
			}
		}

		if (bVarious)
		{
			CStdString strSQL;
			strSQL.Format("update album set strArtist='%s' where idAlbum=%i", strVariousArtists, album.idAlbum);
			m_pDS->exec(strSQL.c_str());
		}

		CStdString strTempCoverArt;
		CStdString strCoverArt;
		CUtil::GetAlbumThumb(album.strAlbum+album.strPath, strTempCoverArt, true);
		//	Was the album art of this album read during scan?
		if (CUtil::ThumbCached(strTempCoverArt))
		{
			//	Yes.
			VECALBUMS albums;
			GetAlbumsByPath(album.strPath, albums);
			CUtil::GetAlbumThumb(album.strPath, strCoverArt);
			//	Do we have more then one album in this directory...
			if (albums.size()==1)
			{
				//	...no, copy as permanent directory thumb
				if (::CopyFile(strTempCoverArt, strCoverArt, false))
					CUtil::ThumbCacheAdd(strCoverArt, true);
			}
			else if (CUtil::ThumbCached(strCoverArt))
			{
				//	...yes, we have more then one album in this directory
				//	and have saved a thumb for this directory from another album
				//	so delete the directory thumb.
				if (CUtil::ThumbExists(strCoverArt))
				{
					if (::DeleteFile(strCoverArt))
						CUtil::ThumbCacheAdd(strCoverArt, false);
				}
			}
			
			//	And move as permanent thumb for files and directory, where
			//	album and path is known
			CUtil::GetAlbumThumb(album.strAlbum+album.strPath, strCoverArt);
			::MoveFileEx(strTempCoverArt, strCoverArt, MOVEFILE_REPLACE_EXISTING);
		}
	}

	m_albumCache.erase(m_albumCache.begin(), m_albumCache.end());
}

long CMusicDatabase::AddGenre(const CStdString& strGenre1)
{
	CStdString strSQL;
	try
	{
		CStdString strGenre=strGenre1;
		RemoveInvalidChars(strGenre);

		if (NULL==m_pDB.get()) return -1;
		if (NULL==m_pDS.get()) return -1;
		map <CStdString, CGenreCache>::const_iterator it;

		it=m_genreCache.find(strGenre1);
		if (it!=m_genreCache.end())
			return it->second.idGenre;

		strSQL.Format("select * from genre where strGenre like '%s'", strGenre);
		m_pDS->query(strSQL.c_str());
		if (m_pDS->num_rows() == 0) 
		{
			// doesnt exists, add it
			strSQL.Format("insert into genre (idGenre, strGenre) values( NULL, '%s' )", strGenre);
			m_pDS->exec(strSQL.c_str());

			CGenreCache genre;
			genre.idGenre = sqlite_last_insert_rowid(m_pDB->getHandle());
			genre.strGenre = strGenre1;
			m_genreCache.insert(pair<CStdString, CGenreCache>(genre.strGenre, genre));
			return genre.idGenre;
		}
		else
		{
			CGenreCache genre;
			genre.idGenre = m_pDS->fv("idGenre").get_asLong();
			genre.strGenre = strGenre1;
			m_genreCache.insert(pair<CStdString, CGenreCache>(genre.strGenre, genre));
			return genre.idGenre;
		}
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:unable to addgenre (%s)", strSQL.c_str());
	}

	return -1;
}

long CMusicDatabase::AddArtist(const CStdString& strArtist1)
{
	CStdString strSQL;
	try 
	{
		CStdString strArtist=strArtist1;
		RemoveInvalidChars(strArtist);

		if (NULL==m_pDB.get()) return -1;
		if (NULL==m_pDS.get()) return -1;

		map <CStdString, CArtistCache>::const_iterator it;

		it=m_artistCache.find(strArtist1);
		if (it!=m_artistCache.end())
			return it->second.idArtist;

		strSQL.Format("select * from artist where strArtist like '%s'", strArtist);
		m_pDS->query(strSQL.c_str());

		if (m_pDS->num_rows() == 0) 
		{
			// doesnt exists, add it
			strSQL .Format("insert into artist (idArtist, strArtist) values( NULL, '%s' )", strArtist);
			m_pDS->exec(strSQL.c_str());
			CArtistCache artist;
			artist.idArtist = sqlite_last_insert_rowid(m_pDB->getHandle());
			artist.strArtist = strArtist1;
			m_artistCache.insert(pair<CStdString, CArtistCache>(artist.strArtist, artist));
			return artist.idArtist;
		}
		else
		{
			CArtistCache artist;
			artist.idArtist = m_pDS->fv("idArtist").get_asLong();
			artist.strArtist = strArtist1;
			m_artistCache.insert(pair<CStdString, CArtistCache>(artist.strArtist, artist));
			return artist.idArtist;
		}
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:unable to addartist (%s)", strSQL.c_str());
	}

	return -1;
}

void CMusicDatabase::AddArtists(const CStdString &strArtists, long lSongId, long lAlbumId, bool bCheck)
{
	// split the possibly multi-artist string into individual artists
	VECLONGS vecIds;
	CStdStringArray vecArtists;
	StringUtils::SplitString(strArtists, "/", vecArtists);
	for (int i=0; i<(int)vecArtists.size(); i++)
	{
		long lArtistId = AddArtist(vecArtists[i]);
		if (lArtistId>=0)
		{
			AddArtistLinks(lArtistId, lSongId, lAlbumId);
		}
	}
}

void CMusicDatabase::AddGenres(const CStdString &strGenres, long lSongId, long lAlbumId, bool bCheck)
{
	// split the possibly multi-genre string into individual genres
	VECLONGS vecIds;
	CStdStringArray vecGenres;
	StringUtils::SplitString(strGenres, "/", vecGenres);
	for (int i=0; i<(int)vecGenres.size(); i++)
	{
		long lGenreId = AddGenre(vecGenres[i]);
		if (lGenreId>=0)
			AddGenreLinks(lGenreId, lSongId, lAlbumId);
	}
}

long CMusicDatabase::AddPath(const CStdString& strPath1)
{
	CStdString strSQL;
	try
	{
		CStdString strPath=strPath1;
		//	musicdatabase always stores directories 
		//	without a slash at the end 
		if (CUtil::HasSlashAtEnd(strPath))
			strPath.Delete(strPath.size()-1);
		RemoveInvalidChars(strPath);

		if (NULL==m_pDB.get()) return -1;
		if (NULL==m_pDS.get()) return -1;

		map <CStdString, CPathCache>::const_iterator it;

		it=m_pathCache.find(strPath1);
		if (it!=m_pathCache.end())
			return it->second.idPath;

		strSQL.Format( "select * from path where strPath like '%s'", strPath);
		m_pDS->query(strSQL.c_str());
		if (m_pDS->num_rows() == 0) 
		{
			// doesnt exists, add it
			strSQL.Format("insert into path (idPath, strPath) values ( NULL, '%s' )", strPath);
			m_pDS->exec(strSQL.c_str());

			CPathCache path;
			path.idPath = sqlite_last_insert_rowid(m_pDB->getHandle());
			path.strPath = strPath1;
			m_pathCache.insert(pair<CStdString, CPathCache>(path.strPath, path));
			return path.idPath;
		}
		else
		{
			CPathCache path;
			path.idPath = m_pDS->fv("idPath").get_asLong();
			path.strPath = strPath1;
			m_pathCache.insert(pair<CStdString, CPathCache>(path.strPath, path));
			return path.idPath;
		}
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:unable to addpath (%s)", strSQL.c_str());
	}

	return -1;
}

void CMusicDatabase::AddArtistLinks(long lArtistId, long lSongId, long lAlbumId, bool bCheck)
{
	try
	{
		CStdString strSQL;
		// first link the artist with the song
		bool bInsert = true;
		if (lSongId)
		{
			if (bCheck)
			{
				strSQL.Format("select * from song2artist where idSong=%i AND idArtist=%i", 
						lSongId,lArtistId);
				if (!m_pDS->query(strSQL.c_str())) return;
				if (m_pDS->num_rows() != 0)
					bInsert = false; // already exists
			}
			if (bInsert)
			{
				strSQL.Format("insert into song2artist (idSong2Artist,idSong,idArtist) values(NULL,%i,%i)",
												lSongId,lArtistId);

				m_pDS->exec(strSQL.c_str());
			}
		}
		// now link the artist with the album
		bInsert = true;
		if (bCheck)
		{
			strSQL.Format("select * from album2artist where idAlbum=%i AND idArtist=%i", 
					lAlbumId,lArtistId);
			if (!m_pDS->query(strSQL.c_str())) return;
			if (m_pDS->num_rows() != 0)
				bInsert = false; // already exists
		}
		if (bInsert)
		{
			strSQL.Format("insert into album2artist (idAlbum2Artist,idAlbum,idArtist) values(NULL,%i,%i)",
											lAlbumId,lArtistId);

			m_pDS->exec(strSQL.c_str());
		}
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "CMusicDatabase:AddArtistLinks(%i,%i,%i) failed", lArtistId, lSongId, lAlbumId);
	}
}

void CMusicDatabase::AddGenreLinks(long lGenreId, long lSongId, long lAlbumId, bool bCheck)
{
	try
	{
		CStdString strSQL;
		// first link the genre with the song
		bool bInsert = true;
		if (lSongId)
		{
			if (bCheck)
			{
				strSQL.Format("select * from song2genre where idSong=%i AND idGenre=%i", 
						lSongId,lGenreId);
				if (!m_pDS->query(strSQL.c_str())) return;
				if (m_pDS->num_rows() != 0)
					bInsert = false; // already exists
			}
			if (bInsert)
			{
				strSQL.Format("insert into song2genre (idSong2Genre,idSong,idGenre) values(NULL,%i,%i)",
												lSongId,lGenreId);

				m_pDS->exec(strSQL.c_str());
			}
		}
		// now link the genre with the album
		bInsert = true;
		if (bCheck)
		{
			strSQL.Format("select * from album2genre where idAlbum=%i AND idGenre=%i", 
					lAlbumId,lGenreId);
			if (!m_pDS->query(strSQL.c_str())) return;
			if (m_pDS->num_rows() != 0)
				bInsert = false; // already exists
		}
		if (bInsert)
		{
			strSQL.Format("insert into album2genre (idAlbum2Genre,idAlbum,idGenre) values(NULL,%i,%i)",
											lAlbumId,lGenreId);

			m_pDS->exec(strSQL.c_str());
		}
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "CMusicDatabase:AddGenreLinks(%i,%i,%i) failed", lGenreId, lSongId, lAlbumId);
	}
}

bool CMusicDatabase::GetSongByFileName(const CStdString& strFileName1, CSong& song)
{
	try
	{
		song.Clear();
		CStdString strFileName=strFileName1;
		RemoveInvalidChars(strFileName);

		CStdString strPath, strFName;
		CUtil::Split(strFileName, strPath, strFName);

		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;

		Crc32 crc;
		crc.Reset();
		crc.Compute(strFileName1.c_str(),strlen(strFileName1.c_str()));

		DWORD dwCRC=crc;

		CStdString strSQL;
		strSQL.Format("select * from song,album,path where song.idPath=path.idPath and song.idAlbum=album.idAlbum and dwFileNameCRC='%ul' and strPath='%s'",
										dwCRC,
										strPath.c_str());

		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;

		song.strArtist    = m_pDS->fv("song.strArtist").get_asString();
		song.strAlbum     = m_pDS->fv("album.strAlbum").get_asString();
		song.strGenre     = m_pDS->fv("song.strGenre").get_asString();
		song.iTrack       = m_pDS->fv("song.iTrack").get_asLong() ;
		song.iDuration    = m_pDS->fv("song.iDuration").get_asLong() ;
		song.iYear        = m_pDS->fv("song.iYear").get_asLong() ;
		song.strTitle     = m_pDS->fv("song.strTitle").get_asString() ;
		song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
		song.iStartOffset = m_pDS->fv("song.iStartOffset").get_asLong();
		song.iEndOffset	  = m_pDS->fv("song.iEndOffset").get_asLong();
		song.strFileName  = strFileName1;
		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetSongByFileName(%s) failed", strFileName1.c_str());
	}

	return false;
}

bool CMusicDatabase::GetSong(const CStdString& strTitle1, CSong& song)
{
	try
	{
		song.Clear();
		CStdString strTitle=strTitle1;
		RemoveInvalidChars(strTitle);

		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;

		CStdString strSQL;
		strSQL.Format("select * from song,album,genre,path where song.idPath=path.idPath and song.idAlbum=album.idAlbum and strTitle='%s'",strTitle.c_str() );

		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;

		song.strArtist = m_pDS->fv("song.strArtist").get_asString();
		song.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
		song.strGenre  = m_pDS->fv("song.strGenre").get_asString();
		song.iTrack    = m_pDS->fv("song.iTrack").get_asLong();
		song.iDuration = m_pDS->fv("song.iDuration").get_asLong();
		song.iYear     = m_pDS->fv("song.iYear").get_asLong();
		song.strTitle  = m_pDS->fv("song.strTitle").get_asString();
		song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
		song.iStartOffset = m_pDS->fv("song.iStartOffset").get_asLong();
		song.iEndOffset	  = m_pDS->fv("song.iEndOffset").get_asLong();

		CStdString strFileName = m_pDS->fv("path.strPath").get_asString();
		strFileName += m_pDS->fv("song.strFileName").get_asString();
		song.strFileName = strFileName;
		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetSong(%s) failed", strTitle1.c_str());
	}

	return false;
}

bool CMusicDatabase::GetSongsByArtist(const CStdString strArtist1, VECSONGS& songs)
{	
	try
	{
		CStdString strArtist=strArtist1;
		RemoveInvalidChars(strArtist);

		songs.erase(songs.begin(), songs.end());
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;

		// find the artist iD (should be fast)
		CStdString strSQL;
		strSQL.Format("select * from song,song2artist,album,path,artist where artist.strArtist='%s' and song2artist.idArtist=artist.idArtist and song.idSong=song2artist.idSong and song.idPath=path.idPath and song.idAlbum=album.idAlbum",strArtist.c_str() );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof()) 
		{
			CSong song;
			song.strArtist = m_pDS->fv("song.strArtist").get_asString();
			song.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
			song.strGenre  = m_pDS->fv("song.strGenre").get_asString();
			song.iTrack    = m_pDS->fv("song.iTrack").get_asLong();
			song.iDuration = m_pDS->fv("song.iDuration").get_asLong();
			song.iYear     = m_pDS->fv("song.iYear").get_asLong();
			song.strTitle  = m_pDS->fv("song.strTitle").get_asString();
			song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
			song.iStartOffset = m_pDS->fv("song.iStartOffset").get_asLong();
			song.iEndOffset = m_pDS->fv("song.iEndOffset").get_asLong();

			CStdString strFileName = m_pDS->fv("path.strPath").get_asString();
			strFileName += m_pDS->fv("song.strFileName").get_asString();
			song.strFileName = strFileName;
			songs.push_back(song);
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetSongsByArtist(%s) failed", strArtist1.c_str());
	}

	return false;
}

bool CMusicDatabase::GetSongsByAlbum(const CStdString& strAlbum1, const CStdString& strPath1, VECSONGS& songs)
{
	try
	{
		CStdString strAlbum=strAlbum1;
		CStdString strPath=strPath1;
		//	musicdatabase always stores directories 
		//	without a slash at the end 
		if (CUtil::HasSlashAtEnd(strPath))
			strPath.Delete(strPath.size()-1);
		RemoveInvalidChars(strAlbum);
		RemoveInvalidChars(strPath);

		songs.erase(songs.begin(), songs.end());
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		strSQL.Format("select * from song,path,album where song.idPath=path.idPath and song.idAlbum=album.idAlbum and album.strAlbum like '%s' and path.strPath like '%s' order by song.iTrack", strAlbum, strPath );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof()) 
		{
			CSong song;
			song.strArtist = m_pDS->fv("song.strArtist").get_asString();
			song.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
			song.strGenre  = m_pDS->fv("song.strGenre").get_asString();
			song.iTrack    = m_pDS->fv("song.iTrack").get_asLong();
			song.iDuration = m_pDS->fv("song.iDuration").get_asLong();
			song.iYear     = m_pDS->fv("song.iYear").get_asLong();
			song.strTitle  = m_pDS->fv("song.strTitle").get_asString();
			song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
			song.iStartOffset = m_pDS->fv("song.iStartOffset").get_asLong();
			song.iEndOffset = m_pDS->fv("song.iEndOffset").get_asLong();

			CStdString strFileName = m_pDS->fv("path.strPath").get_asString();
			strFileName += m_pDS->fv("song.strFileName").get_asString();
			song.strFileName = strFileName;

			songs.push_back(song);
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetSongsByAlbum(%s, %s) failed", strAlbum1.c_str(), strPath1.c_str());
	}

	return false;
}

bool CMusicDatabase::GetArtists(VECARTISTS& artists)
{
	try
	{
		artists.erase(artists.begin(), artists.end());

		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;

		// Exclude "Various Artists"
//		CStdString strVariousArtists=g_localizeStrings.Get(340);
//		long lVariousArtistId=AddArtist(strVariousArtists);
		CStdString strSQL;
		strSQL.Format("select * from artist");// where idArtist <> %i ", lVariousArtistId );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof()) 
		{
			CStdString strArtist = m_pDS->fv("strArtist").get_asString();
			artists.push_back(strArtist);
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetArtists() failed");
	}

	return false;
}

bool CMusicDatabase::GetArtistsByName(const CStdString& strArtist1, VECARTISTS& artists)
{
	try
	{
		CStdString strArtist=strArtist1;
		RemoveInvalidChars(strArtist);
		artists.erase(artists.begin(), artists.end());

		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;

		// Exclude "Various Artists"
//		CStdString strVariousArtists=g_localizeStrings.Get(340);
//		long lVariousArtistId=AddArtist(strVariousArtists);
		CStdString strSQL;
		strSQL.Format("select * from artist where strArtist like '%%%s%%'",strArtist); // and idArtist <> %i ", strArtist, lVariousArtistId );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof()) 
		{
			CStdString strArtist = m_pDS->fv("strArtist").get_asString();
			artists.push_back(strArtist);
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetArtists() failed");
	}

	return false;
}

bool CMusicDatabase::GetAlbums(VECALBUMS& albums)
{
	try
	{
		albums.erase(albums.begin(), albums.end());
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		strSQL.Format("select * from album,path where album.idPath=path.idPath" );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof()) 
		{
			CAlbum album;
			album.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
			album.strArtist = m_pDS->fv("album.strArtist").get_asString();
			album.strPath   = m_pDS->fv("path.strPath").get_asString();
			albums.push_back(album);
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbums() failed");
	}

	return false;
}

bool CMusicDatabase::GetRecentlyPlayedAlbums(VECALBUMS& albums)
{
	try
	{
		albums.erase(albums.begin(), albums.end());
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		strSQL.Format("select distinct album.*, path.* from album,path,song where album.idAlbum=song.idAlbum and album.idPath=path.idPath and song.iTimesPlayed > 0 order by song.iTimesPlayed limit 20" );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof()) 
		{
			CAlbum album;
			album.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
			album.strArtist = m_pDS->fv("album.strArtist").get_asString();
			album.strPath   = m_pDS->fv("path.strPath").get_asString();
			albums.push_back(album);
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetRecentlyPlayedAlbums() failed");
	}

	return false;
}

long CMusicDatabase::AddAlbumInfo(const CAlbum& album1)
{
	CStdString strSQL;
	try
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
		RemoveInvalidChars(album.strPath);

		if (NULL==m_pDB.get()) return -1;
		if (NULL==m_pDS.get()) return -1;
		long lPathId   = AddPath(album1.strPath);
		long lAlbumId  = AddAlbum(album1.strAlbum,album1.strArtist,lPathId,album1.strPath);
		AddArtists(album.strArtist, 0, lAlbumId);
		AddGenres(album.strGenre, 0, lAlbumId);

		strSQL.Format("select * from albuminfo where idAlbum=%i AND strGenre='%s' AND strArtist='%s'", lAlbumId,album.strGenre.c_str(),album.strArtist.c_str());
		if (!m_pDS->query(strSQL.c_str())) return -1;

		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound!= 0) 
			return m_pDS->fv("idAlbumInfo").get_asLong();

		strSQL.Format("insert into albuminfo (idAlbumInfo,idAlbum,strArtist,strGenre,strTones,strStyles,strReview,strImage,iRating,iYear) values(NULL,%i,%i,%i,'%s','%s','%s','%s',%i,%i)",
												lAlbumId,album.strArtist.c_str(),album.strGenre.c_str(),
												album.strTones.c_str(),
												album.strStyles.c_str(),
												album.strReview.c_str(),
												album.strImage.c_str(),
												album.iRating,
												album.iYear);
		m_pDS->exec(strSQL.c_str());

		long lAlbumInfoId=sqlite_last_insert_rowid(m_pDB->getHandle());
		return lAlbumInfoId;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:unable to addalbuminfo (%s)", strSQL.c_str());
	}

	return -1;
}

bool CMusicDatabase::GetSongsByGenre(const CStdString& strGenre, VECSONGS& songs)
{
	try
	{
		CStdString strSQLGenre=strGenre;
		RemoveInvalidChars(strSQLGenre);

		songs.erase(songs.begin(), songs.end());
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		strSQL.Format("select * from song,song2genre,genre,album,path where song.idPath=path.idPath and song.idAlbum=album.idAlbum and song2genre.idGenre=genre.idGenre and song.idSong=song2genre.idSong and genre.strGenre='%s'",strSQLGenre.c_str() );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		DWORD dwStartTime = timeGetTime();
		while (!m_pDS->eof()) 
		{
			CSong song;
			song.strArtist    = m_pDS->fv("song.strArtist").get_asString();
			song.strAlbum     = m_pDS->fv("album.strAlbum").get_asString();
			song.strGenre     = m_pDS->fv("song.strGenre").get_asString();
			song.iTrack       = m_pDS->fv("song.iTrack").get_asLong() ;
			song.iDuration    = m_pDS->fv("song.iDuration").get_asLong() ;
			song.iYear        = m_pDS->fv("song.iYear").get_asLong() ;
			song.strTitle     = m_pDS->fv("song.strTitle").get_asString();
			song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
			song.iStartOffset = m_pDS->fv("song.iStartOffset").get_asLong();
			song.iEndOffset	  = m_pDS->fv("song.iEndOffset").get_asLong();
			
			CStdString strFileName = m_pDS->fv("path.strPath").get_asString() ;
			strFileName += m_pDS->fv("song.strFileName").get_asString() ;
			song.strFileName = strFileName;

			songs.push_back(song);
			m_pDS->next();
		}
		m_pDS->close();	//	cleanup recordset data
		DWORD dwEndTime = timeGetTime();
		CStdString strOut;
		strOut.Format("Time taken for seek = %d", dwEndTime-dwStartTime);
		OutputDebugString(strOut.c_str());
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetSongsByGenre() failed");
	}

	return false;
}

bool CMusicDatabase::GetGenres(VECGENRES& genres)
{
	try
	{
		genres.erase(genres.begin(), genres.end());
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		strSQL.Format("select * from genre");
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof()) 
		{
			CStdString strGenre = m_pDS->fv("strGenre").get_asString();
			genres.push_back(strGenre);
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetGenres() failed");
	}

	return false;
}

bool CMusicDatabase::GetAlbumInfo(const CStdString& strAlbum1, const CStdString& strPath1, CAlbum& album)
{
	try
	{
		CStdString strAlbum = strAlbum1;
		CStdString strPath = strPath1;
		//	musicdatabase always stores directories 
		//	without a slash at the end 
		if (CUtil::HasSlashAtEnd(strPath))
			strPath.Delete(strPath.size()-1);
		RemoveInvalidChars(strAlbum);
		RemoveInvalidChars(strPath);
		CStdString strSQL;
		strSQL.Format("select * from albuminfo,album,path where album.strAlbum like '%s' and path.strPath like '%s' and album.idPath = path.idPath and albuminfo.idAlbum=album.idAlbum",strAlbum, strPath );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound!= 0) 
		{
			album.iRating	= m_pDS->fv("albuminfo.iRating").get_asLong() ;
			album.iYear	= m_pDS->fv("albuminfo.iYear").get_asLong() ;
			album.strAlbum	= m_pDS->fv("album.strAlbum").get_asString();
			album.strArtist	= m_pDS->fv("albuminfo.strArtist").get_asString();
			album.strGenre	= m_pDS->fv("albuminfo.strGenre").get_asString();
			album.strImage	= m_pDS->fv("albuminfo.strImage").get_asString();
			album.strReview	= m_pDS->fv("albuminfo.strReview").get_asString();
			album.strStyles	= m_pDS->fv("albuminfo.strStyles").get_asString();
			album.strTones	= m_pDS->fv("albuminfo.strTones").get_asString();
			album.strPath   = m_pDS->fv("path.strPath").get_asString();
			m_pDS->close();	//	cleanup recordset data
			return true;
		}
		return false;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbumInfo(%s, %s) failed", strAlbum1.c_str(), strPath1.c_str());
	}

	return false;
}

bool CMusicDatabase::GetTop100(VECSONGS& songs)
{
	try
	{
		songs.erase(songs.begin(), songs.end());
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		strSQL.Format("select * from song,album,path where song.idPath=path.idPath and song.idAlbum=album.idAlbum and iTimesPlayed > 0 order by song.iTimesPlayed desc limit 100"  );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		int iCount=1;
		while (!m_pDS->eof()) 
		{
			CSong song;
			song.strArtist    = m_pDS->fv("song.strArtist").get_asString();
			song.strAlbum     = m_pDS->fv("album.strAlbum").get_asString();
			song.strGenre     = m_pDS->fv("song.strGenre").get_asString();
			song.iTrack       = m_pDS->fv("song.iTrack").get_asLong() ;
			song.iDuration    = m_pDS->fv("song.iDuration").get_asLong() ;
			song.iYear        = m_pDS->fv("song.iYear").get_asLong() ;
			song.strTitle     = m_pDS->fv("song.strTitle").get_asString();
			song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
			song.iStartOffset = m_pDS->fv("song.iStartOffset").get_asLong();
			song.iEndOffset = m_pDS->fv("song.iEndOffset").get_asLong();
			song.iTrack       = iCount++;
			
			CStdString strFileName = m_pDS->fv("path.strPath").get_asString() ;
			strFileName += m_pDS->fv("song.strFileName").get_asString() ;
			song.strFileName = strFileName;

			songs.push_back(song);
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetTop100() failed");
	}

	return false;
}

bool CMusicDatabase::IncrTop100CounterByFileName(const CStdString& strFileName1)
{
	try
	{
		CSong song;
		CStdString strFileName=strFileName1;
		RemoveInvalidChars(strFileName);

		CStdString strPath, strFName;
		CUtil::Split(strFileName, strPath, strFName);

		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		DWORD dwCRC;
		Crc32 crc;
		crc.Reset();
		crc.Compute(strFileName1.c_str(),strlen(strFileName1.c_str()));
		dwCRC=crc;

		strSQL.Format("select * from song,path where song.idPath=path.idPath and dwFileNameCRC='%ul' and strPath='%s'",
										dwCRC,
										strPath.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;

		int idSong        = m_pDS->fv("song.idSong").get_asLong();
		int iTimesPlayed  = m_pDS->fv("song.iTimesPlayed").get_asLong();

		strSQL.Format("update song set iTimesPlayed=%i where idSong=%i",
									++iTimesPlayed, idSong);
		m_pDS->exec(strSQL.c_str());
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:IncrTop100Counter(%s) failed", strFileName1.c_str());
	}

	return false;
}

bool CMusicDatabase::GetSongsByPath(const CStdString& strPath1, VECSONGS& songs)
{
	try
	{
		songs.erase(songs.begin(),songs.end());
		CStdString strPath=strPath1;
		//	musicdatabase always stores directories 
		//	without a slash at the end 
		if (CUtil::HasSlashAtEnd(strPath))
			strPath.Delete(strPath.size()-1);
		RemoveInvalidChars(strPath);
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		strSQL.Format("select * from song,path,album where song.idPath=path.idPath and song.idAlbum=album.idAlbum and path.strPath like '%s'",strPath.c_str() );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof()) 
		{
			CSong song;
			song.strArtist    = m_pDS->fv("song.strArtist").get_asString();
			song.strAlbum     = m_pDS->fv("album.strAlbum").get_asString();
			song.strGenre     = m_pDS->fv("song.strGenre").get_asString();
			song.iTrack       = m_pDS->fv("song.iTrack").get_asLong() ;
			song.iDuration    = m_pDS->fv("song.iDuration").get_asLong() ;
			song.iYear        = m_pDS->fv("song.iYear").get_asLong() ;
			song.strTitle     = m_pDS->fv("song.strTitle").get_asString();
			song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
			song.iStartOffset = m_pDS->fv("song.iStartOffset").get_asLong();
			song.iEndOffset = m_pDS->fv("song.iEndOffset").get_asLong();
			
			CStdString strFileName = m_pDS->fv("path.strPath").get_asString() ;
			strFileName += m_pDS->fv("song.strFileName").get_asString() ;
			song.strFileName = strFileName;
			
			songs.push_back(song);
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetVecSongsByPath(%s) failed", strPath1.c_str());
	}

	return false;
}

bool CMusicDatabase::GetSongsByPath(const CStdString& strPath1, MAPSONGS& songs, bool bAppendToMap)
{
	try
	{
		if (!bAppendToMap)
			songs.erase(songs.begin(),songs.end());
		CStdString strPath=strPath1;
		//	musicdatabase always stores directories 
		//	without a slash at the end 
		if (CUtil::HasSlashAtEnd(strPath))
			strPath.Delete(strPath.size()-1);
		RemoveInvalidChars(strPath);
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		strSQL.Format("select song.*, path.strPath, album.strAlbum from song,path,album where song.idPath=path.idPath and song.idAlbum=album.idAlbum and path.strPath like '%s'",strPath.c_str() );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof()) 
		{
			CSong song;
			song.strArtist    = m_pDS->fv("song.strArtist").get_asString();
			song.strAlbum     = m_pDS->fv("album.strAlbum").get_asString();
			song.strGenre     = m_pDS->fv("song.strGenre").get_asString();
			song.iTrack       = m_pDS->fv("song.iTrack").get_asLong() ;
			song.iDuration    = m_pDS->fv("song.iDuration").get_asLong() ;
			song.iYear        = m_pDS->fv("song.iYear").get_asLong() ;
			song.strTitle     = m_pDS->fv("song.strTitle").get_asString();
			song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
			song.iStartOffset = m_pDS->fv("song.iStartOffset").get_asLong();
			song.iEndOffset = m_pDS->fv("song.iEndOffset").get_asLong();
			
			CStdString strFileName = m_pDS->fv("path.strPath").get_asString() ;
			strFileName += m_pDS->fv("song.strFileName").get_asString() ;
			song.strFileName = strFileName;
			
			songs.insert(pair<CStdString, CSong>(song.strFileName, song));
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetMapSongsByPath(%s) failed", strPath1.c_str());
	}

	return false;
}

void CMusicDatabase::BeginTransaction()
{
	try
	{
		if (NULL!=m_pDB.get())
			m_pDB->start_transaction();
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:begintransaction failed");
	}
}

void CMusicDatabase::CommitTransaction()
{
	try
	{
		if (NULL!=m_pDB.get())
			m_pDB->commit_transaction();
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:committransaction failed");
	}
}

void CMusicDatabase::RollbackTransaction()
{
	try
	{
		if (NULL!=m_pDB.get())
			m_pDB->rollback_transaction();
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:rollbacktransaction failed");
	}
}

bool CMusicDatabase::InTransaction()
{
	if (NULL!=m_pDB.get()) return false;
	return m_pDB->in_transaction();
}

void CMusicDatabase::EmptyCache()
{
	m_artistCache.erase(m_artistCache.begin(), m_artistCache.end());
	m_genreCache.erase(m_genreCache.begin(), m_genreCache.end());
	m_pathCache.erase(m_pathCache.begin(), m_pathCache.end());
	m_albumCache.erase(m_albumCache.begin(), m_albumCache.end());
}

bool CMusicDatabase::GetSongsByPathes(SETPATHES& pathes, MAPSONGS& songs)
{
	try
	{
		songs.erase(songs.begin(),songs.end());
		if (pathes.size()<=0)
			return false;
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;

		CStdString strSQL("select * from song,album,path where song.idPath=path.idPath and song.idAlbum=album.idAlbum and path.strPath in ( ");
		for (ISETPATHES it=pathes.begin(); it!=pathes.end(); it++)
		{
			CStdString strPath=*it;
			//	musicdatabase always stores directories 
			//	without a slash at the end 
			if (CUtil::HasSlashAtEnd(strPath))
				strPath.Delete(strPath.size()-1);
			RemoveInvalidChars(strPath);

			CStdString strPart;
			strPart.Format("'%s', ", strPath);
			strSQL+=strPart;
		}
		strSQL.TrimRight( ", " );
		strSQL+=" )";

		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof()) 
		{
			CSong song;
			song.strArtist    = m_pDS->fv("song.strArtist").get_asString();
			song.strAlbum     = m_pDS->fv("album.strAlbum").get_asString();
			song.strGenre     = m_pDS->fv("song.strGenre").get_asString();
			song.iTrack       = m_pDS->fv("song.iTrack").get_asLong() ;
			song.iDuration    = m_pDS->fv("song.iDuration").get_asLong() ;
			song.iYear        = m_pDS->fv("song.iYear").get_asLong() ;
			song.strTitle     = m_pDS->fv("song.strTitle").get_asString();
			song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
			song.iStartOffset = m_pDS->fv("song.iStartOffset").get_asLong();
			song.iEndOffset = m_pDS->fv("song.iEndOffset").get_asLong();
			
			CStdString strFileName = m_pDS->fv("path.strPath").get_asString() ;
			strFileName += m_pDS->fv("song.strFileName").get_asString() ;
			song.strFileName = strFileName;
			
			songs.insert(pair<CStdString, CSong>(song.strFileName, song));
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetSongsByPathes() failed");
	}

	return false;
}

bool CMusicDatabase::GetAlbumByPath(const CStdString& strPath1, CAlbum& album)
{
	try
	{
		CStdString strPath=strPath1;
		//	musicdatabase always stores directories 
		//	without a slash at the end 
		if (CUtil::HasSlashAtEnd(strPath))
			strPath.Delete(strPath.size()-1);
		RemoveInvalidChars(strPath);

		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		strSQL.Format("select * from album,path where album.idPath=path.idPath and path.strPath='%s'", strPath );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;

		album.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
		album.strArtist = m_pDS->fv("album.strArtist").get_asString();
		album.strPath   = m_pDS->fv("path.strPath").get_asString();

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbumByPath() for %s failed", strPath1.c_str());
	}

	return false;
}

bool CMusicDatabase::GetAlbumsByPath(const CStdString& strPath1, VECALBUMS& albums)
{
	try
	{
		CStdString strPath=strPath1;
		albums.erase(albums.begin(), albums.end());
		//	musicdatabase always stores directories 
		//	without a slash at the end 
		if (CUtil::HasSlashAtEnd(strPath))
			strPath.Delete(strPath.size()-1);
		RemoveInvalidChars(strPath);

		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		strSQL.Format("select * from album,path where album.idPath=path.idPath and path.strPath='%s'", strPath );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;

		while (!m_pDS->eof()) 
		{
			CAlbum album;
			album.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
			album.strArtist = m_pDS->fv("album.strArtist").get_asString();
			album.strPath   = m_pDS->fv("path.strPath").get_asString();
			albums.push_back(album);
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbumsByPath() for %s failed", strPath1.c_str());
	}

	return false;
}

bool CMusicDatabase::GetAlbumsByArtist(const CStdString& strArtist1, VECALBUMS& albums)
{
	try
	{
		CStdString strArtist=strArtist1;
		albums.erase(albums.begin(), albums.end());
		RemoveInvalidChars(strArtist);

		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
//		strSQL.Format("select distinct album.*, artist.*, path.* from song,album,path where song.strArtist like '%s' and song.idAlbum=album.idAlbum and album.idArtist=artist.idArtist and album.idPath=path.idPath", strArtist );
		strSQL.Format("select * from album, album2artist, artist, path where album.idAlbum = album2artist.idAlbum and album2artist.idArtist = artist.idArtist and artist.strArtist like '%s' and album.idPath = path.idPath",strArtist.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;

		while (!m_pDS->eof()) 
		{
			CAlbum album;
			album.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
			album.strArtist = m_pDS->fv("album.strArtist").get_asString();
			album.strPath   = m_pDS->fv("path.strPath").get_asString();
			albums.push_back(album);
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbumsByArtist() for %s failed", strArtist1.c_str());
	}

	return false;
}

bool CMusicDatabase::FindSongsByName(const CStdString& strSearch1, VECSONGS& songs)
{
	try
	{
		songs.erase(songs.begin(),songs.end());
		CStdString strSearch=strSearch1;
		RemoveInvalidChars(strSearch);

		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		strSQL.Format("select * from song,path,album where song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.strTitle like '%%%s%%'", strSearch.c_str(), strSearch.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof()) 
		{
			CSong song;
			song.strArtist    = m_pDS->fv("song.strArtist").get_asString();
			song.strAlbum     = m_pDS->fv("album.strAlbum").get_asString();
			song.strGenre     = m_pDS->fv("song.strGenre").get_asString();
			song.iTrack       = m_pDS->fv("song.iTrack").get_asLong() ;
			song.iDuration    = m_pDS->fv("song.iDuration").get_asLong() ;
			song.iYear        = m_pDS->fv("song.iYear").get_asLong() ;
			song.strTitle     = m_pDS->fv("song.strTitle").get_asString();
			song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
			song.iStartOffset = m_pDS->fv("song.iStartOffset").get_asLong();
			song.iEndOffset	  = m_pDS->fv("song.iEndOffset").get_asLong();
			
			CStdString strFileName = m_pDS->fv("path.strPath").get_asString() ;
			strFileName += m_pDS->fv("song.strFileName").get_asString() ;
			song.strFileName = strFileName;
			
			songs.push_back(song);
			m_pDS->next();
		}

		m_pDS->close();
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetSongsByName() failed");
	}

	return false;
}

bool CMusicDatabase::FindSongsByNameAndArtist(const CStdString& strSearch1, VECSONGS& songs)
{
	try
	{
		songs.erase(songs.begin(),songs.end());
		CStdString strSearch=strSearch1;
		RemoveInvalidChars(strSearch);

		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		strSQL.Format("select * from song,path,album where song.idPath=path.idPath and song.idAlbum=album.idAlbum and (song.strTitle like '%%%s%%' or song.strArtist like '%%%s%%')", strSearch.c_str(), strSearch.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof()) 
		{
			CSong song;
			song.strArtist    = m_pDS->fv("song.strArtist").get_asString();
			song.strAlbum     = m_pDS->fv("album.strAlbum").get_asString();
			song.strGenre     = m_pDS->fv("song.strGenre").get_asString();
			song.iTrack       = m_pDS->fv("song.iTrack").get_asLong() ;
			song.iDuration    = m_pDS->fv("song.iDuration").get_asLong() ;
			song.iYear        = m_pDS->fv("song.iYear").get_asLong() ;
			song.strTitle     = m_pDS->fv("song.strTitle").get_asString();
			song.iTimedPlayed = m_pDS->fv("song.iTimesPlayed").get_asLong();
			song.iStartOffset = m_pDS->fv("song.iStartOffset").get_asLong();
			song.iEndOffset	  = m_pDS->fv("song.iEndOffset").get_asLong();
			
			CStdString strFileName = m_pDS->fv("path.strPath").get_asString() ;
			strFileName += m_pDS->fv("song.strFileName").get_asString() ;
			song.strFileName = strFileName;
			
			songs.push_back(song);
			m_pDS->next();
		}

		m_pDS->close();
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetSongsByName() failed");
	}

	return false;
}

bool CMusicDatabase::FindAlbumsByName(const CStdString& strSearch1, VECALBUMS& albums)
{
	try
	{
		CStdString strSearch=strSearch1;
		RemoveInvalidChars(strSearch);
		albums.erase(albums.begin(), albums.end());
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		strSQL.Format("select * from album,path where album.idPath=path.idPath and (album.strAlbum like '%%%s%%' or album.strArtist like '%%%s%%')", strSearch.c_str(), strSearch.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof()) 
		{
			CAlbum album;
			album.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
			album.strArtist = m_pDS->fv("album.strArtist").get_asString();
			album.strPath   = m_pDS->fv("path.strPath").get_asString();
			albums.push_back(album);
			m_pDS->next();
		}

		m_pDS->close();
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbumsByName() failed");
	}

	return false;
}

long CMusicDatabase::UpdateAlbumInfo(const CAlbum& album1)
{
	CStdString strSQL;
	try
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
		RemoveInvalidChars(album.strPath);

		if (NULL==m_pDB.get()) return -1;
		if (NULL==m_pDS.get()) return -1;
		long lPathId   = AddPath(album1.strPath);
		long lAlbumId  = AddAlbum(album1.strAlbum,album1.strArtist,lPathId,album1.strPath);
		AddArtists(album.strArtist, 0, lAlbumId);
		AddGenres(album.strGenre, 0, lAlbumId);

		strSQL.Format("select * from albuminfo where idAlbum=%i", lAlbumId);
		if (!m_pDS->query(strSQL.c_str())) return -1;

		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) 
		{
			return AddAlbumInfo(album1);
		}

		long idAlbumInfo=m_pDS->fv("idAlbumInfo").get_asLong();


		strSQL.Format("update albuminfo set idAlbum=%i,strArtist='%s',strGenre='%s',strTones='%s',strStyles='%s',strReview='%s',strImage='%s',iRating=%i,iYear=%i where idAlbumInfo=%i",
												lAlbumId,album.strArtist.c_str(),album.strGenre.c_str(),
												album.strTones.c_str(),
												album.strStyles.c_str(),
												album.strReview.c_str(),
												album.strImage.c_str(),
												album.iRating,
												album.iYear,
												idAlbumInfo);
		m_pDS->exec(strSQL.c_str());

		return idAlbumInfo;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:unable to updatealbuminfo (%s)", strSQL.c_str());
	}

	return -1;
}
