
#include "../stdafx.h"

#include ".\musicinfoscraper.h"
#include ".\http.h"
#include ".\htmlutil.h"
#include ".\htmltable.h"
#include "../util.h"
using namespace HTML;

CMusicInfoScraper::CMusicInfoScraper(void)
{
}

CMusicInfoScraper::~CMusicInfoScraper(void)
{
}


int	 CMusicInfoScraper::GetAlbumCount() const
{
	return (int)m_vecAlbums.size();
}

 CMusicAlbumInfo& CMusicInfoScraper::GetAlbum(int iAlbum) 
{
	return m_vecAlbums[iAlbum];
}

bool CMusicInfoScraper::FindAlbuminfo(const CStdString& strAlbum)
{
	CStdString strHTML;
	m_vecAlbums.erase(m_vecAlbums.begin(),m_vecAlbums.end());
	// make request
	// type is 
	// http://www.allmusic.com/cg/amg.dll?P=amg&sql=escapolygy&opt1=2&Image1.x=18&Image1.y=14

	CHTTP http;
	CStdString strPostData;
	strPostData.Format("P=amg&sql=%s&opt1=2&Image1.x=18&Image1.y=14", strAlbum.c_str() );

	// get the HTML
	if (! http.Post("http://www.allmusic.com/cg/amg.dll",strPostData,strHTML) )
	{
		return false;
	}
	
	// check if this is an album
	{
		CStdString strURL="http://www.allmusic.com/cg/amg.dll?";
		CUtil::URLEncode(strPostData);
		strURL+=strPostData;
		CMusicAlbumInfo newAlbum("",strURL);
		if ( newAlbum.Parse(strHTML) )
		{
			m_vecAlbums.push_back(newAlbum);
			return true;
		}
	}
	// check if we found a list of albums

	CHTMLTable table;
	CHTMLUtil  util;
	table.Parse(strHTML);
	for (int i=0; i < table.GetRows(); ++i)
	{
		const CHTMLRow& row=table.GetRow(i);
		for (int iCol=0; iCol < row.GetColumns(); ++iCol)
		{
			CStdString strColum=row.GetColumValue(iCol);
			if (strColum.Find("<a class=a") >= 0)
			{
				CStdString strAlbumName;
				CStdString strAlbumURL;
				int iPos=strColum.ReverseFind("</a>");
				if (iPos >= 0)
				{
					iPos+=4;
					strAlbumName=strColum.Right((int)strColum.size()-iPos);
					int iOpen=strAlbumName.Find("<");
					if (iOpen>=0)
					{
						strAlbumName=strAlbumName.Left(iOpen);
					}
					int iPosCookieStart=strColum.Find("('");
					int iPosCookieEnd=strColum.Find("')",iPosCookieStart);
					if (iPosCookieStart>=0 && iPosCookieEnd>=0)
					{
						iPosCookieStart+=2;
						CStdString strCookie=strColum.Mid(iPosCookieStart,(int)iPosCookieEnd-iPosCookieStart);
						// full album url:
						// http://www.allmusic.com/cg/amg.dll?p=amg&uid=MISS70308120341&sql=Ayduj6j7371r0

						strAlbumURL.Format("http://www.allmusic.com/cg/amg.dll?p=amg&uid=MISS70308120341&sql=%s", strCookie.c_str());
						CMusicAlbumInfo newAlbum(strAlbumName,strAlbumURL);
						m_vecAlbums.push_back(newAlbum);
					}
				}
			}
		}
	}
	
	return true;
}

