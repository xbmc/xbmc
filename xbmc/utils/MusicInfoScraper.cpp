
#include "../stdafx.h"

#include ".\musicinfoscraper.h"
#include ".\htmlutil.h"
#include ".\htmltable.h"
#include "../util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace HTML;

CMusicInfoScraper::CMusicInfoScraper(void)
{
  m_bSuccessfull=false;
  m_bCanceled=false;
  m_iAlbum=-1;
}

CMusicInfoScraper::~CMusicInfoScraper(void)
{

}

int CMusicInfoScraper::GetAlbumCount() const
{
  return (int)m_vecAlbums.size();
}

CMusicAlbumInfo& CMusicInfoScraper::GetAlbum(int iAlbum)
{
  return m_vecAlbums[iAlbum];
}

void CMusicInfoScraper::FindAlbuminfo(const CStdString& strAlbum)
{
  m_strAlbum=strAlbum;
  StopThread();
  Create();
}

void CMusicInfoScraper::FindAlbuminfo()
{
  CStdString strAlbum=m_strAlbum;
  CStdString strHTML;
  m_vecAlbums.erase(m_vecAlbums.begin(), m_vecAlbums.end());
  // make request
  // type is
  // http://www.allmusic.com/cg/amg.dll?P=amg&SQL=escapolygy&OPT1=2

  CStdString strPostData;
  CUtil::URLEncode(strAlbum);
  strPostData.Format("P=amg&SQL=%s&OPT1=2", strAlbum.c_str());

  // get the HTML
  if (!m_http.Post("http://www.allmusic.com/cg/amg.dll", strPostData, strHTML))
    return;

  // check if this is an album
  CStdString strURL = "http://www.allmusic.com/cg/amg.dll?";
  strURL += strPostData;
  CMusicAlbumInfo newAlbum("", strURL);
  if (strHTML.Find("No Results Found") > -1) return;
  if (strHTML.Find("Album Search Results for:") == -1)
  {
    if (newAlbum.Parse(strHTML, m_http))
    {
      m_vecAlbums.push_back(newAlbum);
      m_bSuccessfull=true;
      return;
    }
    return;
  }

  // check if we found a list of albums
  CStdString strHTMLLow = strHTML;
  strHTMLLow.MakeLower();

  int iStartOfTable = strHTMLLow.Find("id=\"expansiontable1\"");
  if (iStartOfTable < 0) return;
  iStartOfTable = strHTMLLow.ReverseFind("<table", iStartOfTable);
  if (iStartOfTable < 0) return;

  CHTMLTable table;
  CHTMLUtil util;
  CStdString strTable = strHTML.Right((int)strHTML.size() - iStartOfTable);
  table.Parse(strTable);
  for (int i = 1; i < table.GetRows(); ++i)
  {
    const CHTMLRow& row = table.GetRow(i);
    CStdString strAlbumName;

    for (int iCol = 0; iCol < row.GetColumns(); ++iCol)
    {
      CStdString strColum = row.GetColumValue(iCol);

      // Year
      if (iCol == 1 && !strColum.IsEmpty())
      {
        CStdString strYear = "(" + strColum + ")";
        util.ConvertHTMLToAnsi(strYear, strAlbumName);
      }

      // Artist
      if (iCol == 2)
      {
        if (strColum != "&nbsp;")
        {
          CStdString strArtist;
          util.RemoveTags(strColum);
          util.ConvertHTMLToAnsi(strColum, strArtist);
          strAlbumName = "- " + strArtist + " " + strAlbumName;
        }
      }

      // Album
      if (iCol == 4)
      {
        CStdString strTemp = strColum;
        util.RemoveTags(strTemp);

        CStdString strAlbum;
        util.ConvertHTMLToAnsi(strTemp, strAlbum);
        strAlbumName = strAlbum + " " + strAlbumName;
      }
      // Album URL
      if (iCol == 4 && strColum.Find("<a href") >= 0)
      {
        CStdString strAlbumURL;
        int iStartOfUrl = strColum.Find("<a href", 0);
        int iEndOfUrl = strColum.Find(">", iStartOfUrl);
        CStdString strAlbum = strColum.Mid(iStartOfUrl, iEndOfUrl + 1);
        util.getAttributeOfTag(strAlbum, "href=\"", strAlbumURL);

        if (!strAlbumURL.IsEmpty())
        {
          CMusicAlbumInfo newAlbum(strAlbumName, "http://www.allmusic.com" + strAlbumURL);
          m_vecAlbums.push_back(newAlbum);
        }
      }
    }
  }

  if (m_vecAlbums.size()>0)
    m_bSuccessfull=true;

  return;
}

void CMusicInfoScraper::LoadAlbuminfo(int iAlbum)
{
  m_iAlbum=iAlbum;
  StopThread();
  Create();
}

void CMusicInfoScraper::LoadAlbuminfo()
{
  if (m_iAlbum<0 || m_iAlbum>(int)m_vecAlbums.size())
    return;

  CMusicAlbumInfo& album=m_vecAlbums[m_iAlbum];
  if (!album.Load(m_http))
    return;

  CStdString strThumb;
  CStdString strImage = album.GetImageURL();
  CUtil::GetAlbumThumb(album.GetTitle(), album.GetAlbumPath(), strThumb);
  if (!CFile::Exists(strThumb) && !strImage.IsEmpty() )
  {
    // Download image and save as
    // permanent thumb
    m_http.Download(strImage, strThumb);
  }

  m_bSuccessfull=true;
}

bool CMusicInfoScraper::Completed()
{
  return m_eventStop.WaitMSec(10);
}

bool CMusicInfoScraper::Successfull()
{
  return m_bSuccessfull;
}

void CMusicInfoScraper::Cancel()
{
  m_http.Cancel();
  m_bCanceled=true;
}

bool CMusicInfoScraper::IsCanceled()
{
  return m_bCanceled;
}

void CMusicInfoScraper::OnStartup()
{
  m_bSuccessfull=false;
  m_bCanceled=false;
}

void CMusicInfoScraper::Process()
{
  try
  {
    if (m_strAlbum.size())
    {
      FindAlbuminfo();
      m_strAlbum.Empty();
    }
    if (m_iAlbum>-1)
    {
      LoadAlbuminfo();
      m_iAlbum=-1;
    }
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "Exception in CMusicInfoScraper::Process()");
  }
}
