// IMDB1.cpp: implementation of the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#include "IMDB.h"
#include "../util.h"
#include "log.h"
#include "HTMLUtil.h"
using namespace HTML;
#pragma warning (disable:4018)
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIMDB::CIMDB()
{
}
CIMDB::CIMDB(const CStdString& strProxyServer, int iProxyPort)
:m_http(strProxyServer,iProxyPort)
{

}

CIMDB::~CIMDB()
{

}

bool CIMDB::FindMovie(const CStdString &strMovie,IMDB_MOVIELIST& movielist)
{
	char szTitle[1024];
	char szURL[1024];
	CIMDBUrl url;
	movielist.clear();
	bool bSkip=false;
	int ipos=0;

	CStdString strURL,strHTML;
	GetURL(strMovie,strURL);
  CLog::Log("Retrieve:%s",strURL.c_str());
	if (!m_http.Get(strURL,strHTML))
	{
    CLog::Log("Unable to retrieve web page:%%s ",strURL.c_str());
		return false;
	}

	if (strHTML.size()==0) 
  {
    CLog::Log("empty document returned");
    return false;
  }

	char *szBuffer= new char [strHTML.size()+1];
	if (!szBuffer) return false;
	strcpy(szBuffer,strHTML.c_str());
	
	/*
	<b>Exact Matches</b> (11 matches, by popularity)
    </p>
    <p>
    <table>
    <tr><td valign="top" align="right">1.&#160;</td><td valign="top" width="100%"><a href="/title/tt0313443/">Out of Time (2003/I)</a></td></tr>
	///old way
	<H2><A NAME="mov">Movies</A></H2>
	<OL><LI><A HREF="/Title?0167261">Lord of the Rings: The Two Towers, The (2002)</A><BR>...aka <I>Two Towers, The (2002) (USA: short title)</I>
	</LI>
	*/

	char *pStartOfMovieList=strstr(szBuffer," Matches</b>");
	if (!pStartOfMovieList)
	{
		char* pMovieTitle	 = strstr(szBuffer,"\"title\">");
		char* pMovieDirector = strstr(szBuffer,"Directed");
		char* pMovieGenre	 = strstr(szBuffer,"Genre:");
		char* pMoviePlot	 = strstr(szBuffer,"Plot");

		// oh i say, it appears we've been redirected to the actual movie details...
		if (pMovieTitle && pMovieDirector && pMovieGenre && pMoviePlot)
		{
			pMovieTitle+=8;
			char *pEnd = strstr(pMovieTitle,"<");
			if (pEnd) *pEnd=0;

      OutputDebugString("Got movie:");
      OutputDebugString(pMovieTitle);
      OutputDebugString("url:" );
      OutputDebugString(strURL.c_str());
      OutputDebugString("\n" );
			url.m_strTitle = pMovieTitle;
			url.m_strURL   = strURL;
			movielist.push_back(url);

			delete [] szBuffer;
			return true;
		}

		OutputDebugString("Unable to locate start of movie list.\n");
		delete [] szBuffer;
		return false;
	}

	pStartOfMovieList+=strlen("<table>");
	char *pEndOfMovieList=strstr(pStartOfMovieList,"</table>");
	if (!pEndOfMovieList)
	{
		pEndOfMovieList=pStartOfMovieList+strlen(pStartOfMovieList);
		OutputDebugString("Unable to locate end of movie list.\n");
//		delete [] szBuffer;
	//	return false;
	}

	CHTMLUtil html;

	*pEndOfMovieList=0;
	while(1)
	{
		char* pAHREF=strstr(pStartOfMovieList,"<a href=");
		if (pAHREF)
		{
            //<a href="/title/tt0313443/">Out of Time (2003/I)</a>
    		//old way
			//<A HREF="/Title?0167261">Lord of the Rings: The Two Towers, The (2002)</A>
			char* pendAHREF=strstr(pStartOfMovieList,"</a>");
			if (pendAHREF)
			{
				*pendAHREF	=0;
				pAHREF+=strlen("<a href=.");
				// get url
				char *pURL=strstr(pAHREF,">");
				if (pURL)
				{
					pURL--;
					*pURL=0;
					pURL++;
					*pURL=0;
          char* pURLEnd=strstr(pURL+1,"<");
          strcpy(szURL, pAHREF);
					if (pURLEnd)  
          {
            *pURLEnd=0;
            strcpy(szTitle, pURL+1);
            *pURLEnd='<';
          }
          else
            strcpy(szTitle, pURL+1);
					html.ConvertHTMLToAnsi(szTitle, url.m_strTitle);
		
					sprintf(szURL,"http://us.imdb.com/%s", &pAHREF[1]);
					url.m_strURL=szURL;
					movielist.push_back(url);
				}
				pStartOfMovieList = pendAHREF+1;
				if (pStartOfMovieList >=pEndOfMovieList) break;
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	delete [] szBuffer;
	return true;
}


bool CIMDB::GetDetails(const CIMDBUrl& url, CIMDBMovie& movieDetails)
{
	CStdString strHTML;
	CStdString strURL = url.m_strURL;

	movieDetails.m_strTitle=url.m_strTitle;
	movieDetails.m_strDirector="Not available.";
	movieDetails.m_strWritingCredits="Not available.";
	movieDetails.m_strGenre="Not available.";
	movieDetails.m_strTagLine="Not available.";
	movieDetails.m_strPlotOutline="Not available.";
	movieDetails.m_strPlot="Not available.";
	movieDetails.m_strPictureURL="";
	movieDetails.m_iYear=0;
	movieDetails.m_fRating=0.0;
	movieDetails.m_strVotes="Not available.";
	movieDetails.m_strCast="Not available.";
	movieDetails.m_iTop250=0;

	if (!m_http.Get(strURL,strHTML))
		return false;
	if (strHTML.size()==0) 
		return false;
	char *szBuffer= new char[strHTML.size()+1];
	strcpy(szBuffer,strHTML.c_str());
	
  char szURL[1024];
  strcpy(szURL, strURL.c_str());
  if (CUtil::HasSlashAtEnd(strURL)) szURL[ strURL.size()-1 ]=0;
  int ipos=strlen(szURL)-1;
  while (szURL[ipos] != '/' && ipos>0) ipos--;

  movieDetails.m_strIMDBNumber=&szURL[ipos+1];
  CLog::Log("imdb number:%s\n", movieDetails.m_strIMDBNumber.c_str());
	char *pDirectedBy=strstr(szBuffer,"Directed by");
	char *pCredits=strstr(szBuffer,"Writing credits");
	char* pGenre=strstr(szBuffer,"Genre:");
	char* pTagLine=strstr(szBuffer,"Tagline:</b>");	
	char* pPlotOutline=strstr(szBuffer,"Plot Outline:</b>");	
	char* pPlotSummary=strstr(szBuffer,"Plot Summary:</b>");	
	char* pPlot=strstr(szBuffer,"<a href=\"plotsummary");
	char* pImage=strstr(szBuffer,"<img alt=\"cover\" align=\"left\" src=\"");
	char* pRating=strstr(szBuffer,"User Rating:</b>");
	char* pCast=strstr(szBuffer,"first billed only: </b></td></tr>");
	char* pCred=strstr(szBuffer,"redited cast:"); // Complete credited cast or Credited cast
	char* pTop=strstr(szBuffer, "top 250:");

	char *pYear=strstr(szBuffer,"/Sections/Years/");
	if (pYear)
	{
		char szYear[5];
		pYear+=strlen("/Sections/Years/");
		strncpy(szYear,pYear,4);
		szYear[4]=0;
		movieDetails.m_iYear=atoi(szYear);
	}

	if (pDirectedBy) 
		ParseAHREF(pDirectedBy, strURL, movieDetails.m_strDirector);

	if (pCredits) 
		ParseAHREF(pCredits, strURL, movieDetails.m_strWritingCredits);

	if (pGenre)  
		ParseGenres(pGenre, strURL, movieDetails.m_strGenre);

	if (pRating) // and votes
	{
		char *pStart = strstr(pRating, "<b>"); 
		if(pStart) 
		{
			char *pEnd = strstr(pStart, "/");
			*pEnd = 0;
			// set rating
			movieDetails.m_fRating = (float)atof(pStart+3);

			if(movieDetails.m_fRating != 0.0) {
				// now, votes
				pStart = strstr(pEnd+2, "(");
				if(pStart)
					pEnd = strstr(pStart, " votes)");

				if(pEnd) {
					*pEnd = 0;
					pStart++; // skip the parantese before votes
					movieDetails.m_strVotes = pStart; // save
				} else {
					movieDetails.m_strVotes = "0";
				}
			}
		}
	}

	if(pTop) // top rated movie :)
	{
		pTop += strlen("top 250:") + 2; // jump space and #
		char *pEnd = strstr(pTop, "</a>");
		*pEnd = 0;
		movieDetails.m_iTop250 = atoi(pTop);
	}

	if(!pCast) 
	{
		pCast=pCred;
	}

	CHTMLUtil html;
	if(pCast) 
	{
		char *pRealEnd = strstr(pCast, "&nbsp;");
		char *pStart = strstr(pCast, "<a href");
		char *pEnd = pCast;

		if(pRealEnd &&  pStart && pEnd)
		{
			 movieDetails.m_strCast = "Cast overview:\n";

			 while(pRealEnd > pStart) 
			 {
				CStdString url = "";
				CStdString actor = "";
				CStdString role = "";
		
				// actor
					
				pEnd = strstr(pStart, "</a>");

				pEnd += 4;
				*pEnd = 0;

				pEnd += 1;

				ParseAHREF(pStart, url, actor);


				// role

				pStart = strstr(pEnd, "<td valign=\"top\">");
				pStart += strlen("<td valign=\"top\">");
			
				pEnd = strstr(pStart, "</td>");
				*pEnd = 0;
				
				html.ConvertHTMLToAnsi(pStart, role);

				pEnd += 1;

				// add to cast
				movieDetails.m_strCast += actor;
				if(!role.empty()) // Role not always listed
					movieDetails.m_strCast += " as " + role;
				
				movieDetails.m_strCast += "\n";
				
				// next actor
				pStart = strstr(pEnd, "<a href");
			}
		}
			
	}

	if (pTagLine)
	{
		pTagLine += strlen("Tagline:</b>");
		char *pEnd = strstr(pTagLine,"<");
		if (pEnd) *pEnd=0;
		html.ConvertHTMLToAnsi(pTagLine,movieDetails.m_strTagLine);
	}

	if (!pPlotOutline)
	{
		if (pPlotSummary)
		{
			pPlotSummary += strlen("Plot Summary:</b>");
			char *pEnd = strstr(pPlotSummary,"<");
			if (pEnd) *pEnd=0;
			html.ConvertHTMLToAnsi(pPlotSummary,movieDetails.m_strPlotOutline);			
		}
	}
	else
	{
		pPlotOutline += strlen("Plot Outline:</b>");
		char *pEnd = strstr(pPlotOutline,"<");
		if (pEnd) *pEnd=0;
		html.ConvertHTMLToAnsi(pPlotOutline,movieDetails.m_strPlotOutline);
		movieDetails.m_strPlot=movieDetails.m_strPlotOutline;
	}

	if (pImage)
	{
		pImage += strlen("<img alt=\"cover\" align=\"left\" src=\"");
		char *pEnd = strstr(pImage,"\"");
		if (pEnd) *pEnd=0;
		movieDetails.m_strPictureURL=pImage;
	}

	if (pPlot)
	{
		CStdString strPlotURL= url.m_strURL + "plotsummary";
		CStdString strPlotHTML;
		if ( m_http.Get(strPlotURL,strPlotHTML))
		{
			if (0!=strPlotHTML.size())
			{
				char* szPlotHTML = new char[1+strPlotHTML.size()];
				strcpy(szPlotHTML ,strPlotHTML.c_str());
				char *strPlotStart=strstr(szPlotHTML,"<p class=\"plotpar\">");
				if (strPlotStart)
				{
					strPlotStart += strlen("<p class=\"plotpar\">");
					char *strPlotEnd=strstr(strPlotStart,"</p>");
					if (strPlotEnd) *strPlotEnd=0;
					html.ConvertHTMLToAnsi(strPlotStart, movieDetails.m_strPlot);
				}
				delete [] szPlotHTML;
			}
		}
	}
	delete [] szBuffer;

	return true;
}

void CIMDB::ParseAHREF(const char *ahref, CStdString &strURL, CStdString &strTitle)
{
	char* szAHRef;
	szAHRef=new char[strlen(ahref)+1];
	strncpy(szAHRef,ahref,strlen(ahref));
	szAHRef[strlen(ahref)]=0;
	strURL="";
	strTitle="";
	char *pStart=strstr(szAHRef,"<a href=\"");
	if (!pStart) pStart=strstr(szAHRef,"<A HREF=\"");
	if (!pStart) 
	{
		delete [] szAHRef;
		return;
	}
	char* pEnd = strstr(szAHRef,"</a>"); 
	if (!pEnd) pEnd = strstr(szAHRef,"</A>"); 

	if (!pEnd)
	{
		delete [] szAHRef;
		return;
	}
	*pEnd=0;
	pEnd+=2;
	pStart += strlen("<a href=\"");
	
	char *pSep=strstr(pStart,">");
	if (pSep) *pSep=0;
	strURL=pStart;
	pSep++;

	CHTMLUtil html;
	//strTitle=pSep;
	html.ConvertHTMLToAnsi(pSep,strTitle);
	
	delete [] szAHRef;
}

void CIMDB::ParseGenres(const char *ahref, CStdString &strURL, CStdString &strTitle)
{
	char* szAHRef;
	szAHRef=new char[strlen(ahref)+1];
	strncpy(szAHRef,ahref,strlen(ahref));
	szAHRef[strlen(ahref)]=0;

	CStdString strGenre = "";

	char *pStart;
	char *pEnd=szAHRef;
	
	char *pSlash=strstr(szAHRef," / ");

	strTitle = "";

	if(pSlash) 
	{
		char *pRealEnd = strstr(szAHRef, "(more)");
		if(!pRealEnd) pRealEnd=strstr(szAHRef, "<br><br>");
    if (!pRealEnd)
    {
      OutputDebugString("1");
    }
		while(pSlash<pRealEnd)
		{
			pStart = pEnd+2;
			pEnd = pSlash;
			*pEnd = 0; // terminate CStdString after current genre
      int iLen=pEnd-pStart;
      if (iLen < 0) break;
			char *szTemp = new char[iLen+1];
			strncpy(szTemp, pStart, iLen);
			szTemp[iLen] = 0;
	
			ParseAHREF(szTemp, strURL, strGenre);
      delete [] szTemp;
			strTitle = strTitle + strGenre + " / ";
			pSlash=strstr(pEnd+2," / ");
			if(!pSlash) pSlash = pRealEnd;
		}
	}
	// last genre
	pEnd+=2;
	ParseAHREF(pEnd, strURL, strGenre);
	strTitle = strTitle + strGenre;

	delete [] szAHRef;
}

bool CIMDB::Download(const CStdString &strURL, const CStdString &strFileName)
{
	CStdString strHTML;
	if (!m_http.Download(strURL,strFileName)) 
  {
    CLog::Log("failed to download %s -> %s", strURL.c_str(), strFileName.c_str());
    return false;
  }

	return true;
}

void CIMDBMovie::Reset()
{
  m_strDirector="";
	m_strWritingCredits="";
	m_strGenre="";
	m_strTagLine="";
	m_strPlotOutline="";
	m_strPlot="";
	m_strPictureURL="";
	m_strTitle="";
	m_strVotes="";
	m_strCast="";
	m_strSearchString="";
  m_strFile="";
  m_strPath="";
  m_strDVDLabel="";
  m_strIMDBNumber="";
	m_iTop250=0;
	m_iYear=0;
	m_fRating=0.0f;
}
void CIMDBMovie::Save(const CStdString &strFileName)
{
}

bool CIMDBMovie::Load(const CStdString& strFileName)
{

	return true;
}


void CIMDB::RemoveAllAfter(char* szMovie,const char* szSearch)
{
  char* pPtr=strstr(szMovie,szSearch);
  if (pPtr) *pPtr=0;
}

void CIMDB::GetURL(const CStdString &strMovie, CStdString& strURL)
{
	char szURL[1024];
	char szMovie[1024];
	CIMDBUrl url;
	bool bSkip=false;
	int ipos=0;

	// skip extension
	int imax=0;
	for (int i=0; i < strMovie.size();i++)
	{
		if (strMovie[i]=='.') imax=i;
		if (i+2 < strMovie.size())
		{
			// skip -CDx. also
			if (strMovie[i]=='-')
			{
				if (strMovie[i+1]=='C' || strMovie[i+1]=='c')
				{
					if (strMovie[i+2]=='D' || strMovie[i+2]=='d')
					{
						imax=i;
						break;
					}
				}
			}
		}
	}
	if (!imax) imax=strMovie.size();
	for (int i=0; i < imax;i++)
	{
		for (int c=0;isdigit(strMovie[i+c]);c++)
		{
			if (c==3)
			{
				i+=4;
				break;
			}
		}
    	char kar=strMovie[i];
		if (kar =='.') kar=' ';
    	if (kar ==32) kar = '+';
		if (kar == '[' || kar=='(' ) bSkip=true;			//skip everthing between () and []
		else if (kar == ']' || kar==')' ) bSkip=false;
		else if (!bSkip)
		{
			if (ipos > 0)
			{
				if (!isalnum(kar)) 
				{
					if (szMovie[ipos-1] != '+')
					kar='+';
					else 
					kar='.';
				}
			}
			if (isalnum(kar) ||kar==' ' || kar=='+')
			{
				szMovie[ipos]=kar;
				szMovie[ipos+1]=0;
				ipos++;
			}
		}
	
	}

	CStdString strTmp=szMovie;
	strTmp.ToLower();
	strTmp.Trim();
	strcpy(szMovie,strTmp.c_str());

	RemoveAllAfter(szMovie," divx ");
	RemoveAllAfter(szMovie," xvid ");
	RemoveAllAfter(szMovie," dvd ");
	RemoveAllAfter(szMovie," svcd ");
	RemoveAllAfter(szMovie," ac3 ");
	RemoveAllAfter(szMovie," ogg ");
	RemoveAllAfter(szMovie," ogm ");
	RemoveAllAfter(szMovie," internal ");
	RemoveAllAfter(szMovie," fragment ");
	RemoveAllAfter(szMovie," dvdrip ");
	RemoveAllAfter(szMovie," proper ");
	RemoveAllAfter(szMovie," limited ");
	RemoveAllAfter(szMovie," rerip ");
  
	CStdString strHTML;
	sprintf(szURL,"http://us.imdb.com/Tsearch?title=%s", szMovie);
	strURL = szURL;
}
