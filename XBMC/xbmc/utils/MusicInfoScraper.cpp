
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
	// http://www.allmusic.com/cg/amg.dll?P=amg&SQL=escapolygy&OPT1=2

	CHTTP http;
	CStdString strPostData;
	strPostData.Format("P=amg&SQL=%s&OPT1=2", strAlbum.c_str());

	// get the HTML
	if (!http.Post("http://www.allmusic.com/cg/amg.dll",strPostData,strHTML))
		return false;
	
	// check if this is an album
	CStdString strURL="http://www.allmusic.com/cg/amg.dll?";
	CUtil::URLEncode(strPostData);
	strURL+=strPostData;
	CMusicAlbumInfo newAlbum("",strURL);
	if ( newAlbum.Parse(strHTML) )
	{
		m_vecAlbums.push_back(newAlbum);
		return true;
	}

	// check if we found a list of albums
	CStdString strHTMLLow=strHTML;
	strHTMLLow.MakeLower();
	
	int iStartOfTable=strHTMLLow.Find("id=\"expansiontable1\"");
	if (iStartOfTable< 0) return false;
	iStartOfTable=strHTMLLow.ReverseFind("<table",iStartOfTable);
	if (iStartOfTable < 0) return false;

	CHTMLTable table;
	CHTMLUtil  util;
	CStdString strTable=strHTML.Right((int)strHTML.size()-iStartOfTable);
	table.Parse(strTable);
	for (int i=1; i < table.GetRows(); ++i)
	{
		const CHTMLRow& row=table.GetRow(i);
		CStdString strAlbumName;
		CStdString strAlbumURL;

		for (int iCol=0; iCol < row.GetColumns(); ++iCol)
		{
			CStdString strColum=row.GetColumValue(iCol);

			if (iCol==1 && !strColum.IsEmpty())
				strAlbumName="("+strColum+")";

			if (iCol==3)
			{
				CStdString strArtist=strColum;
				util.RemoveTags(strArtist);
				if (strColum!="&nbsp;")
					strAlbumName="- " + strArtist + " " + strAlbumName;
			}

			if (iCol==5)
			{
				CStdString strAlbum=strColum;
				util.RemoveTags(strAlbum);
				strAlbumName=strAlbum + " " + strAlbumName;
			}
			if (iCol==5 && strColum.Find("a onclick=\"z('") >= 0)
			{
				int iPos=strColum.Find(">");
				if (iPos >= 0)
				{
					int iPosCookieStart=strColum.Find("('");
					int iPosCookieEnd=strColum.Find("')",iPosCookieStart);
					if (iPosCookieStart>=0 && iPosCookieEnd>=0)
					{
						iPosCookieStart+=2;
						CStdString strCookie=strColum.Mid(iPosCookieStart,(int)iPosCookieEnd-iPosCookieStart);
						// full album url:
						// http://www.allmusic.com/cg/amg.dll?p=amg&token=&sql=10:66jieal64xs7
						strAlbumURL.Format("http://www.allmusic.com/cg/amg.dll?p=amg&token=&sql=%s", strCookie.c_str());

						CMusicAlbumInfo newAlbum(strAlbumName,strAlbumURL);
						m_vecAlbums.push_back(newAlbum);
					}
				}
			}
		}
	}
	
	return true;
}

