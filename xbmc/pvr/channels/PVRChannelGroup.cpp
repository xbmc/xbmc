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
 * - use Observervable here, so we can use event driven operations later
 */

#include "settings/GUISettings.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "music/tags/MusicInfoTag.h"
#include "utils/log.h"

#include "PVRChannelGroupsContainer.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/epg/PVREpgInfoTag.h"

using namespace MUSIC_INFO;

CPVRChannelGroup::CPVRChannelGroup(bool bRadio, unsigned int iGroupId, const CStdString &strGroupName, int iSortOrder)
{
  m_bRadio       = bRadio;
  m_bIsSorted    = false;
  m_iGroupId     = iGroupId;
  m_strGroupName = strGroupName;
  m_iSortOrder   = iSortOrder;
}

CPVRChannelGroup::CPVRChannelGroup(bool bRadio)
{
  m_bRadio       = bRadio;
  m_bIsSorted    = false;
  m_iGroupId     = -1;
  m_strGroupName.clear();
  m_iSortOrder   = -1;
}

CPVRChannelGroup::~CPVRChannelGroup(void)
{
  Unload();
}

int CPVRChannelGroup::Load(void)
{
  /* make sure this container is empty before loading */
  Unload();

  int iReturn = 0;

  CPVRDatabase *database = CPVRManager::Get()->GetTVDatabase();
  database->Open();

  iReturn = database->GetChannelsInGroup(this);

  database->Close();

  return iReturn;
}

void CPVRChannelGroup::Unload()
{
  clear();
}

bool CPVRChannelGroup::Update()
{
  return (Load() >= 0);
}

bool CPVRChannelGroup::Update(const CPVRChannelGroup &group)
{
  m_strGroupName = group.GroupName();
  m_iSortOrder   = group.SortOrder();

  return true;
}

void CPVRChannelGroup::MoveChannel(unsigned int iOldIndex, unsigned int iNewIndex)
{
  // TODO non-system groups. need a mapping table first
}

bool CPVRChannelGroup::HideChannel(CPVRChannel *channel, bool bShowDialog /* = true */)
{
  return RemoveFromGroup(channel);
}

void CPVRChannelGroup::SearchAndSetChannelIcons(bool bUpdateDb /* = false */)
{
  if (g_guiSettings.GetString("pvrmenu.iconpath") == "")
    return;

  CPVRDatabase *database = CPVRManager::Get()->GetTVDatabase();
  database->Open();

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    CPVRChannel *channel = at(ptr);

    /* skip if an icon is already set */
    if (channel->IconPath() != "")
      continue;

    CStdString strBasePath = g_guiSettings.GetString("pvrmenu.iconpath");
    CStdString strChannelName = channel->ClientChannelName();

    CStdString strIconPath = strBasePath + channel->ClientChannelName();
    CStdString strIconPathLower = strBasePath + strChannelName.ToLower();
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

void CPVRChannelGroup::SortByClientChannelNumber(void)
{
  sort(begin(), end(), sortByClientChannelNumber());
  m_bIsSorted = false;
}

void CPVRChannelGroup::SortByChannelNumber(void)
{
  if (!m_bIsSorted)
  {
    sort(begin(), end(), sortByChannelNumber());
    m_bIsSorted = true;
  }
}

/********** getters **********/

const CPVRChannel *CPVRChannelGroup::GetByClient(int iClientChannelNumber, int iClientID) const
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

const CPVRChannel *CPVRChannelGroup::GetByChannelID(int iChannelID) const
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

const CPVRChannel *CPVRChannelGroup::GetByUniqueID(int iUniqueID) const
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

const CPVRChannel *CPVRChannelGroup::GetByChannelNumber(int iChannelNumber) const
{
  CPVRChannel *channel = NULL;

  if (iChannelNumber <= (int) size())
  {
    ((CPVRChannelGroup *) this)->SortByChannelNumber(); // TODO support group channel numbers
    channel = at(iChannelNumber - 1);
  }

  return channel;
}

const CPVRChannel *CPVRChannelGroup::GetByChannelUp(const CPVRChannel *channel) const
{
  int iGetChannel = channel->ChannelNumber() + 1; // XXX
  if (iGetChannel > (int) size())
    iGetChannel = 1;

  return GetByChannelNumber(iGetChannel - 1);
}

const CPVRChannel *CPVRChannelGroup::GetByChannelDown(const CPVRChannel *channel) const
{
  int iGetChannel = channel->ChannelNumber() - 1; // XXX
  if (iGetChannel <= 0)
    iGetChannel = size();

  return GetByChannelNumber(iGetChannel - 1);
}

const CPVRChannel *CPVRChannelGroup::GetByIndex(unsigned int iIndex) const
{
  return iIndex < size() ?
    at(iIndex) :
    NULL;
}

// TODO support groups
int CPVRChannelGroup::GetChannels(CFileItemList* results, int iGroupID /* = -1 */, bool bHidden /* = false */) const
{
  int iAmount = 0;

  ((CPVRChannelGroup *) this)->SortByChannelNumber();

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

int CPVRChannelGroup::GetHiddenChannels(CFileItemList* results) const
{
  return GetChannels(results, -1, true);
}

/********** operations on all channels **********/

void CPVRChannelGroup::SearchMissingChannelIcons()
{
  CLog::Log(LOGINFO, "PVRChannelGroup - %s - Manual Channel Icon search started...",
      __FUNCTION__);
  ((CPVRChannelGroup *) CPVRManager::GetChannelGroups()->GetGroupAllTV())->SearchAndSetChannelIcons(true);
  ((CPVRChannelGroup *) CPVRManager::GetChannelGroups()->GetGroupAllRadio())->SearchAndSetChannelIcons(true);
  // TODO: Add Process dialog here
  CGUIDialogOK::ShowAndGetInput(19103,0,20177,0);
}

/********** static getters **********/

const CPVRChannel *CPVRChannelGroup::GetByPath(const CStdString &strPath)
{
  const CPVRChannelGroup *channels = NULL;
  int iChannelNumber = -1;

  /* get the filename from curl */
  CURL url(strPath);
  CStdString strFileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(strFileName);

  if (strFileName.Left(16) == "channels/tv/all/")
  {
    strFileName.erase(0,16);
    iChannelNumber = atoi(strFileName.c_str());
    channels = CPVRManager::GetChannelGroups()->GetGroupAllTV();
  }
  else if (strFileName.Left(19) == "channels/radio/all/")
  {
    strFileName.erase(0,19);
    iChannelNumber = atoi(strFileName.c_str());
    channels = CPVRManager::GetChannelGroups()->GetGroupAllRadio();
  }

  return channels ? channels->GetByChannelNumber(iChannelNumber) : NULL;
}

bool CPVRChannelGroup::GetDirectory(const CStdString& strPath, CFileItemList &results)
{
  CStdString strBase(strPath);
  URIUtils::RemoveSlashAtEnd(strBase);

  /* get the filename from curl */
  CURL url(strPath);
  CStdString fileName = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(fileName);

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
      CPVRManager::GetChannelGroups()->GetGroupAllTV()->GetChannels(&results, -1, true);
    }
    else
    {
      int iGroupID = CPVRManager::GetChannelGroups()->GetTV()->GetGroupId(fileName.substr(12));
      CPVRManager::GetChannelGroups()->GetGroupAllTV()->GetChannels(&results, iGroupID, false);
    }
    return true;
  }
  else if (fileName.Left(15) == "channels/radio/")
  {
    if (fileName.substr(15) == ".hidden")
    {
      CPVRManager::GetChannelGroups()->GetGroupAllRadio()->GetChannels(&results, -1, true);
    }
    else
    {
      int iGroupID = CPVRManager::GetChannelGroups()->GetRadio()->GetGroupId(fileName.substr(15));
      CPVRManager::GetChannelGroups()->GetGroupAllRadio()->GetChannels(&results, iGroupID, false);
    }
    return true;
  }

  return false;
}

int CPVRChannelGroup::GetNumChannelsFromAll()
{
  return CPVRManager::GetChannelGroups()->GetGroupAllTV()->GetNumChannels() + CPVRManager::GetChannelGroups()->GetGroupAllRadio()->GetNumChannels();
}


const CPVRChannel *CPVRChannelGroup::GetByClientFromAll(int iClientChannelNumber, int iClientID)
{
  const CPVRChannel *channel = NULL;

  channel = CPVRManager::GetChannelGroups()->GetGroupAllTV()->GetByClient(iClientChannelNumber, iClientID);

  if (channel == NULL)
    channel = CPVRManager::GetChannelGroups()->GetGroupAllRadio()->GetByClient(iClientChannelNumber, iClientID);

  return channel;
}

const CPVRChannel *CPVRChannelGroup::GetByChannelIDFromAll(int iChannelID)
{
  const CPVRChannel *channel = NULL;

  channel = CPVRManager::GetChannelGroups()->GetGroupAllTV()->GetByChannelID(iChannelID);

  if (channel == NULL)
    channel = CPVRManager::GetChannelGroups()->GetGroupAllRadio()->GetByChannelID(iChannelID);

  return channel;
}

const CPVRChannel *CPVRChannelGroup::GetByUniqueIDFromAll(int iUniqueID)
{
  const CPVRChannel *channel;

  channel = CPVRManager::GetChannelGroups()->GetGroupAllTV()->GetByUniqueID(iUniqueID);

  if (channel == NULL)
    channel = CPVRManager::GetChannelGroups()->GetGroupAllRadio()->GetByUniqueID(iUniqueID);

  return NULL;
}

/********** private methods **********/

int CPVRChannelGroup::LoadFromDb(bool bCompress /* = false */)
{
  // TODO load group members
  return -1;
}

int CPVRChannelGroup::LoadFromClients(bool bAddToDb /* = true */)
{
  // TODO add support to load channel groups from clients
  return -1;
}

int CPVRChannelGroup::GetFromClients(void)
{
  return -1;
}

bool CPVRChannelGroup::RemoveByUniqueID(int iUniqueID)
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

bool CPVRChannelGroup::UpdateGroupEntries(CPVRChannelGroup *channels)
{
  // TODO
  return false;
}

void CPVRChannelGroup::RemoveInvalidChannels(void)
{
  for (unsigned int ptr = 0; ptr < size(); ptr--)
  {
    if (at(ptr)->IsVirtual())
      continue;

    if (at(ptr)->ClientChannelNumber() <= 0)
    {
      CLog::Log(LOGERROR, "PVRChannelGroup - %s - removing invalid channel '%s' from client '%i': no valid channel number",
          __FUNCTION__, at(ptr)->ChannelName().c_str(), at(ptr)->ClientID());
      erase(begin() + ptr);
      ptr--;
      continue;
    }

    if (at(ptr)->UniqueID() <= 0)
    {
      CLog::Log(LOGERROR, "PVRChannelGroup - %s - removing invalid channel '%s' from client '%i': no valid unique ID",
          __FUNCTION__, at(ptr)->ChannelName().c_str(), at(ptr)->ClientID());
      erase(begin() + ptr);
      ptr--;
      continue;
    }
  }
}

bool CPVRChannelGroup::PersistChannels(void)
{
  bool bReturn = false;
  bool bRefreshChannelList = false;
  CPVRDatabase *database = CPVRManager::Get()->GetTVDatabase();

  if (!database->Open())
    return bReturn;

  bReturn = true;
  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    /* if this channel has an invalid ID, reload the list afterwards */
    bRefreshChannelList = at(iChannelPtr)->ChannelID() < 0;

    /* queue a persist query if needed */
    bReturn = at(iChannelPtr)->Persist(true) && bReturn;
  }

  /* commit all queries */
  database->CommitInsertQueries();

  /* refresh the channel list if needed */
  if (bRefreshChannelList)
  {
    CLog::Log(LOGDEBUG, "PVRChannelGroup - %s - reloading the channels list to get channel IDs",
        __FUNCTION__);
    Unload();
    bReturn = LoadFromDb(true) > 0;
  }

  database->Close();

  return bReturn;
}

bool CPVRChannelGroup::GetGroupsDirectory(const CStdString &strBase, CFileItemList *results, bool bRadio)
{
  const CPVRChannelGroup * channels = CPVRManager::GetChannelGroups()->GetGroupAll(bRadio);
  const CPVRChannelGroups *channelGroups  = CPVRManager::GetChannelGroups()->Get(bRadio);
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
    const CPVRChannelGroup group = channelGroups->at(ptr);
    CStdString strGroup = strBase + "/" + group.GroupName() + "/";
    item.reset(new CFileItem(strGroup, true));
    item->SetLabel(group.GroupName());
    item->SetLabelPreformated(true);
    results->Add(item);
  }

  return true;
}

bool CPVRChannelGroup::RemoveFromGroup(const CPVRChannel *channel)
{
  bool bReturn = false;

  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    if (*channel == *at(iChannelPtr))
    {
      // TODO notify observers
      erase(begin() + iChannelPtr);
      bReturn = true;
      break;
    }
  }

  return bReturn;
}

bool CPVRChannelGroup::AddToGroup(CPVRChannel *channel)
{
  bool bReturn = false;

  if (!IsGroupMember(channel))
  {
    // TODO notify observers
    push_back(channel);
    m_bIsSorted = false;
    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannelGroup::IsGroupMember(const CPVRChannel *channel) const
{
  bool bReturn = false;

  for (unsigned int iChannelPtr = 0; iChannelPtr < size(); iChannelPtr++)
  {
    if (*channel == *at(iChannelPtr))
    {
      bReturn = true;
      break;
    }
  }

  return bReturn;
}

const CPVRChannel *CPVRChannelGroup::GetFirstChannel(void) const
{
  CPVRChannel *channel = NULL;

  if (size() > 0)
    channel = at(0);

  return channel;
}

bool CPVRChannelGroup::SetGroupName(const CStdString &strGroupName, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_strGroupName != strGroupName)
  {
    /* update the name */
    m_strGroupName = strGroupName;
//    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannelGroup::Persist(bool bQueueWrite /* = false */)
{
  CPVRDatabase *database = CPVRManager::Get()->GetTVDatabase();
  if (database)
  {
    database->Open();
    database->Persist(*this, bQueueWrite);
    database->Close();

    return true;
  }

  return false;
}
