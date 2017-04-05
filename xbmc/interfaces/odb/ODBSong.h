/*
 *      Copyright (C) 2017 Team Kodi
 *      https://kodi.tv
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef ODBSONG_H
#define ODBSONG_H

#include <odb/core.hxx>

#include "ODBAlbum.h"
#include "ODBGenre.h"
#include "ODBPersonLink.h"
#include "ODBFile.h"
#include "ODBDate.h"
#include "ODBArtistDetail.h"

#include <string>

PRAGMA_DB (model version(1, 1, open))

PRAGMA_DB (object pointer(std::shared_ptr) \
                  table("song"))
class CODBSong
{
public:
  CODBSong()
  {
    m_track = 0;
    m_duration = 0;
    m_year = 0;
    m_startOffset = 0;
    m_endOffset = 0;
    m_rating = 0.0;
    m_userrating = 0;
    m_votes = 0;
    
    m_synced = false;
  };
  
PRAGMA_DB (id auto)
  unsigned long m_idSong;
  std::string m_title;
  int m_track;
  int m_duration;
  int m_year;
  std::string m_musicBrainzTrackID;
  int m_startOffset;
  int m_endOffset;
  float m_rating;
  int m_userrating;
  std::string m_comment;
  std::string m_mood;
  int m_votes;
  std::string m_replayGain;
  
  std::string m_artistDisp;
  std::string m_artistSort;
  std::string m_genresString;
  
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBAlbum> m_album;
PRAGMA_DB (section(section_foreign))
  odb::lazy_shared_ptr<CODBFile> m_file;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBPersonLink> > m_artists;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBGenre> > m_genres;
PRAGMA_DB (section(section_foreign))
  std::vector< odb::lazy_shared_ptr<CODBArt> > m_artwork;
  
  //Members not stored in the db, used for sync ...
PRAGMA_DB (transient)
  bool m_synced;
  
PRAGMA_DB (load(lazy) update(change))
  odb::section section_foreign;
  
private:
  friend class odb::access;
};

PRAGMA_DB (view object(CODBSong) \
                object(CODBAlbum: CODBSong::m_album) \
                object(CODBPersonLink inner: CODBSong::m_artists) \
                object(CODBPerson inner: CODBPersonLink::m_person) \
                object(CODBRole: CODBPersonLink::m_role) \
                object(CODBPersonLink = albumArtistLink: CODBAlbum::m_artists) \
                object(CODBPerson = albumArist: albumArtistLink::m_person) \
                object(CODBRole = albumArtistRole: albumArtistLink::m_role) \
                object(CODBGenre: CODBAlbum::m_genres) \
                object(CODBFile: CODBSong::m_file) \
                object(CODBPath: CODBFile::m_path) \
                query(distinct))
struct ODBView_Song
{
  std::shared_ptr<CODBSong> song;
};

PRAGMA_DB (view object(CODBSong) \
           object(CODBAlbum: CODBSong::m_album) \
           object(CODBPersonLink inner: CODBSong::m_artists) \
           object(CODBPerson inner: CODBPersonLink::m_person) \
           object(CODBRole: CODBPersonLink::m_role) \
           object(CODBPersonLink = albumArtistLink: CODBAlbum::m_artists) \
           object(CODBPerson = albumArist: albumArtistLink::m_person) \
           object(CODBRole = albumArtistRole: albumArtistLink::m_role) \
           object(CODBGenre: CODBAlbum::m_genres) \
           object(CODBFile: CODBSong::m_file) \
           object(CODBPath: CODBFile::m_path))
struct ODBView_Song_Total
{
  PRAGMA_DB (column("COUNT(DISTINCT(" + CODBSong::m_idSong + "))"))
  unsigned int total;
};

PRAGMA_DB (view object(CODBSong) \
           object(CODBAlbum inner: CODBSong::m_album) \
           object(CODBPersonLink inner: CODBAlbum::m_artists) \
           object(CODBPerson inner: CODBPersonLink::m_person) \
           object(CODBRole: CODBPersonLink::m_role) \
           object(CODBPersonLink = songArtistLink: CODBSong::m_artists) \
           object(CODBPerson = songArist: songArtistLink::m_person) \
           object(CODBRole = songArtistRole: songArtistLink::m_role) \
           object(CODBGenre: CODBAlbum::m_genres) \
           query(distinct) )
struct ODBView_Album
{
  std::shared_ptr<CODBAlbum> album;
};

PRAGMA_DB (view object(CODBSong) \
           object(CODBAlbum inner: CODBSong::m_album) \
           object(CODBPersonLink inner: CODBAlbum::m_artists) \
           object(CODBPerson inner: CODBPersonLink::m_person) \
           object(CODBArtistDetail inner: CODBPersonLink::m_person) \
           object(CODBRole: CODBPersonLink::m_role) \
           object(CODBPersonLink = songArtistLink: CODBSong::m_artists) \
           object(CODBPerson = songArist: songArtistLink::m_person) \
           object(CODBRole = songArtistRole: songArtistLink::m_role) \
           object(CODBGenre: CODBAlbum::m_genres) \
           query(distinct) )
struct ODBView_Album_Artist_Detail
{
  std::shared_ptr<CODBArtistDetail> detail;
};

PRAGMA_DB (view object(CODBSong) \
           object(CODBAlbum inner: CODBSong::m_album) \
           object(CODBPersonLink inner: CODBAlbum::m_artists) \
           object(CODBPerson inner: CODBPersonLink::m_person) \
           object(CODBRole: CODBPersonLink::m_role) \
           object(CODBPersonLink = songArtistLink: CODBSong::m_artists) \
           object(CODBPerson = songArist: songArtistLink::m_person) \
           object(CODBRole = songArtistRole: songArtistLink::m_role) \
           object(CODBGenre: CODBAlbum::m_genres))
struct ODBView_Album_Total
{
  PRAGMA_DB (column("COUNT(DISTINCT(" + CODBAlbum::m_idAlbum+ "))"))
  unsigned int total;
};

PRAGMA_DB (view object(CODBSong) \
                object(CODBAlbum: CODBSong::m_album) \
                object(CODBPersonLink: CODBSong::m_artists) \
                object(CODBPerson: CODBPersonLink::m_person) \
                query(distinct))
struct ODBView_Song_Album_Artist
{
  std::shared_ptr<CODBSong> song;
};

PRAGMA_DB (view object(CODBSong) \
                object(CODBArt: CODBSong::m_artwork) \
                query(distinct))
struct ODBView_Song_Art
{
  std::shared_ptr<CODBArt> art;
};

PRAGMA_DB (view object(CODBSong) \
                object(CODBAlbum: CODBSong::m_album) \
                object(CODBFile inner: CODBSong::m_file) \
                object(CODBPath inner: CODBFile::m_path) \
                query(distinct))
struct ODBView_Song_Album_File_Path
{
  std::shared_ptr<CODBSong> song;
  std::shared_ptr<CODBPath> path;
};

PRAGMA_DB (view object(CODBSong) \
                object(CODBAlbum: CODBSong::m_album) \
                object(CODBPersonLink: CODBSong::m_artists) \
                object(CODBPerson: CODBPersonLink::m_person) \
                object(CODBFile inner: CODBSong::m_file) \
                object(CODBPath inner: CODBFile::m_path) \
                query(distinct))
struct ODBView_Song_Artist_Paths
{
  std::shared_ptr<CODBPath> path;
};

PRAGMA_DB (view object(CODBSong) \
                object(CODBAlbum: CODBSong::m_album) \
                object(CODBArt inner: CODBSong::m_artwork) \
                query(distinct))
struct ODBView_Song_Album_Art
{
  std::shared_ptr<CODBArt> art;
};

PRAGMA_DB (view object(CODBSong) \
                object(CODBAlbum: CODBSong::m_album) \
                object(CODBFile inner: CODBSong::m_file) \
                query(distinct))
struct ODBView_Album_File_Details
{
  PRAGMA_DB (column("MAX(" + CODBFile::m_lastPlayed.m_ulong_date + ")"))
  unsigned long lastPlayedULong;
  
  PRAGMA_DB (column("COUNT(" + CODBFile::m_playCount + ")"))
  unsigned int watchedCount;
  
  PRAGMA_DB (column("MAX(" + CODBFile::m_dateAdded.m_ulong_date + ")"))
  unsigned long dateAddedULong;
};

PRAGMA_DB (view object(CODBSong) \
                object(CODBPersonLink inner: CODBSong::m_artists) \
                object(CODBPerson: CODBPersonLink::m_person) \
                query(distinct))
struct ODBView_Song_Artists_Link
{
  std::shared_ptr<CODBPersonLink> artist;
};

PRAGMA_DB (view object(CODBSong) \
                object(CODBPersonLink: CODBSong::m_artists) \
                object(CODBPerson: CODBPersonLink::m_person) \
                object(CODBGenre inner: CODBSong::m_genres) \
                query(distinct))
struct ODBView_Artist_Genres
{
  std::shared_ptr<CODBGenre> genre;
};

PRAGMA_DB (view object(CODBSong) \
                object(CODBAlbum inner: CODBSong::m_album) \
                object(CODBFile: CODBSong::m_file) \
                object(CODBPath: CODBFile::m_path) \
                query(distinct))
struct ODBView_Album_File_Paths
{
  std::shared_ptr<CODBAlbum> album;
};

PRAGMA_DB (view object(CODBSong) \
                object(CODBAlbum: CODBSong::m_album) \
                object(CODBPersonLink: CODBSong::m_artists) \
                object(CODBRole: CODBPersonLink::m_role) \
                object(CODBGenre inner: CODBSong::m_genres) \
                query(distinct))
struct ODBView_Music_Genres
{
  std::shared_ptr<CODBGenre> genre;
};

PRAGMA_DB (view object(CODBSong) \
                object(CODBPersonLink: CODBSong::m_artists) \
                object(CODBRole inner: CODBPersonLink::m_role) \
                query(distinct))
struct ODBView_Music_Roles
{
  std::shared_ptr<CODBRole> role;
};

PRAGMA_DB (view object(CODBSong) \
           object(CODBAlbum: CODBSong::m_album) \
           object(CODBPersonLink inner: CODBSong::m_artists) \
           object(CODBPerson inner: CODBPersonLink::m_person) \
           object(CODBRole: CODBPersonLink::m_role) \
           object(CODBPersonLink = albumArtistLink: CODBAlbum::m_artists) \
           object(CODBPerson = albumArist: albumArtistLink::m_person) \
           object(CODBRole = albumArtistRole: albumArtistLink::m_role) \
           object(CODBArtistDetail: CODBPersonLink::m_person == CODBArtistDetail::m_person) \
           object(CODBGenre = genre: CODBSong::m_genres) \
           object(CODBFile: CODBSong::m_file) \
           object(CODBPath: CODBFile::m_path) \
           query(distinct) )
struct ODBView_Song_Artists
{
  std::shared_ptr<CODBPerson> artist;
};

PRAGMA_DB (view object(CODBSong) \
           object(CODBAlbum: CODBSong::m_album) \
           object(CODBPersonLink inner: CODBSong::m_artists) \
           object(CODBPerson inner: CODBPersonLink::m_person) \
           object(CODBRole: CODBPersonLink::m_role) \
           object(CODBPersonLink = albumArtistLink: CODBAlbum::m_artists) \
           object(CODBPerson = albumArist: albumArtistLink::m_person) \
           object(CODBRole = albumArtistRole: albumArtistLink::m_role) \
           object(CODBArtistDetail inner: CODBPersonLink::m_person == CODBArtistDetail::m_person) \
           object(CODBGenre = genre: CODBSong::m_genres) \
           object(CODBFile: CODBSong::m_file) \
           object(CODBPath: CODBFile::m_path) \
           query(distinct) )
struct ODBView_Song_Artist_Detail
{
  std::shared_ptr<CODBArtistDetail> detail;
};

PRAGMA_DB (view object(CODBSong) \
           object(CODBAlbum: CODBSong::m_album) \
           object(CODBPersonLink inner: CODBSong::m_artists) \
           object(CODBPerson inner: CODBPersonLink::m_person) \
           object(CODBRole: CODBPersonLink::m_role) \
           object(CODBPersonLink = albumArtistLink: CODBAlbum::m_artists) \
           object(CODBPerson = albumArist: albumArtistLink::m_person) \
           object(CODBRole = albumArtistRole: albumArtistLink::m_role) \
           object(CODBArtistDetail: CODBPersonLink::m_person == CODBArtistDetail::m_person) \
           object(CODBGenre = genre: CODBSong::m_genres) \
           object(CODBFile: CODBSong::m_file) \
           object(CODBPath: CODBFile::m_path) \
           query(distinct) )
struct ODBView_Song_Artists_Total
{
  PRAGMA_DB (column("COUNT(DISTINCT(" + CODBPerson::m_idPerson + "))"))
  unsigned int total;
};

PRAGMA_DB (view object(CODBSong)
          object(CODBAlbum: CODBSong::m_album) ) \
struct ODBView_Song_Count
{
  PRAGMA_DB (column("count(1)"))
  std::size_t count;
};

PRAGMA_DB (view object(CODBSong) \
           object(CODBFile inner: CODBSong::m_file) \
           object(CODBPath inner: CODBFile::m_path) \
           query(distinct))
struct ODBView_Song_Paths
{
  std::shared_ptr<CODBPath> path;
};

PRAGMA_DB (view object(CODBSong) \
                object(CODBPersonLink: CODBSong::m_artists) \
                object(CODBPerson: CODBPersonLink::m_person) \
                object(CODBRole: CODBPersonLink::m_role) \
                object(CODBArt inner: CODBPerson::m_art) \
                query(distinct))
struct ODBView_Song_Artist_Art
{
  std::shared_ptr<CODBArt> art;
};


#endif /* ODBSONG_H */
