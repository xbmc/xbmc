#pragma once

/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "FileItem.h"
#include "windows/GUIMediaWindow.h"
#include "GUIWindowPVRCommon.h"
#include "threads/CriticalSection.h"

namespace PVR
{
  enum PVRWindow
  {
    PVR_WINDOW_UNKNOWN         = 0,
    PVR_WINDOW_EPG             = 1,
    PVR_WINDOW_CHANNELS_TV     = 2,
    PVR_WINDOW_CHANNELS_RADIO  = 3,
    PVR_WINDOW_RECORDINGS      = 4,
    PVR_WINDOW_TIMERS          = 5,
    PVR_WINDOW_SEARCH          = 6
  };

  #define CONTROL_LIST_TIMELINE        10
  #define CONTROL_LIST_CHANNELS_TV     11
  #define CONTROL_LIST_CHANNELS_RADIO  12
  #define CONTROL_LIST_RECORDINGS      13
  #define CONTROL_LIST_TIMERS          14
  #define CONTROL_LIST_GUIDE_CHANNEL   15
  #define CONTROL_LIST_GUIDE_NOW_NEXT  16
  #define CONTROL_LIST_SEARCH          17

  #define CONTROL_LABELHEADER          29
  #define CONTROL_LABELGROUP           30

  #define CONTROL_BTNGUIDE             31
  #define CONTROL_BTNCHANNELS_TV       32
  #define CONTROL_BTNCHANNELS_RADIO    33
  #define CONTROL_BTNRECORDINGS        34
  #define CONTROL_BTNTIMERS            35
  #define CONTROL_BTNSEARCH            36
  #define CONTROL_BTNGUIDE_CHANNEL     37
  #define CONTROL_BTNGUIDE_NOW         38
  #define CONTROL_BTNGUIDE_NEXT        39
  #define CONTROL_BTNGUIDE_TIMELINE    40

  class CGUIWindowPVR;

  class CGUIWindowPVRCommon
  {
    friend class CGUIWindowPVR;

  public:
    CGUIWindowPVRCommon(CGUIWindowPVR *parent, PVRWindow window,
        unsigned int iControlButton, unsigned int iControlList);
    virtual ~CGUIWindowPVRCommon(void) {};

    bool operator ==(const CGUIWindowPVRCommon &right) const;
    bool operator !=(const CGUIWindowPVRCommon &right) const;

    virtual const char *GetName(void) const;
    virtual PVRWindow GetWindowId(void) const { return m_window; }
    virtual bool IsVisible(void) const;
    virtual bool IsActive(void) const;
    virtual bool IsSavedView(void) const;
    virtual bool IsSelectedButton(CGUIMessage &message) const;
    virtual bool IsSelectedControl(CGUIMessage &message) const;
    virtual bool IsSelectedList(CGUIMessage &message) const;

    virtual bool OnAction(const CAction &action);
    virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);

    virtual void GetContextButtons(int itemNumber, CContextButtons &buttons) const = 0;
    virtual void UpdateData(void) = 0;
    virtual void SetInvalid(void);

    virtual void OnInitWindow(void);
    virtual void OnWindowUnload(void);

  protected:
    virtual bool SelectPlayingFile(void);
    virtual bool OnMessageFocus(CGUIMessage &message);

    virtual bool OnClickButton(CGUIMessage &message) = 0;
    virtual bool OnClickList(CGUIMessage &message) = 0;

    virtual bool ActionDeleteTimer(CFileItem *item);
    virtual bool ActionShowTimer(CFileItem *item);
    virtual bool ActionRecord(CFileItem *item);
    virtual bool ActionDeleteRecording(CFileItem *item);
    virtual bool ActionPlayChannel(CFileItem *item);
    virtual bool ActionPlayEpg(CFileItem *item);
    virtual bool ActionDeleteChannel(CFileItem *item);

    virtual bool PlayRecording(CFileItem *item, bool bPlayMinimized = false);
    virtual bool PlayFile(CFileItem *item, bool bPlayMinimized = false);
    virtual bool StartRecordFile(CFileItem *item);
    virtual bool StopRecordFile(CFileItem *item);
    virtual void ShowEPGInfo(CFileItem *item);
    virtual void ShowRecordingInfo(CFileItem *item);
    virtual bool ShowTimerSettings(CFileItem *item);
    virtual bool ShowNewTimerDialog(void);

    virtual bool OnContextButtonMenuHooks(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonSortAsc(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonSortBy(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonSortByDate(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonSortByName(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonSortByChannel(CFileItem *item, CONTEXT_BUTTON button);
    virtual bool OnContextButtonFind(CFileItem *item, CONTEXT_BUTTON button);

    CGUIWindowPVR *  m_parent;
    PVRWindow        m_window;
    unsigned int     m_iControlButton;
    unsigned int     m_iControlList;
    bool             m_bUpdateRequired;
    int              m_iSelected;
    SORT_ORDER       m_iSortOrder;
    SORT_METHOD      m_iSortMethod;
    CCriticalSection m_critSection;
  };
}
