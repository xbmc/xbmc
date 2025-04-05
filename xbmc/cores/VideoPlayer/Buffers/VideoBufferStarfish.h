/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "VideoBuffer.h"

class AcbHandle
{
public:
  AcbHandle();
  ~AcbHandle() noexcept;

  AcbHandle(const AcbHandle&) = delete;
  AcbHandle& operator=(const AcbHandle&) = delete;

  [[nodiscard]] long Id() const noexcept;
  [[nodiscard]] long& TaskId() noexcept;

  void Reset() noexcept;

private:
  long m_handle{0};
  long m_taskId{0};
};

class CStarfishVideoBuffer final : public CVideoBuffer
{
public:
  explicit CStarfishVideoBuffer();
  AVPixelFormat GetFormat() override;
  std::unique_ptr<AcbHandle>& GetAcbHandle() noexcept;
  std::unique_ptr<AcbHandle>& CreateAcbHandle();
  void ResetAcbHandle();

private:
  std::unique_ptr<AcbHandle> m_acbHandle{nullptr};
};
