// IMDB1.h: interface for the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
#define AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_

#include "HTTP.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "http.h"
#include <vector>
#include <string>
using namespace std;


class CIMDBUrl
{
	public:
		string m_strURL;
		string m_strTitle;
};
typedef vector<CIMDBUrl> IMDB_MOVIELIST;

class CIMDBMovie
{
	public:
		bool 				Load(const string& strFileName);
		void 				Save(const string& strFileName);
		string m_strDirector;
		string m_strWritingCredits;
		string m_strGenre;
		string m_strTagLine;
		string m_strPlotOutline;
		string m_strPlot;
		string m_strPictureURL;
		string m_strTitle;
		string m_strVotes;
		string m_strCast;
		string m_strSearchString;
		int				 m_iTop250;
		int    		 m_iYear;
		float  		 m_fRating;
private:
};

class CIMDB  
{
public:
	CIMDB();
	CIMDB(const string& strProxyServer, int iProxyPort);
	virtual ~CIMDB();

	bool FindMovie(const string& strMovie,	IMDB_MOVIELIST& movielist);
	bool GetDetails(const CIMDBUrl& url, CIMDBMovie& movieDetails);
	bool Download(const string &strURL, const string &strFileName);
	void GetURL(const string& strMovie,string& strURL);
protected:
	void ParseAHREF(const char* ahref, string& strURL, string& strTitle);
	void ParseGenres(const char* ahref, string& strURL, string& strTitle);
	void RemoveAllAfter(char* szMovie,const char* szSearch);
  CHTTP m_http;
};

#endif // !defined(AFX_IMDB1_H__562A722A_CD2A_4B4A_8A67_32DE8088A7D3__INCLUDED_)
