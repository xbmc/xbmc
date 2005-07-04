#include "stdafx.h"
#include "BackgroundInfoLoader.h"

CBackgroundInfoLoader::CBackgroundInfoLoader()
{
  m_bRunning = false;
}

CBackgroundInfoLoader::~CBackgroundInfoLoader()
{
}

void CBackgroundInfoLoader::OnStartup()
{
  m_bRunning = true;
}

void CBackgroundInfoLoader::Process()
{
  try
  {
    CFileItemList& vecItems = (*m_pVecItems);

    if (vecItems.Size() <= 0)
      return ;

    OnLoaderStart();

    for (int i = 0; i < (int)vecItems.Size(); ++i)
    {
      CFileItem* pItem = vecItems[i];

      if (m_bStop)
        break;

      // Fill in tag for the item
      if (!LoadItem(pItem))
        continue;

      // Notify observer a item
      // is loaded.
      if (m_pObserver)
      {
        m_pObserver->OnItemLoaded(pItem);
      }
    }

    OnLoaderFinish();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "BackgroundInfoLoader thread: Unhandled exception");
  }
}

void CBackgroundInfoLoader::OnExit()
{
  m_bRunning = false;
}

void CBackgroundInfoLoader::Load(CFileItemList& items)
{
  m_pVecItems = &items;
  StopThread();
  Create();
}

bool CBackgroundInfoLoader::IsLoading()
{
  return m_bRunning;
}

void CBackgroundInfoLoader::SetObserver(IBackgroundLoaderObserver* pObserver)
{
  m_pObserver = pObserver;
}
