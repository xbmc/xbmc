/*
 *  Copyright (C) 2016-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


#pragma once

#include "FileItem.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/VirtualDirectory.h"
#include "music/MusicDatabase.h"
#include "music/MusicDbUrl.h"
#include "threads/Event.h"
#include "utils/JobManager.h"

class CQueueAndPlayUtils
{
public:
  virtual ~CQueueAndPlayUtils() = default;
  bool PlayItem(const CFileItemPtr& itemToPlay, const bool showBusyDialog = true);
  void PlaySong(const CFileItemPtr& itemToPlay);
  bool AddItemToPlayList(const CFileItemPtr& pItem,
                         CFileItemList& queuedItems,
                         bool bShowBusyDialog);
  void AddToPlayList(const CFileItemPtr& pItem, CFileItemList& queuedItems);
  bool GetDirectory(const std::string& strDirectory, CFileItemList& items);
  void FormatAndSort(CFileItemList& items);
  void JobStart();
  void JobFinish();
  void QueueItem(const CFileItemPtr& itemIn, bool first = false, const bool showBusyDialog = true);

private:
  bool GetDirectoryItems(CURL& url, CFileItemList& items, bool useDir);
  void FormatItemLabels(CFileItemList& items, const LABEL_MASKS& labelMasks);
  void SetupShares();
  XFILE::CVirtualDirectory m_rootDir;
  static CEvent m_utilsJobEvent;
  CMusicDatabase m_musicDatabase;
};
