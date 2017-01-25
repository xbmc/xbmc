/*
 *      Copyright (C) 2015 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AddonEvent.h"

CAddonEvent::CAddonEvent(ADDON::AddonInfoPtr addonInfo, const CVariant& description)
  : CUniqueEvent(addonInfo->Name(), description, addonInfo->Icon()),
    m_addonInfo(addonInfo)
{ }

CAddonEvent::CAddonEvent(ADDON::AddonInfoPtr addonInfo, const CVariant& description, const CVariant& details)
  : CUniqueEvent(addonInfo->Name(), description, addonInfo->Icon(), details),
  m_addonInfo(addonInfo)
{ }

CAddonEvent::CAddonEvent(ADDON::AddonInfoPtr addonInfo, const CVariant& description, const CVariant& details, const CVariant& executionLabel)
  : CUniqueEvent(addonInfo->Name(), description, addonInfo->Icon(), details, executionLabel),
  m_addonInfo(addonInfo)
{ }

CAddonEvent::CAddonEvent(ADDON::AddonInfoPtr addonInfo, EventLevel level, const CVariant& description)
  : CUniqueEvent(addonInfo->Name(), description, addonInfo->Icon(), level),
  m_addonInfo(addonInfo)
{ }

CAddonEvent::CAddonEvent(ADDON::AddonInfoPtr addonInfo, EventLevel level, const CVariant& description, const CVariant& details)
  : CUniqueEvent(addonInfo->Name(), description, addonInfo->Icon(), details, level),
  m_addonInfo(addonInfo)
{ }

CAddonEvent::CAddonEvent(ADDON::AddonInfoPtr addonInfo, EventLevel level, const CVariant& description, const CVariant& details, const CVariant& executionLabel)
  : CUniqueEvent(addonInfo->Name(), description, addonInfo->Icon(), details, executionLabel, level),
  m_addonInfo(addonInfo)
{ }
