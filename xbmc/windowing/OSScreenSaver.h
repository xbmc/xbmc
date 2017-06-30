/*
 *      Copyright (C) 2017 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
  COSScreenSaverInhibitor();
  COSScreenSaverInhibitor(COSScreenSaverInhibitor&& other);
  COSScreenSaverInhibitor& operator=(COSScreenSaverInhibitor&& other);
  ~COSScreenSaverInhibitor();
  void Release();
  bool IsActive() const;
  operator bool() const;

private:
  friend class COSScreenSaverManager;
  COSScreenSaverInhibitor(COSScreenSaverManager* manager);
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
  COSScreenSaverManager(std::unique_ptr<IOSScreenSaver>&& impl);
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

  unsigned int m_inhibitionCount = 0;
  std::unique_ptr<IOSScreenSaver> m_impl;
};

}
}