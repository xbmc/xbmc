/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

namespace KODI::RETRO
{
class CRenderContext;
struct HwContextProperties;

class IHwRenderingContext
{
public:
  virtual ~IHwRenderingContext() = default;
  virtual bool SupportsHardwareRendering() const = 0;
  virtual bool Create(const HwContextProperties& properties) = 0;
  virtual bool IsCreated() const = 0;

  // The pool serializes calls and binds only for the outermost client scope.
  // Failed binding preserves the caller's context; restoration may clear it
  // on failure, but must never leave the client context current.
  virtual bool MakeCurrent() = 0;
  virtual void RestoreCurrent() = 0;
  // Release native ownership without requiring a bind, and always invalidate IsCreated().
  virtual void Destroy() = 0;
};

std::unique_ptr<IHwRenderingContext> CreateHwRenderingContext(CRenderContext& context);
} // namespace KODI::RETRO
