#include "stdafx.h"
#include "./MusicAlbumInfo.h"
#include "./HTMLTable.h"
#include "./HTMLUtil.h"

using namespace MUSIC_GRABBER;
using namespace HTML;

CMusicAlbumInfo::CMusicAlbumInfo(void)
{
  m_strTitle2 = "";
  m_strDateOfRelease = "";
  m_strAlbumURL = "";
  m_bLoaded = false;
}

CMusicAlbumInfo::~CMusicAlbumInfo(void)
{
}

CMusicAlbumInfo::CMusicAlbumInfo(const CStdString& strAlbumInfo, const CStdString& strAlbumURL)
{
  m_strTitle2 = strAlbumInfo;
  m_strDateOfRelease = "";
  m_strAlbumURL = strAlbumURL;
  m_bLoaded = false;
}

CMusicAlbumInfo::CMusicAlbumInfo(const CStdString& strAlbum, const CStdString& strArtist, const CStdString& strAlbumInfo, const CStdString& strAlbumURL)
{
  m_album.strAlbum = strAlbum;
  m_album.strArtist = strArtist;
  m_strTitle2 = strAlbumInfo;
  m_strDateOfRelease = "";
  m_strAlbumURL = strAlbumURL;
  m_bLoaded = false;
}

const CAlbum& CMusicAlbumInfo::GetAlbum() const
{
  return m_album;
}

void CMusicAlbumInfo::SetAlbum(CAlbum& album)
{
  m_album = album;
  m_strDateOfRelease.Format("%i", album.iYear);
  m_strAlbumURL = "";
  m_strTitle2 = "";
  m_bLoaded = true;
}

const VECSONGS &CMusicAlbumInfo::GetSongs() const
{
  return m_songs;
}

void CMusicAlbumInfo::SetSongs(VECSONGS &songs)
{
  m_songs = songs;
}

void CMusicAlbumInfo::SetTitle(const CStdString& strTitle)
{
  m_album.strAlbum = strTitle;
}

const CStdString& CMusicAlbumInfo::GetAlbumURL() const
{
  return m_strAlbumURL;
}

const CStdString& CMusicAlbumInfo::GetTitle2() const
{
  return m_strTitle2;
}

const CStdString& CMusicAlbumInfo::GetDateOfRelease() const
{
  return m_strDateOfRelease;
}

bool CMusicAlbumInfo::Parse(const CStdString& strHTML, CHTTP& http)
{
  m_songs.clear();
  CHTMLUtil util;
  CStdString strHTMLLow = strHTML;
  strHTMLLow.MakeLower();

  if (strHTML.Find("id=\"albumpage\"") == -1)
    return false;

  // Extract Cover URL
  int iStartOfCover = strHTMLLow.Find("image.allmusic.com");
  if (iStartOfCover >= 0)
  {
    iStartOfCover = strHTMLLow.ReverseFind("<img", iStartOfCover);
    int iEndOfCover = strHTMLLow.Find(">", iStartOfCover);
    CStdString strCover = strHTMLLow.Mid(iStartOfCover, iEndOfCover);
    util.getAttributeOfTag(strCover, "src=\"", m_album.strImage);
  }

  // Extract Review
  int iStartOfReview = strHTMLLow.Find("id=\"bio\"");
  if (iStartOfReview >= 0)
  {
    iStartOfReview = strHTMLLow.Find("<table", iStartOfReview);
    if (iStartOfReview >= 0)
    {
      CHTMLTable table;
      CStdString strTable = strHTML.Right((int)strHTML.size() - iStartOfReview);
      table.Parse(strTable);

      if (table.GetRows() > 0)
      {
        CHTMLRow row = table.GetRow(1);
        CStdString strReview = row.GetColumValue(0);
        util.RemoveTags(strReview);
        util.ConvertHTMLToUTF8(strReview, m_album.strReview);
      }
    }
  }

  // if the review has "read more..." get the full review
  CStdString strReview = m_album.strReview;
  strReview.ToLower();
  if (strReview.Find("read more...") >= 0)
  {
    m_strAlbumURL += "~T1";
    return Load(http);
  }

  // Extract album, artist...
  int iStartOfTable = strHTMLLow.Find("id=\"albumpage\"");
  iStartOfTable = strHTMLLow.Find("<table cellpadding=\"0\" cellspacing=\"0\">", iStartOfTable);
  if (iStartOfTable < 0) return false;

  CHTMLTable table;
  CStdString strTable = strHTML.Right((int)strHTML.size() - iStartOfTable);
  table.Parse(strTable);

  // Check if page has the album browser
  int iStartRow = 2;
  if (strHTMLLow.Find("class=\"album-browser\"") == -1)
    iStartRow = 1;

  for (int iRow = iStartRow; iRow < table.GetRows(); iRow++)
  {
    const CHTMLRow& row = table.GetRow(iRow);

    CStdString strColumn = row.GetColumValue(0);
    CHTMLTable valueTable;
    valueTable.Parse(strColumn);
    strColumn = valueTable.GetRow(0).GetColumValue(0);
    util.RemoveTags(strColumn);

    if (strColumn.Find("Artist") >= 0 && valueTable.GetRows() >= 2)
    {
      CStdString strValue = valueTable.GetRow(2).GetColumValue(0);
      util.RemoveTags(strValue);
      util.ConvertHTMLToUTF8(strValue, m_album.strArtist);
    }
    if (strColumn.Find("Album") >= 0 && valueTable.GetRows() >= 2)
    {
      CStdString strValue = valueTable.GetRow(2).GetColumValue(0);
      util.RemoveTags(strValue);
      util.ConvertHTMLToUTF8(strValue, m_album.strAlbum);
    }
    if (strColumn.Find("Release Date") >= 0 && valueTable.GetRows() >= 2)
    {
      CStdString strValue = valueTable.GetRow(2).GetColumValue(0);
      util.RemoveTags(strValue);
      util.ConvertHTMLToUTF8(strValue, m_strDateOfRelease);

      CStdString releaseYear(m_strDateOfRelease);
      // extract the year out of something like "1998 (release)" or "12 feb 2003"
      int nPos = releaseYear.Find("19");
      if (nPos > -1)
      {
        if ((int)releaseYear.size() >= nPos + 3 && ::isdigit(releaseYear.GetAt(nPos + 2)) && ::isdigit(releaseYear.GetAt(nPos + 3)))
        {
          CStdString strYear = releaseYear.Mid(nPos, 4);
          releaseYear = strYear;
        }
        else
        {
          nPos = releaseYear.Find("19", nPos + 2);
          if (nPos > -1)
          {
            if ((int)releaseYear.size() >= nPos + 3 && ::isdigit(releaseYear.GetAt(nPos + 2)) && ::isdigit(releaseYear.GetAt(nPos + 3)))
            {
              CStdString strYear = releaseYear.Mid(nPos, 4);
              releaseYear = strYear;
            }
          }
        }
      }

      nPos = releaseYear.Find("20");
      if (nPos > -1)
      {
        if ((int)releaseYear.size() > nPos + 3 && ::isdigit(releaseYear.GetAt(nPos + 2)) && ::isdigit(releaseYear.GetAt(nPos + 3)))
        {
          CStdString strYear = releaseYear.Mid(nPos, 4);
          releaseYear = strYear;
        }
        else
        {
          nPos = releaseYear.Find("20", nPos + 1);
          if (nPos > -1)
          {
            if ((int)releaseYear.size() > nPos + 3 && ::isdigit(releaseYear.GetAt(nPos + 2)) && ::isdigit(releaseYear.GetAt(nPos + 3)))
            {
              CStdString strYear = releaseYear.Mid(nPos, 4);
              releaseYear = strYear;
            }
          }
        }
      }
      m_album.iYear = atol(releaseYear.c_str());
    }
    if (strColumn.Find("Genre") >= 0 && valueTable.GetRows() >= 1)
    {
      CStdString strHTML = valueTable.GetRow(1).GetColumValue(0);
      CStdString strTag;
      int iStartOfGenre = util.FindTag(strHTML, "<li", strTag);
      if (iStartOfGenre >= 0)
      {
        iStartOfGenre += (int)strTag.size();
        int iEndOfGenre = util.FindClosingTag(strHTML, "li", strTag, iStartOfGenre) - 1;
        if (iEndOfGenre < 0)
        {
          iEndOfGenre = (int)strHTML.size();
        }

        CStdString strValue = strHTML.Mid(iStartOfGenre, 1 + iEndOfGenre - iStartOfGenre);
        util.RemoveTags(strValue);
        util.ConvertHTMLToUTF8(strValue, m_album.strGenre);
      }

      if (valueTable.GetRow(0).GetColumns() >= 2)
      {
        strColumn = valueTable.GetRow(0).GetColumValue(2);
        util.RemoveTags(strColumn);

        CStdString strStyles;
        if (strColumn.Find("Styles") >= 0)
        {
          CStdString strHTML = valueTable.GetRow(1).GetColumValue(1);
          CStdString strTag;
          int iStartOfStyle = 0;
          while (iStartOfStyle >= 0)
          {
            iStartOfStyle = util.FindTag(strHTML, "<li", strTag, iStartOfStyle);
            iStartOfStyle += (int)strTag.size();
            int iEndOfStyle = util.FindClosingTag(strHTML, "li", strTag, iStartOfStyle) - 1;
            if (iEndOfStyle < 0)
              break;

            CStdString strValue = strHTML.Mid(iStartOfStyle, 1 + iEndOfStyle - iStartOfStyle);
            util.RemoveTags(strValue);
            strStyles += strValue + ", ";
          }

          strStyles.TrimRight(", ");
          util.ConvertHTMLToUTF8(strStyles, m_album.strStyles);
        }
      }
    }
    if (strColumn.Find("Moods") >= 0)
    {
      CStdString strHTML = valueTable.GetRow(1).GetColumValue(0);
      CStdString strTag, strMoods;
      int iStartOfMoods = 0;
      while (iStartOfMoods >= 0)
      {
        iStartOfMoods = util.FindTag(strHTML, "<li", strTag, iStartOfMoods);
        iStartOfMoods += (int)strTag.size();
        int iEndOfMoods = util.FindClosingTag(strHTML, "li", strTag, iStartOfMoods) - 1;
        if (iEndOfMoods < 0)
          break;

        CStdString strValue = strHTML.Mid(iStartOfMoods, 1 + iEndOfMoods - iStartOfMoods);
        util.RemoveTags(strValue);
        strMoods += strValue + ", ";
      }

      strMoods.TrimRight(", ");
      util.ConvertHTMLToUTF8(strMoods, m_album.strTones);
    }
    if (strColumn.Find("Rating") >= 0)
    {
      CStdString strValue = valueTable.GetRow(1).GetColumValue(0);
      CStdString strRating;
      util.getAttributeOfTag(strValue, "src=", strRating);
      strRating.Delete(0, 25);
      strRating.Delete(1, 4);
      m_album.iRating = atoi(strRating);
    }
  }

  // parse songs...
  iStartOfTable = strHTMLLow.Find("id=\"expansiontable1\"", 0);
  if (iStartOfTable >= 0)
  {
    iStartOfTable = strHTMLLow.ReverseFind("<table", iStartOfTable);
    if (iStartOfTable >= 0)
    {
      strTable = strHTML.Right((int)strHTML.size() - iStartOfTable);
      table.Parse(strTable);
      for (int iRow = 1; iRow < table.GetRows(); iRow++)
      {
        const CHTMLRow& row = table.GetRow(iRow);
        int iCols = row.GetColumns();
        if (iCols >= 7)
        {
          CSong song;
          // Tracknumber
          song.iTrack = atoi(row.GetColumValue(2));

          // Songname
          CStdString strValue, strName;
          strValue = row.GetColumValue(4);
          util.RemoveTags(strValue);
          strValue.Trim();
          if (strValue.Find("[*]") > -1)
            strValue.TrimRight("[*]");
          util.ConvertHTMLToUTF8(strValue, song.strTitle);

          // Duration
          CStdString strDuration = row.GetColumValue(6);
          int iPos = strDuration.Find(":");
          if (iPos >= 0)
          {
            CStdString strMin, strSec;
            strMin = strDuration.Left(iPos);
            iPos++;
            strSec = strDuration.Right((int)strDuration.size() - iPos);
            int iMin = atoi(strMin.c_str());
            int iSec = atoi(strSec.c_str());
            song.iDuration = iMin * 60 + iSec;
          }
          m_songs.push_back(song);
        }
      }
    }
  }
  if (m_strTitle2 = "") m_strTitle2 = m_album.strAlbum;
  SetLoaded(true);
  return true;
}


bool CMusicAlbumInfo::Load(CHTTP& http)
{
  CStdString strHTML;
  if ( !http.Get(m_strAlbumURL, strHTML)) return false;
  return Parse(strHTML, http);
}

void CMusicAlbumInfo::SetLoaded(bool bOnOff)
{
  m_bLoaded = bOnOff;
}

bool CMusicAlbumInfo::Loaded() const
{
  return m_bLoaded;
}
