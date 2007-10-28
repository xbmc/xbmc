
#include "stdafx.h"
#include "shoutcastdirectory.h"
#include "directorycache.h"
#include "../util.h"
#include "FileCurl.h"
#include "../utils/HttpHeader.h"

using namespace XFILE;
using namespace DIRECTORY;

CShoutcastDirectory::CShoutcastDirectory(void)
{
}

CShoutcastDirectory::~CShoutcastDirectory(void)
{
}

bool CShoutcastDirectory::ParseGenres(TiXmlElement *root, CFileItemList &items, CURL &url)
{
  TiXmlElement *element = root->FirstChildElement("genre");
  
  if(element == NULL)
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - No genres found");
    return false;
  }
    
  items.m_idepth = 1; /* genre list */

  CStdString genre, path;
  while(element != NULL)
  {
    genre = element->Attribute("name");
    path = genre;

    /* genre must be urlencoded */
    CUtil::URLEncode(path);

    url.SetOptions("?genre=" + path);
    url.GetURL(path);


    CFileItem* pItem = new CFileItem;
    pItem->m_bIsFolder = true;
    pItem->SetLabel(genre);
    pItem->GetMusicInfoTag()->SetGenre(genre);
    pItem->m_strPath = path;  
    
    items.Add(pItem);

    element = element->NextSiblingElement("genre");
  }

  return true;
}

bool CShoutcastDirectory::ParseStations(TiXmlElement *root, CFileItemList &items, CURL &url)
{
  TiXmlElement *element = NULL;
  CStdString path;

  items.m_idepth = 2; /* station list */

  element = root->FirstChildElement("tunein");
  if(element == NULL) 
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - No tunein base found");
    return false;
  }
  
  path = element->Attribute("base");
  path.TrimLeft("/");

  url.SetFileName(path);

  element = root->FirstChildElement("station");

  if(element == NULL)
  {
    CLog::Log(LOGWARNING, __FUNCTION__" - No stations found");
    return false;
  }
  int stations = 0;
  while(element != NULL && stations < 1000)
  {
    CStdString name = element->Attribute("name");
    CStdString id = element->Attribute("id");
    CStdString bitrate = element->Attribute("br");
    CStdString genre = element->Attribute("genre");
    CStdString listeners = element->Attribute("lc");
    CStdString content = element->Attribute("mt");

    CStdString label = name;

    url.SetOptions("?id=" + id);
    url.GetURL(path);

    CFileItem* pItem = new CFileItem;
    pItem->m_bIsFolder = false;
    
    /* we highjack the music tag for this stuff, they will be used by */
    /* viewstates to sort and display proper information */
    pItem->GetMusicInfoTag()->SetArtist(listeners);
    pItem->GetMusicInfoTag()->SetAlbum(bitrate);
    pItem->GetMusicInfoTag()->SetGenre(genre);

    /* this is what will be sorted upon */
    pItem->GetVideoInfoTag()->m_fRating = (float)atoi(listeners.c_str());
    pItem->m_dwSize = atoi(bitrate.c_str());


    pItem->SetLabel(label);

    /* content type is known before hand, should save later lookup */
    /* wonder if we could combine the contentype of the playlist and of the real stream */
    pItem->SetContentType("audio/x-scpls");

    pItem->m_strPath = path;
    
    items.Add(pItem);

    stations++;
    element = element->NextSiblingElement("station");
  }

  return true;
}

bool CShoutcastDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{

  CStdString strRoot = strPath;
  if (CUtil::HasSlashAtEnd(strRoot))
    strRoot.Delete(strRoot.size() - 1);

  /* for old users wich doesn't have the full url */
  if( strRoot.Equals("shout://www.shoutcast.com") )
    strRoot = "shout://www.shoutcast.com/sbin/newxml.phtml";

  if (g_directoryCache.GetDirectory(strRoot, items))
    return true;

  /* display progress dialog after 2 seconds */
  DWORD dwTimeStamp = GetTickCount() + 2000;

  CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  bool dialogopen = false;
  if (dlgProgress)
  {
    dlgProgress->ShowProgressBar(false);
    dlgProgress->SetHeading(260);
    dlgProgress->SetLine(0, 14003);
    dlgProgress->SetLine(1, "");
    dlgProgress->SetLine(2, "");
  }

  CURL url(strRoot);
  CStdString protocol = url.GetProtocol();
  url.SetProtocol("http");

  CFileCurl http;

  //CURL doesn't seem to understand that data is encoded.. odd
  // opening as text for now
  //http.SetContentEncoding("deflate");

  if( !http.Open(url, false) ) 
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unable to get shoutcast dir");
    if (dlgProgress) dlgProgress->Close();
    return false;
  }

  /* restore protocol */
  url.SetProtocol(protocol);

  CStdString content = http.GetContent();
  if( !(content.Equals("text/html") || content.Equals("text/xml") 
	  || content.Equals("text/html;charset=utf-8") || content.Equals("text/xml;charset=utf-8") ))
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Invalid content type %s", content.c_str());
    if (dlgProgress) dlgProgress->Close();
    return false;
  }
  
  
  int size_read = 0;  
  int size_total = (int)http.GetLength();
  int data_size = 0;

  CStdString data;
  data.reserve(size_total);
  
  /* read response from server into string buffer */
  char buffer[16384];
  while( (size_read = http.Read(buffer, sizeof(buffer)-1)) > 0 )
  {
    buffer[size_read] = 0;
    data += buffer;
    data_size += size_read;

    if( dialogopen )
    {
      dlgProgress->Progress();
    }
    else if( GetTickCount() > dwTimeStamp )
    {
      dlgProgress->StartModal();
      dlgProgress->Progress();
      dialogopen = true;
    }

    if (dlgProgress->IsCanceled())
    {
      dlgProgress->Close();
      return false;
    }
  }

  /* parse returned xml */
  TiXmlDocument doc;
  doc.Parse(data.c_str());

  TiXmlElement *root = doc.RootElement();
  if(root == NULL)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unable to parse xml");
    CLog::Log(LOGDEBUG, __FUNCTION__" - Sample follows...\n%s", data.c_str());

    dlgProgress->Close();
    return false;
  }

  /* clear data to keep memusage down, not needed anymore */
  data.Empty();

  bool result;
  if( strcmp(root->Value(), "genrelist") == 0 )
    result = ParseGenres(root, items, url);
  else if( strcmp(root->Value(), "stationlist") == 0 )
    result = ParseStations(root, items, url);
  else
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Invalid root xml element for shoutcast");
    CLog::Log(LOGDEBUG, __FUNCTION__" - Sample follows...\n%s", data.c_str);
  }

  CFileItemList vecCacheItems;  
  g_directoryCache.ClearDirectory(strRoot);
  for( int i = 0; i <items.Size(); i++ )
  {
    CFileItem* pItem=items[i];
    if (!pItem->IsParentFolder())
      vecCacheItems.Add(new CFileItem( *pItem ));
  }
  g_directoryCache.SetDirectory(strRoot, vecCacheItems);
  if (dlgProgress) dlgProgress->Close();
  return result;
}
