/*
 *      Copyright (C) 2015-2016 Team KODI
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

#include "AddonInterfaceManager.h"

#include "Application.h"
#include "utils/log.h"

#include <netinet/in.h>

#define LOCK_AND_COPY(type, dest, src) \
  if (!m_bInitialized) return; \
  CSingleLock lock(src); \
  src.hadSomethingRemoved = false; \
  type dest; \
  dest = src

#define CHECK_FOR_ENTRY(l,v) \
  (l.hadSomethingRemoved ? (std::find(l.begin(),l.end(),v) != l.end()) : true)

namespace ADDON
{

CAddonInterfaceManager::CAddonInterfaceManager()
 : m_bInitialized(false)
{
}

CAddonInterfaceManager::~CAddonInterfaceManager()
{
}

bool CAddonInterfaceManager::StartManager()
{
  m_bInitialized = true;
  return true;
}

void CAddonInterfaceManager::StopManager()
{
  m_bInitialized = false;
}

} /* namespace ADDON */
