/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "BackgroundInfoLoader.h"

CBackgroundInfoLoader::CBackgroundInfoLoader()
{
  m_bRunning = false;
  m_pObserver=NULL;
  m_pProgressCallback=NULL;
}

CBackgroundInfoLoader::~CBackgroundInfoLoader()
{
}

void CBackgroundInfoLoader::OnStartup()
{
  SetPriority( THREAD_PRIORITY_LOWEST );
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

      // Ask the callback if we should abort
      if (m_pProgressCallback && m_pProgressCallback->Abort())
        m_bStop=true;

      if (m_bStop)
        break;

      // load the item
      if (!LoadItem(pItem))
        continue;

      // Notify observer a item
      // is loaded.
      if (m_pObserver)
        m_pObserver->OnItemLoaded(pItem);
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
  m_bRunning = true;
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

void CBackgroundInfoLoader::SetProgressCallback(IProgressCallback* pCallback)
{
  m_pProgressCallback = pCallback;
}