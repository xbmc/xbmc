#include "stdafx.h"
#include "MusicInfoScanner.h"
#include "musicdatabase.h"
#include "musicInfoTagLoaderFactory.h"
#include "FileSystem/DirectoryCache.h"
#include "Util.h"


using namespace MUSIC_INFO;
using namespace DIRECTORY;

CMusicInfoScanner::CMusicInfoScanner()
{
  m_bRunning = false;
  m_pObserver = NULL;
  m_bCanInterrupt = false;
  m_currentItem=0;
  m_itemCount=0;
}

CMusicInfoScanner::~CMusicInfoScanner()
{
}

void OutputDebugMem(const char *comment)
{
  MEMORYSTATUS stat;
  GlobalMemoryStatus(&stat);
  CLog::Log(LOGDEBUG, "Mem: %d, %s", stat.dwAvailPhys, comment);
}

void CMusicInfoScanner::Process()
{
  try
  {
    DWORD dwTick = timeGetTime();

    m_musicDatabase.Open();

    if (m_pObserver)
      m_pObserver->OnStateChanged(PREPARING);

    m_bCanInterrupt = true;

    // check whether we have scanned here before
    CStdString strPaths;
    if (!m_musicDatabase.GetSubpathsFromPath(m_strStartDir, strPaths))
    {
      m_musicDatabase.Close();
      return ;
    }

    CUtil::ThumbCacheClear();
    g_directoryCache.ClearMusicThumbCache();

    OutputDebugMem("Starting scan");
    m_musicDatabase.BeginTransaction();

    bool bOKtoScan = true;
    if (m_bUpdateAll)
    {
      if (m_pObserver)
        m_pObserver->OnStateChanged(REMOVING_OLD);

      bOKtoScan = m_musicDatabase.RemoveSongsFromPaths(strPaths);
    }

    if (bOKtoScan)
    {
      if (m_bUpdateAll)
      {
        if (m_pObserver)
          m_pObserver->OnStateChanged(CLEANING_UP_DATABASE);

        bOKtoScan = m_musicDatabase.CleanupAlbumsArtistsGenres(strPaths);
      }

      if (m_pObserver)
        m_pObserver->OnStateChanged(READING_MUSIC_INFO);

      // Reset progress vars
      m_currentItem=0;
      m_itemCount=-1;

      // Create the thread to count all files to be scanned
      CThread fileCountReader(this);
      if (m_pObserver)
        fileCountReader.Create();

      // Database operations should not be canceled
      // using Interupt() while scanning as it could
      // result in unexpected behaviour.
      m_bCanInterrupt = false;

      OutputDebugMem("Loaded filecount reader, starting scan");
      bool bCommit = false;
      if (bOKtoScan)
        bCommit = DoScan(m_strStartDir);

      if (bCommit)
      {
        m_musicDatabase.CommitTransaction();

        if (m_bUpdateAll)
        {
          if (m_pObserver)
            m_pObserver->OnStateChanged(COMPRESSING_DATABASE);

          m_musicDatabase.Compress();
        }
      }
      else
        m_musicDatabase.RollbackTransaction();

      fileCountReader.StopThread();

    }
    else
      m_musicDatabase.RollbackTransaction();

    OutputDebugMem("Finished, emptying database cache");
    m_musicDatabase.EmptyCache();

    OutputDebugMem("Finished, emptying thumb cache");
    CUtil::ThumbCacheClear();
    OutputDebugMem("Finished, emptying music thumb cache");
    g_directoryCache.ClearMusicThumbCache();
    OutputDebugMem("Finished, closing database");

    m_musicDatabase.Close();
    OutputDebugMem("Finished");

    dwTick = timeGetTime() - dwTick;
    CStdString strTmp, strTmp1;
    CUtil::SecondsToHMSString(dwTick / 1000, strTmp1);
    strTmp.Format("My Music: Scanning for music info using worker thread, operation took %s", strTmp1);
    CLog::Log(LOGNOTICE, strTmp.c_str());

    m_bRunning = false;
    if (m_pObserver)
      m_pObserver->OnFinished();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "MusicInfoScanner: Exception will scanning.");
  }
}

void CMusicInfoScanner::Start(const CStdString& strDirectory, bool bUpdateAll)
{
  m_strStartDir = strDirectory;
  m_bUpdateAll = bUpdateAll;
  StopThread();
  Create();
  m_bRunning = true;
}

bool CMusicInfoScanner::IsScanning()
{
  return m_bRunning;
}

void CMusicInfoScanner::Stop()
{
  if (m_bCanInterrupt)
    m_musicDatabase.Interupt();

  StopThread();
}

void CMusicInfoScanner::SetObserver(IMusicInfoScannerObserver* pObserver)
{
  m_pObserver = pObserver;
}

bool CMusicInfoScanner::DoScan(const CStdString& strDirectory)
{
  if (m_pObserver)
    m_pObserver->OnDirectoryChanged(strDirectory);

  CStdString format;
  format.Format("Scanning dir: %s", strDirectory.c_str());
  OutputDebugMem(format.c_str());
  // load subfolder
  CFileItemList items;
  CDirectory::GetDirectory(strDirectory, items, g_stSettings.m_szMyMusicExtensions);
  // filter items in the sub dir (for .cue sheet support)
  items.FilterCueItems();
  items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

  if (RetrieveMusicInfo(items, strDirectory) > 0)
  {
    m_musicDatabase.CheckVariousArtistsAndCoverArt();

    if (m_pObserver)
      m_pObserver->OnDirectoryScanned(strDirectory);
  }
  format.Format("Finished scanning dir: %s", strDirectory.c_str());
  OutputDebugMem(format.c_str());

  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem *pItem = items[i];

    if (m_bStop)
      break;

    if (pItem->m_bIsFolder && !pItem->IsParentFolder())
    {
      // get the item's thumb (this will cache the album thumb)
      pItem->SetMusicThumb();
      CStdString strPath=pItem->m_strPath;
      if (!DoScan(strPath))
      {
        m_bStop = true;
      }
    }
  }

  return !m_bStop;
}

int CMusicInfoScanner::RetrieveMusicInfo(CFileItemList& items, const CStdString& strDirectory)
{
  int nFolderCount = items.GetFolderCount();
  // Skip items with folders only
  if (nFolderCount == items.Size())
    return 0;

  int nFileCount = items.Size() - nFolderCount;

  CStdString strItem;
  CSongMap songsMap;
  // get all information for all files in current directory from database
  m_musicDatabase.GetSongsByPath(strDirectory, songsMap);

  int iFilesAdded = 0;
  // for every file found, but skip folder
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItem* pItem = items[i];
    CStdString strExtension;
    CUtil::GetExtension(pItem->m_strPath, strExtension);

    if (m_bStop)
      return iFilesAdded;

    // dont try reading id3tags for folders, playlists or shoutcast streams
    if (!pItem->m_bIsFolder && !pItem->IsPlayList() && !pItem->IsShoutCast() )
    {
      m_currentItem++;
      // is tag for this file already loaded?
      bool bNewFile = false;
      CMusicInfoTag& tag = pItem->m_musicInfoTag;
      if (!tag.Loaded() )
      {
        // no, then we gonna load it.
        // first search for file in our list of the current directory
        CSong *song = songsMap.Find(pItem->m_strPath);
        if (song)
        {
          tag.SetSong(*song);
        }
        else
        {
          // if id3 tag scanning is turned on OR we're scanning the directory
          // then parse id3tag from file
          // get correct tag parser
          CMusicInfoTagLoaderFactory factory;
          auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(pItem->m_strPath));
          if (NULL != pLoader.get())
          {
            // get id3tag
            if ( pLoader->Load(pItem->m_strPath, tag))
            {
              bNewFile = true;
            }
          }
        }
      } //if (!tag.Loaded() )
      else
      {
        CSong *song = songsMap.Find(pItem->m_strPath);
        if (!song)
          bNewFile = true;
      }

      // if we have the itemcount, notify our 
      // observer with the progress we made
      if (m_pObserver && m_itemCount>0)
        m_pObserver->OnSetProgress(m_currentItem, m_itemCount);

      if (tag.Loaded() && bNewFile)
      {
        CSong song(tag);
        song.iStartOffset = pItem->m_lStartOffset;
        song.iEndOffset = pItem->m_lEndOffset;
        pItem->SetMusicThumb();
        song.strThumb = pItem->GetThumbnailImage();
        m_musicDatabase.AddSong(song, false);
        iFilesAdded++;
      }
    } //if (!pItem->m_bIsFolder)
  }

  return iFilesAdded;
}

// This function is run by another thread
void CMusicInfoScanner::Run()
{
  m_itemCount=CountFiles(m_strStartDir);
}

// Recurse through all folders we scan and count files
int CMusicInfoScanner::CountFiles(const CStdString& strPath)
{
  int count=0;
  // load subfolder
  CFileItemList items;
  CDirectory::GetDirectory(strPath, items, g_stSettings.m_szMyMusicExtensions, false);
  for (int i=0; i<items.Size(); ++i)
  {
    CFileItem* pItem=items[i];

    if (m_bStop)
      return 0;

    if (pItem->m_bIsFolder)
      count+=CountFiles(pItem->m_strPath);
    else if (pItem->IsAudio() && !pItem->IsPlayList() && !pItem->IsNFO())
      count++;
  }

  return count;
}
