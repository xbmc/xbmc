/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "threads/CriticalSection.h"
#include "utils/JobManager.h"

#include <string>

enum SUBTITLE_STORAGEMODE
{
  SUBTITLE_STORAGEMODE_MOVIEPATH = 0,
  SUBTITLE_STORAGEMODE_CUSTOMPATH
};

class CFileItem;
class CFileItemList;

class CGUIDialogSubtitles : public CGUIDialog, CJobQueue
{
public:
  CGUIDialogSubtitles(void);
  ~CGUIDialogSubtitles(void) override;
  bool OnMessage(CGUIMessage& message) override;
  void OnInitWindow() override;

protected:
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;

  bool SetService(const std::string &service);
  const CFileItemPtr GetService() const;
  void FillServices();
  void ClearServices();
  void ClearSubtitles();

  enum STATUS { NO_SERVICES = 0, SEARCHING, SEARCH_COMPLETE, DOWNLOADING };
  void UpdateStatus(STATUS status);

  void Search(const std::string &search="");
  void OnSearchComplete(const CFileItemList *items);

  void Download(const CFileItem &subtitle);
  void OnDownloadComplete(const CFileItemList *items, const std::string &language);


  /*!
   \brief Called when the context menu is requested on a subtitle service
   present on the list of installed subtitle addons
   \param itemIdx the index of the selected subtitle service on the list
  */
  void OnSubtitleServiceContextMenu(int itemIdx);

  void SetSubtitles(const std::string &subtitle);

  CCriticalSection m_critsection;
  CFileItemList* m_subtitles;
  CFileItemList* m_serviceItems;
  std::string    m_currentService;
  std::string    m_status;
  std::string     m_strManualSearch;
  bool           m_pausedOnRun = false;
  bool           m_updateSubsList = false; ///< true if we need to update our subs list
  std::string     m_LastAutoDownloaded; ///< Last video file path which automatically downloaded subtitle
};
