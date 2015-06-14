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

#include "windows/GUIMediaWindow.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"

#define CONTROL_BTNVIEWASICONS            2
#define CONTROL_BTNSORTBY                 3
#define CONTROL_BTNSORTASC                4
#define CONTROL_BTNGROUPITEMS             5
#define CONTROL_BTNSHOWHIDDEN             6
#define CONTROL_BTNSHOWDELETED            7
#define CONTROL_BTNTIMERTYPEFILTER        8
#define CONTROL_BTNCHANNELGROUPS          28
#define CONTROL_BTNFILTERCHANNELS         31

#define CONTROL_LABEL_HEADER1             29
#define CONTROL_LABEL_HEADER2             30

namespace PVR
{
  enum EpgGuideView
  {
    GUIDE_VIEW_TIMELINE = 10,
    GUIDE_VIEW_NOW      = 11,
    GUIDE_VIEW_NEXT     = 12,
    GUIDE_VIEW_CHANNEL  = 13
  };

  enum EPGSelectAction
  {
    EPG_SELECT_ACTION_CONTEXT_MENU   = 0,
    EPG_SELECT_ACTION_SWITCH         = 1,
    EPG_SELECT_ACTION_INFO           = 2,
    EPG_SELECT_ACTION_RECORD         = 3,
    EPG_SELECT_ACTION_PLAY_RECORDING = 4,
  };

  class CGUIWindowPVRBase : public CGUIMediaWindow, public Observer
  {
  public:
    virtual ~CGUIWindowPVRBase(void);
    virtual void OnInitWindow(void);
    virtual void OnDeinitWindow(int nextWindowID);
    virtual bool OnMessage(CGUIMessage& message);
    virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
    virtual bool OnContextButton(const CFileItem &item, CONTEXT_BUTTON button) { return false; };
    virtual void UpdateButtons(void);
    virtual bool OnAction(const CAction &action);
    virtual bool OnBack(int actionID);
    virtual bool OpenGroupSelectionDialog(void);
    virtual void ResetObservers(void) {};
    virtual void Notify(const Observable &obs, const ObservableMessage msg);
    virtual void SetInvalid();

    static std::string GetSelectedItemPath(bool bRadio);
    static void SetSelectedItemPath(bool bRadio, const std::string &path);

  protected:
    CGUIWindowPVRBase(bool bRadio, int id, const std::string &xmlFile);

    virtual std::string GetDirectoryPath(void) { return ""; };
    virtual CPVRChannelGroupPtr GetGroup(void);
    virtual void SetGroup(CPVRChannelGroupPtr group);

    virtual bool ActionRecord(CFileItem *item);
    virtual bool ActionPlayChannel(CFileItem *item);
    virtual bool ActionPlayEpg(CFileItem *item, bool bPlayRecording);
    virtual bool ActionDeleteChannel(CFileItem *item);
    virtual bool ActionInputChannelNumber(int input);

    virtual bool PlayRecording(CFileItem *item, bool bPlayMinimized = false, bool bCheckResume = true);
    virtual bool PlayFile(CFileItem *item, bool bPlayMinimized = false, bool bCheckResume = true);
    virtual bool ShowTimerSettings(CFileItem *item);
    virtual bool StartRecordFile(CFileItem *item, bool bAdvanced = false);
    virtual bool StopRecordFile(CFileItem *item);
    virtual void ShowEPGInfo(CFileItem *item);
    virtual void ShowRecordingInfo(CFileItem *item);
    virtual bool UpdateEpgForChannel(CFileItem *item);
    virtual void UpdateSelectedItemPath();
    void CheckResumeRecording(CFileItem *item);

    /*!
     * @brief Open a dialog to confirm timer delete.
     * @param item the timer to delete.
     * @param bDeleteSchedule in: ignored
     *                        out, for timer schedules: true to delete the timers currently
     *                             scheduled by the timer schedule, false otherwise.
     *                        out, for one shot timers: ignored
     * @return true, if the timer shall be deleted, false otherwise.
     */
    static bool ConfirmDeleteTimer(CFileItem *item, bool &bDeleteSchedule);

    static std::map<bool, std::string> m_selectedItemPaths;

    CCriticalSection m_critSection;
    bool m_bRadio;

  private:
    CPVRChannelGroupPtr m_group;
  };
}
