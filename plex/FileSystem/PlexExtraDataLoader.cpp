#include "PlexExtraDataLoader.h"
#include "PlexDirectory.h"
#include "URL.h"
#include "PlexJobs.h"
#include <stdlib.h>
#include "boost/lexical_cast.hpp"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "PlexApplication.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexExtraDataLoader::CPlexExtraDataLoader()
{
  CSingleLock lock(m_itemMapLock);
  m_items = CFileItemListPtr(new CFileItemList());
  m_type = NONE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexExtraDataLoader::loadDataForItem(CFileItemPtr pItem, ExtraDataType type)
{
  if (!pItem)
    return;

  if (!pItem->IsPlexMediaServerLibrary())
    return;
  
  m_path = pItem->GetPath();
  m_type = type;
  CURL url = getItemURL(pItem, type);

  CJobManager::GetInstance().AddJob(new CPlexDirectoryFetchJob(url), this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexExtraDataLoader::getDataForItem(CFileItemPtr pItem, ExtraDataType type)
{
  if (!pItem)
    return false;
  
  if (!pItem->IsPlexMediaServerLibrary())
    return false;

  m_path = pItem->GetPath();
  m_type = type;
  CURL url = getItemURL(pItem, type);

  return g_plexApplication.busy.blockWaitingForJob(new CPlexDirectoryFetchJob(url), this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CURL CPlexExtraDataLoader::getItemURL(CFileItemPtr pItem, CPlexExtraDataLoader::ExtraDataType type)
{
  if (!pItem)
    return CURL();
  
  CURL url(pItem->GetPath());
  
  PlexUtils::AppendPathToURL(url, "extras");
  
  url.SetOptions("");
  if (type != NONE)
    url.SetOption("extratype", boost::lexical_cast<std::string>((int)type));
  
  return url;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexExtraDataLoader::OnJobComplete(unsigned int jobID, bool success, CJob* job)
{
  CSingleLock lock(m_itemMapLock);
  
  // grab the job
  CPlexDirectoryFetchJob* fjob = static_cast<CPlexDirectoryFetchJob*>(job);
  if (!fjob)
    return;

  if (success)
  {
    // store the job list
    m_items = CFileItemListPtr(new CFileItemList());
    m_items->Clear();
    if (fjob->m_items.Size())
      m_items->Copy(fjob->m_items);

    // add the extra types description
    for (int i=0; i< m_items->Size(); i++)
      m_items->Get(i)->SetProperty("extraTypeStr",
                                   g_localizeStrings.Get(44500 + m_items->Get(i)->GetProperty("extraType").asInteger()));

    // send the dataloaded event to listeners
    CLog::Log(LOGDEBUG, "CPlexExtraInfoLoader : job %d succeeded for %s, (%d extra found)", jobID,
              m_path.c_str(), fjob->m_items.Size());
    CGUIMessage msg(GUI_MSG_PLEX_EXTRA_DATA_LOADED, PLEX_EXTRADATA_LOADER, 0, 0, 0);
    g_windowManager.SendThreadMessage(msg);
  }
  else
  {
    CLog::Log(LOGERROR, "CPlexExtraInfoLoader : job %d failed for %s", jobID, m_path.c_str());
  }
}
