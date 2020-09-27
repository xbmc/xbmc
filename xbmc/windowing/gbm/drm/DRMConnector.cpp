/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMConnector.h"

#include "utils/log.h"

using namespace KODI::WINDOWING::GBM;

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
    KODI::TIME::Sleep(1000);

    m_connector.reset(drmModeGetConnector(m_fd, m_connector->connector_id));
  }

  return m_connector->connection == DRM_MODE_CONNECTED;
}
