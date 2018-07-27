/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <utility>

namespace KODI
{
namespace WINDOWING
{

class COSScreenSaverManager;

/**
 * Inhibit the OS screen saver as long as this object is alive
 *
 * Destroy or call \ref Release to stop this inhibitor from being active.
 * The OS screen saver may still be inhibited as long as other inhibitors are
 * active though.
 *
 * \note Make sure to release or destroy the inhibitor before the \ref
 *       COSScreenSaverManager is destroyed
 */
class COSScreenSaverInhibitor
{
public:
  COSScreenSaverInhibitor() noexcept;
  COSScreenSaverInhibitor(COSScreenSaverInhibitor&& other) noexcept;
  COSScreenSaverInhibitor& operator=(COSScreenSaverInhibitor&& other) noexcept;
  ~COSScreenSaverInhibitor() noexcept;
  void Release();
  bool IsActive() const;
  operator bool() const;

private:
  friend class COSScreenSaverManager;
  explicit COSScreenSaverInhibitor(COSScreenSaverManager* manager);
  bool m_active;
  COSScreenSaverManager* m_manager;

  COSScreenSaverInhibitor(COSScreenSaverInhibitor const& other) = delete;
  COSScreenSaverInhibitor& operator=(COSScreenSaverInhibitor const& other) = delete;
};

/**
 * Interface for OS screen saver control implementations
 */
class IOSScreenSaver
{
public:
  virtual ~IOSScreenSaver() = default;
  /**
   * Do not allow the OS screen saver to become active
   *
   * Calling this function multiple times without calling \ref Unhibit
   * MUST NOT produce any side-effects.
   */
  virtual void Inhibit() = 0;
  /**
   * Allow the OS screen saver to become active again
   *
   * Calling this function multiple times or at all without calling \ref Inhibit
   * MUST NOT produce any side-effects.
   */
  virtual void Uninhibit() = 0;
};

/**
 * Dummy implementation of IOSScreenSaver
 */
class CDummyOSScreenSaver : public IOSScreenSaver
{
public:
  void Inhibit() override {}
  void Uninhibit() override {}
};

/**
 * Manage the OS screen saver
 *
 * This class keeps track of a number of \ref COSScreenSaverInhibitor instances
 * and keeps the OS screen saver inhibited as long as at least one of them
 * exists and is active.
 */
class COSScreenSaverManager
{
public:
  /**
   * Create manager with backing OS-specific implementation
   */
  explicit COSScreenSaverManager(std::unique_ptr<IOSScreenSaver> impl);
  /**
   * Create inhibitor that prevents the OS screen saver from becoming active as
   * long as it is alive
   */
  COSScreenSaverInhibitor CreateInhibitor();
  /**
   * Check whether the OS screen saver is currently inhibited
   */
  bool IsInhibited();

private:
  friend class COSScreenSaverInhibitor;
  void RemoveInhibitor();

  unsigned int m_inhibitionCount{0u};
  std::unique_ptr<IOSScreenSaver> m_impl;
};

}
}
