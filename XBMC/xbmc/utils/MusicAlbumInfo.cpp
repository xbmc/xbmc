#include "../stdafx.h"
#include ".\musicalbuminfo.h"
#include ".\htmltable.h"
#include ".\htmlutil.h"
#include ".\http.h"
#include "../util.h"

using namespace MUSIC_GRABBER;
using namespace HTML;

CMusicAlbumInfo::CMusicAlbumInfo(void)
{
	m_strArtist="";
	m_strTitle="";
	m_strDateOfRelease="";
	m_strGenre="";
	m_strTones="";
	m_strStyles="";
	m_strReview="";
	m_strImageURL="";
	m_strTitle2="";
	m_iRating=0;
	m_bLoaded=false;
}

CMusicAlbumInfo::~CMusicAlbumInfo(void)
{

}

CMusicAlbumInfo::CMusicAlbumInfo(const CStdString& strAlbumInfo, const CStdString& strAlbumURL)
{
	m_strArtist="";
	m_strDateOfRelease="";
	m_strGenre="";
	m_strTones="";
	m_strStyles="";
	m_strReview="";
	m_strTitle2="";
	m_iRating=0;
	m_bLoaded=false;
	m_strTitle2		= strAlbumInfo;
	m_strAlbumURL	= strAlbumURL;
}

const CStdString& CMusicAlbumInfo::GetAlbumURL() const
{
	return m_strAlbumURL;
}

const CStdString& CMusicAlbumInfo::GetArtist() const
{
	return m_strArtist;
}

const CStdString& CMusicAlbumInfo::GetTitle() const
{
	return m_strTitle;
}

void CMusicAlbumInfo::SetTitle(const CStdString& strTitle)
{
	m_strTitle=strTitle;
}

const CStdString& CMusicAlbumInfo::GetTitle2() const
{
	return m_strTitle2;
}

const CStdString& CMusicAlbumInfo::GetDateOfRelease() const
{
	return m_strDateOfRelease;
}

const CStdString& CMusicAlbumInfo::GetGenre() const
{
	return m_strGenre;
}

const CStdString& CMusicAlbumInfo::GetTones() const
{
	return m_strTones;
}

const CStdString& CMusicAlbumInfo::GetStyles() const
{
	return m_strStyles;
}

const CStdString& CMusicAlbumInfo::GetReview() const
{
	return m_strReview;
}

const CStdString& CMusicAlbumInfo::GetImageURL() const
{
	return m_strImageURL;
}

int	CMusicAlbumInfo::GetRating() const
{
	return m_iRating;
}

int	CMusicAlbumInfo::GetNumberOfSongs() const
{
	return (int)m_vecSongs.size();
}

const CMusicSong& CMusicAlbumInfo::GetSong(int iSong)
{
	return m_vecSongs[iSong];
}

bool	CMusicAlbumInfo::Parse(const CStdString& strHTML)
{
	m_vecSongs.erase(m_vecSongs.begin(),m_vecSongs.end());
	CHTMLUtil  util;
	CStdString strHTMLLow=strHTML;
	strHTMLLow.MakeLower();
	
  if (strHTML.Find("id=\"albumpage\"")==-1)
    return false;

	//	Extract Cover URL
	int iStartOfCover=strHTMLLow.Find("image.allmusic.com");
	if (iStartOfCover>=0)
	{
		iStartOfCover=strHTMLLow.ReverseFind("<img", iStartOfCover);
		int iEndOfCover=strHTMLLow.Find(">", iStartOfCover);
		CStdString strCover=strHTMLLow.Mid(iStartOfCover, iEndOfCover);
		util.getAttributeOfTag(strCover, "src=\"", m_strImageURL);
	}

	//	Extract Review
	int iStartOfReview=strHTMLLow.Find("id=\"bio\"");
	if (iStartOfReview>=0)
	{
		iStartOfReview=strHTMLLow.Find("<table", iStartOfReview);
		if (iStartOfReview>=0)
		{
			CHTMLTable table;
			CStdString strTable=strHTML.Right((int)strHTML.size()-iStartOfReview);
			table.Parse(strTable);

			if (table.GetRows()>0)
			{
				CHTMLRow row=table.GetRow(1);
				CStdString strReview=row.GetColumValue(0);
				util.RemoveTags(strReview);
				util.ConvertHTMLToAnsi(strReview, m_strReview);
			}
		}
	}

	if (m_strReview.IsEmpty())
		m_strReview=g_localizeStrings.Get(414);

	// if the review has "read more..." get the full review
	CStdString strReview=m_strReview;
	strReview.ToLower();
	if (strReview.Find("read more...") >= 0)
	{
		m_strAlbumURL += "~T1";
		return Load();
	}
  
	//	Extract album, artist...
	int iStartOfTable=strHTMLLow.Find("id=\"albumpage\"");
  iStartOfTable=strHTMLLow.Find("<table cellpadding=\"0\" cellspacing=\"0\">", iStartOfTable);
	if (iStartOfTable< 0) return false;

	CHTMLTable table;
	CStdString strTable=strHTML.Right((int)strHTML.size()-iStartOfTable);
	table.Parse(strTable);

	//	Check if page has the album browser
	int iStartRow=2;
	if (strHTMLLow.Find("class=\"album-browser\"")==-1)
		iStartRow=1;

	for (int iRow=iStartRow; iRow < table.GetRows(); iRow++)
	{
		const CHTMLRow& row=table.GetRow(iRow);

		CStdString strColumn=row.GetColumValue(0);
		CHTMLTable valueTable;
		valueTable.Parse(strColumn);
		strColumn=valueTable.GetRow(0).GetColumValue(0);
		util.RemoveTags(strColumn);

		if (strColumn.Find("Artist") >=0 && valueTable.GetRows()>=2)
		{
			CStdString strValue=valueTable.GetRow(2).GetColumValue(0);
			util.RemoveTags(strValue);
			util.ConvertHTMLToAnsi(strValue, m_strArtist);
		}
		if (strColumn.Find("Album") >=0 && valueTable.GetRows()>=2)
		{
			CStdString strValue=valueTable.GetRow(2).GetColumValue(0);
			util.RemoveTags(strValue);
			util.ConvertHTMLToAnsi(strValue, m_strTitle);
		}
		if (strColumn.Find("Release Date") >=0 && valueTable.GetRows()>=2)
		{
			CStdString strValue=valueTable.GetRow(2).GetColumValue(0);
			util.RemoveTags(strValue);
			util.ConvertHTMLToAnsi(strValue, m_strDateOfRelease);

			//	extract the year out of something like "1998 (release)" or "12 feb 2003"
			int nPos=m_strDateOfRelease.Find("19");
			if (nPos>-1)
			{
				if ((int)m_strDateOfRelease.size() >= nPos+3 && ::isdigit(m_strDateOfRelease.GetAt(nPos+2))&&::isdigit(m_strDateOfRelease.GetAt(nPos+3)))
				{
					CStdString strYear=m_strDateOfRelease.Mid(nPos, 4);
					m_strDateOfRelease=strYear;
				}
				else
				{
					nPos=m_strDateOfRelease.Find("19", nPos+2);
					if (nPos>-1)
					{
						if ((int)m_strDateOfRelease.size() >= nPos+3 && ::isdigit(m_strDateOfRelease.GetAt(nPos+2))&&::isdigit(m_strDateOfRelease.GetAt(nPos+3)))
						{
							CStdString strYear=m_strDateOfRelease.Mid(nPos, 4);
							m_strDateOfRelease=strYear;
						}
					}
				}
			}

			nPos=m_strDateOfRelease.Find("20");
			if (nPos>-1)
			{
				if ((int)m_strDateOfRelease.size() > nPos+3 && ::isdigit(m_strDateOfRelease.GetAt(nPos+2))&&::isdigit(m_strDateOfRelease.GetAt(nPos+3)))
				{
					CStdString strYear=m_strDateOfRelease.Mid(nPos, 4);
					m_strDateOfRelease=strYear;
				}
				else
				{
					nPos=m_strDateOfRelease.Find("20", nPos+1);
					if (nPos>-1)
					{
						if ((int)m_strDateOfRelease.size() > nPos+3 && ::isdigit(m_strDateOfRelease.GetAt(nPos+2))&&::isdigit(m_strDateOfRelease.GetAt(nPos+3)))
						{
							CStdString strYear=m_strDateOfRelease.Mid(nPos, 4);
							m_strDateOfRelease=strYear;
						}
					}
				}
			}
		}
		if (strColumn.Find("Genre") >=0 && valueTable.GetRows()>=1)
		{
			CStdString strHTML=valueTable.GetRow(1).GetColumValue(0);
			CStdString strTag;
			int iStartOfGenre=util.FindTag(strHTML,"<li",strTag);
			if (iStartOfGenre>=0)
			{
				iStartOfGenre+=(int)strTag.size();
				int iEndOfGenre=util.FindClosingTag(strHTML,"li", strTag,iStartOfGenre)-1;
				if (iEndOfGenre < 0)
				{
					iEndOfGenre=(int)strHTML.size();
				}
				
				CStdString strValue=strHTML.Mid(iStartOfGenre,1+iEndOfGenre-iStartOfGenre);
				util.RemoveTags(strValue);
				util.ConvertHTMLToAnsi(strValue, m_strGenre);
			}

			if (valueTable.GetRow(0).GetColumns()>=2)
			{
				strColumn=valueTable.GetRow(0).GetColumValue(2);
				util.RemoveTags(strColumn);

				CStdString strStyles;
				if (strColumn.Find("Styles") >=0)
				{
					CStdString strHTML=valueTable.GetRow(1).GetColumValue(1);
					CStdString strTag;
					int iStartOfStyle=0;
					while (iStartOfStyle>=0)
					{
						iStartOfStyle=util.FindTag(strHTML, "<li", strTag, iStartOfStyle);
						iStartOfStyle+=(int)strTag.size();
						int iEndOfStyle=util.FindClosingTag(strHTML, "li", strTag, iStartOfStyle)-1;
						if (iEndOfStyle < 0)
							break;
						
						CStdString strValue=strHTML.Mid(iStartOfStyle, 1+iEndOfStyle-iStartOfStyle);
						util.RemoveTags(strValue);
						strStyles+=strValue + ", ";
					}

					strStyles.TrimRight(", ");
					util.ConvertHTMLToAnsi(strStyles, m_strStyles);
				}
			}
		}
		if (strColumn.Find("Moods") >=0)
		{
				CStdString strHTML=valueTable.GetRow(1).GetColumValue(0);
				CStdString strTag, strMoods;
				int iStartOfMoods=0;
				while (iStartOfMoods>=0)
				{
					iStartOfMoods=util.FindTag(strHTML, "<li", strTag, iStartOfMoods);
					iStartOfMoods+=(int)strTag.size();
					int iEndOfMoods=util.FindClosingTag(strHTML, "li", strTag, iStartOfMoods)-1;
					if (iEndOfMoods < 0)
						break;
					
					CStdString strValue=strHTML.Mid(iStartOfMoods, 1+iEndOfMoods-iStartOfMoods);
					util.RemoveTags(strValue);
					strMoods+=strValue + ", ";
				}

				strMoods.TrimRight(", ");
				util.ConvertHTMLToAnsi(strMoods, m_strTones);
		}
		if (strColumn.Find("Rating") >=0)
		{
			CStdString strValue=valueTable.GetRow(1).GetColumValue(0);
			CStdString strRating;
			util.getAttributeOfTag(strValue, "src=", strRating);
			strRating.Delete(0, 25);
			strRating.Delete(1, 4);
			m_iRating=atoi(strRating);
		}
	}

	//	Set to "Not available" if no value from web
	if (m_strArtist.IsEmpty())
		m_strArtist=g_localizeStrings.Get(416);
	if (m_strDateOfRelease.IsEmpty())
		m_strDateOfRelease=g_localizeStrings.Get(416);
	if (m_strGenre.IsEmpty())
		m_strGenre=g_localizeStrings.Get(416);
	if (m_strTones.IsEmpty())
		m_strTones=g_localizeStrings.Get(416);
	if (m_strStyles.IsEmpty())
		m_strStyles=g_localizeStrings.Get(416);
	if (m_strTitle.IsEmpty())
		m_strTitle=g_localizeStrings.Get(416);


	// parse songs...
	iStartOfTable=strHTMLLow.Find("id=\"expansiontable1\"",0);
	if (iStartOfTable >= 0)
	{
		iStartOfTable=strHTMLLow.ReverseFind("<table",iStartOfTable);
		if (iStartOfTable >= 0)
		{
			strTable=strHTML.Right((int)strHTML.size()-iStartOfTable);
			table.Parse(strTable);
			for (int iRow=1; iRow < table.GetRows(); iRow++)
			{
				const CHTMLRow& row=table.GetRow(iRow);
				int iCols=row.GetColumns();
				if (iCols >=7)
				{

					//	Tracknumber
					int iTrack=atoi(row.GetColumValue(2));

					//	Songname
					CStdString strValue, strName;
					strValue=row.GetColumValue(4);
					util.RemoveTags(strValue);
					strValue.Trim();
					if (strValue.Find("[*]")>-1)
						strValue.TrimRight("[*]");
					util.ConvertHTMLToAnsi(strValue, strName);

					//	Duration
					int iDuration=0;
					CStdString strDuration=row.GetColumValue(6);
					int iPos=strDuration.Find(":");
					if (iPos>=0)
					{
						CStdString strMin, strSec;
						strMin=strDuration.Left(iPos);
						iPos++;
						strSec=strDuration.Right((int)strDuration.size()-iPos);
						int iMin=atoi(strMin.c_str());
						int iSec=atoi(strSec.c_str());
						iDuration=iMin*60+iSec;
					}

					//	Create new song object
					CMusicSong newSong(iTrack, strName, iDuration);
					m_vecSongs.push_back(newSong);
				}
			}
		}
	}
	if (m_strTitle2="") m_strTitle2=m_strTitle;
	SetLoaded(true);
	return true;
}


bool CMusicAlbumInfo::Load()
{
	CStdString strHTML;
	CHTTP http;
	if ( !http.Get(m_strAlbumURL, strHTML)) return false;
	return Parse(strHTML);
}

void CMusicAlbumInfo::Save(CStdString& strFileName)
{
	
	FILE* fd = fopen(strFileName.c_str(), "wb+");
	if (fd)
	{
		CUtil::SaveString(m_strArtist,fd);
		CUtil::SaveString(m_strTitle,fd);
		CUtil::SaveString(m_strDateOfRelease,fd);
		CUtil::SaveString(m_strGenre,fd);
		CUtil::SaveString(m_strTones,fd);
		CUtil::SaveString(m_strStyles,fd);
		CUtil::SaveString(m_strReview,fd);
		CUtil::SaveString(m_strImageURL,fd);
		CUtil::SaveString(m_strTitle2,fd);
		CUtil::SaveInt(m_iRating,fd);
		CUtil::SaveInt(m_vecSongs.size(),fd);
		for (int iSong=0; iSong < (int)m_vecSongs.size(); iSong++)
		{
			CMusicSong& song=m_vecSongs[iSong];
			song.Save(fd);
		}
		fclose(fd);
	}
}

bool CMusicAlbumInfo::Load(CStdString& strFileName)
{
	m_vecSongs.erase(m_vecSongs.begin(),m_vecSongs.end());
	FILE* fd = fopen(strFileName.c_str(), "rb");
	if (fd)
	{
		CUtil::LoadString(m_strArtist,fd);
		CUtil::LoadString(m_strTitle,fd);
		CUtil::LoadString(m_strDateOfRelease,fd);
		CUtil::LoadString(m_strGenre,fd);
		CUtil::LoadString(m_strTones,fd);
		CUtil::LoadString(m_strStyles,fd);
		CUtil::LoadString(m_strReview,fd);
		CUtil::LoadString(m_strImageURL,fd);
		CUtil::LoadString(m_strTitle2,fd);
		m_iRating=CUtil::LoadInt(fd);
		int iSongs=CUtil::LoadInt(fd);
		for (int iSong=0; iSong < iSongs; iSong++)
		{
			CMusicSong song;
			song.Load(fd);
			m_vecSongs.push_back(song);
		}
		
		SetLoaded(true);
		fclose(fd);
		return true;
	}
	return false;
}

void CMusicAlbumInfo::SetLoaded(bool bOnOff)
{
	m_bLoaded=bOnOff;
}

bool CMusicAlbumInfo::Loaded() const
{
	return m_bLoaded;
}

void  CMusicAlbumInfo::Set(CAlbum& album)
{
	m_strArtist				= album.strArtist;
	m_strTitle				= album.strAlbum;
	m_strDateOfRelease.Format("%i",album.iYear);
	m_strGenre				= album.strGenre;
	m_strTones				= album.strTones;
	m_strStyles				= album.strStyles;
	m_strReview				= album.strReview;
	m_strImageURL			= album.strImage;
	m_iRating					= album.iRating;
	m_strAlbumPath		= album.strPath;
	m_strTitle2				= "";
	m_bLoaded					= true;
}

void CMusicAlbumInfo::SetAlbumPath(const CStdString& strAlbumPath)
{
	m_strAlbumPath=strAlbumPath;
}

const CStdString& CMusicAlbumInfo::GetAlbumPath()
{
	return m_strAlbumPath;
}

void CMusicAlbumInfo::SetSongs(vector<CMusicSong> songs)
{
	m_vecSongs=songs;
}
