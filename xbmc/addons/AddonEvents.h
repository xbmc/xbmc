/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"

#include <string>

namespace ADDON
{

struct AddonEvent
{
  std::string addonId;
  AddonInstanceId instanceId{ADDON_SINGLETON_INSTANCE_ID};

  explicit AddonEvent(std::string addonId) : addonId(std::move(addonId)) {}
  AddonEvent(std::string addonId, AddonInstanceId instanceId)
    : addonId(std::move(addonId)), instanceId(instanceId)
  {
  }

  // Note: Do not remove the virtual dtor. There are types derived from AddonEvent (see below)
  //       and there are several places where 'typeid' is used to determine the runtime type of
  //       AddonEvent references. And 'typeid' only works for polymorphic objects.
  virtual ~AddonEvent() = default;
};

namespace AddonEvents
{

/**
 * Emitted after the add-on has been enabled.
 */
struct Enabled : AddonEvent
{
  explicit Enabled(std::string addonId) : AddonEvent(std::move(addonId)) {}
};

/**
 * Emitted after the add-on has been disabled.
 */
struct Disabled : AddonEvent
{
  explicit Disabled(std::string addonId) : AddonEvent(std::move(addonId)) {}
};

/**
 * Emitted after a new usable add-on instance was added.
 */
struct InstanceAdded : AddonEvent
{
  InstanceAdded(std::string addonId, AddonInstanceId instanceId)
    : AddonEvent(std::move(addonId), instanceId)
  {
  }
};

/**
 * Emitted after an add-on instance was removed.
 */
struct InstanceRemoved : AddonEvent
{
  InstanceRemoved(std::string addonId, AddonInstanceId instanceId)
    : AddonEvent(std::move(addonId), instanceId)
  {
  }
};

/**
 * Emitted after the add-on's metadata has been changed.
 */
struct MetadataChanged : AddonEvent
{
  explicit MetadataChanged(std::string addonId) : AddonEvent(std::move(addonId)) {}
};

/**
 * Emitted when a different version of the add-on has been installed
 * to the file system and should be reloaded.
 */
struct ReInstalled : AddonEvent
{
  explicit ReInstalled(std::string addonId) : AddonEvent(std::move(addonId)) {}
};

/**
 * Emitted after the add-on has been uninstalled.
 */
struct UnInstalled : AddonEvent
{
  explicit UnInstalled(std::string addonId) : AddonEvent(std::move(addonId)) {}
};

/**
 * Emitted after the add-on has been loaded.
 */
struct Load : AddonEvent
{
  explicit Load(std::string addonId) : AddonEvent(std::move(addonId)) {}
};

/**
 * Emitted after the add-on has been unloaded.
 */
struct Unload : AddonEvent
{
  explicit Unload(std::string addonId) : AddonEvent(std::move(addonId)) {}
};

/**
 * Emitted after the auto-update state of the add-on has been changed.
 */
struct AutoUpdateStateChanged : AddonEvent
{
  explicit AutoUpdateStateChanged(std::string addonId) : AddonEvent(std::move(addonId)) {}
};

} // namespace AddonEvents
} // namespace ADDON
