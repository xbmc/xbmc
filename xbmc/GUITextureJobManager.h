/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUILargeTextureManager.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <deque>

using namespace std::chrono_literals;

namespace KODI::GUILIB
{

class CGUITextureJobManager;
class CGUITextureLoaderThread : protected CThread
{
public:
  CGUITextureLoaderThread(CGUITextureJobManager* manager, unsigned int threadID);

protected:
  void Process() override;

private:
  static constexpr auto THREAD_SLEEP_DURATION{16ms};
  unsigned int m_threadID;
  CGUITextureJobManager* m_manager{nullptr};
};

class CGUITextureJobManager
{
public:
  CGUITextureJobManager();
  ~CGUITextureJobManager();

  /*!
   \brief Adds a image to the processing queue.

   \param image CImageLoader object which should be processed
   \return the ID of the added image
   */
  unsigned int AddImageToQueue(CImageLoader* image);
  /*!
   \brief Cancels a queued image. Images in flight can't be canceled.

   \param imageID image to cancel
   */
  void CancelImageLoad(const unsigned int& imageID);
  /*!
   \brief Gets a image next in queue.

   \return CImageLoader object 
   */
  CImageLoader* GetNextImage();

private:
  mutable CCriticalSection m_section;
  unsigned int m_imageIDCounter;
  std::vector<CGUITextureLoaderThread*> m_textureThread;
  std::deque<CImageLoader*> m_imageQueue{};
};

} // namespace KODI::GUILIB
