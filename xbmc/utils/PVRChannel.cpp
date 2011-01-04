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
          m_iChannelNumber          == right.m_iChannelNumber &&
          m_iChannelGroupId         == right.m_iChannelGroupId &&
          m_bIsRadio                == right.m_bIsRadio &&
          m_bIsHidden               == right.m_bIsHidden &&
          m_bClientIsRecording      == right.m_bClientIsRecording &&
          m_strIconPath             == right.m_strIconPath &&
          m_strChannelName          == right.m_strChannelName &&
          m_bIsVirtual              == right.m_bIsVirtual &&

          m_iUniqueId               == right.m_iUniqueId &&
          m_iClientId               == right.m_iClientId &&
          m_iClientChannelNumber    == right.m_iClientChannelNumber &&
          m_strClientChannelName    == right.m_strClientChannelName &&
          m_strStreamURL            == right.m_strStreamURL &&
          m_strFileNameAndPath      == right.m_strFileNameAndPath &&
          m_iClientEncryptionSystem == right.m_iClientEncryptionSystem);
}

bool CPVRChannel::operator!=(const CPVRChannel &right) const
{
  return !(*this == right);
}

CPVRChannel::CPVRChannel()
{
  m_iDatabaseId             = -1;
  m_iChannelNumber          = -1;
  m_iChannelGroupId         = -1;
  m_bIsRadio                = false;
  m_bIsHidden               = false;
  m_bClientIsRecording      = false;
  m_strIconPath             = "";
  m_strChannelName          = "";
  m_bIsVirtual              = false;

  m_EPG                     = NULL;
  m_EPGNow                  = NULL;
  m_bEPGEnabled             = true;
  m_strEPGScraper           = "client";

  m_iUniqueId               = -1;
  m_iClientId               = -1;
  m_iClientChannelNumber    = -1;
  m_strClientChannelName    = "";
  m_strInputFormat          = "";
  m_strStreamURL            = "";
  m_strFileNameAndPath      = "";
  m_iClientEncryptionSystem = -1;
}

/********** XBMC related channel methods **********/

void CPVRChannel::SetChannelID(long iDatabaseId)
{
  m_iDatabaseId = iDatabaseId;
  SetChanged();
}

void CPVRChannel::SetChannelNumber(int iChannelNumber)
{
  m_iChannelNumber = iChannelNumber;
  SetChanged();
}

void CPVRChannel::SetGroupID(int iChannelGroupId)
{
  m_iChannelGroupId = iChannelGroupId;
  SetChanged();
}

void CPVRChannel::SetRadio(bool bIsRadio)
{
  m_bIsRadio = bIsRadio;
  SetChanged();
}

void CPVRChannel::SetHidden(bool bIsHidden)
{
  m_bIsHidden = bIsHidden;
  SetChanged();
}

void CPVRChannel::SetRecording(bool bClientIsRecording)
{
  m_bClientIsRecording = bClientIsRecording;
  SetChanged();
}

void CPVRChannel::SetIconPath(CStdString strIconPath)
{
  m_strIconPath = strIconPath;
  SetChanged();
}

void CPVRChannel::SetChannelName(CStdString strChannelName)
{
  m_strChannelName = strChannelName;
  SetChanged();
}

void CPVRChannel::SetVirtual(bool bIsVirtual)
{
  m_bIsVirtual = bIsVirtual;
  SetChanged();
}

bool CPVRChannel::IsEmpty() const
{
  return (m_strFileNameAndPath.IsEmpty() ||
          m_strStreamURL.IsEmpty());
}

/********** Client related channel methods **********/

void CPVRChannel::SetUniqueID(int iUniqueId)
{
  m_iUniqueId = iUniqueId;
  SetChanged();
}

void CPVRChannel::SetClientID(int iClientId)
{
  m_iClientId = iClientId;
  SetChanged();
}

void CPVRChannel::SetClientNumber(int iClientChannelNumber)
{
  m_iClientChannelNumber = iClientChannelNumber;
  SetChanged();
}

void CPVRChannel::SetClientChannelName(CStdString strClientChannelName)
{
  m_strClientChannelName = strClientChannelName;
  SetChanged();
}

void CPVRChannel::SetInputFormat(CStdString strInputFormat)
{
  m_strInputFormat = strInputFormat;
  SetChanged();
}

void CPVRChannel::SetStreamURL(CStdString strStreamURL)
{
  m_strStreamURL = strStreamURL;
  SetChanged();
}

void CPVRChannel::SetPath(CStdString strFileNameAndPath)
{
  m_strFileNameAndPath = strFileNameAndPath;
  SetChanged();
}

void CPVRChannel::SetEncryptionSystem(int iClientEncryptionSystem)
{
  m_iClientEncryptionSystem = iClientEncryptionSystem;
  SetChanged();
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

/********** EPG methods **********/

CPVREpg *CPVRChannel::GetEPG(void)
{
  if (m_EPG == NULL)
  {
    /* will be cleaned up by CPVREpgs on exit */
    m_EPG = new CPVREpg(this);
    PVREpgs.push_back(m_EPG);
  }

  return m_EPG;
}

bool CPVRChannel::ClearEPG()
{
  if (m_EPG != NULL)
  {
    GetEPG()->Clear();
    m_EPGNow = NULL;
  }

  return true;
}

const CPVREpgInfoTag* CPVRChannel::GetEPGNow(void) const
{
  if (m_bIsHidden || !m_bEPGEnabled || m_EPGNow == NULL)
    return m_EmptyEpgInfoTag;

  return m_EPGNow;
}

const CPVREpgInfoTag* CPVRChannel::GetEPGNext(void) const
{
  if (m_bIsHidden || !m_bEPGEnabled || m_EPGNow == NULL)
    return m_EmptyEpgInfoTag;

  const CPVREpgInfoTag *nextTag = m_EPGNow->GetNextEvent();
  return nextTag == NULL ?
      m_EmptyEpgInfoTag :
      nextTag;
}

void CPVRChannel::SetEPGEnabled(bool EPGEnabled /* = true */)
{
  m_bEPGEnabled = EPGEnabled;
  SetChanged();
}

void CPVRChannel::SetEPGScraper(CStdString Grabber)
{
  m_strEPGScraper = Grabber;
  SetChanged();
}

void CPVRChannel::UpdateEPGPointers(void)
{
  if (m_bIsHidden || !m_bEPGEnabled)
    return;

  CPVREpg *epg = GetEPG();

  if (!epg->IsUpdateRunning() &&
      (m_EPGNow == NULL ||
       m_EPGNow->End() <= CDateTime::GetCurrentDateTime()))
  {
    SetChanged();
    m_EPGNow  = epg->InfoTagNow();
    if (m_EPGNow)
    {
      CLog::Log(LOGDEBUG, "%s - EPG now pointer for channel '%s' updated to '%s'",
          __FUNCTION__, m_strChannelName.c_str(), m_EPGNow->Title().c_str());
    }
    else
    {
      CLog::Log(LOGDEBUG, "%s - no EPG now pointer for channel '%s'",
          __FUNCTION__, m_strChannelName.c_str());
    }
  }

  NotifyObservers("epg");
}
