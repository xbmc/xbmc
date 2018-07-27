/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRGuideSearch.h"

#include <utility>

#include "ServiceBroker.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIEditControl.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgSearchFilter.h"

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
    group = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(m_searchFilter->IsRadio());
  else
    group = CServiceBroker::GetPVRManager().ChannelGroups()->GetByIdFromAll(iChannelGroup);

  if (!group)
    group = CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(m_searchFilter->IsRadio());

  m_channelNumbersMap.clear();
  const std::vector<PVRChannelGroupMember> groupMembers(group->GetMembers());
  int iIndex = 0;
  int iSelectedChannel = EPG_SEARCH_UNSET;
  for (const auto& groupMember : groupMembers)
  {
    if (groupMember.channel)
    {
      labels.emplace_back(std::make_pair(groupMember.channel->ChannelName(), iIndex));
      m_channelNumbersMap.insert(std::make_pair(iIndex, groupMember.channelNumber));

      if (iSelectedChannel == EPG_SEARCH_UNSET && groupMember.channelNumber == m_searchFilter->GetChannelNumber())
        iSelectedChannel = iIndex;

      ++iIndex;
    }
  }

  SET_CONTROL_LABELS(CONTROL_SPIN_CHANNELS, iSelectedChannel, &labels);
}

void CGUIDialogPVRGuideSearch::UpdateGroupsSpin(void)
{
  std::vector< std::pair<std::string, int> > labels;

  /* groups */
  std::vector<CPVRChannelGroupPtr> groups = CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_searchFilter->IsRadio())->GetMembers();
  for (std::vector<CPVRChannelGroupPtr>::const_iterator it = groups.begin(); it != groups.end(); ++it)
    labels.push_back(std::make_pair((*it)->GroupName(), (*it)->GroupID()));

  SET_CONTROL_LABELS(CONTROL_SPIN_GROUPS, m_searchFilter->GetChannelGroup(), &labels);
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

  SET_CONTROL_LABELS(CONTROL_SPIN_GENRE, m_searchFilter->GetGenreType(), &labels);
}

void CGUIDialogPVRGuideSearch::UpdateDurationSpin(void)
{
  /* minimum duration */
  std::vector< std::pair<std::string, int> > labels;

  labels.push_back(std::make_pair("-", EPG_SEARCH_UNSET));
  for (int i = 1; i < 12*60/5; ++i)
    labels.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(14044).c_str(), i*5), i*5));

  SET_CONTROL_LABELS(CONTROL_SPIN_MIN_DURATION, m_searchFilter->GetMinimumDuration(), &labels);

  /* maximum duration */
  labels.clear();

  labels.push_back(std::make_pair("-", EPG_SEARCH_UNSET));
  for (int i = 1; i < 12*60/5; ++i)
    labels.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(14044).c_str(), i*5), i*5));

  SET_CONTROL_LABELS(CONTROL_SPIN_MAX_DURATION, m_searchFilter->GetMaximumDuration(), &labels);
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

CDateTime CGUIDialogPVRGuideSearch::ReadDateTime(const std::string &strDate, const std::string &strTime) const
{
  CDateTime dateTime;
  int iHours, iMinutes;
  sscanf(strTime.c_str(), "%d:%d", &iHours, &iMinutes);
  dateTime.SetFromDBDate(strDate);
  dateTime.SetDateTime(dateTime.GetYear(), dateTime.GetMonth(), dateTime.GetDay(), iHours, iMinutes, 0);
  return dateTime;
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
  return msg.GetParam1();
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

  m_searchFilter->SetSearchTerm(GetEditValue(CONTROL_EDIT_SEARCH));

  m_searchFilter->SetSearchInDescription(IsRadioSelected(CONTROL_BTN_INC_DESC));
  m_searchFilter->SetCaseSensitive(IsRadioSelected(CONTROL_BTN_CASE_SENS));
  m_searchFilter->SetFreeToAirOnly(IsRadioSelected(CONTROL_BTN_FTA_ONLY));
  m_searchFilter->SetIncludeUnknownGenres(IsRadioSelected(CONTROL_BTN_UNK_GENRE));
  m_searchFilter->SetIgnorePresentRecordings(IsRadioSelected(CONTROL_BTN_IGNORE_REC));
  m_searchFilter->SetIgnorePresentTimers(IsRadioSelected(CONTROL_BTN_IGNORE_TMR));
  m_searchFilter->SetRemoveDuplicates(IsRadioSelected(CONTROL_SPIN_NO_REPEATS));

  m_searchFilter->SetGenreType(GetSpinValue(CONTROL_SPIN_GENRE));
  m_searchFilter->SetMinimumDuration(GetSpinValue(CONTROL_SPIN_MIN_DURATION));
  m_searchFilter->SetMaximumDuration(GetSpinValue(CONTROL_SPIN_MAX_DURATION));

  auto it = m_channelNumbersMap.find(GetSpinValue(CONTROL_SPIN_CHANNELS));
  m_searchFilter->SetChannelNumber(it == m_channelNumbersMap.end() ? CPVRChannelNumber() : (*it).second);

  m_searchFilter->SetChannelGroup(GetSpinValue(CONTROL_SPIN_GROUPS));

  m_searchFilter->SetStartDateTime(ReadDateTime(GetEditValue(CONTROL_EDIT_START_DATE), GetEditValue(CONTROL_EDIT_START_TIME)));
  m_searchFilter->SetEndDateTime(ReadDateTime(GetEditValue(CONTROL_EDIT_STOP_DATE), GetEditValue(CONTROL_EDIT_STOP_TIME)));
}

void CGUIDialogPVRGuideSearch::Update()
{
  if (!m_searchFilter)
    return;

  SET_CONTROL_LABEL2(CONTROL_EDIT_SEARCH, m_searchFilter->GetSearchTerm());
  {
    CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), CONTROL_EDIT_SEARCH, CGUIEditControl::INPUT_TYPE_TEXT, 16017);
    OnMessage(msg);
  }

  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_CASE_SENS, m_searchFilter->IsCaseSensitive());
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_INC_DESC, m_searchFilter->ShouldSearchInDescription());
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_FTA_ONLY, m_searchFilter->IsFreeToAirOnly());
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_UNK_GENRE, m_searchFilter->ShouldIncludeUnknownGenres());
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_IGNORE_REC, m_searchFilter->ShouldIgnorePresentRecordings());
  SET_CONTROL_SELECTED(GetID(), CONTROL_BTN_IGNORE_TMR, m_searchFilter->ShouldIgnorePresentTimers());
  SET_CONTROL_SELECTED(GetID(), CONTROL_SPIN_NO_REPEATS, m_searchFilter->ShouldRemoveDuplicates());

  /* Set time fields */
  SET_CONTROL_LABEL2(CONTROL_EDIT_START_TIME, m_searchFilter->GetStartDateTime().GetAsLocalizedTime("", false));
  {
    CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), CONTROL_EDIT_START_TIME, CGUIEditControl::INPUT_TYPE_TIME, 14066);
    OnMessage(msg);
  }
  SET_CONTROL_LABEL2(CONTROL_EDIT_STOP_TIME, m_searchFilter->GetEndDateTime().GetAsLocalizedTime("", false));
  {
    CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), CONTROL_EDIT_STOP_TIME, CGUIEditControl::INPUT_TYPE_TIME, 14066);
    OnMessage(msg);
  }
  SET_CONTROL_LABEL2(CONTROL_EDIT_START_DATE, m_searchFilter->GetStartDateTime().GetAsDBDate());
  {
    CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), CONTROL_EDIT_START_DATE, CGUIEditControl::INPUT_TYPE_DATE, 14067);
    OnMessage(msg);
  }
  SET_CONTROL_LABEL2(CONTROL_EDIT_STOP_DATE, m_searchFilter->GetEndDateTime().GetAsDBDate());
  {
    CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), CONTROL_EDIT_STOP_DATE, CGUIEditControl::INPUT_TYPE_DATE, 14067);
    OnMessage(msg);
  }

  UpdateDurationSpin();
  UpdateGroupsSpin();
  UpdateChannelSpin();
  UpdateGenreSpin();
}
