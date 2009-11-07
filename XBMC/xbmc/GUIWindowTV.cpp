/*
 *      Copyright (C) 2005-2008 Team XBMC
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

/* Standart includes */
#include "Application.h"
#include "GUIWindowManager.h"
#include "URL.h"
#include "MediaManager.h"
#include "LocalizeStrings.h"
#include "utils/log.h"
#include "GUISettings.h"
#include "Settings.h"

/* Dialog windows includes */
#include "GUIDialogFileBrowser.h"
#include "GUIDialogProgress.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogTVEPGProgInfo.h"
#include "GUIDialogTVRecordingInfo.h"
#include "GUIDialogTVTimerSettings.h"
#include "GUIDialogTVGroupManager.h"
#include "GUIDialogTVGuideSearch.h"

/* self include */
#include "GUIWindowTV.h"

/* TV control */
#include "PVRManager.h"


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
#define CONTROL_LABELEMPTY           30

#define CONTROL_BTNGUIDE             31
#define CONTROL_BTNCHANNELS_TV       32
#define CONTROL_BTNCHANNELS_RADIO    33
#define CONTROL_BTNRECORDINGS        34
#define CONTROL_BTNTIMERS            35
#define CONTROL_BTNSEARCH            36

#define CONTROL_AREA_GUIDE           9001
#define CONTROL_AREA_CHANNELS_TV     9002
#define CONTROL_AREA_CHANNELS_RADIO  9003
#define CONTROL_AREA_RECORDINGS      9004
#define CONTROL_AREA_TIMERS          9005
#define CONTROL_AREA_SEARCH          9006

#define GUIDE_VIEW_CHANNEL          0
#define GUIDE_VIEW_NOW              1
#define GUIDE_VIEW_NEXT             2
#define GUIDE_VIEW_TIMELINE         3

/**
 * \brief Class constructor
 */
CGUIWindowTV::CGUIWindowTV(void) : CGUIMediaWindow(WINDOW_TV, "MyTV.xml")
{
  m_iCurrSubTVWindow              = TV_WINDOW_UNKNOWN;
  m_iSavedSubTVWindow             = TV_WINDOW_UNKNOWN;
  m_iSelected_GUIDE               = 0;
  m_iSelected_CHANNELS_TV         = 0;
  m_iSelected_CHANNELS_RADIO      = 0;
  m_iSelected_RECORDINGS          = 0;
  m_iSelected_TIMERS              = 0;
  m_iSelected_SEARCH              = 0;
  m_iCurrentTVGroup               = -1;
  m_iCurrentRadioGroup            = -1;
  m_bShowHiddenChannels           = false;
  m_bSearchStarted                = false;
  m_bSearchConfirmed              = false;
  m_iGuideView                    = GUIDE_VIEW_CHANNEL;
  m_guideGrid                     = NULL;
  m_iSortOrder_SEARCH             = SORT_ORDER_ASC;
  m_iSortMethod_SEARCH            = SORT_METHOD_DATE;
}

/**
 * \brief Class destructor
 */
CGUIWindowTV::~CGUIWindowTV()
{
}

/**
 * \brief GUI action handler
 * \param const CAction &action     = GUI action class
 * \return bool                     = true if ok
 * Info:
 * Actions handled:
 * ACTION_PREVIOUS_MENU             goes to previous window
 */
bool CGUIWindowTV::OnAction(const CAction &action)
{
  if (action.id == ACTION_MOVE_LEFT || action.id == ACTION_MOVE_RIGHT)
  {
    if (GetFocusedControlID() == CONTROL_BTNGUIDE)
    {
      if (m_iGuideView == GUIDE_VIEW_CHANNEL)
      {
        SET_CONTROL_FOCUS(CONTROL_LIST_GUIDE_CHANNEL, 0);
      }
      else if (m_iGuideView == GUIDE_VIEW_NOW)
      {
        SET_CONTROL_FOCUS(CONTROL_LIST_GUIDE_NOW_NEXT, 0);
      }
      else if (m_iGuideView == GUIDE_VIEW_NEXT)
      {
        SET_CONTROL_FOCUS(CONTROL_LIST_GUIDE_NOW_NEXT, 0);
      }
      else if (m_iGuideView == GUIDE_VIEW_TIMELINE)
      {
        SET_CONTROL_FOCUS(CONTROL_LIST_TIMELINE, 0);
      }

      return true;
    }
  }
  else if (action.id == ACTION_PREVIOUS_MENU)
  {
    g_windowManager.PreviousWindow();
    return true;
  }

  return CGUIMediaWindow::OnAction(action);
}

/**
 * \brief GUI message handler
 * \param CGUIMessage& message      = GUI message class
 * \return bool                     = true if ok
 */
bool CGUIWindowTV::OnMessage(CGUIMessage& message)
{
  unsigned int iControl = 0;
  unsigned int iMessage = message.GetMessage();

  if (iMessage == GUI_MSG_WINDOW_INIT)
  {
    if (!g_PVRManager.HaveActiveClients())
    {
      g_windowManager.PreviousWindow();
      CGUIDialogOK::ShowAndGetInput(18100,0,18091,18092);
      return true;
    }

    if (!m_bSearchStarted)
    {
      m_bSearchStarted = true;
      m_searchfilter.SetDefaults();
    }
  }
  else if (iMessage == GUI_MSG_WINDOW_DEINIT)
  {
    /* Save current Subwindow selected list position */
    if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM)
    {
      if (m_iGuideView == GUIDE_VIEW_CHANNEL)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_GUIDE_CHANNEL);
        g_windowManager.SendMessage(msg);
        m_iSelected_GUIDE = msg.GetParam1();
      }
      else if (m_iGuideView == GUIDE_VIEW_NOW || m_iGuideView == GUIDE_VIEW_NEXT)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_GUIDE_NOW_NEXT);
        g_windowManager.SendMessage(msg);
        m_iSelected_GUIDE = msg.GetParam1();
      }
      else if (m_iGuideView == GUIDE_VIEW_TIMELINE)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_TIMELINE);
        g_windowManager.SendMessage(msg);
        m_iSelected_GUIDE = msg.GetParam1();
      }
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_CHANNELS_TV);
      g_windowManager.SendMessage(msg);
      m_iSelected_CHANNELS_TV = msg.GetParam1();
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_CHANNELS_RADIO);
      g_windowManager.SendMessage(msg);
      m_iSelected_CHANNELS_RADIO = msg.GetParam1();
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_RECORDINGS);
      g_windowManager.SendMessage(msg);
      m_iSelected_RECORDINGS = msg.GetParam1();
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_TIMERS);
      g_windowManager.SendMessage(msg);
      m_iSelected_TIMERS = msg.GetParam1();
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_SEARCH);
      g_windowManager.SendMessage(msg);
      m_iSelected_SEARCH = msg.GetParam1();
    }

    m_iSavedSubTVWindow = m_iCurrSubTVWindow;
    m_iCurrSubTVWindow  = TV_WINDOW_UNKNOWN;
  }
  else if (iMessage == GUI_MSG_FOCUSED)
  {
    /* Get the focused control Identifier */
    iControl = message.GetControlId();

    /* Process Identifier for focused Subwindow select buttons or list item.
     * If a new conrol becomes highlighted load his subwindow data
     */

    if (iControl == CONTROL_BTNGUIDE || m_iSavedSubTVWindow == TV_WINDOW_TV_PROGRAM)
    {
      SET_CONTROL_HIDDEN(CONTROL_AREA_CHANNELS_TV);
      SET_CONTROL_HIDDEN(CONTROL_AREA_CHANNELS_RADIO);
      SET_CONTROL_HIDDEN(CONTROL_AREA_RECORDINGS);
      SET_CONTROL_HIDDEN(CONTROL_AREA_TIMERS);
      SET_CONTROL_HIDDEN(CONTROL_AREA_SEARCH);

      if (m_iCurrSubTVWindow != TV_WINDOW_TV_PROGRAM)
      {
        m_iCurrSubTVWindow = TV_WINDOW_TV_PROGRAM;

        UpdateGuide();
        UpdateButtons();
      }
      else
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_TIMELINE);
        g_windowManager.SendMessage(msg);
        m_iSelected_GUIDE = msg.GetParam1();
      }

      SET_CONTROL_VISIBLE(CONTROL_AREA_GUIDE);
    }
    else if (iControl == CONTROL_BTNCHANNELS_TV || m_iSavedSubTVWindow == TV_WINDOW_CHANNELS_TV)
    {
      SET_CONTROL_HIDDEN(CONTROL_AREA_GUIDE);
      SET_CONTROL_HIDDEN(CONTROL_AREA_CHANNELS_RADIO);
      SET_CONTROL_HIDDEN(CONTROL_AREA_RECORDINGS);
      SET_CONTROL_HIDDEN(CONTROL_AREA_TIMERS);
      SET_CONTROL_HIDDEN(CONTROL_AREA_SEARCH);

      if (m_iCurrSubTVWindow != TV_WINDOW_CHANNELS_TV)
      {
        m_iCurrSubTVWindow = TV_WINDOW_CHANNELS_TV;

        UpdateChannelsTV();
        UpdateButtons();
      }
      else
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_CHANNELS_TV);
        g_windowManager.SendMessage(msg);
        m_iSelected_CHANNELS_TV = msg.GetParam1();
      }

      SET_CONTROL_VISIBLE(CONTROL_AREA_CHANNELS_TV);
    }
    else if (iControl == CONTROL_BTNCHANNELS_RADIO || m_iSavedSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
    {
      SET_CONTROL_HIDDEN(CONTROL_AREA_GUIDE);
      SET_CONTROL_HIDDEN(CONTROL_AREA_CHANNELS_TV);
      SET_CONTROL_HIDDEN(CONTROL_AREA_RECORDINGS);
      SET_CONTROL_HIDDEN(CONTROL_AREA_TIMERS);
      SET_CONTROL_HIDDEN(CONTROL_AREA_SEARCH);

      if (m_iCurrSubTVWindow != TV_WINDOW_CHANNELS_RADIO)
      {
        m_iCurrSubTVWindow = TV_WINDOW_CHANNELS_RADIO;

        UpdateChannelsRadio();
        UpdateButtons();
      }
      else
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_CHANNELS_RADIO);
        g_windowManager.SendMessage(msg);
        m_iSelected_CHANNELS_RADIO = msg.GetParam1();
      }

      SET_CONTROL_VISIBLE(CONTROL_AREA_CHANNELS_RADIO);
    }
    else if (iControl == CONTROL_BTNRECORDINGS || m_iSavedSubTVWindow == TV_WINDOW_RECORDINGS)
    {
      SET_CONTROL_HIDDEN(CONTROL_AREA_GUIDE);
      SET_CONTROL_HIDDEN(CONTROL_AREA_CHANNELS_TV);
      SET_CONTROL_HIDDEN(CONTROL_AREA_CHANNELS_RADIO);
      SET_CONTROL_HIDDEN(CONTROL_AREA_TIMERS);
      SET_CONTROL_HIDDEN(CONTROL_AREA_SEARCH);

      if (m_iCurrSubTVWindow != TV_WINDOW_RECORDINGS)
      {
        m_iCurrSubTVWindow = TV_WINDOW_RECORDINGS;

        UpdateRecordings();
        UpdateButtons();
      }
      else
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_RECORDINGS);
        g_windowManager.SendMessage(msg);
        m_iSelected_RECORDINGS = msg.GetParam1();
      }

      SET_CONTROL_VISIBLE(CONTROL_AREA_RECORDINGS);
    }
    else if (iControl == CONTROL_BTNTIMERS || m_iSavedSubTVWindow == TV_WINDOW_TIMERS)
    {
      SET_CONTROL_HIDDEN(CONTROL_AREA_GUIDE);
      SET_CONTROL_HIDDEN(CONTROL_AREA_CHANNELS_TV);
      SET_CONTROL_HIDDEN(CONTROL_AREA_CHANNELS_RADIO);
      SET_CONTROL_HIDDEN(CONTROL_AREA_RECORDINGS);
      SET_CONTROL_HIDDEN(CONTROL_AREA_SEARCH);

      if (m_iCurrSubTVWindow != TV_WINDOW_TIMERS)
      {
        m_iCurrSubTVWindow = TV_WINDOW_TIMERS;

        UpdateTimers();
        UpdateButtons();
      }
      else
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_TIMERS);
        g_windowManager.SendMessage(msg);
        m_iSelected_TIMERS = msg.GetParam1();
      }

      SET_CONTROL_VISIBLE(CONTROL_AREA_TIMERS);
    }
    else if (iControl == CONTROL_BTNSEARCH || m_iSavedSubTVWindow == TV_WINDOW_SEARCH)
    {
      SET_CONTROL_HIDDEN(CONTROL_AREA_GUIDE);
      SET_CONTROL_HIDDEN(CONTROL_AREA_CHANNELS_TV);
      SET_CONTROL_HIDDEN(CONTROL_AREA_CHANNELS_RADIO);
      SET_CONTROL_HIDDEN(CONTROL_AREA_RECORDINGS);
      SET_CONTROL_HIDDEN(CONTROL_AREA_TIMERS);

      if (m_iCurrSubTVWindow != TV_WINDOW_SEARCH)
      {
        m_iCurrSubTVWindow = TV_WINDOW_SEARCH;

        UpdateSearch();
        UpdateButtons();
      }
      else
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_SEARCH);
        g_windowManager.SendMessage(msg);
        m_iSelected_SEARCH = msg.GetParam1();
      }

      SET_CONTROL_VISIBLE(CONTROL_AREA_SEARCH);
    }

    if (m_iSavedSubTVWindow != TV_WINDOW_UNKNOWN)
    {
      m_iSavedSubTVWindow = TV_WINDOW_UNKNOWN;
    }
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
    }
    else if (iControl == CONTROL_BTNCHANNELS_TV)
    {
      m_iCurrentTVGroup = PVRChannelGroups.GetNextGroupID(m_iCurrentTVGroup);
      UpdateChannelsTV();
    }
    else if (iControl == CONTROL_BTNCHANNELS_RADIO)
    {
      m_iCurrentRadioGroup = PVRChannelGroups.GetNextGroupID(m_iCurrentRadioGroup);
      UpdateChannelsRadio();
    }
    else if (iControl == CONTROL_BTNRECORDINGS)
    {
      g_PVRManager.TriggerRecordingsUpdate();
      UpdateRecordings();
    }
    else if (iControl == CONTROL_LIST_TIMELINE ||
             iControl == CONTROL_LIST_GUIDE_CHANNEL ||
             iControl == CONTROL_LIST_GUIDE_NOW_NEXT)
    {
      CFileItemPtr pItem;
      int iItem;

      /* Get currently performed action */
      int iAction = message.GetParam1();

      if (iControl == CONTROL_LIST_TIMELINE)
      {
        /* Get currently selected item from grid container */
        pItem = m_guideGrid->GetSelectedItemPtr();

        if (!pItem) return false;
      }
      else
      {
        /* Get currently selected item from file list */
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
        g_windowManager.SendMessage(msg);
        iItem = msg.GetParam1();

        /* Check file item is in list range and get his pointer */

        if (iItem < 0 || iItem >= (int)m_vecItems->Size()) return true;

        pItem = m_vecItems->Get(iItem);
      }

      /* Process actions */
      if ((iAction == ACTION_SELECT_ITEM) || (iAction == ACTION_SHOW_INFO || iAction == ACTION_MOUSE_LEFT_CLICK))
      {
        /* Show information Dialog */
        ShowEPGInfo(pItem.get());
        return true;
      }
      else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      {
        //contextmenu
        OnPopupMenu(iItem);
        return true;
      }
      else if (iAction == ACTION_RECORD)
      {
        if (pItem->GetEPGInfoTag()->ChannelNumber() != -1)
        {
          if (pItem->GetEPGInfoTag()->Timer() == NULL)
          {
            // prompt user for confirmation of channel record
            CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

            if (pDialog)
            {
              pDialog->SetHeading(264);
              pDialog->SetLine(0, "");
              pDialog->SetLine(1, pItem->GetEPGInfoTag()->Title());
              pDialog->SetLine(2, "");
              pDialog->DoModal();

              if (pDialog->IsConfirmed())
              {
                cPVRTimerInfoTag newtimer(*pItem.get());
                CFileItem *item = new CFileItem(newtimer);

                if (cPVRTimers::AddTimer(*item))
                  cPVREpgs::SetVariableData(m_vecItems);
              }
            }
          }
          else
          {
            CGUIDialogOK::ShowAndGetInput(18100,18107,0,0);
          }
        }
      }
    }
    else if ((iControl == CONTROL_LIST_CHANNELS_TV) || (iControl == CONTROL_LIST_CHANNELS_RADIO))
    {
      /* Get currently performed action */
      int iAction = message.GetParam1();

      /* Get currently selected item from file list */
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
      g_windowManager.SendMessage(msg);
      int iItem = msg.GetParam1();

      /* Check file item is in list range and get his pointer */

      if (iItem < 0 || iItem >= (int)m_vecItems->Size()) return true;

      CFileItemPtr pItem = m_vecItems->Get(iItem);

      /* Process actions */
      if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
      {
        /* Check if "Add channel..." entry is pressed by OK, if yes
           create a new channel and open settings dialog, otherwise
           open channel with player */
        if (pItem->m_strPath == g_localizeStrings.Get(18061))
        {
          cPVRChannelInfoTag newchannel;
          CFileItem *item = new CFileItem(newchannel);
          CGUIDialogOK::ShowAndGetInput(18100,0,18059,0);
        }
        else
        {
          if (g_guiSettings.GetBool("pvrplayback.timeshift") && g_guiSettings.GetString("pvrplayback.timeshiftpath") == "")
          {
            CGUIDialogOK::ShowAndGetInput(18100,18124,0,18125);
            return false;
          }

          if (iControl == CONTROL_LIST_CHANNELS_TV)
            g_PVRManager.SetPlayingGroup(m_iCurrentTVGroup);
          if (iControl == CONTROL_LIST_CHANNELS_RADIO)
            g_PVRManager.SetPlayingGroup(m_iCurrentRadioGroup);

          /* Open tv channel by Player and return */
          if (g_guiSettings.GetBool("pvrplayback.playminimized"))
          {
            if (pItem->m_strPath == g_application.CurrentFile())
            {
              CGUIMessage msg(GUI_MSG_FULLSCREEN, 0, GetID());
              g_windowManager.SendMessage(msg);
              return true;
            }
            else
            {
              g_stSettings.m_bStartVideoWindowed = true;
            }
          }
          if (!g_application.PlayFile(*pItem))
          {
            CGUIDialogOK::ShowAndGetInput(18100,0,18134,0);
            return false;
          }
          return true;
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
        int iChannel = pItem->GetPVRChannelInfoTag()->Number();

        if (iChannel != -1)
        {
          CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

          if (pDialog)
          {
            pDialog->SetHeading(18196);
            pDialog->SetLine(0, "");
            pDialog->SetLine(1, pItem->GetPVRChannelInfoTag()->Name());
            pDialog->SetLine(2, "");
            pDialog->DoModal();

            if (!pDialog->IsConfirmed()) return false;

            if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
            {
              PVRChannelsTV.HideChannel(pItem->GetPVRChannelInfoTag()->Number());
              UpdateChannelsTV();
            }
            else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
            {
              PVRChannelsRadio.HideChannel(pItem->GetPVRChannelInfoTag()->Number());
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
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
      g_windowManager.SendMessage(msg);
      int iItem = msg.GetParam1();

      /* Check file item is in list range and get his pointer */

      if (iItem < 0 || iItem >= (int)m_vecItems->Size()) return true;

      CFileItemPtr pItem = m_vecItems->Get(iItem);

      /* Process actions */
      if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
      {
        /* Open recording with Player and return */
        return g_application.PlayFile(*pItem);
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

          if (pDialog)
          {
            pDialog->SetHeading(122);
            pDialog->SetLine(0, 18192);
            pDialog->SetLine(1, "");
            pDialog->SetLine(2, pItem->GetPVRRecordingInfoTag()->Title());
            pDialog->DoModal();

            if (pDialog->IsConfirmed())
            {
              if (cPVRRecordings::DeleteRecording(*pItem))
              {
                PVRRecordings.Update(true);
                UpdateRecordings();
              }
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
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
      g_windowManager.SendMessage(msg);
      int iItem = msg.GetParam1();

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
          cPVRTimerInfoTag newtimer(true);
          CFileItem *item = new CFileItem(newtimer);

          if (ShowTimerSettings(item))
          {
            /* Add timer to backend */
            cPVRTimers::AddTimer(*item);
            UpdateTimers();
          }
        }
        else
        {
          CFileItem fileitem(*pItem);

          if (ShowTimerSettings(&fileitem))
          {
            /* Update timer on pvr backend */
            cPVRTimers::UpdateTimer(fileitem);
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

          if (pDialog)
          {
            pDialog->SetHeading(122);
            pDialog->SetLine(0, 18076);
            pDialog->SetLine(1, "");
            pDialog->SetLine(2, pItem->GetPVRTimerInfoTag()->Title());
            pDialog->DoModal();

            if (pDialog->IsConfirmed())
            {
              cPVRTimers::DeleteTimer(*pItem);
              UpdateTimers();
            }
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
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
      g_windowManager.SendMessage(msg);
      int iItem = msg.GetParam1();

      /* Check file item is in list range and get his pointer */

      if (iItem < 0 || iItem >= (int)m_vecItems->Size()) return true;

      CFileItemPtr pItem = m_vecItems->Get(iItem);

      /* Process actions */
      if (iAction == ACTION_SHOW_INFO || iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
      {
        if (pItem->GetLabel() == g_localizeStrings.Get(18164))
        {
          ShowSearchResults();
        }
        else
        {
          /* Show information Dialog */
          ShowEPGInfo(pItem.get());
          return true;
        }

        return true;
      }
      else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
      {
        //contextmenu
        OnPopupMenu(iItem);
        return true;
      }
      else if (iAction == ACTION_RECORD)
      {
        int iChannel = pItem->GetEPGInfoTag()->ChannelNumber();

        if (iChannel != -1)
        {
          if (pItem->GetEPGInfoTag()->Timer() == NULL)
          {
            // prompt user for confirmation of channel record
            CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

            if (pDialog)
            {
              pDialog->SetHeading(264);
              pDialog->SetLine(0, "");
              pDialog->SetLine(1, pItem->GetEPGInfoTag()->Title());
              pDialog->SetLine(2, "");
              pDialog->DoModal();

              if (pDialog->IsConfirmed())
              {
                cPVRTimerInfoTag newtimer(*pItem.get());
                CFileItem *item = new CFileItem(newtimer);

                if (cPVRTimers::AddTimer(*item))
                  cPVREpgs::SetVariableData(m_vecItems);
              }
            }
          }
          else
          {
            CGUIDialogOK::ShowAndGetInput(18100,18107,0,0);
          }
        }
      }
    }
    else if (iControl == CONTROL_BTNSEARCH)
    {
      ShowSearchResults();
    }
  }

  return CGUIMediaWindow::OnMessage(message);
}

/**
 * \brief Perform popup menu
 * \param int iItem                 = Index number inside global file item list
 * \return bool                     = true if ok
 * Note:
 * Is a copy from CGUIMediaWindow with a little change for the window position
 * without it the context menu is always in the left upper corner. Why ?????
 */
bool CGUIWindowTV::OnPopupMenu(int iItem)
{
  /* Check if it is inside a list */
  unsigned int iControl = GetFocusedControlID();
  int m_iSelected;

  if (iControl < 10 || iControl > 17) return false;

  /* Save current Subwindow selected list position */
  if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM)
  {
    if (m_iGuideView == GUIDE_VIEW_CHANNEL)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_GUIDE_CHANNEL);
      g_windowManager.SendMessage(msg);
      m_iSelected = m_iSelected_GUIDE = msg.GetParam1();
    }
    else if (m_iGuideView == GUIDE_VIEW_NEXT || m_iGuideView == GUIDE_VIEW_NOW)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_GUIDE_NOW_NEXT);
      g_windowManager.SendMessage(msg);
      m_iSelected = m_iSelected_GUIDE = msg.GetParam1();
    }
    else if (m_iGuideView == GUIDE_VIEW_TIMELINE)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_TIMELINE);
      g_windowManager.SendMessage(msg);
      m_iSelected = m_iSelected_GUIDE = msg.GetParam1();
    }
  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_CHANNELS_TV);
    g_windowManager.SendMessage(msg);
    m_iSelected = m_iSelected_CHANNELS_TV = msg.GetParam1();
  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_CHANNELS_RADIO);
    g_windowManager.SendMessage(msg);
    m_iSelected = m_iSelected_CHANNELS_RADIO = msg.GetParam1();
  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_RECORDINGS);
    g_windowManager.SendMessage(msg);
    m_iSelected = m_iSelected_RECORDINGS = msg.GetParam1();
  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_TIMERS);
    g_windowManager.SendMessage(msg);
    m_iSelected = m_iSelected_TIMERS = msg.GetParam1();
  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_SEARCH);
    g_windowManager.SendMessage(msg);
    m_iSelected = m_iSelected_SEARCH = msg.GetParam1();
  }

  // popup the context menu
  // grab our context menu
  CContextButtons buttons;

  GetContextButtons(m_iSelected, buttons);

  if (buttons.size())
  {

    // mark the item
    if (m_iSelected >= 0 && m_iSelected < m_vecItems->Size())
      m_vecItems->Get(m_iSelected)->Select(true);

    CGUIDialogContextMenu *pMenu = (CGUIDialogContextMenu *)g_windowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);

    if (!pMenu) return false;

    // load our menu
    pMenu->Initialize();

    // add the buttons and execute it
    for (CContextButtons::iterator it = buttons.begin(); it != buttons.end(); it++)
      pMenu->AddButton((*it).second);

    // position it correctly
    pMenu->SetPosition(GetWidth() / 2 - pMenu->GetWidth() / 2, GetHeight() / 2 - pMenu->GetHeight() / 2);

    pMenu->DoModal();

    // translate our button press
    CONTEXT_BUTTON btn = CONTEXT_BUTTON_CANCELLED;

    if (pMenu->GetButton() > 0 && pMenu->GetButton() <= (int)buttons.size())
      btn = buttons[pMenu->GetButton() - 1].first;

    // deselect our item
    if (m_iSelected >= 0 && m_iSelected < m_vecItems->Size())
      m_vecItems->Get(m_iSelected)->Select(false);

    if (btn != CONTEXT_BUTTON_CANCELLED)
      return OnContextButton(m_iSelected, btn);
  }

  return false;
}

/**
 * \brief Get context button names for given sub window
 * \param int itemNumber            = Index number inside global file item list
 * \param CContextButtons &buttons  = context button class
 */
void CGUIWindowTV::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  /* Perform file item for specified sub window */
  CFileItemPtr pItem;
  if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM && m_iGuideView == GUIDE_VIEW_TIMELINE)
  {
    /* Get currently selected item from grid container */
    pItem = m_guideGrid->GetSelectedItemPtr();
  }
  else
  {
    /* Check file item is in list range and get his pointer */
    if (itemNumber < 0 || itemNumber >= (int)m_vecItems->Size()) return;

    pItem = m_vecItems->Get(itemNumber);
  }

  if (pItem)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV || m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
    {
      if (itemNumber < 0 || itemNumber >= m_vecItems->Size())
        return;

      /* check that file item is in list and get his pointer*/
      if (pItem->m_strPath == g_localizeStrings.Get(18061))
      {
        /* If yes show only "New Channel" on context menu */
        buttons.Add(CONTEXT_BUTTON_ADD, 18049); /* NEW CHANNEL */
      }
      else
      {
        buttons.Add(CONTEXT_BUTTON_INFO, 18163);          /* Channel info button */
        buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 19000);   /* switch to channel */
        buttons.Add(CONTEXT_BUTTON_SET_THUMB, 18161);   /* Set icon */
        buttons.Add(CONTEXT_BUTTON_GROUP_MANAGER, 18126);       /* Group managment */
        buttons.Add(CONTEXT_BUTTON_HIDE, m_bShowHiddenChannels ? 18193 : 18198);        /* HIDE CHANNEL */

        if (m_vecItems->Size() > 1 && !m_bShowHiddenChannels)
          buttons.Add(CONTEXT_BUTTON_MOVE, 116);          /* MOVE CHANNEL */

        if ((m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV && PVRChannelsTV.GetNumHiddenChannels() > 0) ||
            (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO && PVRChannelsRadio.GetNumHiddenChannels() > 0) ||
            m_bShowHiddenChannels)
          buttons.Add(CONTEXT_BUTTON_SHOW_HIDDEN, m_bShowHiddenChannels ? 18194 : 18195);  /* SHOW HIDDEN CHANNELS */

        CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
      }
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS)           /* Add recordings context buttons */
    {
      buttons.Add(CONTEXT_BUTTON_INFO, 18073);
      buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 12021);
//            buttons.Add(CONTEXT_BUTTON_RESUME_ITEM, 12022);
      buttons.Add(CONTEXT_BUTTON_RENAME, 118);
      buttons.Add(CONTEXT_BUTTON_DELETE, 117);
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)           /* Add timer context buttons */
    {
      /* Check for a empty file item list, means only a
          file item with the name "Add timer..." is present */
      if (pItem->m_strPath == "pvr://timers/add.timer")
      {
        /* If yes show only "New Timer" on context menu */
        buttons.Add(CONTEXT_BUTTON_ADD, 18072); /* NEW TIMER */
      }
      else
      {
        /* If any timers are present show more */
        buttons.Add(CONTEXT_BUTTON_EDIT, 18068);
        buttons.Add(CONTEXT_BUTTON_ADD, 18072); /* NEW TIMER */
        buttons.Add(CONTEXT_BUTTON_ACTIVATE, 18070); /* ON/OFF */
        buttons.Add(CONTEXT_BUTTON_RENAME, 118);
        buttons.Add(CONTEXT_BUTTON_DELETE, 117);
      }
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM)
    {
      if (pItem->GetEPGInfoTag()->End() > CDateTime::GetCurrentDateTime())
      {
        if (pItem->GetEPGInfoTag()->Timer() == NULL)
        {
          if (pItem->GetEPGInfoTag()->Start() < CDateTime::GetCurrentDateTime())
          {
            buttons.Add(CONTEXT_BUTTON_START_RECORD, 264);             /* RECORD programme */
          }
          else
          {
            buttons.Add(CONTEXT_BUTTON_START_RECORD, 18416);
          }
        }
        else
        {
          if (pItem->GetEPGInfoTag()->Start() < CDateTime::GetCurrentDateTime())
          {
            buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 18414);
          }
          else
          {
            buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 18415);
          }
        }
      }

      buttons.Add(CONTEXT_BUTTON_INFO, 658);              /* Epg info button */
      buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 19000);           /* Switch channel */
      if (m_iGuideView == GUIDE_VIEW_TIMELINE)
      {
        buttons.Add(CONTEXT_BUTTON_USER4, 18166);           /* Go to begin */
        buttons.Add(CONTEXT_BUTTON_USER5, 18167);           /* Go to end */
      }
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
    {
      if (pItem->GetLabel() != g_localizeStrings.Get(18164))
      {
        if (pItem->GetEPGInfoTag()->End() > CDateTime::GetCurrentDateTime())
        {
          if (pItem->GetEPGInfoTag()->Timer() == NULL)
          {
            if (pItem->GetEPGInfoTag()->Start() < CDateTime::GetCurrentDateTime())
            {
              buttons.Add(CONTEXT_BUTTON_START_RECORD, 264);             /* RECORD programme */
            }
            else
            {
              buttons.Add(CONTEXT_BUTTON_START_RECORD, 18416);
            }
          }
          else
          {
            if (pItem->GetEPGInfoTag()->Start() < CDateTime::GetCurrentDateTime())
            {
              buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 18414);
            }
            else
            {
              buttons.Add(CONTEXT_BUTTON_STOP_RECORD, 18415);
            }
          }
        }

        buttons.Add(CONTEXT_BUTTON_INFO, 658);              /* Epg info button */
        buttons.Add(CONTEXT_BUTTON_USER1, 18165);
        buttons.Add(CONTEXT_BUTTON_USER2, 103);
        buttons.Add(CONTEXT_BUTTON_USER3, 104);
        buttons.Add(CONTEXT_BUTTON_CLEAR, 20375);
      }
    }
  }

  return;
}

/**
 * \brief Do functions for given context menu button for current subwindow
 * \param int itemNumber            = Index number inside global file item list
 * \param CONTEXT_BUTTON button     = button type pressed
 * \return bool                     = true if ok
 */
bool CGUIWindowTV::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr pItem;
  if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM && m_iGuideView == GUIDE_VIEW_TIMELINE)
  {
    /* Get currently selected item from grid container */
    pItem = m_guideGrid->GetSelectedItemPtr();
  }
  else
  {
    /* Check file item is in list range and get his pointer */
    if (itemNumber < 0 || itemNumber >= (int)m_vecItems->Size()) return false;

    pItem = m_vecItems->Get(itemNumber);
  }

  if (!pItem)
    return false;

  if (button == CONTEXT_BUTTON_PLAY_ITEM)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS)
    {
      return g_application.PlayFile(*pItem);
    }

    if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV ||
        m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
    {
      if (g_guiSettings.GetBool("pvrplayback.timeshift") && g_guiSettings.GetString("pvrplayback.timeshiftpath") == "")
      {
        CGUIDialogOK::ShowAndGetInput(18100,18124,0,18125);
        return false;
      }

      if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
        g_PVRManager.SetPlayingGroup(m_iCurrentTVGroup);
      if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
        g_PVRManager.SetPlayingGroup(m_iCurrentRadioGroup);

      if (!g_application.PlayFile(*pItem))
      {
        CGUIDialogOK::ShowAndGetInput(18100,0,18134,0);
        return false;
      }
      return true;
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM)
    {
      CFileItemList channelslist;
      int ret_channels;

      if (!pItem->GetEPGInfoTag()->IsRadio())
      {
        ret_channels = PVRChannelsTV.GetChannels(&channelslist, m_iCurrentTVGroup);
      }
      else
      {
        ret_channels = PVRChannelsRadio.GetChannels(&channelslist, m_iCurrentRadioGroup);
      }

      if (ret_channels > 0)
      {
        if (g_guiSettings.GetBool("pvrplayback.timeshift") && g_guiSettings.GetString("pvrplayback.timeshiftpath") == "")
        {
          CGUIDialogOK::ShowAndGetInput(18100,18124,0,18125);
          return false;
        }

        g_PVRManager.SetPlayingGroup(-1);
        if (!g_application.PlayFile(*channelslist[pItem->GetEPGInfoTag()->ChannelNumber()-1]))
        {
          CGUIDialogOK::ShowAndGetInput(18100,0,18134,0);
          return false;
        }
        return true;
      }
    }
  }
  else if (button == CONTEXT_BUTTON_MOVE)
  {
    CStdString strIndex;
    strIndex.Format("%i", pItem->GetPVRChannelInfoTag()->Number());
    CGUIDialogNumeric::ShowAndGetNumber(strIndex, g_localizeStrings.Get(18197));
    int newIndex = atoi(strIndex.c_str());

    if (newIndex != pItem->GetPVRChannelInfoTag()->Number())
    {
      if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
      {
        PVRChannelsTV.MoveChannel(pItem->GetPVRChannelInfoTag()->Number(), newIndex);
        UpdateChannelsTV();
      }
      else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
      {
        PVRChannelsRadio.MoveChannel(pItem->GetPVRChannelInfoTag()->Number(), newIndex);
        UpdateChannelsRadio();
      }
    }
  }
  else if (button == CONTEXT_BUTTON_HIDE)
  {
    // prompt user for confirmation of channel hide
    CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

    if (pDialog)
    {
      pDialog->SetHeading(18196);
      pDialog->SetLine(0, "");
      pDialog->SetLine(1, pItem->GetPVRChannelInfoTag()->Name());
      pDialog->SetLine(2, "");
      pDialog->DoModal();

      if (!pDialog->IsConfirmed()) return false;

      if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
      {
        PVRChannelsTV.HideChannel(pItem->GetPVRChannelInfoTag()->Number());
        UpdateChannelsTV();
      }
      else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
      {
        PVRChannelsRadio.HideChannel(pItem->GetPVRChannelInfoTag()->Number());
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
    /* Check if a icon path is set inside settings */
    if (g_guiSettings.GetString("pvrmenu.iconpath") == "")
    {
      CGUIDialogOK::ShowAndGetInput(18100,18812,0,18813);
      return true;
    }

    CStdString strIcon = pItem->GetPVRChannelInfoTag()->Icon() == "" ? g_guiSettings.GetString("pvrmenu.iconpath") : pItem->GetPVRChannelInfoTag()->Icon();

    VECSOURCES shares;
    g_mediaManager.GetLocalDrives(shares);

    if (CGUIDialogFileBrowser::ShowAndGetImage(shares,g_localizeStrings.Get(1030), strIcon))
    {
      if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV)
      {
        PVRChannelsTV.SetChannelIcon(pItem->GetPVRChannelInfoTag()->Number(), strIcon);
        UpdateChannelsTV();
      }
      else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO)
      {
        PVRChannelsRadio.SetChannelIcon(pItem->GetPVRChannelInfoTag()->Number(), strIcon);
        UpdateChannelsRadio();
      }
    }
  }
  else if (button == CONTEXT_BUTTON_EDIT)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)
    {
      CFileItem fileitem(*pItem);

      if (ShowTimerSettings(&fileitem))
      {
        cPVRTimers::UpdateTimer(fileitem);
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
      cPVRChannelInfoTag newchannel;
      CFileItem *item = new CFileItem(newchannel);
      CGUIDialogOK::ShowAndGetInput(18100,0,18059,0);
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)
    {
      cPVRTimerInfoTag newtimer(true);
      CFileItem *item = new CFileItem(newtimer);

      if (ShowTimerSettings(item))
      {
        cPVRTimers::AddTimer(*item);
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

    CGUIDialogOK::ShowAndGetInput(18100, 18076, 0, return_str_id);

    cPVRTimers::UpdateTimer(*pItem);
    UpdateTimers(); /** Force list update **/
    return true;
  }
  else if (button == CONTEXT_BUTTON_RENAME)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS)
    {
      CStdString strNewName = pItem->GetPVRTimerInfoTag()->Title();
      if (CGUIDialogKeyboard::ShowAndGetInput(strNewName, g_localizeStrings.Get(18400), false))
      {
        cPVRTimers::RenameTimer(*pItem, strNewName);
        UpdateTimers();
      }

      return true;
    }
    else if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS)
    {
      CStdString strNewName = pItem->GetPVRRecordingInfoTag()->Title();
      if (CGUIDialogKeyboard::ShowAndGetInput(strNewName, g_localizeStrings.Get(18399), false))
      {
        if (cPVRRecordings::DeleteRecording(*pItem))
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
        pDialog->SetLine(0, 18076);
        pDialog->SetLine(1, "");
        pDialog->SetLine(2, pItem->GetPVRTimerInfoTag()->Title());
        pDialog->DoModal();

        if (!pDialog->IsConfirmed()) return false;

        cPVRTimers::DeleteTimer(*pItem);

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
        pDialog->SetLine(0, 18192);
        pDialog->SetLine(1, "");
        pDialog->SetLine(2, pItem->GetPVRRecordingInfoTag()->Title());
        pDialog->DoModal();

        if (!pDialog->IsConfirmed()) return false;

        if (cPVRRecordings::DeleteRecording(*pItem))
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

      if (m_iGuideView == GUIDE_VIEW_TIMELINE)
      {
        /* Get currently selected item from grid container */
        pItem = m_guideGrid->GetSelectedItemPtr();

        if (!pItem) return false;
      }

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
      int iChannel = pItem->GetEPGInfoTag()->ChannelNumber();

      if (iChannel != -1)
      {
        if (pItem->GetEPGInfoTag()->Timer() == NULL)
        {
          // prompt user for confirmation of channel record
          CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

          if (pDialog)
          {
            pDialog->SetHeading(264);
            pDialog->SetLine(0, pItem->GetEPGInfoTag()->ChannelName());
            pDialog->SetLine(1, "");
            pDialog->SetLine(2, pItem->GetEPGInfoTag()->Title());
            pDialog->DoModal();

            if (pDialog->IsConfirmed())
            {
              cPVRTimerInfoTag newtimer(*pItem.get());
              CFileItem *item = new CFileItem(newtimer);

              if (cPVRTimers::AddTimer(*item))
                cPVREpgs::SetVariableData(m_vecItems);
            }
          }
        }
        else
        {
          CGUIDialogOK::ShowAndGetInput(18100,18107,0,0);
        }
      }
    }
  }
  else if (button == CONTEXT_BUTTON_STOP_RECORD)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM || m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
    {
      int iChannel = pItem->GetEPGInfoTag()->ChannelNumber();

      if (iChannel != -1)
      {
        if (pItem->GetEPGInfoTag()->Timer() != NULL)
        {
          CFileItemList timerlist;

          if (PVRTimers.GetTimers(&timerlist) > 0)
          {
            for (int i = 0; i < timerlist.Size(); ++i)
            {
              if ((timerlist[i]->GetPVRTimerInfoTag()->Number() == pItem->GetEPGInfoTag()->ChannelNumber()) &&
                  (timerlist[i]->GetPVRTimerInfoTag()->Start() <= pItem->GetEPGInfoTag()->Start()) &&
                  (timerlist[i]->GetPVRTimerInfoTag()->Stop() >= pItem->GetEPGInfoTag()->End()) &&
                  (timerlist[i]->GetPVRTimerInfoTag()->IsRepeating() != true))
              {
                if (cPVRTimers::DeleteTimer(*timerlist[i]))
                  cPVREpgs::SetVariableData(m_vecItems);
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
      m_searchfilter.SetDefaults();
      UpdateSearch();
    }
  }
  else if (button == CONTEXT_BUTTON_USER1)
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
  else if (button == CONTEXT_BUTTON_USER2)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
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
  else if (button == CONTEXT_BUTTON_USER3)
  {
    if (m_iCurrSubTVWindow == TV_WINDOW_SEARCH)
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
  else if (button == CONTEXT_BUTTON_USER4)
  {
    m_guideGrid->GoToBegin();
  }
  else if (button == CONTEXT_BUTTON_USER5)
  {
    m_guideGrid->GoToEnd();
  }

  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

/**
 * \brief Show programme summary information dialog for given epg or channel info tag
 * \param CFileItem *item           = pointer to file item with TV epg or channel info tag
 */
void CGUIWindowTV::ShowEPGInfo(CFileItem *item)
{
  /* Check item is TV epg or channel information tag */
  if (item->IsEPG())
  {
    /* Load programme info dialog */
    CGUIDialogTVEPGProgInfo* pDlgInfo = (CGUIDialogTVEPGProgInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_TV_GUIDE_INFO);

    if (!pDlgInfo)
      return;

    /* inform dialog about the file item */
    pDlgInfo->SetProgInfo(item);

    /* Open dialog window */
    pDlgInfo->DoModal();
  }
  else if (item->IsPVRChannel())
  {
    cPVREpgsLock EpgsLock;
    cPVREpgs *s = (cPVREpgs *)cPVREpgs::EPGs(EpgsLock);
    if (s)
    {
      const cPVREPGInfoTag *epgnow = s->GetEPG(item->GetPVRChannelInfoTag(), true)->GetInfoTagNow();
      CFileItem *itemNow  = new CFileItem(*epgnow);

      /* Load programme info dialog */
      CGUIDialogTVEPGProgInfo* pDlgInfo = (CGUIDialogTVEPGProgInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_TV_GUIDE_INFO);

      if (!pDlgInfo)
        return;

      /* inform dialog about the file item */
      pDlgInfo->SetProgInfo(itemNow);

      /* Open dialog window */
      pDlgInfo->DoModal();

      cPVREpgs::SetVariableData(m_vecItems);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "CGUIWindowTV: Can't open programme info dialog, no epg or channel info tag!");
    return;
  }

  /* Return to caller */
  return;

}

/**
 * \brief Show recording summary information dialog for given record info tag
 * \param CFileItem *item           = pointer to file item with TV recording info tag
 */
void CGUIWindowTV::ShowRecordingInfo(CFileItem *item)
{
  /* Check item is TV record information tag */
  if (!item->IsPVRRecording())
  {
    CLog::Log(LOGERROR, "CGUIWindowTV: Can't open recording info dialog, no record info tag!");
    return;
  }

  /* Load record info dialog */
  CGUIDialogTVRecordingInfo* pDlgInfo = (CGUIDialogTVRecordingInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_TV_RECORDING_INFO);

  if (!pDlgInfo)
    return;

  /* inform dialog about the file item */
  pDlgInfo->SetRecording(item);

  /* Open dialog window */
  pDlgInfo->DoModal();

  /* Return to caller */
  return;
}

/**
 * \brief Show Timer settings dialog for given timer info tag
 * \param CFileItem *item           = pointer to file item with TV timer info tag
 * \return bool                     = true if tag is changed, false for no change
 */
bool CGUIWindowTV::ShowTimerSettings(CFileItem *item)
{
  /* Check item is TV timer information tag */
  if (!item->IsPVRTimer())
  {
    CLog::Log(LOGERROR, "CGUIWindowTV: Can't open timer settings dialog, no timer info tag!");
    return false;
  }

  /* Load timer settings dialog */
  CGUIDialogTVTimerSettings* pDlgInfo = (CGUIDialogTVTimerSettings*)g_windowManager.GetWindow(WINDOW_DIALOG_TV_TIMER_SETTING);

  if (!pDlgInfo)
    return false;

  /* inform dialog about the file item */
  pDlgInfo->SetTimer(item);

  /* Open dialog window */
  pDlgInfo->DoModal();

  /* Get modify flag from window and return it to caller */
  return pDlgInfo->GetOK();
}

/**
 * \brief Show Channel group managment
 */
void CGUIWindowTV::ShowGroupManager(bool IsRadio)
{
  /* Load timer settings dialog */
  CGUIDialogTVGroupManager* pDlgInfo = (CGUIDialogTVGroupManager*)g_windowManager.GetWindow(WINDOW_DIALOG_TV_GROUP_MANAGER);

  if (!pDlgInfo)
    return;

  pDlgInfo->SetRadio(IsRadio);

  /* Open dialog window */
  pDlgInfo->DoModal();

  return;
}

void CGUIWindowTV::ShowSearchResults()
{
  /* Load timer settings dialog */
  CGUIDialogTVEPGSearch* pDlgInfo = (CGUIDialogTVEPGSearch*)g_windowManager.GetWindow(WINDOW_DIALOG_TV_GUIDE_SEARCH);

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

/**
 * \brief Update TV Guide list
 */
void CGUIWindowTV::UpdateGuide()
{
  CStdString strLabel;

  m_vecItems->Clear();

  bool RadioPlaying;
  int CurrentChannel;
  g_PVRManager.GetCurrentChannel(&CurrentChannel, &RadioPlaying);

  if (m_iGuideView == GUIDE_VIEW_CHANNEL)
  {
    m_guideGrid = NULL;
    CStdString strChannel;

    SET_CONTROL_HIDDEN(8002);
    SET_CONTROL_HIDDEN(8003);
    SET_CONTROL_VISIBLE(8001);

    if (!RadioPlaying)
      strChannel = PVRChannelsTV.GetNameForChannel(CurrentChannel);
    else
      strChannel = PVRChannelsRadio.GetNameForChannel(CurrentChannel);

    strLabel.Format("%s: %s", g_localizeStrings.Get(18050), strChannel.c_str());

    if (cPVREpgs::GetEPGChannel(CurrentChannel, m_vecItems, RadioPlaying) > 0)
    {
      CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_GUIDE_CHANNEL, 0, 0, m_vecItems);
      g_windowManager.SendMessage(msg);
    }

    /* Set Selected item inside list */
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST_GUIDE_CHANNEL, m_iSelected_GUIDE);
    g_windowManager.SendMessage(msg);
  }
  else if (m_iGuideView == GUIDE_VIEW_NOW)
  {
    m_guideGrid = NULL;
    SET_CONTROL_HIDDEN(8001);
    SET_CONTROL_HIDDEN(8003);
    SET_CONTROL_VISIBLE(8002);

    strLabel.Format("%s: %s", g_localizeStrings.Get(18050), g_localizeStrings.Get(18101));

    if (cPVREpgs::GetEPGNow(m_vecItems, RadioPlaying) > 0)
    {
      CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_GUIDE_NOW_NEXT, 0, 0, m_vecItems);
      g_windowManager.SendMessage(msg);
    }

    /* Set Selected item inside list */
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST_GUIDE_NOW_NEXT, m_iSelected_GUIDE);
    g_windowManager.SendMessage(msg);
  }
  else if (m_iGuideView == GUIDE_VIEW_NEXT)
  {
    m_guideGrid = NULL;
    SET_CONTROL_HIDDEN(8001);
    SET_CONTROL_HIDDEN(8003);
    SET_CONTROL_VISIBLE(8002);

    strLabel.Format("%s: %s", g_localizeStrings.Get(18050), g_localizeStrings.Get(18102));

    if (cPVREpgs::GetEPGNext(m_vecItems, RadioPlaying) > 0)
    {
      CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_GUIDE_NOW_NEXT, 0, 0, m_vecItems);
      g_windowManager.SendMessage(msg);
    }

    /* Set Selected item inside list */
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST_GUIDE_NOW_NEXT, m_iSelected_GUIDE);

    g_windowManager.SendMessage(msg);
  }
  else if (m_iGuideView == GUIDE_VIEW_TIMELINE)
  {
    SET_CONTROL_HIDDEN(8001);
    SET_CONTROL_HIDDEN(8002);
    SET_CONTROL_VISIBLE(8003);

    strLabel.Format("%s: %s", g_localizeStrings.Get(18050), g_localizeStrings.Get(18103));

    if (cPVREpgs::GetEPGAll(m_vecItems, RadioPlaying) > 0)
    {
      time_t time_start;
      time_t time_end;
      CDateTime       now = CDateTime::GetCurrentDateTime();

      CDateTime m_gridStart = now - CDateTimeSpan(0, 0, 0, (now.GetMinute() % 30) * 60 + now.GetSecond())
                              - CDateTimeSpan(0, g_guiSettings.GetInt("pvrmenu.lingertime") / 60, g_guiSettings.GetInt("pvrmenu.lingertime") % 60, 0);
      CDateTime m_gridEnd = m_gridStart + CDateTimeSpan(g_guiSettings.GetInt("pvrmenu.daystodisplay"), 0, 0, 0);

      m_gridStart.GetAsTime(time_start);
      m_gridEnd.GetAsTime(time_end);

      CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_TIMELINE, time_start, time_end, m_vecItems);
      g_windowManager.SendMessage(msg);
      m_guideGrid = (CGUIEPGGridContainer*)GetControl(CONTROL_LIST_TIMELINE);
    }

    /* Set Selected item inside list */
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST_TIMELINE, m_iSelected_GUIDE);
    g_windowManager.SendMessage(msg);
  }

  SET_CONTROL_LABEL(CONTROL_BTNGUIDE, strLabel);

  strLabel.Format("%s - %s", g_localizeStrings.Get(9), g_localizeStrings.Get(18050));
  SET_CONTROL_LABEL(CONTROL_LABELHEADER, strLabel);
}

/**
 * \brief Update TV Channels list
 */
void CGUIWindowTV::UpdateChannelsTV()
{
  SET_CONTROL_HIDDEN(CONTROL_LIST_CHANNELS_TV);

  m_vecItems->Clear();

  int channels;
  if (!m_bShowHiddenChannels)
    channels = PVRChannelsTV.GetChannels(m_vecItems, m_iCurrentTVGroup);
  else
    channels = PVRChannelsTV.GetHiddenChannels(m_vecItems);

  if (channels > 0)
  {
    CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_CHANNELS_TV, 0, 0, m_vecItems);
    g_windowManager.SendMessage(msg);
  }
  else if (m_bShowHiddenChannels)
  {
    m_bShowHiddenChannels = false;
    UpdateChannelsTV();
    return;
  }
  else if (m_iCurrentTVGroup != -1)
  {
    m_iCurrentTVGroup = PVRChannelGroups.GetNextGroupID(m_iCurrentTVGroup);
    UpdateChannelsTV();
    return;
  }

  /* Set Selected item inside list */
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST_CHANNELS_TV, m_iSelected_CHANNELS_TV);
  g_windowManager.SendMessage(msg);

  CStdString strLabel;
  if (m_bShowHiddenChannels)
    strLabel.Format("%s - %s: %s", g_localizeStrings.Get(9), g_localizeStrings.Get(18051), g_localizeStrings.Get(18151));
  else
    strLabel.Format("%s - %s: %s", g_localizeStrings.Get(9), g_localizeStrings.Get(18051), PVRChannelGroups.GetGroupName(m_iCurrentTVGroup));
  SET_CONTROL_LABEL(CONTROL_LABELHEADER, strLabel);

  SET_CONTROL_VISIBLE(CONTROL_LIST_CHANNELS_TV);
}

/**
 * \brief Update Radio Channels list
 */
void CGUIWindowTV::UpdateChannelsRadio()
{
  SET_CONTROL_HIDDEN(CONTROL_LIST_CHANNELS_RADIO);

  m_vecItems->Clear();

  int channels;
  if (!m_bShowHiddenChannels)
    channels = PVRChannelsRadio.GetChannels(m_vecItems, m_iCurrentRadioGroup);
  else
    channels = PVRChannelsRadio.GetHiddenChannels(m_vecItems);

  if (channels > 0)
  {
    CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_CHANNELS_RADIO, 0, 0, m_vecItems);
    g_windowManager.SendMessage(msg);
  }
  else if (m_bShowHiddenChannels)
  {
    m_bShowHiddenChannels = false;
    UpdateChannelsRadio();
    return;
  }
  else if (m_iCurrentRadioGroup != -1)
  {
    m_iCurrentRadioGroup = PVRChannelGroups.GetNextGroupID(m_iCurrentRadioGroup);
    UpdateChannelsRadio();
    return;
  }

  /* Set Selected item inside list */
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST_CHANNELS_RADIO, m_iSelected_CHANNELS_RADIO);
  g_windowManager.SendMessage(msg);

  CStdString strLabel;
  if (m_bShowHiddenChannels)
    strLabel.Format("%s - %s: %s", g_localizeStrings.Get(9), g_localizeStrings.Get(18052), g_localizeStrings.Get(18151));
  else
    strLabel.Format("%s - %s: %s", g_localizeStrings.Get(9), g_localizeStrings.Get(18052), PVRChannelGroups.GetGroupName(m_iCurrentTVGroup));
  SET_CONTROL_LABEL(CONTROL_LABELHEADER, strLabel);

  SET_CONTROL_VISIBLE(CONTROL_LIST_CHANNELS_RADIO);
}

/**
 * \brief Update Radio Channels list
 */
void CGUIWindowTV::UpdateRecordings()
{
  SET_CONTROL_HIDDEN(CONTROL_LIST_RECORDINGS);

  m_vecItems->Clear();
  PVRRecordings.GetRecordings(m_vecItems);
  {
    CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_RECORDINGS, 0, 0, m_vecItems);
    g_windowManager.SendMessage(msg);
  }

  /* Set Selected item inside list */
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST_RECORDINGS, m_iSelected_RECORDINGS);
    g_windowManager.SendMessage(msg);
  }

  CStdString strLabel;
  strLabel.Format("%s - %s", g_localizeStrings.Get(9), g_localizeStrings.Get(18066));
  SET_CONTROL_LABEL(CONTROL_LABELHEADER, strLabel);

  SET_CONTROL_VISIBLE(CONTROL_LIST_RECORDINGS);
}

/**
 * \brief Update Radio Channels list
 */
void CGUIWindowTV::UpdateTimers()
{
  PVRTimers.Update();

  SET_CONTROL_HIDDEN(CONTROL_LIST_TIMERS);

  m_vecItems->Clear();

  cPVRTimerInfoTag newtimer;
  newtimer.SetPath("pvr://timers/add.timer");
  CFileItemPtr addtimer(new CFileItem(newtimer));
  m_vecItems->Add(addtimer);

  PVRTimers.GetTimers(m_vecItems);
  CGUIMessage msg1(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_TIMERS, 0, 0, m_vecItems);
  g_windowManager.SendMessage(msg1);

  /* Set Selected item inside list */
  CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST_TIMERS, m_iSelected_TIMERS);
  g_windowManager.SendMessage(msg2);

  CStdString strLabel;
  strLabel.Format("%s - %s", g_localizeStrings.Get(9), g_localizeStrings.Get(18054));
  SET_CONTROL_LABEL(CONTROL_LABELHEADER, strLabel);

  SET_CONTROL_VISIBLE(CONTROL_LIST_TIMERS);
}

void CGUIWindowTV::UpdateSearch()
{
  int items = 0;

  SET_CONTROL_HIDDEN(CONTROL_LIST_SEARCH);

  m_vecItems->Clear();

  if (m_bSearchConfirmed)
  {
    CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (dlgProgress)
    {
      dlgProgress->SetHeading(194);
      dlgProgress->SetLine(0, m_searchfilter.m_SearchString);
      dlgProgress->SetLine(1, "");
      dlgProgress->SetLine(2, "");
      dlgProgress->StartModal();
      dlgProgress->Progress();
    }

    items = cPVREpgs::GetEPGSearch(m_vecItems, m_searchfilter);

    if (dlgProgress)
      dlgProgress->Close();

    if (items == 0)
    {
      CGUIDialogOK::ShowAndGetInput(194, 284, 0, 0);
      m_bSearchConfirmed = false;
    }
  }

  if (items == 0)
  {
    CFileItemPtr addsearch(new CFileItem(g_localizeStrings.Get(18164)));
    m_vecItems->Add(addsearch);
  }
  else
  {
    m_vecItems->Sort(m_iSortMethod_SEARCH, m_iSortOrder_SEARCH);
  }

  CGUIMessage msg1(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_SEARCH, 0, 0, m_vecItems);
  g_windowManager.SendMessage(msg1);

  /* Set Selected item inside list */
  CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST_SEARCH, m_iSelected_SEARCH);
  g_windowManager.SendMessage(msg2);

  CStdString strLabel;
  strLabel.Format("%s - %s", g_localizeStrings.Get(9), g_localizeStrings.Get(283));
  SET_CONTROL_LABEL(CONTROL_LABELHEADER, strLabel);

  SET_CONTROL_VISIBLE(CONTROL_LIST_SEARCH);
}

/**
 * \brief Update changed buttons
 */
void CGUIWindowTV::UpdateButtons()
{
  CStdString strLabel;

  if (m_iGuideView == GUIDE_VIEW_CHANNEL)
  {
    CStdString strChannel;
    bool RadioPlaying;
    int CurrentChannel;
    g_PVRManager.GetCurrentChannel(&CurrentChannel, &RadioPlaying);

    if (!RadioPlaying)
      strChannel = PVRChannelsTV.GetNameForChannel(CurrentChannel);
    else
      strChannel = PVRChannelsRadio.GetNameForChannel(CurrentChannel);

    strLabel.Format("%s: %s", g_localizeStrings.Get(18050), strChannel.c_str());
  }
  else if (m_iGuideView == GUIDE_VIEW_NOW)
  {
    strLabel.Format("%s: %s", g_localizeStrings.Get(18050), g_localizeStrings.Get(18101));
  }
  else if (m_iGuideView == GUIDE_VIEW_NEXT)
  {
    strLabel.Format("%s: %s", g_localizeStrings.Get(18050), g_localizeStrings.Get(18102));
  }
  else if (m_iGuideView == GUIDE_VIEW_TIMELINE)
  {
    strLabel.Format("%s: %s", g_localizeStrings.Get(18050), g_localizeStrings.Get(18103));
  }

  SET_CONTROL_LABEL(CONTROL_BTNGUIDE, strLabel);
}

void CGUIWindowTV::UpdateData(TVWindow update)
{
  if (m_iCurrSubTVWindow == TV_WINDOW_TV_PROGRAM && update == TV_WINDOW_TV_PROGRAM)
  {

  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_TV && update == TV_WINDOW_CHANNELS_TV)
  {
    SET_CONTROL_HIDDEN(CONTROL_AREA_CHANNELS_TV);
    UpdateChannelsTV();
    SET_CONTROL_VISIBLE(CONTROL_AREA_CHANNELS_TV);
  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_CHANNELS_RADIO && update == TV_WINDOW_CHANNELS_RADIO)
  {
    SET_CONTROL_HIDDEN(CONTROL_AREA_CHANNELS_RADIO);
    UpdateChannelsRadio();
    SET_CONTROL_VISIBLE(CONTROL_AREA_CHANNELS_RADIO);
  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_RECORDINGS && update == TV_WINDOW_RECORDINGS)
  {
    SET_CONTROL_HIDDEN(CONTROL_AREA_RECORDINGS);
    UpdateRecordings();
    SET_CONTROL_VISIBLE(CONTROL_AREA_RECORDINGS);
  }
  else if (m_iCurrSubTVWindow == TV_WINDOW_TIMERS && update == TV_WINDOW_TIMERS)
  {
    SET_CONTROL_HIDDEN(CONTROL_AREA_TIMERS);
    UpdateTimers();
    SET_CONTROL_VISIBLE(CONTROL_AREA_TIMERS);
  }
  UpdateButtons();
}
