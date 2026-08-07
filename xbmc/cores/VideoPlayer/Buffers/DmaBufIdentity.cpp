/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DmaBufIdentity.h"

#include <sys/stat.h>

namespace DRMPRIME
{

bool StatInode(int fd, uint64_t& inode)
{
  struct stat st
  {
  };
  if (fstat(fd, &st) != 0)
    return false;

  inode = st.st_ino;
  return true;
}

bool DmaBufIdentity::SameMemory(const DmaBufIdentity& other) const
{
  auto contains = [](const DmaBufIdentity& haystack, uint64_t needle)
  {
    for (int i = 0; i < haystack.nbObjects; i++)
      if (haystack.inode[i] == needle)
        return true;
    return false;
  };
  for (int i = 0; i < nbObjects; i++)
    if (!contains(other, inode[i]))
      return false;
  for (int i = 0; i < other.nbObjects; i++)
    if (!contains(*this, other.inode[i]))
      return false;
  return true;
}

std::optional<DmaBufIdentity> ComputeDmaBufIdentity(const AVDRMFrameDescriptor* descriptor,
                                                    uint32_t width,
                                                    uint32_t height,
                                                    const StatInodeFn& statInode)
{
  if (!descriptor || descriptor->nb_layers < 1 || descriptor->nb_objects < 1 ||
      descriptor->nb_objects > AV_DRM_MAX_PLANES)
    return std::nullopt;

  const AVDRMLayerDescriptor& layer = descriptor->layers[0];
  if (layer.nb_planes < 1 || layer.nb_planes > AV_DRM_MAX_PLANES)
    return std::nullopt;

  DmaBufIdentity identity;
  identity.nbObjects = descriptor->nb_objects;
  for (int i = 0; i < descriptor->nb_objects; i++)
  {
    if (!statInode(descriptor->objects[i].fd, identity.inode[i]))
      return std::nullopt;
    identity.modifier[i] = descriptor->objects[i].format_modifier;
  }

  identity.width = width;
  identity.height = height;
  identity.format = layer.format;
  identity.nbPlanes = layer.nb_planes;
  for (int i = 0; i < layer.nb_planes; i++)
  {
    identity.objectIndex[i] = layer.planes[i].object_index;
    identity.offset[i] = layer.planes[i].offset;
    identity.pitch[i] = layer.planes[i].pitch;
  }

  return identity;
}

} // namespace DRMPRIME
