/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DRMObject.h"

#include <string>

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

class CDRMConnector : public CDRMObject
{
public:
  explicit CDRMConnector(int fd, uint32_t connector);
  CDRMConnector(const CDRMConnector&) = delete;
  CDRMConnector& operator=(const CDRMConnector&) = delete;
  ~CDRMConnector() = default;

  std::string GetType();
  std::string GetStatus();
  std::string GetName();

  uint32_t GetEncoderId() const { return m_connector->encoder_id; }
  uint32_t* GetConnectorId() const { return &m_connector->connector_id; }
  int GetModesCount() const { return m_connector->count_modes; }
  drmModeModeInfoPtr GetModeForIndex(int index) const { return &m_connector->modes[index]; }

  bool IsConnected() { return m_connector->connection == DRM_MODE_CONNECTED; }
  bool CheckConnector();

  std::vector<uint8_t> GetEDID() const;

private:
  struct DrmModeConnectorDeleter
  {
    void operator()(drmModeConnector* p) { drmModeFreeConnector(p); }
  };

  std::unique_ptr<drmModeConnector, DrmModeConnectorDeleter> m_connector;
};

} // namespace GBM
} // namespace WINDOWING
} // namespace KODI
