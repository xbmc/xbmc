/*
 *  Copyright (C) 2024-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2VehicleManager.h"

#include "Ros2VehicleSubscriber.h"

using namespace KODI;
using namespace SMART_HOME;

CRos2VehicleManager::CRos2VehicleManager(std::string rosNamespace)
  : m_rosNamespace(std::move(rosNamespace))
{
}

CRos2VehicleManager::~CRos2VehicleManager() = default;

void CRos2VehicleManager::Initialize(std::shared_ptr<rclcpp::Node> node)
{
  m_node = std::move(node);
}

void CRos2VehicleManager::Deinitialize()
{
  for (auto& vehicle : m_vehicles)
    vehicle.second.Deinitialize();

  m_vehicles.clear();
}

float CRos2VehicleManager::ForwardSpeed(const std::string& vehicleName)
{
  auto it = m_vehicles.find(vehicleName);
  if (it == m_vehicles.end())
  {
    AddVehicle(vehicleName);
    it = m_vehicles.find(vehicleName);
  }

  return it->second.ForwardSpeed();
}

float CRos2VehicleManager::ForwardSpeedStdDev(const std::string& vehicleName)
{
  auto it = m_vehicles.find(vehicleName);
  if (it == m_vehicles.end())
  {
    AddVehicle(vehicleName);
    it = m_vehicles.find(vehicleName);
  }

  return it->second.ForwardSpeedStdDev();
}

float CRos2VehicleManager::Roll(const std::string& vehicleName)
{
  auto it = m_vehicles.find(vehicleName);
  if (it == m_vehicles.end())
  {
    AddVehicle(vehicleName);
    it = m_vehicles.find(vehicleName);
  }

  return it->second.Roll();
}

float CRos2VehicleManager::RollStdDev(const std::string& vehicleName)
{
  auto it = m_vehicles.find(vehicleName);
  if (it == m_vehicles.end())
  {
    AddVehicle(vehicleName);
    it = m_vehicles.find(vehicleName);
  }

  return it->second.RollStdDev();
}

float CRos2VehicleManager::Pitch(const std::string& vehicleName)
{
  auto it = m_vehicles.find(vehicleName);
  if (it == m_vehicles.end())
  {
    AddVehicle(vehicleName);
    it = m_vehicles.find(vehicleName);
  }

  return it->second.Pitch();
}

float CRos2VehicleManager::PitchStdDev(const std::string& vehicleName)
{
  auto it = m_vehicles.find(vehicleName);
  if (it == m_vehicles.end())
  {
    AddVehicle(vehicleName);
    it = m_vehicles.find(vehicleName);
  }

  return it->second.PitchStdDev();
}

float CRos2VehicleManager::Yaw(const std::string& vehicleName)
{
  auto it = m_vehicles.find(vehicleName);
  if (it == m_vehicles.end())
  {
    AddVehicle(vehicleName);
    it = m_vehicles.find(vehicleName);
  }

  return it->second.Yaw();
}

float CRos2VehicleManager::YawStdDev(const std::string& vehicleName)
{
  auto it = m_vehicles.find(vehicleName);
  if (it == m_vehicles.end())
  {
    AddVehicle(vehicleName);
    it = m_vehicles.find(vehicleName);
  }

  return it->second.YawStdDev();
}

float CRos2VehicleManager::Tilt(const std::string& vehicleName)
{
  auto it = m_vehicles.find(vehicleName);
  if (it == m_vehicles.end())
  {
    AddVehicle(vehicleName);
    it = m_vehicles.find(vehicleName);
  }

  return it->second.Tilt();
}

float CRos2VehicleManager::TiltStdDev(const std::string& vehicleName)
{
  auto it = m_vehicles.find(vehicleName);
  if (it == m_vehicles.end())
  {
    AddVehicle(vehicleName);
    it = m_vehicles.find(vehicleName);
  }

  return it->second.TiltStdDev();
}

void CRos2VehicleManager::AddVehicle(const std::string& vehicleName)
{
  if (!m_node)
    return;

  auto it = m_vehicles.find(vehicleName);
  if (it == m_vehicles.end())
  {
    // Add a new subscriber to the map
    m_vehicles.emplace(vehicleName, m_rosNamespace);

    // Initialize subscriber
    m_vehicles.at(vehicleName).Initialize(m_node, vehicleName);
  }
}
