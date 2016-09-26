/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "Platform.h"
#include "Application.h"
#include "network/Network.h"
#include "utils/log.h"
#include "utils/md5.h"

const std::string CPlatform::NoValidUUID = "NOUUID";

// Override for platform ports
#if !defined(PLATFORM_OVERRIDE_CLASSPLATFORM)

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatform();
}

#endif

// base class definitions

CPlatform::CPlatform()
{
  m_uuid = NoValidUUID;
}

void CPlatform::Init()
{
  InitUniqueHardwareIdentifier();
}

void CPlatform::InitUniqueHardwareIdentifier()
{
  CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
  if (iface)
  {
    m_uuid = iface->GetMacAddress();
    m_uuid = XBMC::XBMC_MD5::GetMD5(m_uuid);
  }
}

std::string CPlatform::GetUniqueHardwareIdentifier()
{
  return m_uuid;
}

