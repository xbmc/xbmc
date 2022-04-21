/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <memory>
#include <typeindex>
#include <unordered_map>

//! \brief Base class for services.
class IPlatformService
{
public:
  virtual ~IPlatformService() = default;
};

/**\brief Class for the Platform object
 *
 * Contains methods to retrieve platform specific information
 * and methods for doing platform specific environment preparation/initialisation
 */
class CPlatform
{
public:
  /**\brief Creates the Platform object
   *
   *@return the platform object
  */
  static CPlatform *CreateInstance();

  /**\brief C'tor */
  CPlatform() = default;

  /**\brief D'tor */
  virtual ~CPlatform() = default;

  /**\brief Called at an early stage of application startup
   *
   * This method can be used to do platform specific environment preparation
   * or initialisation (like setting environment variables for example)
   */
  virtual bool InitStageOne() { return true; }

  /**\brief Called at a middle stage of application startup
   *
   * This method can be used for starting platform specific services that
   * do not depend on windowing/gui. (eg macos XBMCHelper)
   */
  virtual bool InitStageTwo() { return true; }

  /**\brief Called at a late stage of application startup
   *
   * This method can be used for starting platform specific Window/GUI related
   * services/components. (eg , WS-Discovery Daemons)
   */
  virtual bool InitStageThree() { return true; }

  /**\brief Called at a late stage of application shutdown
   *
   * This method should be used to cleanup resources allocated in InitStageOne
   */
  virtual void DeinitStageOne() {}

  /**\brief Called at a middle stage of application shutdown
   *
   * This method should be used to cleanup resources allocated in InitStageTwo
   */
  virtual void DeinitStageTwo() {}

  /**\brief Called at an early stage of application shutdown
   *
   * This method should be used to cleanup resources allocated in InitStageThree
   */
  virtual void DeinitStageThree() {}

  /**\brief Flag whether disabled add-ons - installed via packagemanager or manually - should be
   * offered for configuration and activation on kodi startup for this platform
   */
  virtual bool IsConfigureAddonsAtStartupEnabled() { return false; }

  /**\brief Flag whether this platform supports user installation of binary add-ons.
   */
  virtual bool SupportsUserInstalledBinaryAddons() { return true; }

  /**\brief Print platform specific info to log
   *
   * Logs platform specific system info during application creation startup
   */
  virtual void PlatformSyslog() {}

  /**\brief Get a platform service instance.
   */
  template<class T>
  std::shared_ptr<T> GetService()
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    const auto it = m_services.find(std::type_index(typeid(T)));
    if (it != m_services.end())
      return std::static_pointer_cast<T>((*it).second);

    return nullptr;
  }

protected:
  /**\brief Register a new platform service instance.
   */
  void RegisterService(const std::shared_ptr<IPlatformService>& service)
  {
    if (!service)
      return;

    // Note: Extra var needed to avoid clang warning
    // "Expression with side effects will be evaluated despite being used as an operand to 'typeid'"
    // https://stackoverflow.com/questions/46494928/clang-warning-on-expression-side-effects
    const auto& serviceRef = *service;

    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_services.insert({std::type_index(typeid(serviceRef)), service});
  }

  /**\brief Deregister a platform service instance.
   */
  void DeregisterService(const std::type_info& typeInfo)
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_services.erase(typeInfo);
  }

private:
  CCriticalSection m_critSection;
  std::unordered_map<std::type_index, std::shared_ptr<IPlatformService>> m_services;
};
