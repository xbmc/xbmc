/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "games/ports/types/PortNode.h"
#include "utils/Digest.h"

#include <string>

namespace tinyxml2
{
class XMLElement;
} // namespace tinyxml2

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief A branch in the controller tree
 */
class CControllerHub
{
public:
  CControllerHub() = default;
  CControllerHub(const CControllerHub& other) { *this = other; }
  CControllerHub(CControllerHub&& other) = default;
  CControllerHub& operator=(const CControllerHub& rhs);
  CControllerHub& operator=(CControllerHub&& rhs) noexcept;
  ~CControllerHub();

  void Clear();

  bool HasPorts() const { return !m_ports.empty(); }
  PortVec& GetPorts() { return m_ports; }
  const PortVec& GetPorts() const { return m_ports; }
  void SetPorts(PortVec ports);

  bool IsControllerAccepted(const std::string& controllerId) const;
  bool IsControllerAccepted(const std::string& portAddress, const std::string& controllerId) const;
  ControllerVector GetControllers() const;
  void GetControllers(ControllerVector& controllers) const;

  const CPortNode& GetPort(const std::string& address) const;

  /*!
   * \brief Get a list of ports that accept player input
   *
   * \param[out] inputPorts The list of input ports
   */
  void GetInputPorts(std::vector<std::string>& inputPorts) const;

  /*!
   * \brief Get a list of ports that accept keyboard input
   *
   * \param[out] keyboardPorts The list of keyboard ports
   */
  void GetKeyboardPorts(std::vector<std::string>& keyboardPorts) const;

  /*!
   * \brief Get a list of ports that accept mouse input
   *
   * \param[out] mousePorts The list of mouse ports
   */
  void GetMousePorts(std::vector<std::string>& mousePorts) const;

  // XML functions
  bool Serialize(tinyxml2::XMLElement& controllerElement) const;
  bool Deserialize(const tinyxml2::XMLElement& controllerElement);

  // Crypto functions
  std::string GetDigest(UTILITY::CDigest::Type digestType) const;

private:
  static const CPortNode& GetPortInternal(const PortVec& ports, const std::string& address);

  PortVec m_ports;
};
} // namespace GAME
} // namespace KODI
