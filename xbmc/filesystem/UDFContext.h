/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/UDFBlockInput.h"

#include <memory>
#include <string>

struct udfread;

namespace XFILE
{
/*!
 \brief A mounted UDF volume, shared by everything reading from the same image.

 Mounting parses the volume descriptor sequence, the file set descriptor and the root directory -
 a dozen or so reads of the image, and so a dozen round trips when the image is on a network share.
 That is paid on every file open unless the volume is shared, which made examining a bluray image
 (one open per playlist, of which a disc can have hundreds) dominated by re-mounting the same
 volume.

 libudfread supports a shared handle: its lazily built directory cache is maintained with atomic
 compare-and-exchange, per-file and per-directory read positions live in the UDFFILE/UDFDIR
 handles, and the only other shared state is the block input, whose reads CUDFBlockInput
 serialises.
 */
class CUDFContext
{
public:
  ~CUDFContext();
  CUDFContext(const CUDFContext&) = delete;
  CUDFContext& operator=(const CUDFContext&) = delete;
  CUDFContext(CUDFContext&&) = delete;
  CUDFContext& operator=(CUDFContext&&) = delete;

  /*!
   \brief Get the mounted volume of an image, mounting it if nothing holds it mounted already.
   The volume stays mounted until the last holder lets go - it is never held by the cache alone, as
   that would leave a handle open on the image and, on Windows, stop the user deleting or renaming
   it. A single open is therefore no cheaper than before; to make a run of opens share one mount,
   hold a CUDFMount across them.
   \param image path to the image
   \return the mounted volume, or nullptr if the image could not be mounted
   */
  static std::shared_ptr<CUDFContext> Get(const std::string& image);

  udfread* GetHandle() const { return m_udf; }

private:
  CUDFContext() = default;
  bool Mount(const std::string& image);

  CUDFBlockInput m_blockInput;
  udfread* m_udf{nullptr};
};

/*!
 \brief Keeps the UDF volume of an image mounted for as long as the guard lives.

 Hold one across a run of reads from the same image - examining a bluray reads one file per
 playlist - so that they share a single mount instead of re-mounting the volume for each. Costs
 nothing for a path that does not name a file on a UDF image.
 */
class CUDFMount
{
public:
  /*!
   \param path any path or URL. Nothing is held unless it is a udf:// URL.
   */
  explicit CUDFMount(const std::string& path);

  ~CUDFMount() = default;
  CUDFMount(const CUDFMount&) = delete;
  CUDFMount& operator=(const CUDFMount&) = delete;
  CUDFMount(CUDFMount&&) noexcept = default;
  CUDFMount& operator=(CUDFMount&&) noexcept = default;

private:
  std::shared_ptr<CUDFContext> m_context;
};
} // namespace XFILE
