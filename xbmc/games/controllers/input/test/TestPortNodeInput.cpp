/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/controllers/types/ControllerNode.h"
#include "games/ports/types/PortNode.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
ControllerPtr LoadController(const std::string& controllerId)
{
  ADDON::AddonPtr addon;
  EXPECT_TRUE(CServiceBroker::GetAddonMgr().GetAddon(
      controllerId, addon, ADDON::AddonType::GAME_CONTROLLER, ADDON::OnlyEnabled::CHOICE_YES));

  ControllerPtr controller = std::static_pointer_cast<CController>(addon);
  EXPECT_TRUE(controller && controller->LoadLayout());

  return controller;
}

CPortNode MakeControllerPort(const std::string& address, CControllerNode controller, bool connected)
{
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID(address.substr(address.rfind('/') + 1));
  port.SetAddress(address);
  port.SetCompatibleControllers({std::move(controller)});
  port.SetConnected(connected);
  port.SetActiveController(0);
  return port;
}

CControllerNode MakeControllerNode(const ControllerPtr& controller)
{
  CControllerNode node;
  node.SetController(controller);
  return node;
}
} // namespace

/*!
 * Verify that a keyboard port at the root level is returned by GetKeyboardPorts.
 */
TEST(TestPortNodeInput, RootKeyboard)
{
  // Build a hub containing a single keyboard port
  CPortNode keyboardPort;
  keyboardPort.SetPortType(PORT_TYPE::KEYBOARD);
  keyboardPort.SetPortID(KEYBOARD_PORT_ID);
  keyboardPort.SetAddress(KEYBOARD_PORT_ADDRESS);

  CControllerHub hub;
  hub.SetPorts({keyboardPort});

  // Query keyboard ports
  std::vector<std::string> keyboardPorts;
  hub.GetKeyboardPorts(keyboardPorts);

  ASSERT_EQ(keyboardPorts.size(), 1u);
  EXPECT_EQ(keyboardPorts.front(), KEYBOARD_PORT_ADDRESS);
}

/*!
 * Verify that a mouse port at the root level is returned by GetMousePorts.
 */
TEST(TestPortNodeInput, RootMouse)
{
  // Build a hub containing a single mouse port
  CPortNode mousePort;
  mousePort.SetPortType(PORT_TYPE::MOUSE);
  mousePort.SetPortID(MOUSE_PORT_ID);
  mousePort.SetAddress(MOUSE_PORT_ADDRESS);

  CControllerHub hub;
  hub.SetPorts({mousePort});

  // Query mouse ports
  std::vector<std::string> mousePorts;
  hub.GetMousePorts(mousePorts);

  ASSERT_EQ(mousePorts.size(), 1u);
  EXPECT_EQ(mousePorts.front(), MOUSE_PORT_ADDRESS);
}

/*!
 * Verify that nested keyboard ports are discovered through connected controllers.
 */
TEST(TestPortNodeInput, NestedKeyboard)
{
  // Leaf keyboard port inside a controller hub
  CPortNode keyboardPort;
  keyboardPort.SetPortType(PORT_TYPE::KEYBOARD);
  keyboardPort.SetPortID(KEYBOARD_PORT_ID);
  keyboardPort.SetAddress(std::string(DEFAULT_PORT_ADDRESS) + "/" + KEYBOARD_PORT_ID);

  CControllerHub childHub;
  childHub.SetPorts({keyboardPort});

  // Controller node exposing the child hub
  CControllerNode controllerNode;
  controllerNode.SetHub(childHub);

  // Root port connected to the controller node
  CPortNode rootPort;
  rootPort.SetPortType(PORT_TYPE::CONTROLLER);
  rootPort.SetPortID(DEFAULT_PORT_ID);
  rootPort.SetAddress(DEFAULT_PORT_ADDRESS);
  rootPort.SetCompatibleControllers({controllerNode});
  rootPort.SetConnected(true);
  rootPort.SetActiveController(0);

  CControllerHub rootHub;
  rootHub.SetPorts({rootPort});

  // Query keyboard ports from the root
  std::vector<std::string> keyboardPorts;
  rootHub.GetKeyboardPorts(keyboardPorts);

  ASSERT_EQ(keyboardPorts.size(), 1u);
  EXPECT_EQ(keyboardPorts.front(), std::string(DEFAULT_PORT_ADDRESS) + "/" + KEYBOARD_PORT_ID);
}

/*!
 * Verify that nested mouse ports are discovered through connected controllers.
 */
TEST(TestPortNodeInput, NestedMouse)
{
  // Leaf mouse port inside a controller hub
  CPortNode mousePort;
  mousePort.SetPortType(PORT_TYPE::MOUSE);
  mousePort.SetPortID(MOUSE_PORT_ID);
  mousePort.SetAddress(std::string(DEFAULT_PORT_ADDRESS) + "/" + MOUSE_PORT_ID);

  CControllerHub childHub;
  childHub.SetPorts({mousePort});

  // Controller node exposing the child hub
  CControllerNode controllerNode;
  controllerNode.SetHub(childHub);

  // Root port connected to the controller node
  CPortNode rootPort;
  rootPort.SetPortType(PORT_TYPE::CONTROLLER);
  rootPort.SetPortID(DEFAULT_PORT_ID);
  rootPort.SetAddress(DEFAULT_PORT_ADDRESS);
  rootPort.SetCompatibleControllers({controllerNode});
  rootPort.SetConnected(true);
  rootPort.SetActiveController(0);

  CControllerHub rootHub;
  rootHub.SetPorts({rootPort});

  // Query mouse ports from the root
  std::vector<std::string> mousePorts;
  rootHub.GetMousePorts(mousePorts);

  ASSERT_EQ(mousePorts.size(), 1u);
  EXPECT_EQ(mousePorts.front(), std::string(DEFAULT_PORT_ADDRESS) + "/" + MOUSE_PORT_ID);
}

/*!
 * Verify that only a connected controller port with a mouse connected is returned
 * by GetMouseControllerPorts.
 */
TEST(TestPortNodeInput, MouseControllerPorts)
{
  const ControllerPtr mouse = LoadController(DEFAULT_MOUSE_ID);
  const ControllerPtr gamepad = LoadController(DEFAULT_CONTROLLER_ID);

  CPortNode mousePort;
  mousePort.SetPortType(PORT_TYPE::MOUSE);
  mousePort.SetPortID(MOUSE_PORT_ID);
  mousePort.SetAddress(MOUSE_PORT_ADDRESS);
  mousePort.SetCompatibleControllers({MakeControllerNode(mouse)});
  mousePort.SetConnected(true);
  mousePort.SetActiveController(0);

  CControllerHub hub;
  hub.SetPorts({MakeControllerPort("/1", MakeControllerNode(gamepad), true),
                MakeControllerPort("/2", MakeControllerNode(mouse), true),
                MakeControllerPort("/3", MakeControllerNode(mouse), false), mousePort});

  std::vector<std::string> mouseControllerPorts;
  hub.GetMouseControllerPorts(mouseControllerPorts);

  ASSERT_EQ(mouseControllerPorts.size(), 1u);
  EXPECT_EQ(mouseControllerPorts.front(), "/2");
}

/*!
 * Verify that a mouse connected to a port of a connected controller, such as a
 * multitap, is discovered.
 */
TEST(TestPortNodeInput, NestedMouseControllerPort)
{
  const ControllerPtr mouse = LoadController(DEFAULT_MOUSE_ID);
  const ControllerPtr gamepad = LoadController(DEFAULT_CONTROLLER_ID);

  CControllerHub tapHub;
  tapHub.SetPorts({MakeControllerPort("/1/tap/2", MakeControllerNode(mouse), true)});

  CControllerNode tap = MakeControllerNode(gamepad);
  tap.SetHub(tapHub);

  CControllerHub hub;
  hub.SetPorts({MakeControllerPort("/1", std::move(tap), true)});

  std::vector<std::string> mouseControllerPorts;
  hub.GetMouseControllerPorts(mouseControllerPorts);

  ASSERT_EQ(mouseControllerPorts.size(), 1u);
  EXPECT_EQ(mouseControllerPorts.front(), "/1/tap/2");
}
