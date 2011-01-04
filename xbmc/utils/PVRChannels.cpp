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

#include "FileItem.h"
#include "GUISettings.h"
#include "GUIWindowManager.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "LocalizeStrings.h"
#include "utils/log.h"
#include "Util.h"
#include "URL.h"
#include "FileSystem/File.h"
#include "MusicInfoTag.h"

#include "PVRChannelGroups.h"
#include "PVRChannelGroup.h"
#include "PVRChannels.h"
#include "TVDatabase.h"
#include "PVRTimerInfoTag.h"
#include "PVRManager.h"
#include "PVREpgInfoTag.h"

using namespace XFILE;
using namespace MUSIC_INFO;

CPVRChannels PVRChannelsTV;
CPVRChannels PVRChannelsRadio;

CPVRChannels::CPVRChannels(void)
{
  m_bRadio = false;
  m_iHiddenChannels = 0;
}

bool CPVRChannels::Load(bool radio)
{
  m_bRadio = radio;
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  CLIENTMAP   *clients  = g_PVRManager.Clients();

  Clear();

  database->Open();
  if (database->GetDBNumChannels(m_bRadio) > 0)
  {
    database->GetDBChannelList(*this, m_bRadio);
    database->Close();
    Update();
    ReNumberAndCheck();
  }
  else
  {
    CLog::Log(LOGNOTICE, "PVR: TV Database holds no %s channels, reading channels from clients", m_bRadio ? "Radio" : "TV");

    CLIENTMAPITR itr = clients->begin();
    while (itr != clients->end())
    {
      if ((*itr).second->ReadyToUse() && (*itr).second->GetNumChannels() > 0)
      {
        (*itr).second->GetChannelList(*this, m_bRadio);
      }
      itr++;
    }
    ReNumberAndCheck();
    SearchAndSetChannelIcons();
    for (unsigned int i = 0; i < size(); i++)
      database->AddDBChannel(at(i), false, (i==0), (i >= size()-1));

    clear();
    database->GetDBChannelList(*this, m_bRadio);
    database->Compress(true);
    database->Close();
    ReNumberAndCheck();
  }
  return false;
}

void CPVRChannels::Unload()
{
  Clear();
}

bool CPVRChannels::Update()
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  CLIENTMAP   *clients  = g_PVRManager.Clients();
  CPVRChannels PVRChannels_tmp;

  database->Open();

  CLIENTMAPITR itr = clients->begin();
  while (itr != clients->end())
  {
    if ((*itr).second->ReadyToUse() && (*itr).second->GetNumChannels() > 0)
    {
      (*itr).second->GetChannelList(PVRChannels_tmp, m_bRadio);
    }
    itr++;
  }

  PVRChannels_tmp.ReNumberAndCheck();

  /*
   * First whe look for moved channels on backend (other backend number)
   * and delete no more present channels inside database.
   * Problem:
   * If a channel on client is renamed, it is deleted from Database
   * and later added as new channel and loose his Group Information
   */
  for (unsigned int i = 0; i < size(); i++)
  {
    bool found = false;
    bool changed = false;

    if (!at(i).IsVirtual())
    {
      for (unsigned int j = 0; j < PVRChannels_tmp.size(); j++)
      {
        if (at(i).UniqueID() == PVRChannels_tmp[j].UniqueID() &&
            at(i).ClientID() == PVRChannels_tmp[j].ClientID())
        {
          if (at(i).ClientChannelNumber() != PVRChannels_tmp[j].ClientChannelNumber())
          {
            at(i).SetClientNumber(PVRChannels_tmp[j].ClientChannelNumber());
            changed = true;
          }

          if (at(i).ClientChannelName() != PVRChannels_tmp[j].ClientChannelName())
          {
            at(i).SetClientChannelName(PVRChannels_tmp[j].ClientChannelName());
            at(i).SetChannelName(PVRChannels_tmp[j].ClientChannelName());
            changed = true;
          }

          found = true;
          PVRChannels_tmp.erase(PVRChannels_tmp.begin()+j);
          break;
        }
      }

      if (changed)
      {
        database->UpdateDBChannel(at(i));
        CLog::Log(LOGINFO,"PVR: Updated %s channel %s", m_bRadio?"Radio":"TV", at(i).ChannelName().c_str());
      }

      if (!found)
      {
        CLog::Log(LOGINFO,"PVR: Removing %s channel %s (no more present)", m_bRadio?"Radio":"TV", at(i).ChannelName().c_str());
        database->RemoveDBChannel(at(i));
        erase(begin()+i);
        i--;
      }
    }
  }

  /*
   * Now whe add new channels to frontend
   * All entries now present in the temp lists, are new entries
   */
  for (unsigned int i = 0; i < PVRChannels_tmp.size(); i++)
  {
    PVRChannels_tmp[i].SetChannelID(database->AddDBChannel(PVRChannels_tmp[i]));
    push_back(PVRChannels_tmp[i]);
    CLog::Log(LOGINFO,"PVR: Added %s channel %s", m_bRadio?"Radio":"TV", PVRChannels_tmp[i].ChannelName().c_str());
  }

  database->Close();

  m_iHiddenChannels = 0;
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).IsHidden())
      m_iHiddenChannels++;



  }

  return false;
}

void CPVRChannels::SearchAndSetChannelIcons(bool writeDB)
{
  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  for (unsigned int i = 0; i < size(); i++)
  {
    CStdString iconpath;

    /* If the Icon is already set continue with next channel */
    if (at(i).Icon() != "")
      continue;

    if (g_guiSettings.GetString("pvrmenu.iconpath") != "")
    {
      /* Search icon by channel name */
      iconpath = g_guiSettings.GetString("pvrmenu.iconpath") + at(i).ClientChannelName();
      if (CFile::Exists(iconpath + ".tbn"))
      {
        at(i).SetIcon(iconpath + ".tbn");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
      else if (CFile::Exists(iconpath + ".jpg"))
      {
        at(i).SetIcon(iconpath + ".jpg");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
      else if (CFile::Exists(iconpath + ".png"))
      {
        at(i).SetIcon(iconpath + ".png");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }

      /* Search icon by channel name in lower case */
      iconpath = g_guiSettings.GetString("pvrmenu.iconpath") + at(i).ClientChannelName().ToLower();
      if (CFile::Exists(iconpath + ".tbn"))
      {
        at(i).SetIcon(iconpath + ".tbn");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
      else if (CFile::Exists(iconpath + ".jpg"))
      {
        at(i).SetIcon(iconpath + ".jpg");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
      else if (CFile::Exists(iconpath + ".png"))
      {
        at(i).SetIcon(iconpath + ".png");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }

      /* Search Icon by Unique Id */
      iconpath.Format("%s/%08d",g_guiSettings.GetString("pvrmenu.iconpath"), at(i).UniqueID());
      if (CFile::Exists(iconpath + ".tbn"))
      {
        at(i).SetIcon(iconpath + ".tbn");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
      else if (CFile::Exists(iconpath + ".jpg"))
      {
        at(i).SetIcon(iconpath + ".jpg");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
      else if (CFile::Exists(iconpath + ".png"))
      {
        at(i).SetIcon(iconpath + ".png");
        if (writeDB) database->UpdateDBChannel(at(i));
        continue;
      }
    }

    /* Start channel icon scraper here if nothing was found*/
    /// TODO


    CLog::Log(LOGNOTICE,"PVR: No channel icon found for %s, use '%s' or '%i' with extension 'tbn', 'jpg' or 'png'", at(i).ChannelName().c_str(), at(i).ClientChannelName().c_str(), at(i).UniqueID());
  }

  database->Close();
}

void CPVRChannels::ReNumberAndCheck(void)
{
  int Number = 1;
  m_iHiddenChannels = 0;
  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).ClientChannelNumber() <= 0 && !at(i).IsVirtual())
    {
      CLog::Log(LOGERROR, "PVR: Channel '%s' from client '%ld' is invalid, removing from list", at(i).ChannelName().c_str(), at(i).ClientID());
      erase(begin()+i);
      i--;
      break;
    }

    if (at(i).UniqueID() <= 0 && !at(i).IsVirtual())
      CLog::Log(LOGNOTICE, "PVR: Channel '%s' from client '%ld' have no unique ID. Contact PVR Client developer.", at(i).ChannelName().c_str(), at(i).ClientID());

    if (at(i).ChannelName().IsEmpty())
    {
      CStdString name;
      CLog::Log(LOGERROR, "PVR: Client channel '%i' from client '%ld' have no channel name", at(i).ClientChannelNumber(), at(i).ClientID());
      name.Format(g_localizeStrings.Get(19085), at(i).ClientChannelNumber());
      at(i).SetChannelName(name);
    }

    if (at(i).IsHidden())
      m_iHiddenChannels++;

    at(i).SetChannelNumber(Number);

    CStdString path;
    if (!m_bRadio)
      path.Format("pvr://channels/tv/all/%i.pvr", Number);
    else
      path.Format("pvr://channels/radio/all/%i.pvr", Number);

    at(i).SetPath(path);
    Number++;
  }
}

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

void CPVRChannels::MoveChannel(unsigned int oldindex, unsigned int newindex)
{
  CPVRChannels m_channels_temp;

  if ((newindex == oldindex) || (newindex == 0))
    return;

  CTVDatabase *database = g_PVRManager.GetTVDatabase();
  database->Open();

  m_channels_temp.push_back(at(oldindex-1));
  erase(begin()+oldindex-1);
  if (newindex < size())
    insert(begin()+newindex-1, m_channels_temp[0]);
  else
    push_back(m_channels_temp[0]);

  for (unsigned int i = 0; i < size(); i++)
  {
    if (at(i).ChannelNumber() != (int) i+1)
    {
      CStdString path;
      at(i).SetChannelNumber(i+1);

      if (!m_bRadio)
        path.Format("pvr://channels/tv/all/%i.pvr", at(i).ChannelNumber());
      else
        path.Format("pvr://channels/radio/all/%i.pvr", at(i).ChannelNumber());

      at(i).SetPath(path);
      database->UpdateDBChannel(at(i));
    }
  }

  CLog::Log(LOGNOTICE, "PVR: TV Channel %d moved to %d", oldindex, newindex);
  database->Close();

  /* Synchronize channel numbers inside timers */
  for (unsigned int i = 0; i < PVRTimers.size(); i++)
  {
    CPVRChannel *tag = GetByClient(PVRTimers[i].Number(), PVRTimers[i].ClientID());
    if (tag)
      PVRTimers[i].SetNumber(tag->ChannelNumber());
  }

  return;
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
    ((CPVREpg *) at(number-1).GetEpg())->Clear();
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

  return at(Number-1).Icon();
}

void CPVRChannels::SetChannelIcon(unsigned int Number, CStdString Icon)
{
  if (Number > size()+1)
    return;

  if (at(Number-1).Icon() != Icon)
  {
    CTVDatabase *database = g_PVRManager.GetTVDatabase();
    database->Open();
    at(Number-1).SetIcon(Icon);
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
          const CPVREpgInfoTag *epgNow = channel->GetEpgNow();
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
