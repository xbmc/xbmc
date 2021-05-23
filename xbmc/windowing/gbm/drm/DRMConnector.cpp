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

std::tuple<bool, std::vector<uint8_t>> CDRMConnector::GetEDID() const
{
  std::vector<uint8_t> edid;

  auto property = std::find_if(m_propsInfo.begin(), m_propsInfo.end(),
                               [](auto& prop) { return prop->name == std::string("EDID"); });

  if (property == m_propsInfo.end())
  {
    CLog::Log(LOGDEBUG, "CDRMConnector::{} - failed to find EDID property for connector: {}",
              __FUNCTION__, m_connector->connector_id);
    return std::make_tuple(false, edid);
  }

  uint64_t blob_id = m_props->prop_values[std::distance(m_propsInfo.begin(), property)];
  if (blob_id == 0)
    return std::make_tuple(false, edid);

  drmModePropertyBlobPtr blob = drmModeGetPropertyBlob(m_fd, blob_id);
  if (!blob)
    return std::make_tuple(false, edid);

  auto data = static_cast<uint8_t*>(blob->data);
  uint32_t length = blob->length;

  edid = std::vector<uint8_t>(data, data + length);

  drmModeFreePropertyBlob(blob);

  return std::make_tuple(true, edid);
}
