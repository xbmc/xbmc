/*
 *  Copyright (C) 2024-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerTranslator.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/controllers/types/ControllerNode.h"
#include "games/ports/types/PortNode.h"
#include "utils/Digest.h"
#include "utils/StringUtils.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

/*!
 * Verify digest of an empty controller hub matches the digest of empty data
 */
TEST(TestControllerDigest, EmptyHub)
{
  const UTILITY::CDigest::Type type = UTILITY::CDigest::Type::MD5;

  CControllerHub hub;

  // Digest from API
  const std::string hubDigest = hub.GetDigest(type);

  // Manual digest of empty input
  UTILITY::CDigest manual{type};
  const std::string expected = manual.FinalizeRaw();

  EXPECT_EQ(hubDigest, expected);
}

/*!
 * Verify digest calculation for a simple controller tree
 */
TEST(TestControllerDigest, SimpleTree)
{
  const UTILITY::CDigest::Type type = UTILITY::CDigest::Type::MD5;

  // Build controller
  auto addonInfo = std::make_shared<ADDON::CAddonInfo>("game.controller.test",
                                                       ADDON::AddonType::GAME_CONTROLLER);
  auto controller = std::make_shared<CController>(addonInfo);

  // Controller node containing the controller and an empty hub
  CControllerNode node;
  node.SetController(controller);

  // Port accepting the controller
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID("1");
  port.SetCompatibleControllers({node});

  // Hub with the single port
  CControllerHub hub;
  hub.SetPorts({port});

  // Digests from API
  const std::string nodeDigest = node.GetDigest(type);
  const std::string portDigest = port.GetDigest(type);
  const std::string hubDigest = hub.GetDigest(type);

  // Manually compute node digest
  UTILITY::CDigest emptyHub{type};
  std::string emptyHubDigest = emptyHub.FinalizeRaw();

  UTILITY::CDigest manualNode{type};
  manualNode.Update(controller->ID());
  manualNode.Update(emptyHubDigest);
  const std::string expectedNodeDigest = manualNode.FinalizeRaw();
  EXPECT_EQ(nodeDigest, expectedNodeDigest);

  // Manually compute port digest
  UTILITY::CDigest manualPort{type};
  manualPort.Update(CControllerTranslator::TranslatePortType(PORT_TYPE::CONTROLLER));
  manualPort.Update("1");
  manualPort.Update(expectedNodeDigest);
  const std::string expectedPortDigest = manualPort.FinalizeRaw();
  EXPECT_EQ(portDigest, expectedPortDigest);

  // Manually compute hub digest
  UTILITY::CDigest manualHub{type};
  manualHub.Update(expectedPortDigest);
  const std::string expectedHubDigest = manualHub.FinalizeRaw();
  EXPECT_EQ(hubDigest, expectedHubDigest);
}

/*!
 * Verify digest calculation for a port with multiple controllers
 */
TEST(TestControllerDigest, MultiControllerPort)
{
  const UTILITY::CDigest::Type type = UTILITY::CDigest::Type::MD5;

  // Build controllers
  auto addonInfo1 =
      std::make_shared<ADDON::CAddonInfo>("game.controller.one", ADDON::AddonType::GAME_CONTROLLER);
  auto controller1 = std::make_shared<CController>(addonInfo1);

  auto addonInfo2 =
      std::make_shared<ADDON::CAddonInfo>("game.controller.two", ADDON::AddonType::GAME_CONTROLLER);
  auto controller2 = std::make_shared<CController>(addonInfo2);

  // Controller nodes containing the controllers and empty hubs
  CControllerNode node1;
  node1.SetController(controller1);

  CControllerNode node2;
  node2.SetController(controller2);

  // Port accepting both controllers
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID("1");
  port.SetCompatibleControllers({node1, node2});

  // Hub with the single port
  CControllerHub hub;
  hub.SetPorts({port});

  // Digests from API
  const std::string nodeDigest1 = node1.GetDigest(type);
  const std::string nodeDigest2 = node2.GetDigest(type);
  const std::string portDigest = port.GetDigest(type);
  const std::string hubDigest = hub.GetDigest(type);

  // Manual digest for empty hub used by controller nodes
  UTILITY::CDigest emptyHub{type};
  const std::string emptyHubDigest = emptyHub.FinalizeRaw();

  // Manually compute node digests
  UTILITY::CDigest manualNode1{type};
  manualNode1.Update(controller1->ID());
  manualNode1.Update(emptyHubDigest);
  const std::string expectedNodeDigest1 = manualNode1.FinalizeRaw();
  EXPECT_EQ(nodeDigest1, expectedNodeDigest1);

  UTILITY::CDigest manualNode2{type};
  manualNode2.Update(controller2->ID());
  manualNode2.Update(emptyHubDigest);
  const std::string expectedNodeDigest2 = manualNode2.FinalizeRaw();
  EXPECT_EQ(nodeDigest2, expectedNodeDigest2);

  // Manually compute port digest
  UTILITY::CDigest manualPort{type};
  manualPort.Update(CControllerTranslator::TranslatePortType(PORT_TYPE::CONTROLLER));
  manualPort.Update("1");
  manualPort.Update(expectedNodeDigest1);
  manualPort.Update(expectedNodeDigest2);
  const std::string expectedPortDigest = manualPort.FinalizeRaw();
  EXPECT_EQ(portDigest, expectedPortDigest);

  // Manually compute hub digest
  UTILITY::CDigest manualHub{type};
  manualHub.Update(expectedPortDigest);
  const std::string expectedHubDigest = manualHub.FinalizeRaw();
  EXPECT_EQ(hubDigest, expectedHubDigest);
}

/*!
 * Verify digest calculation for a nested controller hub tree
 */
TEST(TestControllerDigest, NestedHub)
{
  const UTILITY::CDigest::Type type = UTILITY::CDigest::Type::MD5;

  // Build controllers for both levels
  auto addonInfo1 = std::make_shared<ADDON::CAddonInfo>("game.controller.parent",
                                                        ADDON::AddonType::GAME_CONTROLLER);
  auto controller1 = std::make_shared<CController>(addonInfo1);

  auto addonInfo2 = std::make_shared<ADDON::CAddonInfo>("game.controller.child",
                                                        ADDON::AddonType::GAME_CONTROLLER);
  auto controller2 = std::make_shared<CController>(addonInfo2);

  // Leaf node with controller2 and empty hub
  CControllerNode nodeChild;
  nodeChild.SetController(controller2);

  // Port inside the nested hub
  CPortNode portChild;
  portChild.SetPortType(PORT_TYPE::CONTROLLER);
  portChild.SetPortID("2");
  portChild.SetCompatibleControllers({nodeChild});

  // Nested hub containing the inner port
  CControllerHub hubChild;
  hubChild.SetPorts({portChild});

  // Parent node contains controller1 and the nested hub
  CControllerNode nodeParent;
  nodeParent.SetController(controller1);
  nodeParent.SetHub(hubChild);

  // Port at the root accepting the parent node
  CPortNode portParent;
  portParent.SetPortType(PORT_TYPE::CONTROLLER);
  portParent.SetPortID("1");
  portParent.SetCompatibleControllers({nodeParent});

  // Root hub containing the outer port
  CControllerHub hubRoot;
  hubRoot.SetPorts({portParent});

  // Digests from API
  const std::string nodeDigestChild = nodeChild.GetDigest(type);
  const std::string portDigestChild = portChild.GetDigest(type);
  const std::string hubDigestChild = hubChild.GetDigest(type);
  const std::string nodeDigestParent = nodeParent.GetDigest(type);
  const std::string portDigestParent = portParent.GetDigest(type);
  const std::string hubDigestRoot = hubRoot.GetDigest(type);

  // Manual digest for empty hub used by the leaf node
  UTILITY::CDigest emptyHub{type};
  const std::string emptyHubDigest = emptyHub.FinalizeRaw();

  // Compute expected digests bottom-up
  UTILITY::CDigest manualNodeChild{type};
  manualNodeChild.Update(controller2->ID());
  manualNodeChild.Update(emptyHubDigest);
  const std::string expectedNodeChild = manualNodeChild.FinalizeRaw();
  EXPECT_EQ(nodeDigestChild, expectedNodeChild);

  UTILITY::CDigest manualPortChild{type};
  manualPortChild.Update(CControllerTranslator::TranslatePortType(PORT_TYPE::CONTROLLER));
  manualPortChild.Update("2");
  manualPortChild.Update(expectedNodeChild);
  const std::string expectedPortChild = manualPortChild.FinalizeRaw();
  EXPECT_EQ(portDigestChild, expectedPortChild);

  UTILITY::CDigest manualHubChild{type};
  manualHubChild.Update(expectedPortChild);
  const std::string expectedHubChild = manualHubChild.FinalizeRaw();
  EXPECT_EQ(hubDigestChild, expectedHubChild);

  UTILITY::CDigest manualNodeParent{type};
  manualNodeParent.Update(controller1->ID());
  manualNodeParent.Update(expectedHubChild);
  const std::string expectedNodeParent = manualNodeParent.FinalizeRaw();
  EXPECT_EQ(nodeDigestParent, expectedNodeParent);

  UTILITY::CDigest manualPortParent{type};
  manualPortParent.Update(CControllerTranslator::TranslatePortType(PORT_TYPE::CONTROLLER));
  manualPortParent.Update("1");
  manualPortParent.Update(expectedNodeParent);
  const std::string expectedPortParent = manualPortParent.FinalizeRaw();
  EXPECT_EQ(portDigestParent, expectedPortParent);

  UTILITY::CDigest manualHubRoot{type};
  manualHubRoot.Update(expectedPortParent);
  const std::string expectedHubRoot = manualHubRoot.FinalizeRaw();
  EXPECT_EQ(hubDigestRoot, expectedHubRoot);
}
