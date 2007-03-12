// IMDB1.h: interface for the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
#define AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "http.h"
#include "thread.h"
#include "ScraperParser.h"

// forward definitions
class TiXmlDocument;
class CGUIDialogProgress;

struct SScraperInfo
{
  CStdString strTitle;
  CStdString strPath;
  CStdString strThumb;
  CStdString strContent; // dupe, whatever
};

class CIMDBUrl
{
public:
  std::vector<CScraperUrl> m_scrURL;
  CStdString m_strID;  
  CStdString m_strTitle;
  bool Parse(CStdString);
};
typedef vector<CIMDBUrl> IMDB_MOVIELIST;
typedef std::map<std::pair<int,int>,CIMDBUrl> IMDB_EPISODELIST;

class CIMDBMovie
{
public:
  CIMDBMovie() { Reset(); };
  void Reset();
  bool Load(const TiXmlElement *node);
  bool Save(TiXmlNode *node, const CStdString &tag);

  CStdString m_strDirector;
  CStdString m_strWritingCredits;
  CStdString m_strGenre;
  CStdString m_strTagLine;
  CStdString m_strPlotOutline;
  CStdString m_strPlot;
  CScraperUrl m_strPictureURL;
  CStdString m_strTitle;
  CStdString m_strVotes;
  vector< pair<CStdString, CStdString> > m_cast;
  typedef vector< pair<CStdString, CStdString> >::const_iterator iCast;

  CStdString m_strSearchString;
  CStdString m_strRuntime;
  CStdString m_strFile;
  CStdString m_strPath;
  CStdString m_strIMDBNumber;
  CStdString m_strMPAARating;
  CStdString m_strFileNameAndPath;
  CStdString m_strOriginalTitle;
  CStdString m_strEpisodeGuide;
  CStdString m_strPremiered;
  CStdString m_strStatus;
  CStdString m_strProductionCode;
  CStdString m_strFirstAired;
  bool m_bWatched;
  int m_iTop250;
  int m_iYear;
  int m_iSeason;
  int m_iEpisode;
  float m_fRating;
};

class CIMDB : public CThread
{
public:
  CIMDB();
  CIMDB(const CStdString& strProxyServer, int iProxyPort);
  virtual ~CIMDB();

  bool LoadDLL();
  bool InternalFindMovie(const CStdString& strMovie, IMDB_MOVIELIST& movielist);
  bool InternalGetDetails(const CIMDBUrl& url, CIMDBMovie& movieDetails, const CStdString& strFunction="GetDetails");
  bool InternalGetEpisodeList(const CIMDBUrl& url, IMDB_EPISODELIST& details);
  bool ParseDetails(TiXmlDocument &doc, CIMDBMovie &movieDetails);
  bool LoadXML(const CStdString& strXMLFile, CIMDBMovie &movieDetails, bool bDownload = true);
  bool Download(const CStdString &strURL, const CStdString &strFileName);
  void GetURL(const CStdString& strMovie, CScraperUrl& strURL, CStdString& strYear);

  // threaded lookup functions
  bool FindMovie(const CStdString& strMovie, IMDB_MOVIELIST& movielist, CGUIDialogProgress *pProgress = NULL);
  bool GetDetails(const CIMDBUrl& url, CIMDBMovie &movieDetails, CGUIDialogProgress *pProgress = NULL);
  bool GetEpisodeDetails(const CIMDBUrl& url, CIMDBMovie &movieDetails, CGUIDialogProgress *pProgress = NULL);
  bool GetEpisodeList(const CIMDBUrl& url, IMDB_EPISODELIST& details, CGUIDialogProgress *pProgress = NULL);

  void SetScraperInfo(const SScraperInfo& info) { m_info = info; }
protected:
  bool Get(CScraperUrl& , string& );
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
  CIMDBMovie        m_movieDetails;
  CIMDBUrl          m_url;
  IMDB_EPISODELIST  m_episode;
  LOOKUP_STATE      m_state;
  bool              m_found;
  SScraperInfo      m_info;
};

#endif // !defined(AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
