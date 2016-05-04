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

#include "GUIDialogPVRGuideSearch.h"

#include <utility>

#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "epg/EpgSearchFilter.h"
#include "guilib/GUIEditControl.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/PVRManager.h"
#include "utils/StringUtils.h"

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
  int iChannelGroup = GetSpinValue(CONTROL_SPIN_GROUPS);

  std::vector< std::pair<std::string, int> > labels;
  labels.push_back(std::make_pair(g_localizeStrings.Get(19217), EPG_SEARCH_UNSET));

  CPVRChannelGroupPtr group;
  if (iChannelGroup == EPG_SEARCH_UNSET)
    group = g_PVRChannelGroups->GetGroupAll(m_searchFilter->m_bIsRadio);
  else
    group = g_PVRChannelGroups->GetByIdFromAll(iChannelGroup);

  if (!group)
    group = g_PVRChannelGroups->GetGroupAll(m_searchFilter->m_bIsRadio);

  std::vector<PVRChannelGroupMember> groupMembers(group->GetMembers());
  for (std::vector<PVRChannelGroupMember>::const_iterator it = groupMembers.begin(); it != groupMembers.end(); ++it)
  {
    if ((*it).channel)
      labels.push_back(std::make_pair((*it).channel->ChannelName(), (*it).iChannelNumber));
  }

  SET_CONTROL_LABELS(CONTROL_SPIN_CHANNELS, m_searchFilter->m_iChannelNumber, &labels);
}

void CGUIDialogPVRGuideSearch::UpdateGroupsSpin(void)
{
  std::vector< std::pair<std::string, int> > labels;

  /* groups */
  std::vector<CPVRChannelGroupPtr> groups = g_PVRChannelGroups->Get(m_searchFilter->m_bIsRadio)->GetMembers();
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = groups.begin(); it != groups.end(); ++it)
    labels.push_back(std::make_pair((*it)->GroupName(), (*it)->GroupID()));

  SET_CONTROL_LABELS(CONTROL_SPIN_GROUPS, m_searchFilter->m_iChannelGroup, &labels);
}

void CGUIDialogPVRGuideSearch::UpdateGenreSpin(void)
{
  std::vector< std::pair<std::string, int> > labels;
  labels.push_back(std::make_pair(g_localizeStrings.Get(593),   EPG_SEARCH_UNSET));
  labels.push_back(std::make_pair(g_localizeStrings.Get(19500), EPG_EVENT_CONTENTMASK_MOVIEDRAMA));
  labels.push_back(std::make_pair(g_localizeStrings.Get(19516), EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS));
  labels.push_back(std::make_pair(g_localizeStrings.Get(19532), EPG_EVENT_CONTENTMASK_SHOW));
  labels.push_back(std::make_pair(g_localizeStrings.Get(19548), EPG_EVENT_CONTENTMASK_SPORTS));
  labels.push_back(std::make_pair(g_localizeStrings.Get(19564), EPG_EVENT_CONTENTMASK_CHILDRENYOUTH));
  labels.push_back(std::make_pair(g_localizeStrings.Get(19580), EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE));
  labels.push_back(std::make_pair(g_localizeStrings.Get(19596), EPG_EVENT_CONTENTMASK_ARTSCULTURE));
  labels.push_back(std::make_pair(g_localizeStrings.Get(19612), EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS));
  labels.push_back(std::make_pair(g_localizeStrings.Get(19628), EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE));
  labels.push_back(std::make_pair(g_localizeStrings.Get(19644), EPG_EVENT_CONTENTMASK_LEISUREHOBBIES));
  labels.push_back(std::make_pair(g_localizeStrings.Get(19660), EPG_EVENT_CONTENTMASK_SPECIAL));
  labels.push_back(std::make_pair(g_localizeStrings.Get(19499), EPG_EVENT_CONTENTMASK_USERDEFINED));

  SET_CONTROL_LABELS(CONTROL_SPIN_GENRE, m_searchFilter->m_iGenreType, &labels);
}

void CGUIDialogPVRGuideSearch::UpdateDurationSpin(void)
{
  /* minimum duration */
  std::vector< std::pair<std::string, int> > labels;

  labels.push_back(std::make_pair("-", EPG_SEARCH_UNSET));
  for (int i = 1; i < 12*60/5; ++i)
    labels.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(14044).c_str(), i*5), i*5));

  SET_CONTROL_LABELS(CONTROL_SPIN_MIN_DURATION, m_searchFilter->m_iMinimumDuration, &labels);

  /* maximum duration */
  labels.clear();

  labels.push_back(std::make_pair("-", EPG_SEARCH_UNSET));
  for (int i = 1; i < 12*60/5; ++i)
    labels.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(14044).c_str(), i*5), i*5));

  SET_CONTROL_LABELS(CONTROL_SPIN_MAX_DURATION, m_searchFilter->m_iMaximumDuration, &labels);
}

bool CGUIDialogPVRGuideSearch::OnMessage(CGUIMessage& message)
{
  CGUIDialog::OnMessage(message);

  switch (message.GetMessage())
  {
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

void CGUIDialogPVRGuideSearch::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  m_bConfirmed = false;
  m_bCanceled = false;
}

void CGUIDialogPVRGuideSearch::OnWindowLoaded()
{
  Update();
  return CGUIDialog::OnWindowLoaded();
}

void CGUIDialogPVRGuideSearch::ReadDateTime(const std::string &strDate, const std::string &strTime, CDateTime &dateTime) const
{
  int iHours, iMinutes;
  sscanf(strTime.c_str(), "%d:%d", &iHours, &iMinutes);
  dateTime.SetFromDBDate(strDate);
  dateTime.SetDateTime(dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetDay(), iHours, iMinutes, 0);
}

bool CGUIDialogPVRGuideSearch::IsRadioSelected(int controlID)
{
  CGUIMessage msg(GUI_MSG_IS_SELECTED, GetID(), controlID);
  OnMessage(msg);
  return (msg.GetParam1() == 1);
}

int CGUIDialogPVRGuideSearch::GetSpinValue(int controlID)
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), controlID);
  OnMessage(msg);
  return (int)msg.GetParam1();
}

std::string CGUIDialogPVRGuideSearch::GetEditValue(int controlID)
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), controlID);
  OnMessage(msg);
  return msg.GetLabel();
}

void CGUIDialogPVRGuideSearch::OnSearch()
{
  if (!m_searchFilter)
    return;

  m_searchFilter->m_strSearchTerm = GetEditValue(CONTROL_EDIT_SEARCH);

  m_searchFilter->m_bSearchInDescription = IsRadioSelected(CONTROL_BTN_INC_DESC);
  m_searchFilter->m_bIsCaseSensitive = IsRadioSelected(CONTROL_BTN_CASE_SENS);
  m_searchFilter->m_bFTAOnly = IsRadioSelected(CONTROL_BTN_FTA_ONLY);
  m_searchFilter->m_bIncludeUnknownGenres = IsRadioSelected(CONTROL_BTN_UNK_GENRE);
  m_searchFilter->m_bIgnorePresentRecordings = IsRadioSelected(CONTROL_BTN_IGNORE_REC);
  m_searchFilter->m_bIgnorePresentTimers = IsRadioSelected(CONTROL_BTN_IGNORE_TMR);
  m_searchFilter->m_bPreventRepeats = IsRadioSelected(CONTROL_SPIN_NO_REPEATS);

  m_searchFilter->m_iGenreType = GetSpinValue(CONTROL_SPIN_GENRE);
  m_searchFilter->m_iMinimumDuration = GetSpinValue(CONTROL_SPIN_MIN_DURATION);
  m_searchFilter->m_iMaximumDuration = GetSpinValue(CONTROL_SPIN_MAX_DURATION);
  m_searchFilter->m_iChannelNumber = GetSpinValue(CONTROL_SPIN_CHANNELS);
  m_searchFilter->m_iChannelGroup = GetSpinValue(CONTROL_SPIN_GROUPS);

  std::string strTmp = GetEditValue(CONTROL_EDIT_START_TIME);
  ReadDateTime(GetEditValue(CONTROL_EDIT_START_DATE), strTmp, m_searchFilter->m_startDateTime);
  strTmp = GetEditValue(CONTROL_EDIT_STOP_TIME);
  ReadDateTime(GetEditValue(CONTROL_EDIT_STOP_DATE), strTmp, m_searchFilter->m_endDateTime);
}

void CGUIDialogPVRGuideSearch::Update()
{
  if (!m_searchFilter)
    return;

  SET_CONTROL_LABEL2(CONTROL_EDIT_SEARCH, m_searchFilter->m_strSearchTerm);
  {
    CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), CONTROL_EDIT_SEARCH, CGUIEditControl::INPUT_TYPE_TEXT, 16017);
    OnMessage(msg);
  }

  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_CASE_SENS, m_searchFilter->m_bIsCaseSensitive);
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_INC_DESC, m_searchFilter->m_bSearchInDescription);
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_FTA_ONLY, m_searchFilter->m_bFTAOnly);
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_UNK_GENRE, m_searchFilter->m_bIncludeUnknownGenres);
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_IGNORE_REC, m_searchFilter->m_bIgnorePresentRecordings);
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_IGNORE_TMR, m_searchFilter->m_bIgnorePresentTimers);
  SET_CONTROL_SELECTED(GetID(), CONTROL_SPIN_NO_REPEATS, m_searchFilter->m_bPreventRepeats);

  /* Set time fields */
  SET_CONTROL_LABEL2(CONTROL_EDIT_START_TIME, m_searchFilter->m_startDateTime.GetAsLocalizedTime("", false));
  {
    CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), CONTROL_EDIT_START_TIME, CGUIEditControl::INPUT_TYPE_TIME, 14066);
    OnMessage(msg);
  }
  SET_CONTROL_LABEL2(CONTROL_EDIT_STOP_TIME, m_searchFilter->m_endDateTime.GetAsLocalizedTime("", false));
  {
    CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), CONTROL_EDIT_STOP_TIME, CGUIEditControl::INPUT_TYPE_TIME, 14066);
    OnMessage(msg);
  }
  SET_CONTROL_LABEL2(CONTROL_EDIT_START_DATE, m_searchFilter->m_startDateTime.GetAsDBDate());
  {
    CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), CONTROL_EDIT_START_DATE, CGUIEditControl::INPUT_TYPE_DATE, 14067);
    OnMessage(msg);
  }
  SET_CONTROL_LABEL2(CONTROL_EDIT_STOP_DATE, m_searchFilter->m_endDateTime.GetAsDBDate());
  {
    CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), CONTROL_EDIT_STOP_DATE, CGUIEditControl::INPUT_TYPE_DATE, 14067);
    OnMessage(msg);
  }

  UpdateDurationSpin();
  UpdateGroupsSpin();
  UpdateChannelSpin();
  UpdateGenreSpin();
}
