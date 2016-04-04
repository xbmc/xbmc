/*
 *      Copyright (C) 2010-2014 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ActiveAEDSPMode.h"
#include "ActiveAEDSPDatabase.h"
#include "ActiveAEDSP.h"
#include "utils/StringUtils.h"

using namespace ActiveAE;

bool CActiveAEDSPMode::operator==(const CActiveAEDSPMode &right) const
{
  return (m_iModeId           == right.m_iModeId &&
          m_iAddonId          == right.m_iAddonId &&
          m_iAddonModeNumber  == right.m_iAddonModeNumber &&
          m_iModeType         == right.m_iModeType &&
          m_iModePosition     == right.m_iModePosition);
}

bool CActiveAEDSPMode::operator!=(const CActiveAEDSPMode &right) const
{
  return !(*this == right);
}

CActiveAEDSPMode::CActiveAEDSPMode()
{
  m_iModeType               = AE_DSP_MODE_TYPE_UNDEFINED;
  m_iModeId                 = -1;
  m_iModePosition           = -1;
  m_bIsEnabled              = false;
  m_strOwnIconPath          = "";
  m_strOverrideIconPath     = "";
  m_iStreamTypeFlags        = 0;
  m_iBaseType               = AE_DSP_ABASE_INVALID;
  m_iModeName               = -1;
  m_iModeSetupName          = -1;
  m_iModeDescription        = -1;
  m_iModeHelp               = -1;
  m_bChanged                = false;
  m_bIsInternal             = false;
  m_bHasSettingsDialog      = false;

  m_fCPUUsage               = 0.0f;

  m_iAddonId                = -1;
  m_iAddonModeNumber        = -1;
  m_strModeName             = "";
}

CActiveAEDSPMode::CActiveAEDSPMode(int modeId, const AE_DSP_BASETYPE baseType)
{
  m_iModeType               = AE_DSP_MODE_TYPE_MASTER_PROCESS;
  m_iModeId                 = modeId;
  m_iModePosition           = 0;
  m_bIsEnabled              = true;
  m_strOwnIconPath          = "";
  m_strOverrideIconPath     = "";
  m_iStreamTypeFlags        = AE_DSP_PRSNT_ASTREAM_BASIC |
                              AE_DSP_PRSNT_ASTREAM_MUSIC |
                              AE_DSP_PRSNT_ASTREAM_MOVIE/* |
                              AE_DSP_PRSNT_ASTREAM_GAME |
                              AE_DSP_PRSNT_ASTREAM_APP |
                              AE_DSP_PRSNT_ASTREAM_PHONE |
                              AE_DSP_PRSNT_ASTREAM_MESSAGE*/;
  m_iBaseType               = baseType;

  switch (modeId)
  {
    case AE_DSP_MASTER_MODE_ID_PASSOVER:
      m_iModeName           = 16039;
      m_iModeDescription    = -1;
      m_iModeHelp           = -1;
      m_iModeSetupName      = -1;
      m_strModeName         = "Passover";
      break;
    case AE_DSP_MASTER_MODE_ID_INTERNAL_STEREO_UPMIX:
      m_iModeName           = 252;
      m_iModeDescription    = 36364;
      m_iModeHelp           = -1;
      m_iModeSetupName      = -1;
      m_strModeName         = "Stereo Upmix";
      break;
    default:
      m_iModeName           = 16039;
      m_iModeDescription    = -1;
      m_iModeHelp           = -1;
      m_iModeSetupName      = -1;
      m_strModeName         = "Unknown";
      break;
  };

  m_bChanged                = false;
  m_bIsInternal             = true;
  m_bHasSettingsDialog      = false;

  m_fCPUUsage               = 0.0f;

  m_iAddonId                = -1;
  m_iAddonModeNumber        = -1;
}

CActiveAEDSPMode::CActiveAEDSPMode(const AE_DSP_MODES::AE_DSP_MODE &mode, int iAddonId)
{
  m_iModeType               = mode.iModeType;
  m_iModePosition           = -1;
  m_iModeId                 = mode.iUniqueDBModeId;
  m_iAddonId                = iAddonId;
  m_iBaseType               = AE_DSP_ABASE_INVALID;
  m_bIsEnabled              = m_iModeType == AE_DSP_MODE_TYPE_MASTER_PROCESS ? !mode.bIsDisabled : false;
  m_strOwnIconPath          = mode.strOwnModeImage;
  m_strOverrideIconPath     = mode.strOverrideModeImage;
  m_iStreamTypeFlags        = mode.iModeSupportTypeFlags;
  m_iModeName               = mode.iModeName;
  m_iModeSetupName          = mode.iModeSetupName;
  m_iModeDescription        = mode.iModeDescription;
  m_iModeHelp               = mode.iModeHelp;
  m_iAddonModeNumber        = mode.iModeNumber;
  m_strModeName             = mode.strModeName;
  m_bHasSettingsDialog      = mode.bHasSettingsDialog;
  m_bChanged                = false;
  m_bIsInternal             = false;

  m_fCPUUsage               = 0.0f;

  if (m_strModeName.empty())
    m_strModeName = StringUtils::Format("%s %d", g_localizeStrings.Get(15023).c_str(), m_iModeId);
}

CActiveAEDSPMode::CActiveAEDSPMode(const CActiveAEDSPMode &mode)
{
  *this = mode;
}

CActiveAEDSPMode &CActiveAEDSPMode::operator=(const CActiveAEDSPMode &mode)
{
  m_iModeId                 = mode.m_iModeId;
  m_iModeType               = mode.m_iModeType;
  m_iModePosition           = mode.m_iModePosition;
  m_bIsEnabled              = mode.m_bIsEnabled;
  m_strOwnIconPath          = mode.m_strOwnIconPath;
  m_strOverrideIconPath     = mode.m_strOverrideIconPath;
  m_iStreamTypeFlags        = mode.m_iStreamTypeFlags;
  m_iBaseType               = mode.m_iBaseType;
  m_iModeName               = mode.m_iModeName;
  m_iModeSetupName          = mode.m_iModeSetupName;
  m_iModeDescription        = mode.m_iModeDescription;
  m_iModeHelp               = mode.m_iModeHelp;
  m_iAddonId                = mode.m_iAddonId;
  m_iAddonModeNumber        = mode.m_iAddonModeNumber;
  m_strModeName             = mode.m_strModeName;
  m_bChanged                = mode.m_bChanged;
  m_bIsInternal             = mode.m_bIsInternal;
  m_bHasSettingsDialog      = mode.m_bHasSettingsDialog;
  m_fCPUUsage               = mode.m_fCPUUsage;

  return *this;
}

/********** General mode related functions **********/

bool CActiveAEDSPMode::IsNew(void) const
{
  CSingleLock lock(m_critSection);
  return m_iModeId <= 0;
}

bool CActiveAEDSPMode::IsChanged(void) const
{
  CSingleLock lock(m_critSection);
  return m_bChanged;
}


bool CActiveAEDSPMode::IsInternal(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsInternal;
}

bool CActiveAEDSPMode::IsEnabled(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsEnabled;
}

bool CActiveAEDSPMode::SetEnabled(bool bIsEnabled)
{
  CSingleLock lock(m_critSection);

  if (m_bIsEnabled != bIsEnabled)
  {
    /* update the Enabled flag */
    m_bIsEnabled = bIsEnabled;
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

int CActiveAEDSPMode::ModePosition(void) const
{
  CSingleLock lock(m_critSection);
  return m_iModePosition;
}

bool CActiveAEDSPMode::SetModePosition(int iModePosition)
{
  CSingleLock lock(m_critSection);
  if (m_iModePosition != iModePosition)
  {
    /* update the type */
    m_iModePosition = iModePosition;
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

bool CActiveAEDSPMode::SupportStreamType(AE_DSP_STREAMTYPE streamType, unsigned int flags)
{
       if (streamType == AE_DSP_ASTREAM_BASIC   && (flags & AE_DSP_PRSNT_ASTREAM_BASIC))   return true;
  else if (streamType == AE_DSP_ASTREAM_MUSIC   && (flags & AE_DSP_PRSNT_ASTREAM_MUSIC))   return true;
  else if (streamType == AE_DSP_ASTREAM_MOVIE   && (flags & AE_DSP_PRSNT_ASTREAM_MOVIE))   return true;
  else if (streamType == AE_DSP_ASTREAM_GAME    && (flags & AE_DSP_PRSNT_ASTREAM_GAME))    return true;
  else if (streamType == AE_DSP_ASTREAM_APP     && (flags & AE_DSP_PRSNT_ASTREAM_APP))     return true;
  else if (streamType == AE_DSP_ASTREAM_PHONE   && (flags & AE_DSP_PRSNT_ASTREAM_PHONE))   return true;
  else if (streamType == AE_DSP_ASTREAM_MESSAGE && (flags & AE_DSP_PRSNT_ASTREAM_MESSAGE)) return true;
  return false;
}

bool CActiveAEDSPMode::SupportStreamType(AE_DSP_STREAMTYPE streamType) const
{
  return SupportStreamType(streamType, m_iStreamTypeFlags);
}

/********** Mode user interface related data functions **********/

int CActiveAEDSPMode::ModeName(void) const
{
  CSingleLock lock(m_critSection);
  return m_iModeName;
}

int CActiveAEDSPMode::ModeSetupName(void) const
{
  CSingleLock lock(m_critSection);
  return m_iModeSetupName;
}

int CActiveAEDSPMode::ModeDescription(void) const
{
  CSingleLock lock(m_critSection);
  return m_iModeDescription;
}

int CActiveAEDSPMode::ModeHelp(void) const
{
  CSingleLock lock(m_critSection);
  return m_iModeHelp;
}

const std::string &CActiveAEDSPMode::IconOwnModePath(void) const
{
  CSingleLock lock(m_critSection);
  return m_strOwnIconPath;
}

const std::string &CActiveAEDSPMode::IconOverrideModePath(void) const
{
  CSingleLock lock(m_critSection);
  return m_strOverrideIconPath;
}


/********** Master mode type related functions **********/

bool CActiveAEDSPMode::SetBaseType(AE_DSP_BASETYPE baseType)
{
  CSingleLock lock(m_critSection);
  if (m_iBaseType != baseType)
  {
    /* update the mode base */
    m_iBaseType = baseType;
    SetChanged();
    m_bChanged = true;

    return true;
  }

  return false;
}

AE_DSP_BASETYPE CActiveAEDSPMode::BaseType(void) const
{
  CSingleLock lock(m_critSection);
  return m_iBaseType;
}


/********** Audio DSP database related functions **********/

int CActiveAEDSPMode::ModeID(void) const
{
  CSingleLock lock(m_critSection);
  return m_iModeId;
}

int CActiveAEDSPMode::AddUpdate(bool force)
{
  if (!force)
  {
    // not changed
    CSingleLock lock(m_critSection);
    if (!m_bChanged && m_iModeId > 0)
      return m_iModeId;
  }

  CActiveAEDSPDatabase *database = CServiceBroker::GetADSP().GetADSPDatabase();
  if (!database || !database->IsOpen())
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - failed to open the database");
    return -1;
  }

  database->AddUpdateMode(*this);
  m_iModeId = database->GetModeId(*this);

  return m_iModeId;
}

bool CActiveAEDSPMode::Delete(void)
{
  CActiveAEDSPDatabase *database = CServiceBroker::GetADSP().GetADSPDatabase();
  if (!database || !database->IsOpen())
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - failed to open the database");
    return false;
  }

  return database->DeleteMode(*this);
}

bool CActiveAEDSPMode::IsKnown(void) const
{
  CActiveAEDSPDatabase *database = CServiceBroker::GetADSP().GetADSPDatabase();
  if (!database || !database->IsOpen())
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - failed to open the database");
    return false;
  }

  return database->GetModeId(*this) > 0;
}


/********** Dynamic processing related data methods **********/

void CActiveAEDSPMode::SetCPUUsage(float percent)
{
  CSingleLock lock(m_critSection);
  m_fCPUUsage = percent;
}

float CActiveAEDSPMode::CPUUsage(void) const
{
  CSingleLock lock(m_critSection);
  return m_fCPUUsage;
}


/********** Fixed addon related Mode methods **********/

int CActiveAEDSPMode::AddonID(void) const
{
  CSingleLock lock(m_critSection);
  return m_iAddonId;
}

unsigned int CActiveAEDSPMode::AddonModeNumber(void) const
{
  CSingleLock lock(m_critSection);
  return m_iAddonModeNumber;
}

AE_DSP_MODE_TYPE CActiveAEDSPMode::ModeType(void) const
{
  CSingleLock lock(m_critSection);
  return m_iModeType;
}

const std::string &CActiveAEDSPMode::AddonModeName(void) const
{
  CSingleLock lock(m_critSection);
  return m_strModeName;
}

bool CActiveAEDSPMode::HasSettingsDialog(void) const
{
  CSingleLock lock(m_critSection);
  return m_bHasSettingsDialog;
}

unsigned int CActiveAEDSPMode::StreamTypeFlags(void) const
{
  CSingleLock lock(m_critSection);
  return m_iStreamTypeFlags;
}
