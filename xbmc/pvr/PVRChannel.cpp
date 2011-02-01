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
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "Util.h"
#include "filesystem/File.h"
#include "music/tags/MusicInfoTag.h"

#include "PVRChannelGroupsContainer.h"
#include "PVREpgContainer.h"
#include "PVREpg.h"
#include "PVREpgInfoTag.h"
#include "PVRDatabase.h"
#include "PVRManager.h"

using namespace XFILE;
using namespace MUSIC_INFO;

const CPVREpgInfoTag *m_EmptyEpgInfoTag = new CPVREpgInfoTag();

bool CPVRChannel::operator==(const CPVRChannel& right) const
{
  if (this == &right) return true;

  return (m_iChannelId              == right.m_iChannelId &&
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
  m_iChannelId              = -1;
  m_iChannelNumber          = -1;
  m_iChannelGroupId         = -1;
  m_bIsRadio                = false;
  m_bIsHidden               = false;
  m_bClientIsRecording      = false;
  m_strIconPath             = "";
  m_strChannelName          = "";
  m_bIsVirtual              = false;

  m_EPG                     = NULL;
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

bool CPVRChannel::Delete(void)
{
  bool bReturn = false;
  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  if (!database || !database->Open())
    return bReturn;

  /* delete the EPG table */
  if (m_EPG)
  {
    m_EPG->Delete();
    delete m_EPG;
  }

  bReturn = database->Delete(*this);

  database->Close();

  return bReturn;
}

bool CPVRChannel::UpdateFromClient(const CPVRChannel &channel)
{
  bool bChanged = false;

  bChanged = SetClientID(channel.ClientID()) || bChanged;
  bChanged = SetClientChannelNumber(channel.ClientChannelNumber()) || bChanged;
  bChanged = SetClientChannelName(channel.ClientChannelName()) || bChanged;
  bChanged = SetInputFormat(channel.InputFormat()) || bChanged;
  bChanged = SetStreamURL(channel.StreamURL()) || bChanged;
  bChanged = SetEncryptionSystem(channel.EncryptionSystem()) || bChanged;

  return bChanged;
}

bool CPVRChannel::Persist(bool bQueueWrite /* = false */)
{
  CPVRDatabase *database = g_PVRManager.GetTVDatabase();
  if (database)
  {
    database->Open();
    database->Persist(*this, bQueueWrite);
    database->Close();

    return true;
  }

  return false;
}

bool CPVRChannel::SetChannelID(int iChannelId, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_iChannelId != iChannelId)
  {
    /* update the id */
    m_iChannelId = iChannelId;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannel::SetChannelNumber(int iChannelNumber, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_iChannelNumber != iChannelNumber)
  {
    /* update the channel number */
    m_iChannelNumber = iChannelNumber;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  UpdatePath();

  return bReturn;
}

bool CPVRChannel::SetGroupID(int iChannelGroupId, bool bSaveInDb /* = false */)
{
  bool bRemoveFromOldGroup = true; // TODO support multiple groups and make this a parameter
  bool bReturn = false;

  if (m_iChannelGroupId != iChannelGroupId)
  {
    const CPVRChannelGroups *groups = g_PVRChannelGroups.Get(IsRadio());

    if (bRemoveFromOldGroup)
    {
      CPVRChannelGroup *oldGroup = (CPVRChannelGroup *) groups->GetGroupById(m_iChannelGroupId);
      if (oldGroup)
        oldGroup->RemoveFromGroup(this);
    }

    CPVRChannelGroup *newGroup = (CPVRChannelGroup *) groups->GetGroupById(iChannelGroupId);
    if (newGroup)
      newGroup->AddToGroup(this);

    /* update the group id */
    m_iChannelGroupId = iChannelGroupId;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannel::SetRadio(bool bIsRadio, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_bIsRadio != bIsRadio)
  {
    /* update the radio flag */
    m_bIsRadio = bIsRadio;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  UpdatePath();

  return bReturn;
}

bool CPVRChannel::SetHidden(bool bIsHidden, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_bIsHidden != bIsHidden)
  {
    /* update the hidden flag */
    m_bIsHidden = bIsHidden;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannel::SetRecording(bool bClientIsRecording)
{
  bool bReturn = false;

  if (m_bClientIsRecording != bClientIsRecording)
  {
    /* update the recording false */
    m_bClientIsRecording = bClientIsRecording;
    SetChanged();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannel::SetIconPath(const CStdString &strIconPath, bool bSaveInDb /* = false */)
{
  bool bReturn = true; // different from the behaviour of the rest of this class

  /* check if the path is valid */
  if (!CFile::Exists(strIconPath))
    return false;

  if (m_strIconPath != strIconPath)
  {
    /* update the path */
    m_strIconPath = CStdString(strIconPath);
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannel::SetChannelName(const CStdString &strChannelName, bool bSaveInDb /* = false */)
{
  bool bReturn = false;
  CStdString strName(strChannelName);

  if (strName.IsEmpty())
  {
    strName.Format(g_localizeStrings.Get(19085), ClientChannelNumber());
  }

  if (m_strChannelName != strName)
  {
    /* update the channel name */
    m_strChannelName = strName;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannel::SetVirtual(bool bIsVirtual, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_bIsVirtual != bIsVirtual)
  {
    /* update the virtual flag */
    m_bIsVirtual = bIsVirtual;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannel::IsEmpty() const
{
  return (m_strFileNameAndPath.IsEmpty() ||
          m_strStreamURL.IsEmpty());
}

/********** Client related channel methods **********/

bool CPVRChannel::SetUniqueID(int iUniqueId, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_iUniqueId != iUniqueId)
  {
    /* update the unique ID */
    m_iUniqueId = iUniqueId;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannel::SetClientID(int iClientId, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_iClientId != iClientId)
  {
    /* update the client ID */
    m_iClientId = iClientId;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannel::SetClientChannelNumber(int iClientChannelNumber, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_iClientChannelNumber != iClientChannelNumber)
  {
    /* update the client channel number */
    m_iClientChannelNumber = iClientChannelNumber;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannel::SetClientChannelName(const CStdString &strClientChannelName, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_strClientChannelName != strClientChannelName)
  {
    /* update the client channel name */
    m_strClientChannelName = CStdString(strClientChannelName);
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannel::SetInputFormat(const CStdString &strInputFormat, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_strInputFormat != strInputFormat)
  {
    /* update the input format */
    m_strInputFormat = CStdString(strInputFormat);
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannel::SetStreamURL(const CStdString &strStreamURL, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_strStreamURL != strStreamURL)
  {
    /* update the stream url */
    m_strStreamURL = CStdString(strStreamURL);
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

void CPVRChannel::UpdatePath(void)
{
  CStdString strFileNameAndPath;
  strFileNameAndPath.Format("pvr://channels/%s/all/%i.pvr", (m_bIsRadio ? "radio" : "tv"), m_iChannelNumber);
  if (m_strFileNameAndPath != strFileNameAndPath)
  {
    m_strFileNameAndPath = strFileNameAndPath;
    SetChanged();
  }
}

bool CPVRChannel::SetEncryptionSystem(int iClientEncryptionSystem, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_iClientEncryptionSystem != iClientEncryptionSystem)
  {
    /* update the client encryption system */
    m_iClientEncryptionSystem = iClientEncryptionSystem;
    UpdateEncryptionName();
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    bReturn = true;
  }

  return bReturn;
}

void CPVRChannel::UpdateEncryptionName(void)
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

  m_strClientEncryptionName = strName;
}

/********** EPG methods **********/

CPVREpg *CPVRChannel::GetEPG(void)
{
  if (m_EPG == NULL)
  {
    /* will be cleaned up by CPVREpgContainer on exit */
    m_EPG = new CPVREpg(this);
    m_EPG->Persist();
    g_PVREpgContainer.push_back(m_EPG);
  }

  return m_EPG;
}

int CPVRChannel::GetEPG(CFileItemList *results)
{
  CPVREpg *epg = GetEPG();
  if (!epg)
  {
    CLog::Log(LOGERROR, "PVR - %s - cannot get EPG for channel '%s'",
        __FUNCTION__, m_strChannelName.c_str());
    return -1;
  }

  return epg->Get(results);
}

bool CPVRChannel::ClearEPG()
{
  if (m_EPG)
    GetEPG()->Clear();

  return true;
}

const CPVREpgInfoTag* CPVRChannel::GetEPGNow(void) const
{
  const CPVREpgInfoTag *tag = NULL;

  if (!m_bIsHidden && m_bEPGEnabled && m_EPG)
    tag = (CPVREpgInfoTag *) m_EPG->InfoTagNow();

  return !tag ? m_EmptyEpgInfoTag : tag;
}

const CPVREpgInfoTag* CPVRChannel::GetEPGNext(void) const
{
  const CPVREpgInfoTag *tag = NULL;

  if (!m_bIsHidden && m_bEPGEnabled && m_EPG)
    tag = (CPVREpgInfoTag *) m_EPG->InfoTagNext();

  return !tag ? m_EmptyEpgInfoTag : tag;
}

bool CPVRChannel::SetEPGEnabled(bool bEPGEnabled /* = true */, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_bEPGEnabled != bEPGEnabled)
  {
    /* update the EPG flag */
    m_bEPGEnabled = bEPGEnabled;
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    /* clear the previous EPG entries if needed */
    if (!m_bEPGEnabled && m_EPG)
      m_EPG->Clear();

    bReturn = true;
  }

  return bReturn;
}

bool CPVRChannel::SetEPGScraper(const CStdString &strScraper, bool bSaveInDb /* = false */)
{
  bool bReturn = false;

  if (m_strEPGScraper != strScraper)
  {
    bool bCleanEPG = !m_strEPGScraper.IsEmpty() || strScraper.IsEmpty();

    /* update the scraper name */
    m_strEPGScraper = CStdString(strScraper);
    SetChanged();

    /* persist the changes */
    if (bSaveInDb)
      Persist();

    /* clear the previous EPG entries if needed */
    if (bCleanEPG && m_bEPGEnabled && m_EPG)
      m_EPG->Clear();

    bReturn = true;
  }

  return bReturn;
}
