/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ThumbLoader.h"
#include "utils/JobManager.h"

class CPictureThumbLoader : public CThumbLoader, public CJobQueue
{
public:
  CPictureThumbLoader();
  ~CPictureThumbLoader() override;

  bool LoadItem(CFileItem* pItem) override;
  bool LoadItemCached(CFileItem* pItem) override;
  bool LoadItemLookup(CFileItem* pItem) override;
  void SetRegenerateThumbs(bool regenerate) { m_regenerateThumbs = regenerate; };
  static void ProcessFoldersAndArchives(CFileItem *pItem);

  /*!
   \brief Callback from CThumbExtractor on completion of a generated image

   Performs the callbacks and updates the GUI.

   \sa CImageLoader, IJobCallback
   */
  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;

protected:
  void OnLoaderFinish() override;

private:
  bool m_regenerateThumbs;
};
