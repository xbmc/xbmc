#include "Utility/PlexGlobalCacher.h"
#include "FileSystem/PlexDirectory.h"
#include "PlexApplication.h"
#include "utils/Stopwatch.h"
#include "utils/log.h"
#include "filesystem/File.h"
#include "TextureCache.h"
#include "Client/PlexServerDataLoader.h"
#include "guilib/GUIWindowManager.h"
#include "LocalizeStrings.h"

using namespace XFILE;

CPlexGlobalCacher* CPlexGlobalCacher::m_globalCacher = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexGlobalCacher::CPlexGlobalCacher() : CThread("Plex Global Cacher")
{
  m_continue = true;

  m_dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(2);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexGlobalCacher* CPlexGlobalCacher::GetInstance()
{
  if (!m_globalCacher)
    m_globalCacher = new CPlexGlobalCacher();
  return m_globalCacher;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalCacher::DeleteInstance()
{
  if (m_globalCacher)
    delete m_globalCacher;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalCacher::Start()
{
  CLog::Log(LOGNOTICE, "Global Cache : Creating cacher thread");
  CThread::Create(true);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemPtr CPlexGlobalCacher::PickItem()
{
  CSingleLock lock(m_picklock);

  CFileItemPtr pick = m_listToCache.Get(0);
  m_listToCache.Remove(0);
  return pick;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalCacher::Process()
{
  CStopWatch timer;

  // setup the progress dialog info
  m_dlgProgress->SetHeading(g_localizeStrings.Get(44403));
  m_dlgProgress->StartModal();
  m_dlgProgress->ShowProgressBar(true);
  timer.StartZero();

  m_continue = !m_dlgProgress->IsCanceled();

  // now just process the items
  for (int iSection = 0; iSection < m_Sections->Size() && m_continue; iSection++)
  {
    m_listToCache.Clear();
    ProcessSection(m_Sections->Get(iSection), iSection, m_Sections->Size());
  }

  CLog::Log(LOGNOTICE, "Global Cache : Full operation took %f", timer.GetElapsedSeconds());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalCacher::SetProgress(CStdString& Line1, CStdString& Line2, int percentage)
{
  CStdString progressMsg;

  m_dlgProgress->SetLine(0, Line1);
  m_dlgProgress->SetLine(1, Line2);

  if (percentage > 0)
    progressMsg.Format(g_localizeStrings.Get(44405) + " : %2d%%", percentage);
  else
    progressMsg = "";

  m_dlgProgress->SetLine(2, progressMsg);
  m_dlgProgress->SetPercentage(percentage);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalCacher::ProcessSection(CFileItemPtr Section, int iSection, int TotalSections)
{
  CStdString message1, message2;
  CStopWatch looptimer;

  looptimer.StartZero();

  // display section retrieval info
  message1.Format(g_localizeStrings.Get(44401) + " %d / %d : '%s'", iSection + 1, TotalSections, Section->GetLabel());
  message2.Format(g_localizeStrings.Get(44402) + " '%s'...", Section->GetLabel());
  SetProgress(message1, message2, 0);

  // gets all the data from one section
  CURL url(Section->GetPath());
  PlexUtils::AppendPathToURL(url, "all");
  CPlexDirectory dir;
  dir.GetDirectory(url, m_listToCache);

  // Grab the server Name for this section from the first item
  CStdString ServerName = "<unknown>";
  if (m_listToCache.Size())
  {
    CPlexServerPtr pServer = g_plexApplication.serverManager->FindFromItem(m_listToCache.Get(0));
    if (pServer)
      ServerName = pServer->GetName();

    CLog::Log(LOGNOTICE, "Global Cache : Processed +%d items in '%s' on %s , took %f", m_listToCache.Size(), Section->GetLabel().c_str(), ServerName.c_str(), looptimer.GetElapsedSeconds());
  }

  int itemsToCache = m_listToCache.Size();
  int itemsProcessed = 0;

  // create the workers
  for (int iWorker = 0; iWorker < MAX_CACHE_WORKERS; iWorker++)
  {
    m_pWorkers[iWorker] = new CPlexGlobalCacherWorker(this);
    m_pWorkers[iWorker]->Create(false);
  }

  // update the displayed information on progress dialog while
  // threads are doing the job
  while ((itemsProcessed < itemsToCache) && (m_continue))
  {
    m_continue = !m_dlgProgress->IsCanceled();

    int progress = itemsProcessed * 100 / itemsToCache;
    itemsProcessed = itemsToCache - m_listToCache.Size();

    message1.Format(g_localizeStrings.Get(44403) + " %d / %d : '%s' on '%s' ", iSection, TotalSections, Section->GetLabel(), ServerName);
    message2.Format(g_localizeStrings.Get(44404) + " %d/%d ...", itemsProcessed, itemsToCache);
    SetProgress(message1, message2, progress);

    Sleep(200);
  }

  // stop all the threads if we cancelled it
  if (!m_continue)
  {
    for (int iWorker = 0; iWorker < MAX_CACHE_WORKERS; iWorker++)
    {
      m_pWorkers[iWorker]->StopThread(false);
    }
  }

  // wait for workers to terminate
  for (int iWorker = 0; iWorker < MAX_CACHE_WORKERS; iWorker++)
  {
    while (m_pWorkers[iWorker]->IsRunning())
      Sleep(10);

    delete m_pWorkers[iWorker];
    m_pWorkers[iWorker] = NULL;
  }

  CLog::Log(LOGNOTICE, "Global Cache : Processing section %s took %f", Section->GetLabel().c_str(), looptimer.GetElapsedSeconds());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalCacher::OnExit()
{
  m_dlgProgress->Close();
  m_globalCacher = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexGlobalCacherWorker::Process()
{
  CStdStringArray art;
  art.push_back("smallThumb");
  art.push_back("thumb");
  art.push_back("bigthumb");
  art.push_back("smallPoster");
  art.push_back("poster");
  art.push_back("bigPoster");
  art.push_back("smallGrandparentThumb");
  art.push_back("grandparentThumb");
  art.push_back("bigGrandparentThumb");
  art.push_back("fanart");
  art.push_back("banner");

  CFileItemPtr pItem;
  while ((pItem = m_pCacher->PickItem()))
  {
    BOOST_FOREACH (CStdString artKey, art)
    {
      if (pItem->HasArt(artKey) && !CTextureCache::Get().HasCachedImage(pItem->GetArt(artKey)))
        CTextureCache::Get().CacheImage(pItem->GetArt(artKey));
    }
  }
}
