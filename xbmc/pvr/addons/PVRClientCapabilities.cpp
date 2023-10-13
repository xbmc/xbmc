/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRClientCapabilities.h"

#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

#include <algorithm>
#include <iterator>
#include <memory>

using namespace PVR;

CPVRClientCapabilities::CPVRClientCapabilities(const CPVRClientCapabilities& other)
{
  if (other.m_addonCapabilities)
    m_addonCapabilities = std::make_unique<PVR_ADDON_CAPABILITIES>(*other.m_addonCapabilities);
  InitRecordingsLifetimeValues();
}

const CPVRClientCapabilities& CPVRClientCapabilities::operator=(const CPVRClientCapabilities& other)
{
  if (other.m_addonCapabilities)
    m_addonCapabilities = std::make_unique<PVR_ADDON_CAPABILITIES>(*other.m_addonCapabilities);
  InitRecordingsLifetimeValues();
  return *this;
}

const CPVRClientCapabilities& CPVRClientCapabilities::operator=(
    const PVR_ADDON_CAPABILITIES& addonCapabilities)
{
  m_addonCapabilities = std::make_unique<PVR_ADDON_CAPABILITIES>(addonCapabilities);
  InitRecordingsLifetimeValues();
  return *this;
}

void CPVRClientCapabilities::clear()
{
  m_recordingsLifetimeValues.clear();
  m_addonCapabilities.reset();
}

void CPVRClientCapabilities::InitRecordingsLifetimeValues()
{
  m_recordingsLifetimeValues.clear();
  if (m_addonCapabilities && m_addonCapabilities->iRecordingsLifetimesSize > 0)
  {
    for (unsigned int i = 0; i < m_addonCapabilities->iRecordingsLifetimesSize; ++i)
    {
      int iValue = m_addonCapabilities->recordingsLifetimeValues[i].iValue;
      std::string strDescr(m_addonCapabilities->recordingsLifetimeValues[i].strDescription);
      if (strDescr.empty())
      {
        // No description given by addon. Create one from value.
        strDescr = std::to_string(iValue);
      }
      m_recordingsLifetimeValues.emplace_back(strDescr, iValue);
    }
  }
  else if (SupportsRecordingsLifetimeChange())
  {
    // No values given by addon, but lifetime supported. Use default values 1..365
    for (int i = 1; i < 366; ++i)
    {
      m_recordingsLifetimeValues.emplace_back(StringUtils::Format(g_localizeStrings.Get(17999), i),
                                              i); // "{} days"
    }
  }
  else
  {
    // No lifetime supported.
  }
}

void CPVRClientCapabilities::GetRecordingsLifetimeValues(
    std::vector<std::pair<std::string, int>>& list) const
{
  std::copy(m_recordingsLifetimeValues.cbegin(), m_recordingsLifetimeValues.cend(),
            std::back_inserter(list));
}
