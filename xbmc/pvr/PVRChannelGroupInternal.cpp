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

#include "settings/GUISettings.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "log.h"

#include "PVRChannelGroupInternal.h"
#include "PVRDatabase.h"
#include "PVRManager.h"

CPVRChannelGroupInternal::CPVRChannelGroupInternal(bool bRadio) : CPVRChannelGroup(bRadio)
{
  m_iHiddenChannels = 0;
  m_bIsSorted       = false;
  m_iGroupId        = XBMC_INTERNAL_GROUPID;
  m_strGroupName    = "";
  m_iSortOrder      = 0;
}

int CPVRChannelGroupInternal::Load()
{
  int iChannelCount = LoadFromDb();

  /* try to get the channels from clients if there are none in the database */
  if (iChannelCount <= 0)
  {
    CLog::Log(LOGDEBUG, "PVRChannelGroupInternal - %s - no %s channels stored in the database. Reading channels from clients",
        __FUNCTION__, m_bRadio ? "radio" : "TV");

    iChannelCount = LoadFromClients();

    CLog::Log(LOGNOTICE, "PVRChannelGroupInternal - %s - %d %s channels added from clients",
        __FUNCTION__, iChannelCount, m_bRadio ? "radio" : "TV");
  }
  else
  {
    CLog::Log(LOGDEBUG, "PVRChannelGroupInternal - %s - %d %s channels loaded from the database",
        __FUNCTION__, iChannelCount, m_bRadio ? "radio" : "TV");
  }

  return iChannelCount;
}

void CPVRChannelGroupInternal::Unload()
{
  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    delete at(iChannelPtr);
  }

  CPVRChannelGroup::Unload();
}

bool CPVRChannelGroupInternal::Update()
{
  bool          bReturn  = false;
  CPVRDatabase *database = g_PVRManager.GetTVDatabase();

  if (database && database->Open())
  {
    CPVRChannelGroupInternal PVRChannels_tmp(m_bRadio);

    PVRChannels_tmp.LoadFromClients(false);
    bReturn = Update(&PVRChannels_tmp);

    database->Close();
  }

  return bReturn;
}

bool CPVRChannelGroupInternal::UpdateTimers(void)
{
  /* update the timers with the new channel numbers */
  for (unsigned int ptr = 0; ptr < PVRTimers.size(); ptr++)
  {
    CPVRTimerInfoTag timer = PVRTimers[ptr];
    CPVRChannel *tag = GetByClient(timer.Number(), timer.ClientID());
    if (tag)
      timer.SetNumber(tag->ChannelNumber());
  }

  return true;
}

bool CPVRChannelGroupInternal::MoveChannel(unsigned int iOldIndex, unsigned int iNewIndex, bool bSaveInDb /* = true */)
{
  bool bReturn = false;

  if (iNewIndex == iOldIndex || iNewIndex == 0)
    return bReturn;

  /* make sure the list is sorted by channel number */
  SortByChannelNumber();

  /* get the channel at the old position */
  CPVRChannel *channel = at(iOldIndex - 1);

  /* remove channel at the old position */
  erase(begin() + iOldIndex - 1);

  if (iNewIndex < size())
  {
    /* insert somewhere in the vector */
    insert(begin() + iNewIndex - 1, channel);
    m_bIsSorted = false;
  }
  else
  {
    /* append */
    push_back(channel);
  }

  /* update channel numbers */
  ReNumberAndCheck();

  /* update timer channel numbers */
  UpdateTimers();

  if (bSaveInDb)
    bReturn = PersistChannels();
  else
    bReturn = true;

  CLog::Log(LOGNOTICE, "PVRChannelGroupInternal - %s - %s channel '%d' moved to position '%d'",
      __FUNCTION__, (m_bRadio ? "radio" : "tv"), iOldIndex, iNewIndex);

  return bReturn;
}

bool CPVRChannelGroupInternal::HideChannel(CPVRChannel *channel, bool bShowDialog /* = true */)
{
  bool bReturn = false;

  if (!channel)
    return bReturn;

  /* check if there are active timers on this channel if we are hiding it */
  if (!channel->IsHidden() && PVRTimers.ChannelHasTimers(*channel))
  {
    if (bShowDialog)
    {
      CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
      if (!pDialog)
        return bReturn;

      pDialog->SetHeading(19098);
      pDialog->SetLine(0, 19099);
      pDialog->SetLine(1, "");
      pDialog->SetLine(2, 19100);
      pDialog->DoModal();

      if (!pDialog->IsConfirmed())
        return bReturn;
    }

    /* delete the timers */
    PVRTimers.DeleteTimersOnChannel(*channel, true);
  }

  /* check if this channel is currently playing if we are hiding it */
  if (!channel->IsHidden() &&
      (g_PVRManager.IsPlayingTV() || g_PVRManager.IsPlayingRadio()) &&
      (g_PVRManager.GetCurrentPlayingItem()->GetPVRChannelInfoTag() == channel))
  {
    CGUIDialogOK::ShowAndGetInput(19098,19101,0,19102);
    return bReturn;
  }

  /* switch the hidden flag */
  channel->SetHidden(!channel->IsHidden());

  /* update the hidden channel counter */
  if (channel->IsHidden())
    ++m_iHiddenChannels;
  else
    --m_iHiddenChannels;

  /* update the database entry */
  channel->Persist();

  /* move the channel to the end of the list */
  MoveChannel(channel->ChannelNumber(), size());

  bReturn = true;

  return bReturn;
}

int CPVRChannelGroupInternal::LoadFromDb(bool bCompress /* = false */)
{
  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  if (!database || !database->Open())
    return -1;

  int iChannelCount = size();

  if (database->GetChannels(*this, m_bRadio) > 0)
  {
    if (bCompress)
      database->Compress(true);

    Update();
  }

  database->Close();

  return size() - iChannelCount;
}

int CPVRChannelGroupInternal::LoadFromClients(bool bAddToDb /* = true */)
{
  int iReturn = -1;
  int iCurSize = size();

  if (GetFromClients() == -1)
    return iReturn;

  SortByClientChannelNumber();
  ReNumberAndCheck();
  SearchAndSetChannelIcons();

  if (bAddToDb)
    PersistChannels();

  return size() - iCurSize;
}

void CPVRChannelGroupInternal::ReNumberAndCheck(void)
{
  RemoveInvalidChannels();

  int iChannelNumber = 1;
  m_iHiddenChannels = 0;
  for (unsigned int ptr = 0; ptr < size();  ptr++)
  {
    if (at(ptr)->IsHidden())
      m_iHiddenChannels++;
    else
      at(ptr)->SetChannelNumber(iChannelNumber++);
  }
  m_bIsSorted = false;
}

int CPVRChannelGroupInternal::GetFromClients(void)
{
  CLIENTMAP *clients = g_PVRManager.Clients();
  if (!clients)
    return 0;

  int iCurSize = size();

  /* get the channel list from each client */
  CLIENTMAPITR itrClients = clients->begin();
  while (itrClients != clients->end())
  {
    if ((*itrClients).second->ReadyToUse() && (*itrClients).second->GetNumChannels() > 0)
      (*itrClients).second->GetChannelList(*this, m_bRadio);

    itrClients++;
  }

  return size() - iCurSize;
}

bool CPVRChannelGroupInternal::Update(CPVRChannelGroup *channels)
{
  /* the database has already been opened */
  CPVRDatabase *database = g_PVRManager.GetTVDatabase();

  int iSize = size();
  for (int ptr = 0; ptr < iSize; ptr++)
  {
    CPVRChannel *channel = at(ptr);

    /* ignore virtual channels */
    if (channel->IsVirtual())
      continue;

    /* check if this channel is still present */
    CPVRChannel *existingChannel = channels->GetByUniqueID(channel->UniqueID());
    if (existingChannel)
    {
      /* if it's present, update the current tag */
      if (channel->UpdateFromClient(*existingChannel))
      {
        channel->Persist(true);
        CLog::Log(LOGINFO,"PVRChannelGroupInternal - %s - updated %s channel '%s'",
            __FUNCTION__, m_bRadio ? "radio" : "TV", channel->ChannelName().c_str());
      }

      /* remove this tag from the temporary channel list */
      channels->RemoveByUniqueID(channel->UniqueID());
    }
    else
    {
      /* channel is no longer present */
      CLog::Log(LOGINFO,"PVRChannelGroupInternal - %s - removing %s channel '%s'",
          __FUNCTION__, m_bRadio ? "radio" : "TV", channel->ChannelName().c_str());
      database->DeleteChannel(*channel);
      erase(begin() + ptr);
      ptr--;
      iSize--;
    }
  }

  /* the temporary channel list only contains new channels now */
  for (unsigned int ptr = 0; ptr < channels->size(); ptr++)
  {
    CPVRChannel *channel = channels->at(ptr);
    channel->Persist(true);
    push_back(channel);

    CLog::Log(LOGINFO,"PVRChannelGroupInternal - %s - added %s channel '%s'",
        __FUNCTION__, m_bRadio ? "radio" : "TV", channel->ChannelName().c_str());
  }

  /* post the queries generated by the update */
  database->CommitInsertQueries();

  /* recount hidden channels */
  m_iHiddenChannels = 0;
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i)->IsHidden())
      m_iHiddenChannels++;
  }

  m_bIsSorted = false;
  return true;
}
