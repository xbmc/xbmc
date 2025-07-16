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
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerIDs.h"
#include "games/controllers/ControllerTranslator.h"
#include "games/controllers/types/ControllerHub.h"
#include "games/controllers/types/ControllerNode.h"
#include "games/ports/types/PortNode.h"
#include "test/TestUtils.h"
#include "utils/XBMCTinyXML2.h"

#include <gtest/gtest.h>

using namespace KODI;
using namespace GAME;

/*!
 * Verify translation between port type strings and enums
 */
TEST(TestControllerTreeXML, TranslatePortType)
{
  // Ensure each known string maps to the correct enum value and back
  EXPECT_EQ(CControllerTranslator::TranslatePortType("keyboard"), PORT_TYPE::KEYBOARD);
  EXPECT_STREQ(CControllerTranslator::TranslatePortType(PORT_TYPE::KEYBOARD), "keyboard");

  EXPECT_EQ(CControllerTranslator::TranslatePortType("mouse"), PORT_TYPE::MOUSE);
  EXPECT_STREQ(CControllerTranslator::TranslatePortType(PORT_TYPE::MOUSE), "mouse");

  EXPECT_EQ(CControllerTranslator::TranslatePortType("controller"), PORT_TYPE::CONTROLLER);
  EXPECT_STREQ(CControllerTranslator::TranslatePortType(PORT_TYPE::CONTROLLER), "controller");

  // Unknown strings should map to UNKNOWN
  EXPECT_EQ(CControllerTranslator::TranslatePortType("unknown"), PORT_TYPE::UNKNOWN);
}

/*!
 * Serialize a minimal controller tree to XML
 *
 * The tree contains one controller port accepting the default controller.
 */
TEST(TestControllerTreeXML, SerializeSimpleTree)
{
  // Load the default controller add-on
  const ADDON::CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();
  ADDON::AddonPtr addon;
  ASSERT_TRUE(addonManager.GetAddon(DEFAULT_CONTROLLER_ID, addon, ADDON::AddonType::GAME_CONTROLLER,
                                    ADDON::OnlyEnabled::CHOICE_YES));
  ControllerPtr controller = std::static_pointer_cast<CController>(addon);
  ASSERT_NE(controller.get(), nullptr);

  // Build the controller node
  CControllerNode node;
  node.SetController(controller);

  // Build the port node accepting the controller
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID("1");
  port.SetCompatibleControllers({node});

  // Build the hub containing the port
  CControllerHub hub;
  hub.SetPorts({port});

  // Serialize to XML
  CXBMCTinyXML2 doc;
  auto* root = doc.NewElement("controller");
  ASSERT_TRUE(hub.Serialize(*root));
  doc.InsertFirstChild(root);

  tinyxml2::XMLPrinter printer;
  doc.Print(&printer);
  std::string xml = printer.CStr();

  // Validate the generated XML contains the expected attributes
  EXPECT_NE(xml.find("<port"), std::string::npos);
  EXPECT_NE(xml.find("type=\"controller\""), std::string::npos);
  EXPECT_NE(xml.find("id=\"1\""), std::string::npos);
  EXPECT_NE(xml.find("<accepts"), std::string::npos);
  EXPECT_NE(xml.find("controller=\"game.controller.default\""), std::string::npos);
}

/*!
 * Deserialize a simple controller tree from XML
 *
 * The XML contains one controller port accepting the default controller.
 */
TEST(TestControllerTreeXML, DeserializeSimpleTree)
{
  // Build an XML document representing a controller tree
  const char* xml = R"(<controller>
                         <port type="controller" id="1">
                           <accepts controller="game.controller.default"/>
                         </port>
                       </controller>)";
  std::string xmlStr{xml};
  CXBMCTinyXML2 doc;
  ASSERT_TRUE(doc.Parse(xmlStr));
  const tinyxml2::XMLElement* root = doc.RootElement();
  ASSERT_NE(root, nullptr);

  // Deserialize into a controller hub
  CControllerHub hub;
  ASSERT_TRUE(hub.Deserialize(*root));

  // Validate the hub contents
  ASSERT_EQ(hub.GetPorts().size(), 1u);
  const CPortNode& port = hub.GetPorts().front();
  EXPECT_EQ(port.GetPortType(), PORT_TYPE::CONTROLLER);
  EXPECT_EQ(port.GetPortID(), "1");
  ASSERT_EQ(port.GetCompatibleControllers().size(), 1u);
  const CControllerNode& node = port.GetCompatibleControllers().front();
  ASSERT_NE(node.GetController(), nullptr);
  EXPECT_EQ(node.GetController()->ID(), std::string(DEFAULT_CONTROLLER_ID));
}

/*!
 * Round trip a controller tree through serialization and deserialization
 */
TEST(TestControllerTreeXML, SerializeDeserializeRoundTrip)
{
  // Load the default controller add-on
  const ADDON::CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();
  ADDON::AddonPtr addon;
  ASSERT_TRUE(addonManager.GetAddon(DEFAULT_CONTROLLER_ID, addon, ADDON::AddonType::GAME_CONTROLLER,
                                    ADDON::OnlyEnabled::CHOICE_YES));
  ControllerPtr controller = std::static_pointer_cast<CController>(addon);
  ASSERT_NE(controller.get(), nullptr);

  // Build the original hub
  CControllerNode node;
  node.SetController(controller);
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID("1");
  port.SetCompatibleControllers({node});
  CControllerHub hub;
  hub.SetPorts({port});

  // Serialize the hub
  CXBMCTinyXML2 doc1;
  auto* root1 = doc1.NewElement("controller");
  ASSERT_TRUE(hub.Serialize(*root1));
  doc1.InsertFirstChild(root1);
  tinyxml2::XMLPrinter printer1;
  doc1.Print(&printer1);
  std::string xml = printer1.CStr();

  // Deserialize into a new hub
  CXBMCTinyXML2 doc2;
  ASSERT_TRUE(doc2.Parse(xml));
  const tinyxml2::XMLElement* root2 = doc2.RootElement();
  ASSERT_NE(root2, nullptr);
  CControllerHub hub2;
  ASSERT_TRUE(hub2.Deserialize(*root2));

  // Verify the deserialized hub matches the original
  ASSERT_EQ(hub2.GetPorts().size(), 1u);
  const CPortNode& port2 = hub2.GetPorts().front();
  EXPECT_EQ(port2.GetPortType(), PORT_TYPE::CONTROLLER);
  EXPECT_EQ(port2.GetPortID(), "1");
  ASSERT_EQ(port2.GetCompatibleControllers().size(), 1u);
  const CControllerNode& node2 = port2.GetCompatibleControllers().front();
  ASSERT_NE(node2.GetController(), nullptr);
  EXPECT_EQ(node2.GetController()->ID(), std::string(DEFAULT_CONTROLLER_ID));
}

/*!
 * Fail serialization for an unknown port type
 */
TEST(TestControllerTreeXML, SerializePortInvalidType)
{
  // Load the default controller add-on
  const ADDON::CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();
  ADDON::AddonPtr addon;
  ASSERT_TRUE(addonManager.GetAddon(DEFAULT_CONTROLLER_ID, addon, ADDON::AddonType::GAME_CONTROLLER,
                                    ADDON::OnlyEnabled::CHOICE_YES));
  ControllerPtr controller = std::static_pointer_cast<CController>(addon);
  ASSERT_NE(controller.get(), nullptr);

  // Build a port with no type set (defaults to UNKNOWN)
  CControllerNode node;
  node.SetController(controller);
  CPortNode port;
  port.SetPortID("1");
  port.SetCompatibleControllers({node});

  // Attempt serialization
  CXBMCTinyXML2 doc;
  auto* root = doc.NewElement("port");
  EXPECT_FALSE(port.Serialize(*root));
}

/*!
 * Fail serialization when a port is missing an id
 */
TEST(TestControllerTreeXML, SerializePortMissingID)
{
  // Load the default controller add-on
  const ADDON::CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();
  ADDON::AddonPtr addon;
  ASSERT_TRUE(addonManager.GetAddon(DEFAULT_CONTROLLER_ID, addon, ADDON::AddonType::GAME_CONTROLLER,
                                    ADDON::OnlyEnabled::CHOICE_YES));
  ControllerPtr controller = std::static_pointer_cast<CController>(addon);
  ASSERT_NE(controller.get(), nullptr);

  // Build a port with no id set
  CControllerNode node;
  node.SetController(controller);
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetCompatibleControllers({node});

  // Attempt serialization
  CXBMCTinyXML2 doc;
  auto* root = doc.NewElement("port");
  EXPECT_FALSE(port.Serialize(*root));
}

/*!
 * Fail serialization when a port has no accepted controllers
 */
TEST(TestControllerTreeXML, SerializePortNoControllers)
{
  // Build a port with type and id but no controllers
  CPortNode port;
  port.SetPortType(PORT_TYPE::CONTROLLER);
  port.SetPortID("1");

  // Attempt serialization
  CXBMCTinyXML2 doc;
  auto* root = doc.NewElement("port");
  EXPECT_FALSE(port.Serialize(*root));
}

/*!
 * Fail serialization when a controller node lacks a controller profile
 */
TEST(TestControllerTreeXML, SerializeControllerNodeMissingController)
{
  CControllerNode node;

  CXBMCTinyXML2 doc;
  auto* root = doc.NewElement("accepts");
  EXPECT_FALSE(node.Serialize(*root));
}

/*!
 * Fail deserialization when a port is missing an id
 */
TEST(TestControllerTreeXML, DeserializePortMissingID)
{
  // XML representing a port with a type but no id
  const char* xml = R"(<port type="controller">
                         <accepts controller="game.controller.default"/>
                       </port>)";
  std::string xmlStr{xml};
  CXBMCTinyXML2 doc;
  ASSERT_TRUE(doc.Parse(xmlStr));
  const tinyxml2::XMLElement* root = doc.RootElement();
  ASSERT_NE(root, nullptr);

  CPortNode port;
  EXPECT_FALSE(port.Deserialize(*root));
}

/*!
 * Fail deserialization when a controller node lacks the controller attribute
 */
TEST(TestControllerTreeXML, DeserializeControllerNodeMissingController)
{
  // XML node missing the controller attribute
  const char* xml = "<accepts/>";
  std::string xmlStr{xml};
  CXBMCTinyXML2 doc;
  ASSERT_TRUE(doc.Parse(xmlStr));
  const tinyxml2::XMLElement* root = doc.RootElement();
  ASSERT_NE(root, nullptr);

  CControllerNode node;
  EXPECT_FALSE(node.Deserialize(*root));
}

/*!
 * Fail deserialization when a controller references an unknown profile
 */
TEST(TestControllerTreeXML, DeserializeControllerNodeUnknownController)
{
  // XML node referencing a non-existent controller id
  const char* xml = "<accepts controller=\"game.controller.fake\"/>";
  std::string xmlStr{xml};
  CXBMCTinyXML2 doc;
  ASSERT_TRUE(doc.Parse(xmlStr));
  const tinyxml2::XMLElement* root = doc.RootElement();
  ASSERT_NE(root, nullptr);

  CControllerNode node;
  EXPECT_FALSE(node.Deserialize(*root));
}

/*!
 * Fail deserialization of a hub when a child port is invalid
 */
TEST(TestControllerTreeXML, DeserializeHubInvalidPort)
{
  // XML hub containing a port with no id
  const char* xml = "<controller>"
                    "<port type=\"controller\"/>"
                    "</controller>";
  std::string xmlStr{xml};
  CXBMCTinyXML2 doc;
  ASSERT_TRUE(doc.Parse(xmlStr));
  const tinyxml2::XMLElement* root = doc.RootElement();
  ASSERT_NE(root, nullptr);

  CControllerHub hub;
  EXPECT_FALSE(hub.Deserialize(*root));
}
