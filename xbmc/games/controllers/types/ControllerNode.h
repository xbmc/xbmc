/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"

#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
class CControllerHub;

/*!
 * \brief Node in the controller tree
 *
 * The node identifies the controller profile, and optionally the available
 * controller ports.
 */
class CControllerNode
{
public:
  CControllerNode();
  CControllerNode(const CControllerNode& other) { *this = other; }
  CControllerNode(CControllerNode&& other) = default;
  CControllerNode& operator=(const CControllerNode& rhs);
  CControllerNode& operator=(CControllerNode&& rhs) noexcept;
  ~CControllerNode();

  void Clear();

  /*!
   * \brief Controller profile of this code
   *
   * \return Controller profile, or empty if this node is invalid
   *
   * \sa IsValid()
   */
  const ControllerPtr& GetController() const { return m_controller; }
  void SetController(ControllerPtr controller);

  void GetControllers(ControllerVector& controllers) const;

  /*!
   * \brief Address given to the node by the implementation
   */
  const std::string& GetAddress() const { return m_address; }
  void SetAddress(std::string address);

  /*!
   * \brief Collection of ports on this controller
   *
   * \return A hub with controller ports, or an empty hub if this controller
   *         has no available ports
   */
  const CControllerHub& GetHub() const { return *m_hub; }
  CControllerHub& GetHub() { return *m_hub; }
  void SetHub(CControllerHub hub);

  /*!
   * \brief Check if this node has a valid controller profile
   */
  bool IsValid() const { return static_cast<bool>(m_controller); }

  /*!
   * \brief Check to see if a controller is compatible with a controller port
   *
   * \param controllerId The ID of the controller
   *
   * \return True if the controller is compatible with a port, false otherwise
   */
  bool IsControllerAccepted(const std::string& controllerId) const;

  /*!
   * \brief Check to see if a controller is compatible with a controller port
   *
   * \param portAddress The port address
   * \param controllerId The ID of the controller
   *
   * \return True if the controller is compatible with a port, false otherwise
   */
  bool IsControllerAccepted(const std::string& portAddress, const std::string& controllerId) const;

  /*!
   * \brief Check if this node provides input
   */
  bool ProvidesInput() const;

private:
  ControllerPtr m_controller;
  std::string m_address;
  std::unique_ptr<CControllerHub> m_hub;
};

using ControllerNodeVec = std::vector<CControllerNode>;
} // namespace GAME
} // namespace KODI
