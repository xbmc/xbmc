/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "utils/JobManager.h"
#include "video/VideoDatabase.h"
#include "view/GUIViewControl.h"

class CFileItemList;

class CGUIDialogVideoBookmarks : public CGUIDialog, public CJobQueue
{
  typedef std::map<CJob*, unsigned int> MAPJOBSCHAPS;

public:
  CGUIDialogVideoBookmarks(void);
  ~CGUIDialogVideoBookmarks(void) override;
  bool OnMessage(CGUIMessage& message) override;
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  bool OnAction(const CAction &action) override;

  /*!
   \brief Creates a bookmark of the currently playing video file.

          NOTE: sends a GUI_MSG_REFRESH_LIST message to DialogVideoBookmark on success
   \return True if creation of bookmark was successful
   \sa OnAddEpisodeBookmark
   */
  static bool OnAddBookmark();

  /*!
   \brief Creates an episode bookmark of the currently playing file

          An episode bookmark specifies the end/beginning of episodes on files like: S01E01E02
          Fails if the current video isn't a multi-episode file
          NOTE: sends a GUI_MSG_REFRESH_LIST message to DialogVideoBookmark on success
   \return True, if bookmark was successfully created
   \sa OnAddBookmark
   **/
  static bool OnAddEpisodeBookmark();


  void Update();
protected:
  void GotoBookmark(int iItem);
  void ClearBookmarks();
  static bool AddEpisodeBookmark();
  static bool AddBookmark(CVideoInfoTag *tag=NULL);
  void Delete(int item);
  void Clear();
  void OnRefreshList();
  void OnPopupMenu(int item);
  CGUIControl *GetFirstFocusableControl(int id) override;

  void OnJobComplete(unsigned int jobID, bool success, CJob* job) override;

  CFileItemList* m_vecItems;
  CGUIViewControl m_viewControl;
  VECBOOKMARKS m_bookmarks;

private:
  void UpdateItem(unsigned int chapterIdx);

  int m_jobsStarted;
  std::string m_filePath;
  CCriticalSection m_refreshSection;
  MAPJOBSCHAPS m_mapJobsChapter;
};
