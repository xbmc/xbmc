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

#include "GUISettings.h"
#include "GUIWindowManager.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "log.h"
#include "MusicInfoTag.h"

#include "PVRChannelGroups.h"
#include "PVRChannelGroup.h"
#include "PVRChannels.h"
#include "TVDatabase.h"
#include "PVRManager.h"
#include "PVREpgInfoTag.h"

using namespace MUSIC_INFO;

CPVRChannels PVRChannelsTV(false);
CPVRChannels PVRChannelsRadio(true);

struct sortByClientChannelNumber
{
  bool operator()(const CPVRChannel &channel1, const CPVRChannel &channel2)
  {
    return channel1.ClientChannelNumber() < channel2.ClientChannelNumber();
  }
};

struct sortByChannelNumber
{
  bool operator()(const CPVRChannel &channel1, const CPVRChannel &channel2)
  {
    return channel1.ChannelNumber() < channel2.ChannelNumber();
  }
};

CPVRChannels::CPVRChannels(bool bRadio)
{
  m_bRadio          = bRadio;
  m_iHiddenChannels = 0;
}

int CPVRChannels::LoadFromDb(bool bCompress /* = false */)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  if (!database || !database->Open())
    return -1;

  int iChannelCount = size();

  database->GetDBChannelList(*this, m_bRadio);

  if (bCompress)
    database->Compress(true);

  Update();
  SortByChannelNumber();
  ReNumberAndCheck();

  database->Close();

  return size() - iChannelCount;
}

int CPVRChannels::LoadFromClients(void)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  CLIENTMAP   *clients  = g_PVRManager.Clients();

  if (!clients || !database || !database->Open())
    return -1;

  /* get the channel list from each client */
  CLIENTMAPITR itrClients = clients->begin();
  while (itrClients != clients->end())
  {
    if ((*itrClients).second->ReadyToUse() && (*itrClients).second->GetNumChannels() > 0)
      (*itrClients).second->GetChannelList(*this, m_bRadio);

    itrClients++;
  }

  SortByClientChannelNumber();
  ReNumberAndCheck();
  SearchAndSetChannelIcons();

  /* add all channels to the database */
  for (unsigned int ptr = 0; ptr < size(); ptr++)
    database->AddDBChannel(at(ptr), false, (ptr==0), (ptr >= size() - 1));

  database->Close();

  clear();
  return LoadFromDb(true);
}

int CPVRChannels::Load(void)
{
  Clear();

  int iChannelCount = LoadFromDb();

  if (iChannelCount <= 0)
  {
    CLog::Log(LOGNOTICE, "%s - No %s channels stored in the database. Reading channels from clients",
        __FUNCTION__, m_bRadio ? "Radio" : "TV");

    return LoadFromClients();
  }

  return iChannelCount;
}

void CPVRChannels::Unload()
{
  Clear();
}

bool CPVRChannels::RemoveByUniqueID(long iUniqueID)
{
  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    if (at(ptr).UniqueID() == iUniqueID)
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
  CTVDatabase *database = g_PVRManager.GetTVDatabase();

  int iSize = size();
  for (int ptr = 0; ptr < iSize; ptr++)
  {
    CPVRChannel *channel = &at(ptr);

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
        database->UpdateDBChannel(*channel);
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
      database->RemoveDBChannel(*channel);
      erase(begin() + ptr);
      ptr--;
      iSize--;
    }
  }

  /* the temporary channel list only contains new channels now */
  for (unsigned int ptr = 0; ptr < channels->size(); ptr++)
  {
    CPVRChannel channel = channels->at(ptr);

    channel.SetChannelID(database->AddDBChannel(channel));
    push_back(channel);

    CLog::Log(LOGINFO,"%s - added %s channel '%s'",
        __FUNCTION__, m_bRadio ? "radio" : "TV", channel.ChannelName().c_str());
  }

  /* recount hidden channels */
  m_iHiddenChannels = 0;
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).IsHidden())
      m_iHiddenChannels++;
  }

  return true;
}

bool CPVRChannels::Update()
{
  bool         bReturn  = false;
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  CLIENTMAP   *clients  = g_PVRManager.Clients();
  CPVRChannels PVRChannels_tmp(m_bRadio);

  database->Open();

  /* get the channel list from all clients */
  CLIENTMAPITR itr = clients->begin();
  while (itr != clients->end())
  {
    if ((*itr).second->ReadyToUse() && (*itr).second->GetNumChannels() > 0)
    {
      (*itr).second->GetChannelList(PVRChannels_tmp, m_bRadio);
    }
    itr++;
  }

  PVRChannels_tmp.SortByClientChannelNumber();
  PVRChannels_tmp.ReNumberAndCheck();
  bReturn = Update(&PVRChannels_tmp);

  database->Close();

  return bReturn;
}

bool CPVRChannels::SetIconIfValid(CPVRChannel *channel, CStdString strIconPath, bool bUpdateDb /* = false */)
{
  if (CFile::Exists(strIconPath))
  {
    channel->SetIconPath(strIconPath);

    if (bUpdateDb)
    {
      CTVDatabase *database = g_PVRManager.GetTVDatabase();
      database->UpdateDBChannel(*channel);
    }

    return true;
  }

  return false;
}

void CPVRChannels::SearchAndSetChannelIcons(bool bUpdateDb /* = false */)
{
  if (g_guiSettings.GetString("pvrmenu.iconpath") == "")
    return;

  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  for (unsigned int ptr = 0; ptr < size(); ptr++)
  {
    CPVRChannel channel = at(ptr);

    /* skip if an icon is already set */
    if (channel.IconPath() != "")
      continue;

    CStdString strBasePath = g_guiSettings.GetString("pvrmenu.iconpath");

    CStdString strIconPath = strBasePath + channel.ClientChannelName();
    CStdString strIconPathLower = strBasePath + channel.ClientChannelName().ToLower();
    CStdString strIconPathUid;
    strIconPathUid.Format("%s/%08d", strBasePath, channel.UniqueID());

    SetIconIfValid(&channel, strIconPath      + ".tbn", bUpdateDb) ||
    SetIconIfValid(&channel, strIconPath      + ".jpg", bUpdateDb) ||
    SetIconIfValid(&channel, strIconPath      + ".png", bUpdateDb) ||
    SetIconIfValid(&channel, strIconPath      + ".tbn", bUpdateDb) ||

    SetIconIfValid(&channel, strIconPathLower + ".tbn", bUpdateDb) ||
    SetIconIfValid(&channel, strIconPathLower + ".jpg", bUpdateDb) ||
    SetIconIfValid(&channel, strIconPathLower + ".png", bUpdateDb) ||
    SetIconIfValid(&channel, strIconPathLower + ".tbn", bUpdateDb) ||

    SetIconIfValid(&channel, strIconPathUid   + ".tbn", bUpdateDb) ||
    SetIconIfValid(&channel, strIconPathUid   + ".jpg", bUpdateDb) ||
    SetIconIfValid(&channel, strIconPathUid   + ".png", bUpdateDb) ||
    SetIconIfValid(&channel, strIconPathUid   + ".tbn", bUpdateDb);

    /* TODO: start channel icon scraper here if nothing was found */
  }

  database->Close();
}

void CPVRChannels::SortByClientChannelNumber(void)
{
  sort(begin(), end(), sortByClientChannelNumber());
}

void CPVRChannels::SortByChannelNumber(void)
{
  sort(begin(), end(), sortByChannelNumber());
}

void CPVRChannels::RemoveInvalidChannels(void)
{
  for (unsigned int ptr = 0; ptr < size(); ptr--)
  {
    if (at(ptr).IsVirtual())
      continue;

    if (at(ptr).ClientChannelNumber() <= 0)
    {
      CLog::Log(LOGERROR, "%s - removing invalid channel '%s' from client '%i': no valid channel number",
          __FUNCTION__, at(ptr).ChannelName().c_str(), at(ptr).ClientID());
      erase(begin() + ptr);
      ptr--;
      continue;
    }

    if (at(ptr).UniqueID() <= 0)
    {
      CLog::Log(LOGERROR, "%s - removing invalid channel '%s' from client '%i': no valid unique ID",
          __FUNCTION__, at(ptr).ChannelName().c_str(), at(ptr).ClientID());
      erase(begin() + ptr);
      ptr--;
      continue;
    }
  }
}

void CPVRChannels::ReNumberAndCheck(void)
{
  RemoveInvalidChannels();

  int iChannelNumber = 1;
  for (unsigned int ptr = 0; ptr < size();  ptr++)
  {
    if (at(ptr).IsHidden())
      m_iHiddenChannels++;
    else
      at(ptr).SetChannelNumber(iChannelNumber++);
  }
}

void CPVRChannels::MoveChannel(unsigned int iOldIndex, unsigned int iNewIndex)
{
  if (iNewIndex == iOldIndex || iNewIndex == 0)
    return;

  CTVDatabase *database = g_PVRManager.GetTVDatabase();
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
    CPVRChannel channel = at(ptr);

    if (channel.ChannelNumber() != (int) ptr + 1)
    {
      channel.SetChannelNumber(ptr + 1);
      database->UpdateDBChannel(channel);
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
}

////////////////////////////////////////////////////////

int CPVRChannels::GetChannels(CFileItemList* results, int group_id)
{
  int cnt = 0;

  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).IsHidden())
      continue;

    if ((group_id != -1) && (at(i).GroupID() != group_id))
      continue;

    CFileItemPtr channel(new CFileItem(at(i)));

    results->Add(channel);
    cnt++;
  }
  return cnt;
}

int CPVRChannels::GetHiddenChannels(CFileItemList* results)
{
  int cnt = 0;

  for (unsigned int i = 0; i < size(); i++)
  {
    if (!at(i).IsHidden())
      continue;

    CFileItemPtr channel(new CFileItem(at(i)));
    results->Add(channel);
    cnt++;
  }
  return cnt;
}

void CPVRChannels::HideChannel(unsigned int number)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();

  for (unsigned int i = 0; i < PVRTimers.size(); i++)
  {
    if ((PVRTimers[i].ChannelNumber() == (int) number) && (PVRTimers[i].IsRadio() == m_bRadio))
    {
      CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);
      if (!pDialog)
        return;

      pDialog->SetHeading(19098);
      pDialog->SetLine(0, 19099);
      pDialog->SetLine(1, "");
      pDialog->SetLine(2, 19100);
      pDialog->DoModal();

      if (!pDialog->IsConfirmed())
        return;

      PVRTimers.DeleteTimer(PVRTimers[i], true);
    }
  }

  if ((g_PVRManager.IsPlayingTV() || g_PVRManager.IsPlayingRadio()) && (g_PVRManager.GetCurrentPlayingItem()->GetPVRChannelInfoTag()->ChannelNumber() == (int) number))
  {
    CGUIDialogOK::ShowAndGetInput(19098,19101,0,19102);
    return;
  }

  if (at(number-1).IsHidden())
  {
    at(number-1).SetHidden(false);
    database->Open();
    database->UpdateDBChannel(at(number-1));
    m_iHiddenChannels = database->GetNumHiddenChannels();
    database->Close();
  }
  else
  {
    at(number-1).SetHidden(true);
    ((CPVREpg *) at(number-1).GetEPG())->Clear();
    database->Open();
    database->UpdateDBChannel(at(number-1));
    m_iHiddenChannels = database->GetNumHiddenChannels();
    database->Close();
    MoveChannel(number, size());
  }
}

CPVRChannel *CPVRChannels::GetByNumber(int Number)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).ChannelNumber() == Number)
      return &at(i);
  }
  return NULL;
}

CPVRChannel *CPVRChannels::GetByClient(int Number, int ClientID)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).ClientChannelNumber() == Number && at(i).ClientID() == ClientID)
      return &at(i);
  }
  return NULL;
}

CPVRChannel *CPVRChannels::GetByIndex(unsigned int iIndex)
{
  return iIndex < size() ?
    &at(iIndex) :
    NULL;
}

CPVRChannel *CPVRChannels::GetByChannelID(long ChannelID)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).ChannelID() == ChannelID)
      return &at(i);
  }
  return NULL;
}

CPVRChannel *CPVRChannels::GetByUniqueID(long UniqueID)
{
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).UniqueID() != 0)
    {
      if (at(i).UniqueID() == UniqueID)
        return &at(i);
    }
    else
    {
      if (at(i).ChannelID() == UniqueID)
        return &at(i);
    }
  }
  return NULL;
}

CStdString CPVRChannels::GetNameForChannel(unsigned int Number)
{
  if ((Number <= size()+1) && (Number > 0))
  {
    if (at(Number-1).ChannelName() != NULL)
      return at(Number-1).ChannelName();
    else
      return g_localizeStrings.Get(13205);
  }
  return "";
}

CStdString CPVRChannels::GetChannelIcon(unsigned int Number)
{
  if (Number > 0 && Number <= size()+1)
    return "";

  return at(Number-1).IconPath();
}

void CPVRChannels::SetChannelIcon(unsigned int Number, CStdString Icon)
{
  if (Number > size()+1)
    return;

  if (at(Number-1).IconPath() != Icon)
  {
    CTVDatabase *database = g_PVRManager.GetTVDatabase();
    database->Open();
    at(Number-1).SetIconPath(Icon);
    database->UpdateDBChannel(at(Number-1));
    database->Close();
  }
}

void CPVRChannels::Clear()
{
  /* Clear all current present Channels inside list */
  clear();
}

int CPVRChannels::GetNumChannelsFromAll()
{
  return PVRChannelsTV.GetNumChannels()+PVRChannelsRadio.GetNumChannels();
}

void CPVRChannels::SearchMissingChannelIcons()
{
  CLog::Log(LOGINFO,"PVR: Manual Channel Icon search started...");
  PVRChannelsTV.SearchAndSetChannelIcons(true);
  PVRChannelsRadio.SearchAndSetChannelIcons(true);
  /// TODO: Add Process dialog here
  CGUIDialogOK::ShowAndGetInput(19103,0,20177,0);
}

CPVRChannel *CPVRChannels::GetByClientFromAll(int Number, int ClientID)
{
  CPVRChannel *channel;

  channel = PVRChannelsTV.GetByClient(Number, ClientID);
  if (channel != NULL)
    return channel;

  channel = PVRChannelsRadio.GetByClient(Number, ClientID);
  if (channel != NULL)
    return channel;

  return NULL;
}

CPVRChannel *CPVRChannels::GetByChannelIDFromAll(long ChannelID)
{
  CPVRChannel *channel;

  channel = PVRChannelsTV.GetByChannelID(ChannelID);
  if (channel != NULL)
    return channel;

  channel = PVRChannelsRadio.GetByChannelID(ChannelID);
  if (channel != NULL)
    return channel;

  return NULL;
}

CPVRChannel *CPVRChannels::GetByUniqueIDFromAll(long UniqueID)
{
  CPVRChannel *channel;

  channel = PVRChannelsTV.GetByUniqueID(UniqueID);
  if (channel != NULL)
    return channel;

  channel = PVRChannelsRadio.GetByUniqueID(UniqueID);
  if (channel != NULL)
    return channel;

  return NULL;
}

bool CPVRChannels::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString base(strPath);
  CUtil::RemoveSlashAtEnd(base);

  CURL url(strPath);
  CStdString fileName = url.GetFileName();
  CUtil::RemoveSlashAtEnd(fileName);

  if (fileName == "channels")
  {
    CFileItemPtr item;

    item.reset(new CFileItem(base + "/tv/", true));
    item->SetLabel(g_localizeStrings.Get(19020));
    item->SetLabelPreformated(true);
    items.Add(item);

    item.reset(new CFileItem(base + "/radio/", true));
    item->SetLabel(g_localizeStrings.Get(19021));
    item->SetLabelPreformated(true);
    items.Add(item);

    return true;
  }
  else if (fileName == "channels/tv")
  {
    CFileItemPtr item;

    item.reset(new CFileItem(base + "/all/", true));
    item->SetLabel(g_localizeStrings.Get(593));
    item->SetLabelPreformated(true);
    items.Add(item);

    if (PVRChannelsTV.GetNumHiddenChannels() > 0)
    {
      item.reset(new CFileItem(base + "/.hidden/", true));
      item->SetLabel(g_localizeStrings.Get(19022));
      item->SetLabelPreformated(true);
      items.Add(item);
    }

    for (unsigned int i = 0; i < PVRChannelGroupsTV.size(); i++)
    {
      base += "/" + PVRChannelGroupsTV[i].GroupName() + "/";
      item.reset(new CFileItem(base, true));
      item->SetLabel(PVRChannelGroupsTV[i].GroupName());
      item->SetLabelPreformated(true);
      items.Add(item);
    }

    return true;
  }
  else if (fileName == "channels/radio")
  {
    CFileItemPtr item;

    item.reset(new CFileItem(base + "/all/", true));
    item->SetLabel(g_localizeStrings.Get(593));
    item->SetLabelPreformated(true);
    items.Add(item);

    if (PVRChannelsTV.GetNumHiddenChannels() > 0)
    {
      item.reset(new CFileItem(base + "/.hidden/", true));
      item->SetLabel(g_localizeStrings.Get(19022));
      item->SetLabelPreformated(true);
      items.Add(item);
    }

    for (unsigned int i = 0; i < PVRChannelGroupsRadio.size(); i++)
    {
      base += "/" + PVRChannelGroupsRadio[i].GroupName() + "/";
      item.reset(new CFileItem(base, true));
      item->SetLabel(PVRChannelGroupsRadio[i].GroupName());
      item->SetLabelPreformated(true);
      items.Add(item);
    }

    return true;
  }
  else if (fileName.Left(12) == "channels/tv/")
  {
    if (fileName.substr(12) == ".hidden")
    {
      for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
      {
        if (!PVRChannelsTV[i].IsHidden())
          continue;

        CFileItemPtr channel(new CFileItem(PVRChannelsTV[i]));
        items.Add(channel);
      }
    }
    else
    {
      int groupID = PVRChannelGroupsTV.GetGroupId(fileName.substr(12));

      for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
      {
        if (PVRChannelsTV[i].IsHidden())
          continue;

        if ((groupID != -1) && (PVRChannelsTV[i].GroupID() != groupID))
          continue;

        CFileItemPtr channel(new CFileItem(PVRChannelsTV[i]));
        items.Add(channel);
      }
    }
    return true;
  }
  else if (fileName.Left(15) == "channels/radio/")
  {
    if (fileName.substr(15) == ".hidden")
    {
      for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
      {
        if (!PVRChannelsRadio[i].IsHidden())
          continue;

        CFileItemPtr channel(new CFileItem(PVRChannelsRadio[i]));
        items.Add(channel);
      }
    }
    else
    {
      int groupID = PVRChannelGroupsRadio.GetGroupId(fileName.substr(15));

      for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
      {
        if (PVRChannelsRadio[i].IsHidden())
          continue;

        if ((groupID != -1) && (PVRChannelsRadio[i].GroupID() != groupID))
          continue;

        CFileItemPtr channel(new CFileItem(PVRChannelsRadio[i]));
        CMusicInfoTag* musictag = channel->GetMusicInfoTag();
        if (musictag)
        {
          const CPVRChannel *channel = &PVRChannelsRadio[i];
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
        items.Add(channel);
      }
    }
    return true;
  }

  return false;
}

CPVRChannel *CPVRChannels::GetByPath(CStdString &path)
{
  CURL url(path);
  CStdString fileName = url.GetFileName();
  CUtil::RemoveSlashAtEnd(fileName);

  if (fileName.Left(16) == "channels/tv/all/")
  {
    fileName.erase(0,16);
    int channelNr = atoi(fileName.c_str());

    for (unsigned int i = 0; i < PVRChannelsTV.size(); i++)
    {
      if (PVRChannelsTV[i].ChannelNumber() == channelNr)
        return &PVRChannelsTV[i];
    }
  }
  else if (fileName.Left(19) == "channels/radio/all/")
  {
    fileName.erase(0,19);
    int channelNr = atoi(fileName.c_str());

    for (unsigned int i = 0; i < PVRChannelsRadio.size(); i++)
    {
      if (PVRChannelsRadio[i].ChannelNumber() == channelNr)
        return &PVRChannelsRadio[i];
    }
  }

  return NULL;
}
