
#include "../stdafx.h"
#include "shoutcastdirectory.h"
#include "directorycache.h"
#include "../util.h"
#include "../PlayListFactory.h"
#include "FileCurl.h"
#include "../utils/HttpHeader.h"

/* small dummy class to be able to get headers simply */
class CDummyHeaders : public IHttpHeaderCallback
{
public:
  CDummyHeaders(CHttpHeader *headers)
  {
    m_headers = headers;
  }

  virtual void ParseHeaderData(CStdString strData)
  {
    m_headers->Parse(strData);
  }
  CHttpHeader *m_headers;
};


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

  CStdString genre, path;
  while(element != NULL)
  {
    genre = element->Attribute("name");

    url.SetOptions("genre=" + genre);
    url.GetURL(path);


    CFileItem* pItem = new CFileItem;
    pItem->m_bIsFolder = true;
    pItem->SetLabelPreformated(true);
    pItem->SetLabel(genre);
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

  while(element != NULL)
  {
    CStdString name = element->Attribute("name");
    CStdString id = element->Attribute("id");
    CStdString bitrate = element->Attribute("br");
    CStdString genre = element->Attribute("genre");

    CStdString label = name;
    CStdString label2;
    label2.Format("%s kbps", bitrate);

    url.SetOptions("id=" + id);
    url.GetURL(path);

    CFileItem* pItem = new CFileItem;
    pItem->m_bIsFolder = false;
    pItem->SetLabelPreformated(true);
        
    pItem->SetLabel(label);
    pItem->SetLabel2(label2);

    pItem->m_strPath = path;
    
    items.Add(pItem);

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


  CHttpHeader headers;
  CDummyHeaders dummy(&headers);
  http.SetHttpHeaderCallback(&dummy);

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

  CStdString content = headers.GetContentType();
  if( !(content.Equals("text/html") || content.Equals("text/xml")) )
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Invalid content type %s", headers.GetContentType().c_str());
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
      dlgProgress->StartModal(m_gWindowManager.GetActiveWindow());
      dlgProgress->Progress();
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
  
  /* TODO: clear string data here */


  TiXmlElement *root = doc.RootElement();
  if(root == NULL)
  {
    CLog::Log(LOGERROR, __FUNCTION__" - Unable to parse xml");
    dlgProgress->Close();
    return false;
  }

  bool result;
  if( strcmp(root->Value(), "genrelist") == 0 )
    result = ParseGenres(root, items, url);
  else if( strcmp(root->Value(), "stationlist") == 0 )
    result = ParseStations(root, items, url);

  CFileItemList vecCacheItems;  
  g_directoryCache.ClearDirectory(strRoot);
  for( int i = 0; i <items.Size(); i++ )
  {
    vecCacheItems.Add(new CFileItem( *items.Get(i) ));
  }
  g_directoryCache.SetDirectory(strRoot, vecCacheItems);
  if (dlgProgress) dlgProgress->Close();
  return result;
}
