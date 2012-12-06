//
//  GUIPlexMusicWindow.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-12-05.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef GUIPLEXMUSICWINDOW_H
#define GUIPLEXMUSICWINDOW_H

#include "music/windows/GUIWindowMusicNav.h"
#include "music/windows/GUIWindowMusicSongs.h"
#include "Utility/PlexFilterHelper.h"
#include "plex/PlexTypes.h"

class CGUIPlexMusicWindow : public CGUIWindowMusicSongs
{
  public:
    CGUIPlexMusicWindow() : CGUIWindowMusicSongs(), m_returningFromSkinLoad(false), m_filterHelper(this) {};
    bool OnMessage(CGUIMessage &message);

  protected:
    bool Update(const CStdString &strDirectory, bool updateFilterPath, bool updateFilters);
    bool Update(const CStdString &strDirectory, bool updateFilterPath);
    void BuildFilters(const CStdString& strDirectory);

  private:
    CPlexFilterHelper m_filterHelper;
    bool m_returningFromSkinLoad;
};

#endif // GUIPLEXMUSICWINDOW_H
