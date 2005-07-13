
#include "../stdafx.h"
#include "shoutcastdirectory.h"
#include "directorycache.h"
#include "../lib/libid3/zlib.h"
#include "../util.h"
#include "../utils/http.h"
#include "../PlayListFactory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Quoting shoutcast on how ofter users could update their streams list:
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
}

CShoutcastDirectory::~CShoutcastDirectory(void)
{
}

bool CShoutcastDirectory::IsCacheValid()
{
  __stat64 stat;
  if (CFile::Stat("Z:\\cachedPlaylists.txt", &stat) == 0)
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
  if (file.OpenForWrite("Z:\\cachedPlaylists.txt", true, true)) // always overwrite
  {
    int i = 0;
    if (items[0]->GetLabel() == "..")
      i = 1;

    for (i; i < (int)items.Size(); ++i)
    {
      CFileItem* pItem = items[i];

      CStdString strLabel1 = pItem->GetLabel() + "\n";
      file.Write(strLabel1.c_str(), strLabel1.size());

      CStdString strLabel2 = pItem->GetLabel2() + "\n";
      file.Write(strLabel2.c_str(), strLabel2.size());

      CStdString strPath = pItem->m_strPath + "\n";
      file.Write(strPath.c_str(), strPath.size());
    }

    file.Close();
  }
}

void CShoutcastDirectory::LoadCachedItems(CFileItemList &items)
{
  CFile file;
  if (file.Open("Z:\\cachedPlaylists.txt", false))
  {
    for (int i = 0; i < 500; ++i)
    {
      CFileItem* pItem = new CFileItem;

      CStdString strLabel1;
      file.ReadString(strLabel1.GetBuffer(1024), 1024);
      strLabel1.ReleaseBuffer();
      strLabel1.TrimRight("\n");
      pItem->SetLabel(strLabel1);

      CStdString strLabel2;
      file.ReadString(strLabel2.GetBuffer(1024), 1024);
      strLabel2.ReleaseBuffer();
      strLabel2.TrimRight("\n");
      pItem->SetLabel2(strLabel2);

      CStdString strPath;
      file.ReadString(strPath.GetBuffer(1024), 1024);
      strPath.ReleaseBuffer();
      strPath.TrimRight("\n");
      pItem->m_strPath = strPath;

      pItem->m_bIsFolder = true;

      items.Add(pItem);
    }

    file.Close();
  }
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

  CGUIDialogOK* dlgOk = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  if (dlgOk)
  {
    dlgOk->SetHeading(260);
    dlgOk->SetLine(0, 14006);
    dlgOk->SetLine(1, L"");
    dlgOk->SetLine(2, L"");
  }

  CHTTP http;
  if (!http.Download("http://shoutcast.com/sbin/xmllister.phtml?service=XBMC&limit=500", "Z:\\xmllister.zli"))
  {
    if (dlgProgress) dlgProgress->Close();
    if (dlgOk) dlgOk->DoModal(m_gWindowManager.GetActiveWindow());
    CLog::Log(LOGERROR, "Unable to download playlistfile from shoutcast");
    return false;
  }

  CFile file;
  if (!file.Open("Z:\\xmllister.zli"))
  {
    if (dlgProgress) dlgProgress->Close();
    if (dlgOk) dlgOk->DoModal(m_gWindowManager.GetActiveWindow());
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
    if (dlgOk) dlgOk->DoModal(m_gWindowManager.GetActiveWindow());
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
    if (dlgOk) dlgOk->DoModal(m_gWindowManager.GetActiveWindow());
    CLog::Log(LOGERROR, "Error parsing file from shoutcast, Line %d\n%s", xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    return false;
  }

  //  Is this a WinampXML file
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (pRootElement->Value() == "WinampXML")
  {
    if (dlgProgress) dlgProgress->Close();
    if (dlgOk) dlgOk->DoModal(m_gWindowManager.GetActiveWindow());
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
      if (dlgOk) dlgOk->DoModal(m_gWindowManager.GetActiveWindow());
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

    CGUIDialogOK* dlgOk = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
    if (dlgOk)
    {
      dlgOk->SetHeading(260);
      dlgOk->SetLine(0, 14007);
      dlgOk->SetLine(1, L"");
      dlgOk->SetLine(2, L"");
    }

    CHTTP http;
    if (!http.Download(strPlayList, "Z:\\playlist.pls"))
    {
      if (dlgProgress) dlgProgress->Close();
      if (dlgOk) dlgOk->DoModal(m_gWindowManager.GetActiveWindow());
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
