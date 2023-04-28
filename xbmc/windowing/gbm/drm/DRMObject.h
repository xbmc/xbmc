/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include <xf86drmMode.h>

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

class CDRMObject
{
public:
  CDRMObject(const CDRMObject&) = delete;
  CDRMObject& operator=(const CDRMObject&) = delete;
  virtual ~CDRMObject() = default;

  std::string GetTypeName() const;
  std::string GetPropertyName(uint32_t propertyId) const;

  uint32_t GetId() const { return m_id; }
  uint32_t GetPropertyId(const std::string& name) const;
  std::optional<uint64_t> GetPropertyValue(std::string_view name, std::string_view valueName) const;

  bool SetProperty(const std::string& name, uint64_t value);
  bool SupportsProperty(const std::string& name);

protected:
  explicit CDRMObject(int fd);

  bool GetProperties(uint32_t id, uint32_t type);

  struct DrmModeObjectPropertiesDeleter
  {
    void operator()(drmModeObjectProperties* p) { drmModeFreeObjectProperties(p); }
  };

  std::unique_ptr<drmModeObjectProperties, DrmModeObjectPropertiesDeleter> m_props;

  struct DrmModePropertyResDeleter
  {
    void operator()(drmModePropertyRes* p) { drmModeFreeProperty(p); }
  };

  std::vector<std::unique_ptr<drmModePropertyRes, DrmModePropertyResDeleter>> m_propsInfo;

  int m_fd{-1};

private:
  uint32_t m_id{0};
  uint32_t m_type{0};
};

} // namespace GBM
} // namespace WINDOWING
} // namespace KODI
