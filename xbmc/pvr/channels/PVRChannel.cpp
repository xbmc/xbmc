/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannel.h"

#include "ServiceBroker.h"
#include "addons/PVRClient.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/epg/EpgChannelData.h"
#include "pvr/epg/EpgContainer.h"

using namespace PVR;

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
  m_bHasArchive             = false;

  m_iEpgId                  = -1;
  m_bEPGEnabled             = true;
  m_strEPGScraper           = "client";

  m_iUniqueId               = -1;
  m_iClientId               = -1;
  m_iClientEncryptionSystem = -1;
  UpdateEncryptionName();
}

CPVRChannel::CPVRChannel(const PVR_CHANNEL &channel, unsigned int iClientId)
: m_clientChannelNumber(channel.iChannelNumber, channel.iSubChannelNumber)
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
  m_strClientChannelName    = channel.strChannelName;
  m_strInputFormat          = channel.strInputFormat;
  m_iClientEncryptionSystem = channel.iEncryptionSystem;
  m_iClientId               = iClientId;
  m_iLastWatched            = 0;
  m_bEPGEnabled             = !channel.bIsHidden;
  m_strEPGScraper           = "client";
  m_iEpgId                  = -1;
  m_bChanged                = false;
  m_bHasArchive             = channel.bHasArchive;

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
  value["uniqueid"]  = m_iUniqueId;
  CDateTime lastPlayed(m_iLastWatched);
  value["lastplayed"] = lastPlayed.IsValid() ? lastPlayed.GetAsDBDate() : "";
  value["channelnumber"] = m_channelNumber.GetChannelNumber();
  value["subchannelnumber"] = m_channelNumber.GetSubChannelNumber();

  CPVREpgInfoTagPtr epg = GetEPGNow();
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

  value["isrecording"] = false; // compat
  value["hasarchive"] = m_bHasArchive;
}

/********** XBMC related channel methods **********/

bool CPVRChannel::Delete(void)
{
  bool bReturn = false;
  const CPVRDatabasePtr database = CServiceBroker::GetPVRManager().GetTVDatabase();
  if (!database)
    return bReturn;

  const CPVREpgPtr epg = GetEPG();
  if (epg)
  {
    CServiceBroker::GetPVRManager().EpgContainer().DeleteEpg(epg, true);

    CSingleLock lock(m_critSection);
    m_epg.reset();
  }

  bReturn = database->Delete(*this);
  return bReturn;
}

CPVREpgPtr CPVRChannel::GetEPG(void) const
{
  const_cast<CPVRChannel*>(this)->CreateEPG();

  CSingleLock lock(m_critSection);
  if (!m_bIsHidden && m_bEPGEnabled)
    return m_epg;

  return {};
}

bool CPVRChannel::CreateEPG()
{
  CSingleLock lock(m_critSection);
  if (!m_epg)
  {
    m_epg = CServiceBroker::GetPVRManager().EpgContainer().CreateChannelEpg(m_iEpgId,
                                                                            m_strEPGScraper,
                                                                            std::make_shared<CPVREpgChannelData>(*this));
    if (m_epg)
    {
      if (m_epg->EpgID() != m_iEpgId)
      {
        m_iEpgId = m_epg->EpgID();
        m_bChanged = true;
      }
      return true;
    }
  }
  return false;
}

bool CPVRChannel::UpdateFromClient(const CPVRChannelPtr &channel)
{
  SetClientID(channel->ClientID());

  CSingleLock lock(m_critSection);

  if (m_clientChannelNumber     != channel->m_clientChannelNumber ||
      m_strInputFormat          != channel->InputFormat() ||
      m_iClientEncryptionSystem != channel->EncryptionSystem() ||
      m_strClientChannelName    != channel->ClientChannelName() ||
      m_bHasArchive             != channel->HasArchive())
  {
    m_clientChannelNumber     = channel->m_clientChannelNumber;
    m_strInputFormat          = channel->InputFormat();
    m_iClientEncryptionSystem = channel->EncryptionSystem();
    m_strClientChannelName    = channel->ClientChannelName();
    m_bHasArchive             = channel->HasArchive();

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

  const CPVRDatabasePtr database = CServiceBroker::GetPVRManager().GetTVDatabase();
  if (database)
  {
    bool bReturn = database->Persist(*this, true);

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
    m_iChannelId = iChannelId;

    const std::shared_ptr<CPVREpg> epg = GetEPG();
    if (epg)
      epg->GetChannelData()->SetChannelId(m_iChannelId);

    SetChanged();
    m_bChanged = true;
    return true;
  }

  return false;
}

const CPVRChannelNumber& CPVRChannel::ChannelNumber() const
{
  CSingleLock lock(m_critSection);
  return m_channelNumber;
}

bool CPVRChannel::SetHidden(bool bIsHidden)
{
  CSingleLock lock(m_critSection);

  if (m_bIsHidden != bIsHidden)
  {
    m_bIsHidden = bIsHidden;
    m_bEPGEnabled = !bIsHidden;

    const std::shared_ptr<CPVREpg> epg = GetEPG();
    if (epg)
    {
      epg->GetChannelData()->SetHidden(m_bIsHidden);
      epg->GetChannelData()->SetEPGEnabled(m_bEPGEnabled);
    }

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
    m_bIsLocked = bIsLocked;

    const std::shared_ptr<CPVREpg> epg = GetEPG();
    if (epg)
      epg->GetChannelData()->SetLocked(m_bIsLocked);

    SetChanged();
    m_bChanged = true;
    return true;
  }

  return false;
}

std::shared_ptr<CPVRRadioRDSInfoTag> CPVRChannel::GetRadioRDSInfoTag() const
{
  CSingleLock lock(m_critSection);
  return m_rdsTag;
}

void CPVRChannel::SetRadioRDSInfoTag(const std::shared_ptr<CPVRRadioRDSInfoTag>& tag)
{
  CSingleLock lock(m_critSection);
  m_rdsTag = tag;
}

bool CPVRChannel::HasArchive(void) const
{
  CSingleLock lock(m_critSection);
  return m_bHasArchive;
}

bool CPVRChannel::SetIconPath(const std::string &strIconPath, bool bIsUserSetIcon /* = false */)
{
  CSingleLock lock(m_critSection);
  if (m_strIconPath != strIconPath)
  {
    m_strIconPath = StringUtils::Format("%s", strIconPath.c_str());

    const std::shared_ptr<CPVREpg> epg = GetEPG();
    if (epg)
      epg->GetChannelData()->SetIconPath(m_strIconPath);

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
    strName = StringUtils::Format(g_localizeStrings.Get(19085).c_str(), m_clientChannelNumber.FormattedChannelNumber().c_str());

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

    const std::shared_ptr<CPVREpg> epg = GetEPG();
    if (epg)
      epg->GetChannelData()->SetChannelName(m_strChannelName);

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
    {
      m_iLastWatched = iLastWatched;

      const std::shared_ptr<CPVREpg> epg = GetEPG();
      if (epg)
        epg->GetChannelData()->SetLastWatched(iLastWatched);
    }
  }

  const CPVRDatabasePtr database = CServiceBroker::GetPVRManager().GetTVDatabase();
  if (database)
    return database->UpdateLastWatched(*this);

  return false;
}

bool CPVRChannel::IsEmpty() const
{
  CSingleLock lock(m_critSection);
  return m_strFileNameAndPath.empty();
}

/********** Client related channel methods **********/

bool CPVRChannel::SetClientID(int iClientId)
{
  CSingleLock lock(m_critSection);

  if (m_iClientId != iClientId)
  {
    m_iClientId = iClientId;
    SetChanged();
    m_bChanged = true;
    return true;
  }

  return false;
}

void CPVRChannel::UpdatePath(const std::string& groupPath)
{
  const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client)
  {
    CSingleLock lock(m_critSection);
    const std::string strFileNameAndPath = StringUtils::Format("%s%s_%d.pvr",
                                                               groupPath,
                                                               client->ID().c_str(),
                                                               m_iUniqueId);
    if (m_strFileNameAndPath != strFileNameAndPath)
    {
      m_strFileNameAndPath = strFileNameAndPath;
      SetChanged();
    }
  }
}

std::string CPVRChannel::GetEncryptionName(int iCaid)
{
  // http://www.dvb.org/index.php?id=174
  // http://en.wikipedia.org/wiki/Conditional_access_system
  std::string strName(g_localizeStrings.Get(13205)); /* Unknown */

  if (     iCaid == 0x0000)
    strName = g_localizeStrings.Get(19013); /* Free To Air */
  else if (iCaid >= 0x0001 &&
           iCaid <= 0x009F)
    strName = g_localizeStrings.Get(19014); /* Fixed */
  else if (iCaid >= 0x00A0 &&
           iCaid<= 0x00A1)
    strName = g_localizeStrings.Get(338); /* Analog */
  else if (iCaid >= 0x00A2 &&
           iCaid <= 0x00FF)
    strName = g_localizeStrings.Get(19014); /* Fixed */
  else if (iCaid >= 0x0100 &&
           iCaid <= 0x01FF)
    strName = "SECA Mediaguard";
  else if (iCaid == 0x0464)
    strName = "EuroDec";
  else if (iCaid >= 0x0500 &&
           iCaid <= 0x05FF)
    strName = "Viaccess";
  else if (iCaid >= 0x0600 &&
           iCaid <= 0x06FF)
    strName = "Irdeto";
  else if (iCaid >= 0x0900 &&
           iCaid <= 0x09FF)
    strName = "NDS Videoguard";
  else if (iCaid >= 0x0B00 &&
           iCaid <= 0x0BFF)
    strName = "Conax";
  else if (iCaid >= 0x0D00 &&
           iCaid <= 0x0DFF)
    strName = "CryptoWorks";
  else if (iCaid >= 0x0E00 &&
           iCaid <= 0x0EFF)
    strName = "PowerVu";
  else if (iCaid == 0x1000)
    strName = "RAS";
  else if (iCaid >= 0x1200 &&
           iCaid <= 0x12FF)
    strName = "NagraVision";
  else if (iCaid >= 0x1700 &&
           iCaid <= 0x17FF)
    strName = "BetaCrypt";
  else if (iCaid >= 0x1800 &&
           iCaid <= 0x18FF)
    strName = "NagraVision";
  else if (iCaid == 0x22F0)
    strName = "Codicrypt";
  else if (iCaid == 0x2600)
    strName = "BISS";
  else if (iCaid == 0x4347)
    strName = "CryptOn";
  else if (iCaid == 0x4800)
    strName = "Accessgate";
  else if (iCaid == 0x4900)
    strName = "China Crypt";
  else if (iCaid == 0x4A10)
    strName = "EasyCas";
  else if (iCaid == 0x4A20)
    strName = "AlphaCrypt";
  else if (iCaid == 0x4A70)
    strName = "DreamCrypt";
  else if (iCaid == 0x4A60)
    strName = "SkyCrypt";
  else if (iCaid == 0x4A61)
    strName = "Neotioncrypt";
  else if (iCaid == 0x4A62)
    strName = "SkyCrypt";
  else if (iCaid == 0x4A63)
    strName = "Neotion SHL";
  else if (iCaid >= 0x4A64 &&
           iCaid <= 0x4A6F)
    strName = "SkyCrypt";
  else if (iCaid == 0x4A80)
    strName = "ThalesCrypt";
  else if (iCaid == 0x4AA1)
    strName = "KeyFly";
  else if (iCaid == 0x4ABF)
    strName = "DG-Crypt";
  else if (iCaid >= 0x4AD0 &&
           iCaid <= 0x4AD1)
    strName = "X-Crypt";
  else if (iCaid == 0x4AD4)
    strName = "OmniCrypt";
  else if (iCaid == 0x4AE0)
    strName = "RossCrypt";
  else if (iCaid == 0x5500)
    strName = "Z-Crypt";
  else if (iCaid == 0x5501)
    strName = "Griffin";
  else if (iCaid == 0x5601)
    strName = "Verimatrix";

  if (iCaid >= 0)
    strName += StringUtils::Format(" (%04X)", iCaid);

  return strName;
}

void CPVRChannel::UpdateEncryptionName(void)
{
  CSingleLock lock(m_critSection);
  m_strClientEncryptionName = GetEncryptionName(m_iClientEncryptionSystem);
}

/********** EPG methods **********/

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVRChannel::GetEpgTags() const
{
  const CPVREpgPtr epg = GetEPG();
  if (!epg)
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Cannot get EPG for channel '%s'", m_strChannelName.c_str());
    return {};
  }

  return epg->GetTags();
}

bool CPVRChannel::ClearEPG() const
{
  const CPVREpgPtr epg = GetEPG();
  if (epg)
    epg->Clear();

  return true;
}

CPVREpgInfoTagPtr CPVRChannel::GetEPGNow() const
{
  CPVREpgInfoTagPtr tag;
  const CPVREpgPtr epg = GetEPG();
  if (epg)
    tag = epg->GetTagNow();

  return tag;
}

CPVREpgInfoTagPtr CPVRChannel::GetEPGNext() const
{
  CPVREpgInfoTagPtr tag;
  const CPVREpgPtr epg = GetEPG();
  if (epg)
    tag = epg->GetTagNext();

  return tag;
}

CPVREpgInfoTagPtr CPVRChannel::GetEPGPrevious() const
{
  CPVREpgInfoTagPtr tag;
  const CPVREpgPtr epg = GetEPG();
  if (epg)
    tag = epg->GetTagPrevious();

  return tag;
}

bool CPVRChannel::SetEPGEnabled(bool bEPGEnabled)
{
  CSingleLock lock(m_critSection);

  if (m_bEPGEnabled != bEPGEnabled)
  {
    m_bEPGEnabled = bEPGEnabled;

    const std::shared_ptr<CPVREpg> epg = GetEPG();
    if (epg)
      epg->GetChannelData()->SetEPGEnabled(m_bEPGEnabled);

    SetChanged();
    m_bChanged = true;

    /* clear the previous EPG entries if needed */
    if (!m_bEPGEnabled && m_epg)
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

    m_strEPGScraper = StringUtils::Format("%s", strScraper.c_str());
    SetChanged();
    m_bChanged = true;

    /* clear the previous EPG entries if needed */
    if (bCleanEPG && m_bEPGEnabled && m_epg)
      ClearEPG();

    return true;
  }

  return false;
}

void CPVRChannel::SetChannelNumber(const CPVRChannelNumber& channelNumber)
{
  CSingleLock lock(m_critSection);
  if (m_channelNumber != channelNumber)
  {
    m_channelNumber = channelNumber;

    const std::shared_ptr<CPVREpg> epg = GetEPG();
    if (epg)
      epg->GetChannelData()->SetSortableChannelNumber(m_channelNumber.SortableChannelNumber());
  }
}

void CPVRChannel::ToSortable(SortItem& sortable, Field field) const
{
  CSingleLock lock(m_critSection);
  if (field == FieldChannelName)
    sortable[FieldChannelName] = m_strChannelName;
  else if (field == FieldChannelNumber)
    sortable[FieldChannelNumber] = m_channelNumber.SortableChannelNumber();
  else if (field == FieldLastPlayed)
  {
    const CDateTime lastWatched(m_iLastWatched);
    sortable[FieldLastPlayed] = lastWatched.IsValid() ? lastWatched.GetAsDBDateTime() : StringUtils::Empty;
  }
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

bool CPVRChannel::IsLocked(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsLocked;
}

std::string CPVRChannel::IconPath(void) const
{
  CSingleLock lock(m_critSection);
  return m_strIconPath;
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

void CPVRChannel::Persisted()
{
  CSingleLock lock(m_critSection);
  m_bChanged = false;
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

const CPVRChannelNumber& CPVRChannel::ClientChannelNumber() const
{
  CSingleLock lock(m_critSection);
  return m_clientChannelNumber;
}

std::string CPVRChannel::ClientChannelName(void) const
{
  CSingleLock lock(m_critSection);
  return m_strClientChannelName;
}

std::string CPVRChannel::InputFormat(void) const
{
  CSingleLock lock(m_critSection);
  return m_strInputFormat;
}

std::string CPVRChannel::Path(void) const
{
  CSingleLock lock(m_critSection);
  return m_strFileNameAndPath;
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
  return m_strClientEncryptionName;
}

int CPVRChannel::EpgID(void) const
{
  CSingleLock lock(m_critSection);
  return m_iEpgId;
}

void CPVRChannel::SetEpgID(int iEpgId)
{
  CSingleLock lock(m_critSection);
  if (m_iEpgId != iEpgId)
  {
    m_iEpgId = iEpgId;
    m_epg.reset();
    SetChanged();
    m_bChanged = true;
  }
}

bool CPVRChannel::EPGEnabled(void) const
{
  CSingleLock lock(m_critSection);
  return m_bEPGEnabled;
}

std::string CPVRChannel::EPGScraper(void) const
{
  CSingleLock lock(m_critSection);
  return m_strEPGScraper;
}

bool CPVRChannel::CanRecord(void) const
{
  const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  return client && client->GetClientCapabilities().SupportsRecordings();
}
