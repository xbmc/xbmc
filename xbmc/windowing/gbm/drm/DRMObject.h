/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/log.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
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
  std::optional<uint64_t> GetPropertyEnumValue(std::string_view name,
                                               std::string_view valueName) const;

  bool SetProperty(const std::string& name, uint64_t value);
  bool SupportsProperty(const std::string& name);
  std::optional<uint64_t> GetPropertyValue(const std::string& name) const;
  bool CachePropertyValue(uint32_t propertyId, uint64_t value);
  std::optional<bool> IsPropertyImmutable(const std::string& name) const;
  std::optional<std::pair<uint64_t, uint64_t>> GetRangePropertyLimits(
      const std::string& name) const;

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

/*!
 * \brief RAII wrapper around a DRM property blob.
 *
 * Owns both the drm fd and the blob id, and destroys the blob on
 * scope exit. Move-only.
 *
 * Lifetime note: an atomic commit that references the blob id must
 * complete BEFORE this object is destroyed; DRM requires the blob to
 * be alive for the duration of the commit. Declare at a scope (local
 * or member) that ends AFTER drmModeAtomicCommit returns.
 */
class CDRMPropertyBlob
{
public:
  CDRMPropertyBlob() = default;

  CDRMPropertyBlob(int fd, const void* data, std::size_t size) : m_fd(fd)
  {
    if (drmModeCreatePropertyBlob(fd, data, size, &m_id) != 0)
      m_id = 0;
  }

  ~CDRMPropertyBlob() { Reset(); }

  CDRMPropertyBlob(const CDRMPropertyBlob&) = delete;
  CDRMPropertyBlob& operator=(const CDRMPropertyBlob&) = delete;

  CDRMPropertyBlob(CDRMPropertyBlob&& other) noexcept
    : m_fd(other.m_fd),
      m_id(std::exchange(other.m_id, 0))
  {
  }

  CDRMPropertyBlob& operator=(CDRMPropertyBlob&& other) noexcept
  {
    if (this != &other)
    {
      Reset();
      m_fd = other.m_fd;
      m_id = std::exchange(other.m_id, 0);
    }
    return *this;
  }

  /*! \brief Blob id to pass to AddProperty / atomic commit. */
  std::uint32_t Get() const { return m_id; }

  /*! \brief True if the blob was successfully created and not yet released. */
  bool IsValid() const { return m_id != 0; }

  /*!
   * \brief Destroy the owned blob (if any) and return to the null state.
   *
   * Safe to call on a default-constructed or already-released instance.
   */
  void Reset()
  {
    if (m_id != 0 && drmModeDestroyPropertyBlob(m_fd, m_id) != 0)
      CLog::LogF(LOGERROR, "failed to destroy property blob: {}", std::strerror(errno));
    m_id = 0;
  }

private:
  int m_fd{-1};
  std::uint32_t m_id{0};
};

} // namespace GBM
} // namespace WINDOWING
} // namespace KODI
