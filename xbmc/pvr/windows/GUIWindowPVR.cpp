/*
 *      Copyright (C) 2005-2010 Team XBMC
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

/* Standard includes */
#include "Application.h"
#include "filesystem/File.h"
#include "filesystem/StackDirectory.h"
#include "settings/GUISettings.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "storage/MediaManager.h"
#include "pictures/Picture.h"
#include "settings/Settings.h"
#include "TextureCache.h"
#include "utils/log.h"
#include "URL.h"

/* Dialog windows includes */
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "pvr/dialogs/GUIDialogPVRGuideInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"
#include "pvr/dialogs/GUIDialogPVRGroupManager.h"
#include "pvr/dialogs/GUIDialogPVRGuideSearch.h"
#include "GUIUserMessages.h"
#include "guilib/GUIEPGGridContainer.h"

/* self include */
#include "GUIWindowPVR.h"

/* TV control */
#include "pvr/PVRManager.h"
#include "pvr/epg/PVREpgInfoTag.h"
#include "pvr/PVRTimerInfoTag.h"
#include "pvr/PVRChannelGroupsContainer.h"

using namespace std;

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

/**
 * \brief Class constructor
 */
CGUIWindowPVR::CGUIWindowPVR(void) : CGUIMediaWindow(WINDOW_PVR, "MyTV.xml")
{
  m_iCurrSubTVWindow              = TV_WINDOW_UNKNOWN;
  m_iSavedSubTVWindow             = TV_WINDOW_UNKNOWN;
  m_iSelected_GUIDE               = 0;
  m_iSelected_CHANNELS_TV         = 0;
  m_iSelected_CHANNELS_RADIO      = 0;
  m_iSelected_RECORDINGS          = 0;
  m_iSelected_RECORDINGS_Path     = "pvr://recordings/";
  m_iSelected_TIMERS              = 0;
  m_iSelected_SEARCH              = 0;
  m_iCurrentTVGroup               = -1;
  m_iCurrentRadioGroup            = -1;
  m_bShowHiddenChannels           = false;
  m_bSearchStarted                = false;
  m_bSearchConfirmed              = false;
  m_iGuideView                    = g_guiSettings.GetInt("pvrmenu.defaultguideview");
  m_guideGrid                     = NULL;
  m_iSortOrder_SEARCH             = SORT_ORDER_ASC;
  m_iSortMethod_SEARCH            = SORT_METHOD_DATE;
  m_iSortOrder_TIMERS             = SORT_ORDER_ASC;
  m_iSortMethod_TIMERS            = SORT_METHOD_DATE;
}

CGUIWindowPVR::~CGUIWindowPVR()
{
}

bool CGUIWindowPVR::OnMessage(CGUIMessage& message)
{
  unsigned int iControl = 0;
  unsigned int iMessage = message.GetMessage();

  if (iMessage == GUI_MSG_FOCUSED)
  {
    /* Get the focused control Identifier */
    iControl = message.GetControlId();

    /* Process Identifier for focused Subwindow select buttons or list item.
     * If a new conrol becomes highlighted load his subwindow data
     */
    if (iControl == CONTROL_BTNGUIDE || m_iSavedSubTVWindow == TV_WINDOW_TV_PROGRAM)
    {
      if (m_iCurrSubTVWindow != TV_WINDOW_TV_PROGRAM)
        UpdateGuide();
      else
        m_iSelected_GUIDE = m_viewControl.GetSelectedItem();

      m_iCurrSubTVWindow = TV_WINDOW_TV_PROGRAM;
    }
    else if (iControl == CONTROL_BTNCHANNELS_TV || m_iSavedSubTVWindow == TV_WINDOW_CHANNELS_TV)
    {
      if (m_iCurrSubTVWindow != TV_WINDOW_CHANNELS_TV)
        UpdateChannelsTV();
      else
        m_iSelected_CHANNELS_TV = m_viewControl.GetSelectedItem();

      m_iCurrSubTVWindow = TV_WINDOW_CHANNELS_TV;
    }
    else if (iControl == CONTROL_BTNCHANNELS_RADIO || m_iSavedSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
    {
      if (m_iCurrSubTVWindow != TV_WINDOW_CHANNELS_RADIO)
        UpdateChannelsRadio();
      else
        m_iSelected_CHANNELS_RADIO = m_viewControl.GetSelectedItem();

      m_iCurrSubTVWindow = TV_WINDOW_CHANNELS_RADIO;
    }
    else if (iControl == CONTROL_BTNRECORDINGS || m_iSavedSubTVWindow == TV_WINDOW_RECORDINGS)
    {
      if (m_iCurrSubTVWindow != TV_WINDOW_RECORDINGS)
        UpdateRecordings();
      else
      {
        m_iSelected_RECORDINGS_Path = m_vecItems->m_strPath;
        m_iSelected_RECORDINGS = m_viewControl.GetSelectedItem();
      }

      m_iCurrSubTVWindow = TV_WINDOW_RECORDINGS;
    }
    else if (iControl == CONTROL_BTNTIMERS || m_iSavedSubTVWindow == TV_WINDOW_TIMERS)
    {
      if (m_iCurrSubTVWindow != TV_WINDOW_TIMERS)
        UpdateTimers();
      else
        m_iSelected_TIMERS =  m_viewControl.GetSelectedItem();

      m_iCurrSubTVWindow = TV_WINDOW_TIMERS;
    }
    else if (iControl == CONTROL_BTNSEARCH || m_iSavedSubTVWindow == TV_WINDOW_SEARCH)
    {
      if (m_iCurrSubTVWindow != TV_WINDOW_SEARCH)
        UpdateSearch();
      else
        m_iSelected_SEARCH = m_viewControl.GetSelectedItem();

      m_iCurrSubTVWindow = TV_WINDOW_SEARCH;
    }

    if (m_iSavedSubTVWindow != TV_WINDOW_UNKNOWN)
      m_iSavedSubTVWindow = TV_WINDOW_UNKNOWN;
  }
  else if (iMessage == GUI_MSG_CLICKED)
  {
    iControl = message.GetSenderId();

    if (iControl == CONTROL_BTNGUIDE)
    {
      m_iGuideView++;

      if (m_iGuideView > GUIDE_VIEW_TIMELINE)
      {
        m_iGuideView = 0;
      }

      UpdateGuide();
      return true;
    }
    else if (iControl == CONTROL_BTNGUIDE_CHANNEL)
    {
      m_iGuideView = GUIDE_VIEW_CHANNEL;
      UpdateGuide();
      return true;
    }
    else if (iControl == CONTROL_BTNGUIDE_NOW)
    {
      m_iGuideView = GUIDE_VIEW_NOW;
      UpdateGuide();
      return true;
    }
    else if (iControl == CONTROL_BTNGUIDE_NEXT)
    {
      m_iGuideView = GUIDE_VIEW_NEXT;
      UpdateGuide();
      return true;
    }
    else if (iControl == CONTROL_BTNGUIDE_TIMELINE)
    {
      m_iGuideView = GUIDE_VIEW_TIMELINE;
      UpdateGuide();
      return true;
    }
    else if (iControl == CONTROL_BTNCHANNELS_TV)
    {
      m_iCurrentTVGroup = g_PVRChannelGroups.GetTV()->GetNextGroupID(m_iCurrentTVGroup);
      UpdateChannelsTV();
      return true;
    }
    else if (iControl == CONTROL_BTNCHANNELS_RADIO)
    {
      m_iCurrentRadioGroup = g_PVRChannelGroups.GetRadio()->GetNextGroupID(m_iCurrentRadioGroup);
      UpdateChannelsRadio();
      return true;
    }
    else if (iControl == CONTROL_BTNRECORDINGS)
    {
      g_PVRManager.TriggerRecordingsUpdate();
      UpdateRecordings();
      return true;
    }
    else if (iControl == CONTROL_BTNTIMERS)
    {
      g_PVRTimers.Update();
      UpdateTimers();
      return true;
    }
    else if (iControl == CONTROL_BTNSEARCH)
    {
      ShowSearchResults();
    }
    else if (iControl == CONTROL_LIST_TIMELINE ||
             iControl == CONTROL_LIST_GUIDE_CHANNEL ||
             iControl == CONTROL_LIST_GUIDE_NOW_NEXT)
    {
      int iAction = message.GetParam1();
      int iItem = m_viewControl.GetSelectedItem();

      /* Check file item is in list range and get his pointer */
      if (iItem < 0 || iItem >= (int)m_vecItems->Size()) return true;

      CFileItemPtr pItem = m_vecItems->Get(iItem);

      /* Process actions */
      if ((iAction == ACTION_SELECT_ITEM) || (iAction == ACTION_SHOW_INFO || iAction == ACTION_MOUSE_LEFT_CLICK))
      {
        /* Show information Dialog */
        ShowEPGInfo(pItem.get());
        return true;
      }
      else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      {
        /* Show Contextmenu */
        OnPopupMenu(iItem);
        return true;
      }
      else if (iAction == ACTION_RECORD)
      {
        if (((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->ChannelNumber() != -1)
        {
          if (((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->Timer() == NULL)
          {
            // prompt user for confirmation of channel record
            CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
            if (!pDialog)
              return false;

            pDialog->SetHeading(264);
            pDialog->SetLine(0, "");
            pDialog->SetLine(1, pItem->GetEPGInfoTag()->Title());
            pDialog->SetLine(2, "");
            pDialog->DoModal();

            if (!pDialog->IsConfirmed())
              return true;

            CPVREpgInfoTag *tag = (CPVREpgInfoTag *) pItem->GetEPGInfoTag();
            CPVRTimerInfoTag *newtimer = CPVRTimerInfoTag::CreateFromEpg(*tag);
            CFileItem *item = new CFileItem(*newtimer);

            if (g_PVRTimers.AddTimer(*item))
              g_PVRTimers.Update();
          }
          else
          {
            CGUIDialogOK::ShowAndGetInput(19033,19034,0,0);
          }
        }
      }
      else if (iAction == ACTION_PLAY)
      {
        const CPVRChannelGroup *channels = g_PVRChannelGroups.GetGroupAll(((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->IsRadio());
        if (!g_application.PlayFile(CFileItem(*channels->at(((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->ChannelNumber()-1))))
        {
          CGUIDialogOK::ShowAndGetInput(19033,0,19035,0);
          return false;
        }
        return true;
      }
    }
    else if ((iControl == CONTROL_LIST_CHANNELS_TV) || (iControl == CONTROL_LIST_CHANNELS_RADIO))
    {
      /* Get currently performed action */
      int iAction = message.GetParam1();

      /* Get currently selected item from file list */
      int iItem = m_viewControl.GetSelectedItem();

      /* Check file item is in list range and get his pointer */
      if (iItem < 0 || iItem >= (int)m_vecItems->Size()) return true;

      CFileItemPtr pItem = m_vecItems->Get(iItem);

      /* Process actions */
      if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK || iAction == ACTION_PLAY)
      {
        /* Check if "Add channel..." entry is pressed by OK, if yes
           create a new channel and open settings dialog, otherwise
           open channel with player */
        if (pItem->m_strPath == "pvr://channels/.add.channel")
        {
          CGUIDialogOK::ShowAndGetInput(19033,0,19038,0);
        }
        else
        {
          if (iControl == CONTROL_LIST_CHANNELS_TV)
            g_PVRManager.SetPlayingGroup(m_iCurrentTVGroup);
          if (iControl == CONTROL_LIST_CHANNELS_RADIO)
            g_PVRManager.SetPlayingGroup(m_iCurrentRadioGroup);

          /* Open tv channel by Player and return */
          return PlayFile(pItem.get(), g_guiSettings.GetBool("pvrplayback.playminimized"));
        }
      }
      else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      {
        //contextmenu
        OnPopupMenu(iItem);
        return true;
      }
      else if (iAction == ACTION_SHOW_INFO)
      {
        /* Show information Dialog */
        ShowEPGInfo(pItem.get());
        return true;
      }
      else if (iAction == ACTION_DELETE_ITEM)
      {
        /* Check if entry is a valid deleteable channel */
        int iChannel = pItem->GetPVRChannelInfoTag()->ChannelNumber();

        if (iChannel != -1)
        {
          CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

          if (pDialog)
          {
            pDialog->SetHeading(19039);
            pDialog->SetLine(0, "");
            pDialog->SetLine(1, pItem->GetPVRChannelInfoTag()->ChannelName());
            pDialog->SetLine(2, "");
            pDialog->DoModal();

            if (!pDialog->IsConfirmed()) return false;

            if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
            {
              ((CPVRChannelGroup *) g_PVRChannelGroups.GetGroupAll(false))->HideChannel(pItem->GetPVRChannelInfoTag(), true);
              UpdateChannelsTV();
            }
            else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
            {
              ((CPVRChannelGroup *) g_PVRChannelGroups.GetGroupAll(true))->HideChannel(pItem->GetPVRChannelInfoTag(), true);
              UpdateChannelsRadio();
            }
          }
          return true;
        }
      }
    }
    else if (iControl == CONTROL_LIST_RECORDINGS)
    {
      /* Get currently performed action */
      int iAction = message.GetParam1();

      /* Get currently selected item from file list */
      int iItem = m_viewControl.GetSelectedItem();

      /* Check file item is in list range and get his pointer */
      if (iItem < 0 || iItem >= (int)m_vecItems->Size()) return true;

      CFileItemPtr pItem = m_vecItems->Get(iItem);
      if (pItem->m_bIsFolder || pItem->IsParentFolder())
        return OnClick(iItem);

      /* Process actions */
      if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK || iAction == ACTION_PLAY)
      {
        /* Open recording with Player and return */
        return PlayFile(pItem.get(), false);
      }
      else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      {
        //contextmenu
        OnPopupMenu(iItem);
        return true;
      }
      else if (iAction == ACTION_SHOW_INFO)
      {
        /* Show information Dialog */
        ShowRecordingInfo(pItem.get());
        return true;
      }
      else if (iAction == ACTION_DELETE_ITEM)
      {
        /* Check if entry is a valid deleteable record */
        if (pItem->GetPVRRecordingInfoTag()->ClientIndex() != -1)
        {
          // prompt user for confirmation of record deletion
          CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
          if (!pDialog)
            return false;

          pDialog->SetHeading(122);
          pDialog->SetLine(0, 19043);
          pDialog->SetLine(1, "");
          pDialog->SetLine(2, pItem->GetPVRRecordingInfoTag()->Title());
          pDialog->DoModal();

          if (pDialog->IsConfirmed())
          {
            if (CPVRRecordings::DeleteRecording(*pItem))
            {
              PVRRecordings.Update(true);
              UpdateRecordings();
            }
          }
          return true;
        }
      }
    }
    else if (iControl == CONTROL_LIST_TIMERS)
    {
      /* Get currently performed action */
      int iAction = message.GetParam1();

      /* Get currently selected item from file list */
      int iItem = m_viewControl.GetSelectedItem();

      /* Check file item is in list range and get his pointer */
      if (iItem < 0 || iItem >= (int)m_vecItems->Size()) return true;

      CFileItemPtr pItem = m_vecItems->Get(iItem);

      /* Process actions */
      if (iAction == ACTION_SHOW_INFO || iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
      {
        /* Check if "Add timer..." entry is pressed by OK, if yes
           create a new timer and open settings dialog, otherwise
           open settings for selected timer entry */
        if (pItem->m_strPath == "pvr://timers/add.timer")
        {
          CPVRTimerInfoTag *newtimer = g_PVRTimers.InstantTimer(NULL, false);
          if (!newtimer)
          {
            CLog::Log(LOGWARNING,"%s - could not create instant timer",
                __FUNCTION__);
            return false;
          }
          CFileItem *item = new CFileItem(*newtimer);

          if (ShowTimerSettings(item))
          {
            /* Add timer to backend */
            g_PVRTimers.AddTimer(*item);
            UpdateTimers();
          }
        }
        else
        {
          CFileItem fileitem(*pItem);

          if (ShowTimerSettings(&fileitem))
          {
            /* Update timer on pvr backend */
            g_PVRTimers.UpdateTimer(fileitem);
            UpdateTimers();
          }
        }

        return true;
      }
      else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      {
        //contextmenu
        OnPopupMenu(iItem);
        return true;
      }
      else if (iAction == ACTION_DELETE_ITEM)
      {
        /* Check if entry is a valid deleteable timer */
        if (pItem->GetPVRTimerInfoTag()->ClientIndex() != -1)
        {
          // prompt user for confirmation of timer deletion
          CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
          if (!pDialog)
            return false;

          pDialog->SetHeading(122);
          pDialog->SetLine(0, 19040);
          pDialog->SetLine(1, "");
          pDialog->SetLine(2, pItem->GetPVRTimerInfoTag()->Title());
          pDialog->DoModal();

          if (pDialog->IsConfirmed())
          {
            g_PVRTimers.DeleteTimer(*pItem);
            UpdateTimers();
          }
          return true;
        }
      }
    }
    else if (iControl == CONTROL_LIST_SEARCH)
    {
      /* Get currently performed action */
      int iAction = message.GetParam1();

      /* Get currently selected item from file list */
      int iItem = m_viewControl.GetSelectedItem();

      /* Check file item is in list range and get his pointer */
      if (iItem < 0 || iItem >= (int)m_vecItems->Size()) return true;

      CFileItemPtr pItem = m_vecItems->Get(iItem);

      /* Process actions */
      if (iAction == ACTION_SHOW_INFO || iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
      {
        if (pItem->m_strPath == "pvr://guide/searchresults/empty.epg")
          ShowSearchResults();
        else
          ShowEPGInfo(pItem.get());

        return true;
      }
      else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      {
        /* Show context menu */
        OnPopupMenu(iItem);
        return true;
      }
      else if (iAction == ACTION_RECORD)
      {
        if (((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->ChannelNumber() != -1)
        {
          if (((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->Timer() == NULL)
          {
            // prompt user for confirmation of channel record
            CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
            if (!pDialog)
              return false;

            CPVREpgInfoTag *epgTag = (CPVREpgInfoTag *) pItem->GetEPGInfoTag();

            pDialog->SetHeading(264);
            pDialog->SetLine(0, "");
            pDialog->SetLine(1, epgTag->Title());
            pDialog->SetLine(2, "");
            pDialog->DoModal();

            if (!pDialog->IsConfirmed())
              return true;


            CPVRTimerInfoTag *newtimer = CPVRTimerInfoTag::CreateFromEpg(*epgTag);
            CFileItem *item = new CFileItem(*newtimer);

            if (g_PVRTimers.AddTimer(*item))
              g_PVRTimers.Update();
          }
          else
            CGUIDialogOK::ShowAndGetInput(19033,19034,0,0);
        }
      }
    }
  }
  return CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowPVR::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PREVIOUS_MENU)
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  else if (action.GetID() == ACTION_PARENT_DIR)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS && m_vecItems->m_strPath != "pvr://recordings/")
      GoParentFolder();
    else
      g_windowManager.PreviousWindow();

    return true;
  }
  return CGUIMediaWindow::OnAction(action);
}

void CGUIWindowPVR::OnWindowLoaded()
{
  CGUIMediaWindow::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_LIST_TIMELINE));
  m_viewControl.AddView(GetControl(CONTROL_LIST_CHANNELS_TV));
  m_viewControl.AddView(GetControl(CONTROL_LIST_CHANNELS_RADIO));
  m_viewControl.AddView(GetControl(CONTROL_LIST_RECORDINGS));
  m_viewControl.AddView(GetControl(CONTROL_LIST_TIMERS));
  m_viewControl.AddView(GetControl(CONTROL_LIST_GUIDE_CHANNEL));
  m_viewControl.AddView(GetControl(CONTROL_LIST_GUIDE_NOW_NEXT));
  m_viewControl.AddView(GetControl(CONTROL_LIST_SEARCH));
}

void CGUIWindowPVR::OnWindowUnload()
{
  /* Save current Subwindow selected list position */
  if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM)
    m_iSelected_GUIDE = m_viewControl.GetSelectedItem();
  else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
    m_iSelected_CHANNELS_TV = m_viewControl.GetSelectedItem();
  else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
    m_iSelected_CHANNELS_RADIO = m_viewControl.GetSelectedItem();
  else if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS)
  {
    m_iSelected_RECORDINGS_Path = m_vecItems->m_strPath;
    m_iSelected_RECORDINGS = m_viewControl.GetSelectedItem();
  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)
    m_iSelected_TIMERS = m_viewControl.GetSelectedItem();
  else if (m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
    m_iSelected_SEARCH = m_viewControl.GetSelectedItem();

  m_iSavedSubTVWindow = m_iCurrSubTVWindow;
  m_iCurrSubTVWindow  = TV_WINDOW_UNKNOWN;

  m_viewControl.Reset();
  CGUIMediaWindow::OnWindowUnload();
  return;
}

void CGUIWindowPVR::OnInitWindow()
{
  /* Make sure we have active running clients, otherwise return to
   * Previous Window.
   */
  if (!g_PVRManager.HasActiveClients())
  {
    g_windowManager.PreviousWindow();
    CGUIDialogOK::ShowAndGetInput(19033,0,19045,19044);
    return;
  }

  /* This is a bad way but the SetDefaults function use the first and last
   * epg date which is not available on construction time, thats why we
   * but it to Window initialization.
   */
  if (!m_bSearchStarted)
  {
    m_bSearchStarted = true;
    m_searchfilter.Reset();
  }

  if (m_iSavedSubTVWindow == TV_WINDOW_TV_PROGRAM)
    m_viewControl.SetCurrentView(CONTROL_LIST_GUIDE_CHANNEL);
  else if (m_iSavedSubTVWindow == TV_WINDOW_CHANNELS_TV)
    m_viewControl.SetCurrentView(CONTROL_LIST_CHANNELS_TV);
  else if (m_iSavedSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
    m_viewControl.SetCurrentView(CONTROL_LIST_CHANNELS_RADIO);
  else if (m_iSavedSubTVWindow == TV_WINDOW_RECORDINGS)
    m_viewControl.SetCurrentView(CONTROL_LIST_RECORDINGS);
  else if (m_iSavedSubTVWindow == TV_WINDOW_TIMERS)
    m_viewControl.SetCurrentView(CONTROL_LIST_TIMERS);
  else if (m_iSavedSubTVWindow == TV_WINDOW_SEARCH)
    m_viewControl.SetCurrentView(CONTROL_LIST_SEARCH);

  CGUIMediaWindow::OnInitWindow();
  return;
}

void CGUIWindowPVR::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  /* Check file item is in list range and get his pointer */
  if (itemNumber < 0 || itemNumber >= (int)m_vecItems->Size()) return;

  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV || m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
  {
    if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
      return;

    /* check that file item is in list and get his pointer*/
    if (pItem->m_strPath == "pvr://channels/.add.channel")
    {
      /* If yes show only "New Channel" on context menu */
      buttons.Add(CONTEXT_BUTTON_ADD, 19046);               /* Add new channel */
    }
    else
    {
      buttons.Add(CONTEXT_BUTTON_INFO, 19047);              /* Channel info button */
      buttons.Add(CONTEXT_BUTTON_FIND, 19003);              /* Find similar program */
      buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 19000);         /* switch to channel */
      buttons.Add(CONTEXT_BUTTON_SET_THUMB, 20019);         /* Set icon */
      buttons.Add(CONTEXT_BUTTON_GROUP_MANAGER, 19048);     /* Group managment */
      buttons.Add(CONTEXT_BUTTON_HIDE, m_bShowHiddenChannels ? 19049 : 19054);        /* HIDE CHANNEL */

      if (m_vecItems->Size() > 1 && !m_bShowHiddenChannels)
        buttons.Add(CONTEXT_BUTTON_MOVE, 116);              /* Move channel up or down */

      if ((m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV && g_PVRChannelGroups.GetGroupAll(false)->GetNumHiddenChannels() > 0) ||
          (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO && g_PVRChannelGroups.GetGroupAll(true)->GetNumHiddenChannels() > 0) ||
          m_bShowHiddenChannels)
        buttons.Add(CONTEXT_BUTTON_SHOW_HIDDEN, m_bShowHiddenChannels ? 19050 : 19051);  /* SHOW HIDDEN CHANNELS */

      CGUIMediaWindow::GetContextButtons(itemNumber, buttons);

      if (g_PVRManager.HasMenuHooks(pItem->GetPVRChannelInfoTag()->ClientID()))
        buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);        /* PVR client specific action */
    }
  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS)           /* Add recordings context buttons */
  {
    buttons.Add(CONTEXT_BUTTON_INFO, 19053);              /* Get Information of this recording */
    buttons.Add(CONTEXT_BUTTON_FIND, 19003);              /* Find similar program */
    buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 12021);         /* Play this recording */
//            buttons.Add(CONTEXT_BUTTON_RESUME_ITEM, 12022);
    // Update sort by button
//    if (m_guiState->GetSortMethod()!=SORT_METHOD_NONE)
//    {
//      CStdString sortLabel;
//      sortLabel.Format(g_localizeStrings.Get(550).c_str(), g_localizeStrings.Get(m_guiState->GetSortMethodLabel()).c_str());
//      buttons.Add(CONTEXT_BUTTON_SORTBY, sortLabel);   /* Sort method */
//
//      if (m_guiState->GetDisplaySortOrder()==SORT_ORDER_ASC)
//        buttons.Add(CONTEXT_BUTTON_SORTASC, 584);        /* Sort up or down */
//      else
//        buttons.Add(CONTEXT_BUTTON_SORTASC, 585);        /* Sort up or down */
//    }

    buttons.Add(CONTEXT_BUTTON_RENAME, 118);              /* Rename this recording */
    buttons.Add(CONTEXT_BUTTON_DELETE, 117);              /* Delete this recording */
  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)           /* Add timer context buttons */
  {
    /* Check for a empty file item list, means only a
        file item with the name "Add timer..." is present */
    if (pItem->m_strPath == "pvr://timers/add.timer")
    {
      /* If yes show only "New Timer" on context menu */
      buttons.Add(CONTEXT_BUTTON_ADD, 19056);             /* NEW TIMER */
      if (m_vecItems->Size() > 1)
      {
        buttons.Add(CONTEXT_BUTTON_SORTBY_NAME, 103);     /* Sort by Name */
        buttons.Add(CONTEXT_BUTTON_SORTBY_DATE, 104);     /* Sort by Date */
      }
    }
    else
    {
      /* If any timers are present show more */
      buttons.Add(CONTEXT_BUTTON_EDIT, 19057);            /* Edit Timer */
      buttons.Add(CONTEXT_BUTTON_ADD, 19056);             /* NEW TIMER */
      buttons.Add(CONTEXT_BUTTON_ACTIVATE, 19058);        /* ON/OFF */
      buttons.Add(CONTEXT_BUTTON_RENAME, 118);            /* Rename Timer */
      buttons.Add(CONTEXT_BUTTON_DELETE, 117);            /* Delete Timer */
      buttons.Add(CONTEXT_BUTTON_SORTBY_NAME, 103);       /* Sort by Name */
      buttons.Add(CONTEXT_BUTTON_SORTBY_DATE, 104);       /* Sort by Date */
      if (g_PVRManager.HasMenuHooks(pItem->GetPVRTimerInfoTag()->ClientID()))
        buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);    /* PVR client specific action */
    }
  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM)
  {
    if (pItem->GetEPGInfoTag()->End() > CDateTime::GetCurrentDateTime())
    {
      if (((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->Timer() == NULL)
      {
        if (pItem->GetEPGInfoTag()->Start() < CDateTime::GetCurrentDateTime())
        {
          buttons.Add(CONTEXT_BUTTON_START_RECORD, 264);             /* RECORD programme */
        }
        else
        {
          buttons.Add(CONTEXT_BUTTON_START_RECORD, 19061);
        }
      }
      else
      {
        if (pItem->GetEPGInfoTag()->Start() < CDateTime::GetCurrentDateTime())
        {
          buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19059);
        }
        else
        {
          buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19060);
        }
      }
    }

    buttons.Add(CONTEXT_BUTTON_INFO, 658);               /* Epg info button */
    buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 19000);        /* Switch channel */
    buttons.Add(CONTEXT_BUTTON_FIND, 19003);             /* Find similar program */
    if (m_iGuideView == GUIDE_VIEW_TIMELINE)
    {
      buttons.Add(CONTEXT_BUTTON_BEGIN, 19063);          /* Go to begin */
      buttons.Add(CONTEXT_BUTTON_END, 19064);            /* Go to end */
    }
    if (g_PVRManager.HasMenuHooks(((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->ClientID()))
      buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);        /* PVR client specific action */
  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
  {
    if (pItem->GetLabel() != g_localizeStrings.Get(19027))
    {
      if (pItem->GetEPGInfoTag()->End() > CDateTime::GetCurrentDateTime())
      {
        if (((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->Timer() == NULL)
        {
          if (pItem->GetEPGInfoTag()->Start() < CDateTime::GetCurrentDateTime())
            buttons.Add(CONTEXT_BUTTON_START_RECORD, 264);             /* RECORD programme */
          else
            buttons.Add(CONTEXT_BUTTON_START_RECORD, 19061);    /* Create a Timer */
        }
        else
        {
          if (pItem->GetEPGInfoTag()->Start() < CDateTime::GetCurrentDateTime())
            buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19059);     /* Stop recording */
          else
            buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 19060);     /* Delete Timer */
        }
      }

      buttons.Add(CONTEXT_BUTTON_INFO, 658);              /* Epg info button */
      buttons.Add(CONTEXT_BUTTON_SORTBY_CHANNEL, 19062);  /* Sort by channel */
      buttons.Add(CONTEXT_BUTTON_SORTBY_NAME, 103);       /* Sort by Name */
      buttons.Add(CONTEXT_BUTTON_SORTBY_DATE, 104);       /* Sort by Date */
      buttons.Add(CONTEXT_BUTTON_CLEAR, 20375);           /* Clear search results */
      if (g_PVRManager.HasMenuHooks(((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->ClientID()))
        buttons.Add(CONTEXT_BUTTON_MENU_HOOKS, 19195);        /* PVR client specific action */
    }
  }
}

bool CGUIWindowPVR::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  /* Check file item is in list range and get his pointer */
  if (itemNumber < 0 || itemNumber >= (int)m_vecItems->Size()) return false;

  CFileItemPtr pItem = m_vecItems->Get(itemNumber);

  if (button == CONTEXT_BUTTON_PLAY_ITEM)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS)
    {
      return PlayFile(pItem.get(), false);
    }

    if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV ||
        m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
    {
      if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
        g_PVRManager.SetPlayingGroup(m_iCurrentTVGroup);
      if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
        g_PVRManager.SetPlayingGroup(m_iCurrentRadioGroup);

      return PlayFile(pItem.get(), g_guiSettings.GetBool("pvrplayback.playminimized"));
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM)
    {
      const CPVRChannelGroup *channels = g_PVRChannelGroups.GetGroupAll(((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->IsRadio());
      if (!g_application.PlayFile(CFileItem(*channels->at(((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->ChannelNumber()-1))))
      {
        CGUIDialogOK::ShowAndGetInput(19033,0,19035,0);
        return false;
      }
      return true;
    }
  }
  else if (button == CONTEXT_BUTTON_MOVE)
  {
    CStdString strIndex;
    strIndex.Format("%i", pItem->GetPVRChannelInfoTag()->ChannelNumber());
    CGUIDialogNumeric::ShowAndGetNumber(strIndex, g_localizeStrings.Get(19052));
    int newIndex = atoi(strIndex.c_str());

    if (newIndex != pItem->GetPVRChannelInfoTag()->ChannelNumber())
    {
      ((CPVRChannelGroup *) g_PVRChannelGroups.GetGroupAll(m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO))->MoveChannel(pItem->GetPVRChannelInfoTag()->ChannelNumber(), newIndex);
      if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
        UpdateChannelsTV();
      else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
        UpdateChannelsRadio();
    }
  }
  else if (button == CONTEXT_BUTTON_HIDE)
  {
    // prompt user for confirmation of channel hide
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

    if (pDialog)
    {
      pDialog->SetHeading(19039);
      pDialog->SetLine(0, "");
      pDialog->SetLine(1, pItem->GetPVRChannelInfoTag()->ChannelName());
      pDialog->SetLine(2, "");
      pDialog->DoModal();

      if (!pDialog->IsConfirmed()) return false;

      if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
      {
        ((CPVRChannelGroup *) g_PVRChannelGroups.GetGroupAll(false))->HideChannel(pItem->GetPVRChannelInfoTag());
        UpdateChannelsTV();
      }
      else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
      {
        ((CPVRChannelGroup *) g_PVRChannelGroups.GetGroupAll(true))->HideChannel(pItem->GetPVRChannelInfoTag());
        UpdateChannelsRadio();
      }
    }
  }
  else if (button == CONTEXT_BUTTON_SHOW_HIDDEN)
  {
    if (m_bShowHiddenChannels)
      m_bShowHiddenChannels = false;
    else
      m_bShowHiddenChannels = true;

    if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
      UpdateChannelsTV();
    else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
      UpdateChannelsRadio();
  }
  else if (button == CONTEXT_BUTTON_SET_THUMB)
  {
    if (g_settings.GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
      return false;
    else if (!g_passwordManager.IsMasterLockUnlocked(true))
      return false;

    // setup our thumb list
    CFileItemList items;

    // add the current thumb, if available
    if (!pItem->GetPVRChannelInfoTag()->IconPath().IsEmpty())
    {
      CFileItemPtr current(new CFileItem("thumb://Current", false));
      current->SetThumbnailImage(pItem->GetPVRChannelInfoTag()->IconPath());
      current->SetLabel(g_localizeStrings.Get(20016));
      items.Add(current);
    }
    else if (pItem->HasThumbnail())
    { // already have a thumb that the share doesn't know about - must be a local one, so we mayaswell reuse it.
      CFileItemPtr current(new CFileItem("thumb://Current", false));
      current->SetThumbnailImage(pItem->GetThumbnailImage());
      current->SetLabel(g_localizeStrings.Get(20016));
      items.Add(current);
    }

    // and add a "no thumb" entry as well
    CFileItemPtr nothumb(new CFileItem("thumb://None", false));
    nothumb->SetIconImage(pItem->GetIconImage());
    nothumb->SetLabel(g_localizeStrings.Get(20018));
    items.Add(nothumb);

    CStdString strThumb;
    VECSOURCES shares;
    if (g_guiSettings.GetString("pvrmenu.iconpath") != "")
    {
      CMediaSource share1;
      share1.strPath = g_guiSettings.GetString("pvrmenu.iconpath");
      share1.strName = g_localizeStrings.Get(19018);
      shares.push_back(share1);
    }
    g_mediaManager.GetLocalDrives(shares);
    if (!CGUIDialogFileBrowser::ShowAndGetImage(items, shares, g_localizeStrings.Get(1030), strThumb))
      return false;

    if (strThumb == "thumb://Current")
      return true;

    if (strThumb == "thumb://None")
      strThumb = "";

    if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
    {
      pItem->GetPVRChannelInfoTag()->SetIconPath(strThumb, true);
      UpdateChannelsTV();
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
    {
      pItem->GetPVRChannelInfoTag()->SetIconPath(strThumb, true);
      UpdateChannelsRadio();
    }

    return true;
  }
  else if (button == CONTEXT_BUTTON_EDIT)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)
    {
      CFileItem fileitem(*pItem);

      if (ShowTimerSettings(&fileitem))
      {
        g_PVRTimers.UpdateTimer(fileitem);
        UpdateTimers();
      }
    }

    return true;
  }
  else if (button == CONTEXT_BUTTON_ADD)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO ||
        m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
    {
      CGUIDialogOK::ShowAndGetInput(19033,0,19038,0);
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)
    {
      CPVRTimerInfoTag *newtimer = g_PVRTimers.InstantTimer(NULL, false);
      CFileItem *item = new CFileItem(*newtimer);

      if (ShowTimerSettings(item))
      {
        g_PVRTimers.AddTimer(*item);
        UpdateTimers();
      }

      return true;
    }
  }
  else if (button == CONTEXT_BUTTON_ACTIVATE)
  {
    int return_str_id;

    if (pItem->GetPVRTimerInfoTag()->Active() == true)
    {
      pItem->GetPVRTimerInfoTag()->SetActive(false);
      return_str_id = 13106;
    }
    else
    {
      pItem->GetPVRTimerInfoTag()->SetActive(true);
      return_str_id = 305;
    }

    CGUIDialogOK::ShowAndGetInput(19033, 19040, 0, return_str_id);

    g_PVRTimers.UpdateTimer(*pItem);
    UpdateTimers(); /** Force list update **/
    return true;
  }
  else if (button == CONTEXT_BUTTON_RENAME)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)
    {
      CStdString strNewName = pItem->GetPVRTimerInfoTag()->Title();
      if (CGUIDialogKeyboard::ShowAndGetInput(strNewName, g_localizeStrings.Get(19042), false))
      {
        g_PVRTimers.RenameTimer(*pItem, strNewName);
        UpdateTimers();
      }

      return true;
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS)
    {
      CStdString strNewName = pItem->GetPVRRecordingInfoTag()->Title();
      if (CGUIDialogKeyboard::ShowAndGetInput(strNewName, g_localizeStrings.Get(19041), false))
      {
        if (CPVRRecordings::RenameRecording(*pItem, strNewName))
        {
          UpdateRecordings();
        }
      }
    }
  }
  else if (button == CONTEXT_BUTTON_DELETE)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)
    {
      // prompt user for confirmation of timer deletion
      CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

      if (pDialog)
      {
        pDialog->SetHeading(122);
        pDialog->SetLine(0, 19040);
        pDialog->SetLine(1, "");
        pDialog->SetLine(2, pItem->GetPVRTimerInfoTag()->Title());
        pDialog->DoModal();

        if (!pDialog->IsConfirmed()) return false;

        g_PVRTimers.DeleteTimer(*pItem);

        UpdateTimers();
      }

      return true;
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS)
    {
      CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

      if (pDialog)
      {
        pDialog->SetHeading(122);
        pDialog->SetLine(0, 19043);
        pDialog->SetLine(1, "");
        pDialog->SetLine(2, pItem->GetPVRRecordingInfoTag()->Title());
        pDialog->DoModal();

        if (!pDialog->IsConfirmed()) return false;

        if (CPVRRecordings::DeleteRecording(*pItem))
        {
          PVRRecordings.Update(true);
          UpdateRecordings();
        }
      }

      return true;
    }
  }
  else if (button == CONTEXT_BUTTON_INFO)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV ||
        m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO ||
        m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM ||
        m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
    {
      ShowEPGInfo(pItem.get());
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS)
    {
      ShowRecordingInfo(pItem.get());
    }
  }
  else if (button == CONTEXT_BUTTON_START_RECORD)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM || m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
    {
      int iChannel = ((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->ChannelNumber();

      if (iChannel != -1)
      {
        if (((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->Timer() == NULL)
        {
          // prompt user for confirmation of channel record
          CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

          if (pDialog)
          {
            pDialog->SetHeading(264);
            pDialog->SetLine(0, ((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->ChannelName());
            pDialog->SetLine(1, "");
            pDialog->SetLine(2, pItem->GetEPGInfoTag()->Title());
            pDialog->DoModal();

            if (pDialog->IsConfirmed())
            {
              CPVREpgInfoTag *tag = (CPVREpgInfoTag *) pItem->GetEPGInfoTag();
              CPVRTimerInfoTag *newtimer = CPVRTimerInfoTag::CreateFromEpg(*tag);
              CFileItem *item = new CFileItem(*newtimer);

              if (g_PVRTimers.AddTimer(*item))
                g_PVRTimers.Update();
            }
          }
        }
        else
        {
          CGUIDialogOK::ShowAndGetInput(19033,19034,0,0);
        }
      }
    }
  }
  else if (button == CONTEXT_BUTTON_STOP_RECORD)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM || m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
    {
      int iChannel = ((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->ChannelNumber();

      if (iChannel != -1)
      {
        if (((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->Timer() != NULL)
        {
          CFileItemList timerlist;

          if (g_PVRTimers.GetTimers(&timerlist) > 0)
          {
            for (int i = 0; i < timerlist.Size(); ++i)
            {
              if ((timerlist[i]->GetPVRTimerInfoTag()->Number() == ((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->ChannelNumber()) &&
                  (timerlist[i]->GetPVRTimerInfoTag()->Start() <= pItem->GetEPGInfoTag()->Start()) &&
                  (timerlist[i]->GetPVRTimerInfoTag()->Stop() >= pItem->GetEPGInfoTag()->End()) &&
                  (timerlist[i]->GetPVRTimerInfoTag()->IsRepeating() != true))
              {
                if (g_PVRTimers.DeleteTimer(*timerlist[i]))
                  g_PVRTimers.Update();
              }
            }
          }
        }
      }
    }
  }
  else if (button == CONTEXT_BUTTON_GROUP_MANAGER)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
      ShowGroupManager(false);
    else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
      ShowGroupManager(true);
  }
  else if (button == CONTEXT_BUTTON_RESUME_ITEM)
  {

  }
  else if (button == CONTEXT_BUTTON_CLEAR)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
    {
      m_bSearchStarted = false;
      m_bSearchConfirmed = false;
      m_searchfilter.Reset();
      UpdateSearch();
    }
  }
  else if (button == CONTEXT_BUTTON_SORTASC) // sort asc
  {
    if (m_guiState.get())
      m_guiState->SetNextSortOrder();
    UpdateFileList();
    return true;
  }
  else if (button == CONTEXT_BUTTON_SORTBY) // sort by
  {
    if (m_guiState.get())
      m_guiState->SetNextSortMethod();
    UpdateFileList();
    return true;
  }
  else if (button == CONTEXT_BUTTON_SORTBY_CHANNEL)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
    {
      if (m_iSortMethod_SEARCH != SORT_METHOD_CHANNEL)
      {
        m_iSortMethod_SEARCH = SORT_METHOD_CHANNEL;
        m_iSortOrder_SEARCH  = SORT_ORDER_ASC;
      }
      else
      {
        m_iSortOrder_SEARCH = m_iSortOrder_SEARCH == SORT_ORDER_ASC ? SORT_ORDER_DESC : SORT_ORDER_ASC;
      }
      UpdateSearch();
    }
  }
  else if (button == CONTEXT_BUTTON_SORTBY_NAME)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)
    {
      if (m_iSortMethod_TIMERS != SORT_METHOD_LABEL)
      {
        m_iSortMethod_TIMERS = SORT_METHOD_LABEL;
        m_iSortOrder_TIMERS  = SORT_ORDER_ASC;
      }
      else
      {
        m_iSortOrder_TIMERS = m_iSortOrder_TIMERS == SORT_ORDER_ASC ? SORT_ORDER_DESC : SORT_ORDER_ASC;
      }
      UpdateTimers();
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
    {
      if (m_iSortMethod_SEARCH != SORT_METHOD_LABEL)
      {
        m_iSortMethod_SEARCH = SORT_METHOD_LABEL;
        m_iSortOrder_SEARCH  = SORT_ORDER_ASC;
      }
      else
      {
        m_iSortOrder_SEARCH = m_iSortOrder_SEARCH == SORT_ORDER_ASC ? SORT_ORDER_DESC : SORT_ORDER_ASC;
      }
      UpdateSearch();
    }
  }
  else if (button == CONTEXT_BUTTON_SORTBY_DATE)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)
    {
      if (m_iSortMethod_TIMERS != SORT_METHOD_DATE)
      {
        m_iSortMethod_TIMERS = SORT_METHOD_DATE;
        m_iSortOrder_TIMERS  = SORT_ORDER_ASC;
      }
      else
      {
        m_iSortOrder_TIMERS = m_iSortOrder_TIMERS == SORT_ORDER_ASC ? SORT_ORDER_DESC : SORT_ORDER_ASC;
      }
      UpdateTimers();
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
    {
      if (m_iSortMethod_SEARCH != SORT_METHOD_DATE)
      {
        m_iSortMethod_SEARCH = SORT_METHOD_DATE;
        m_iSortOrder_SEARCH  = SORT_ORDER_ASC;
      }
      else
      {
        m_iSortOrder_SEARCH = m_iSortOrder_SEARCH == SORT_ORDER_ASC ? SORT_ORDER_DESC : SORT_ORDER_ASC;
      }
      UpdateSearch();
    }
  }
  else if (button == CONTEXT_BUTTON_BEGIN)
  {
    m_guideGrid->GoToBegin();
  }
  else if (button == CONTEXT_BUTTON_END)
  {
    m_guideGrid->GoToEnd();
  }
  else if (button == CONTEXT_BUTTON_FIND)
  {
    m_searchfilter.Reset();
    if (pItem->IsEPG())
    {
      m_searchfilter.m_strSearchTerm = "\"" + pItem->GetEPGInfoTag()->Title() + "\"";
    }
    else if (pItem->IsPVRChannel())
    {
      m_searchfilter.m_strSearchTerm = "\"" + pItem->GetPVRChannelInfoTag()->GetEPGNow()->Title() + "\"";
    }
    else if (pItem->IsPVRRecording())
    {
      m_searchfilter.m_strSearchTerm = "\"" + pItem->GetPVRRecordingInfoTag()->Title() + "\"";
    }
    m_bSearchConfirmed = true;
    SET_CONTROL_FOCUS(CONTROL_BTNSEARCH, 0);
    UpdateSearch();
    SET_CONTROL_FOCUS(CONTROL_LIST_SEARCH, 0);
  }
  else if (button == CONTEXT_BUTTON_MENU_HOOKS)
  {
    if (pItem->IsEPG())
    {
      g_PVRManager.ProcessMenuHooks(((CPVREpgInfoTag *) pItem->GetEPGInfoTag())->ChannelTag()->ClientID());
    }
    else if (pItem->IsPVRChannel())
    {
      g_PVRManager.ProcessMenuHooks(pItem->GetPVRChannelInfoTag()->ClientID());
    }
    else if (pItem->IsPVRRecording())
    {
      g_PVRManager.ProcessMenuHooks(pItem->GetPVRRecordingInfoTag()->ClientID());
    }
    else if (pItem->IsPVRTimer())
    {
      g_PVRManager.ProcessMenuHooks(pItem->GetPVRTimerInfoTag()->ClientID());
    }
  }

  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

void CGUIWindowPVR::ShowEPGInfo(CFileItem *item)
{
  /* Check item is TV epg or channel information tag */
  if (item->IsEPG())
  {
    /* Load programme info dialog */
    CGUIDialogPVRGuideInfo* pDlgInfo = (CGUIDialogPVRGuideInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_INFO);
    if (!pDlgInfo)
      return;

    /* inform dialog about the file item */
    pDlgInfo->SetProgInfo(item);
//    PVREpgs.SetVariableData(m_vecItems);

    /* Open dialog window */
    pDlgInfo->DoModal();
  }
  else if (item->IsPVRChannel())
  {
    const CPVREpgInfoTag *epgnow = item->GetPVRChannelInfoTag()->GetEPGNow();
    if (!epgnow)
    {
      CGUIDialogOK::ShowAndGetInput(19033,0,19055,0);
      return;
    }

    CFileItem *itemNow  = new CFileItem(*epgnow);

    /* Load programme info dialog */
    CGUIDialogPVRGuideInfo* pDlgInfo = (CGUIDialogPVRGuideInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_INFO);
    if (!pDlgInfo)
      return;

    /* inform dialog about the file item */
    pDlgInfo->SetProgInfo(itemNow);

    /* Open dialog window */
    pDlgInfo->DoModal();
  }
  else
    CLog::Log(LOGERROR, "CGUIWindowPVR: Can't open programme info dialog, no epg or channel info tag!");
}

void CGUIWindowPVR::ShowRecordingInfo(CFileItem *item)
{
  /* Check item is TV record information tag */
  if (!item->IsPVRRecording())
  {
    CLog::Log(LOGERROR, "CGUIWindowPVR: Can't open recording info dialog, no record info tag!");
    return;
  }

  /* Load record info dialog */
  CGUIDialogPVRRecordingInfo* pDlgInfo = (CGUIDialogPVRRecordingInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_RECORDING_INFO);

  if (!pDlgInfo)
    return;

  /* inform dialog about the file item */
  pDlgInfo->SetRecording(item);

  /* Open dialog window */
  pDlgInfo->DoModal();

  /* Return to caller */
  return;
}

bool CGUIWindowPVR::ShowTimerSettings(CFileItem *item)
{
  /* Check item is TV timer information tag */
  if (!item->IsPVRTimer())
  {
    CLog::Log(LOGERROR, "CGUIWindowPVR: Can't open timer settings dialog, no timer info tag!");
    return false;
  }

  /* Load timer settings dialog */
  CGUIDialogPVRTimerSettings* pDlgInfo = (CGUIDialogPVRTimerSettings*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_TIMER_SETTING);

  if (!pDlgInfo)
    return false;

  /* inform dialog about the file item */
  pDlgInfo->SetTimer(item);

  /* Open dialog window */
  pDlgInfo->DoModal();

  /* Get modify flag from window and return it to caller */
  return pDlgInfo->GetOK();
}

void CGUIWindowPVR::ShowGroupManager(bool IsRadio)
{
  /* Load group manager dialog */
  CGUIDialogPVRGroupManager* pDlgInfo = (CGUIDialogPVRGroupManager*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GROUP_MANAGER);

  if (!pDlgInfo)
    return;

  pDlgInfo->SetRadio(IsRadio);

  /* Open dialog window */
  pDlgInfo->DoModal();

  return;
}

void CGUIWindowPVR::ShowSearchResults()
{
  /* Load timer settings dialog */
  CGUIDialogPVRGuideSearch* pDlgInfo = (CGUIDialogPVRGuideSearch*)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_GUIDE_SEARCH);

  if (!pDlgInfo)
    return;

  pDlgInfo->SetFilterData(&m_searchfilter);

  /* Open dialog window */
  pDlgInfo->DoModal();

  if (pDlgInfo->IsConfirmed())
  {
    m_bSearchConfirmed = true;
    UpdateSearch();
  }

  return;
}

void CGUIWindowPVR::UpdateGuide()
{
  CPVRChannel CurrentChannel;
  g_PVRManager.GetCurrentChannel(&CurrentChannel);

  m_vecItems->Clear();

  if (m_iGuideView == GUIDE_VIEW_CHANNEL)
  {
    m_guideGrid = NULL;
    m_viewControl.SetCurrentView(CONTROL_LIST_GUIDE_CHANNEL);

    SET_CONTROL_LABEL(CONTROL_BTNGUIDE, g_localizeStrings.Get(19029));
    SET_CONTROL_LABEL(CONTROL_LABELGROUP, CurrentChannel.ChannelName().c_str());

    if (CurrentChannel.GetEPG(m_vecItems) == 0)
    {
      CFileItemPtr item;
      item.reset(new CFileItem("pvr://guide/" + CurrentChannel.ChannelName() + "/empty.epg", false));
      item->SetLabel(g_localizeStrings.Get(19028));
      item->SetLabelPreformated(true);
      m_vecItems->Add(item);
    }
    m_viewControl.SetItems(*m_vecItems);
  }
  else if (m_iGuideView == GUIDE_VIEW_NOW)
  {
    m_guideGrid = NULL;

    m_viewControl.SetCurrentView(CONTROL_LIST_GUIDE_NOW_NEXT);

    SET_CONTROL_LABEL(CONTROL_BTNGUIDE, g_localizeStrings.Get(19029) + ": " + g_localizeStrings.Get(19030));
    SET_CONTROL_LABEL(CONTROL_LABELGROUP, g_localizeStrings.Get(19030));

    if (g_PVREpgContainer.GetEPGNow(m_vecItems, CurrentChannel.IsRadio()) == 0)
    {
      CFileItemPtr item;
      item.reset(new CFileItem("pvr://guide/now/empty.epg", false));
      item->SetLabel(g_localizeStrings.Get(19028));
      item->SetLabelPreformated(true);
      m_vecItems->Add(item);
    }
    m_viewControl.SetItems(*m_vecItems);
  }
  else if (m_iGuideView == GUIDE_VIEW_NEXT)
  {
    m_guideGrid = NULL;

    m_viewControl.SetCurrentView(CONTROL_LIST_GUIDE_NOW_NEXT);

    SET_CONTROL_LABEL(CONTROL_BTNGUIDE, g_localizeStrings.Get(19029) + ": " + g_localizeStrings.Get(19031));
    SET_CONTROL_LABEL(CONTROL_LABELGROUP, g_localizeStrings.Get(19031));

    if (g_PVREpgContainer.GetEPGNext(m_vecItems, CurrentChannel.IsRadio()) == 0)
    {
      CFileItemPtr item;
      item.reset(new CFileItem("pvr://guide/next/empty.epg", false));
      item->SetLabel(g_localizeStrings.Get(19028));
      item->SetLabelPreformated(true);
      m_vecItems->Add(item);
    }
    m_viewControl.SetItems(*m_vecItems);
  }
  else if (m_iGuideView == GUIDE_VIEW_TIMELINE)
  {
    SET_CONTROL_LABEL(CONTROL_BTNGUIDE, g_localizeStrings.Get(19029) + ": " + g_localizeStrings.Get(19032));
    SET_CONTROL_LABEL(CONTROL_LABELGROUP, g_localizeStrings.Get(19032));

    if (g_PVREpgContainer.GetEPGAll(m_vecItems, CurrentChannel.IsRadio()) > 0)
    {
      CDateTime now = CDateTime::GetCurrentDateTime();
      CDateTime m_gridStart = now - CDateTimeSpan(0, 0, 0, (now.GetMinute() % 30) * 60 + now.GetSecond()) - CDateTimeSpan(0, g_guiSettings.GetInt("epg.lingertime") / 60, g_guiSettings.GetInt("epg.lingertime") % 60, 0);
      CDateTime m_gridEnd = m_gridStart + CDateTimeSpan(g_guiSettings.GetInt("epg.daystodisplay"), 0, 0, 0);
      m_guideGrid = (CGUIEPGGridContainer*)GetControl(CONTROL_LIST_TIMELINE);
      m_guideGrid->SetStartEnd(m_gridStart, m_gridEnd);
      m_viewControl.SetCurrentView(CONTROL_LIST_TIMELINE);

//      m_viewControl.SetSelectedItem(m_iSelected_GUIDE);
    }
  }

  SET_CONTROL_LABEL(CONTROL_LABELHEADER, g_localizeStrings.Get(19029));
}

void CGUIWindowPVR::UpdateChannelsTV()
{
  m_vecItems->Clear();
  m_viewControl.SetCurrentView(CONTROL_LIST_CHANNELS_TV);
  if (!m_bShowHiddenChannels)
    m_vecItems->m_strPath = "pvr://channels/tv/" + g_PVRChannelGroups.GetTV()->GetGroupName(m_iCurrentTVGroup) + "/";
  else
    m_vecItems->m_strPath = "pvr://channels/tv/.hidden/";
  Update(m_vecItems->m_strPath);

  if (m_vecItems->Size() == 0)
  {
    if (m_bShowHiddenChannels)
    {
      m_bShowHiddenChannels = false;
      UpdateChannelsTV();
      return;
    }
    else if (m_iCurrentTVGroup != -1)
    {
      m_iCurrentTVGroup = g_PVRChannelGroups.GetTV()->GetNextGroupID(m_iCurrentTVGroup);
      UpdateChannelsTV();
      return;
    }
  }

  m_viewControl.SetSelectedItem(m_iSelected_CHANNELS_TV);

  SET_CONTROL_LABEL(CONTROL_LABELHEADER, g_localizeStrings.Get(19023));
  if (m_bShowHiddenChannels)
    SET_CONTROL_LABEL(CONTROL_LABELGROUP, g_localizeStrings.Get(19022));
  else
    SET_CONTROL_LABEL(CONTROL_LABELGROUP, g_PVRChannelGroups.GetTV()->GetGroupName(m_iCurrentTVGroup));
}

void CGUIWindowPVR::UpdateChannelsRadio()
{
  m_vecItems->Clear();
  m_viewControl.SetCurrentView(CONTROL_LIST_CHANNELS_RADIO);
  if (!m_bShowHiddenChannels)
    m_vecItems->m_strPath = "pvr://channels/radio/" + g_PVRChannelGroups.GetRadio()->GetGroupName(m_iCurrentRadioGroup) + "/";
  else
    m_vecItems->m_strPath = "pvr://channels/radio/.hidden/";
  Update(m_vecItems->m_strPath);

  if (m_vecItems->Size() == 0)
  {
    if (m_bShowHiddenChannels)
    {
      m_bShowHiddenChannels = false;
      UpdateChannelsRadio();
      return;
    }
    else if (m_iCurrentRadioGroup != -1)
    {
      m_iCurrentRadioGroup = g_PVRChannelGroups.GetRadio()->GetNextGroupID(m_iCurrentRadioGroup);
      UpdateChannelsRadio();
      return;
    }
  }

  m_viewControl.SetSelectedItem(m_iSelected_CHANNELS_RADIO);

  SET_CONTROL_LABEL(CONTROL_LABELHEADER, g_localizeStrings.Get(19024));
  if (m_bShowHiddenChannels)
    SET_CONTROL_LABEL(CONTROL_LABELGROUP, g_localizeStrings.Get(19022));
  else
    SET_CONTROL_LABEL(CONTROL_LABELGROUP, g_PVRChannelGroups.GetRadio()->GetGroupName(m_iCurrentRadioGroup));
}

void CGUIWindowPVR::UpdateRecordings()
{
  m_vecItems->Clear();
  m_viewControl.SetCurrentView(CONTROL_LIST_RECORDINGS);
  m_vecItems->m_strPath = "pvr://recordings/";
  Update(m_iSelected_RECORDINGS_Path);
  m_viewControl.SetSelectedItem(m_iSelected_RECORDINGS);

  SET_CONTROL_LABEL(CONTROL_LABELHEADER, g_localizeStrings.Get(19017));
  SET_CONTROL_LABEL(CONTROL_LABELGROUP, "");
}

void CGUIWindowPVR::UpdateTimers()
{
  m_vecItems->Clear();
  m_viewControl.SetCurrentView(CONTROL_LIST_TIMERS);
  m_vecItems->m_strPath = "pvr://timers/";
  Update(m_vecItems->m_strPath);
  m_vecItems->Sort(m_iSortMethod_TIMERS, m_iSortOrder_TIMERS);
  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(m_iSelected_TIMERS);

  SET_CONTROL_LABEL(CONTROL_LABELHEADER, g_localizeStrings.Get(19025));
  SET_CONTROL_LABEL(CONTROL_LABELGROUP, "");
}

void CGUIWindowPVR::UpdateSearch()
{
  m_vecItems->Clear();
  m_viewControl.SetCurrentView(CONTROL_LIST_SEARCH);

  if (m_bSearchConfirmed)
  {
    CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (dlgProgress)
    {
      dlgProgress->SetHeading(194);
      dlgProgress->SetLine(0, m_searchfilter.m_strSearchTerm);
      dlgProgress->SetLine(1, "");
      dlgProgress->SetLine(2, "");
      dlgProgress->StartModal();
      dlgProgress->Progress();
    }

    g_PVREpgContainer.GetEPGSearch(m_vecItems, m_searchfilter);
    if (dlgProgress)
      dlgProgress->Close();

    if (m_vecItems->Size() == 0)
    {
      CGUIDialogOK::ShowAndGetInput(194, 284, 0, 0);
      m_bSearchConfirmed = false;
    }
  }

  if (m_vecItems->Size() == 0)
  {
    CFileItemPtr item;
    item.reset(new CFileItem("pvr://guide/searchresults/empty.epg", false));
    item->SetLabel(g_localizeStrings.Get(19027));
    item->SetLabelPreformated(true);
    m_vecItems->Add(item);
  }
  else
  {
    m_vecItems->Sort(m_iSortMethod_SEARCH, m_iSortOrder_SEARCH);
  }

  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(m_iSelected_SEARCH);

  SET_CONTROL_LABEL(CONTROL_LABELHEADER, g_localizeStrings.Get(283));
  SET_CONTROL_LABEL(CONTROL_LABELGROUP, "");
}

void CGUIWindowPVR::UpdateButtons()
{
  if (m_iGuideView == GUIDE_VIEW_CHANNEL)
    SET_CONTROL_LABEL(CONTROL_BTNGUIDE, g_localizeStrings.Get(19029));
  else if (m_iGuideView == GUIDE_VIEW_NOW)
    SET_CONTROL_LABEL(CONTROL_BTNGUIDE, g_localizeStrings.Get(19029) + ": " + g_localizeStrings.Get(19030));
  else if (m_iGuideView == GUIDE_VIEW_NEXT)
    SET_CONTROL_LABEL(CONTROL_BTNGUIDE, g_localizeStrings.Get(19029) + ": " + g_localizeStrings.Get(19031));
  else if (m_iGuideView == GUIDE_VIEW_TIMELINE)
    SET_CONTROL_LABEL(CONTROL_BTNGUIDE, g_localizeStrings.Get(19029) + ": " + g_localizeStrings.Get(19032));
}

void CGUIWindowPVR::UpdateData(TVWindow update)
{
  if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM && update == TV_WINDOW_TV_PROGRAM)
  {

  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV && update == TV_WINDOW_CHANNELS_TV)
    UpdateChannelsTV();
  else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO && update == TV_WINDOW_CHANNELS_RADIO)
    UpdateChannelsRadio();
  else if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS && update == TV_WINDOW_RECORDINGS)
    UpdateRecordings();
  else if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS && update == TV_WINDOW_TIMERS)
    UpdateTimers();

  UpdateButtons();
}

bool CGUIWindowPVR::PlayFile(CFileItem *item, bool playMinimized)
{
  if (playMinimized)
  {
    if (item->m_strPath == g_application.CurrentFile())
    {
      CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, GetID());
      g_windowManager.SendMessage(msg);
      return true;
    }
    else
    {
      g_settings.m_bStartVideoWindowed = true;
    }
  }

  if (item->m_strPath.Left(17) == "pvr://recordings/")
  {
    /* For recordings we check here for a available stream URL */
    CStdString stream = item->GetPVRRecordingInfoTag()->StreamURL();
    if (stream != "")
    {
      /* Isolate the folder from the filename */
      size_t found = stream.find_last_of("/");
      if (found == CStdString::npos)
        found = stream.find_last_of("\\");

      if (found != CStdString::npos)
      {
        /* Check here for asterix at the begin of the filename */
        if (stream[found+1] == '*')
        {
          /* Create a "stack://" url with all files matching the extension */
          CStdString ext = URIUtils::GetExtension(stream);
          CStdString dir = stream.substr(0, found).c_str();

          CFileItemList items;
          CDirectory::GetDirectory(dir, items);
          items.Sort(SORT_METHOD_FILE ,SORT_ORDER_ASC);

          vector<int> stack;
          for (int i = 0; i < items.Size(); ++i)
          {
            if (URIUtils::GetExtension(items[i]->m_strPath) == ext)
              stack.push_back(i);
          }

          if (stack.size() > 0)
          {
            /* If we have a stack change the path of the item to it */
            CStackDirectory dir;
            CStdString stackPath = dir.ConstructStackPath(items, stack);
            item->m_strPath = stackPath;
          }
        }
        else
        {
          /* If no asterix is present play only the given stream URL */
          item->m_strPath = stream;
        }
      }
      else
      {
        CLog::Log(LOGERROR, "CGUIWindowPVR: Can't open recording, no valid filename!");
        CGUIDialogOK::ShowAndGetInput(19033,0,19036,0);
        return false;
      }
    }

    if (!g_application.PlayFile(*item, false))
    {
      CGUIDialogOK::ShowAndGetInput(19033,0,19036,0);
      return false;
    }
  }
  else
  {
    /* Play Live TV */
    if (!g_application.PlayFile(*item, false))
    {
      CGUIDialogOK::ShowAndGetInput(19033,0,19035,0);
      return false;
    }
  }

  return true;
}
