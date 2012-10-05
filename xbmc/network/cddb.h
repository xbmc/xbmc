#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h" // for HAS_DVD_DRIVE

#ifdef HAS_DVD_DRIVE

#include <sstream>
#include <iostream>
#include <map>
#ifndef _LINUX
#include <strstream>
#endif
#include "storage/cdioSupport.h"

#include "utils/AutoPtrHandle.h"

namespace CDDB
{

//Can be removed if/when removing Xcddb::queryCDinfo(int real_track_count, toc cdtoc[])
//#define IN_PROGRESS           -1
//#define QUERRY_OK             7
//#define E_INEXACT_MATCH_FOUND      211
//#define W_CDDB_already_shook_hands      402
//#define E_CDDB_Handshake_not_successful 431

#define E_TOC_INCORRECT           2
#define E_NETWORK_ERROR_OPEN_SOCKET     3
#define E_NETWORK_ERROR_SEND       4
#define E_WAIT_FOR_INPUT         5
#define E_PARAMETER_WRONG         6
#define E_NO_MATCH_FOUND        202

#define CDDB_PORT 8880


struct toc
{
  int min;
  int sec;
  int frame;
};


class Xcddb
{
public:
  Xcddb();
  virtual ~Xcddb();
  void setCDDBIpAdress(const CStdString& ip_adress);
  void setCacheDir(const CStdString& pCacheDir );

//  int queryCDinfo(int real_track_count, toc cdtoc[]);
  bool queryCDinfo(MEDIA_DETECT::CCdInfo* pInfo, int inexact_list_select);
  bool queryCDinfo(MEDIA_DETECT::CCdInfo* pInfo);
  int getLastError() const;
  const char * getLastErrorText() const;
  const CStdString& getYear() const;
  const CStdString& getGenre() const;
  const CStdString& getTrackArtist(int track) const;
  const CStdString& getTrackTitle(int track) const;
  void getDiskArtist(CStdString& strdisk_artist) const;
  void getDiskTitle(CStdString& strdisk_title) const;
  const CStdString& getTrackExtended(int track) const;
  unsigned long calc_disc_id(int nr_of_tracks, toc cdtoc[]);
  const CStdString& getInexactArtist(int select) const;
  const CStdString& getInexactTitle(int select) const;
  bool queryCache( unsigned long discid );
  bool writeCacheFile( const char* pBuffer, unsigned long discid );
  bool isCDCached( int nr_of_tracks, toc cdtoc[] );
  bool isCDCached( MEDIA_DETECT::CCdInfo* pInfo );

protected:
  CStdString m_strNull;
  AUTOPTR::CAutoPtrSocket m_cddb_socket;
  const static int recv_buffer = 4096;
  int m_lastError;
  std::map<int, CStdString> m_mapTitles;
  std::map<int, CStdString> m_mapArtists;
  std::map<int, CStdString> m_mapExtended_track;

  std::map<int, CStdString> m_mapInexact_cddb_command_list;
  std::map<int, CStdString> m_mapInexact_artist_list;
  std::map<int, CStdString> m_mapInexact_title_list;


  CStdString m_strDisk_artist;
  CStdString m_strDisk_title;
  CStdString m_strYear;
  CStdString m_strGenre;

  void addTitle(const char *buffer);
  void addExtended(const char *buffer);
  void parseData(const char *buffer);
  bool Send( const void *buffer, int bytes );
  bool Send( const char *buffer);
  std::string Recv(bool wait4point);
  bool openSocket();
  bool closeSocket();
  struct toc cdtoc[100];
  int cddb_sum(int n);
  void addInexactList(const char *list);
  void addInexactListLine(int line_cnt, const char *line, int len);
  const CStdString& getInexactCommand(int select) const;
  CStdString GetCacheFile(unsigned int disc_id) const;
  /*! \brief Trim and convert some text to UTF8
   \param untrimmedText original text to trim and convert
   \return a utf8 version of the trimmed text
   */
  CStdString TrimToUTF8(const CStdString &untrimmed);

  CStdString m_cddb_ip_adress;
  CStdString cCacheDir;
};
}

#endif
