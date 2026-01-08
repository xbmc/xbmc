/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMPlane.h"

#include "DRMUtils.h"
#include "utils/DRMHelpers.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>

using namespace KODI::WINDOWING::GBM;

CDRMPlane::CDRMPlane(int fd, uint32_t plane) : CDRMObject(fd), m_plane(drmModeGetPlane(m_fd, plane))
{
  if (!m_plane)
    throw std::runtime_error("drmModeGetPlane failed: " + std::string{strerror(errno)});

  if (!GetProperties(m_plane->plane_id, DRM_MODE_OBJECT_PLANE))
    throw std::runtime_error("failed to get properties for plane: " +
                             std::to_string(m_plane->plane_id));
}

bool CDRMPlane::SupportsFormat(uint32_t format)
{
  for (uint32_t i = 0; i < m_plane->count_formats; i++)
    if (m_plane->formats[i] == format)
      return true;

  return false;
}

bool CDRMPlane::SupportsFormatAndModifier(uint32_t format, uint64_t modifier)
{
  /*
   * Some broadcom modifiers have parameters encoded which need to be
   * masked out before comparing with reported modifiers.
   */
  if (modifier >> 56 == DRM_FORMAT_MOD_VENDOR_BROADCOM)
    modifier = fourcc_mod_broadcom_mod(modifier);

  if (modifier == DRM_FORMAT_MOD_LINEAR)
  {
    if (!SupportsFormat(format))
    {
      CLog::Log(LOGDEBUG, "CDRMPlane::{} - format not supported: {}", __FUNCTION__,
                DRMHELPERS::FourCCToString(format));
      return false;
    }
  }
  else
  {
    auto formatModifiers = &m_modifiers_map[format];
    if (formatModifiers->empty())
    {
      CLog::Log(LOGDEBUG, "CDRMPlane::{} - format not supported: {}", __FUNCTION__,
                DRMHELPERS::FourCCToString(format));
      return false;
    }

    auto formatModifier = std::ranges::find(*formatModifiers, modifier);
    if (formatModifier == formatModifiers->end())
    {
      CLog::Log(LOGDEBUG, "CDRMPlane::{} - modifier ({}) not supported for format ({})",
                __FUNCTION__, DRMHELPERS::ModifierToString(modifier),
                DRMHELPERS::FourCCToString(format));
      return false;
    }
  }

  CLog::Log(LOGDEBUG, "CDRMPlane::{} - found plane format ({}) and modifier ({})", __FUNCTION__,
            DRMHELPERS::FourCCToString(format), DRMHELPERS::ModifierToString(modifier));

  return true;
}

void CDRMPlane::FindModifiers()
{
  uint64_t blob_id = GetPropertyValue("IN_FORMATS").value_or(0);

  if (blob_id == 0)
    return;

  drmModePropertyBlobPtr blob = drmModeGetPropertyBlob(m_fd, blob_id);
  if (!blob)
    return;

  drm_format_modifier_blob* header = static_cast<drm_format_modifier_blob*>(blob->data);
  uint32_t* formats =
      reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(header) + header->formats_offset);
  drm_format_modifier* mod = reinterpret_cast<drm_format_modifier*>(
      reinterpret_cast<char*>(header) + header->modifiers_offset);

  for (uint32_t i = 0; i < header->count_formats; i++)
  {
    std::vector<uint64_t> modifiers;
    for (uint32_t j = 0; j < header->count_modifiers; j++)
    {
      if (mod[j].formats & 1ULL << i)
        modifiers.emplace_back(mod[j].modifier);
    }

    m_modifiers_map.emplace(formats[i], modifiers);
  }

  if (blob)
    drmModeFreePropertyBlob(blob);
}

bool CDRMPlane::Check(
    uint64_t w, uint64_t h, uint32_t format, uint64_t modifier, CDRMCrtc* crtc, PlaneType type)
{
  if (crtc != nullptr && !(m_plane->possible_crtcs & (1 << crtc->GetOffset())))
    return false;

  // format and modifier is not supported, invalid format/modifier means ignore
  if (format != DRM_FORMAT_INVALID)
  {
    if (modifier != DRM_FORMAT_MOD_INVALID && !SupportsFormatAndModifier(format, modifier))
      return false;
    if (modifier == DRM_FORMAT_MOD_INVALID && !SupportsFormat(format))
      return false;
  }

  const auto plane_type = GetPropertyValue("type").value_or(PLANE_TYPE_UNKNOWN);

  if (type != PLANE_TYPE_ANY && plane_type != type)
    return false;

  auto input_width_limits = GetRangePropertyLimits("INPUT_WIDTH");
  // check if plane has defined max width and it satifies the buffer width
  if (input_width_limits && input_width_limits->second < w)
    return false;

  // if dont use a cursor plane unless drm hints us it can be used
  // currently only hint is INPUT_WIDTH/HEIGHT attribute provided by rockchip drm
  // in future if any other drm really needs cursor planes we can determine it
  // by carefully inspecting their drm implementation or adding QUIRK_USECURSORPLANES
  if (!input_width_limits && plane_type == PLANE_TYPE_CURSOR)
    return false;

  auto input_height_limits = GetRangePropertyLimits("INPUT_HEIGHT");
  if (input_height_limits && input_height_limits->second < h)
    return false;

  if (!input_height_limits && plane_type == PLANE_TYPE_CURSOR)
    return false;

  return true;
}
