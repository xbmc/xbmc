/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PeripheralCecAdapter.h"
#include "utils/log.h"

using namespace PERIPHERALS;
using namespace ANNOUNCEMENT;

CPeripheralCecAdapter::CPeripheralCecAdapter(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId) :
  CPeripheral(type, busType, strLocation, strDeviceName, iVendorId, iProductId)
{
  m_features.push_back(FEATURE_CEC);
}

CPeripheralCecAdapter::~CPeripheralCecAdapter(void)
{
  CAnnouncementManager::RemoveAnnouncer(this);
}

void CPeripheralCecAdapter::Announce(EAnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (flag == System && !strcmp(sender, "xbmc") && !strcmp(message, "ApplicationStop"))
  {
    ScreenSetPower(false);
  }
}

bool CPeripheralCecAdapter::ScreenSetPower(bool bSetTo)
{
  // TODO
  return false;
}

bool CPeripheralCecAdapter::InitialiseFeature(const PeripheralFeature feature)
{
  if (feature == FEATURE_CEC)
    CAnnouncementManager::AddAnnouncer(this);

  return CPeripheral::InitialiseFeature(feature);
}
