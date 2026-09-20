/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>

extern "C"
{
#include <libavutil/hwcontext_drm.h>
}

namespace DRMPRIME
{

//! \brief Writes the dma-buf inode for fd; false on failure.
using StatInodeFn = std::function<bool(int fd, uint64_t& inode)>;

// fstat st_ino; unique and never reused for dma-bufs (kernel 5.3+)
bool StatInode(int fd, uint64_t& inode);

//! \brief Identifies a frame's memory and layout across file descriptor reuse.
struct DmaBufIdentity
{
  // memory: dma-buf inode per descriptor object, in object order
  int nbObjects{0};
  std::array<uint64_t, AV_DRM_MAX_PLANES> inode{};
  std::array<uint64_t, AV_DRM_MAX_PLANES> modifier{};
  // layout: everything AddFB2 / EGL import consume besides the memory
  uint32_t width{0};
  uint32_t height{0};
  uint32_t format{0};
  int nbLayers{0};
  int nbPlanes{0};
  std::array<int, AV_DRM_MAX_PLANES> objectIndex{};
  std::array<uint64_t, AV_DRM_MAX_PLANES> offset{};
  std::array<uint64_t, AV_DRM_MAX_PLANES> pitch{};

  // compares the arrays' unused tails too; sound only for value-initialized, never-mutated instances
  bool operator==(const DmaBufIdentity& other) const = default;

  //! \brief True when the underlying memory (inodes) matches, layout aside.
  bool SameMemory(const DmaBufIdentity& other) const;
};

//! \brief Identity of descriptor layer[0] only; nbLayers is keyed, layers 1+ are
//! not inspected: a consumer that reads layers 1+ must not key on this identity.
//! Returns nullopt for a missing or malformed descriptor or an unreadable inode.
std::optional<DmaBufIdentity> GetDmaBufIdentity(const AVDRMFrameDescriptor* descriptor,
                                                uint32_t width,
                                                uint32_t height);

//! \brief Test seam: same, with an injectable stat function.
std::optional<DmaBufIdentity> GetDmaBufIdentity(const AVDRMFrameDescriptor* descriptor,
                                                uint32_t width,
                                                uint32_t height,
                                                const StatInodeFn& statInode);

} // namespace DRMPRIME
