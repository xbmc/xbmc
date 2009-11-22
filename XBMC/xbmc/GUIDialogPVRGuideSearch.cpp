/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "Application.h"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "GUIDialogPVRGuideSearch.h"
#include "utils/PVREpg.h"
#include "utils/PVRChannels.h"
#include "GUISpinControlEx.h"
#include "GUIEditControl.h"
#include "GUIRadioButtonControl.h"

using namespace std;

#define CONTROL_EDIT_SEARCH       9
#define CONTROL_BTN_INC_DESC      10
#define CONTROL_BTN_CASE_SENS     11
#define CONTROL_SPIN_MIN_DURATION 12
#define CONTROL_SPIN_MAX_DURATION 13
#define CONTROL_EDIT_START_DATE   14
#define CONTROL_EDIT_STOP_DATE    15
#define CONTROL_EDIT_START_TIME   16
#define CONTROL_EDIT_STOP_TIME    17
#define CONTROL_SPIN_GENRE        18
#define CONTROL_SPIN_NO_REPEATS   19
#define CONTROL_BTN_UNK_GENRE     20
#define CONTROL_SPIN_GROUPS       21
#define CONTROL_BTN_FTA_ONLY      22
#define CONTROL_SPIN_CHANNELS     23
#define CONTROL_BTN_IGNORE_TMR    24
#define CONTROL_BTN_CANCEL        25
#define CONTROL_BTN_SEARCH        26
#define CONTROL_BTN_IGNORE_REC    27
#define CONTROL_BTN_DEFAULTS      28

CGUIDialogPVRGuideSearch::CGUIDialogPVRGuideSearch(void)
    : CGUIDialog(WINDOW_DIALOG_PVR_GUIDE_SEARCH, "DialogPVRGuideSearch.xml")
{
  m_bConfirmed   = false;
  m_searchfilter = NULL;
}

CGUIDialogPVRGuideSearch::~CGUIDialogPVRGuideSearch(void)
{
}

bool CGUIDialogPVRGuideSearch::OnMessage(CGUIMessage& message)
{
  CGUIDialog::OnMessage(message);

  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      m_bConfirmed = false;
      m_bCanceled = false;
    }
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_SEARCH)
      {
        OnSearch();
        m_bCanceled = false;
        Close();
        return true;
      }
      else if (iControl == CONTROL_BTN_CANCEL)
      {
        Close();
        m_bCanceled = true;
        return true;
      }
      else if (iControl == CONTROL_BTN_DEFAULTS)
      {
        if (m_searchfilter)
        {
          m_searchfilter->SetDefaults();
          Update();
        }

        return true;
      }
      else if (iControl == CONTROL_SPIN_GROUPS)
      {
        /* Set Channel list spin */
        CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_CHANNELS);
        if (pSpin)
        {
          CFileItemList channelslist_tv;
          PVRChannelsTV.GetChannels(&channelslist_tv, m_searchfilter->m_Group);

          pSpin->Clear();
          pSpin->AddLabel(g_localizeStrings.Get(18131), -1);
          pSpin->AddLabel(g_localizeStrings.Get(18051), -2);
          pSpin->AddLabel(g_localizeStrings.Get(18052), -3);

          for (int i = 0; i < channelslist_tv.Size(); i++)
          {
            int chanNumber = channelslist_tv[i]->GetPVRChannelInfoTag()->Number();
            CStdString string;
            string.Format("%i %s", chanNumber, channelslist_tv[i]->GetPVRChannelInfoTag()->Name().c_str());
            pSpin->AddLabel(string, chanNumber);
          }
        }
        return true;
      }
    }
    break;
  }

  return false;
}

void CGUIDialogPVRGuideSearch::OnWindowLoaded()
{
  Update();
  return CGUIDialog::OnWindowLoaded();
}

void CGUIDialogPVRGuideSearch::OnSearch()
{
  CGUISpinControlEx      *pSpin;
  CGUIEditControl        *pEdit;
  CGUIRadioButtonControl *pRadioButton;
  CDateTime               dateTime;

  if (!m_searchfilter)
    return;

  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_SEARCH);
  if (pEdit) m_searchfilter->m_SearchString = pEdit->GetLabel2();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_INC_DESC);
  if (pRadioButton) m_searchfilter->m_SearchDescription = pRadioButton->IsSelected();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_CASE_SENS);
  if (pRadioButton) m_searchfilter->m_CaseSensitive = pRadioButton->IsSelected();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_FTA_ONLY);
  if (pRadioButton) m_searchfilter->m_FTAOnly = pRadioButton->IsSelected();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_UNK_GENRE);
  if (pRadioButton) m_searchfilter->m_IncUnknGenres = pRadioButton->IsSelected();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_IGNORE_REC);
  if (pRadioButton) m_searchfilter->m_IgnPresentRecords = pRadioButton->IsSelected();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_IGNORE_TMR);
  if (pRadioButton) m_searchfilter->m_IgnPresentTimers = pRadioButton->IsSelected();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_SPIN_NO_REPEATS);
  if (pRadioButton) m_searchfilter->m_PreventRepeats = pRadioButton->IsSelected();

  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_GENRE);
  if (pSpin) m_searchfilter->m_GenreType = pSpin->GetValue();

  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_MIN_DURATION);
  if (pSpin) m_searchfilter->m_minDuration = pSpin->GetValue();

  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_MAX_DURATION);
  if (pSpin) m_searchfilter->m_maxDuration = pSpin->GetValue();

  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_CHANNELS);
  if (pSpin) m_searchfilter->m_ChannelNumber = pSpin->GetValue();

  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_GROUPS);
  if (pSpin) m_searchfilter->m_Group = pSpin->GetValue();

  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_START_TIME);
  if (pEdit)
  {
    dateTime.SetFromDBTime(pEdit->GetLabel2());
    dateTime.GetAsSystemTime(m_searchfilter->m_startTime);
  }

  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_STOP_TIME);
  if (pEdit)
  {
    dateTime.SetFromDBTime(pEdit->GetLabel2());
    dateTime.GetAsSystemTime(m_searchfilter->m_endTime);
  }

  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_START_DATE);
  if (pEdit)
  {
    dateTime.SetFromDBDate(pEdit->GetLabel2());
    dateTime.GetAsSystemTime(m_searchfilter->m_startDate);
  }

  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_STOP_DATE);
  if (pEdit)
  {
    dateTime.SetFromDBDate(pEdit->GetLabel2());
    dateTime.GetAsSystemTime(m_searchfilter->m_endDate);
  }

  m_bConfirmed = true;
}

void CGUIDialogPVRGuideSearch::Update()
{
  CGUISpinControlEx      *pSpin;
  CGUIEditControl        *pEdit;
  CGUIRadioButtonControl *pRadioButton;

  if (!m_searchfilter)
    return;

  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_SEARCH);
  if (pEdit)
  {
    pEdit->SetLabel2(m_searchfilter->m_SearchString);
    pEdit->SetInputType(CGUIEditControl::INPUT_TYPE_TEXT, 16017);
  }

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_CASE_SENS);
  if (pRadioButton) pRadioButton->SetSelected(m_searchfilter->m_CaseSensitive);

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_INC_DESC);
  if (pRadioButton) pRadioButton->SetSelected(m_searchfilter->m_SearchDescription);

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_FTA_ONLY);
  if (pRadioButton) pRadioButton->SetSelected(m_searchfilter->m_FTAOnly);

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_UNK_GENRE);
  if (pRadioButton) pRadioButton->SetSelected(m_searchfilter->m_IncUnknGenres);

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_IGNORE_REC);
  if (pRadioButton) pRadioButton->SetSelected(m_searchfilter->m_IgnPresentRecords);

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_IGNORE_TMR);
  if (pRadioButton) pRadioButton->SetSelected(m_searchfilter->m_IgnPresentTimers);

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_SPIN_NO_REPEATS);
  if (pRadioButton) pRadioButton->SetSelected(m_searchfilter->m_PreventRepeats);

  /* Set duration list spin */
  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_MIN_DURATION);
  if (pSpin)
  {
    pSpin->Clear();
    pSpin->AddLabel("-", -1);
    for (int i = 1; i < 12*60/5; i++)
    {
      CStdString string;
      string.Format(g_localizeStrings.Get(14044),i*5);
      pSpin->AddLabel(string, i*5);
      pSpin->SetValue(m_searchfilter->m_minDuration);
    }
  }

  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_MAX_DURATION);
  if (pSpin)
  {
    pSpin->Clear();
    pSpin->AddLabel("-", -1);
    for (int i = 1; i < 12*60/5; i++)
    {
      CStdString string;
      string.Format(g_localizeStrings.Get(14044),i*5);
      pSpin->AddLabel(string, i*5);
      pSpin->SetValue(m_searchfilter->m_maxDuration);
    }
  }

  /* Set time fields */
  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_START_TIME);
  if (pEdit)
  {
    CDateTime time = m_searchfilter->m_startTime;
    pEdit->SetLabel2(time.GetAsLocalizedTime("", false));
    pEdit->SetInputType(CGUIEditControl::INPUT_TYPE_TIME, 14066);
  }
  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_STOP_TIME);
  if (pEdit)
  {
    CDateTime time = m_searchfilter->m_endTime;
    pEdit->SetLabel2(time.GetAsLocalizedTime("", false));
    pEdit->SetInputType(CGUIEditControl::INPUT_TYPE_TIME, 14066);
  }
  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_START_DATE);
  if (pEdit)
  {
    CDateTime date = m_searchfilter->m_startDate;
    pEdit->SetLabel2(date.GetAsDBDate());
    pEdit->SetInputType(CGUIEditControl::INPUT_TYPE_DATE, 14067);
  }
  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_STOP_DATE);
  if (pEdit)
  {
    CDateTime date = m_searchfilter->m_endDate;
    pEdit->SetLabel2(date.GetAsDBDate());
    pEdit->SetInputType(CGUIEditControl::INPUT_TYPE_DATE, 14067);
  }

  /* Set Channel list spin */
  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_CHANNELS);
  if (pSpin)
  {
    CFileItemList channelslist_tv;
    PVRChannelsTV.GetChannels(&channelslist_tv, m_searchfilter->m_Group);

    pSpin->Clear();
    pSpin->AddLabel(g_localizeStrings.Get(18131), -1);
    pSpin->AddLabel(g_localizeStrings.Get(18051), -2);
    pSpin->AddLabel(g_localizeStrings.Get(18052), -3);

    for (int i = 0; i < channelslist_tv.Size(); i++)
    {
      int chanNumber = channelslist_tv[i]->GetPVRChannelInfoTag()->Number();
      CStdString string;
      string.Format("%i %s", chanNumber, channelslist_tv[i]->GetPVRChannelInfoTag()->Name().c_str());
      pSpin->AddLabel(string, chanNumber);
    }
    pSpin->SetValue(m_searchfilter->m_ChannelNumber);
  }

  /* Set Group list spin */
  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_GROUPS);
  if (pSpin)
  {
    CFileItemList grouplist;
    PVRChannelGroups.GetGroupList(&grouplist);

    pSpin->Clear();
    pSpin->AddLabel(g_localizeStrings.Get(593), -1);

    for (int i = 0; i < grouplist.Size(); i++)
      pSpin->AddLabel(grouplist[i]->GetLabel(), atoi(grouplist[i]->m_strPath));

    pSpin->SetValue(m_searchfilter->m_Group);
  }

  /* Set Genre list spin */
  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_GENRE);
  if (pSpin)
  {
    pSpin->Clear();
    pSpin->AddLabel(g_localizeStrings.Get(593), -1);
    pSpin->AddLabel(g_localizeStrings.Get(18110), EVCONTENTMASK_MOVIEDRAMA);
    pSpin->AddLabel(g_localizeStrings.Get(18111), EVCONTENTMASK_NEWSCURRENTAFFAIRS);
    pSpin->AddLabel(g_localizeStrings.Get(18112), EVCONTENTMASK_SHOW);
    pSpin->AddLabel(g_localizeStrings.Get(18113), EVCONTENTMASK_SPORTS);
    pSpin->AddLabel(g_localizeStrings.Get(18114), EVCONTENTMASK_CHILDRENYOUTH);
    pSpin->AddLabel(g_localizeStrings.Get(18157), EVCONTENTMASK_MUSICBALLETDANCE);
    pSpin->AddLabel(g_localizeStrings.Get(18158), EVCONTENTMASK_ARTSCULTURE);
    pSpin->AddLabel(g_localizeStrings.Get(18121), EVCONTENTMASK_SOCIALPOLITICALECONOMICS);
    pSpin->AddLabel(g_localizeStrings.Get(18117), EVCONTENTMASK_EDUCATIONALSCIENCE);
    pSpin->AddLabel(g_localizeStrings.Get(18118), EVCONTENTMASK_LEISUREHOBBIES);
    pSpin->AddLabel(g_localizeStrings.Get(18122), EVCONTENTMASK_SPECIAL);
    pSpin->AddLabel(g_localizeStrings.Get(18119), EVCONTENTMASK_USERDEFINED);
    pSpin->SetValue(m_searchfilter->m_GenreType);
  }
}

bool CGUIDialogPVRGuideSearch::IsConfirmed() const
{
  return m_bConfirmed;
}

bool CGUIDialogPVRGuideSearch::IsCanceled() const
{
  return m_bCanceled;
}

void CGUIDialogPVRGuideSearch::SetFilterData(EPGSearchFilter *searchfilter)
{
  m_searchfilter = searchfilter;
}
