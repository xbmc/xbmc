
#include "stdafx.h"
#include ".\musicdatabase.h"
#include "settings.h"
#include "StdString.h"
#include "crc32.h"
#include "LocalizeStrings.h"
#include "filesystem/File.h"
#include "util.h"
#include "utils/log.h"
#include "autoptrhandle.h"
#include "GUIDialogProgress.h"
#include "GUIDialogOK.h"
#include "GUIDialogSelect.h"
#include "musicInfoTag.h"
#include "filesystem/cddb.h"
#include "filesystem/directorycache.h"
#include "filesystem/FactoryDirectory.h"

using namespace XFILE;
using namespace CDDB;
using namespace DIRECTORY;

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
	m_iSongsBeforeCommit = 0;
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
	musicDatabase+="\\MyMusic6.db";

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
	m_pDS2.reset(m_pDB->CreateDataset());

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
	m_pDS2->exec("PRAGMA cache_size=8192\n");
	m_pDS2->exec("PRAGMA synchronous='OFF'\n");
	m_pDS2->exec("PRAGMA count_changes='OFF'\n");
//	m_pDS2->exec("PRAGMA temp_store='MEMORY'\n");
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
  if (NULL!=m_pDS.get()) m_pDS->close();
	m_pDB->disconnect();
	m_pDB.reset();
	//EmptyCache();
}

bool CMusicDatabase::CreateTables()
{
	try
	{
		//	Tables
    CLog::Log(LOGINFO, "create artist table");
		m_pDS->exec("CREATE TABLE artist ( idArtist integer primary key, strArtist text)\n");
    CLog::Log(LOGINFO, "create album table");
		m_pDS->exec("CREATE TABLE album ( idAlbum integer primary key, strAlbum text, idArtist integer, iNumArtists integer, idPath integer)\n");
    CLog::Log(LOGINFO, "create genre table");
		m_pDS->exec("CREATE TABLE genre ( idGenre integer primary key, strGenre text)\n");
    CLog::Log(LOGINFO, "create path table");
		m_pDS->exec("CREATE TABLE path ( idPath integer primary key,  strPath text)\n");
    CLog::Log(LOGINFO,"create song table");
		m_pDS->exec("CREATE TABLE song ( idSong integer primary key, idAlbum integer, idPath integer, idArtist integer, iNumArtists integer, idGenre integer, iNumGenres integer, strTitle text, iTrack integer, iDuration integer, iYear integer, dwFileNameCRC text, strFileName text, iTimesPlayed integer, iStartOffset integer, iEndOffset integer)\n");
    CLog::Log(LOGINFO,"create albuminfo table");
		m_pDS->exec("CREATE TABLE albuminfo ( idAlbumInfo integer primary key, idAlbum integer, iYear integer, idGenre integer, iNumGenres integer, strTones text, strStyles text, strReview text, strImage text, iRating integer)\n");
    CLog::Log(LOGINFO,"create albuminfosong table");
		m_pDS->exec("CREATE TABLE albuminfosong ( idAlbumInfoSong integer primary key, idAlbumInfo integer, iTrack integer, strTitle text, iDuration integer)\n");

		CLog::Log(LOGINFO,"create exartistsong table");
		m_pDS->exec("CREATE TABLE exartistsong ( idSong integer, iPosition integer, idArtist integer)\n");
		CLog::Log(LOGINFO,"create extragenresong table");
		m_pDS->exec("CREATE TABLE exgenresong ( idSong integer, iPosition integer, idGenre integer)\n");
		CLog::Log(LOGINFO,"create exartistalbum table");
		m_pDS->exec("CREATE TABLE exartistalbum ( idAlbum integer, iPosition integer, idArtist integer)\n");
		CLog::Log(LOGINFO,"create exgenrealbum table");
	  m_pDS->exec("CREATE TABLE exgenrealbum ( idAlbum integer, iPosition integer, idGenre integer)\n");

		//	Indexes
		CLog::Log(LOGINFO,"create exartistsong index");
		m_pDS->exec("CREATE INDEX idxExtraArtistSong ON exartistsong(idSong)");
		m_pDS->exec("CREATE INDEX idxExtraArtistSong2 ON exartistsong(idArtist)");
		CLog::Log(LOGINFO,"create exgenresong index");
		m_pDS->exec("CREATE INDEX idxExtraGenreSong ON exgenresong(idSong)");
		m_pDS->exec("CREATE INDEX idxExtraGenreSong2 ON exgenresong(idGenre)");
		CLog::Log(LOGINFO,"create exartistalbum index");
		m_pDS->exec("CREATE INDEX idxExtraArtistAlbum ON exartistalbum(idAlbum)");
		m_pDS->exec("CREATE INDEX idxExtraArtistAlbum2 ON exartistalbum(idArtist)");
		CLog::Log(LOGINFO,"create exgenrealbum index");
		m_pDS->exec("CREATE INDEX idxExtraGenreAlbum ON exgenrealbum(idAlbum)");
		m_pDS->exec("CREATE INDEX idxExtraGenreAlbum2 ON exgenrealbum(idGenre)");

		CLog::Log(LOGINFO, "create album index");
		m_pDS->exec("CREATE INDEX idxAlbum ON album(strAlbum)");
    CLog::Log(LOGINFO, "create genre index");
		m_pDS->exec("CREATE INDEX idxGenre ON genre(strGenre)");
    CLog::Log(LOGINFO, "create artist index");
		m_pDS->exec("CREATE INDEX idxArtist ON artist(strArtist)");
    CLog::Log(LOGINFO, "create path index");
		m_pDS->exec("CREATE INDEX idxPath ON path(strPath)");
		//m_pDS->exec("CREATE INDEX idxSong ON song(dwFileNameCRC)");

		//	Trigger
    CLog::Log(LOGINFO, "create albuminfo trigger");
		m_pDS->exec("CREATE TRIGGER tgrAlbumInfo AFTER delete ON albuminfo FOR EACH ROW BEGIN delete from albuminfosong where albuminfosong.idAlbumInfo=old.idAlbumInfo; END");
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
		RemoveInvalidChars(song.strTitle);

		CStdString strPath, strFileName;
		CUtil::Split(song1.strFileName, strPath, strFileName);
		RemoveInvalidChars(strFileName);

		if (NULL==m_pDB.get()) return;
		if (NULL==m_pDS.get()) return;

		// split our (possibly) multiple artist string into individual artists
		CStdStringArray vecArtists;
		int iNumArtists = StringUtils::SplitString(song.strArtist," / ",vecArtists);
		// and the same for our genres
		CStdStringArray vecGenres;
		int iNumGenres = StringUtils::SplitString(song.strGenre," / ",vecGenres);
		// add the primary artist/genre (and pop off of the vectors)
		long lArtistId = AddArtist(vecArtists[0]);
		long lGenreId = AddGenre(vecGenres[0]);

		long lPathId   = AddPath(strPath);
		long lAlbumId  = AddAlbum(song1.strAlbum,lArtistId,iNumArtists,song1.strArtist,lPathId,strPath);

		DWORD dwCRC;
		Crc32 crc;
		crc.Reset();
		crc.Compute(song1.strFileName.c_str(),strlen(song1.strFileName.c_str()));
		dwCRC=crc;

		bool bInsert = true;
		int lSongId = -1;
		if (bCheck)
		{
			strSQL.Format("select * from song where idAlbum=%i and dwFileNameCRC='%ul' and strTitle='%s'",
					lAlbumId,dwCRC,song.strTitle.c_str());
			if (!m_pDS->query(strSQL.c_str())) return;
			if (m_pDS->num_rows() != 0)
			{
				lSongId = m_pDS->fv("idSong").get_asLong();
				bInsert = false;
			}
      m_pDS->close();
		}
		if (bInsert)
		{
			strSQL.Format("insert into song (idSong,idAlbum,idPath,idArtist,iNumArtists,idGenre,iNumGenres,strTitle,iTrack,iDuration,iYear,dwFileNameCRC,strFileName,iTimesPlayed,iStartOffset,iEndOffset) values(NULL,%i,%i,%i,%i,%i,%i,'%s',%i,%i,%i,'%ul','%s',%i,%i,%i)",
										lAlbumId,lPathId,lArtistId,iNumArtists,lGenreId,iNumGenres,
										song.strTitle.c_str(),
										song.iTrack,song.iDuration,song.iYear,
										dwCRC,
										strFileName.c_str(), 0, song.iStartOffset, song.iEndOffset);

			m_pDS->exec(strSQL.c_str());
			lSongId = sqlite_last_insert_rowid(m_pDB->getHandle());
		}

		// add artists and genres (don't need to add to exgenrealbum table - it's only used for albuminfo)
		AddExtraArtists(vecArtists, lSongId, lAlbumId, bCheck);
		AddExtraGenres(vecGenres, lSongId, 0, bCheck);

		// increment the number of songs we've added since the last commit, and check if we should commit
		if (m_iSongsBeforeCommit++ > NUM_SONGS_BEFORE_COMMIT)
		{
			CommitTransaction();
			BeginTransaction();
		}
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:unable to addsong (%s)", strSQL.c_str());
	}
}

long CMusicDatabase::AddAlbum(const CStdString& strAlbum1, const long lArtistId, const int iNumArtists, const CStdString &strArtist, long lPathId, const CStdString& strPath)
{
	CStdString strSQL;
	try
	{
		CStdString strAlbum=strAlbum1;
		RemoveInvalidChars(strAlbum);

		if (NULL==m_pDB.get()) return -1;
		if (NULL==m_pDS.get()) return -1;

		map <CStdString, CAlbumCache>::const_iterator it;

		it=m_albumCache.find(strAlbum1+strPath);
		if (it!=m_albumCache.end())
			return it->second.idAlbum;

		strSQL.Format("select * from album where idPath=%i and strAlbum like '%s'", lPathId, strAlbum);
		m_pDS->query(strSQL.c_str());

		if (m_pDS->num_rows() == 0)
		{
      m_pDS->close();
			// doesnt exists, add it
			strSQL.Format("insert into album (idAlbum, strAlbum, idArtist, iNumArtists, idPath) values( NULL, '%s', %i, %i, %i)", strAlbum, lArtistId, iNumArtists, lPathId);
			m_pDS->exec(strSQL.c_str());

			CAlbumCache album;
			album.idAlbum  = sqlite_last_insert_rowid(m_pDB->getHandle());
			album.strPath  = strPath;
			album.idPath   = lPathId;
			album.strAlbum = strAlbum1;
			album.strArtist = strArtist;
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
			album.strArtist = strArtist;
			m_albumCache.insert(pair<CStdString, CAlbumCache>(album.strAlbum+album.strPath, album));
      m_pDS->close();
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
		CAlbumCache album = it->second;
		long lAlbumId=album.idAlbum;
		CStdString strSQL;
		// Albums by various artists will have different songs with different artists
		GetSongsByAlbum(album.strAlbum, album.strPath, songs);
		long lVariousArtistsId=-1;
		long lArtistsId=-1;
		bool bVarious = false;
		bool bSingleArtistCompilation=false;
		CStdString strArtist;
		if (songs.size()>1)
		{
			//	Are the artists of this album all the same
			for (int i=0; i < (int)songs.size()-1; i++)
			{
				CSong song=songs[i];
				CSong song1=songs[i+1];

				CStdStringArray vecArtists, vecArtists1;
				CStdString strArtist, strArtist1;
				int iNumArtists=StringUtils::SplitString(song.strArtist, " / ", vecArtists);
				int iNumArtists1=StringUtils::SplitString(song1.strArtist, " / ", vecArtists1);
				strArtist=vecArtists[0];
				strArtist1=vecArtists1[0];
				strArtist.TrimRight();
				strArtist1.TrimRight();
				// Only check the first artist if its different
				if (strArtist!=strArtist1)
				{
					CStdString strVariousArtists=g_localizeStrings.Get(340);
					lVariousArtistsId = AddArtist(strVariousArtists);
					bSingleArtistCompilation=false;
					bVarious=true;
					break;
				}
				else if (iNumArtists>1 && iNumArtists1>1 && song.strArtist!=song1.strArtist)
				{
					// Artists don't all agree.  Instead of some complex function to compare all
					// the artists of each particular track, let's just take the first artist
					// and just use that.
					bSingleArtistCompilation=true;
					lArtistsId = AddArtist(vecArtists[0]);
				}
			}
		}

		if (bVarious)
		{
			CStdString strSQL;
			strSQL.Format("UPDATE album SET iNumArtists=1, idArtist=%i where idAlbum=%i", lVariousArtistsId, album.idAlbum);
			m_pDS->exec(strSQL.c_str());
			// we must also update our exartistalbum mapping.
			// In the case of multiple artists, we need to add entries to extra artist album table so that we
			// can find the albums by a particular artist (even if they only contribute a single song to the
			// album in question)
			strSQL.Format("DELETE from exartistalbum where idAlbum=%i", album.idAlbum);
			m_pDS->exec(strSQL.c_str());
			// run through the songs and add the artists we need to link to...
			CStdStringArray vecArtists;
			for (int i=0; i<(int)songs.size(); i++)
			{
				CStdStringArray vecTemp;
				StringUtils::SplitString(songs[i].strArtist, " / ", vecTemp);
				for (int j=0; j<(int)vecTemp.size(); j++)
					vecArtists.push_back(vecTemp[j]);
			}
			AddExtraArtists(vecArtists, 0, album.idAlbum, true);
		}

		if (bSingleArtistCompilation)
		{
			CStdString strSQL;
			strSQL.Format("UPDATE album SET iNumArtists=1, idArtist=%i where idAlbum=%i", lArtistsId, album.idAlbum);
			m_pDS->exec(strSQL.c_str());
			// we must also update our exartistalbum mapping.
			// In the case of multiple artists, we need to add entries to extra artist album table so that we
			// can find the albums by a particular artist (even if they only contribute a single song to the
			// album in question)
			strSQL.Format("DELETE from exartistalbum where idAlbum=%i", album.idAlbum);
			m_pDS->exec(strSQL.c_str());
			// run through the songs and add the artists we need to link to...
			CStdStringArray vecArtists;
			for (int i=0; i<(int)songs.size(); i++)
			{
				CStdStringArray vecTemp;
				StringUtils::SplitString(songs[i].strArtist, " / ", vecTemp);
				for (int j=1; j<(int)vecTemp.size(); j++)
					vecArtists.push_back(vecTemp[j]);
			}
			AddExtraArtists(vecArtists, 0, album.idAlbum, true);
		}

		CStdString strTempCoverArt;
		CStdString strCoverArt;
		CUtil::GetAlbumThumb(album.strAlbum, album.strPath, strTempCoverArt, true);
		//	Was the album art of this album read during scan?
		if (CUtil::ThumbCached(strTempCoverArt))
		{
			//	Yes.
			VECALBUMS albums;
			GetAlbumsByPath(album.strPath, albums);
			CUtil::GetAlbumFolderThumb(album.strPath, strCoverArt);
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
			CUtil::GetAlbumThumb(album.strAlbum, album.strPath, strCoverArt);
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
		strGenre.TrimLeft(" ");
		strGenre.TrimRight(" ");

		if (NULL==m_pDB.get()) return -1;
		if (NULL==m_pDS.get()) return -1;
		map <CStdString, CGenreCache>::const_iterator it;

		it=m_genreCache.find(strGenre);
		if (it!=m_genreCache.end())
			return it->second.idGenre;

		RemoveInvalidChars(strGenre);

		strSQL.Format("select * from genre where strGenre like '%s'", strGenre);
		m_pDS->query(strSQL.c_str());
		if (m_pDS->num_rows() == 0)
		{
      m_pDS->close();
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
      m_pDS->close();
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
		strArtist.TrimLeft(" ");
		strArtist.TrimRight(" ");

		if (NULL==m_pDB.get()) return -1;
		if (NULL==m_pDS.get()) return -1;

		map <CStdString, CArtistCache>::const_iterator it;

		it=m_artistCache.find(strArtist);
		if (it!=m_artistCache.end())
			return it->second.idArtist;

		RemoveInvalidChars(strArtist);

		strSQL.Format("select * from artist where strArtist like '%s'", strArtist);
		m_pDS->query(strSQL.c_str());

		if (m_pDS->num_rows() == 0)
		{
      m_pDS->close();
			// doesnt exists, add it
			strSQL .Format("insert into artist (idArtist, strArtist) valueS( NULL, '%s' )", strArtist);
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
      m_pDS->close();
			return artist.idArtist;
		}
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:unable to addartist (%s)", strSQL.c_str());
	}

	return -1;
}

void CMusicDatabase::AddExtraArtists(const CStdStringArray &vecArtists, long lSongId, long lAlbumId, bool bCheck)
{
	try
	{
		// add each of the artists in the vector of artists
		for (int i=1; i<(int)vecArtists.size(); i++)
		{
			long lArtistId = AddArtist(vecArtists[i]);
			if (lArtistId>=0)
			{// added successfully, we must now add entries to the exartistsong table
				CStdString strSQL;
				// first link the artist with the song
				bool bInsert = true;
				if (lSongId)
				{
					if (bCheck)
					{
						strSQL.Format("select * from exartistsong where idSong=%i and idArtist=%i",
								lSongId,lArtistId);
						if (!m_pDS->query(strSQL.c_str())) return;
						if (m_pDS->num_rows() != 0)
							bInsert = false; // already exists
            m_pDS->close();
					}
					if (bInsert)
					{
						strSQL.Format("insert into exartistsong (idSong,iPosition,idArtist) values(%i,%i,%i)",
														lSongId,i,lArtistId);

						m_pDS->exec(strSQL.c_str());
					}
				}
				// now link the artist with the album
				bInsert = true;
				// always check artists (as this routine is called whenever a song is added)
				strSQL.Format("select * from exartistalbum where idAlbum=%i and idArtist=%i",
						lAlbumId,lArtistId);
				if (!m_pDS->query(strSQL.c_str())) return;
				if (m_pDS->num_rows() != 0)
					bInsert = false; // already exists
        m_pDS->close();
				if (bInsert)
				{
					strSQL.Format("insert into exartistalbum (idAlbum,iPosition,idArtist) values(%i,%i,%i)",
													lAlbumId,i,lArtistId);

					m_pDS->exec(strSQL.c_str());
				}
			}
		}
	}
	catch(...)
    {
		CLog::Log(LOGERROR, "CMusicDatabase:AddExtraArtists(%i,%i) failed", lSongId, lAlbumId);
    }
}

void CMusicDatabase::AddExtraGenres(const CStdStringArray &vecGenres, long lSongId, long lAlbumId, bool bCheck)
{
	try
	{
		// add each of the genres in the vector
		for (int i=1; i<(int)vecGenres.size(); i++)
		{
			long lGenreId = AddGenre(vecGenres[i]);
			if (lGenreId>=0)
			{	// added successfully!
				CStdString strSQL;
				// first link the genre with the song
				bool bInsert = true;
				if (lSongId)
				{
					if (bCheck)
					{
						strSQL.Format("select * from exgenresong where idSong=%i and idGenre=%i",
								lSongId,lGenreId);
						if (!m_pDS->query(strSQL.c_str())) return;
						if (m_pDS->num_rows() != 0)
							bInsert = false; // already exists
            m_pDS->close();
					}
					if (bInsert)
					{
						strSQL.Format("insert into exgenresong (idSong,iPosition,idGenre) values(%i,%i,%i)",
														lSongId,i,lGenreId);

						m_pDS->exec(strSQL.c_str());
					}
				}
    			// now link the genre with the album
				bInsert = true;
				if (lAlbumId)
				{
					if (bCheck)
					{
						strSQL.Format("select * from exgenrealbum where idAlbum=%i and idGenre=%i",
								lAlbumId,lGenreId);
						if (!m_pDS->query(strSQL.c_str())) return;
						if (m_pDS->num_rows() != 0)
							bInsert = false; // already exists
            m_pDS->close();
					}
					if (bInsert)
					{
						strSQL.Format("insert into exgenrealbum (idAlbum,iPosition,idGenre) values(%i,%i,%i)",
														lAlbumId,i,lGenreId);

						m_pDS->exec(strSQL.c_str());
					}
				}
			}
		}
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "CMusicDatabase:AddExtraGenres(%i,%i) failed", lSongId, lAlbumId);
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

		strSQL.Format( "select * from path where strPath='%s'", strPath);
		m_pDS->query(strSQL.c_str());
		if (m_pDS->num_rows() == 0)
		{
      m_pDS->close();
			// doesnt exists, add it
			strSQL.Format("insert into path (idPath, strPath) values( NULL, '%s' )", strPath);
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
      m_pDS->close();
			return path.idPath;
		}
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "musicdatabase:unable to addpath (%s)", strSQL.c_str());
	}

	return -1;
}

CSong CMusicDatabase::GetSongFromDataset()
{
  CSong song;
  // get the full artist string
  song.strArtist = m_pDS->fv("artist.strArtist").get_asString();
  if (m_pDS->fv("song.iNumArtists").get_asLong() > 1)
	  GetExtraArtistsForSong(m_pDS->fv("song.idSong").get_asLong(), song.strArtist);
  // and the full genre string
  song.strGenre     = m_pDS->fv("genre.strGenre").get_asString();
  if (m_pDS->fv("song.iNumGenres").get_asLong() > 1)
    GetExtraGenresForSong(m_pDS->fv("song.idSong").get_asLong(), song.strGenre);
  // and the rest...
  song.strAlbum     = m_pDS->fv("album.strAlbum").get_asString();
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

  return song;
}

CAlbum CMusicDatabase::GetAlbumFromDataset()
{
  CAlbum album;
  album.strAlbum  = m_pDS->fv("album.strAlbum").get_asString();
  album.strArtist = m_pDS->fv("artist.strArtist").get_asString();
	int test = m_pDS->fv("album.iNumArtists").get_asLong();
  if (m_pDS->fv("album.iNumArtists").get_asLong() > 1)
    GetExtraArtistsForAlbum(m_pDS->fv("album.idAlbum").get_asLong(), album.strArtist);
  album.strPath   = m_pDS->fv("path.strPath").get_asString();
  return album;
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
		strSQL.Format("select * from song,album,path,artist,genre where song.dwFileNameCRC='%ul' and path.strPath='%s' and song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idArtist=artist.idArtist and song.idGenre=genre.idGenre",
										dwCRC,
										strPath.c_str());

		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }
		song = GetSongFromDataset();
		m_pDS->close();	//	cleanup recordset data
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetSongByFileName(%s) failed", strFileName1.c_str());
	}

	return false;
}

bool CMusicDatabase::GetExtraArtistsForSong(long lSongId, CStdString &strArtist)
{
	// gets called when we have multiple artists for this song.
	CStdString strSQL;
	strSQL.Format("select artist.strArtist from exartistsong, artist where exartistsong.idSong=%i and exartistsong.idArtist=artist.idArtist order by exartistsong.iPosition", lSongId);
	if (NULL==m_pDS2.get() || !m_pDS2->query(strSQL.c_str())) return false;
	while (!m_pDS2->eof())
    {
		strArtist += " / " + m_pDS2->fv("artist.strArtist").get_asString();
		m_pDS2->next();
    }
	m_pDS2->close();
	return true;
}

bool CMusicDatabase::GetExtraGenresForSong(long lSongId, CStdString &strGenre)
{
	// gets called when we have multiple genres for this song.
	CStdString strSQL;
	strSQL.Format("select genre.strGenre from exgenresong,genre where exgenresong.idSong=%i and exgenresong.idGenre=genre.idGenre order by exgenresong.iPosition", lSongId);
	if (NULL==m_pDS2.get() || !m_pDS2->query(strSQL.c_str())) return false;
	while (!m_pDS2->eof())
    {
		strGenre += " / " + m_pDS2->fv("genre.strGenre").get_asString();
		m_pDS2->next();
    }
	m_pDS2->close();
	return true;
}

bool CMusicDatabase::GetExtraArtistsForAlbum(long lAlbumId, CStdString &strArtist)
{
	// gets called when we have multiple artists for this album (but not for various artists).
	CStdString strSQL;
	strSQL.Format("select artist.strArtist from exartistalbum, artist where exartistalbum.idAlbum=%i and exartistalbum.idArtist=artist.idArtist order by exartistalbum.iPosition", lAlbumId);
	if (NULL==m_pDS2.get() || !m_pDS2->query(strSQL.c_str())) return false;
	while (!m_pDS2->eof())
    {
		strArtist += " / " + m_pDS2->fv("artist.strArtist").get_asString();
		m_pDS2->next();
    }
	return true;
}

bool CMusicDatabase::GetExtraGenresForAlbum(long lAlbumId, CStdString &strGenre)
{
	// gets called when we have multiple genres for this album (only for albuminfo).
	CStdString strSQL;
	strSQL.Format("select genre.strGenre from exgenrealbum,genre where exgenrealbum.idAlbum=%i and exgenrealbum.idGenre=genre.idGenre order by exgenrealbum.iPosition", lAlbumId);
	if (NULL==m_pDS2.get() || !m_pDS2->query(strSQL.c_str())) return false;
	while (!m_pDS2->eof())
    {
		strGenre += " / " + m_pDS2->fv("genre.strGenre").get_asString();
		m_pDS2->next();
    }
	m_pDS2->close();
	return true;
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
		strSQL.Format("select * from song,album,path,artist,genre where song.strTitle like '%s' and song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idArtist=artist.idArtist and song.idGenre=genre.idGenre",strTitle.c_str() );

		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }

		song = GetSongFromDataset();
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

		// find the songs whose primary artist is this one
		CStdString strSQL;
		strSQL.Format("select * from song,album,path,artist,genre where artist.strArtist like '%s' and song.idArtist=artist.idArtist and song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre",strArtist.c_str() );
		if (!m_pDS->query(strSQL.c_str())) return false;
		while (!m_pDS->eof())
		{
			songs.push_back(GetSongFromDataset());
			m_pDS->next();
		}
    m_pDS->close();
		// find songs with this artist as secondary artist
		strSQL.Format("select * from song,album,path,artist,exartistsong,genre where artist.strArtist like '%s' and exartistsong.idArtist=artist.idArtist and exartistsong.idSong=song.idSong and song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre",strArtist.c_str() );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		while (!m_pDS->eof())
		{
			songs.push_back(GetSongFromDataset());
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
		strSQL.Format("select * from song,path,album,artist,genre where album.strAlbum like '%s' and song.idAlbum=album.idAlbum and song.idPath=path.idPath and path.strPath='%s' and song.idArtist=artist.idArtist and song.idGenre=genre.idGenre order by song.iTrack", strAlbum, strPath );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }
		while (!m_pDS->eof())
		{
			songs.push_back(GetSongFromDataset());
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
		CStdString strVariousArtists=g_localizeStrings.Get(340);
		long lVariousArtistId=AddArtist(strVariousArtists);
		CStdString strSQL;
		strSQL.Format("select * from artist where idArtist <> %i ", lVariousArtistId );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }
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
		CStdString strVariousArtists=g_localizeStrings.Get(340);
		long lVariousArtistId=AddArtist(strVariousArtists);
		CStdString strSQL;
		strSQL.Format("select * from artist where strArtist LIKE '%%%s%%' and idArtist <> %i ", strArtist, lVariousArtistId );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }
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
		strSQL.Format("select * from album,path,artist where album.idPath=path.idPath and album.idArtist=artist.idArtist" );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }
		while (!m_pDS->eof())
		{
			albums.push_back(GetAlbumFromDataset());
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
		strSQL.Format("select * from album,path,artist where album.idPath=path.idPath and album.idArtist=artist.idArtist and album.idAlbum in (select distinct song.idAlbum from song where song.iTimesPlayed>0 order by song.iTimesPlayed desc limit 20)" );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }
		while (!m_pDS->eof())
		{
			albums.push_back(GetAlbumFromDataset());
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

long CMusicDatabase::AddAlbumInfo(const CAlbum& album1, const VECSONGS& songs)
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

		// split our (possibly) multiple artist string into individual artists
		CStdStringArray vecArtists;
		int iNumArtists = StringUtils::SplitString(album.strArtist," / ",vecArtists);
		// split our (possibly) multiple genre string into individual genres
		CStdStringArray vecGenres;
		int iNumGenres = StringUtils::SplitString(album.strGenre," / ",vecGenres);
		// add the primary artist/genre
		long lArtistId = AddArtist(vecArtists[0]);
		long lGenreId = AddGenre(vecGenres[0]);

		long lPathId   = AddPath(album1.strPath);
		long lAlbumId  = AddAlbum(album1.strAlbum,lArtistId,iNumArtists,album1.strArtist,lPathId,album1.strPath);

		AddExtraArtists(vecArtists, 0, lAlbumId);
		AddExtraGenres(vecGenres, 0, lAlbumId);

		strSQL.Format("select * from albuminfo where idAlbum=%i", lAlbumId);
		if (!m_pDS->query(strSQL.c_str())) return -1;

		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound!= 0)
    {
			long iID = m_pDS->fv("idAlbumInfo").get_asLong();
      m_pDS->close();
      return iID;
    }
    m_pDS->close();

		strSQL.Format("insert into albuminfo (idAlbumInfo,idAlbum,idGenre,iNumGenres,strTones,strStyles,strReview,strImage,iRating,iYear) values(NULL,%i,%i,%i,'%s','%s','%s','%s',%i,%i)",
												lAlbumId,lGenreId,iNumGenres,
												album.strTones.c_str(),
												album.strStyles.c_str(),
												album.strReview.c_str(),
												album.strImage.c_str(),
												album.iRating,
												album.iYear);
		m_pDS->exec(strSQL.c_str());

		long lAlbumInfoId=sqlite_last_insert_rowid(m_pDB->getHandle());

		AddAlbumInfoSongs(lAlbumInfoId, songs);

		return lAlbumInfoId;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:unable to addalbuminfo (%s)", strSQL.c_str());
	}

	return -1;
}

bool CMusicDatabase::AddAlbumInfoSongs(long idAlbumInfo, const VECSONGS& songs)
{
	CStdString strSQL;
	try
	{
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;

		for (int i=0; i<(int)songs.size(); i++)
		{
			CSong song=songs[i];
			RemoveInvalidChars(song.strTitle);

			strSQL.Format("select * from albuminfosong where idAlbumInfo=%i and iTrack=%i", idAlbumInfo, song.iTrack);
			if (!m_pDS->query(strSQL.c_str())) continue;

			int iRowsFound = m_pDS->num_rows();
			if (iRowsFound!= 0)
				continue;

			strSQL.Format("insert into albuminfosong (idAlbumInfoSong,idAlbumInfo,iTrack,strTitle,iDuration) values(NULL,%i,%i,'%s',%i)",
													idAlbumInfo,
													song.iTrack,
													song.strTitle,
													song.iDuration);
			m_pDS->exec(strSQL.c_str());
		}

		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:unable to addalbuminfosong (%s)", strSQL.c_str());
	}

	return false;
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
		// first try the primary genre
		strSQL.Format("select * from song,genre,album,path,artist where genre.strGenre like '%s' and song.idGenre=genre.idGenre and song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idArtist=artist.idArtist",strSQLGenre.c_str() );
		if (!m_pDS->query(strSQL.c_str())) return false;
		while (!m_pDS->eof())
		{
		  songs.push_back(GetSongFromDataset());
		  m_pDS->next();
		}
    m_pDS->close();
		// now the secondary genres
		strSQL.Format("select * from song,exgenresong,genre,album,path,artist where genre.strGenre like '%s' and exgenresong.idGenre=genre.idGenre and song.idSong=exgenresong.idSong and song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idArtist=artist.idArtist",strSQLGenre.c_str() );
		if (!m_pDS->query(strSQL.c_str())) return false;
		while (!m_pDS->eof())
		{
		  songs.push_back(GetSongFromDataset());
		  m_pDS->next();
		}
		m_pDS->close();	//	cleanup recordset data
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
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }
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

bool CMusicDatabase::GetAlbumInfo(const CStdString& strAlbum1, const CStdString& strPath1, CAlbum& album, VECSONGS& songs)
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
		strSQL.Format("select * from albuminfo,album,path,artist,genre where album.strAlbum like '%s' and path.strPath='%s' and album.idPath=path.idPath and albuminfo.idAlbum=album.idAlbum and album.idArtist=artist.idArtist and albuminfo.idGenre=genre.idGenre",strAlbum, strPath );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound!= 0)
		{
			album.iRating	= m_pDS->fv("albuminfo.iRating").get_asLong() ;
			album.iYear	= m_pDS->fv("albuminfo.iYear").get_asLong() ;
			album.strAlbum	= m_pDS->fv("album.strAlbum").get_asString();
			album.strArtist = m_pDS->fv("artist.strArtist").get_asString();
			if (m_pDS->fv("album.iNumArtists").get_asLong() > 1)
			  GetExtraArtistsForAlbum(m_pDS->fv("album.idAlbum").get_asLong(), album.strArtist);
			album.strGenre = m_pDS->fv("genre.strGenre").get_asString();
			if (m_pDS->fv("albuminfo.iNumGenres").get_asLong() > 1)
			  GetExtraGenresForAlbum(m_pDS->fv("albuminfo.idAlbum").get_asLong(), album.strGenre);
			album.strImage	= m_pDS->fv("albuminfo.strImage").get_asString();
			album.strReview	= m_pDS->fv("albuminfo.strReview").get_asString();
			album.strStyles	= m_pDS->fv("albuminfo.strStyles").get_asString();
			album.strTones	= m_pDS->fv("albuminfo.strTones").get_asString();
			album.strPath   = m_pDS->fv("path.strPath").get_asString();

			long idAlbumInfo = m_pDS->fv("albuminfo.idAlbumInfo").get_asLong();

			m_pDS->close();	//	cleanup recordset data

			GetAlbumInfoSongs(idAlbumInfo, songs);

			return true;
		}
    m_pDS->close();
		return false;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbumInfo(%s, %s) failed", strAlbum1.c_str(), strPath1.c_str());
	}

	return false;
}

bool CMusicDatabase::GetAlbumInfoSongs(long idAlbumInfo, VECSONGS& songs)
{
	try
	{
		CStdString strSQL;
		strSQL.Format("select * from albuminfosong where idAlbumInfo=%i order by iTrack", idAlbumInfo);

		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound==0) return false;
		while (!m_pDS->eof())
		{
			CSong song;
			song.iTrack	= m_pDS->fv("iTrack").get_asLong();
			song.strTitle	= m_pDS->fv("strTitle").get_asString();
			song.iDuration	= m_pDS->fv("iDuration").get_asLong();

			songs.push_back(song);
			m_pDS->next();
		}

		m_pDS->close();	//	cleanup recordset data

		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbumInfoSongs(%i) failed", idAlbumInfo);
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
		strSQL.Format("select * from song,album,path,artist,genre where song.iTimesPlayed>0 and song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idArtist=artist.idArtist and song.idGenre=genre.idGenre order by song.iTimesPlayed desc limit 100"  );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }
		int iCount=1;
		while (!m_pDS->eof())
		{
			songs.push_back(GetSongFromDataset());
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

		strSQL.Format("select * from song,path where song.dwFileNameCRC='%ul' and song.idPath=path.idPath and path.strPath='%s'",
										dwCRC,
										strPath.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }

		int idSong        = m_pDS->fv("song.idSong").get_asLong();
		int iTimesPlayed  = m_pDS->fv("song.iTimesPlayed").get_asLong();
    m_pDS->close();

		strSQL.Format("UPDATE song SET iTimesPlayed=%i where idSong=%i",
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
		strSQL.Format("select * from song,path,album,artist,genre where path.strPath='%s' and song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idArtist=artist.idArtist and song.idGenre=genre.idGenre",strPath.c_str() );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }
		while (!m_pDS->eof())
		{
			songs.push_back(GetSongFromDataset());
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
		strSQL.Format("select * from song,path,album,artist,genre where path.strPath='%s' and song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idArtist=artist.idArtist and song.idGenre=genre.idGenre",strPath.c_str() );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }
		while (!m_pDS->eof())
		{
			CSong song = GetSongFromDataset();
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

bool CMusicDatabase::CommitTransaction()
{
	try
	{
		if (NULL!=m_pDB.get())
			m_pDB->commit_transaction();
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:committransaction failed");
		return false;
	}
	m_iSongsBeforeCommit=0;
	return true;
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

		CStdString strSQL("select * from song,album,path,artist,genre where song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idArtist=artist.idArtist and song.idGenre=genre.idGenre and path.strPath in ( ");
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
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }
		while (!m_pDS->eof())
		{
		  CSong song = GetSongFromDataset();
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
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }

		album = GetAlbumFromDataset();

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
		strSQL.Format("select * from album,path,artist where album.idPath=path.idPath and path.strPath='%s' and album.idArtist=artist.idArtist", strPath );
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0)
    {
      m_pDS->close();
      return false;
    }

		while (!m_pDS->eof())
		{
			albums.push_back(GetAlbumFromDataset());
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
		// first try the primary artist
		strSQL.Format("select * from album, artist, path where artist.strArtist like '%s' and album.idArtist=artist.idArtist and album.idPath = path.idPath",strArtist.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		while (!m_pDS->eof())
		{
			albums.push_back(GetAlbumFromDataset());
			m_pDS->next();
		}
    m_pDS->close();
		// now the secondary artists
		strSQL.Format("select * from album, exartistalbum, artist, path where artist.strArtist like '%s' and exartistalbum.idArtist = artist.idArtist and album.idAlbum = exartistalbum.idAlbum and album.idPath = path.idPath",strArtist.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		while (!m_pDS->eof())
		{
			albums.push_back(GetAlbumFromDataset());
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
		strSQL.Format("select * from song,path,album,artist,genre where song.strTitle LIKE '%%%s%%' and song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idArtist=artist.idArtist and song.idGenre=genre.idGenre", strSearch.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0) return false;
		while (!m_pDS->eof())
		{
			songs.push_back(GetSongFromDataset());
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
		// use a set as we don't need more than 1 of each thing
		set<CSong> setSongs;
		CStdString strSearch=strSearch1;
		RemoveInvalidChars(strSearch);

		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		// find songs that match in name
		strSQL.Format("select * from song,path,album,artist,genre where song.strTitle LIKE '%%%s%%' and song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idArtist=artist.idArtist and song.idGenre=genre.idGenre", strSearch.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		while (!m_pDS->eof())
		{
			//songs.push_back(GetSongFromDataset());
			setSongs.insert(GetSongFromDataset());
			m_pDS->next();
		}
    m_pDS->close();
		// and then songs that match in primary artist
		strSQL.Format("select * from song,path,album,artist,genre where artist.strArtist LIKE '%%%s%%' and song.idArtist=artist.idArtist and song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre", strSearch.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		while (!m_pDS->eof())
		{
			setSongs.insert(GetSongFromDataset());
			m_pDS->next();
		}
    m_pDS->close();
		// and then songs that match in the secondary artists
		strSQL.Format("select * from song,path,album,artist,exartistsong,genre where artist.strArtist LIKE '%%%s%%' and exartistsong.idArtist=artist.idArtist and exartistsong.idSong=song.idSong and song.idPath=path.idPath and song.idAlbum=album.idAlbum and song.idGenre=genre.idGenre", strSearch.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		while (!m_pDS->eof())
		{
			setSongs.insert(GetSongFromDataset());
			m_pDS->next();
		}
		m_pDS->close();
		// now dump our set back into our vector
		for (set<CSong>::iterator it=setSongs.begin(); it!=setSongs.end(); it++) songs.push_back(*it);
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
		// use a set for fast lookups
		set<CAlbum> setAlbums;
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		CStdString strSQL;
		// first get matching albums by name.
		strSQL.Format("select * from album,path,artist where album.strAlbum like '%%%s%%' and album.idPath=path.idPath and artist.idArtist=album.idArtist", strSearch.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		while (!m_pDS->eof())
		{
			setAlbums.insert(GetAlbumFromDataset());
			m_pDS->next();
		}
    m_pDS->close();
		// now try and match the primary artist.
		strSQL.Format("select * from album,path,artist where artist.strArtist like '%%%s%%' and artist.idArtist=album.idArtist and album.idPath=path.idPath", strSearch.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		while (!m_pDS->eof())
		{
			setAlbums.insert(GetAlbumFromDataset());
			m_pDS->next();
		}
    m_pDS->close();
		// and finally try the secondary artists.
		strSQL.Format("select * from album,path,artist,exartistalbum where artist.strArtist like '%%%s%%' and artist.idArtist=exartistalbum.idArtist and album.idAlbum=exartistalbum.idAlbum and album.idPath=path.idPath", strSearch.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		while (!m_pDS->eof())
		{
			setAlbums.insert(GetAlbumFromDataset());
			m_pDS->next();
		}
		m_pDS->close();
		// and copy our set into the vector
		for (set<CAlbum>::iterator it=setAlbums.begin(); it!=setAlbums.end(); it++) albums.push_back(*it);
		return true;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "CMusicDatabase:GetAlbumsByName() failed");
	}

	return false;
}

long CMusicDatabase::UpdateAlbumInfo(const CAlbum& album1, const VECSONGS& songs)
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

		// split our (possibly) multiple artist string into single artists.
		CStdStringArray vecArtists;
		long iNumArtists = StringUtils::SplitString(album.strArtist, " / ", vecArtists);
		// and also the multiple genre string into single genres.
		CStdStringArray vecGenres;
		long iNumGenres = StringUtils::SplitString(album.strGenre, " / ", vecGenres);

		long lArtistId = AddArtist(vecArtists[0]);
		long lGenreId = AddGenre(vecGenres[0]);
		long lPathId   = AddPath(album1.strPath);
		long lAlbumId  = AddAlbum(album1.strAlbum,lArtistId,iNumArtists,album1.strArtist,lPathId,album1.strPath);
		// add any extra artists/genres
		AddExtraArtists(vecArtists, 0, lAlbumId);
		AddExtraGenres(vecGenres, 0, lAlbumId);

		strSQL.Format("select * from albuminfo where idAlbum=%i", lAlbumId);
		if (!m_pDS->query(strSQL.c_str())) return -1;

		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound== 0)
		{
      m_pDS->close();
			return AddAlbumInfo(album1, songs);
		}

		long idAlbumInfo=m_pDS->fv("idAlbumInfo").get_asLong();
    m_pDS->close();


		strSQL.Format("update albuminfo set idAlbum=%i,idGenre=%i,iNumGenres=%i,strTones='%s',strStyles='%s',strReview='%s',strImage='%s',iRating=%i,iYear=%i where idAlbumInfo=%i",
												lAlbumId,lGenreId,iNumGenres,
												album.strTones.c_str(),
												album.strStyles.c_str(),
												album.strReview.c_str(),
												album.strImage.c_str(),
												album.iRating,
												album.iYear,
												idAlbumInfo);
		m_pDS->exec(strSQL.c_str());

		UpdateAlbumInfoSongs(idAlbumInfo, songs);

		return idAlbumInfo;
	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:unable to updatealbuminfo (%s)", strSQL.c_str());
	}

	return -1;
}

bool CMusicDatabase::UpdateAlbumInfoSongs(long idAlbumInfo, const VECSONGS& songs)
{
	CStdString strSQL;
	try
	{
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;

		strSQL.Format("delete from albuminfosong where idAlbumInfo=%i", idAlbumInfo);
		if (!m_pDS->exec(strSQL.c_str())) return false;

		return AddAlbumInfoSongs(idAlbumInfo, songs);

	}
	catch(...)
	{
    CLog::Log(LOGERROR, "musicdatabase:unable to updatealbuminfosongs (%s)", strSQL.c_str());
	}

	return false;
}

CStdString CMusicDatabase::GetSubpathsFromPath(const CStdString &strPath)
{
	CStdString strPathIds;
	try
	{
		if (NULL==m_pDB.get()) return strPathIds;
		if (NULL==m_pDS.get()) return strPathIds;
		CStdString strSQL;
		// get all the path id's that are sub dirs of this directory
		strSQL.Format("select idPath from path where strPath like '%s%%'", strPath.c_str());
		if (!m_pDS->query(strSQL.c_str())) return strPathIds;
		// create the idPath search string
		strPathIds = "(";
		while (!m_pDS->eof())
		{
			strPathIds += m_pDS->fv("idPath").get_asString() + ", ";
			m_pDS->next();
		}
		strPathIds.TrimRight(", ");
		strPathIds += ")";
    m_pDS->close();
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "Error in GetSubpathsFromPath()");
	}
	return strPathIds;
}

bool CMusicDatabase::RemoveSongsFromPaths(const CStdString &strPathIds)
{
	try
	{
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		// ok, now find all idSong's
		CStdString strSQL;
		strSQL.Format("select idSong from song where song.idPath in %s",strPathIds.c_str());
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound==0)
    {
      m_pDS->close();
			return true;
    }
		CStdString strSongIds = "(";
		while (!m_pDS->eof())
		{
			strSongIds += m_pDS->fv("idSong").get_asString() + ",";
			m_pDS->next();
		}
    m_pDS->close();
		strSongIds.TrimRight(",");
		strSongIds += ")";
		// ok, now delete all the paths
		strSQL = "delete from path where idPath in " + strPathIds;
		int iRet = m_pDS->exec(strSQL.c_str());
		// and all songs + exartistsongs and exgenresongs
		strSQL = "delete from song where idSong in " + strSongIds;
		m_pDS->exec(strSQL.c_str());
		strSQL = "delete from exartistsong where idSong in " + strSongIds;
		m_pDS->exec(strSQL.c_str());
		strSQL = "delete from exgenresong where idSong in " + strSongIds;
		m_pDS->exec(strSQL.c_str());
		m_pDS->close();
		return true;
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "RemoveSongsFromPath() failed!");
	}
	return false;
}

bool CMusicDatabase::CleanupAlbumsFromPaths(const CStdString &strPathIds)
{
	try
	{
		CStdString strSQL = "select * from album,path where album.idPath in " + strPathIds + " and album.idAlbum not in (select distinct idAlbum from song) and album.idPath=path.idPath";
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound==0)
    {
      m_pDS->close();
			return true;
    }
		// now we have all albums that need to be removed
		CStdString strAlbumIds = "(";
		while (!m_pDS->eof())
		{
			strAlbumIds += m_pDS->fv("album.idAlbum").get_asString() + ",";
			// delete the thumb
			CStdString strThumb;
			CUtil::GetAlbumThumb(m_pDS->fv("album.strAlbum").get_asString(), m_pDS->fv("path.strPath").get_asString(), strThumb);
			::DeleteFile(strThumb);
			m_pDS->next();
		}
    m_pDS->close();
		strAlbumIds.TrimRight(",");
		strAlbumIds += ")";
		// ok, now we can delete them and the references in the exartistalbum and exgenrealbum and albuminfo tables
		strSQL = "delete from album where idAlbum in " + strAlbumIds;
		m_pDS->exec(strSQL.c_str());
		strSQL = "delete from albuminfo where idAlbum in " + strAlbumIds;
		m_pDS->exec(strSQL.c_str());
		strSQL = "delete from exartistalbum where idAlbum in " + strAlbumIds;
		m_pDS->exec(strSQL.c_str());
		strSQL = "delete from exgenrealbum where idAlbum in " + strAlbumIds;
		m_pDS->exec(strSQL.c_str());
		// we must also cleanup any thumbs
		return true;
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupAlbumsFromPaths()");
	}
	return false;
}

bool CMusicDatabase::CleanupSongs()
{
	try
	{
		// run through all songs, checking for existence, and build up a string of those that no longer exist
		CStdString strSQL = "select * from song,path where song.idPath=path.idPath";
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
		if (iRowsFound==0)
    {
      m_pDS->close();
			return true;
    }

		CStdString strSongsToDelete = "(";
		while (!m_pDS->eof())
		{// get the full song path
			CStdString strFileName = m_pDS->fv("path.strPath").get_asString();
			strFileName += m_pDS->fv("song.strFileName").get_asString();
			if (!CUtil::FileExists(strFileName))
			{// file no longer exists, so add to deletion list
				strSongsToDelete += m_pDS->fv("song.idSong").get_asString() + ",";
			}
			m_pDS->next();
		}
    m_pDS->close();
		strSongsToDelete.TrimRight(",");
		strSongsToDelete += ")";
		// ok, now delete these songs + all references to them from the exartistsong and exgenresong tables
 		strSQL = "delete from song where idSong in " + strSongsToDelete;
		m_pDS->exec(strSQL.c_str());
		strSQL = "delete from exartistsong where idSong in " + strSongsToDelete;
		m_pDS->exec(strSQL.c_str());
		strSQL = "delete from exgenresong where idSong in " + strSongsToDelete;
		m_pDS->exec(strSQL.c_str());
		m_pDS->close();
		return true;
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupSongs()");
	}
	return false;
}

bool CMusicDatabase::CleanupAlbums()
{
	try
	{
		// This must be run AFTER songs have been cleaned up
		// delete albums with no reference to songs
		// TODO: shouldn't really delete them if they contain albuminfo
		CStdString strSQL = "select * from album,path where album.idAlbum not in (select distinct idAlbum from song) and album.idPath=path.idPath";
//		strSQL += " or idAlbum not in (select distinct idAlbum from albuminfo)";
		if (!m_pDS->query(strSQL.c_str())) return false;
		int iRowsFound = m_pDS->num_rows();
    if (iRowsFound==0)
    {
      m_pDS->close();
			return true;
    }
		CStdString strAlbumIds = "(";
		while (!m_pDS->eof())
		{
			strAlbumIds += m_pDS->fv("album.idAlbum").get_asString() + ",";
			// delete the thumb for this file, if it has been cached
			CStdString strThumb;
			CUtil::GetAlbumThumb(m_pDS->fv("album.strAlbum").get_asString(), m_pDS->fv("path.strPath").get_asString(), strThumb);
			::DeleteFile(strThumb);
			m_pDS->next();
		}
    m_pDS->close();
		// we should also delete albums with invalid paths
/*		strSQL = "select * from album,path where album.idPath=path.idPath";
		if (!m_pDS->query(strSQL.c_str())) return false;
		while (!m_pDS->eof())
		{
			if (!CUtil::FileExists(m_pDS->fv("path.strPath").get_asString()))
			{
				strAlbumIds += m_pDS->fv("album.idAlbum").get_asString() + ",";
				// delete the thumb for this file, if it has been cached
				CStdString strThumb;
				CUtil::GetAlbumThumb(m_pDS->fv("album.strAlbum").get_asString(), m_pDS->fv("path.strPath").get_asString(), strThumb);
				::DeleteFile(strThumb);
			}
			m_pDS->next();
		}*/
		strAlbumIds.TrimRight(",");
		strAlbumIds += ")";
		// ok, now we can delete them and the references in the exartistalbum, exgenrealbum and albuminfo tables
		strSQL = "delete from album where idAlbum in " + strAlbumIds;
		m_pDS->exec(strSQL.c_str());
		strSQL = "delete from albuminfo where idAlbum in " + strAlbumIds;
		m_pDS->exec(strSQL.c_str());
		strSQL = "delete from exartistalbum where idAlbum in " + strAlbumIds;
		m_pDS->exec(strSQL.c_str());
		strSQL = "delete from exgenrealbum where idAlbum in " + strAlbumIds;
		m_pDS->exec(strSQL.c_str());
		return true;
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupAlbums()");
	}
	return false;
}

bool CMusicDatabase::CleanupPaths()
{
	try
	{
		// needs to be done AFTER the songs and albums have been cleaned up.
		// we can happily delete any path that has no reference to a song or album (we need album, as the
		// user can have albuminfo without having song info for that particular album)
		CStdString strSQL = "delete from path where idPath not in (select distinct idPath from song)";
		strSQL += " and idPath not in (select distinct idPath from album)";
		m_pDS->exec(strSQL.c_str());
		return true;
	}
	catch(...)
	{
		CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupPaths()");
	}
	return false;
}

bool CMusicDatabase::CleanupArtists()
{
	try
	{
		// (nested queries by Bobbin007)
		// must be executed AFTER the song, exartistsong, album and exartistalbum tables are cleaned.
		// don't delete the "Various Artists" string
		CStdString strVariousArtists=g_localizeStrings.Get(340);
		long lVariousArtistsId = AddArtist(strVariousArtists);
		CStdString strSQL = "delete from artist where idArtist not in (select distinct idArtist from song)";
		strSQL += " and idArtist not in (select distinct idArtist from exartistsong)";
		strSQL += " and idArtist not in (select distinct idArtist from album)";
		strSQL += " and idArtist not in (select distinct idArtist from exartistalbum)";
		CStdString strSQL2;
		strSQL2.Format(" and idArtist<>%i",lVariousArtistsId);
		strSQL += strSQL2;
		m_pDS->exec(strSQL.c_str());
		return true;
	}
	catch(...)
	{
	    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupArtists()");
	}
	return false;
}

bool CMusicDatabase::CleanupGenres()
{
	try
	{
		// Cleanup orphaned genres (ie those that don't belong to a song or an albuminfo entry)
		// (nested queries by Bobbin007)
		// Must be executed AFTER the song, exgenresong, albuminfo and exgenrealbum tables have been cleaned.
		CStdString strSQL = "delete from genre where idGenre not in (select distinct idGenre from song) and";
		strSQL += " idGenre not in (select distinct idGenre from exgenresong) and";
		strSQL += " idGenre not in (select distinct idGenre from albuminfo) and";
		strSQL += " idGenre not in (select distinct idGenre from exgenrealbum)";
		m_pDS->exec(strSQL.c_str());
		return true;
	}
	catch(...)
	{
	    CLog::Log(LOGERROR, "Exception in CMusicDatabase::CleanupGenres()");
	}
	return false;
}

bool CMusicDatabase::CleanupAlbumsArtistsGenres(const CStdString &strPathIds)
{
	if (NULL==m_pDB.get()) return false;
	if (NULL==m_pDS.get()) return false;
	// first cleanup any albums that are about
	if (!CleanupAlbumsFromPaths(strPathIds)) return false;
	if (!CleanupArtists()) return false;
	if (!CleanupGenres()) return false;
	if (!CleanupPaths()) return false;
	return true;
}

int CMusicDatabase::Cleanup(CGUIDialogProgress *pDlgProgress)
{
	Open();
	if (NULL==m_pDB.get()) return ERROR_DATABASE;
	if (NULL==m_pDS.get()) return ERROR_DATABASE;
	// first cleanup any songs with invalid paths
	pDlgProgress->SetLine(0, "");
	pDlgProgress->SetLine(1, 318);
	pDlgProgress->SetLine(2, 330);
	pDlgProgress->SetPercentage(0);
	pDlgProgress->Progress();
	if (!CleanupSongs())
	{
		RollbackTransaction();
		return ERROR_REORG_SONGS;
	}
	// then the albums that are not linked to a song or to albuminfo, or whose path is removed
	pDlgProgress->SetLine(1, 326);
	pDlgProgress->SetPercentage(20);
	pDlgProgress->Progress();
	if (!CleanupAlbums())
	{
		RollbackTransaction();
		return ERROR_REORG_ALBUM;
	}
	// now the paths
	pDlgProgress->SetLine(1, 324);
	pDlgProgress->SetPercentage(40);
	pDlgProgress->Progress();
	if (!CleanupPaths())
	{
		RollbackTransaction();
		return ERROR_REORG_PATH;
	}
	// and finally artists + genres
	pDlgProgress->SetLine(1, 320);
	pDlgProgress->SetPercentage(60);
	pDlgProgress->Progress();
	if (!CleanupArtists())
	{
		RollbackTransaction();
		return ERROR_REORG_ARTIST;
	}
	pDlgProgress->SetLine(1, 322);
	pDlgProgress->SetPercentage(80);
	pDlgProgress->Progress();
	if (!CleanupGenres())
	{
		RollbackTransaction();
		return ERROR_REORG_GENRE;
	}
	// commit transaction
	pDlgProgress->SetLine(1, 328);
	pDlgProgress->SetPercentage(90);
	pDlgProgress->Progress();
	if (!CommitTransaction())
	{
		RollbackTransaction();
		return ERROR_WRITING_CHANGES;
	}
	// and compress the database
	pDlgProgress->SetLine(1, 331);
	pDlgProgress->SetPercentage(100);
	pDlgProgress->Progress();
	if (!Compress())
	{
		return ERROR_COMPRESSING;
	}
	return ERROR_OK;
}

bool CMusicDatabase::Compress()
{
	//	compress database
	try
	{
		if (NULL==m_pDB.get()) return false;
		if (NULL==m_pDS.get()) return false;
		if (!m_pDS->exec("vacuum"))
			return false;
	}
	catch(...)
	{
		OutputDebugString("-------ERRORohoh!");
		return false;
	}
	return true;
}

void CMusicDatabase::DeleteAlbumInfo()
{
	// open our database
	Open();
	if (NULL==m_pDB.get()) return;
	if (NULL==m_pDS.get()) return;

	CStdString strSQL;
	strSQL.Format("select * from albuminfo,album,path,artist where album.idPath=path.idPath and albuminfo.idAlbum=album.idAlbum and album.idArtist=artist.idArtist order by album.strAlbum");
	if (!m_pDS->query(strSQL.c_str())) return;
	int iRowsFound = m_pDS->num_rows();
	if (iRowsFound== 0)
	{
    m_pDS->close();
		CGUIDialogOK *pDlg= (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
		if (pDlg)
		{
			pDlg->SetHeading(313);
			pDlg->SetLine(0, 425);
			pDlg->SetLine(1, "");
			pDlg->SetLine(2, "");
			pDlg->DoModal(m_gWindowManager.GetActiveWindow());
		}
		return;
	}
	vector<CAlbumCache> vecAlbums;
	while (!m_pDS->eof())
	{
		CAlbumCache album;
		album.idAlbum   = m_pDS->fv("album.idAlbum").get_asLong() ;
		album.strAlbum	= m_pDS->fv("album.strAlbum").get_asString();
		album.strArtist	= m_pDS->fv("artist.strArtist").get_asString();
		if (m_pDS->fv("album.iNumArtists").get_asLong() > 1)
		{
			auto_ptr<Dataset> pDS;
			pDS.reset(m_pDB->CreateDataset());
			CStdString strSQL;
			strSQL.Format("select artist.strArtist from exartistalbum, artist where exartistalbum.idAlbum=%i and exartistalbum.idArtist=artist.idArtist order by exartistalbum.iPosition", album.idAlbum);
			if (NULL==pDS.get() || !pDS->query(strSQL.c_str())) return;
			while (!pDS->eof())
			{
				album.strArtist += " / " + pDS->fv("artist.strArtist").get_asString();
				pDS->next();
			}
			pDS->close();
		}
		album.strPath   = m_pDS->fv("path.strPath").get_asString();
		vecAlbums.push_back(album);
		m_pDS->next();
	}
  m_pDS->close();

	//	Show a selectdialog that the user can select the albuminfo to delete
	const WCHAR* szText=g_localizeStrings.Get(181).c_str();
	CGUIDialogSelect *pDlg= (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (pDlg)
  {
		pDlg->SetHeading(szText);
		pDlg->Reset();
		for (int i=0; i < (int)vecAlbums.size(); ++i)
		{
			CMusicDatabase::CAlbumCache& album = vecAlbums[i];
			pDlg->Add(album.strAlbum + " - " + album.strArtist);
		}
		pDlg->DoModal(m_gWindowManager.GetActiveWindow());

		// and wait till user selects one
		int iSelectedAlbum= pDlg->GetSelectedLabel();
		if (iSelectedAlbum<0)
		{
			vecAlbums.erase(vecAlbums.begin(), vecAlbums.end());
			return;
		}

		CAlbumCache& album = vecAlbums[iSelectedAlbum];
		strSQL.Format("delete from albuminfo where albuminfo.idAlbum=%i", album.idAlbum);
		if (!m_pDS->exec(strSQL.c_str())) return;

		vecAlbums.erase(vecAlbums.begin(), vecAlbums.end());
  }
}

void CMusicDatabase::DeleteCDDBInfo()
{
	WIN32_FIND_DATA wfd;
	memset(&wfd,0,sizeof(wfd));

	CStdString strCDDBFileMask;
	strCDDBFileMask.Format("%s\\cddb\\*.cddb",g_stSettings.m_szAlbumDirectory);

	map<ULONG, CStdString> mapCDDBIds;

	CAutoPtrFind hFind( FindFirstFile(strCDDBFileMask.c_str(),&wfd));
	if (!hFind.isValid())
	{
		CGUIDialogOK *pDlg= (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
		if (pDlg)
		{
			pDlg->SetHeading(313);
			pDlg->SetLine(0, 426);
			pDlg->SetLine(1, "");
			pDlg->SetLine(2, "");
			pDlg->DoModal(m_gWindowManager.GetActiveWindow());
		}
		return;
	}

	//	Show a selectdialog that the user can select the albuminfo to delete
	const WCHAR* szText=g_localizeStrings.Get(181).c_str();
	CGUIDialogSelect *pDlg= (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (pDlg)
  {
		pDlg->SetHeading(szText);
		pDlg->Reset();
		CStdString strDir;
		strDir.Format("%s\\cddb", g_stSettings.m_szAlbumDirectory);
		do
		{
			if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			{
				CStdString strFile=wfd.cFileName;
				strFile.Delete(strFile.size()-5, 5);
				ULONG lDiscId=strtoul(strFile.c_str(), NULL, 16);
				Xcddb cddb;
				cddb.setCacheDir(strDir);

				if (!cddb.queryCache(lDiscId))
					continue;

				CStdString strDiskTitle, strDiskArtist;
				cddb.getDiskTitle(strDiskTitle);
				cddb.getDiskArtist(strDiskArtist);

				CStdString str;
				if (strDiskArtist.IsEmpty())
					str=strDiskTitle;
				else
					str=strDiskTitle + " - " + strDiskArtist;

				pDlg->Add(str);
				mapCDDBIds.insert(pair<ULONG, CStdString>(lDiscId, str));
			}
		} while (FindNextFile(hFind, &wfd));

		pDlg->Sort();
		pDlg->DoModal(m_gWindowManager.GetActiveWindow());

		// and wait till user selects one
		int iSelectedAlbum= pDlg->GetSelectedLabel();
		if (iSelectedAlbum<0)
		{
			mapCDDBIds.erase(mapCDDBIds.begin(), mapCDDBIds.end());
			return;
		}

		CStdString strSelectedAlbum=pDlg->GetSelectedLabelText();
		map<ULONG,CStdString>::iterator it;
		for (it=mapCDDBIds.begin();it!=mapCDDBIds.end();it++)
		{
			if (it->second==strSelectedAlbum)
			{
				CStdString strFile;
				strFile.Format("%s\\cddb\\%x.cddb",g_stSettings.m_szAlbumDirectory, it->first );
				::DeleteFile(strFile.c_str());
				break;

			}
		}
		mapCDDBIds.erase(mapCDDBIds.begin(), mapCDDBIds.end());
	}
}

/*
void CMusicDatabaseReorg::DeleteSingleAlbum()
{
	// CMusicDatabaseReorg is friend of CMusicDatabase
	if (NULL==g_musicDatabase.m_pDB.get()) return;

	// use the same databaseobject as CMusicDatabase
	// to rollback transactions even if CMusicDatabase
	// memberfunctions are called; create our working dataset
	m_pDSReorg.reset(g_musicDatabase.m_pDB->CreateDataset());
	if (NULL==m_pDSReorg.get()) return;

	CStdString strSQL;
	strSQL.Format("select * from album,path where album.idPath=path.idPath order by album.strAlbum");
	if (!m_pDSReorg->query(strSQL.c_str())) return;
	int iRowsFound = m_pDSReorg->num_rows();
	if (iRowsFound== 0)
	{
		CGUIDialogOK *pDlg= (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
		if (pDlg)
		{
			pDlg->SetHeading(313);
			pDlg->SetLine(0, 425);
			pDlg->SetLine(1, "");
			pDlg->SetLine(2, "");
			pDlg->DoModal(m_gWindowManager.GetActiveWindow());
		}
		return;
	}
	vector<CMusicDatabase::CAlbumCache> vecAlbums;
	while (!m_pDSReorg->eof())
	{
		CMusicDatabase::CAlbumCache album;
		album.idAlbum   = m_pDSReorg->fv("album.idAlbum").get_asLong() ;
		album.strAlbum	= m_pDSReorg->fv("album.strAlbum").get_asString();
//		album.idArtist  = m_pDSReorg->fv("album.idArtist").get_asLong();
		album.strArtist	= m_pDSReorg->fv("album.strArtist").get_asString();
		album.idPath    = m_pDSReorg->fv("path.idPath").get_asLong();
		album.strPath   = m_pDSReorg->fv("path.strPath").get_asString();
		vecAlbums.push_back(album);
		m_pDSReorg->next();
	}

	//	Show a selectdialog that the user can select the album to delete
	const WCHAR* szText=g_localizeStrings.Get(181).c_str();
	CGUIDialogSelect *pDlg= (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (pDlg)
  {
		pDlg->SetHeading(szText);
		pDlg->Reset();
		for (int i=0; i < (int)vecAlbums.size(); ++i)
		{
			CMusicDatabase::CAlbumCache& album = vecAlbums[i];
			pDlg->Add(album.strAlbum + " - " + album.strArtist);
		}
		pDlg->DoModal(m_gWindowManager.GetActiveWindow());

		// and wait till user selects one
		int iSelectedAlbum= pDlg->GetSelectedLabel();
		if (iSelectedAlbum<0)
		{
			vecAlbums.erase(vecAlbums.begin(), vecAlbums.end());
			return;
		}

		CMusicDatabase::CAlbumCache& album = vecAlbums[iSelectedAlbum];

		g_musicDatabase.BeginTransaction();
		//	Delete album
		strSQL.Format("delete from album where idAlbum=%i", album.idAlbum);
		if (!m_pDSReorg->exec(strSQL.c_str()))
		{
			g_musicDatabase.RollbackTransaction();
			return;
		}
		//  Delete from album2genre and album2artist
		strSQL.Format("delete from album2artist where idAlbum=%i", album.idAlbum);
		if (!m_pDSReorg->exec(strSQL.c_str()))
		{
			g_musicDatabase.RollbackTransaction();
			return;
		}
		strSQL.Format("delete from album2genre where idAlbum=%i", album.idAlbum);
		if (!m_pDSReorg->exec(strSQL.c_str()))
		{
			g_musicDatabase.RollbackTransaction();
			return;
		}


		//	Delete album thumb
		CStdString strThumb;
		CUtil::GetAlbumThumb(album.strAlbum, album.strPath, strThumb);
		::DeleteFile(strThumb);


		//	Delete album info
		strSQL.Format("delete from albuminfo where idAlbum=%i", album.idAlbum);
		if (!m_pDSReorg->exec(strSQL.c_str()))
		{
			g_musicDatabase.RollbackTransaction();
			return;
		}

		//	Delete artist

		//	Get the songs of the album
		strSQL.Format("select * from song where idAlbum=%i", album.idAlbum);
		if (!m_pDSReorg->query(strSQL.c_str())) return;
		int iRowsFound = m_pDSReorg->num_rows();
		if (iRowsFound!= 0)
		{
			//	Get all artists of this album
			while (!m_pDSReorg->eof())
			{
				m_artists.insert(m_pDSReorg->fv("strArtist").get_asString());
				m_pDSReorg->next();
			}

			//	Do we have another song of this artist?
			for (it2=m_artists.begin(); it2!=m_artists.end(); it2++)
			{
				strSQL.Format("select * from song, song2artist, artist where artist.strArtist='%s' and song2artist.idArtist=artist.idArtist and song.idSong=song2artist.idSong and idAlbum<>%i", *it2, album.idAlbum);
				if (!m_pDSReorg->query(strSQL.c_str())) return;
				int iRowsFound = m_pDSReorg->num_rows();
				if (iRowsFound==0)
				{
					//	No, delete the artist
					strSQL.Format("delete from artist where strArtist='%s'", *it2);
					if (!m_pDSReorg->exec(strSQL.c_str()))
					{
						g_musicDatabase.RollbackTransaction();
						return;
					}
				}
			}
			m_artists.erase(m_artists.begin(), m_artists.end());
		}

		//	Delete the albums songs
		strSQL.Format("delete from song where idAlbum=%i", album.idAlbum);
		if (!m_pDSReorg->exec(strSQL.c_str()))
		{
			g_musicDatabase.RollbackTransaction();
			return;
		}

		//	Do we have another song in this path?
		strSQL.Format("select * from song where idPath=%i", album.idPath);
		if (!m_pDSReorg->query(strSQL.c_str()))
		{
			g_musicDatabase.RollbackTransaction();
			return;
		}

		iRowsFound = m_pDSReorg->num_rows();
		if (iRowsFound== 0)
		{
			//	Delete path
			strSQL.Format("delete from path where idPath=%i", album.idPath);
			if (!m_pDSReorg->exec(strSQL.c_str()))
			{
				g_musicDatabase.RollbackTransaction();
				return;
			}

		}

		g_musicDatabase.CommitTransaction();
		vecAlbums.erase(vecAlbums.begin(), vecAlbums.end());
  }

}
*/
