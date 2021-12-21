/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMConnector.h"

#include "utils/log.h"

#include <map>

using namespace KODI::WINDOWING::GBM;

using namespace std::chrono_literals;

namespace
{

std::map<int, std::string> connectorTypeNames = {
    {DRM_MODE_CONNECTOR_Unknown, "unknown"},
    {DRM_MODE_CONNECTOR_VGA, "VGA"},
    {DRM_MODE_CONNECTOR_DVII, "DVI-I"},
    {DRM_MODE_CONNECTOR_DVID, "DVI-D"},
    {DRM_MODE_CONNECTOR_DVIA, "DVI-A"},
    {DRM_MODE_CONNECTOR_Composite, "composite"},
    {DRM_MODE_CONNECTOR_SVIDEO, "s-video"},
    {DRM_MODE_CONNECTOR_LVDS, "LVDS"},
    {DRM_MODE_CONNECTOR_Component, "component"},
    {DRM_MODE_CONNECTOR_9PinDIN, "9-pin DIN"},
    {DRM_MODE_CONNECTOR_DisplayPort, "DP"},
    {DRM_MODE_CONNECTOR_HDMIA, "HDMI-A"},
    {DRM_MODE_CONNECTOR_HDMIB, "HDMI-B"},
    {DRM_MODE_CONNECTOR_TV, "TV"},
    {DRM_MODE_CONNECTOR_eDP, "eDP"},
    {DRM_MODE_CONNECTOR_VIRTUAL, "Virtual"},
    {DRM_MODE_CONNECTOR_DSI, "DSI"},
    {DRM_MODE_CONNECTOR_DPI, "DPI"},
};

std::map<drmModeConnection, std::string> connectorStatusNames = {
    {DRM_MODE_CONNECTED, "connected"},
    {DRM_MODE_DISCONNECTED, "disconnected"},
    {DRM_MODE_UNKNOWNCONNECTION, "unknown"},
};

} // namespace

CDRMConnector::CDRMConnector(int fd, uint32_t connector)
  : CDRMObject(fd), m_connector(drmModeGetConnector(m_fd, connector))
{
  if (!m_connector)
    throw std::runtime_error("drmModeGetConnector failed: " + std::string{strerror(errno)});

  if (!GetProperties(m_connector->connector_id, DRM_MODE_OBJECT_CONNECTOR))
    throw std::runtime_error("failed to get properties for connector: " +
                             std::to_string(m_connector->connector_id));
}

bool CDRMConnector::CheckConnector()
{
  unsigned retryCnt = 7;
  while (!IsConnected() && retryCnt > 0)
  {
    CLog::Log(LOGDEBUG, "CDRMConnector::{} - connector is disconnected", __FUNCTION__);
    retryCnt--;
    KODI::TIME::Sleep(1s);

    m_connector.reset(drmModeGetConnector(m_fd, m_connector->connector_id));
  }

  return m_connector->connection == DRM_MODE_CONNECTED;
}

std::string CDRMConnector::GetType()
{
  auto typeName = connectorTypeNames.find(m_connector->connector_type);
  if (typeName == connectorTypeNames.end())
    return connectorTypeNames[DRM_MODE_CONNECTOR_Unknown];

  return typeName->second;
}

std::string CDRMConnector::GetStatus()
{
  auto statusName = connectorStatusNames.find(m_connector->connection);
  if (statusName == connectorStatusNames.end())
    return connectorStatusNames[DRM_MODE_UNKNOWNCONNECTION];

  return statusName->second;
}

std::string CDRMConnector::GetName()
{
  return GetType() + "-" + std::to_string(m_connector->connector_type_id);
}
