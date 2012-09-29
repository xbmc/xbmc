/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogPVRGuideSearch.h"
#include "Application.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISpinControlEx.h"
#include "guilib/GUIWindowManager.h"

#include "addons/include/xbmc_pvr_types.h"
#include "pvr/PVRManager.h"
#include "epg/EpgSearchFilter.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"

using namespace std;
using namespace PVR;

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

CGUIDialogPVRGuideSearch::CGUIDialogPVRGuideSearch(void) :
    CGUIDialog(WINDOW_DIALOG_PVR_GUIDE_SEARCH, "DialogPVRGuideSearch.xml"),
    m_bConfirmed(false),
    m_bCanceled(false),
    m_searchFilter(NULL)
{
}

void CGUIDialogPVRGuideSearch::UpdateChannelSpin(void)
{
  CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_CHANNELS);
  CGUISpinControlEx *pSpinGroups = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_GROUPS);
  if (!pSpin || !pSpinGroups)
    return;

  int iChannelGroup = pSpin->GetValue();

  pSpin->Clear();
  pSpin->AddLabel(g_localizeStrings.Get(19217), EPG_SEARCH_UNSET);

  int iGroupId = (iChannelGroup == EPG_SEARCH_UNSET) ?
      XBMC_INTERNAL_GROUP_TV :
      iChannelGroup;
  CPVRChannelGroupPtr group = g_PVRChannelGroups->GetByIdFromAll(iGroupId);
  if (!group)
    group = g_PVRChannelGroups->GetGroupAllTV();

  for (int iChannelPtr = 0; iChannelPtr < group->Size(); iChannelPtr++)
  {
    CFileItemPtr channel = group->GetByIndex(iChannelPtr);
    if (!channel || !channel->HasPVRChannelInfoTag())
      continue;

    int iChannelNumber = group->GetChannelNumber(*channel->GetPVRChannelInfoTag());
    pSpin->AddLabel(channel->GetPVRChannelInfoTag()->ChannelName().c_str(), iChannelNumber);
  }
}

void CGUIDialogPVRGuideSearch::UpdateGroupsSpin(void)
{
  CFileItemList groups;
  CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_GROUPS);
  if (!pSpin)
    return;

  /* tv groups */
  g_PVRChannelGroups->GetTV()->GetGroupList(&groups);
  for (int iGroupPtr = 0; iGroupPtr < groups.Size(); iGroupPtr++)
    pSpin->AddLabel(groups[iGroupPtr]->GetLabel(), atoi(groups[iGroupPtr]->GetPath()));

  /* radio groups */
  groups.ClearItems();
  g_PVRChannelGroups->GetRadio()->GetGroupList(&groups);
  for (int iGroupPtr = 0; iGroupPtr < groups.Size(); iGroupPtr++)
    pSpin->AddLabel(groups[iGroupPtr]->GetLabel(), atoi(groups[iGroupPtr]->GetPath()));

  pSpin->SetValue(m_searchFilter->m_iChannelGroup);
}

void CGUIDialogPVRGuideSearch::UpdateGenreSpin(void)
{
  CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_GENRE);
  if (!pSpin)
    return;

  pSpin->Clear();
  pSpin->AddLabel(g_localizeStrings.Get(593), EPG_SEARCH_UNSET);
  pSpin->AddLabel(g_localizeStrings.Get(19500), EPG_EVENT_CONTENTMASK_MOVIEDRAMA);
  pSpin->AddLabel(g_localizeStrings.Get(19516), EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS);
  pSpin->AddLabel(g_localizeStrings.Get(19532), EPG_EVENT_CONTENTMASK_SHOW);
  pSpin->AddLabel(g_localizeStrings.Get(19548), EPG_EVENT_CONTENTMASK_SPORTS);
  pSpin->AddLabel(g_localizeStrings.Get(19564), EPG_EVENT_CONTENTMASK_CHILDRENYOUTH);
  pSpin->AddLabel(g_localizeStrings.Get(19580), EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE);
  pSpin->AddLabel(g_localizeStrings.Get(19596), EPG_EVENT_CONTENTMASK_ARTSCULTURE);
  pSpin->AddLabel(g_localizeStrings.Get(19612), EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS);
  pSpin->AddLabel(g_localizeStrings.Get(19628), EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE);
  pSpin->AddLabel(g_localizeStrings.Get(19644), EPG_EVENT_CONTENTMASK_LEISUREHOBBIES);
  pSpin->AddLabel(g_localizeStrings.Get(19660), EPG_EVENT_CONTENTMASK_SPECIAL);
  pSpin->AddLabel(g_localizeStrings.Get(19499), EPG_EVENT_CONTENTMASK_USERDEFINED);
  pSpin->SetValue(m_searchFilter->m_iGenreType);
}

void CGUIDialogPVRGuideSearch::UpdateDurationSpin(void)
{
  /* minimum duration */
  CGUISpinControlEx *pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_MIN_DURATION);
  if (!pSpin)
    return;

  pSpin->Clear();
  pSpin->AddLabel("-", EPG_SEARCH_UNSET);
  for (int i = 1; i < 12*60/5; i++)
  {
    CStdString string;
    string.Format(g_localizeStrings.Get(14044), i*5);
    pSpin->AddLabel(string, i*5);
  }
  pSpin->SetValue(m_searchFilter->m_iMinimumDuration);

  /* maximum duration */
  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_MAX_DURATION);
  if (!pSpin)
    return;

  pSpin->Clear();
  pSpin->AddLabel("-", EPG_SEARCH_UNSET);
  for (int i = 1; i < 12*60/5; i++)
  {
    CStdString string;
    string.Format(g_localizeStrings.Get(14044),i*5);
    pSpin->AddLabel(string, i*5);
  }
  pSpin->SetValue(m_searchFilter->m_iMaximumDuration);
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
        m_bConfirmed = true;
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
        if (m_searchFilter)
        {
          m_searchFilter->Reset();
          Update();
        }

        return true;
      }
      else if (iControl == CONTROL_SPIN_GROUPS)
      {
        UpdateChannelSpin();
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

void CGUIDialogPVRGuideSearch::ReadDateTime(const CStdString &strDate, const CStdString &strTime, CDateTime &dateTime) const
{
  int iHours, iMinutes;
  sscanf(strTime, "%d:%d", &iHours, &iMinutes);
  dateTime.SetFromDBDate(strDate);
  dateTime.SetDateTime(dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetDay(), iHours, iMinutes, 0);
}

void CGUIDialogPVRGuideSearch::OnSearch()
{
  CStdString              strTmp;
  CGUISpinControlEx      *pSpin;
  CGUIEditControl        *pEdit;
  CGUIRadioButtonControl *pRadioButton;

  if (!m_searchFilter)
    return;

  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_SEARCH);
  if (pEdit) m_searchFilter->m_strSearchTerm = pEdit->GetLabel2();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_INC_DESC);
  if (pRadioButton) m_searchFilter->m_bSearchInDescription = pRadioButton->IsSelected();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_CASE_SENS);
  if (pRadioButton) m_searchFilter->m_bIsCaseSensitive = pRadioButton->IsSelected();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_FTA_ONLY);
  if (pRadioButton) m_searchFilter->m_bFTAOnly = pRadioButton->IsSelected();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_UNK_GENRE);
  if (pRadioButton) m_searchFilter->m_bIncludeUnknownGenres = pRadioButton->IsSelected();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_IGNORE_REC);
  if (pRadioButton) m_searchFilter->m_bIgnorePresentRecordings = pRadioButton->IsSelected();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_IGNORE_TMR);
  if (pRadioButton) m_searchFilter->m_bIgnorePresentTimers = pRadioButton->IsSelected();

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_SPIN_NO_REPEATS);
  if (pRadioButton) m_searchFilter->m_bPreventRepeats = pRadioButton->IsSelected();

  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_GENRE);
  if (pSpin) m_searchFilter->m_iGenreType = pSpin->GetValue();

  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_MIN_DURATION);
  if (pSpin) m_searchFilter->m_iMinimumDuration = pSpin->GetValue();

  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_MAX_DURATION);
  if (pSpin) m_searchFilter->m_iMaximumDuration = pSpin->GetValue();

  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_CHANNELS);
  if (pSpin) m_searchFilter->m_iChannelNumber = pSpin->GetValue();

  pSpin = (CGUISpinControlEx *)GetControl(CONTROL_SPIN_GROUPS);
  if (pSpin) m_searchFilter->m_iChannelGroup = pSpin->GetValue();

  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_START_TIME);
  if (pEdit) strTmp = pEdit->GetLabel2();

  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_START_DATE);
  if (pEdit) ReadDateTime(pEdit->GetLabel2(), strTmp, m_searchFilter->m_startDateTime);
  strTmp.clear();

  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_STOP_TIME);
  if (pEdit) strTmp = pEdit->GetLabel2();

  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_STOP_DATE);
  if (pEdit) ReadDateTime(pEdit->GetLabel2(), strTmp, m_searchFilter->m_endDateTime);
}

void CGUIDialogPVRGuideSearch::Update()
{
  CGUIEditControl        *pEdit;
  CGUIRadioButtonControl *pRadioButton;

  if (!m_searchFilter)
    return;

  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_SEARCH);
  if (pEdit)
  {
    pEdit->SetLabel2(m_searchFilter->m_strSearchTerm);
    pEdit->SetInputType(CGUIEditControl::INPUT_TYPE_TEXT, 16017);
  }

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_CASE_SENS);
  if (pRadioButton) pRadioButton->SetSelected(m_searchFilter->m_bIsCaseSensitive);

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_INC_DESC);
  if (pRadioButton) pRadioButton->SetSelected(m_searchFilter->m_bSearchInDescription);

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_FTA_ONLY);
  if (pRadioButton) pRadioButton->SetSelected(m_searchFilter->m_bFTAOnly);

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_UNK_GENRE);
  if (pRadioButton) pRadioButton->SetSelected(m_searchFilter->m_bIncludeUnknownGenres);

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_IGNORE_REC);
  if (pRadioButton) pRadioButton->SetSelected(m_searchFilter->m_bIgnorePresentRecordings);

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_BTN_IGNORE_TMR);
  if (pRadioButton) pRadioButton->SetSelected(m_searchFilter->m_bIgnorePresentTimers);

  pRadioButton = (CGUIRadioButtonControl *)GetControl(CONTROL_SPIN_NO_REPEATS);
  if (pRadioButton) pRadioButton->SetSelected(m_searchFilter->m_bPreventRepeats);

  /* Set time fields */
  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_START_TIME);
  if (pEdit)
  {
    pEdit->SetLabel2(m_searchFilter->m_endDateTime.GetAsLocalizedTime("", false));
    pEdit->SetInputType(CGUIEditControl::INPUT_TYPE_TIME, 14066);
  }
  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_STOP_TIME);
  if (pEdit)
  {
    pEdit->SetLabel2(m_searchFilter->m_startDateTime.GetAsLocalizedTime("", false));
    pEdit->SetInputType(CGUIEditControl::INPUT_TYPE_TIME, 14066);
  }
  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_START_DATE);
  if (pEdit)
  {
    pEdit->SetLabel2(m_searchFilter->m_startDateTime.GetAsDBDate());
    pEdit->SetInputType(CGUIEditControl::INPUT_TYPE_DATE, 14067);
  }
  pEdit = (CGUIEditControl *)GetControl(CONTROL_EDIT_STOP_DATE);
  if (pEdit)
  {
    pEdit->SetLabel2(m_searchFilter->m_endDateTime.GetAsDBDate());
    pEdit->SetInputType(CGUIEditControl::INPUT_TYPE_DATE, 14067);
  }

  UpdateDurationSpin();
  UpdateGroupsSpin();
  UpdateChannelSpin();
  UpdateGenreSpin();
}
