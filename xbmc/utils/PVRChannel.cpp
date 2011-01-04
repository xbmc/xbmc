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

#include "FileItem.h"
#include "LocalizeStrings.h"
#include "utils/log.h"
#include "Util.h"
#include "FileSystem/File.h"
#include "MusicInfoTag.h"

#include "PVRChannel.h"
#include "PVREpgs.h"
#include "PVREpg.h"
#include "PVREpgInfoTag.h"

using namespace XFILE;
using namespace MUSIC_INFO;

const CPVREpgInfoTag *m_EmptyEpgInfoTag = new CPVREpgInfoTag();

bool CPVRChannel::operator==(const CPVRChannel& right) const
{
  if (this == &right) return true;

  return (m_iDatabaseId             == right.m_iDatabaseId &&
          m_iUniqueId               == right.m_iUniqueId &&
          m_iChannelNumber          == right.m_iChannelNumber &&
          m_iClientChannelNumber    == right.m_iClientChannelNumber &&
          m_strClientChannelName    == right.m_strClientChannelName &&
          m_iClientId               == right.m_iClientId &&
          m_iChannelGroupId         == right.m_iChannelGroupId &&
          m_strChannelName          == right.m_strChannelName &&
          m_strIconPath             == right.m_strIconPath &&
          m_bIsRadio                == right.m_bIsRadio &&
          m_bIsHidden               == right.m_bIsHidden &&
          m_bClientIsRecording      == right.m_bClientIsRecording &&
          m_iClientEncryptionSystem == right.m_iClientEncryptionSystem &&
          m_strStreamURL            == right.m_strStreamURL &&
          m_strFileNameAndPath      == right.m_strFileNameAndPath);
}

bool CPVRChannel::operator!=(const CPVRChannel &right) const
{
  return (m_iDatabaseId             != right.m_iDatabaseId ||
          m_iUniqueId               != right.m_iUniqueId ||
          m_iChannelNumber          != right.m_iChannelNumber ||
          m_iClientChannelNumber    != right.m_iClientChannelNumber ||
          m_strClientChannelName    != right.m_strClientChannelName ||
          m_iClientId               != right.m_iClientId ||
          m_iChannelGroupId         != right.m_iChannelGroupId ||
          m_strChannelName          != right.m_strChannelName ||
          m_strIconPath             != right.m_strIconPath ||
          m_bIsRadio                != right.m_bIsRadio ||
          m_bIsHidden               != right.m_bIsHidden ||
          m_bClientIsRecording      != right.m_bClientIsRecording ||
          m_iClientEncryptionSystem != right.m_iClientEncryptionSystem ||
          m_strStreamURL            != right.m_strStreamURL ||
          m_strFileNameAndPath      != right.m_strFileNameAndPath);
}

void CPVRChannel::Reset()
{
  m_iDatabaseId             = -1;
  m_iChannelNumber          = -1;
  m_iChannelGroupId         = 0;
  m_strChannelName          = "";
  m_iClientEncryptionSystem = -1;
  m_iUniqueId               = -1;
  m_bIsRadio                = false;
  m_bIsHidden               = false;
  m_bClientIsRecording      = false;
  m_bGrabEpg                = true;
  m_strGrabber              = "client";
  m_bIsVirtual              = false;
  m_iPortalMasterChannel    = -1;

  m_iClientId               = -1;
  m_iClientChannelNumber    = -1;
  m_strClientChannelName    = "";

  m_strIconPath             = "";
  m_strFileNameAndPath      = "";
  m_strStreamURL            = "";

  m_Epg                     = NULL;

  ResetChannelEPGLinks();
  SetChanged();
}

CPVRChannel::~CPVRChannel()
{
  ResetChannelEPGLinks();
};

void CPVRChannel::ResetChannelEPGLinks()
{
  m_epgNow  = NULL;
}


void CPVRChannel::UpdateEpgPointers(void)
{
  if (m_Epg == NULL)
  {
    m_Epg = PVREpgs.GetEPG((CPVRChannel *) this);
  }

  if (m_Epg == NULL)
  {
    SetChanged(m_epgNow != NULL);
    m_epgNow = NULL;
  }
  else if (!m_Epg->IsUpdateRunning() &&
      (m_epgNow == NULL ||
       m_epgNow->End() < CDateTime::GetCurrentDateTime()))
  {
    SetChanged();
    m_epgNow  = m_Epg->GetInfoTagNow();
  }

  NotifyObservers("epg");
}

const CPVREpgInfoTag* CPVRChannel::GetEpgNow(void) const
{
  if (m_epgNow == NULL)
    return m_EmptyEpgInfoTag;

  return m_epgNow;
}

const CPVREpgInfoTag* CPVRChannel::GetEpgNext(void) const
{
  if (m_epgNow == NULL)
    return m_EmptyEpgInfoTag;

  return m_epgNow->GetNextEvent();
}

bool CPVRChannel::IsEmpty() const
{
  return (m_strFileNameAndPath.IsEmpty() ||
          m_strStreamURL.IsEmpty());
}

int CPVRChannel::GetPortalChannels(CFileItemList* results)
{
  if (m_iPortalMasterChannel != 0)
    return -1;

  for (unsigned int i = 0; i < m_PortalChannels.size(); i++)
  {
    CFileItemPtr channel(new CFileItem(*m_PortalChannels[i]));
    CStdString path;
    path.Format("pvr://channels/tv/portal-%04i/%i.pvr", ChannelID(),i+1);
    channel->m_strPath = path;
    results->Add(channel);
  }
  return results->Size();
}

void CPVRChannel::SetChannelName(CStdString name)
{
  m_strChannelName = name;
  SetChanged();
}

void CPVRChannel::SetChannelNumber(int Number)
{
  m_iChannelNumber = Number;
  SetChanged();
}

void CPVRChannel::SetClientChannelName(CStdString name)
{
  m_strClientChannelName = name;
  SetChanged();
}

void CPVRChannel::SetClientNumber(int Number)
{
  m_iClientChannelNumber = Number;
  SetChanged();
}

void CPVRChannel::SetClientID(int ClientId)
{
  m_iClientId = ClientId;
  SetChanged();
}

void CPVRChannel::SetChannelID(long ChannelID)
{
  m_iDatabaseId = ChannelID;
  SetChanged();
}

void CPVRChannel::SetUniqueID(int id)
{
  m_iUniqueId = id;
  SetChanged();
}

void CPVRChannel::SetGroupID(unsigned int group)
{
  m_iChannelGroupId = group;
  SetChanged();
}

void CPVRChannel::SetEncryptionSystem(int system)
{
  m_iClientEncryptionSystem = system;
  SetChanged();
}

void CPVRChannel::SetRadio(bool radio)
{
  m_bIsRadio = radio;
  SetChanged();
}

void CPVRChannel::SetRecording(bool rec)
{
  m_bClientIsRecording = rec;
  SetChanged();
}

void CPVRChannel::SetStreamURL(CStdString stream)
{
  m_strStreamURL = stream;
  SetChanged();
}

void CPVRChannel::SetPath(CStdString path)
{
  m_strFileNameAndPath = path;
  SetChanged();
}

void CPVRChannel::SetIcon(CStdString icon)
{
  m_strIconPath = icon;
  SetChanged();
}

void CPVRChannel::SetHidden(bool hide)
{
  m_bIsHidden = hide;
  SetChanged();
}

void CPVRChannel::SetGrabEpg(bool grabEpg)
{
  m_bGrabEpg = grabEpg;
  SetChanged();
}

void CPVRChannel::SetVirtual(bool virtualChannel)
{
  m_bIsVirtual = virtualChannel;
  SetChanged();
}

void CPVRChannel::SetGrabber(CStdString Grabber)
{
  m_strGrabber = Grabber;
  SetChanged();
}

void CPVRChannel::SetPortalChannel(long channelID)
{
  m_iPortalMasterChannel = channelID;
  SetChanged();
}

void CPVRChannel::ClearPortalChannels()
{
  m_PortalChannels.erase(m_PortalChannels.begin(), m_PortalChannels.end());
  SetChanged();
}

void CPVRChannel::AddPortalChannel(CPVRChannel* channel)
{
  m_PortalChannels.push_back(channel);
  SetChanged();
}

void CPVRChannel::SetInputFormat(CStdString format)
{
  m_strInputFormat = format;
  SetChanged();
}

void CPVRChannel::ClearLinkedChannels()
{
  m_linkedChannels.erase(m_linkedChannels.begin(), m_linkedChannels.end());
  SetChanged();
}

void CPVRChannel::AddLinkedChannel(long LinkedChannel)
{
  m_linkedChannels.push_back(LinkedChannel);
  SetChanged();
}

bool CPVRChannel::ClearEPG(bool bClearDatabase)
{
  if (m_Epg == NULL)
  {
    CLog::Log(LOGDEBUG, "%s - no EPG to clear", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGINFO, "%s - clearing the EPG for channel %s", __FUNCTION__, m_strChannelName.c_str());

  ((CPVREpg *) m_Epg)->Cleanup(-1);
  m_epgNow = NULL;

  return bClearDatabase ? PVREpgs.ClearEPGForChannel(this) : true;
}

CStdString CPVRChannel::EncryptionName() const
{
  // http://www.dvb.org/index.php?id=174
  // http://en.wikipedia.org/wiki/Conditional_access_system
  CStdString strName;

  if (     m_iClientEncryptionSystem == 0x0000)
    strName = g_localizeStrings.Get(19013); /* Free To Air */
  else if (m_iClientEncryptionSystem <  0x0000)
    strName = g_localizeStrings.Get(13205); /* Unknown */
  else if (m_iClientEncryptionSystem >= 0x0001 &&
           m_iClientEncryptionSystem <= 0x009F)
    strName.Format("%s (%X)", g_localizeStrings.Get(19014).c_str(), m_iClientEncryptionSystem); /* Fixed */
  else if (m_iClientEncryptionSystem >= 0x00A0 &&
           m_iClientEncryptionSystem <= 0x00A1)
    strName.Format("%s (%X)", g_localizeStrings.Get(338).c_str(), m_iClientEncryptionSystem); /* Analog */
  else if (m_iClientEncryptionSystem >= 0x00A2 &&
           m_iClientEncryptionSystem <= 0x00FF)
    strName.Format("%s (%X)", g_localizeStrings.Get(19014).c_str(), m_iClientEncryptionSystem); /* Fixed */
  else if (m_iClientEncryptionSystem >= 0x0100 &&
           m_iClientEncryptionSystem <= 0x01FF)
    strName.Format("%s (%X)", "SECA Mediaguard", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x0464)
    strName.Format("%s (%X)", "EuroDec", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x0500 &&
           m_iClientEncryptionSystem <= 0x05FF)
    strName.Format("%s (%X)", "Viaccess", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x0600 &&
           m_iClientEncryptionSystem <= 0x06FF)
    strName.Format("%s (%X)", "Irdeto", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x0900 &&
           m_iClientEncryptionSystem <= 0x09FF)
    strName.Format("%s (%X)", "NDS Videoguard", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x0B00 &&
           m_iClientEncryptionSystem <= 0x0BFF)
    strName.Format("%s (%X)", "Conax", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x0D00 &&
           m_iClientEncryptionSystem <= 0x0DFF)
    strName.Format("%s (%X)", "CryptoWorks", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x0E00 &&
           m_iClientEncryptionSystem <= 0x0EFF)
    strName.Format("%s (%X)", "PowerVu", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x1000)
    strName.Format("%s (%X)", "RAS", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x1200 &&
           m_iClientEncryptionSystem <= 0x12FF)
    strName.Format("%s (%X)", "NagraVision", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x1700 &&
           m_iClientEncryptionSystem <= 0x17FF)
    strName.Format("%s (%X)", "BetaCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x1800 &&
           m_iClientEncryptionSystem <= 0x18FF)
    strName.Format("%s (%X)", "NagraVision", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x22F0)
    strName.Format("%s (%X)", "Codicrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x2600)
    strName.Format("%s (%X)", "BISS", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4347)
    strName.Format("%s (%X)", "CryptOn", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4800)
    strName.Format("%s (%X)", "Accessgate", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4900)
    strName.Format("%s (%X)", "China Crypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A10)
    strName.Format("%s (%X)", "EasyCas", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A20)
    strName.Format("%s (%X)", "AlphaCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A70)
    strName.Format("%s (%X)", "DreamCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A60)
    strName.Format("%s (%X)", "SkyCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A61)
    strName.Format("%s (%X)", "Neotioncrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A62)
    strName.Format("%s (%X)", "SkyCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A63)
    strName.Format("%s (%X)", "Neotion SHL", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x4A64 &&
           m_iClientEncryptionSystem <= 0x4A6F)
    strName.Format("%s (%X)", "SkyCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4A80)
    strName.Format("%s (%X)", "ThalesCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4AA1)
    strName.Format("%s (%X)", "KeyFly", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4ABF)
    strName.Format("%s (%X)", "DG-Crypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem >= 0x4AD0 &&
           m_iClientEncryptionSystem <= 0x4AD1)
    strName.Format("%s (%X)", "X-Crypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4AD4)
    strName.Format("%s (%X)", "OmniCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x4AE0)
    strName.Format("%s (%X)", "RossCrypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x5500)
    strName.Format("%s (%X)", "Z-Crypt", m_iClientEncryptionSystem);
  else if (m_iClientEncryptionSystem == 0x5501)
    strName.Format("%s (%X)", "Griffin", m_iClientEncryptionSystem);
  else
    strName.Format("%s (%X)", g_localizeStrings.Get(19499).c_str(), m_iClientEncryptionSystem); /* Unknown */

  return strName;
}
