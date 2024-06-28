/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUITextureJobManager.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "windowing/WinSystem.h"

using namespace std::chrono_literals;

using namespace KODI::GUILIB;

CGUITextureLoaderThread::CGUITextureLoaderThread(CGUITextureJobManager* manager,
                                                 unsigned int threadID)
  : CThread("TextureLoader")
{
  m_manager = manager;
  m_threadID = threadID;
  Create();
  SetPriority(ThreadPriority::LOWEST);
}

void CGUITextureLoaderThread::Process()
{
  bool hasContext = false;

  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiAsyncTextureUpload)
    hasContext = CServiceBroker::GetWinSystem()->BindSecondaryGPUContext(m_threadID);

  while (!m_bStop)
  {
    CImageLoader* image = m_manager->GetNextImage();
    if (image)
    {
      image->DoWork(hasContext);
      image->m_callback->OnLoadComplete(image);
    }
    else
    {
      CThread::Sleep(THREAD_SLEEP_DURATION);
    }
  }
}

CGUITextureJobManager::CGUITextureJobManager()
{
  std::unique_lock<CCriticalSection> lock(m_section);

  uint32_t maxThreads =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_guiTextureThreads;
  for (uint32_t i = 0; i < maxThreads; i++)
    m_textureThread.push_back(new CGUITextureLoaderThread(this, i));
}

CGUITextureJobManager::~CGUITextureJobManager()
{
  std::unique_lock<CCriticalSection> lock(m_section);
  for (const auto& thread : m_textureThread)
    delete thread;
}

unsigned int CGUITextureJobManager::AddImageToQueue(CImageLoader* image)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  image->m_imageID = m_imageIDCounter;

  m_imageQueue.push_back(image);

  return m_imageIDCounter++;
}

void CGUITextureJobManager::CancelImageLoad(const unsigned int& imageID)
{
  std::unique_lock<CCriticalSection> lock(m_section);

  for (size_t i = 0; i < m_imageQueue.size(); i++)
  {
    if (m_imageQueue[i] && m_imageQueue[i]->m_imageID == imageID)
    {
      delete m_imageQueue[i];
      m_imageQueue[i] = nullptr;
      return;
    }
  }
}

CImageLoader* CGUITextureJobManager::GetNextImage()
{
  std::unique_lock<CCriticalSection> lock(m_section);

  while (!m_imageQueue.empty())
  {
    CImageLoader* image = m_imageQueue.front();
    m_imageQueue.pop_front();

    if (image)
      return image;
  }

  return nullptr;
}
