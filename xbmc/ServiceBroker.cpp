/*
 *      Copyright (C) 2005-2016 Team XBMC
 *      http://xbmc.org
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

#include "ServiceBroker.h"
#include "Application.h"

ADDON::CAddonMgr &CServiceBroker::GetAddonMgr()
{
  return g_application.m_ServiceManager->GetAddonMgr();
}

ADDON::CBinaryAddonCache &CServiceBroker::GetBinaryAddonCache()
{
  return g_application.m_ServiceManager->GetBinaryAddonCache();
}

ANNOUNCEMENT::CAnnouncementManager &CServiceBroker::GetAnnouncementManager()
{
  return g_application.m_ServiceManager->GetAnnouncementManager();
}

XBPython& CServiceBroker::GetXBPython()
{
  return g_application.m_ServiceManager->GetXBPython();
}

PVR::CPVRManager &CServiceBroker::GetPVRManager()
{
  return g_application.m_ServiceManager->GetPVRManager();
}

ActiveAE::CActiveAEDSP &CServiceBroker::GetADSP()
{
  return g_application.m_ServiceManager->GetADSPManager();
}

CContextMenuManager& CServiceBroker::GetContextMenuManager()
{
  return g_application.m_ServiceManager->GetContextMenuManager();
}

CDataCacheCore &CServiceBroker::GetDataCacheCore()
{
  return g_application.m_ServiceManager->GetDataCacheCore();
}
