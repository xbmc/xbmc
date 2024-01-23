/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "games/addons/input/GameClientPort.h"
#include "games/addons/input/GameClientTopology.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/types/ControllerTree.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

TEST(TestGameClientTopology, Construtor)
{
  //
  // Spec: Test default constructor
  //

  const CGameClientTopology topology;

  EXPECT_EQ(topology.GetPlayerLimit(), -1);
  EXPECT_EQ(topology.GetControllerTree().GetPorts().size(), 0);

  //
  // Spec: Test constructor with parameters
  //

  CGameClientTopology topology2{{}, 4};

  EXPECT_EQ(topology2.GetPlayerLimit(), 4);
  EXPECT_EQ(topology2.GetControllerTree().GetPorts().size(), 0);

  //
  // Spec: Test clearing the topology
  //

  topology2.Clear();

  EXPECT_EQ(topology2.GetPlayerLimit(), 4);
  EXPECT_EQ(topology2.GetControllerTree().GetPorts().size(), 0);
}

TEST(TestGameClientTopology, MakeAddress)
{
  //
  // Spec: Test root address
  //

  EXPECT_EQ(CGameClientTopology::MakeAddress("", ""), ROOT_PORT_ADDRESS);
  EXPECT_EQ(CGameClientTopology::MakeAddress(ROOT_PORT_ADDRESS, ""), ROOT_PORT_ADDRESS);
  EXPECT_EQ(CGameClientTopology::MakeAddress("//", "//"), ROOT_PORT_ADDRESS);

  //
  // Spec: Test default port address
  //

  EXPECT_EQ(CGameClientTopology::MakeAddress("", DEFAULT_PORT_ID), DEFAULT_PORT_ADDRESS);
  EXPECT_EQ(CGameClientTopology::MakeAddress(ROOT_PORT_ADDRESS, DEFAULT_PORT_ID),
            DEFAULT_PORT_ADDRESS);
  EXPECT_EQ(CGameClientTopology::MakeAddress("//", "//1//"), DEFAULT_PORT_ADDRESS);

  //
  // Spec: Test controller address
  //

  EXPECT_EQ(CGameClientTopology::MakeAddress(DEFAULT_PORT_ADDRESS, DEFAULT_CONTROLLER_ID),
            "/1/game.controller.default");

  //
  // Spec: Test multitaps
  //

  const std::string defaultPort =
      CGameClientTopology::MakeAddress(ROOT_PORT_ADDRESS, DEFAULT_PORT_ID);
  EXPECT_EQ(defaultPort, DEFAULT_PORT_ADDRESS);

  const std::string multitapAddress =
      CGameClientTopology::MakeAddress(defaultPort, "game.controller.snes.multitap");
  EXPECT_EQ(multitapAddress, "/1/game.controller.snes.multitap");

  const std::string multitapPort = CGameClientTopology::MakeAddress(multitapAddress, "2");
  EXPECT_EQ(multitapPort, "/1/game.controller.snes.multitap/2");

  const std::string multitapController =
      CGameClientTopology::MakeAddress(multitapPort, "game.controller.snes");
  EXPECT_EQ(multitapController, "/1/game.controller.snes.multitap/2/game.controller.snes");
}

TEST(TestGameClientTopology, SplitAddress)
{
  //
  // Spec: Test root address
  //

  const auto rootAddress1 = CGameClientTopology::SplitAddress("");
  EXPECT_EQ(rootAddress1.first, ROOT_PORT_ADDRESS);
  EXPECT_EQ(rootAddress1.second, "");

  const auto rootAddress2 = CGameClientTopology::SplitAddress(ROOT_PORT_ADDRESS);
  EXPECT_EQ(rootAddress2.first, ROOT_PORT_ADDRESS);
  EXPECT_EQ(rootAddress2.second, "");

  //
  // Spec: Test default port address
  //

  const auto defaultPortAddress = CGameClientTopology::SplitAddress(DEFAULT_PORT_ADDRESS);
  EXPECT_EQ(defaultPortAddress.first, ROOT_PORT_ADDRESS);
  EXPECT_EQ(defaultPortAddress.second, DEFAULT_PORT_ID);

  //
  // Spec: Test controller address
  //

  const auto controllerAddress = CGameClientTopology::SplitAddress("/1/game.controller.default");
  EXPECT_EQ(controllerAddress.first, DEFAULT_PORT_ADDRESS);
  EXPECT_EQ(controllerAddress.second, DEFAULT_CONTROLLER_ID);

  //
  // Spec: Test multitaps
  //

  const auto multitapAddress =
      CGameClientTopology::SplitAddress("/1/game.controller.snes.multitap/2");
  EXPECT_EQ(multitapAddress.first, "/1/game.controller.snes.multitap");
  EXPECT_EQ(multitapAddress.second, "2");

  const auto multitapController =
      CGameClientTopology::SplitAddress("/1/game.controller.snes.multitap/2/game.controller.snes");
  EXPECT_EQ(multitapController.first, "/1/game.controller.snes.multitap/2");
  EXPECT_EQ(multitapController.second, "game.controller.snes");
}

TEST(TestGameClientTopology, TestKeyboard)
{
  //
  // Spec: Test keyboard address
  //

  const auto keyboardAddress = CGameClientTopology::SplitAddress(KEYBOARD_PORT_ADDRESS);
  EXPECT_EQ(keyboardAddress.first, ROOT_PORT_ADDRESS);
  EXPECT_EQ(keyboardAddress.second, KEYBOARD_PORT_ID);

  EXPECT_EQ(CGameClientTopology::MakeAddress(KEYBOARD_PORT_ADDRESS, DEFAULT_KEYBOARD_ID),
            "/keyboard/game.controller.keyboard");

  //
  // Spec: Test keyboard controller
  //

  const auto keyboardController =
      CGameClientTopology::SplitAddress("/keyboard/game.controller.keyboard");
  EXPECT_EQ(keyboardController.first, KEYBOARD_PORT_ADDRESS);
  EXPECT_EQ(keyboardController.second, DEFAULT_KEYBOARD_ID);
}

TEST(TestGameClientTopology, TestMouse)
{
  //
  // Spec: Test mouse address
  //

  const auto mouseAddress = CGameClientTopology::SplitAddress(MOUSE_PORT_ADDRESS);
  EXPECT_EQ(mouseAddress.first, ROOT_PORT_ADDRESS);
  EXPECT_EQ(mouseAddress.second, MOUSE_PORT_ID);

  EXPECT_EQ(CGameClientTopology::MakeAddress(MOUSE_PORT_ADDRESS, DEFAULT_MOUSE_ID),
            "/mouse/game.controller.mouse");

  //
  // Spec: Test mouse controller
  //

  const auto mouseController = CGameClientTopology::SplitAddress("/mouse/game.controller.mouse");
  EXPECT_EQ(mouseController.first, MOUSE_PORT_ADDRESS);
  EXPECT_EQ(mouseController.second, DEFAULT_MOUSE_ID);
}
