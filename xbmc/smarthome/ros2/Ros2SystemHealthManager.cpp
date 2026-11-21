/*
 *  Copyright (C) 2024-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2SystemHealthManager.h"

#include "Ros2PropertyManager.h"
#include "Ros2SystemHealthSubscriber.h"

using namespace KODI;
using namespace SMART_HOME;

CRos2SystemHealthManager::CRos2SystemHealthManager(std::string rosNamespace)
  : m_rosNamespace(std::move(rosNamespace)),
    m_propertyManager(std::make_unique<CRos2PropertyManager>(
        m_rosNamespace, [this](const std::string& systemName) { MarkActive(systemName); }))
{
}

CRos2SystemHealthManager::~CRos2SystemHealthManager()
{
  Deinitialize();
}

void CRos2SystemHealthManager::Initialize(std::shared_ptr<rclcpp::Node> node)
{
  {
    std::lock_guard lock(m_mutex);
    m_node = node;
  }
  m_propertyManager->Initialize(std::move(node));
}

void CRos2SystemHealthManager::Deinitialize()
{
  m_propertyManager->Deinitialize();
  std::lock_guard lock(m_mutex);
  for (auto& systemHealth : m_systemHealths)
    systemHealth.second.Deinitialize();

  m_systemHealths.clear();
}

bool CRos2SystemHealthManager::IsActive(const std::string& systemName,
                                        std::chrono::milliseconds timeout)
{
  std::lock_guard lock(m_mutex);
  auto it = m_systemHealths.find(systemName);
  if (it == m_systemHealths.end())
  {
    AddSystem(systemName);
    it = m_systemHealths.find(systemName);
  }

  if (timeout <= std::chrono::milliseconds::zero())
    timeout = ISystemHealthHUD::DefaultActiveTimeout();

  return it->second.IsActive(timeout);
}

CTemperature CRos2SystemHealthManager::CPUTemperature(const std::string& systemName)
{
  std::lock_guard lock(m_mutex);
  auto it = m_systemHealths.find(systemName);
  if (it == m_systemHealths.end())
  {
    AddSystem(systemName);
    it = m_systemHealths.find(systemName);
  }

  return it->second.CPUTemperature();
}

float CRos2SystemHealthManager::CPUUtilization(const std::string& systemName)
{
  std::lock_guard lock(m_mutex);
  auto it = m_systemHealths.find(systemName);
  if (it == m_systemHealths.end())
  {
    AddSystem(systemName);
    it = m_systemHealths.find(systemName);
  }

  return it->second.CPUUtilization();
}

double CRos2SystemHealthManager::CPUFrequencyHz(const std::string& systemName)
{
  std::lock_guard lock(m_mutex);
  auto it = m_systemHealths.find(systemName);
  if (it == m_systemHealths.end())
  {
    AddSystem(systemName);
    it = m_systemHealths.find(systemName);
  }

  return it->second.CPUFrequencyHz();
}

float CRos2SystemHealthManager::MemoryUtilization(const std::string& systemName)
{
  std::lock_guard lock(m_mutex);
  auto it = m_systemHealths.find(systemName);
  if (it == m_systemHealths.end())
  {
    AddSystem(systemName);
    it = m_systemHealths.find(systemName);
  }

  return it->second.MemoryUtilization();
}

unsigned int CRos2SystemHealthManager::BatteryCharge(const std::string& systemName)
{
  std::lock_guard lock(m_mutex);
  auto it = m_systemHealths.find(systemName);
  if (it == m_systemHealths.end())
  {
    AddSystem(systemName);
    it = m_systemHealths.find(systemName);
  }

  return it->second.BatteryCharge();
}

float CRos2SystemHealthManager::BatteryLoad(const std::string& systemName)
{
  std::lock_guard lock(m_mutex);
  auto it = m_systemHealths.find(systemName);
  if (it == m_systemHealths.end())
  {
    AddSystem(systemName);
    it = m_systemHealths.find(systemName);
  }

  return it->second.BatteryLoad();
}

std::optional<SmartHomePropertyValue> CRos2SystemHealthManager::Property(
    const std::string& systemName, const std::string& propertyName, SmartHomePropertyType type)
{
  {
    std::lock_guard lock(m_mutex);
    AddSystem(systemName);
  }
  return m_propertyManager->Property(systemName, propertyName, type);
}

void CRos2SystemHealthManager::MarkActive(const std::string& systemName)
{
  std::lock_guard lock(m_mutex);
  AddSystem(systemName);
  const auto it = m_systemHealths.find(systemName);
  if (it != m_systemHealths.end())
    it->second.MarkActive();
}

void CRos2SystemHealthManager::AddSystem(const std::string& systemName)
{
  if (!m_node)
    return;

  auto it = m_systemHealths.find(systemName);
  if (it == m_systemHealths.end())
  {
    // Add a new subscriber to the map
    m_systemHealths.emplace(systemName, m_rosNamespace);

    // Initialize subscriber
    m_systemHealths.at(systemName).Initialize(m_node, systemName);
  }
}
