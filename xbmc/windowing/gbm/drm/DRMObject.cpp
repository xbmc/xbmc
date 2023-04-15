/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMObject.h"

#include "utils/log.h"

#include <algorithm>
#include <array>

using namespace KODI::WINDOWING::GBM;

namespace
{

constexpr std::array<std::pair<uint32_t, const char*>, 8> DrmModeObjectTypes = {
    {{DRM_MODE_OBJECT_CRTC, "crtc"},
     {DRM_MODE_OBJECT_CONNECTOR, "connector"},
     {DRM_MODE_OBJECT_ENCODER, "encoder"},
     {DRM_MODE_OBJECT_MODE, "mode"},
     {DRM_MODE_OBJECT_PROPERTY, "property"},
     {DRM_MODE_OBJECT_FB, "framebuffer"},
     {DRM_MODE_OBJECT_BLOB, "blob"},
     {DRM_MODE_OBJECT_PLANE, "plane"}}};
}

CDRMObject::CDRMObject(int fd) : m_fd(fd)
{
}

std::string CDRMObject::GetTypeName() const
{
  auto name = std::find_if(DrmModeObjectTypes.begin(), DrmModeObjectTypes.end(),
                           [this](const auto& p) { return p.first == m_type; });
  if (name != DrmModeObjectTypes.end())
    return name->second;

  return "invalid type";
}

std::string CDRMObject::GetPropertyName(uint32_t propertyId) const
{
  auto prop = std::find_if(m_propsInfo.begin(), m_propsInfo.end(),
                           [&propertyId](const auto& p) { return p->prop_id == propertyId; });
  if (prop != m_propsInfo.end())
    return prop->get()->name;

  return "invalid property";
}

uint32_t CDRMObject::GetPropertyId(const std::string& name) const
{
  auto property = std::find_if(m_propsInfo.begin(), m_propsInfo.end(),
                               [&name](const auto& prop) { return prop->name == name; });

  if (property != m_propsInfo.end())
    return property->get()->prop_id;

  return 0;
}

bool CDRMObject::GetProperties(uint32_t id, uint32_t type)
{
  m_props.reset(drmModeObjectGetProperties(m_fd, id, type));
  if (!m_props)
    return false;

  m_id = id;
  m_type = type;

  for (uint32_t i = 0; i < m_props->count_props; i++)
    m_propsInfo.emplace_back(std::unique_ptr<drmModePropertyRes, DrmModePropertyResDeleter>(
        drmModeGetProperty(m_fd, m_props->props[i])));

  return true;
}

std::optional<uint64_t> CDRMObject::GetPropertyValue(std::string_view name,
                                                     std::string_view valueName) const
{
  auto property = std::find_if(m_propsInfo.begin(), m_propsInfo.end(),
                               [&name](const auto& prop) { return prop->name == name; });

  if (property == m_propsInfo.end())
    return {};

  auto prop = property->get();

  if (!static_cast<bool>(drm_property_type_is(prop, DRM_MODE_PROP_ENUM)))
    return {};

  for (int j = 0; j < prop->count_enums; j++)
  {
    if (prop->enums[j].name != valueName)
      continue;

    return std::make_optional<uint64_t>(prop->enums[j].value);
  }

  return {};
}

bool CDRMObject::SetProperty(const std::string& name, uint64_t value)
{
  auto property = std::find_if(m_propsInfo.begin(), m_propsInfo.end(),
                               [&name](const auto& prop) { return prop->name == name; });

  if (property != m_propsInfo.end())
  {
    int ret = drmModeObjectSetProperty(m_fd, m_id, m_type, property->get()->prop_id, value);
    if (ret == 0)
      return true;
  }

  return false;
}

bool CDRMObject::SupportsProperty(const std::string& name)
{
  auto property = std::find_if(m_propsInfo.begin(), m_propsInfo.end(),
                               [&name](const auto& prop) { return prop->name == name; });

  if (property != m_propsInfo.end())
    return true;

  return false;
}
