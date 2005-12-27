
#include "stdafx.h"
#include "playlistpls.h"
#include "util.h"
#include "utils\HTTP.h"


#define START_PLAYLIST_MARKER "[playlist]"
#define PLAYLIST_NAME     "PlaylistName"
using namespace PLAYLIST;
/*----------------------------------------------------------------------
[playlist]
PlaylistName=Playlist 001
File1=E:\Program Files\Winamp3\demo.mp3
Title1=demo
Length1=5
File2=E:\Program Files\Winamp3\demo.mp3
Title2=demo
Length2=5
NumberOfEntries=2
Version=2
----------------------------------------------------------------------*/
CPlayListPLS::CPlayListPLS(void)
{}

CPlayListPLS::~CPlayListPLS(void)
{}


bool CPlayListPLS::Load(const CStdString& strFileName)
{
  m_vecItems.clear();
  CFileItem item(strFileName, false);
  if (item.IsInternetStream())
  {
    //load it from the url
    if (!LoadFromWeb(item.m_strPath))
    {
      CPlayListItem newItem(item.m_strPath, item.m_strPath, 0);
      Add(newItem);
    }
    return true;
  }
  //read it from the file
  CStdString strBasePath;
  bool bShoutCast = false;
  CStdString strExt = CUtil::GetExtension(strFileName);
  strExt.ToLower();
  if ( strcmpi(strExt, ".pls") == 0) bShoutCast = true;

  Clear();
  m_strPlayListName = CUtil::GetFileName(strFileName);
  CUtil::GetParentPath(strFileName, strBasePath);
  CFile file;
  if (!file.Open(strFileName, false) )
  {
    file.Close();
    return false;
  }

  char szLine[4096];
  if ( !file.ReadString(szLine, sizeof(szLine) ) )
  {
    file.Close();
    return false;
  }
  CStdString strLine = szLine;
  CUtil::RemoveCRLF(strLine);
  if (strLine != START_PLAYLIST_MARKER)
  {
    CFileItem item(strLine, false);
    CURL url(strLine);
    CStdString strProtocol = url.GetProtocol();
    if (item.IsInternetStream())
    {
      if (bShoutCast && !item.IsAudio())
      {
        strLine.Replace("http:", "shout:");
      }

      if (!bShoutCast && strProtocol == "http")
      {
        if (LoadFromWeb(strLine))
        {
          file.Close();
          return true;
        }
      }
      CPlayListItem newItem(strLine, strLine, 0);
      Add(newItem);

      file.Close();
      return true;
    }
    file.Close();
    return false;
  }

  int iMaxSize = 0;
  while (file.ReadString(szLine, sizeof(szLine) ) )
  {
    strLine = szLine;
    CUtil::RemoveCRLF(strLine);
    int iPosEqual = strLine.Find("=");
    if (iPosEqual > 0)
    {
      CStdString strLeft = strLine.Left(iPosEqual);
      iPosEqual++;
      CStdString strValue = strLine.Right(strLine.size() - iPosEqual);
      strLeft.ToLower();

      if (strLeft == "numberofentries")
      {
        m_vecItems.reserve(atoi(strValue.c_str()));
      }
      else if (strLeft.Left(4) == "file")
      {
        vector <int>::size_type idx = atoi(strLeft.c_str() + 4);
        if (idx > m_vecItems.size()) {
          iMaxSize = idx;
          m_vecItems.resize(idx);
        }
        if (m_vecItems[idx - 1].GetDescription().empty())
          m_vecItems[idx - 1].SetDescription(CUtil::GetFileName(strValue));
        CFileItem item(strValue, false);
        if (bShoutCast && !item.IsAudio())
          strValue.Replace("http:", "shout:");

        if (CUtil::IsRemote(strBasePath) && g_settings.m_vecPathSubstitutions.size() > 0)
         strValue = CUtil::SubstitutePath(strValue);
        CUtil::GetQualifiedFilename(strBasePath, strValue);
        m_vecItems[idx - 1].SetFileName(strValue);
      }
      else if (strLeft.Left(5) == "title")
      {
        vector <int>::size_type idx = atoi(strLeft.c_str() + 5);
        if (idx > m_vecItems.size()) {
          iMaxSize = idx;
          m_vecItems.resize(idx);
        }
        m_vecItems[idx - 1].SetDescription(strValue);
      }
      else if (strLeft.Left(6) == "length")
      {
        vector <int>::size_type idx = atoi(strLeft.c_str() + 6);
        if (idx > m_vecItems.size()) {
          iMaxSize = idx;
          m_vecItems.resize(idx);
        }
        m_vecItems[idx - 1].SetDuration(atol(strValue.c_str()));
      }
      else if (strLeft == "playlistname")
      {
        m_strPlayListName = strValue;
      }
    }
  }
  file.Close();
  m_vecItems.resize(iMaxSize);

  // check for missing entries
  for (ivecItems p = m_vecItems.begin(); p != m_vecItems.end(); ++p)
  {
    while (p->GetFileName().empty() && p != m_vecItems.end())
    {
      p = m_vecItems.erase(p);
    }
  }

  return true;
}

bool CPlayListPLS::LoadFromWeb(CStdString& strURL)
{
  CLog::Log(LOGINFO, "Loading web playlist %s", strURL.c_str());

  CHTTP httpUtil;
  CStdString strData;

  // Load up the page's header from the internet.  It is necessary to
  // load the header only, as some HTTP link will be to actual streams
  if (!httpUtil.Head(strURL))
  {
    CLog::Log(LOGNOTICE, "URL %s not found", strURL.c_str());
    return false;
  }

  CStdString strContentType;
  if (!httpUtil.GetHeader("Content-Type", strContentType))
  {
    // Unable to deterine Content-Type
    CLog::Log(LOGNOTICE, "No content type for URL %s", strURL.c_str());
    return false;
  }

  CLog::Log(LOGINFO, "Content-Type is %s", strContentType.c_str());
  if ((strContentType == "video/x-ms-asf") || (strContentType == "video/x-ms-asx"))
  {
    httpUtil.Get(strURL, strData);
    return LoadAsxInfo(strData);
  }
  if (strContentType == "audio/x-pn-realaudio")
  {
    httpUtil.Get(strURL, strData);
    return LoadRAMInfo(strData);
  }

  // Unknown type
  return false;
}

bool CPlayListPLS::LoadAsxInfo(CStdString& strData)
{
  CLog::Log(LOGNOTICE, "Parsing ASX");

  // Check for [Reference] format first
  if (strData[0] == '[')
  {
    return LoadAsxIniInfo(strData);
  }
  else
  {
    // Parse XML format
    // Now load the XML file
    TiXmlDocument xmlDoc;
    xmlDoc.Parse(strData.c_str());
    if (xmlDoc.Error())
    {
      CLog::Log(LOGERROR, "Unable to parse ASX info from XML:\n%s\nError: %s", strData.c_str(), xmlDoc.ErrorDesc());
      return false;
    }

    TiXmlElement *pRootElement = xmlDoc.RootElement();
    //TODO: can timy xml parse case-insensitive?
    TiXmlElement *pElement = pRootElement->FirstChildElement("entry");
    if (!pElement) pElement = pRootElement->FirstChildElement("Entry");
    if (!pElement) pElement = pRootElement->FirstChildElement("ENTRY");
    if (!pElement)
    { // no entry, let's see if there's any entryref's
      pElement = pRootElement->FirstChildElement("entryref");
      if (!pElement) pElement = pRootElement->FirstChildElement("Entryref");
      if (!pElement) pElement = pRootElement->FirstChildElement("EntryRef");
      if (!pElement) pElement = pRootElement->FirstChildElement("ENTRYREF");
      while (pElement)
      {
        CStdString strRef = pElement->Attribute("href");
        if (strRef == "") strRef = pElement->Attribute("Href");
        if (strRef == "") strRef = pElement->Attribute("HRef");
        if (strRef == "") strRef = pElement->Attribute("HREF");

        if (strRef != "")
        { // found an entryref, let's try loading that url
          LoadFromWeb(strRef);
        }
        pElement = pElement->NextSiblingElement();
      }
    }
    else while (pElement)
      {
        TiXmlElement *pRef = pElement->FirstChildElement("ref");
        if (!pRef) pRef = pElement->FirstChildElement("Ref");
        if (!pRef) pRef = pElement->FirstChildElement("REF");
        if (pRef)
        {
          CStdString strMMS = pRef->Attribute("href");
          if (strMMS == "") strMMS = pRef->Attribute("Href");
          if (strMMS == "") strMMS = pRef->Attribute("HRef");
          if (strMMS == "") strMMS = pRef->Attribute("HREF");

          if (strMMS != "")
          {
            CLog::Log(LOGINFO, "Adding element %s", strMMS.c_str());
            CPlayListItem newItem(strMMS, strMMS, 0);
            Add(newItem);
          }
        }
        pElement = pElement->NextSiblingElement();
      }
  }

  return true;
}

bool CPlayListPLS::LoadAsxIniInfo(CStdString& strData)
{
  CLog::Log(LOGINFO, "Parsing INI style ASX");
  string::size_type equals = 0, end = 0;
  CStdString strMMS;

  while ((equals = strData.find('=', end)) != string::npos)
  {
    end = strData.find('\r', equals);
    if (end == string::npos)
    {
      strMMS = strData.substr(equals + 1);
    }
    else
    {
      strMMS = strData.substr(equals + 1, end - equals - 1);
    }
    CLog::Log(LOGINFO, "Adding element %s", strMMS.c_str());
    CPlayListItem newItem(strMMS, strMMS, 0);
    Add(newItem);
  }
  return true;
}

bool CPlayListPLS::LoadRAMInfo(CStdString& strData)
{
  CLog::Log(LOGINFO, "Parsing RAM");
  CLog::Log(LOGDEBUG, "%s", strData.c_str());
  CStdString strMMS;

  strMMS = strData.substr(0, strData.Find('\n'));
  CLog::Log(LOGINFO, "Adding element %s", strMMS.c_str());
  CPlayListItem newItem(strMMS, strMMS, 0);
  Add(newItem);

  return true;
}

void CPlayListPLS::Save(const CStdString& strFileName) const
{
  if (!m_vecItems.size()) return ;
  CStdString strPlaylist = strFileName;
  // force HD saved playlists into fatx compliance
  if (CUtil::IsHD(strPlaylist))
    CUtil::GetFatXQualifiedPath(strPlaylist);
  FILE *fd = fopen(strPlaylist.c_str(), "w+");
  if (!fd)
  {
    CLog::Log(LOGERROR, "Could not save PLS playlist: [%s]", strPlaylist.c_str());
    return ;
  }
  fprintf(fd, "%s\n", START_PLAYLIST_MARKER);
  fprintf(fd, "PlaylistName=%s\n", m_strPlayListName.c_str() );

  for (int i = 0; i < (int)m_vecItems.size(); ++i)
  {
    const CPlayListItem& item = m_vecItems[i];
    fprintf(fd, "File%i=%s\n", i + 1, item.GetFileName().c_str() );
    fprintf(fd, "Title%i=%s\n", i + 1, item.GetDescription().c_str() );
    fprintf(fd, "Length%i=%i\n", i + 1, item.GetDuration() / 1000 );
  }

  fprintf(fd, "NumberOfEntries=%i\n", m_vecItems.size());
  fprintf(fd, "Version=2\n");
  fclose(fd);
}
