/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/types/ControllerTree.h"

#include <future>
#include <mutex>
#include <string>

namespace tinyxml2
{
class XMLElement;
}

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CPortManager
{
public:
  CPortManager();
  ~CPortManager();

  void Initialize(const std::string& profilePath);
  void Deinitialize();

  void SetControllerTree(const CControllerTree& controllerTree);
  void LoadXML();
  void SaveXMLAsync();
  void Clear();

  void ConnectController(const std::string& portAddress,
                         bool connected,
                         const std::string& controllerId = "");

  const CControllerTree& GetControllerTree() const { return m_controllerTree; }

private:
  static void DeserializePorts(const tinyxml2::XMLElement* pElement, PortVec& ports);
  static void DeserializePort(const tinyxml2::XMLElement* pElement, CPortNode& port);
  static void DeserializeControllers(const tinyxml2::XMLElement* pElement,
                                     ControllerNodeVec& controllers);
  static void DeserializeController(const tinyxml2::XMLElement* pElement,
                                    CControllerNode& controller);

  static void SerializePorts(tinyxml2::XMLElement& node, const PortVec& ports);
  static void SerializePort(tinyxml2::XMLElement& portNode, const CPortNode& port);
  static void SerializeControllers(tinyxml2::XMLElement& portNode,
                                   const ControllerNodeVec& controllers);
  static void SerializeController(tinyxml2::XMLElement& controllerNode,
                                  const CControllerNode& controller);

  static bool ConnectController(const std::string& portAddress,
                                bool connected,
                                const std::string& controllerId,
                                PortVec& ports);
  static bool ConnectController(const std::string& portAddress,
                                bool connected,
                                const std::string& controllerId,
                                CPortNode& port);
  static bool ConnectController(const std::string& portAddress,
                                bool connected,
                                const std::string& controllerId,
                                ControllerNodeVec& controllers);
  static bool ConnectController(const std::string& portAddress,
                                bool connected,
                                const std::string& controllerId,
                                CControllerNode& controller);

  static bool HasState(const CPortNode& port);
  static bool HasState(const CControllerNode& controller);

  CControllerTree m_controllerTree;
  std::string m_xmlPath;

  std::vector<std::future<void>> m_saveFutures;
  std::mutex m_saveMutex;
};
} // namespace GAME
} // namespace KODI
