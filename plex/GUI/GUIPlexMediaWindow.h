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

class CGUIPlexMediaWindow : public CGUIWindowVideoNav, public IJobCallback
{    
  public:
    CGUIPlexMediaWindow() : CGUIWindowVideoNav(), m_returningFromSkinLoad(false), m_filterHelper(this) {};
    bool OnMessage(CGUIMessage &message);
    void PlayMovie(const CFileItem *item);
    virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  
  protected:
    bool Update(const CStdString &strDirectory, bool updateFilterPath, bool updateFilters);
    bool Update(const CStdString &strDirectory, bool updateFilterPath);
    void BuildFilter(const CURL &strDirectory);

  private:
    virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
    CPlexFilterHelper m_filterHelper;
    bool m_returningFromSkinLoad;
};

#endif // GUIWINDOWMEDIAFILTERVIEW_H
