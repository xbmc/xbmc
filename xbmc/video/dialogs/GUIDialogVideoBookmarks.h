#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIDialog.h"
#include "view/GUIViewControl.h"
#include "video/VideoDatabase.h"
#include "utils/JobManager.h"

class CFileItemList;

class CGUIDialogVideoBookmarks : public CGUIDialog, public CJobQueue
{
  typedef std::map<CJob*, unsigned int> MAPJOBSCHAPS;

public:
  CGUIDialogVideoBookmarks(void);
  virtual ~CGUIDialogVideoBookmarks(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();
  virtual bool OnAction(const CAction &action);

  /*!
   \brief Creates a bookmark of the currently playing video file.
   
          NOTE: sends a GUI_MSG_REFRESH_LIST message to DialogVideoBookmark on success
   \return True if creation of bookmark was succesful
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
  CGUIControl *GetFirstFocusableControl(int id);

  void OnJobComplete(unsigned int jobID, bool success, CJob* job);

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
