// MediaMonitor.cpp: implementation of the CMediaMonitor class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "MediaMonitor.h"
#include "graphicContext.h"
#include "../Picture.h"
#include "../Util.h"
#include "../Settings.h"
#include "../FileSystem/VirtualDirectory.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool CMediaMonitor::SortMoviesByDateAndTime(Movie aMovie1, Movie aMovie2)
{
	if (aMovie1.dwDate != aMovie2.dwDate)
	{
		return (aMovie1.dwDate < aMovie2.dwDate );
	}

	return (aMovie1.wTime < aMovie2.wTime );
}

CMediaMonitor::CMediaMonitor() : CThread()
{
	m_pObserver = NULL;
}

CMediaMonitor::~CMediaMonitor()
{

}

void CMediaMonitor::Create(IMediaObserver* aObserver)
{
	m_pObserver	= aObserver;

	CThread::Create(false);
}

void CMediaMonitor::Process() 
{
	VECSHARES& vecShares = g_settings.m_vecMyVideoShares;

	DIRECTORY::CVirtualDirectory directory;
	directory.SetShares(g_settings.m_vecMyVideoShares);
	directory.SetMask(g_stSettings.m_szMyVideoExtensions);

	CStdString path;
	VECFILEITEMS items;
	if (directory.GetDirectory(path,items))
	{
		for (int i=0; i < (int)items.size(); ++i)
		{
			CFileItem* pItem = items.at(i);
			if (pItem->m_iDriveType == SHARE_TYPE_REMOTE)
			{
				Scan(directory, pItem->m_strPath);
			}

			delete pItem;
		}
	}

	// sort files by creation date
	sort(m_movies.begin(), m_movies.end(), CMediaMonitor::SortMoviesByDateAndTime );

	// select the 3 newest files (remove similar filenames from consideration):
	int iNewestMovie = max(0, m_movies.size()-1);
	int i = iNewestMovie;

	int selections[3];
	int sel = 0;
    
	while ((i>0) && (sel<3))
	{
		Movie aMovie = m_movies.at(i);

		if (sel>0)
		{
			// ensure at least less than 75% similar compared to the previous selection
			if (!parse_Similar(	aMovie.strFilepath,
								m_movies.at(selections[sel-1]).strFilepath, 75))
			{
				selections[sel++] = i;
			}
		}
		else
		{
			selections[sel++] = i;
		}

		i--;
	}

	m_database.Open();

	// we have selected up to 3 new files
	for(i=0;i<sel;i++)
	{
		CStdString strImagePath;
		CIMDBMovie details;  
		Movie aMovie = m_movies.at(selections[i]);

		if (GetMovieInfo(aMovie.strFilepath, details))
		{
 			if (!details.m_strPictureURL.IsEmpty())
			{
				GetMovieArt(details.m_strIMDBNumber,details.m_strPictureURL,strImagePath);
			}
		}
		else
		{
			details.m_strTitle = CUtil::GetFileName(aMovie.strFilepath);
		}

		if (m_pObserver)
		{
			g_graphicsContext.Lock();
			m_pObserver->OnMediaUpdate(i,aMovie.strFilepath, details.m_strTitle, strImagePath);
			g_graphicsContext.Unlock();
		}
	}

	m_database.Close();
	m_movies.clear();
}


bool CMediaMonitor::GetMovieInfo(CStdString& strFilepath, CIMDBMovie& aMovieRecord)
{
	bool bInfo = false;

	if (m_database.HasMovieInfo(strFilepath))
	{
		m_database.GetMovieInfo(strFilepath, aMovieRecord);
		bInfo = true;
	}
	else
	{
		CIMDB imdb;
		IMDB_MOVIELIST results;
		CStdString strName = CUtil::GetFileName(strFilepath);

		// Clean up filename in an attempt to get an idea of the movie name
		parse_Clean(strName);

		if (imdb.FindMovie(strName, results) ) 
		{
			if (results.size()>0)
			{
				if (! imdb.GetDetails(results[0], aMovieRecord) )
				{
					aMovieRecord.m_strTitle = results[0].m_strTitle;
				}

				CStdString strCDLabel;
				if (m_database.AddMovie(strFilepath, strCDLabel, false))
				{
					m_database.SetMovieInfo(strFilepath, aMovieRecord);
				}

				bInfo = true;
			}
		}
	}

	return bInfo;
}

bool CMediaMonitor::GetMovieArt(CStdString& strIMDBNumber, CStdString& strPictureUrl, CStdString& strImagePath)
{
	CStdString strThum;
	CUtil::GetVideoThumbnail(strIMDBNumber,strThum);

	if (CUtil::FileExists(strThum.c_str()))
	{
		strImagePath = strThum;
		return true;
	}

	CStdString strExtension;
	CUtil::GetExtension(strPictureUrl,strExtension);

	CStdString strTemp;
	strTemp.Format("Z:\\ram_temp%s",strExtension.c_str());
    ::DeleteFile(strTemp.c_str());

	CHTTP http;
    http.Download(strPictureUrl, strTemp);

	try
	{
		CPicture picture;
		picture.Convert(strTemp,strThum);
	}
	catch(...)
	{
		::DeleteFile(strThum.c_str());
	}

	::DeleteFile(strTemp.c_str());

	if (CUtil::FileExists(strThum.c_str()))
	{
		strImagePath = strThum;
		return true;
	}

	return false;
}

void CMediaMonitor::Scan(DIRECTORY::CDirectory& directory, CStdString& aPath)
{
	VECFILEITEMS items;
	if (!directory.GetDirectory(aPath,items))
	{
		return;
	}

	for (int i=0; i < (int)items.size(); ++i)
	{
		CFileItem* pItem = items.at(i);
		
		if (pItem->m_bIsFolder)
		{
			Scan(directory, pItem->m_strPath);
		}
		else
		{
			char* szFilename = (char*) pItem->GetLabel().c_str();
			char* szExtension = strrchr(szFilename,'.');
			char* szExpected = szFilename+(strlen(szFilename)-4);

			// ensure file has a 3 letter extension that isn't .nfo
			if ((szExtension!=NULL) && (szExpected==szExtension) && (strcmp(szExtension,".nfo")!=0) )
			{
				Movie aMovie;
				aMovie.strFilepath	= pItem->m_strPath;
				aMovie.dwDate		= (( (pItem->m_stTime.wYear)			<< 16 ) |
									( (pItem->m_stTime.wMonth & 0x00FF)	<< 8 )  |
									(pItem->m_stTime.wDay & 0x00FF) );
				aMovie.wTime		= (( (pItem->m_stTime.wHour & 0x00FF)	<< 8 )  |
									(pItem->m_stTime.wMinute & 0x00FF) );

				m_movies.push_back(aMovie);

				//OutputDebugString(pItem->GetLabel().c_str());
				//OutputDebugString("\n");
			}
		}

		delete pItem;
	}
}

///////////////////////////////////////////////////////////////////////////
// Filename parsing functions

const char seps[]   = ",._-";
const char* keywords[]   = {"xvid","divx","hdtv","pdtv","dvdrip","dvd","ts","scr",0};

int CMediaMonitor::parse_GetStart(char* szText)
{
	char szCopy[1024];
	strcpy(szCopy,szText);

	unsigned short nLineLen = strlen(szCopy);
	unsigned short nHalfLine= nLineLen >> 1;

	char* token = strtok( szCopy, seps );
	char* find = NULL;

	while (token != NULL )
	{
		if ((token-szCopy)>nHalfLine)
		{
			token = NULL;
			break;
		}	

		for(int i=0;keywords[i]!=NULL;i++)
		{
			if (strcmp(keywords[i],token)==0)
			{
				find = token + strlen(token) +1;
			}
		}


		token = strtok( NULL, seps );
	}

	int start = 0;
	if (find!=NULL)
	{
		start = (int) (find - szCopy);
	}

	return start;
}


int CMediaMonitor::parse_GetLength(char* szText, int start)
{
	char szCopy[1024];
	strcpy(szCopy,&szText[start]);

	char* szExtension = strrchr(szCopy,'.');
	
	if (szExtension==NULL)
	{
		szExtension=(szCopy+strlen(szCopy));
	}

	char* token = strtok( szCopy, seps );
	bool exit = false;

	while ((token != NULL ) && (!exit) && (token<szExtension))
	{
		unsigned short nWordLen = strlen(token);
		unsigned short nHalfWord= nWordLen >> 1;
		unsigned short nCapital = 0;
		
		for(int i=0;i<nWordLen;i++)
		{
			if ((token[i]>='A') && (token[i]<='Z'))
			{
				nCapital++;
			}
		}

		if (nCapital>nHalfWord)
		{
			break;
		}

		for(int j=0;keywords[j]!=NULL;j++)
		{
			if (strcmp(token,keywords[j])==0)
			{
				exit = true;
				break;
			}
		}

		if (exit)
		{
			break;
		}
		else
		{
			token = strtok( NULL, seps );
		}
	}

	int delimiter = strlen(szText);
	if (token!=NULL)
		delimiter = (int)(token - szCopy);

	return delimiter;
}

void CMediaMonitor::parse_Clean(CStdString& strFilename)
{
	char* szBuffer	 = (char*) strFilename.c_str();
	int start		 = parse_GetStart ( szBuffer );
	int length		 = parse_GetLength( szBuffer, start);

	char szCopy[1024];

	if (length<0)
	{
		length = strlen(szBuffer);
	}

	char lc = 0;
	for(int i=start,i2=0; ((i<start+length) && (szBuffer[i]!=0)); i++)
	{
		char c = szBuffer[i];

		switch (c)
		{
			case '\r':
			case '\n':
			case ',':
			case '.':
			case '_':
			case '-':
			case ' ':
				c = ' ';
				if (c==lc)
				{
					continue;
				}
				break;
		}

		szCopy[i2++]=c;
		lc = c;
	}

	szCopy[i2]=0;
	CStdString name = szCopy;
	strFilename = name.Trim();
}

bool CMediaMonitor::parse_Similar(CStdString& strFilepath1, CStdString& strFilepath2, int aPercentage)
{
	char* szText1 = CUtil::GetFileName(strFilepath1);
	char* szText2 = CUtil::GetFileName(strFilepath2);

	for(int i=0, identical=0; ((szText1[i]!=0) && (szText2[i]!=0)); i++)
		if (szText1[i]==szText2[i])
			identical++;

	int len1 = strlen(szText1);
	int len2 = strlen(szText2);

	int similarity = (identical*100) / ((len1<=len2) ? len1 : len2);

	return (similarity>=aPercentage);
}
