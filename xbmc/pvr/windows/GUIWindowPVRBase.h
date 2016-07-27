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
#include "utils/Observer.h"

#define CONTROL_BTNVIEWASICONS            2
#define CONTROL_BTNSORTBY                 3
#define CONTROL_BTNSORTASC                4
#define CONTROL_BTNGROUPITEMS             5
#define CONTROL_BTNSHOWHIDDEN             6
#define CONTROL_BTNSHOWDELETED            7
#define CONTROL_BTNHIDEDISABLEDTIMERS     8
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

  class CPVRChannelGroup;
  typedef std::shared_ptr<CPVRChannelGroup> CPVRChannelGroupPtr;

  class CPVRTimerInfoTag;
  typedef std::shared_ptr<CPVRTimerInfoTag> CPVRTimerInfoTagPtr;

  class CGUIWindowPVRBase : public CGUIMediaWindow, public Observer
  {
  public:
    virtual ~CGUIWindowPVRBase(void);
    virtual void OnInitWindow(void) override;
    virtual void OnDeinitWindow(int nextWindowID) override;
    virtual bool OnMessage(CGUIMessage& message) override;
    virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
    virtual bool OnContextButton(const CFileItem &item, CONTEXT_BUTTON button) { return false; };
    virtual bool OnContextButtonActiveAEDSPSettings(CFileItem *item, CONTEXT_BUTTON button);
    virtual void UpdateButtons(void) override;
    virtual bool OnAction(const CAction &action) override;
    virtual bool OnBack(int actionID) override;
    virtual bool OpenGroupSelectionDialog(void);
    virtual void Notify(const Observable &obs, const ObservableMessage msg) override;
    virtual void SetInvalid() override;
    virtual bool CanBeActivated() const override;

    void ResetObservers(void);

    static std::string GetSelectedItemPath(bool bRadio);
    static void SetSelectedItemPath(bool bRadio, const std::string &path);

    static bool ShowTimerSettings(const CPVRTimerInfoTagPtr &timer);
    static bool AddTimer(CFileItem *item, bool bShowTimerSettings = false);
    static bool AddTimerRule(CFileItem *item, bool bShowTimerSettings = true);
    static bool EditTimer(CFileItem *item);
    static bool EditTimerRule(CFileItem *item);
    static bool DeleteTimer(CFileItem *item);
    static bool DeleteTimerRule(CFileItem *item);
    static bool StopRecordFile(CFileItem *item);

  protected:
    CGUIWindowPVRBase(bool bRadio, int id, const std::string &xmlFile);

    virtual std::string GetDirectoryPath(void) = 0;
    virtual CPVRChannelGroupPtr GetGroup(void);
    virtual void SetGroup(const CPVRChannelGroupPtr &group);

    virtual bool ActionShowTimerRule(CFileItem *item);
    virtual bool ActionToggleTimer(CFileItem *item);
    virtual bool ActionPlayChannel(CFileItem *item);
    virtual bool ActionPlayEpg(CFileItem *item, bool bPlayRecording);
    virtual bool ActionDeleteChannel(CFileItem *item);
    virtual bool ActionInputChannelNumber(int input);

    virtual bool PlayRecording(CFileItem *item, bool bPlayMinimized = false, bool bCheckResume = true);
    virtual bool PlayFile(CFileItem *item, bool bPlayMinimized = false, bool bCheckResume = true);
    virtual void ShowEPGInfo(CFileItem *item);
    virtual void ShowRecordingInfo(CFileItem *item);
    virtual bool UpdateEpgForChannel(CFileItem *item);
    virtual void UpdateSelectedItemPath();
    virtual bool IsValidMessage(CGUIMessage& message);
    bool CheckResumeRecording(CFileItem *item);

    bool OnContextButtonEditTimer(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonEditTimerRule(CFileItem *item, CONTEXT_BUTTON button);
    bool OnContextButtonDeleteTimerRule(CFileItem *item, CONTEXT_BUTTON button);

    virtual void RegisterObservers(void);
    virtual void UnregisterObservers(void);

    static CCriticalSection m_selectedItemPathsLock;
    static std::string m_selectedItemPaths[2];

    CCriticalSection m_critSection;
    bool m_bRadio;

  private:
    /*!
     * @brief Open a dialog to confirm timer delete.
     * @param timer the timer to delete.
     * @param bDeleteRule in: ignored
     *                    out, for one shot timer scheduled by a timer rule: true to also delete the timer
     *                    rule that has scheduled this timer, false to only delete the one shot timer.
     *                    out, for one shot timer not scheduled by a timer rule: ignored
     * @return true, to proceed with delete, false otherwise.
     */
    static bool ConfirmDeleteTimer(const CPVRTimerInfoTagPtr &timer, bool &bDeleteRule);

    /*!
     * @brief Open a dialog to confirm stop recording.
     * @param timer the recording to stop (actually the timer to delete).
     * @return true, to proceed with delete, false otherwise.
     */
    static bool ConfirmStopRecording(const CPVRTimerInfoTagPtr &timer);

    static bool DeleteTimer(CFileItem *item, bool bIsRecording, bool bDeleteRule);
    static bool AddTimer(CFileItem *item, bool bCreateRule, bool bShowTimerSettings);

    CPVRChannelGroupPtr m_group;
    XbmcThreads::EndTime m_refreshTimeout;
  };
}
