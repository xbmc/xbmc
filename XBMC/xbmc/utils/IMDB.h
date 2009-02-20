// IMDB.h: interface for the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
#define AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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

#include "Thread.h"
#include "ScraperParser.h"
#include "VideoInfoTag.h"
#include "ScraperSettings.h"
#include "FileSystem/FileCurl.h"

// forward definitions
class TiXmlDocument;
class CGUIDialogProgress;

typedef std::vector<CScraperUrl> IMDB_MOVIELIST;
typedef std::map<std::pair<int,int>,CScraperUrl> IMDB_EPISODELIST;

class CIMDB : public CThread
{
public:
  CIMDB();
  virtual ~CIMDB();

  bool LoadDLL();
  bool InternalFindMovie(const CStdString& strMovie, IMDB_MOVIELIST& movielist, const CStdString& strFunction="GetSearchResults", CScraperUrl* pUrl=NULL);
  bool InternalGetDetails(const CScraperUrl& url, CVideoInfoTag& movieDetails, const CStdString& strFunction="GetDetails");
  bool InternalGetEpisodeList(const CScraperUrl& url, IMDB_EPISODELIST& details);
  bool ParseDetails(TiXmlDocument &doc, CVideoInfoTag &movieDetails);
  bool LoadXML(const CStdString& strXMLFile, CVideoInfoTag &movieDetails, bool bDownload = true);
  bool Download(const CStdString &strURL, const CStdString &strFileName);
  void GetURL(const CStdString &movieFile, const CStdString &movieName, const CStdString &movieYear, CScraperUrl& strURL);

  // threaded lookup functions
  bool FindMovie(const CStdString& strMovie, IMDB_MOVIELIST& movielist, CGUIDialogProgress *pProgress = NULL);
  bool GetDetails(const CScraperUrl& url, CVideoInfoTag &movieDetails, CGUIDialogProgress *pProgress = NULL);
  bool GetEpisodeDetails(const CScraperUrl& url, CVideoInfoTag &movieDetails, CGUIDialogProgress *pProgress = NULL);
  bool GetEpisodeList(const CScraperUrl& url, IMDB_EPISODELIST& details, CGUIDialogProgress *pProgress = NULL);
  bool ScrapeFilename(const CStdString& strFileName, CVideoInfoTag& details);

  void SetScraperInfo(const SScraperInfo& info) { m_info.Reset(); m_info = info; }
  const SScraperInfo& GetScraperInfo() const { return m_info; }
protected:
  void RemoveAllAfter(char* szMovie, const char* szSearch);
  void GetCleanNameAndYear(CStdString &strMovieName, CStdString &strYear);

  static bool RelevanceSortFunction(const CScraperUrl& left, const CScraperUrl &right);

  XFILE::CFileCurl m_http;
  CScraperParser m_parser;

  // threaded stuff
  void Process();
  void CloseThread();

  enum LOOKUP_STATE { DO_NOTHING = 0,
                      FIND_MOVIE = 1,
                      GET_DETAILS = 2,
                      GET_EPISODE_LIST = 3,
                      GET_EPISODE_DETAILS = 4 };
  CStdString        m_strMovie;
  IMDB_MOVIELIST    m_movieList;
  CVideoInfoTag     m_movieDetails;
  CScraperUrl       m_url;
  IMDB_EPISODELIST  m_episode;
  LOOKUP_STATE      m_state;
  bool              m_found;
  bool              m_retry;
  SScraperInfo      m_info;
};

#endif // !defined(AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
