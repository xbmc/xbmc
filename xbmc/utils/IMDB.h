// IMDB1.h: interface for the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
#define AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HTTP.h"
#include "Thread.h"
#include "ScraperParser.h"
#include "../VideoInfoTag.h"

// forward definitions
class TiXmlDocument;
class CGUIDialogProgress;

typedef vector<CScraperUrl> IMDB_MOVIELIST;
typedef std::map<std::pair<int,int>,CScraperUrl> IMDB_EPISODELIST;

class CIMDB : public CThread
{
public:
  CIMDB();
  CIMDB(const CStdString& strProxyServer, int iProxyPort);
  virtual ~CIMDB();

  bool LoadDLL();
  bool InternalFindMovie(const CStdString& strMovie, IMDB_MOVIELIST& movielist);
  bool InternalGetDetails(const CScraperUrl& url, CVideoInfoTag& movieDetails, const CStdString& strFunction="GetDetails");
  bool InternalGetEpisodeList(const CScraperUrl& url, IMDB_EPISODELIST& details);
  bool ParseDetails(TiXmlDocument &doc, CVideoInfoTag &movieDetails);
  bool LoadXML(const CStdString& strXMLFile, CVideoInfoTag &movieDetails, bool bDownload = true);
  bool Download(const CStdString &strURL, const CStdString &strFileName);
  void GetURL(const CStdString& strMovie, CScraperUrl& strURL, CStdString& strYear);

  // threaded lookup functions
  bool FindMovie(const CStdString& strMovie, IMDB_MOVIELIST& movielist, CGUIDialogProgress *pProgress = NULL);
  bool GetDetails(const CScraperUrl& url, CVideoInfoTag &movieDetails, CGUIDialogProgress *pProgress = NULL);
  bool GetEpisodeDetails(const CScraperUrl& url, CVideoInfoTag &movieDetails, CGUIDialogProgress *pProgress = NULL);
  bool GetEpisodeList(const CScraperUrl& url, IMDB_EPISODELIST& details, CGUIDialogProgress *pProgress = NULL);
  bool ScrapeFilename(const CStdString& strFileName, CVideoInfoTag& details);

  void SetScraperInfo(const SScraperInfo& info) { m_info = info; }
protected:
  void RemoveAllAfter(char* szMovie, const char* szSearch);
  CHTTP m_http;

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
  SScraperInfo      m_info;
};

#endif // !defined(AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
