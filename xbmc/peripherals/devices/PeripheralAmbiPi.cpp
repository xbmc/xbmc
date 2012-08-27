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

#include "PeripheralAmbiPi.h"
#include "utils/log.h"

#define AMBIPI_DEFAULT_PORT 20434

using namespace PERIPHERALS;

CPeripheralAmbiPi::CPeripheralAmbiPi(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId) :
  CPeripheral(type, busType, strLocation, strDeviceName, iVendorId, iProductId),
  m_bStarted(false)
{
  m_features.push_back(FEATURE_AMBIPI);
}

bool CPeripheralAmbiPi::InitialiseFeature(const PeripheralFeature feature)
{
  if (feature != FEATURE_AMBIPI || m_bStarted || !GetSettingBool("enabled")) {
    return CPeripheral::InitialiseFeature(feature);
  }

  CLog::Log(LOGDEBUG, "%s - Initialising AmbiPi on %s", __FUNCTION__, m_strLocation.c_str());

  m_bStarted = true;

  LoadAddressFromConfiguration();

  // TODO connect ambi (TCP)
  ConnectToDevice();

  return CPeripheral::InitialiseFeature(feature);
}

void CPeripheralAmbiPi::ConnectToDevice()
{
  CLog::Log(LOGINFO, "%s - Connecting to AmbiPi on %s:%d", __FUNCTION__, m_address.c_str(), m_port);  
}

void CPeripheralAmbiPi::LoadAddressFromConfiguration()
{
  
  CStdString portFromConfiguration = GetSettingString("port");

  int port;

  if (sscanf(portFromConfiguration.c_str(), "%d", &port) &&
      port >= 0 &&
      port <= 65535)
    m_port = port;
  else
    m_port = AMBIPI_DEFAULT_PORT;

  m_address = GetSettingString("address");
}

CPeripheralAmbiPi::~CPeripheralAmbiPi(void)
{
  CLog::Log(LOGDEBUG, "%s - Removing AmbiPi on %s", __FUNCTION__, m_strLocation.c_str());
}
