/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DRMCrtc.h"
#include "DRMObject.h"

#include <map>

#include <drm_fourcc.h>

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

enum PlaneType
{
  PLANE_TYPE_OVERLAY,
  PLANE_TYPE_PRIMARY,
  PLANE_TYPE_CURSOR,
  PLANE_TYPE_UNKNOWN,
  PLANE_TYPE_ANY,
};

class CDRMPlane : public CDRMObject
{
public:
  explicit CDRMPlane(int fd, uint32_t plane);
  CDRMPlane(const CDRMPlane&) = delete;
  CDRMPlane& operator=(const CDRMPlane&) = delete;
  ~CDRMPlane() = default;

  uint32_t GetPlaneId() const { return m_plane->plane_id; }
  uint32_t GetPossibleCrtcs() const { return m_plane->possible_crtcs; }

  void FindModifiers();

  void SetFormat(const uint32_t newFormat) { m_format = newFormat; }
  uint32_t GetFormat() const { return m_format; }
  void SetModifier(const uint64_t newModifier) { m_modifier = newModifier; }
  uint64_t GetModifier() const { return m_modifier; }
  std::vector<uint64_t>& GetModifiersForFormat(uint32_t format) { return m_modifiers_map[format]; }

  bool SupportsFormat(uint32_t format);
  bool SupportsFormatAndModifier(uint32_t format, uint64_t modifier);
  bool Check(
      uint64_t w, uint64_t h, uint32_t format, uint64_t modifier, CDRMCrtc* crtc, PlaneType type);
  bool MoveOnTopOf(CDRMPlane* other);

private:
  struct DrmModePlaneDeleter
  {
    void operator()(drmModePlane* p) { drmModeFreePlane(p); }
  };

  std::unique_ptr<drmModePlane, DrmModePlaneDeleter> m_plane;

  std::map<uint32_t, std::vector<uint64_t>> m_modifiers_map;
  uint32_t m_format{DRM_FORMAT_XRGB8888};
  uint64_t m_modifier{DRM_FORMAT_MOD_INVALID};
};

} // namespace GBM
} // namespace WINDOWING
} // namespace KODI
