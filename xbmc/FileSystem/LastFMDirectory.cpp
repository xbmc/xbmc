
#include "stdafx.h"
#include "LastFMDirectory.h"
#include "DirectoryCache.h"
#include "../Util.h"
#include "../MusicDatabase.h"

using namespace DIRECTORY;

#define AUDIOSCROBBLER_BASE_URL      "http://ws.audioscrobbler.com/1.0/"

CLastFMDirectory::CLastFMDirectory()
{
  m_Error = false;
  m_Downloaded = false;
}

CLastFMDirectory::~CLastFMDirectory()
{
}

CStdString CLastFMDirectory::BuildURLFromInfo()
{
  CStdString strURL = (CStdString)AUDIOSCROBBLER_BASE_URL;
  strURL += m_objtype + "/" + m_encodedobjname + "/" + m_objrequest + ".xml";

  return strURL;
}

bool CLastFMDirectory::RetrieveList(CStdString url)
{
  m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (m_dlgProgress)
  {
    m_dlgProgress->ShowProgressBar(false);
    m_dlgProgress->SetHeading(2);
    m_dlgProgress->SetLine(0, 15279);
    m_dlgProgress->SetLine(1, m_objrequest);
    m_dlgProgress->SetLine(2, m_objname);
    m_dlgProgress->StartModal();
    m_dlgProgress->Progress();
  }

  CThread thread(this);
  m_strSource = url;
  m_strDestination = _P("Z:\\lastfm.xml");
  thread.Create();

  while (!m_Downloaded)
  {
    m_dlgProgress->Progress();

    if (m_dlgProgress->IsCanceled())
    {
      m_http.Cancel();
      thread.StopThread();
      m_dlgProgress->Close();
      return false;
    }
  }

  if (!m_dlgProgress->IsCanceled() && m_Error)
  {
    if (m_dlgProgress) m_dlgProgress->Close();
    CGUIDialogOK::ShowAndGetInput(257, 15280, 0, 0);
    CLog::Log(LOGERROR, "Unable to retrieve list from last.fm");
    return false;
  }


  if (!m_xmlDoc.LoadFile(m_strDestination.c_str()))
  {
    if (m_dlgProgress) m_dlgProgress->Close();
    CGUIDialogOK::ShowAndGetInput(257, 15280, 0, 0);
    CLog::Log(LOGERROR, "Error parsing file from audioscrobbler web services, Line %d\n%s", m_xmlDoc.ErrorRow(), m_xmlDoc.ErrorDesc());
    return false;
  }

  m_dlgProgress->Close();

  return true;
}

void CLastFMDirectory::AddEntry(int iString, CStdString strPath, CStdString strIconPath, bool bFolder, CFileItemList &items)
{
  CStdString strLabel = g_localizeStrings.Get(iString);
  strLabel.Replace("%name%", m_objname);
  strLabel.Replace("%type%", m_objtype);
  strLabel.Replace("%request%", m_objrequest);
  strPath.Replace("%name%", m_encodedobjname);
  strPath.Replace("%type%", m_objtype);
  strPath.Replace("%request%", m_objrequest);

  CFileItem *pItem = new CFileItem;
  pItem->SetLabel(strLabel);
  pItem->m_strPath = strPath;
  pItem->m_bIsFolder = bFolder;
  pItem->SetLabelPreformated(true);
  //the extra info is used in the mediawindows to determine which items are needed in the contextmenu
  if (strPath.Find("lastfm://xbmc") >= 0)
  {
    pItem->SetCanQueue(false);
    pItem->SetExtraInfo("lastfmitem");
  }
  
  items.Add(pItem);
}

void CLastFMDirectory::AddListEntry(const char *name, const char *artist, const char *count, const char *date, const char *icon, CStdString strPath, CFileItemList &items)
{
  CStdString strName;
  CStdString strCount;
  CFileItem *pItem = new CFileItem;
  CMusicInfoTag* musicinfotag = pItem->GetMusicInfoTag();
  musicinfotag->SetTitle(name);

  if (artist)
  {
    strName.Format("%s - %s", artist, name);
    musicinfotag->SetArtist(artist);
  }
  else
  {
    strName = name;
  }

  if (count)
  {
    pItem->SetLabel2(count);
    pItem->m_dwSize = _atoi64(count) * 100000000;

    const char *dot;
    if ((dot = (const char *)strstr(count, ".")))
      pItem->m_dwSize += _atoi64(dot + 1);
  }

  pItem->SetLabel(strName);
  pItem->m_strPath = strPath;
  pItem->m_bIsFolder = true;
  pItem->SetLabelPreformated(true);

  if (date)
  {
    LONGLONG ll = Int32x32To64(atoi(date), 10000000) + 116444736000000000LL;
    FILETIME ft;

    ft.dwLowDateTime = (DWORD)(ll & 0xFFFFFFFF);
    ft.dwHighDateTime = (DWORD)(ll >> 32);
    
    pItem->m_dateTime=ft;
  }

  pItem->SetCanQueue(false);
  //the extra info is used in the mediawindows to determine which items are needed in the contextmenu
  if (m_objname.Equals(g_guiSettings.GetString("lastfm.username")))
  {
    if (m_objrequest.Equals("recentbannedtracks"))
    {
      pItem->SetExtraInfo("lastfmbanned");
    }
    else if (m_objrequest.Equals("recentlovedtracks"))
    {
      pItem->SetExtraInfo("lastfmloved");
    }
  }
  if (pItem->GetExtraInfo().IsEmpty() && strPath.Find("lastfm://xbmc") >= 0)
  {
    pItem->SetExtraInfo("lastfmitem");
  }

  // icons? would probably take too long to retrieve them all
  items.Add(pItem);
  m_vecCachedItems.Add(new CFileItem(*pItem));
}

bool CLastFMDirectory::ParseArtistList(CStdString url, CFileItemList &items)
{
  if (!RetrieveList(url))
    return false;

  TiXmlElement* pRootElement = m_xmlDoc.RootElement();

  TiXmlElement* pEntry = pRootElement->FirstChildElement("artist");

  while(pEntry)
  {
    TiXmlNode* name = pEntry->FirstChild("name");
    TiXmlNode* count;
    const char *countstr = NULL;
    const char *namestr = NULL;

    count = pEntry->FirstChild("count");
    if (!count) count = pEntry->FirstChild("playcount");
    if (!count) count = pEntry->FirstChild("match");
    if (!count && pEntry->Attribute("count"))
      countstr = pEntry->Attribute("count");
    else
      countstr = count->FirstChild()->Value();
    if (name)
      namestr = name->FirstChild()->Value();
    else
      namestr = pEntry->Attribute("name");


    if (namestr)
      AddListEntry(namestr, NULL, countstr, NULL, NULL,
          "lastfm://xbmc/artist/" + (CStdString)namestr + "/", items);

    pEntry = pEntry->NextSiblingElement("artist");
  }

  m_xmlDoc.Clear();
  return true;
}

bool CLastFMDirectory::ParseAlbumList(CStdString url, CFileItemList &items)
{
  if (!RetrieveList(url))
    return false;

  TiXmlElement* pRootElement = m_xmlDoc.RootElement();

  TiXmlElement* pEntry = pRootElement->FirstChildElement("album");

  while(pEntry)
  {
    TiXmlNode* name = pEntry->FirstChild("name");
    TiXmlNode* artist = pEntry->FirstChild("artist");

    TiXmlNode* count;
    count = pEntry->FirstChild("count");
    if (!count) count = pEntry->FirstChild("playcount");

    if (name)
    {
      AddListEntry(name->FirstChild()->Value(), artist->FirstChild()->Value(), count->FirstChild()->Value(),
          NULL, NULL, "lastfm://xbmc/artist/" + (CStdString)artist->FirstChild()->Value() + "/", items);
    }
    else
    {
      // no luck, try another way :)
      const char *name = pEntry->Attribute("name");
      const char *artist = pEntry->FirstChildElement("artist")->Attribute("name");
      const char *count = pEntry->Attribute("count");

      if (name)
        AddListEntry(name, artist, count, NULL, NULL,
            "lastfm://xbmc/artist/" + (CStdString)artist + "/", items);
    }

    pEntry = pEntry->NextSiblingElement("album");
  }

  m_xmlDoc.Clear();
  return true;
}

bool CLastFMDirectory::ParseUserList(CStdString url, CFileItemList &items)
{
  if (!RetrieveList(url))
    return false;

  TiXmlElement* pRootElement = m_xmlDoc.RootElement();

  TiXmlElement* pEntry = pRootElement->FirstChildElement("user");

  while(pEntry)
  {
    const char *name = pEntry->Attribute("username");

    TiXmlNode* count;
    count = pEntry->FirstChild("weight");
    if (!count) count = pEntry->FirstChild("match");

    if (name)
    {
      AddListEntry(name, NULL, (count) ? count->FirstChild()->Value() : NULL, NULL, NULL,
          "lastfm://xbmc/user/" + (CStdString)name + "/", items);
    }

    pEntry = pEntry->NextSiblingElement("user");
  }

  m_xmlDoc.Clear();
  return true;
}

bool CLastFMDirectory::ParseTagList(CStdString url, CFileItemList &items)
{
  if (!RetrieveList(url))
    return false;

  TiXmlElement* pRootElement = m_xmlDoc.RootElement();

  TiXmlElement* pEntry = pRootElement->FirstChildElement("tag");

  while(pEntry)
  {
    TiXmlNode* name = pEntry->FirstChild("name");
    TiXmlNode* count;
    const char *countstr = NULL;
    const char *namestr = NULL;

    count = pEntry->FirstChild("count");
    if (!count) count = pEntry->FirstChild("playcount");
    if (!count) count = pEntry->FirstChild("match");
    if (!count && pEntry->Attribute("count"))
      countstr = pEntry->Attribute("count");
    else if (count->FirstChild())
      countstr = count->FirstChild()->Value();

    if (name && name->FirstChild())
      namestr = name->FirstChild()->Value();
    else
      namestr = pEntry->Attribute("name");

    if (namestr)
    {
      AddListEntry(namestr, NULL, countstr, NULL, NULL,
          "lastfm://xbmc/tag/" + (CStdString)namestr + "/", items);
    }

    pEntry = pEntry->NextSiblingElement("tag");
  }

  m_xmlDoc.Clear();
  return true;
}

bool CLastFMDirectory::ParseTrackList(CStdString url, CFileItemList &items)
{
  if (!RetrieveList(url))
    return false;

  TiXmlElement* pRootElement = m_xmlDoc.RootElement();

  TiXmlElement* pEntry = pRootElement->FirstChildElement("track");

  while(pEntry)
  {
    TiXmlNode* name = pEntry->FirstChild("name");
    TiXmlNode* artist = pEntry->FirstChild("artist");
    TiXmlElement *date = pEntry->FirstChildElement("date");

    TiXmlNode* count;
    count = pEntry->FirstChild("count");
    if (!count) count = pEntry->FirstChild("playcount");
    if (!count) count = pEntry->FirstChild("match");

    if (name)
    {
      if (artist)
        AddListEntry((name) ? name->FirstChild()->Value() : NULL,
            (artist) ? artist->FirstChild()->Value() : NULL, 
            (count) ? count->FirstChild()->Value() : ((date) ? date->FirstChild()->Value() : NULL),
            (date) ? date->Attribute("uts") : NULL,
            NULL, "lastfm://xbmc/artist/" + (CStdString)artist->FirstChild()->Value() + "/", items);
      else
        // no artist in xml, assuming we're retrieving track list for the artist in m_objname...
        AddListEntry((name) ? name->FirstChild()->Value() : NULL,
            m_objname.c_str(),
            (count) ? count->FirstChild()->Value() : NULL,
            NULL, NULL, "lastfm://xbmc/artist/" + m_objname + "/", items);
    }
    else
    {
      // no luck, try another way :)
      const char *name = pEntry->Attribute("name");
      const char *artist = pEntry->FirstChildElement("artist")->Attribute("name");
      const char *count = pEntry->Attribute("count");

      if (name)
        AddListEntry(name, artist, count, NULL, NULL,
            "lastfm://xbmc/artist/" + (CStdString)artist + "/", items);
    }

    pEntry = pEntry->NextSiblingElement("track");
  }

  m_xmlDoc.Clear();
  return true;
}

bool CLastFMDirectory::SearchSimilarArtists(CFileItemList &items)
{
  CStdString strSearchTerm = "";

  if (!CGUIDialogKeyboard::ShowAndGetInput(strSearchTerm, g_localizeStrings.Get(15281), false))
    return false;

  m_objname = m_encodedobjname = strSearchTerm;
  CUtil::URLEncode(m_encodedobjname);
  CUtil::UrlDecode(m_objname);

  AddEntry(15267, "lastfm://artist/%name%/similarartists", "", false, items);
  return ParseArtistList(BuildURLFromInfo(), items);
}

bool CLastFMDirectory::SearchSimilarTags(CFileItemList &items)
{
  CStdString strSearchTerm = "";

  if (!CGUIDialogKeyboard::ShowAndGetInput(strSearchTerm, g_localizeStrings.Get(15282), false))
    return false;

  m_objname = m_encodedobjname = strSearchTerm;
  CUtil::URLEncode(m_encodedobjname);
  CUtil::UrlDecode(m_objname);

  return ParseTagList(BuildURLFromInfo(), items);
}

bool CLastFMDirectory::GetArtistInfo(CFileItemList &items)
{
  if (m_objname == "*" && m_objrequest == "similar")
    return SearchSimilarArtists(items);

  if (m_objrequest == "similar")
    return ParseArtistList(BuildURLFromInfo(), items);
  else if (m_objrequest == "topalbums")
    return ParseAlbumList(BuildURLFromInfo(), items);
  else if (m_objrequest == "toptracks")
    return ParseTrackList(BuildURLFromInfo(), items);
  else if (m_objrequest == "toptags")
    return ParseTagList(BuildURLFromInfo(), items);
  else if (m_objrequest == "fans")
    return ParseUserList(BuildURLFromInfo(), items);
  else if (m_objrequest == "")
  {
    AddEntry(15261, "lastfm://xbmc/artist/%name%/similar/", "", true, items);
    AddEntry(15262, "lastfm://xbmc/artist/%name%/topalbums/", "", true, items);
    AddEntry(15263, "lastfm://xbmc/artist/%name%/toptracks/", "", true, items);
    AddEntry(15264, "lastfm://xbmc/artist/%name%/toptags/", "", true, items);
    AddEntry(15265, "lastfm://xbmc/artist/%name%/fans/", "", true, items);
    AddEntry(15266, "lastfm://artist/%name%/fans", "", false, items);
    AddEntry(15267, "lastfm://artist/%name%/similarartists", "", false, items);
  }
  else
    return false;

  return true;
}

bool CLastFMDirectory::GetUserInfo(CFileItemList &items)
{
  if (m_objrequest == "topartists")
    return ParseArtistList(BuildURLFromInfo(), items);
  else if (m_objrequest == "topalbums")
    return ParseAlbumList(BuildURLFromInfo(), items);
  else if (m_objrequest == "toptracks")
    return ParseTrackList(BuildURLFromInfo(), items);
  else if (m_objrequest == "toptags")
    return ParseTagList(BuildURLFromInfo(), items);
  else if (m_objrequest == "tags")
    return ParseTagList(BuildURLFromInfo(), items);
  else if (m_objrequest == "friends")
    return ParseUserList(BuildURLFromInfo(), items);
  else if (m_objrequest == "neighbours")
    return ParseUserList(BuildURLFromInfo(), items);
  else if (m_objrequest == "weeklyartistchart")
    return ParseArtistList(BuildURLFromInfo(), items);
  else if (m_objrequest == "weeklyalbumchart")
    return ParseAlbumList(BuildURLFromInfo(), items);
  else if (m_objrequest == "weeklytrackchart")
    return ParseTrackList(BuildURLFromInfo(), items);
  else if (m_objrequest == "recenttracks")
    return ParseTrackList(BuildURLFromInfo(), items);
  else if (m_objrequest == "recentlovedtracks")
    return ParseTrackList(BuildURLFromInfo(), items);
  else if (m_objrequest == "recentbannedtracks")
    return ParseTrackList(BuildURLFromInfo(), items);
  else if (m_objrequest == "")
  {
    AddEntry(15268, "lastfm://xbmc/user/%name%/topartists/", "", true, items);
    AddEntry(15269, "lastfm://xbmc/user/%name%/topalbums/", "", true, items);
    AddEntry(15270, "lastfm://xbmc/user/%name%/toptracks/", "", true, items);
    AddEntry(15285, "lastfm://xbmc/user/%name%/tags/", "", true, items);
    AddEntry(15271, "lastfm://xbmc/user/%name%/friends/", "", true, items);
    AddEntry(15272, "lastfm://xbmc/user/%name%/neighbours/", "", true, items);
    AddEntry(15273, "lastfm://xbmc/user/%name%/weeklyartistchart/", "", true, items);
    AddEntry(15274, "lastfm://xbmc/user/%name%/weeklyalbumchart/", "", true, items);
    AddEntry(15275, "lastfm://xbmc/user/%name%/weeklytrackchart/", "", true, items);
    AddEntry(15283, "lastfm://xbmc/user/%name%/recenttracks/", "", true, items);
    AddEntry(15293, "lastfm://xbmc/user/%name%/recentlovedtracks/", "", true, items);
    AddEntry(15294, "lastfm://xbmc/user/%name%/recentbannedtracks/", "", true, items);
    AddEntry(15276, "lastfm://user/%name%/neighbours", "", false, items);
    AddEntry(15277, "lastfm://user/%name%/personal", "", false, items);
    AddEntry(15278, "lastfm://user/%name%/loved", "", false, items);
    AddEntry(15284, "lastfm://user/%name%/recommended/100", "", false, items);
    AddEntry(15286, "lastfm://user/%name%/playlist", "", false, items);
  }
  else
    return false;

  return true;
}

bool CLastFMDirectory::GetTagInfo(CFileItemList &items)
{
  if (m_objname == "*" && m_objrequest== "search")
    return SearchSimilarTags(items);

  if (m_objrequest == "topartists")
    return ParseArtistList(BuildURLFromInfo(), items);
  else if (m_objrequest == "topalbums")
    return ParseAlbumList(BuildURLFromInfo(), items);
  else if (m_objrequest == "toptracks")
    return ParseTrackList(BuildURLFromInfo(), items);
  else if (m_objrequest == "toptags")
    return ParseTagList(BuildURLFromInfo(), items);
  else if (m_objrequest == "")
  {
    AddEntry(15257, "lastfm://xbmc/tag/%name%/topartists/", "", true, items);
    AddEntry(15258, "lastfm://xbmc/tag/%name%/topalbums/", "", true, items);
    AddEntry(15259, "lastfm://xbmc/tag/%name%/toptracks/", "", true, items);
    AddEntry(15260, "lastfm://globaltags/%name%", "", false, items);
  }

  return true;
}

bool CLastFMDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString strURL = strPath;
  CURL url(strURL);
  strURL=url.GetFileName();

  // parse the URL, finding object type, name, and requested info
  CStdStringArray vecURLParts;
  m_vecCachedItems.Clear();

  m_objtype = "";
  m_objname = "";
  m_objrequest = "";

  switch(StringUtils::SplitString(strURL, "/", vecURLParts))
  {
  case 1:
    // simple lastfm:// root URL...
    g_directoryCache.Clear();
    break;
  // the following fallthru's are on purpose
  case 5:
    m_objrequest = vecURLParts[3];
  case 4:
    m_objname = vecURLParts[2];
    m_encodedobjname = vecURLParts[2];
    CUtil::URLEncode(m_encodedobjname);
    CUtil::UrlDecode(m_objname);
  case 3:
    m_objtype = vecURLParts[1];
  case 2:
    if (vecURLParts[0] != "xbmc")
      return false;
    break;
  default:
    return false;
  }

  if (g_directoryCache.GetDirectory(strPath, items))
    return true;

  if (m_objtype == "user")
    m_Error = GetUserInfo(items);
  else if (m_objtype == "tag")
    m_Error = GetTagInfo(items);
  else if (m_objtype == "artist")
    m_Error = GetArtistInfo(items);
  else if (m_objtype == "")
  {
    AddEntry(15253, "lastfm://xbmc/artist/*/similar/", "", true, items);
    AddEntry(15254, "lastfm://xbmc/tag/*/search/", "", true, items);
    AddEntry(15256, "lastfm://xbmc/tag/xbmc/toptags/", "", true, items);
    if (g_guiSettings.GetString("lastfm.username") != "")
    {
      m_encodedobjname = m_objname = g_guiSettings.GetString("lastfm.username");
      CUtil::UrlDecode(m_encodedobjname);
      AddEntry(15255, "lastfm://xbmc/user/%name%/", "", true, items);
    }
  }
  else
    return false;

  if (m_cacheDirectory && !m_vecCachedItems.IsEmpty() )
    g_directoryCache.SetDirectory(strPath, m_vecCachedItems);

  return true;
}


void CLastFMDirectory::Run()
{
  if (!m_http.Download(m_strSource, m_strDestination))
    m_Error=true;

  m_Downloaded=true;
}
