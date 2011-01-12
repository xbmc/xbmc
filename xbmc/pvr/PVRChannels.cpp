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

/**
 * TODO:
 * - move some logic to channel groups. now the channel group is mostly used as a property of channel
 * - treat hidden channels as a channel group and remove the exceptions that we are making for them now
 * - use Observervable here, so we can use event driven operations later
 */

#include "GUISettings.h"
#include "GUIWindowManager.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "MusicInfoTag.h"
#include "log.h"

#include "PVRChannelGroups.h"
#include "PVRChannelGroup.h"
#include "PVRChannelsContainer.h"
#include "PVRDatabase.h"
#include "PVRManager.h"
#include "PVREpgInfoTag.h"

using namespace MUSIC_INFO;

CPVRChannels::CPVRChannels(bool bRadio)
{
  m_bRadio          = bRadio;
  m_iHiddenChannels = 0;
  m_bIsSorted       = false;
}

CPVRChannels::~CPVRChannels(void)
{
  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    delete at(iChannelPtr);
  }
  erase(begin(), end());
}

int CPVRChannels::Load(void)
{
  /* make sure this container is empty before loading */
  Unload();

  int iChannelCount = LoadFromDb();

  /* try to get the channels from clients if there are none in the database */
  if (iChannelCount <= 0)
  {
    CLog::Log(LOGNOTICE, "%s - No %s channels stored in the database. Reading channels from clients",
        __FUNCTION__, m_bRadio ? "Radio" : "TV");

    iChannelCount = LoadFromClients();
  }

  return iChannelCount;
}

void CPVRChannels::Unload()
{
  clear();
}

bool CPVRChannels::Update()
{
  bool         bReturn  = false;
  CPVRDatabase *database = g_PVRManager.GetTVDatabase();

  if (database && database->Open())
  {
    CPVRChannels PVRChannels_tmp(m_bRadio);

    PVRChannels_tmp.LoadFromClients(false);
    bReturn = Update(&PVRChannels_tmp);

    database->Close();
  }

  return bReturn;
}

bool CPVRChannels::Update(CPVRChannel *channel)
{
  // TODO notify observers
  push_back(channel);
  m_bIsSorted = false;
  return true;
}

void CPVRChannels::MoveChannel(unsigned int iOldIndex, unsigned int iNewIndex)
{
  if (iNewIndex == iOldIndex || iNewIndex == 0)
    return;

  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  CPVRChannels tempChannels(m_bRadio);

  /* move the channel */
  tempChannels.push_back(at(iOldIndex - 1));
  erase(begin() + iOldIndex - 1);
  if (iNewIndex < size())
    insert(begin() + iNewIndex - 1, tempChannels[0]);
  else
    push_back(tempChannels[0]);

  /* update the channel numbers */
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    CPVRChannel *channel = at(ptr);

    if (channel->ChannelNumber() != (int) ptr + 1)
    {
      channel->SetChannelNumber(ptr + 1, true);
    }
  }

  CLog::Log(LOGNOTICE, "%s - %s channel '%d' moved to '%d'",
      __FUNCTION__, (m_bRadio ? "radio" : "tv"), iOldIndex, iNewIndex);

  database->Close();

  /* update the timers with the new channel numbers */
  for (unsigned int ptr = 0; ptr < PVRTimers.size(); ptr++)
  {
    CPVRTimerInfoTag timer = PVRTimers[ptr];
    CPVRChannel *tag = GetByClient(timer.Number(), timer.ClientID());
    if (tag)
      timer.SetNumber(tag->ChannelNumber());
  }

  m_bIsSorted = false;
}

bool CPVRChannels::HideChannel(CPVRChannel *channel, bool bShowDialog /* = true */)
{
  if (!channel)
    return false;

  /* check if there are active timers on this channel if we are hiding it */
  if (!channel->IsHidden() && PVRTimers.ChannelHasTimers(*channel))
  {
    if (bShowDialog)
    {
      CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
      if (!pDialog)
        return false;

      pDialog->SetHeading(19098);
      pDialog->SetLine(0, 19099);
      pDialog->SetLine(1, "");
      pDialog->SetLine(2, 19100);
      pDialog->DoModal();

      if (!pDialog->IsConfirmed())
        return false;
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
    return false;
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

  return true;
}

void CPVRChannels::SearchAndSetChannelIcons(bool bUpdateDb /* = false */)
{
  if (g_guiSettings.GetString("pvrmenu.iconpath") == "")
    return;

  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    CPVRChannel *channel = at(ptr);

    /* skip if an icon is already set */
    if (channel->IconPath() != "")
      continue;

    CStdString strBasePath = g_guiSettings.GetString("pvrmenu.iconpath");

    CStdString strIconPath = strBasePath + channel->ClientChannelName();
    CStdString strIconPathLower = strBasePath + channel->ClientChannelName().ToLower();
    CStdString strIconPathUid;
    strIconPathUid.Format("%s/%08d", strBasePath, channel->UniqueID());

    channel->SetIconPath(strIconPath      + ".tbn", bUpdateDb) ||
    channel->SetIconPath(strIconPath      + ".jpg", bUpdateDb) ||
    channel->SetIconPath(strIconPath      + ".png", bUpdateDb) ||

    channel->SetIconPath(strIconPathLower + ".tbn", bUpdateDb) ||
    channel->SetIconPath(strIconPathLower + ".jpg", bUpdateDb) ||
    channel->SetIconPath(strIconPathLower + ".png", bUpdateDb) ||

    channel->SetIconPath(strIconPathUid   + ".tbn", bUpdateDb) ||
    channel->SetIconPath(strIconPathUid   + ".jpg", bUpdateDb) ||
    channel->SetIconPath(strIconPathUid   + ".png", bUpdateDb);

    /* TODO: start channel icon scraper here if nothing was found */
  }

  database->Close();
}

/********** sort methods **********/

struct sortByClientChannelNumber
{
  bool operator()(CPVRChannel *channel1, CPVRChannel *channel2)
  {
    return channel1->ClientChannelNumber() < channel2->ClientChannelNumber();
  }
};

struct sortByChannelNumber
{
  bool operator()(CPVRChannel *channel1, CPVRChannel *channel2)
  {
    return channel1->ChannelNumber() < channel2->ChannelNumber();
  }
};

void CPVRChannels::SortByClientChannelNumber(void)
{
  sort(begin(), end(), sortByClientChannelNumber());
  m_bIsSorted = false;
}

void CPVRChannels::SortByChannelNumber(void)
{
  if (!m_bIsSorted)
  {
    sort(begin(), end(), sortByChannelNumber());
    m_bIsSorted = true;
  }
}

/********** getters **********/

CPVRChannel *CPVRChannels::GetByClient(int iClientChannelNumber, int iClientID)
{
  CPVRChannel *channel = NULL;

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    CPVRChannel *checkChannel = at(ptr);
    if (checkChannel->ClientChannelNumber() == iClientChannelNumber &&
        checkChannel->ClientID() == iClientID)
    {
      channel = checkChannel;
      break;
    }
  }

  return channel;
}

CPVRChannel *CPVRChannels::GetByChannelID(long iChannelID)
{
  CPVRChannel *channel = NULL;

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    if (at(ptr)->ChannelID() == iChannelID)
    {
      channel = at(ptr);
      break;
    }
  }

  return channel;
}

CPVRChannel *CPVRChannels::GetByUniqueID(int iUniqueID)
{
  CPVRChannel *channel = NULL;

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    if (at(ptr)->UniqueID() == iUniqueID)
    {
      channel = at(ptr);
      break;
    }
  }

  return channel;
}

CPVRChannel *CPVRChannels::GetByChannelNumber(int iChannelNumber) // TODO: move to channelgroup
{
  CPVRChannel *channel = NULL;

  if (iChannelNumber <= (int) size())
  {
    SortByChannelNumber();
    channel = at(iChannelNumber - 1);
  }

  return channel;
}

CPVRChannel *CPVRChannels::GetByChannelNumberUp(int iChannelNumber) // TODO: move to channelgroup
{
  int iGetChannel = iChannelNumber + 1;
  if (iGetChannel > (int) size())
    iGetChannel = 1;

  return GetByChannelNumber(iGetChannel - 1);
}

CPVRChannel *CPVRChannels::GetByChannelNumberDown(int iChannelNumber) // TODO: move to channelgroup
{
  int iGetChannel = iChannelNumber - 1;
  if (iGetChannel <= 0)
    iGetChannel = size();

  return GetByChannelNumber(iGetChannel - 1);
}

CPVRChannel *CPVRChannels::GetByIndex(unsigned int iIndex)
{
  return iIndex < size() ?
    at(iIndex) :
    NULL;
}

int CPVRChannels::GetChannels(CFileItemList* results, int iGroupID /* = -1 */, bool bHidden /* = false */)
{
  int iAmount = 0;

  SortByChannelNumber();

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    CPVRChannel *channel = at(ptr);

    if (channel->IsHidden() != bHidden)
      continue;

    if (iGroupID != -1 && channel->GroupID() != iGroupID)
      continue;

    CFileItemPtr channelFile(new CFileItem(*channel));

    if (channel->IsRadio())
    {
      CMusicInfoTag* musictag = channelFile->GetMusicInfoTag();
      if (musictag)
      {
        const CPVREpgInfoTag *epgNow = channel->GetEPGNow();
        musictag->SetURL(channel->Path());
        musictag->SetTitle(epgNow->Title());
        musictag->SetArtist(channel->ChannelName());
        musictag->SetAlbumArtist(channel->ChannelName());
        musictag->SetGenre(epgNow->Genre());
        musictag->SetDuration(epgNow->GetDuration());
        musictag->SetLoaded(true);
        musictag->SetComment("");
        musictag->SetLyrics("");
      }
    }

    results->Add(channelFile);
    iAmount++;
  }
  return iAmount;
}

int CPVRChannels::GetHiddenChannels(CFileItemList* results)
{
  return GetChannels(results, -1, true);
}

/********** operations on all channels **********/

void CPVRChannels::SearchMissingChannelIcons()
{
  CLog::Log(LOGINFO,"PVR: Manual Channel Icon search started...");
  ((CPVRChannels *) g_PVRChannels.GetTV())->SearchAndSetChannelIcons(true);
  ((CPVRChannels *) g_PVRChannels.GetRadio())->SearchAndSetChannelIcons(true);
  // TODO: Add Process dialog here
  CGUIDialogOK::ShowAndGetInput(19103,0,20177,0);
}

/********** static getters **********/

CPVRChannel *CPVRChannels::GetByPath(const CStdString &strPath)
{
  CPVRChannels *channels = NULL;
  int iChannelNumber = -1;

  /* get the filename from curl */
  CURL url(strPath);
  CStdString strFileName = url.GetFileName();
  CUtil::RemoveSlashAtEnd(strFileName);

  if (strFileName.Left(16) == "channels/tv/all/")
  {
    strFileName.erase(0,16);
    iChannelNumber = atoi(strFileName.c_str());
    channels = (CPVRChannels *) g_PVRChannels.GetTV();
  }
  else if (strFileName.Left(19) == "channels/radio/all/")
  {
    strFileName.erase(0,19);
    iChannelNumber = atoi(strFileName.c_str());
    channels = (CPVRChannels *) g_PVRChannels.GetRadio();
  }

  return channels ? channels->GetByChannelNumber(iChannelNumber) : NULL;
}

bool CPVRChannels::GetDirectory(const CStdString& strPath, CFileItemList &results)
{
  CStdString strBase(strPath);
  CUtil::RemoveSlashAtEnd(strBase);

  /* get the filename from curl */
  CURL url(strPath);
  CStdString fileName = url.GetFileName();
  CUtil::RemoveSlashAtEnd(fileName);

  if (fileName == "channels")
  {
    CFileItemPtr item;

    /* all tv channels */
    item.reset(new CFileItem(strBase + "/tv/", true));
    item->SetLabel(g_localizeStrings.Get(19020));
    item->SetLabelPreformated(true);
    results.Add(item);

    /* all radio channels */
    item.reset(new CFileItem(strBase + "/radio/", true));
    item->SetLabel(g_localizeStrings.Get(19021));
    item->SetLabelPreformated(true);
    results.Add(item);

    return true;
  }
  else if (fileName == "channels/tv")
  {
    return GetGroupsDirectory(strBase, &results, false);
  }
  else if (fileName == "channels/radio")
  {
    return GetGroupsDirectory(strBase, &results, true);
  }
  else if (fileName.Left(12) == "channels/tv/")
  {
    if (fileName.substr(12) == ".hidden")
    {
      ((CPVRChannels *) g_PVRChannels.GetTV())->GetChannels(&results, -1, true);
    }
    else
    {
      int iGroupID = PVRChannelGroupsTV.GetGroupId(fileName.substr(12));
      ((CPVRChannels *) g_PVRChannels.GetTV())->GetChannels(&results, iGroupID, false);
    }
    return true;
  }
  else if (fileName.Left(15) == "channels/radio/")
  {
    if (fileName.substr(15) == ".hidden")
    {
      ((CPVRChannels *) g_PVRChannels.GetRadio())->GetChannels(&results, -1, true);
    }
    else
    {
      int iGroupID = PVRChannelGroupsRadio.GetGroupId(fileName.substr(15));
      ((CPVRChannels *) g_PVRChannels.GetRadio())->GetChannels(&results, iGroupID, false);
    }
    return true;
  }

  return false;
}

int CPVRChannels::GetNumChannelsFromAll()
{
  return g_PVRChannels.GetTV()->GetNumChannels() + g_PVRChannels.GetRadio()->GetNumChannels();
}


CPVRChannel *CPVRChannels::GetByClientFromAll(int iClientChannelNumber, int iClientID)
{
  CPVRChannel *channel;

  channel = ((CPVRChannels *) g_PVRChannels.GetTV())->GetByClient(iClientChannelNumber, iClientID);
  if (channel != NULL)
    return channel;

  channel = ((CPVRChannels *) g_PVRChannels.GetRadio())->GetByClient(iClientChannelNumber, iClientID);
  if (channel != NULL)
    return channel;

  return NULL;
}

CPVRChannel *CPVRChannels::GetByChannelIDFromAll(long iChannelID)
{
  CPVRChannel *channel;

  channel = ((CPVRChannels *) g_PVRChannels.GetTV())->GetByChannelID(iChannelID);
  if (channel != NULL)
    return channel;

  channel = ((CPVRChannels *) g_PVRChannels.GetRadio())->GetByChannelID(iChannelID);
  if (channel != NULL)
    return channel;

  return NULL;
}

CPVRChannel *CPVRChannels::GetByUniqueIDFromAll(int iUniqueID)
{
  CPVRChannel *channel;

  channel = ((CPVRChannels *) g_PVRChannels.GetTV())->GetByUniqueID(iUniqueID);
  if (channel != NULL)
    return channel;

  channel = ((CPVRChannels *) g_PVRChannels.GetRadio())->GetByUniqueID(iUniqueID);
  if (channel != NULL)
    return channel;

  return NULL;
}

/********** private methods **********/

int CPVRChannels::LoadFromDb(bool bCompress /* = false */)
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

int CPVRChannels::LoadFromClients(bool bAddToDb /* = true */)
{
  CPVRDatabase *database = NULL;
  int iCurSize = size();

  if (bAddToDb)
  {
    database = g_PVRManager.GetTVDatabase();

    if (!database || !database->Open())
      return -1;
  }

  if (GetFromClients() == -1)
    return -1;

  SortByClientChannelNumber();
  ReNumberAndCheck();
  SearchAndSetChannelIcons();

  if (bAddToDb)
  {
    /* add all channels to the database */
    for (unsigned int ptr = 0; ptr < size(); ptr++)
      database->UpdateChannel(*at(ptr));

    database->Close();

    clear();

    return LoadFromDb(true);
  }

  return size() - iCurSize;
}

int CPVRChannels::GetFromClients(void)
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

bool CPVRChannels::RemoveByUniqueID(long iUniqueID)
{
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    if (at(ptr)->UniqueID() == iUniqueID)
    {
      erase(begin() + ptr);
      return true;
    }
  }

  return false;
}

bool CPVRChannels::Update(CPVRChannels *channels)
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
        CLog::Log(LOGINFO,"%s - updated %s channel '%s'",
            __FUNCTION__, m_bRadio ? "radio" : "TV", channel->ChannelName().c_str());
      }

      /* remove this tag from the temporary channel list */
      channels->RemoveByUniqueID(channel->UniqueID());
    }
    else
    {
      /* channel is no longer present */
      CLog::Log(LOGINFO,"%s - removing %s channel '%s'",
          __FUNCTION__, m_bRadio ? "radio" : "TV", channel->ChannelName().c_str());
      database->RemoveChannel(*channel);
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

    CLog::Log(LOGINFO,"%s - added %s channel '%s'",
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

void CPVRChannels::RemoveInvalidChannels(void)
{
  for (unsigned int ptr = 0; ptr < size(); ptr--)
  {
    if (at(ptr)->IsVirtual())
      continue;

    if (at(ptr)->ClientChannelNumber() <= 0)
    {
      CLog::Log(LOGERROR, "%s - removing invalid channel '%s' from client '%i': no valid channel number",
          __FUNCTION__, at(ptr)->ChannelName().c_str(), at(ptr)->ClientID());
      erase(begin() + ptr);
      ptr--;
      continue;
    }

    if (at(ptr)->UniqueID() <= 0)
    {
      CLog::Log(LOGERROR, "%s - removing invalid channel '%s' from client '%i': no valid unique ID",
          __FUNCTION__, at(ptr)->ChannelName().c_str(), at(ptr)->ClientID());
      erase(begin() + ptr);
      ptr--;
      continue;
    }
  }
}

bool CPVRChannels::GetGroupsDirectory(const CStdString &strBase, CFileItemList *results, bool bRadio)
{
  const CPVRChannels * channels    = g_PVRChannels.Get(bRadio);
  CPVRChannelGroups *channelGroups = bRadio ? &PVRChannelGroupsRadio : &PVRChannelGroupsTV;
  CFileItemPtr item;

  item.reset(new CFileItem(strBase + "/all/", true));
  item->SetLabel(g_localizeStrings.Get(593));
  item->SetLabelPreformated(true);
  results->Add(item);

  /* container has hidden channels */
  if (channels->GetNumHiddenChannels() > 0)
  {
    item.reset(new CFileItem(strBase + "/.hidden/", true));
    item->SetLabel(g_localizeStrings.Get(19022));
    item->SetLabelPreformated(true);
    results->Add(item);
  }

  /* add all groups */
  for (unsigned int ptr = 0; ptr < channelGroups->size(); ptr++)
  {
    CPVRChannelGroup group = channelGroups->at(ptr);
    CStdString strGroup = strBase + "/" + group.GroupName() + "/";
    item.reset(new CFileItem(strGroup, true));
    item->SetLabel(group.GroupName());
    item->SetLabelPreformated(true);
    results->Add(item);
  }

  return true;
}

void CPVRChannels::ReNumberAndCheck(void)
{
  RemoveInvalidChannels();

  int iChannelNumber = 1;
  for (unsigned int ptr = 0; ptr < size();  ptr++)
  {
    if (at(ptr)->IsHidden())
      m_iHiddenChannels++;
    else
      at(ptr)->SetChannelNumber(iChannelNumber++);
  }
  m_bIsSorted = false;
}
