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

#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "threads/SingleLock.h"

#include "pvr/channels/PVRChannelGroupInternal.h"
#include "epg/EpgContainer.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"

#include <assert.h>

using namespace PVR;
using namespace EPG;

bool CPVRChannel::operator==(const CPVRChannel &right) const
{
  return (m_bIsRadio  == right.m_bIsRadio &&
          m_iUniqueId == right.m_iUniqueId &&
          m_iClientId == right.m_iClientId);
}

bool CPVRChannel::operator!=(const CPVRChannel &right) const
{
  return !(*this == right);
}

CPVRChannel::CPVRChannel(bool bRadio /* = false */)
{
  m_iChannelId              = -1;
  m_bIsRadio                = bRadio;
  m_bIsHidden               = false;
  m_bIsUserSetIcon          = false;
  m_bIsUserSetName          = false;
  m_bIsLocked               = false;
  m_iLastWatched            = 0;
  m_bChanged                = false;
  m_iCachedChannelNumber    = 0;
  m_iCachedSubChannelNumber = 0;

  m_iEpgId                  = -1;
  m_bEPGCreated             = false;
  m_bEPGEnabled             = true;
  m_strEPGScraper           = "client";

  m_iUniqueId               = -1;
  m_iClientId               = -1;
  m_iClientChannelNumber.channel    = 0;
  m_iClientChannelNumber.subchannel = 0;
  m_iClientEncryptionSystem = -1;
  UpdateEncryptionName();
}

CPVRChannel::CPVRChannel(const PVR_CHANNEL &channel, unsigned int iClientId)
{
  m_iChannelId              = -1;
  m_bIsRadio                = channel.bIsRadio;
  m_bIsHidden               = channel.bIsHidden;
  m_bIsUserSetIcon          = false;
  m_bIsUserSetName          = false;
  m_bIsLocked               = false;
  m_strIconPath             = channel.strIconPath;
  m_strChannelName          = channel.strChannelName;
  m_iUniqueId               = channel.iUniqueId;
  m_iClientChannelNumber.channel    = channel.iChannelNumber;
  m_iClientChannelNumber.subchannel = channel.iSubChannelNumber;
  m_strClientChannelName    = channel.strChannelName;
  m_strInputFormat          = channel.strInputFormat;
  m_strStreamURL            = channel.strStreamURL;
  m_iClientEncryptionSystem = channel.iEncryptionSystem;
  m_iCachedChannelNumber    = 0;
  m_iCachedSubChannelNumber = 0;
  m_iClientId               = iClientId;
  m_iLastWatched            = 0;
  m_bEPGEnabled             = !channel.bIsHidden;
  m_strEPGScraper           = "client";
  m_iEpgId                  = -1;
  m_bEPGCreated             = false;
  m_bChanged                = false;

  if (m_strChannelName.empty())
    m_strChannelName = StringUtils::Format("%s %d", g_localizeStrings.Get(19029).c_str(), m_iUniqueId);

  UpdateEncryptionName();
}

void CPVRChannel::Serialize(CVariant& value) const
{
  value["channelid"] = m_iChannelId;
  value["channeltype"] = m_bIsRadio ? "radio" : "tv";
  value["hidden"] = m_bIsHidden;
  value["locked"] = m_bIsLocked;
  value["icon"] = m_strIconPath;
  value["channel"]  = m_strChannelName;
  CDateTime lastPlayed(m_iLastWatched);
  value["lastplayed"] = lastPlayed.IsValid() ? lastPlayed.GetAsDBDate() : "";
  value["channelnumber"] = m_iCachedChannelNumber;
  value["subchannelnumber"] = m_iCachedSubChannelNumber;

  CEpgInfoTagPtr epg(GetEPGNow());
  if (epg)
  {
    // add the properties of the current EPG item to the main object
    epg->Serialize(value);
    // and add an extra sub-object with only the current EPG details
    epg->Serialize(value["broadcastnow"]);
  }

  epg = GetEPGNext();
  if (epg)
    epg->Serialize(value["broadcastnext"]);
}

/********** XBMC related channel methods **********/

bool CPVRChannel::Delete(void)
{
  bool bReturn = false;
  CPVRDatabase *database = GetPVRDatabase();
  if (!database)
    return bReturn;

  /* delete the EPG table */
  CEpg *epg = GetEPG();
  if (epg)
  {
    CPVRChannelPtr empty;
    epg->SetChannel(empty);
    g_EpgContainer.DeleteEpg(*epg, true);
    CSingleLock lock(m_critSection);
    m_bEPGCreated = false;
  }

  bReturn = database->Delete(*this);
  return bReturn;
}

CEpg *CPVRChannel::GetEPG(void) const
{
  int iEpgId(-1);
  {
    CSingleLock lock(m_critSection);
    if (!m_bIsHidden && m_bEPGEnabled && m_iEpgId > 0)
      iEpgId = m_iEpgId;
  }

  return iEpgId > 0 ? g_EpgContainer.GetById(iEpgId) : NULL;
}

bool CPVRChannel::UpdateFromClient(const CPVRChannelPtr &channel)
{
  assert(channel.get());

  SetClientID(channel->ClientID());
  SetStreamURL(channel->StreamURL());

  CSingleLock lock(m_critSection);

  if (m_iClientChannelNumber.channel    != channel->ClientChannelNumber() ||
      m_iClientChannelNumber.subchannel != channel->ClientSubChannelNumber() ||
      m_strInputFormat                  != channel->InputFormat() ||
      m_iClientEncryptionSystem         != channel->EncryptionSystem() ||
      m_strClientChannelName            != channel->ClientChannelName())
  {
    m_iClientChannelNumber.channel    = channel->ClientChannelNumber();
    m_iClientChannelNumber.subchannel = channel->ClientSubChannelNumber();
    m_strInputFormat                  = channel->InputFormat();
    m_iClientEncryptionSystem         = channel->EncryptionSystem();
    m_strClientChannelName            = channel->ClientChannelName();

    UpdateEncryptionName();
    SetChanged();
  }

  // only update the channel name and icon if the user hasn't changed them manually
  if (m_strChannelName.empty() || !IsUserSetName())
    SetChannelName(channel->ClientChannelName());
  if (m_strIconPath.empty() || !IsUserSetIcon())
    SetIconPath(channel->IconPath());

  return m_bChanged;
}

bool CPVRChannel::Persist()
{
  {
    // not changed
    CSingleLock lock(m_critSection);
    if (!m_bChanged && m_iChannelId > 0)
      return true;
  }

  if (CPVRDatabase *database = GetPVRDatabase())
  {
    bool bReturn = database->Persist(*this) && database->CommitInsertQueries();
    CSingleLock lock(m_critSection);
    m_bChanged = !bReturn;
    return bReturn;
  }

  return false;
}

bool CPVRChannel::SetChannelID(int iChannelId)
{
  CSingleLock lock(m_critSection);
  if (m_iChannelId != iChannelId)
  {
    /* update the id */
    m_iChannelId = iChannelId;
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

int CPVRChannel::ChannelNumber(void) const
{
  CSingleLock lock(m_critSection);
  return m_iCachedChannelNumber;
}

int CPVRChannel::SubChannelNumber(void) const
{
  CSingleLock lock(m_critSection);
  return m_iCachedSubChannelNumber;
}

bool CPVRChannel::SetHidden(bool bIsHidden)
{
  CSingleLock lock(m_critSection);

  if (m_bIsHidden != bIsHidden)
  {
    /* update the hidden flag */
    m_bIsHidden = bIsHidden;
	m_bEPGEnabled = !bIsHidden;
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

bool CPVRChannel::SetLocked(bool bIsLocked)
{
  CSingleLock lock(m_critSection);

  if (m_bIsLocked != bIsLocked)
  {
    /* update the locked flag */
    m_bIsLocked = bIsLocked;
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

bool CPVRChannel::IsRecording(void) const
{
  return g_PVRTimers->IsRecordingOnChannel(*this);
}

CPVRRecordingPtr CPVRChannel::GetRecording(void) const
{
  EPG::CEpgInfoTagPtr epgTag = GetEPGNow();
  return (epgTag && epgTag->HasRecording()) ?
      epgTag->Recording() :
      CPVRRecordingPtr();
}

bool CPVRChannel::HasRecording(void) const
{
  EPG::CEpgInfoTagPtr epgTag = GetEPGNow();
  return epgTag && epgTag->HasRecording();
}

bool CPVRChannel::SetIconPath(const std::string &strIconPath, bool bIsUserSetIcon /* = false */)
{
  CSingleLock lock(m_critSection);

  if (m_strIconPath != strIconPath)
  {
    /* update the path */
    m_strIconPath = StringUtils::Format("%s", strIconPath.c_str());
    SetChanged();
    m_bChanged = true;
    m_bIsUserSetIcon = bIsUserSetIcon && !m_strIconPath.empty();

    return true;
  }

  return false;
}

bool CPVRChannel::SetChannelName(const std::string &strChannelName, bool bIsUserSetName /*= false*/)
{
  std::string strName(strChannelName);

  if (strName.empty())
    strName = StringUtils::Format(g_localizeStrings.Get(19085).c_str(), ClientChannelNumber());

  CSingleLock lock(m_critSection);
  if (m_strChannelName != strName)
  {
    m_strChannelName = strName;
    m_bIsUserSetName = bIsUserSetName;

    /* if the user changes the name manually to an empty string we reset the
       flag and use the name from the client instead */
    if (bIsUserSetName && strChannelName.empty())
    {
      m_bIsUserSetName = false;
      m_strChannelName = ClientChannelName();
    }

    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

bool CPVRChannel::SetLastWatched(time_t iLastWatched)
{
  {
    CSingleLock lock(m_critSection);

    if (m_iLastWatched != iLastWatched)
      m_iLastWatched = iLastWatched;
  }

  if (CPVRDatabase *database = GetPVRDatabase())
    return database->UpdateLastWatched(*this);

  return false;
}

bool CPVRChannel::IsEmpty() const
{
  CSingleLock lock(m_critSection);
  return (m_strFileNameAndPath.empty() ||
          m_strStreamURL.empty());
}

/********** Client related channel methods **********/

bool CPVRChannel::SetClientID(int iClientId)
{
  CSingleLock lock(m_critSection);

  if (m_iClientId != iClientId)
  {
    /* update the client ID */
    m_iClientId = iClientId;
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

bool CPVRChannel::SetStreamURL(const std::string &strStreamURL)
{
  CSingleLock lock(m_critSection);

  if (m_strStreamURL != strStreamURL)
  {
    /* update the stream url */
    m_strStreamURL = StringUtils::Format("%s", strStreamURL.c_str());
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

void CPVRChannel::UpdatePath(CPVRChannelGroupInternal* group)
{
  if (!group) return;

  std::string strFileNameAndPath;
  CSingleLock lock(m_critSection);
  strFileNameAndPath = StringUtils::Format("pvr://channels/%s/%s/%s_%d.pvr",
                                           (m_bIsRadio ? "radio" : "tv"),
                                           group->GroupName().c_str(),
                                           g_PVRClients->GetClientAddonId(m_iClientId).c_str(),
                                           m_iUniqueId);
  if (m_strFileNameAndPath != strFileNameAndPath)
  {
    m_strFileNameAndPath = strFileNameAndPath;
    SetChanged();
  }
}

void CPVRChannel::UpdateEncryptionName(void)
{
  // http://www.dvb.org/index.php?id=174
  // http://en.wikipedia.org/wiki/Conditional_access_system
  std::string strName(g_localizeStrings.Get(13205)); /* Unknown */
  CSingleLock lock(m_critSection);

  if (     m_iClientEncryptionSystem == 0x0000)
    strName = g_localizeStrings.Get(19013); /* Free To Air */
  else if (m_iClientEncryptionSystem >= 0x0001 &&
           m_iClientEncryptionSystem <= 0x009F)
    strName = g_localizeStrings.Get(19014); /* Fixed */
  else if (m_iClientEncryptionSystem >= 0x00A0 &&
           m_iClientEncryptionSystem <= 0x00A1)
    strName = g_localizeStrings.Get(338); /* Analog */
  else if (m_iClientEncryptionSystem >= 0x00A2 &&
           m_iClientEncryptionSystem <= 0x00FF)
    strName = g_localizeStrings.Get(19014); /* Fixed */
  else if (m_iClientEncryptionSystem >= 0x0100 &&
           m_iClientEncryptionSystem <= 0x01FF)
    strName = "SECA Mediaguard";
  else if (m_iClientEncryptionSystem == 0x0464)
    strName = "EuroDec";
  else if (m_iClientEncryptionSystem >= 0x0500 &&
           m_iClientEncryptionSystem <= 0x05FF)
    strName = "Viaccess";
  else if (m_iClientEncryptionSystem >= 0x0600 &&
           m_iClientEncryptionSystem <= 0x06FF)
    strName = "Irdeto";
  else if (m_iClientEncryptionSystem >= 0x0900 &&
           m_iClientEncryptionSystem <= 0x09FF)
    strName = "NDS Videoguard";
  else if (m_iClientEncryptionSystem >= 0x0B00 &&
           m_iClientEncryptionSystem <= 0x0BFF)
    strName = "Conax";
  else if (m_iClientEncryptionSystem >= 0x0D00 &&
           m_iClientEncryptionSystem <= 0x0DFF)
    strName = "CryptoWorks";
  else if (m_iClientEncryptionSystem >= 0x0E00 &&
           m_iClientEncryptionSystem <= 0x0EFF)
    strName = "PowerVu";
  else if (m_iClientEncryptionSystem == 0x1000)
    strName = "RAS";
  else if (m_iClientEncryptionSystem >= 0x1200 &&
           m_iClientEncryptionSystem <= 0x12FF)
    strName = "NagraVision";
  else if (m_iClientEncryptionSystem >= 0x1700 &&
           m_iClientEncryptionSystem <= 0x17FF)
    strName = "BetaCrypt";
  else if (m_iClientEncryptionSystem >= 0x1800 &&
           m_iClientEncryptionSystem <= 0x18FF)
    strName = "NagraVision";
  else if (m_iClientEncryptionSystem == 0x22F0)
    strName = "Codicrypt";
  else if (m_iClientEncryptionSystem == 0x2600)
    strName = "BISS";
  else if (m_iClientEncryptionSystem == 0x4347)
    strName = "CryptOn";
  else if (m_iClientEncryptionSystem == 0x4800)
    strName = "Accessgate";
  else if (m_iClientEncryptionSystem == 0x4900)
    strName = "China Crypt";
  else if (m_iClientEncryptionSystem == 0x4A10)
    strName = "EasyCas";
  else if (m_iClientEncryptionSystem == 0x4A20)
    strName = "AlphaCrypt";
  else if (m_iClientEncryptionSystem == 0x4A70)
    strName = "DreamCrypt";
  else if (m_iClientEncryptionSystem == 0x4A60)
    strName = "SkyCrypt";
  else if (m_iClientEncryptionSystem == 0x4A61)
    strName = "Neotioncrypt";
  else if (m_iClientEncryptionSystem == 0x4A62)
    strName = "SkyCrypt";
  else if (m_iClientEncryptionSystem == 0x4A63)
    strName = "Neotion SHL";
  else if (m_iClientEncryptionSystem >= 0x4A64 &&
           m_iClientEncryptionSystem <= 0x4A6F)
    strName = "SkyCrypt";
  else if (m_iClientEncryptionSystem == 0x4A80)
    strName = "ThalesCrypt";
  else if (m_iClientEncryptionSystem == 0x4AA1)
    strName = "KeyFly";
  else if (m_iClientEncryptionSystem == 0x4ABF)
    strName = "DG-Crypt";
  else if (m_iClientEncryptionSystem >= 0x4AD0 &&
           m_iClientEncryptionSystem <= 0x4AD1)
    strName = "X-Crypt";
  else if (m_iClientEncryptionSystem == 0x4AD4)
    strName = "OmniCrypt";
  else if (m_iClientEncryptionSystem == 0x4AE0)
    strName = "RossCrypt";
  else if (m_iClientEncryptionSystem == 0x5500)
    strName = "Z-Crypt";
  else if (m_iClientEncryptionSystem == 0x5501)
    strName = "Griffin";
  else if (m_iClientEncryptionSystem == 0x5601)
    strName = "Verimatrix";

  if (m_iClientEncryptionSystem >= 0)
    strName += StringUtils::Format(" (%04X)", m_iClientEncryptionSystem);

  m_strClientEncryptionName = strName;
}

/********** EPG methods **********/

int CPVRChannel::GetEPG(CFileItemList &results) const
{
  CEpg *epg = GetEPG();
  if (!epg)
  {
    CLog::Log(LOGDEBUG, "PVR - %s - cannot get EPG for channel '%s'",
        __FUNCTION__, m_strChannelName.c_str());
    return -1;
  }

  return epg->Get(results);
}

bool CPVRChannel::ClearEPG() const
{
  CEpg *epg = GetEPG();
  if (epg)
    epg->Clear();

  return true;
}

CEpgInfoTagPtr CPVRChannel::GetEPGNow() const
{
  CEpg *epg = GetEPG();
  if (epg)
    return epg->GetTagNow();

  CEpgInfoTagPtr empty;
  return empty;
}

CEpgInfoTagPtr CPVRChannel::GetEPGNext() const
{
  CEpg *epg = GetEPG();
  if (epg)
    return epg->GetTagNext();

  CEpgInfoTagPtr empty;
  return empty;
}

bool CPVRChannel::SetEPGEnabled(bool bEPGEnabled)
{
  CSingleLock lock(m_critSection);

  if (m_bEPGEnabled != bEPGEnabled)
  {
    /* update the EPG flag */
    m_bEPGEnabled = bEPGEnabled;
    SetChanged();
    m_bChanged = true;

    /* clear the previous EPG entries if needed */
    if (!m_bEPGEnabled && m_bEPGCreated)
      ClearEPG();

    return true;
  }

  return false;
}

bool CPVRChannel::SetEPGScraper(const std::string &strScraper)
{
  CSingleLock lock(m_critSection);

  if (m_strEPGScraper != strScraper)
  {
    bool bCleanEPG = !m_strEPGScraper.empty() || strScraper.empty();

    /* update the scraper name */
    m_strEPGScraper = StringUtils::Format("%s", strScraper.c_str());
    SetChanged();
    m_bChanged = true;

    /* clear the previous EPG entries if needed */
    if (bCleanEPG && m_bEPGEnabled && m_bEPGCreated)
      ClearEPG();

    return true;
  }

  return false;
}

void CPVRChannel::SetCachedChannelNumber(unsigned int iChannelNumber)
{
  CSingleLock lock(m_critSection);
  m_iCachedChannelNumber = iChannelNumber;
}

void CPVRChannel::SetCachedSubChannelNumber(unsigned int iSubChannelNumber)
{
  CSingleLock lock(m_critSection);
  m_iCachedSubChannelNumber = iSubChannelNumber;
}

void CPVRChannel::ToSortable(SortItem& sortable, Field field) const
{
  CSingleLock lock(m_critSection);
  if (field == FieldChannelName)
    sortable[FieldChannelName] = m_strChannelName;
  else if (field == FieldChannelNumber)
    sortable[FieldChannelNumber] = m_iCachedChannelNumber;
}

int CPVRChannel::ChannelID(void) const
{
  CSingleLock lock(m_critSection);
  return m_iChannelId;
}

bool CPVRChannel::IsNew(void) const
{
  CSingleLock lock(m_critSection);
  return m_iChannelId <= 0;
}

bool CPVRChannel::IsHidden(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsHidden;
}

bool CPVRChannel::IsSubChannel(void) const
{
  return SubChannelNumber() > 0;
}

bool CPVRChannel::IsClientSubChannel(void) const
{
  return ClientSubChannelNumber() > 0;
}

std::string CPVRChannel::FormattedChannelNumber(void) const
{
  return !IsSubChannel() ?
      StringUtils::Format("%i", ChannelNumber()) :
      StringUtils::Format("%i.%i", ChannelNumber(), SubChannelNumber());
}

bool CPVRChannel::IsLocked(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsLocked;
}

std::string CPVRChannel::IconPath(void) const
{
  CSingleLock lock(m_critSection);
  std::string strReturn(m_strIconPath);
  return strReturn;
}

bool CPVRChannel::IsUserSetIcon(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsUserSetIcon;
}

bool CPVRChannel::IsIconExists() const
{
  return XFILE::CFile::Exists(IconPath());
}

bool CPVRChannel::IsUserSetName() const
{
  CSingleLock lock(m_critSection);
  return m_bIsUserSetName;
}

std::string CPVRChannel::ChannelName(void) const
{
  CSingleLock lock(m_critSection);
  return m_strChannelName;
}

time_t CPVRChannel::LastWatched(void) const
{
  CSingleLock lock(m_critSection);
  return m_iLastWatched;
}

bool CPVRChannel::IsChanged() const
{
  CSingleLock lock(m_critSection);
  return m_bChanged;
}

int CPVRChannel::UniqueID(void) const
{
  return m_iUniqueId;
}

int CPVRChannel::ClientID(void) const
{
  CSingleLock lock(m_critSection);
  return m_iClientId;
}

unsigned int CPVRChannel::ClientChannelNumber(void) const
{
  CSingleLock lock(m_critSection);
  return m_iClientChannelNumber.channel;
}

unsigned int CPVRChannel::ClientSubChannelNumber(void) const
{
  CSingleLock lock(m_critSection);
  return m_iClientChannelNumber.subchannel;
}

std::string CPVRChannel::ClientChannelName(void) const
{
  CSingleLock lock(m_critSection);
  std::string strReturn(m_strClientChannelName);
  return strReturn;
}

std::string CPVRChannel::InputFormat(void) const
{
  CSingleLock lock(m_critSection);
  std::string strReturn(m_strInputFormat);
  return strReturn;
}

std::string CPVRChannel::StreamURL(void) const
{
  CSingleLock lock(m_critSection);
  std::string strReturn(m_strStreamURL);
  return strReturn;
}

std::string CPVRChannel::Path(void) const
{
  CSingleLock lock(m_critSection);
  std::string strReturn(m_strFileNameAndPath);
  return strReturn;
}

bool CPVRChannel::IsEncrypted(void) const
{
  CSingleLock lock(m_critSection);
  return m_iClientEncryptionSystem > 0;
}

int CPVRChannel::EncryptionSystem(void) const
{
  CSingleLock lock(m_critSection);
  return m_iClientEncryptionSystem;
}

std::string CPVRChannel::EncryptionName(void) const
{
  CSingleLock lock(m_critSection);
  std::string strReturn(m_strClientEncryptionName);
  return strReturn;
}

int CPVRChannel::EpgID(void) const
{
  CSingleLock lock(m_critSection);
  return m_iEpgId;
}

void CPVRChannel::SetEpgID(int iEpgId)
{
  CSingleLock lock(m_critSection);
  m_iEpgId = iEpgId;
  SetChanged();
}

bool CPVRChannel::EPGEnabled(void) const
{
  CSingleLock lock(m_critSection);
  return m_bEPGEnabled;
}

std::string CPVRChannel::EPGScraper(void) const
{
  CSingleLock lock(m_critSection);
  std::string strReturn(m_strEPGScraper);
  return strReturn;
}

bool CPVRChannel::CanRecord(void) const
{
  return g_PVRClients->SupportsRecordings(m_iClientId);
}
