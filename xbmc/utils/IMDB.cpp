// IMDB1.cpp: implementation of the CIMDB class.
//
//////////////////////////////////////////////////////////////////////

#include "IMDB.h"
#include "../util.h"
#include "HTMLUtil.h"
using namespace HTML;
#pragma warning (disable:4018)
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIMDB::CIMDB()
{
}
CIMDB::CIMDB(const string& strProxyServer, int iProxyPort)
:m_http(strProxyServer,iProxyPort)
{

}

CIMDB::~CIMDB()
{

}

bool CIMDB::FindMovie(const string &strMovie,IMDB_MOVIELIST& movielist)
{
	char szTitle[1024];
	char szURL[1024];
	CIMDBUrl url;
	movielist.clear();
	bool bSkip=false;
	int ipos=0;

	string strURL,strHTML;
	GetURL(strMovie,strURL);
  OutputDebugString("Retrieve:");
  OutputDebugString(strURL.c_str());
  OutputDebugString("\n");
	if (!m_http.Get(strURL,strHTML))
	{
		OutputDebugString("Unable to retrieve web page: ");
		OutputDebugString(strURL.c_str());
		OutputDebugString("\n");

		return false;
	}

	if (strHTML.size()==0) return false;

	char *szBuffer= new char[strHTML.size()+1];
	if (!szBuffer) return false;
	strcpy(szBuffer,strHTML.c_str());
	
	/*
	<H2><A NAME="mov">Movies</A></H2>
	<OL><LI><A HREF="/Title?0167261">Lord of the Rings: The Two Towers, The (2002)</A><BR>...aka <I>Two Towers, The (2002) (USA: short title)</I>
	</LI>
	*/

	char *pStartOfMovieList=strstr(szBuffer,"<H2><A NAME=\"mov\">Movies</A></H2>");
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

	pStartOfMovieList+=strlen("<H2><A NAME=\"mov\">Movies</A></H2>");
	char *pEndOfMovieList=strstr(pStartOfMovieList,"</OL>");
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
		char* pAHREF=strstr(pStartOfMovieList,"<A HREF=");
		if (pAHREF)
		{
			//<A HREF="/Title?0167261">Lord of the Rings: The Two Towers, The (2002)</A>
			char* pendAHREF=strstr(pStartOfMovieList,"</A>");
			if (pendAHREF)
			{
				*pendAHREF	=0;
				pAHREF+=strlen("<A HREF=.");
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
	string strHTML;
	string strURL = url.m_strURL;

	movieDetails.m_strTitle=url.m_strTitle;
	movieDetails.m_strDirector=" ";
	movieDetails.m_strWritingCredits=" ";
	movieDetails.m_strGenre=" ";
	movieDetails.m_strTagLine=" ";
	movieDetails.m_strPlotOutline=" ";
	movieDetails.m_strPlot=" ";
	movieDetails.m_strPictureURL="";
	movieDetails.m_iYear=0;
	movieDetails.m_fRating=0.0;
	movieDetails.m_strVotes="";
	movieDetails.m_strCast="";
	movieDetails.m_iTop250=0;

	if (!m_http.Get(strURL,strHTML))
		return false;
	if (strHTML.size()==0) 
		return false;
	char *szBuffer= new char[strHTML.size()+1];
	strcpy(szBuffer,strHTML.c_str());
	//printf ("%s", szBuffer);

	char *pDirectedBy=strstr(szBuffer,"Directed by");
	char *pCredits=strstr(szBuffer,"Writing credits");
	char* pGenre=strstr(szBuffer,"Genre:");
	char* pTagLine=strstr(szBuffer,"Tagline:</b>");	
	char* pPlotOutline=strstr(szBuffer,"Plot Outline:</b>");	
	char* pPlotSummary=strstr(szBuffer,"Plot Summary:</b>");	
	char* pPlot=strstr(szBuffer,"<a href=\"/Plot?");	
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
				string url = "";
				string actor = "";
				string role = "";
		
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
		pPlot+=strlen("<a href=\"");	
		char *pEnd=strstr(pPlot,"\">");
		if (pEnd) *pEnd=0;
		string strPlotURL="http://us.imdb.com";
		strPlotURL+=pPlot;
		string strPlotHTML;
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

void CIMDB::ParseAHREF(const char *ahref, string &strURL, string &strTitle)
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

void CIMDB::ParseGenres(const char *ahref, string &strURL, string &strTitle)
{
	char* szAHRef;
	szAHRef=new char[strlen(ahref)+1];
	strncpy(szAHRef,ahref,strlen(ahref));
	szAHRef[strlen(ahref)]=0;

	string strGenre = "";

	char *pStart;
	char *pEnd=szAHRef;
	
	char *pSlash=strstr(szAHRef," / ");

	strTitle = "";

	if(pSlash) 
	{
		char *pRealEnd = strstr(szAHRef, "(more)");
		if(!pRealEnd) strstr(szAHRef, "<br><br>");
		while(pSlash<pRealEnd)
		{
			pStart = pEnd+2;
			pEnd = pSlash;
			*pEnd = 0; // terminate string after current genre
			char *szTemp = new char[pEnd - pStart];
			strncpy(szTemp, pStart, (pEnd - pStart));
			szTemp[pEnd - pStart] = 0;
	
			ParseAHREF(szTemp, strURL, strGenre);
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

bool CIMDB::Download(const string &strURL, const string &strFileName)
{
	string strHTML;
	if (!m_http.Download(strURL,strFileName)) return false;

	return true;
}

void CIMDBMovie::Save(const string &strFileName)
{
	FILE* fd=fopen(strFileName.c_str(), "wb+");
	if (!fd) return;
	CUtil::SaveString(m_strDirector,fd);
	CUtil::SaveString(m_strGenre,fd);
	CUtil::SaveString(m_strPictureURL,fd);
	CUtil::SaveString(m_strPlot,fd);
	CUtil::SaveString(m_strPlotOutline,fd);
	CUtil::SaveString(m_strTagLine,fd);
	CUtil::SaveString(m_strWritingCredits,fd);
	CUtil::SaveString(m_strTitle,fd);
	CUtil::SaveString(m_strVotes,fd);
	CUtil::SaveString(m_strCast,fd);

	fwrite(&m_iYear,1,sizeof(int),fd);
	fwrite(&m_fRating,1,sizeof(float),fd);
	fwrite(&m_iTop250,1,sizeof(int),fd);
	
	fclose(fd);
}

bool CIMDBMovie::Load(const string& strFileName)
{

	FILE* fd=fopen(strFileName.c_str(), "rb+");
	if (!fd) return false;
	CUtil::LoadString(m_strDirector,fd);
	CUtil::LoadString(m_strGenre,fd);
	CUtil::LoadString(m_strPictureURL,fd);
	CUtil::LoadString(m_strPlot,fd);
	CUtil::LoadString(m_strPlotOutline,fd);
	CUtil::LoadString(m_strTagLine,fd);
	CUtil::LoadString(m_strWritingCredits,fd);
	CUtil::LoadString(m_strTitle,fd);
	CUtil::LoadString(m_strVotes,fd);
	CUtil::LoadString(m_strCast,fd);
		
	fread(&m_iYear,1,sizeof(int),fd);
	fread(&m_fRating,1,sizeof(float),fd);
	fread(&m_iTop250,1,sizeof(int),fd);
	fclose(fd);

	return true;
}


void CIMDB::GetURL(const string &strMovie, string& strURL)
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
	string strHTML;
	sprintf(szURL,"http://us.imdb.com/Tsearch?title=%s", szMovie);
	strURL = szURL;
}
