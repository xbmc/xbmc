/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "Util.h"
#include "filesystem/File.h"
#include "settings/GUISettings.h"
#include "utils/StringUtils.h"
#include "threads/SingleLock.h"

#include "pvr/channels/PVRChannelGroupInternal.h"
#include "epg/EpgContainer.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/PVRDatabase.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"

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
  m_bIsLocked               = false;
  m_strIconPath             = StringUtils::EmptyString;
  m_strChannelName          = StringUtils::EmptyString;
  m_bIsVirtual              = false;
  m_iLastWatched            = 0;
  m_bChanged                = false;
  m_iCachedChannelNumber    = 0;

  m_iEpgId                  = -1;
  m_bEPGCreated             = false;
  m_bEPGEnabled             = true;
  m_strEPGScraper           = "client";

  m_iUniqueId               = -1;
  m_iClientId               = -1;
  m_iClientChannelNumber    = -1;
  m_strClientChannelName    = StringUtils::EmptyString;
  m_strInputFormat          = StringUtils::EmptyString;
  m_strStreamURL            = StringUtils::EmptyString;
  m_strFileNameAndPath      = StringUtils::EmptyString;
  m_iClientEncryptionSystem = -1;
  UpdateEncryptionName();
}

CPVRChannel::CPVRChannel(const PVR_CHANNEL &channel, unsigned int iClientId)
{
  m_iChannelId              = -1;
  m_bIsRadio                = channel.bIsRadio;
  m_bIsHidden               = channel.bIsHidden;
  m_bIsUserSetIcon          = false;
  m_bIsLocked               = false;
  m_strIconPath             = channel.strIconPath;
  m_strChannelName          = channel.strChannelName;
  m_iUniqueId               = channel.iUniqueId;
  m_iClientChannelNumber    = channel.iChannelNumber;
  m_strClientChannelName    = channel.strChannelName;
  m_strInputFormat          = channel.strInputFormat;
  m_strStreamURL            = channel.strStreamURL;
  m_iClientEncryptionSystem = channel.iEncryptionSystem;
  m_iCachedChannelNumber    = 0;
  m_iClientId               = iClientId;
  m_strFileNameAndPath      = StringUtils::EmptyString;
  m_bIsVirtual              = false;
  m_iLastWatched            = 0;
  m_bEPGEnabled             = true;
  m_strEPGScraper           = "client";
  m_iEpgId                  = -1;
  m_bEPGCreated             = false;
  m_bChanged                = false;

  if (m_strChannelName.IsEmpty())
    m_strChannelName.Format("%s %d", g_localizeStrings.Get(19029), m_iUniqueId);

  UpdateEncryptionName();
}

CPVRChannel::CPVRChannel(const CPVRChannel &channel)
{
  *this = channel;
}

CPVRChannel &CPVRChannel::operator=(const CPVRChannel &channel)
{
  m_iChannelId              = channel.m_iChannelId;
  m_bIsRadio                = channel.m_bIsRadio;
  m_bIsHidden               = channel.m_bIsHidden;
  m_bIsUserSetIcon          = channel.m_bIsUserSetIcon;
  m_bIsLocked               = channel.m_bIsLocked;
  m_strIconPath             = channel.m_strIconPath;
  m_strChannelName          = channel.m_strChannelName;
  m_bIsVirtual              = channel.m_bIsVirtual;
  m_iLastWatched            = channel.m_iLastWatched;
  m_bEPGEnabled             = channel.m_bEPGEnabled;
  m_strEPGScraper           = channel.m_strEPGScraper;
  m_iUniqueId               = channel.m_iUniqueId;
  m_iClientId               = channel.m_iClientId;
  m_iClientChannelNumber    = channel.m_iClientChannelNumber;
  m_strClientChannelName    = channel.m_strClientChannelName;
  m_strInputFormat          = channel.m_strInputFormat;
  m_strStreamURL            = channel.m_strStreamURL;
  m_strFileNameAndPath      = channel.m_strFileNameAndPath;
  m_iClientEncryptionSystem = channel.m_iClientEncryptionSystem;
  m_iCachedChannelNumber    = channel.m_iCachedChannelNumber;
  m_iEpgId                  = channel.m_iEpgId;
  m_bEPGCreated             = channel.m_bEPGCreated;
  m_bChanged                = channel.m_bChanged;

  UpdateEncryptionName();

  return *this;
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
  value["lastplayed"] = lastPlayed.IsValid() ? lastPlayed.GetAsDBDate() : StringUtils::EmptyString;
  value["channelnumber"] = m_iCachedChannelNumber;
  
  CEpgInfoTag epg;
  if (GetEPGNow(epg))
    epg.Serialize(value);
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

bool CPVRChannel::UpdateFromClient(const CPVRChannel &channel)
{
  SetClientID(channel.ClientID());
  SetClientChannelNumber(channel.ClientChannelNumber());
  SetInputFormat(channel.InputFormat());
  SetStreamURL(channel.StreamURL());
  SetEncryptionSystem(channel.EncryptionSystem());
  SetClientChannelName(channel.ClientChannelName());

  CSingleLock lock(m_critSection);
  if (m_strChannelName.IsEmpty())
    SetChannelName(channel.ClientChannelName());
  if (m_strIconPath.IsEmpty()||(!m_strIconPath.Equals(channel.IconPath()) && !IsUserSetIcon()))
    SetIconPath(channel.IconPath());

  return m_bChanged;
}

bool CPVRChannel::Persist(bool bQueueWrite /* = false */)
{
  {
    // not changed
    CSingleLock lock(m_critSection);
    if (!m_bChanged && m_iChannelId > 0)
      return true;
  }

  if (CPVRDatabase *database = GetPVRDatabase())
  {
    if (!bQueueWrite)
    {
      bool bReturn = database->Persist(*this, false);
      CSingleLock lock(m_critSection);
      m_bChanged = !bReturn;
    }
    else
    {
      return database->Persist(*this, true);
    }
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

bool CPVRChannel::SetHidden(bool bIsHidden)
{
  CSingleLock lock(m_critSection);

  if (m_bIsHidden != bIsHidden)
  {
    /* update the hidden flag */
    m_bIsHidden = bIsHidden;
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

bool CPVRChannel::SetIconPath(const CStdString &strIconPath, bool bIsUserSetIcon /* = false */)
{
  CSingleLock lock(m_critSection);

  if (m_strIconPath != strIconPath)
  {
    /* update the path */
    m_strIconPath.Format("%s", strIconPath);
    SetChanged();
    m_bChanged = true;

    /* did the user change the icon? */
    if (bIsUserSetIcon)
      m_bIsUserSetIcon = !m_strIconPath.IsEmpty();
	  
    return true;
  }

  return false;
}

bool CPVRChannel::SetChannelName(const CStdString &strChannelName)
{
  CStdString strName(strChannelName);

  if (strName.IsEmpty())
    strName.Format(g_localizeStrings.Get(19085), ClientChannelNumber());

  CSingleLock lock(m_critSection);
  if (m_strChannelName != strName)
  {
    /* update the channel name */
    m_strChannelName = strName;
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

bool CPVRChannel::SetVirtual(bool bIsVirtual)
{
  CSingleLock lock(m_critSection);

  if (m_bIsVirtual != bIsVirtual)
  {
    /* update the virtual flag */
    m_bIsVirtual = bIsVirtual;
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

bool CPVRChannel::SetLastWatched(time_t iLastWatched)
{
  CSingleLock lock(m_critSection);

  if (m_iLastWatched != iLastWatched)
  {
    /* update last watched  */
    m_iLastWatched = iLastWatched;
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

bool CPVRChannel::IsEmpty() const
{
  CSingleLock lock(m_critSection);
  return (m_strFileNameAndPath.IsEmpty() ||
          m_strStreamURL.IsEmpty());
}

/********** Client related channel methods **********/

bool CPVRChannel::SetUniqueID(int iUniqueId)
{
  CSingleLock lock(m_critSection);

  if (m_iUniqueId != iUniqueId)
  {
    /* update the unique ID */
    m_iUniqueId = iUniqueId;
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

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

bool CPVRChannel::SetClientChannelNumber(int iClientChannelNumber)
{
  CSingleLock lock(m_critSection);

  if (m_iClientChannelNumber != iClientChannelNumber && iClientChannelNumber > 0)
  {
    /* update the client channel number */
    m_iClientChannelNumber = iClientChannelNumber;
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

bool CPVRChannel::SetClientChannelName(const CStdString &strClientChannelName)
{
  CSingleLock lock(m_critSection);

  if (m_strClientChannelName != strClientChannelName)
  {
    /* update the client channel name */
    m_strClientChannelName.Format("%s", strClientChannelName);
    SetChanged();
    // this is not persisted, so don't update m_bChanged

    return true;
  }

  return false;
}

bool CPVRChannel::SetInputFormat(const CStdString &strInputFormat)
{
  CSingleLock lock(m_critSection);

  if (m_strInputFormat != strInputFormat)
  {
    /* update the input format */
    m_strInputFormat.Format("%s", strInputFormat);
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

bool CPVRChannel::SetStreamURL(const CStdString &strStreamURL)
{
  CSingleLock lock(m_critSection);

  if (m_strStreamURL != strStreamURL)
  {
    /* update the stream url */
    m_strStreamURL.Format("%s", strStreamURL);
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

void CPVRChannel::UpdatePath(CPVRChannelGroupInternal* group, unsigned int iNewChannelGroupPosition)
{
  if (!group) return;

  CStdString strFileNameAndPath;
  CSingleLock lock(m_critSection);
  strFileNameAndPath.Format("pvr://channels/%s/%s/%i.pvr", (m_bIsRadio ? "radio" : "tv"), group->GroupName().c_str(), iNewChannelGroupPosition);
  if (m_strFileNameAndPath != strFileNameAndPath)
  {
    m_strFileNameAndPath = strFileNameAndPath;
    SetChanged();
  }
}

bool CPVRChannel::SetEncryptionSystem(int iClientEncryptionSystem)
{
  CSingleLock lock(m_critSection);

  if (m_iClientEncryptionSystem != iClientEncryptionSystem)
  {
    /* update the client encryption system */
    m_iClientEncryptionSystem = iClientEncryptionSystem;
    UpdateEncryptionName();
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

void CPVRChannel::UpdateEncryptionName(void)
{
  // http://www.dvb.org/index.php?id=174
  // http://en.wikipedia.org/wiki/Conditional_access_system
  CStdString strName(g_localizeStrings.Get(13205)); /* Unknown */
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

  if (m_iClientEncryptionSystem >= 0)
    strName.AppendFormat(" (%04X)", m_iClientEncryptionSystem);

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

bool CPVRChannel::GetEPGNow(CEpgInfoTag &tag) const
{
  CEpg *epg = GetEPG();
  return epg ? epg->InfoTagNow(tag) : false;
}

bool CPVRChannel::GetEPGNext(CEpgInfoTag &tag) const
{
  CEpg *epg = GetEPG();
  return epg ? epg->InfoTagNext(tag) : false;
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

bool CPVRChannel::SetEPGScraper(const CStdString &strScraper)
{
  CSingleLock lock(m_critSection);

  if (m_strEPGScraper != strScraper)
  {
    bool bCleanEPG = !m_strEPGScraper.IsEmpty() || strScraper.IsEmpty();

    /* update the scraper name */
    m_strEPGScraper.Format("%s", strScraper);
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

void CPVRChannel::ToSortable(SortItem& sortable) const
{
  CSingleLock lock(m_critSection);
  sortable[FieldChannelName] = m_strChannelName;
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

CStdString CPVRChannel::IconPath(void) const
{
  CSingleLock lock(m_critSection);
  CStdString strReturn(m_strIconPath);
  return strReturn;
}

bool CPVRChannel::IsUserSetIcon(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsUserSetIcon;
}

CStdString CPVRChannel::ChannelName(void) const
{
  CSingleLock lock(m_critSection);
  return m_strChannelName;
}

bool CPVRChannel::IsVirtual(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsVirtual;
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
  CSingleLock lock(m_critSection);
  return m_iUniqueId;
}

int CPVRChannel::ClientID(void) const
{
  CSingleLock lock(m_critSection);
  return m_iClientId;
}

int CPVRChannel::ClientChannelNumber(void) const
{
  CSingleLock lock(m_critSection);
  return m_iClientChannelNumber;
}

CStdString CPVRChannel::ClientChannelName(void) const
{
  CSingleLock lock(m_critSection);
  CStdString strReturn(m_strClientChannelName);
  return strReturn;
}

CStdString CPVRChannel::InputFormat(void) const
{
  CSingleLock lock(m_critSection);
  CStdString strReturn(m_strInputFormat);
  return strReturn;
}

CStdString CPVRChannel::StreamURL(void) const
{
  CSingleLock lock(m_critSection);
  CStdString strReturn(m_strStreamURL);
  return strReturn;
}

CStdString CPVRChannel::Path(void) const
{
  CSingleLock lock(m_critSection);
  CStdString strReturn(m_strFileNameAndPath);
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

CStdString CPVRChannel::EncryptionName(void) const
{
  CSingleLock lock(m_critSection);
  CStdString strReturn(m_strClientEncryptionName);
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

CStdString CPVRChannel::EPGScraper(void) const
{
  CSingleLock lock(m_critSection);
  CStdString strReturn(m_strEPGScraper);
  return strReturn;
}

bool CPVRChannel::CanRecord(void) const
{
  return g_PVRClients->SupportsRecordings(m_iClientId);
}
