/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/game.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/types/ControllerNode.h"
#include "games/controllers/types/ControllerTree.h"
#include "games/ports/input/PortManager.h"
#include "games/ports/types/PortNode.h"
#include "utils/URIUtils.h"

#include <initializer_list>
#include <string>

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

namespace
{
constexpr auto TEMP_PROFILE_DIRECTORY = "special://temp/test-port-manager";

ControllerPtr LoadController(const std::string& controllerId)
{
  ADDON::AddonPtr addon;
  EXPECT_TRUE(CServiceBroker::GetAddonMgr().GetAddon(
      controllerId, addon, ADDON::AddonType::GAME_CONTROLLER, ADDON::OnlyEnabled::CHOICE_YES));
  return std::static_pointer_cast<CController>(addon);
}

CControllerTree MakeTree(std::initializer_list<const char*> controllerIds)
{
  ControllerNodeVec controllers;
  for (const char* controllerId : controllerIds)
  {
    CControllerNode node;
    node.SetController(LoadController(controllerId));
    controllers.emplace_back(std::move(node));
  }

  // One port, connected to its first controller as the default, if it has one
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID(DEFAULT_PORT_ID);
  port.SetAddress(DEFAULT_PORT_ADDRESS);
  port.SetConnected(!controllers.empty());
  port.SetCompatibleControllers(std::move(controllers));
  port.SetActiveController(0);

  CControllerTree tree;
  tree.SetPorts({std::move(port)});
  return tree;
}

// Kodi writes no controller attribute for a disconnected port, so an empty ID leaves it out
CPortNode LoadPort(const std::string& connected,
                   const std::string& controllerId,
                   std::initializer_list<const char*> compatible = {DEFAULT_CONTROLLER_ID,
                                                                    DEFAULT_MOUSE_ID})
{
  EXPECT_TRUE(XFILE::CDirectory::Create(TEMP_PROFILE_DIRECTORY));
  std::string xml = "<ports><port id=\"" + std::string{DEFAULT_PORT_ID} + "\" address=\"" +
                    DEFAULT_PORT_ADDRESS + "\" connected=\"" + connected + "\"";
  if (!controllerId.empty())
    xml += " controller=\"" + controllerId + "\"";
  xml += "/></ports>";
  XFILE::CFile file;
  EXPECT_TRUE(
      file.OpenForWrite(URIUtils::AddFileToFolder(TEMP_PROFILE_DIRECTORY, "ports.xml"), true));
  EXPECT_EQ(file.Write(xml.data(), xml.size()), static_cast<ssize_t>(xml.size()));
  file.Close();

  CPortManager portManager;
  portManager.Initialize(TEMP_PROFILE_DIRECTORY);
  portManager.SetControllerTree(MakeTree(compatible));
  portManager.LoadXML();
  CPortNode port = portManager.GetControllerTree().GetPorts().front();
  portManager.Deinitialize();

  XFILE::CFile::Delete(URIUtils::AddFileToFolder(TEMP_PROFILE_DIRECTORY, "ports.xml"));
  XFILE::CDirectory::Remove(TEMP_PROFILE_DIRECTORY);

  return port;
}

std::string ActiveControllerId(const CPortNode& port)
{
  const ControllerPtr& controller = port.GetActiveController().GetController();
  return controller ? controller->ID() : "";
}
} // namespace

TEST(TestPortManager, SavedControllerIsRestored)
{
  const CPortNode port = LoadPort("true", DEFAULT_MOUSE_ID);

  EXPECT_TRUE(port.IsConnected());
  EXPECT_EQ(ActiveControllerId(port), DEFAULT_MOUSE_ID);
}

TEST(TestPortManager, ControllerThePortNoLongerAcceptsKeepsTheDefault)
{
  // Saved before the game add-on had a topology, so the port no longer accepts it
  const CPortNode port = LoadPort("true", DEFAULT_KEYBOARD_ID);

  EXPECT_TRUE(port.IsConnected());
  EXPECT_EQ(ActiveControllerId(port), DEFAULT_CONTROLLER_ID);
}

TEST(TestPortManager, DisconnectedPortStaysDisconnected)
{
  const CPortNode port = LoadPort("false", "");

  EXPECT_FALSE(port.IsConnected());
}

TEST(TestPortManager, PortWithNoControllersStaysDisconnected)
{
  // Every controller the port accepts is missing, so there's no default to use
  const CPortNode port = LoadPort("true", DEFAULT_CONTROLLER_ID, {});

  EXPECT_FALSE(port.IsConnected());
}
