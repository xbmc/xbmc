//
//  GUIWindowMediaFilterView.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-11-19.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef GUIWINDOWMEDIAFILTERVIEW_H
#define GUIWINDOWMEDIAFILTERVIEW_H

#include "video/windows/GUIWindowVideoNav.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "StringUtils.h"
#include "Utility/PlexFilterHelper.h"
#include "JobManager.h"

class CGUIPlexMediaWindow : public CGUIMediaWindow, public IJobCallback
{    
  public:
    CGUIPlexMediaWindow(int windowId = WINDOW_VIDEO_NAV, const CStdString &xml = "MyVideoNav.xml") :
      CGUIMediaWindow(windowId, xml), m_filterHelper(this), m_returningFromSkinLoad(false), m_pagingOffset(0), m_currentJobId(-1) {};
    bool OnMessage(CGUIMessage &message);
    bool OnAction(const CAction& action);
    virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
    void GetContextButtons(int itemNumber, CContextButtons &buttons);

    bool Update(const CStdString &strDirectory, bool updateFilterPath, bool updateFilters);
    bool Update(const CStdString &strDirectory, bool updateFilterPath);
    void BuildFilter(const CURL &strDirectory);
    bool OnSelect(int item);
    bool OnPlayMedia(int iItem);
    bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);

    void ShuffleItem(CFileItemPtr item);
    void QueueItem(CFileItemPtr item);
    void QueueItems(const CFileItemList &list, CFileItemPtr startItem=CFileItemPtr());

    bool OnBack(int actionID);

  private:

    bool IsVideoContainer(CFileItemPtr item=CFileItemPtr()) const;
    bool IsMusicContainer() const;
    bool IsPhotoContainer() const;

    CStdString ShowPluginSearch(CFileItemPtr item);
    CStdString ShowPluginSettings(CFileItemPtr item);

    int ContainerPlaylistType() const
    {
      int currentPlaylist = PLAYLIST_NONE;
      if (IsVideoContainer()) currentPlaylist = PLAYLIST_VIDEO;
      else if (IsMusicContainer()) currentPlaylist = PLAYLIST_MUSIC;
      else if (IsPhotoContainer()) currentPlaylist = PLAYLIST_PICTURE;
      return currentPlaylist;
    }

    void LoadPage(int start, int numberOfItems);
    void LoadNextPage();
    virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
    CPlexFilterHelper m_filterHelper;
    bool m_returningFromSkinLoad;
  
    int m_pagingOffset;
    int m_currentJobId;
};

class CGUIPlexMusicWindow : public CGUIPlexMediaWindow
{
  public:
    CGUIPlexMusicWindow() : CGUIPlexMediaWindow(WINDOW_MUSIC_FILES, "MyMusicSongs.xml") {}
};

class CGUIPlexPictureWindow : public CGUIPlexMediaWindow
{
  public:
    CGUIPlexPictureWindow() : CGUIPlexMediaWindow(WINDOW_PICTURES, "MyPics.xml") {}
};

#endif // GUIWINDOWMEDIAFILTERVIEW_H
