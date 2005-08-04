
#include "../stdafx.h"
#include "shoutcastdirectory.h"
#include "directorycache.h"
#include "../lib/libid3/zlib.h"
#include "../util.h"
#include "../PlayListFactory.h"


// Quoting shoutcast on how often users could update their streams list:
// "You can either choose to cache this data on your own server,
// or limit the number of requests from your clients to us.
// We ask that a client not make a request for the XML list more
// frequently than once per 5 minutes. If we see excessive requests
// coming it, we disable per service or per IP."
//
// So this cache will be refreshed every 30 min for now.
#define CACHE_VALID_TIME 30.0f*60.0f

CShoutcastDirectory::CShoutcastDirectory(void)
{
  m_Downloaded=false;
  m_Error=false;
}

CShoutcastDirectory::~CShoutcastDirectory(void)
{
}

bool CShoutcastDirectory::IsCacheValid()
{
  __stat64 stat;
  if (CFile::Stat("Z:\\cachedPlaylists.fi", &stat) == 0)
  {
    __time64_t time;
    _time64(&time);
    if (difftime((time_t)time, (time_t)stat.st_mtime) < CACHE_VALID_TIME)
      return true;
  }
  return false;
}

void CShoutcastDirectory::CacheItems(CFileItemList &items)
{
  if (items.Size() == 0)
    return ;

  CFile file;
  CArchive ar(&file, CArchive::store);
  if (file.OpenForWrite("Z:\\cachedPlaylists.fi", true, true)) // always overwrite
    ar << items;
}

void CShoutcastDirectory::LoadCachedItems(CFileItemList &items)
{
  CFile file;
  CArchive ar(&file, CArchive::load);
  if (file.Open("Z:\\cachedPlaylists.fi", false))
    ar >> items;
}

bool CShoutcastDirectory::DownloadPlaylists(CFileItemList &items)
{
  CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (dlgProgress)
  {
    dlgProgress->ShowProgressBar(false);
    dlgProgress->SetHeading(260);
    dlgProgress->SetLine(0, 14004);
    dlgProgress->SetLine(1, L"");
    dlgProgress->SetLine(2, L"");
    dlgProgress->StartModal(m_gWindowManager.GetActiveWindow());
    dlgProgress->Progress();
  }

  CThread thread(this);
  m_strSource="http://shoutcast.com/sbin/xmllister.phtml?service=XBMC&limit=500";
  m_strDestination="Z:\\xmllister.zli";
  thread.Create();

  while (!m_Downloaded)
  {
    dlgProgress->Progress();

    if (dlgProgress->IsCanceled())
    {
      m_http.Cancel();
      thread.StopThread();
      dlgProgress->Close();
      return false;
    }
  }

  if (!dlgProgress->IsCanceled() && m_Error)
  {
    if (dlgProgress) dlgProgress->Close();
    CGUIDialogOK::ShowAndGetInput(260, 14006, 0, 0);
    CLog::Log(LOGERROR, "Unable to download playlistfile from shoutcast");
    return false;
  }

  CFile file;
  if (!file.Open("Z:\\xmllister.zli"))
  {
    if (dlgProgress) dlgProgress->Close();
    CGUIDialogOK::ShowAndGetInput(260, 14006, 0, 0);
    CLog::Log(LOGERROR, "Unable to open downloaded file");
    return false;
  }

  uLongf iCompressedLenght = (uLongf)file.GetLength() + 1;
  uLongf iUncompressedLenght = (uLongf)(file.GetLength() * 6) + 1;  // FIXME: Just a guess

  auto_aptr<Bytef> pCompressed(new Bytef[iCompressedLenght]);
  ZeroMemory(pCompressed.get(), iCompressedLenght);
  auto_aptr<Bytef> pUncompressed(new Bytef[iUncompressedLenght]);
  ZeroMemory(pUncompressed.get(), iUncompressedLenght);

  file.Read(pCompressed.get(), iCompressedLenght);
  file.Close();

  //  uncompress downloaded file using zlib
  int iError = 0;
  if ((iError = uncompress OF((pUncompressed.get(), &iUncompressedLenght,
                               pCompressed.get(), iCompressedLenght))) != Z_OK)
  {
    if (dlgProgress) dlgProgress->Close();
    CGUIDialogOK::ShowAndGetInput(260, 14006, 0, 0);
    CLog::Log(LOGERROR, "zlib uncompress returned error %i", iError);
    return false;
  }

  if (dlgProgress)
  {
    dlgProgress->SetLine(0, 14005);
    dlgProgress->Progress();
  }

  TiXmlDocument xmlDoc;
  if (!xmlDoc.Parse((char*)pUncompressed.get()))
  {
    if (dlgProgress) dlgProgress->Close();
    CGUIDialogOK::ShowAndGetInput(260, 14006, 0, 0);
    CLog::Log(LOGERROR, "Error parsing file from shoutcast, Line %d\n%s", xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  //  Is this a WinampXML file
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (pRootElement->Value() == "WinampXML")
  {
    if (dlgProgress) dlgProgress->Close();
    CGUIDialogOK::ShowAndGetInput(260, 14006, 0, 0);
    CLog::Log(LOGERROR, "Shoutcast XML file has no <WinampXML>");
    return false;
  }

  TiXmlElement *pPlaylist = pRootElement->FirstChildElement("playlist");
  if (pPlaylist)
  {
    int iNumEntries = 0;
    pPlaylist->QueryIntAttribute("num_entries", &iNumEntries);

    if (iNumEntries > 0)
    {
      CFileItemList vecCacheItems;

      if (dlgProgress)
      {
        dlgProgress->ShowProgressBar(true);
        dlgProgress->SetPercentage(0);
        dlgProgress->Progress();
      }

      items.Reserve(iNumEntries);

      TiXmlElement* pEntry = pPlaylist->FirstChildElement("entry");
      int i = 1;
      while (pEntry)
      {
        CFileItem* pItem = new CFileItem;
        pItem->m_bIsFolder = true;

        pItem->m_strPath = pEntry->Attribute("Playstring"); //URL to .pls file
        pItem->m_strPath.Replace("http://www.shoutcast.com/sbin/", "shout://www.shoutcast.com/");
        pItem->m_strPath += "/";

        TiXmlNode* pName = pEntry->FirstChild("Name");
        if (pName && !pName->NoChildren())
        {
          CStdString strName;
          strName.Format("%03.3i. %s", i, pName->FirstChild()->Value());
          pItem->SetLabel(strName);
          i++;
        }

        TiXmlNode* pGenre = pEntry->FirstChild("Genre");
        if (pGenre && !pGenre->NoChildren())
        {
          pGenre->FirstChild()->Value();
        }

        TiXmlNode* pNowplaying = pEntry->FirstChild("Nowplaying");
        if (pNowplaying && !pNowplaying->NoChildren())
        {
          pNowplaying->FirstChild()->Value();
        }

        TiXmlNode* pListeners = pEntry->FirstChild("Listeners");
        if (pListeners && !pListeners->NoChildren())
        {
          pListeners->FirstChild()->Value();
        }

        TiXmlNode* pBitrate = pEntry->FirstChild("Bitrate");
        if (pBitrate && !pBitrate->NoChildren())
        {
          pItem->SetLabel2(CStdString(pBitrate->FirstChild()->Value()) + " kbps");
        }

        //  This has to be this ugly way, if we delete the
        //  xml document at once it would take ages.
        //    TiXmlElement* pEntry1=pEntry->NextSiblingElement("entry");
        //        pPlaylist->RemoveChild(pEntry);
        //        pEntry=pEntry1;
        pEntry = pEntry->NextSiblingElement("entry");

        items.Add(pItem);
        vecCacheItems.Add(new CFileItem(*pItem));

        if ((i % 20) == 0)
        {
          dlgProgress->SetPercentage((i*100) / iNumEntries);
          dlgProgress->Progress();
        }
      }
    }
    else
    {
      if (dlgProgress) dlgProgress->Close();
      CGUIDialogOK::ShowAndGetInput(260, 14006, 0, 0);
      CLog::Log(LOGERROR, "Shoutcast XML file has no playlist entries");
      return false;
    }
  }

  if (dlgProgress) dlgProgress->Close();
  CLog::DebugLog("Finished Parsing");
  return true;
}

bool CShoutcastDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString strRoot = strPath;
  if (CUtil::HasSlashAtEnd(strRoot))
    strRoot.Delete(strRoot.size() - 1);

  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strRoot);

  CURL url(strRoot);

  if (url.GetFileName().IsEmpty())
  {
    if (IsCacheValid())
      LoadCachedItems(items);
    else
    {
      DownloadPlaylists(items);
      CLog::DebugLog("Returned from Parsing");
      CacheItems(items);
    }
    for (int i = 0; i < (int)items.Size(); ++i)
    {
      CFileItem* pItem = items[i];
      vecCacheItems.Add(new CFileItem(*pItem));
    }
  }
  else
  {

    CStdString strPlayList = strRoot;
    strPlayList.Replace("shout://www.shoutcast.com/", "http://www.shoutcast.com/sbin/");

    CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (dlgProgress)
    {
      dlgProgress->ShowProgressBar(false);
      dlgProgress->SetHeading(260);
      dlgProgress->SetLine(0, 14003);
      dlgProgress->SetLine(1, "");
      dlgProgress->SetLine(2, "");
      dlgProgress->StartModal(m_gWindowManager.GetActiveWindow());
      dlgProgress->Progress();
    }

    CThread thread(this);
    m_strSource=strPlayList;
    m_strDestination="Z:\\playlist.pls";
    thread.Create();

    while (!m_Downloaded)
    {
      dlgProgress->Progress();

      if (dlgProgress->IsCanceled())
      {
        m_http.Cancel();
        thread.StopThread();
        dlgProgress->Close();
        return false;
      }
    }

    if (!dlgProgress->IsCanceled() && m_Error)
    {
      if (dlgProgress) dlgProgress->Close();
      CGUIDialogOK::ShowAndGetInput(260, 14007, 0, 0);
      CLog::Log(LOGERROR, "Unable to download playlistfile from shoutcast");
      return false;
    }

    CPlayListFactory factory;
    auto_ptr<CPlayList> pPlayList (factory.Create("Z:\\playlist.pls"));
    if ( NULL != pPlayList.get())
    {
      // load it
      if (pPlayList->Load("Z:\\playlist.pls"))
      {
        for (int i = 0; i < pPlayList->size(); ++i)
        {
          const CPlayList::CPlayListItem& pPlayListItem = (*pPlayList)[i];

          CFileItem* pItem = new CFileItem;
          pItem->m_strPath = pPlayListItem.GetFileName();
          pItem->SetLabel(pPlayListItem.GetDescription());
          items.Add(pItem);
          vecCacheItems.Add(new CFileItem(*pItem));
        }
      }
    }
    if (dlgProgress) dlgProgress->Close();
  }

  g_directoryCache.SetDirectory(strRoot, vecCacheItems);
  return true;
}

void CShoutcastDirectory::Run()
{
  if (!m_http.Download(m_strSource, m_strDestination))
      m_Error=true;

  m_Downloaded=true;
}
