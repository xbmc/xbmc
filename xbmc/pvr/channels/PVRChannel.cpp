/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRChannel.h"

#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgChannelData.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/providers/PVRProviders.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <memory>
#include <string>

using namespace PVR;

const std::string CPVRChannel::IMAGE_OWNER_PATTERN = "pvrchannel_{}";

bool CPVRChannel::operator==(const CPVRChannel& right) const
{
  return (m_bIsRadio == right.m_bIsRadio &&
          m_iUniqueId == right.m_iUniqueId &&
          m_iClientId == right.m_iClientId);
}

bool CPVRChannel::operator!=(const CPVRChannel& right) const
{
  return !(*this == right);
}

CPVRChannel::CPVRChannel(bool bRadio)
  : m_bIsRadio(bRadio),
    m_iconPath("", StringUtils::Format(IMAGE_OWNER_PATTERN, bRadio ? "radio" : "tv"))
{
  UpdateEncryptionName();
}

CPVRChannel::CPVRChannel(bool bRadio, const std::string& iconPath)
  : m_bIsRadio(bRadio),
    m_iconPath(iconPath, StringUtils::Format(IMAGE_OWNER_PATTERN, bRadio ? "radio" : "tv"))
{
  UpdateEncryptionName();
}

CPVRChannel::CPVRChannel(const PVR_CHANNEL& channel, unsigned int iClientId)
  : m_bIsRadio(channel.bIsRadio),
    m_bIsHidden(channel.bIsHidden),
    m_iconPath(channel.strIconPath,
               StringUtils::Format(IMAGE_OWNER_PATTERN, channel.bIsRadio ? "radio" : "tv")),
    m_strChannelName(channel.strChannelName),
    m_bHasArchive(channel.bHasArchive),
    m_bEPGEnabled(!channel.bIsHidden),
    m_iUniqueId(channel.iUniqueId),
    m_iClientId(iClientId),
    m_clientChannelNumber(channel.iChannelNumber, channel.iSubChannelNumber),
    m_strClientChannelName(channel.strChannelName),
    m_strMimeType(channel.strMimeType),
    m_iClientEncryptionSystem(channel.iEncryptionSystem),
    m_iClientProviderUid(channel.iClientProviderUid)
{
  if (m_strChannelName.empty())
    m_strChannelName = StringUtils::Format("{} {}", g_localizeStrings.Get(19029), m_iUniqueId);

  UpdateEncryptionName();
}

CPVRChannel::~CPVRChannel()
{
  ResetEPG();
}

void CPVRChannel::Serialize(CVariant& value) const
{
  value["channelid"] = m_iChannelId;
  value["channeltype"] = m_bIsRadio ? "radio" : "tv";
  value["hidden"] = m_bIsHidden;
  value["locked"] = m_bIsLocked;
  value["icon"] = ClientIconPath();
  value["channel"]  = m_strChannelName;
  value["uniqueid"]  = m_iUniqueId;
  CDateTime lastPlayed(m_iLastWatched);
  value["lastplayed"] = lastPlayed.IsValid() ? lastPlayed.GetAsDBDate() : "";

  std::shared_ptr<CPVREpgInfoTag> epg = GetEPGNow();
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

  value["hasarchive"] = m_bHasArchive;
  value["clientid"] = m_iClientId;
}

bool CPVRChannel::QueueDelete()
{
  bool bReturn = false;
  const std::shared_ptr<CPVRDatabase> database = CServiceBroker::GetPVRManager().GetTVDatabase();
  if (!database)
    return bReturn;

  const std::shared_ptr<CPVREpg> epg = GetEPG();
  if (epg)
    ResetEPG();

  bReturn = database->QueueDeleteQuery(*this);
  return bReturn;
}

std::shared_ptr<CPVREpg> CPVRChannel::GetEPG() const
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

      // Subscribe for EPG delete event
      m_epg->Events().Subscribe(this, &CPVRChannel::Notify);
      return true;
    }
  }
  return false;
}

void CPVRChannel::Notify(const PVREvent& event)
{
  if (event == PVREvent::EpgDeleted)
  {
    ResetEPG();
  }
}

void CPVRChannel::ResetEPG()
{
  std::shared_ptr<CPVREpg> epgToUnsubscribe;
  {
    CSingleLock lock(m_critSection);
    if (m_epg)
    {
      epgToUnsubscribe = m_epg;
      m_epg.reset();
    }
  }

  if (epgToUnsubscribe)
    epgToUnsubscribe->Events().Unsubscribe(this);
}

bool CPVRChannel::UpdateFromClient(const std::shared_ptr<CPVRChannel>& channel)
{
  CSingleLock lock(m_critSection);

  SetClientID(channel->ClientID());
  SetArchive(channel->HasArchive());
  SetClientProviderUid(channel->ClientProviderUid());

  m_clientChannelNumber = channel->m_clientChannelNumber;
  m_strMimeType = channel->MimeType();
  m_iClientEncryptionSystem = channel->EncryptionSystem();
  m_strClientChannelName = channel->ClientChannelName();

  UpdateEncryptionName();

  // only update the channel name and icon if the user hasn't changed them manually
  if (m_strChannelName.empty() || !IsUserSetName())
    SetChannelName(channel->ClientChannelName());
  if (IconPath().empty() || !IsUserSetIcon())
    SetIconPath(channel->ClientIconPath());

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

  const std::shared_ptr<CPVRDatabase> database = CServiceBroker::GetPVRManager().GetTVDatabase();
  if (database)
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Persisting channel '{}'", m_strChannelName);

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

    m_bChanged = true;
    return true;
  }

  return false;
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

bool CPVRChannel::HasArchive() const
{
  CSingleLock lock(m_critSection);
  return m_bHasArchive;
}

bool CPVRChannel::SetArchive(bool bHasArchive)
{
  CSingleLock lock(m_critSection);

  if (m_bHasArchive != bHasArchive)
  {
    m_bHasArchive = bHasArchive;
    m_bChanged = true;
    return true;
  }

  return false;
}

bool CPVRChannel::SetIconPath(const std::string& strIconPath, bool bIsUserSetIcon /* = false */)
{
  if (StringUtils::StartsWith(strIconPath, "image://"))
  {
    CLog::LogF(LOGERROR, "Not allowed to call this method with an image URL");
    return false;
  }

  CSingleLock lock(m_critSection);
  if (ClientIconPath() == strIconPath)
    return false;

  m_iconPath.SetClientImage(strIconPath);
  m_bChanged = true;
  m_bIsUserSetIcon = bIsUserSetIcon && !IconPath().empty();
  return true;
}

bool CPVRChannel::SetChannelName(const std::string& strChannelName, bool bIsUserSetName /*= false*/)
{
  std::string strName(strChannelName);

  if (strName.empty())
    strName = StringUtils::Format(g_localizeStrings.Get(19085),
                                  m_clientChannelNumber.FormattedChannelNumber());

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

    m_bChanged = true;
    return true;
  }

  return false;
}

bool CPVRChannel::SetLastWatched(time_t iLastWatched)
{
  {
    CSingleLock lock(m_critSection);
    m_iLastWatched = iLastWatched;
  }

  const std::shared_ptr<CPVRDatabase> database = CServiceBroker::GetPVRManager().GetTVDatabase();
  if (database)
    return database->UpdateLastWatched(*this);

  return false;
}

/********** Client related channel methods **********/

bool CPVRChannel::SetClientID(int iClientId)
{
  CSingleLock lock(m_critSection);

  if (m_iClientId != iClientId)
  {
    m_iClientId = iClientId;
    m_bChanged = true;
    return true;
  }

  return false;
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
    strName += StringUtils::Format(" ({:04X})", iCaid);

  return strName;
}

void CPVRChannel::UpdateEncryptionName()
{
  CSingleLock lock(m_critSection);
  m_strClientEncryptionName = GetEncryptionName(m_iClientEncryptionSystem);
}

bool CPVRChannel::SetClientProviderUid(int iClientProviderUid)
{
  CSingleLock lock(m_critSection);

  if (m_iClientProviderUid != iClientProviderUid)
  {
    m_iClientProviderUid = iClientProviderUid;
    m_bChanged = true;
    return true;
  }

  return false;
}

/********** EPG methods **********/

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVRChannel::GetEpgTags() const
{
  const std::shared_ptr<CPVREpg> epg = GetEPG();
  if (!epg)
  {
    CLog::LogFC(LOGDEBUG, LOGPVR, "Cannot get EPG for channel '{}'", m_strChannelName);
    return {};
  }

  return epg->GetTags();
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVRChannel::GetEPGTimeline(
    const CDateTime& timelineStart,
    const CDateTime& timelineEnd,
    const CDateTime& minEventEnd,
    const CDateTime& maxEventStart) const
{
  const std::shared_ptr<CPVREpg> epg = GetEPG();
  if (epg)
  {
    return epg->GetTimeline(timelineStart, timelineEnd, minEventEnd, maxEventStart);
  }
  else
  {
    // return single gap tag spanning whole timeline
    return std::vector<std::shared_ptr<CPVREpgInfoTag>>{
        CreateEPGGapTag(timelineStart, timelineEnd)};
  }
}

std::shared_ptr<CPVREpgInfoTag> CPVRChannel::CreateEPGGapTag(const CDateTime& start,
                                                             const CDateTime& end) const
{
  const std::shared_ptr<CPVREpg> epg = GetEPG();
  if (epg)
    return std::make_shared<CPVREpgInfoTag>(epg->GetChannelData(), epg->EpgID(), start, end, true);
  else
    return std::make_shared<CPVREpgInfoTag>(std::make_shared<CPVREpgChannelData>(*this), -1, start,
                                            end, true);
}

bool CPVRChannel::ClearEPG() const
{
  const std::shared_ptr<CPVREpg> epg = GetEPG();
  if (epg)
    epg->Clear();

  return true;
}

std::shared_ptr<CPVREpgInfoTag> CPVRChannel::GetEPGNow() const
{
  std::shared_ptr<CPVREpgInfoTag> tag;
  const std::shared_ptr<CPVREpg> epg = GetEPG();
  if (epg)
    tag = epg->GetTagNow();

  return tag;
}

std::shared_ptr<CPVREpgInfoTag> CPVRChannel::GetEPGNext() const
{
  std::shared_ptr<CPVREpgInfoTag> tag;
  const std::shared_ptr<CPVREpg> epg = GetEPG();
  if (epg)
    tag = epg->GetTagNext();

  return tag;
}

std::shared_ptr<CPVREpgInfoTag> CPVRChannel::GetEPGPrevious() const
{
  std::shared_ptr<CPVREpgInfoTag> tag;
  const std::shared_ptr<CPVREpg> epg = GetEPG();
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

    m_bChanged = true;

    /* clear the previous EPG entries if needed */
    if (!m_bEPGEnabled && m_epg)
      ClearEPG();

    return true;
  }

  return false;
}

bool CPVRChannel::SetEPGScraper(const std::string& strScraper)
{
  CSingleLock lock(m_critSection);

  if (m_strEPGScraper != strScraper)
  {
    bool bCleanEPG = !m_strEPGScraper.empty() || strScraper.empty();

    m_strEPGScraper = strScraper;
    m_bChanged = true;

    /* clear the previous EPG entries if needed */
    if (bCleanEPG && m_bEPGEnabled && m_epg)
      ClearEPG();

    return true;
  }

  return false;
}

void CPVRChannel::ToSortable(SortItem& sortable, Field field) const
{
  CSingleLock lock(m_critSection);
  if (field == FieldChannelName)
    sortable[FieldChannelName] = m_strChannelName;
  else if (field == FieldLastPlayed)
  {
    const CDateTime lastWatched(m_iLastWatched);
    sortable[FieldLastPlayed] = lastWatched.IsValid() ? lastWatched.GetAsDBDateTime() : StringUtils::Empty;
  }
}

int CPVRChannel::ChannelID() const
{
  CSingleLock lock(m_critSection);
  return m_iChannelId;
}

bool CPVRChannel::IsNew() const
{
  CSingleLock lock(m_critSection);
  return m_iChannelId <= 0;
}

bool CPVRChannel::IsHidden() const
{
  CSingleLock lock(m_critSection);
  return m_bIsHidden;
}

bool CPVRChannel::IsLocked() const
{
  CSingleLock lock(m_critSection);
  return m_bIsLocked;
}

std::string CPVRChannel::ClientIconPath() const
{
  CSingleLock lock(m_critSection);
  return m_iconPath.GetClientImage();
}

std::string CPVRChannel::IconPath() const
{
  CSingleLock lock(m_critSection);
  return m_iconPath.GetLocalImage();
}

bool CPVRChannel::IsUserSetIcon() const
{
  CSingleLock lock(m_critSection);
  return m_bIsUserSetIcon;
}

bool CPVRChannel::IsUserSetName() const
{
  CSingleLock lock(m_critSection);
  return m_bIsUserSetName;
}

std::string CPVRChannel::ChannelName() const
{
  CSingleLock lock(m_critSection);
  return m_strChannelName;
}

time_t CPVRChannel::LastWatched() const
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

int CPVRChannel::UniqueID() const
{
  return m_iUniqueId;
}

int CPVRChannel::ClientID() const
{
  CSingleLock lock(m_critSection);
  return m_iClientId;
}

const CPVRChannelNumber& CPVRChannel::ClientChannelNumber() const
{
  CSingleLock lock(m_critSection);
  return m_clientChannelNumber;
}

std::string CPVRChannel::ClientChannelName() const
{
  CSingleLock lock(m_critSection);
  return m_strClientChannelName;
}

std::string CPVRChannel::MimeType() const
{
  CSingleLock lock(m_critSection);
  return m_strMimeType;
}

bool CPVRChannel::IsEncrypted() const
{
  CSingleLock lock(m_critSection);
  return m_iClientEncryptionSystem > 0;
}

int CPVRChannel::EncryptionSystem() const
{
  CSingleLock lock(m_critSection);
  return m_iClientEncryptionSystem;
}

std::string CPVRChannel::EncryptionName() const
{
  CSingleLock lock(m_critSection);
  return m_strClientEncryptionName;
}

int CPVRChannel::EpgID() const
{
  CSingleLock lock(m_critSection);
  return m_iEpgId;
}

bool CPVRChannel::EPGEnabled() const
{
  CSingleLock lock(m_critSection);
  return m_bEPGEnabled;
}

std::string CPVRChannel::EPGScraper() const
{
  CSingleLock lock(m_critSection);
  return m_strEPGScraper;
}

bool CPVRChannel::CanRecord() const
{
  const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  return client && client->GetClientCapabilities().SupportsRecordings() &&
         client->GetClientCapabilities().SupportsTimers();
}

std::shared_ptr<CPVRProvider> CPVRChannel::GetDefaultProvider() const
{
  return CServiceBroker::GetPVRManager().Providers()->GetByClient(m_iClientId,
                                                                  PVR_PROVIDER_INVALID_UID);
}

bool CPVRChannel::HasClientProvider() const
{
  CSingleLock lock(m_critSection);
  return m_iClientProviderUid != PVR_PROVIDER_INVALID_UID;
}

std::shared_ptr<CPVRProvider> CPVRChannel::GetProvider() const
{
  return CServiceBroker::GetPVRManager().Providers()->GetByClient(m_iClientId,
                                                                  m_iClientProviderUid);
}
