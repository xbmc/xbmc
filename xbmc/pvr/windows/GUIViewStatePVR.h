#pragma once

/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "view/GUIViewState.h"

namespace PVR
{
  class CGUIViewStatePVR : public CGUIViewState
  {
  public:
    CGUIViewStatePVR(const int windowId, const CFileItemList& items) : CGUIViewState(items) { m_windowId = windowId; }

  protected:
    virtual bool HideParentDirItems(void) { return true; };
    int m_windowId;
  };

  class CGUIViewStateWindowPVRChannels : public CGUIViewStatePVR
  {
  public:
    CGUIViewStateWindowPVRChannels(const int windowId, const CFileItemList& items);

  protected:
    virtual void SaveViewState();
  };

  class CGUIViewStateWindowPVRRecordings : public CGUIViewStatePVR
  {
  public:
    CGUIViewStateWindowPVRRecordings(const int windowId, const CFileItemList& items);

  protected:
    virtual void SaveViewState();
    virtual bool HideParentDirItems(void);
  };

  class CGUIViewStateWindowPVRGuide : public CGUIViewStatePVR
  {
  public:
    CGUIViewStateWindowPVRGuide(const int windowId, const CFileItemList& items);

  protected:
    virtual void SaveViewState();
  };

  class CGUIViewStateWindowPVRTimers : public CGUIViewStatePVR
  {
  public:
    CGUIViewStateWindowPVRTimers(const int windowId, const CFileItemList& items);

  protected:
    virtual void SaveViewState();
  };

  class CGUIViewStateWindowPVRSearch : public CGUIViewStatePVR
  {
  public:
    CGUIViewStateWindowPVRSearch(const int windowId, const CFileItemList& items);

  protected:
    virtual void SaveViewState();
  };
}
