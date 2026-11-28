/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMConnector.h"

#include "utils/XTimeUtils.h"
#include "utils/log.h"

#include <algorithm>
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
    CLog::LogF(LOGDEBUG, "connector is disconnected");
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

namespace
{
constexpr size_t EDID_BLOCK_SIZE = 128;
constexpr uint8_t CTA_EXTENSION_TAG = 0x02;
constexpr uint8_t CTA_DB_VENDOR_SPECIFIC = 3;
constexpr uint8_t CTA_DB_EXTENDED = 7;
constexpr uint8_t CTA_EXT_DB_HF_SCDB = 0x79;

int ParseMaxFrlRate(const std::vector<uint8_t>& edid)
{
  for (size_t block = EDID_BLOCK_SIZE; block + EDID_BLOCK_SIZE <= edid.size();
       block += EDID_BLOCK_SIZE)
  {
    const uint8_t* ext = &edid[block];
    if (ext[0] != CTA_EXTENSION_TAG)
      continue;

    // the data blocks run from byte 4 up to the first detailed timing descriptor, 0 means none
    const size_t end = std::min<size_t>(ext[2], EDID_BLOCK_SIZE);
    for (size_t pos = 4; pos < end;)
    {
      const uint8_t tag = ext[pos] >> 5;
      const size_t length = ext[pos] & 0x1f;
      if (pos + 1 + length > end)
        break;

      const uint8_t* db = &ext[pos];

      // The HDMI Forum vendor block (OUI C4-5D-D8) and the HF-SCDB extended block share their
      // layout: Max_FRL_Rate is the upper four bits of byte 7, as in the kernel.
      const bool forumVendor = tag == CTA_DB_VENDOR_SPECIFIC && length >= 7 && db[1] == 0xd8 &&
                               db[2] == 0x5d && db[3] == 0xc4;
      const bool forumExtended =
          tag == CTA_DB_EXTENDED && length >= 7 && db[1] == CTA_EXT_DB_HF_SCDB;

      if (forumVendor || forumExtended)
        return (db[7] & 0xf0) >> 4;

      pos += 1 + length;
    }
  }

  return 0;
}
} // namespace

std::vector<uint8_t> CDRMConnector::GetEDID() const
{

  uint64_t blob_id = GetPropertyValue("EDID").value_or(0);

  if (blob_id == 0)
  {
    CLog::LogF(LOGDEBUG, "failed to find EDID property for connector: {}",
               m_connector->connector_id);
    return {};
  }

  drmModePropertyBlobPtr blob = drmModeGetPropertyBlob(m_fd, blob_id);
  if (!blob)
    return {};

  auto data = static_cast<uint8_t*>(blob->data);
  auto edid = std::vector<uint8_t>(data, data + blob->length);

  drmModeFreePropertyBlob(blob);

  return edid;
}

int CDRMConnector::GetMaxFrlRate() const
{
  return ParseMaxFrlRate(GetEDID());
}

bool CDRMConnector::HasModeAboveClock(uint32_t clockKHz) const
{
  for (int i = 0; i < m_connector->count_modes; i++)
  {
    if (m_connector->modes[i].clock > clockKHz)
      return true;
  }

  return false;
}
