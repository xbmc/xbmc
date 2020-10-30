/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMObject.h"

#include "utils/StringUtils.h"
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
                           [this](auto& p) { return p.first == m_type; });
  if (name != DrmModeObjectTypes.end())
    return name->second;

  return "invalid type";
}

std::string CDRMObject::GetPropertyName(uint32_t propertyId) const
{
  auto prop = std::find_if(m_propsInfo.begin(), m_propsInfo.end(),
                           [&propertyId](auto& p) { return p->prop_id == propertyId; });
  if (prop != m_propsInfo.end())
    return prop->get()->name;

  return "invalid property";
}

uint32_t CDRMObject::GetPropertyId(const char* name) const
{
  auto property = std::find_if(m_propsInfo.begin(), m_propsInfo.end(), [&name](auto& prop) {
    return StringUtils::EqualsNoCase(prop->name, name);
  });

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

bool CDRMObject::GetPropertyValue(std::string name, const std::string& type, uint64_t& value) const
{
  auto property = std::find_if(m_propsInfo.begin(), m_propsInfo.end(), [&name](auto& prop) {
    return StringUtils::EqualsNoCase(prop->name, name);
  });

  if (property == m_propsInfo.end())
    return false;

  auto prop = property->get();

  if (drm_property_type_is(prop, DRM_MODE_PROP_ENUM) != 0)
  {
    for (int j = 0; j < prop->count_enums; j++)
    {
      if (std::strcmp(prop->enums[j].name, type.c_str()) == 0)
      {
        value = prop->enums[j].value;
        return true;
      }
    }
  }

  return false;
}

bool CDRMObject::SetProperty(const char* name, uint64_t value)
{
  auto property = std::find_if(m_propsInfo.begin(), m_propsInfo.end(), [&name](auto& prop) {
    return StringUtils::EqualsNoCase(prop->name, name);
  });

  if (property != m_propsInfo.end())
  {
    int ret = drmModeObjectSetProperty(m_fd, m_id, m_type, property->get()->prop_id, value);
    if (ret == 0)
      return true;
  }

  return false;
}

bool CDRMObject::SupportsProperty(const char* name)
{
  auto property = std::find_if(m_propsInfo.begin(), m_propsInfo.end(), [&name](auto& prop) {
    return StringUtils::EqualsNoCase(prop->name, name);
  });

  if (property != m_propsInfo.end())
    return true;

  return false;
}

bool CDRMObject::SupportsPropertyAndValue(const char* name, uint64_t value)
{
  auto property = std::find_if(m_propsInfo.begin(), m_propsInfo.end(), [&name](auto& prop) {
    return StringUtils::EqualsNoCase(prop->name, name);
  });

  if (property != m_propsInfo.end())
  {
    if (drm_property_type_is(property->get(), DRM_MODE_PROP_ENUM) != 0)
    {
      for (int j = 0; j < property->get()->count_enums; j++)
      {
        if (property->get()->enums[j].value == value)
          return true;
      }
    }

    CLog::Log(LOGDEBUG, "CDRMObject::{} - property '{}' does not support value '{}'", __FUNCTION__,
              name, value);
  }

  return false;
}
