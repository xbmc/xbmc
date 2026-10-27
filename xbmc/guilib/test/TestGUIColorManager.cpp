/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "filesystem/File.h"
#include "guilib/GUIColorManager.h"
#include "test/TestUtils.h"
#include "utils/XBMCTinyXML.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace
{
constexpr std::string_view COLORS{R"(<colors>
  <color name="red">ffff0000</color>
  <!-- comment -->
  <color name="empty"/>
  <color>ff00ff00</color>
  <color name="blue">ff0000ff</color>
</colors>)"};

class CTestColorManager : public CGUIColorManager
{
public:
  bool LoadXMLFile(const std::string& path)
  {
    CXBMCTinyXML xmlDoc;
    return xmlDoc.LoadFile(path) && LoadXML(xmlDoc);
  }
};

class TestGUIColorManager : public testing::Test
{
protected:
  void TearDown() override
  {
    for (XFILE::CFile* file : m_files)
      XBMC_DELETETEMPFILE(file);
  }

  std::string CreateFile(std::string_view content)
  {
    XFILE::CFile* file = XBMC_CREATETEMPFILE(".xml");
    m_files.push_back(file);
    file->Write(content.data(), content.size());
    file->Close();
    return XBMC_TEMPFILEPATH(file);
  }

  std::vector<XFILE::CFile*> m_files;
};
} // namespace

TEST_F(TestGUIColorManager, LoadXML)
{
  CTestColorManager colorManager;
  ASSERT_TRUE(colorManager.LoadXMLFile(CreateFile(COLORS)));
  EXPECT_EQ(0xffff0000, colorManager.GetColor("red"));
  EXPECT_EQ(0xff0000ff, colorManager.GetColor("blue"));

  ASSERT_TRUE(colorManager.LoadXMLFile(
      CreateFile("<colors><color name=\"red\">ff800000</color></colors>")));
  EXPECT_EQ(0xff800000, colorManager.GetColor("red"));
  EXPECT_EQ(0xff0000ff, colorManager.GetColor("blue"));
}

TEST_F(TestGUIColorManager, LoadXMLWrongRoot)
{
  CTestColorManager colorManager;
  EXPECT_FALSE(colorManager.LoadXMLFile(
      CreateFile("<colours><color name=\"red\">ffff0000</color></colours>")));
}

TEST_F(TestGUIColorManager, LoadColorsListFromXML)
{
  CGUIColorManager colorManager;
  std::vector<std::pair<std::string, KODI::UTILS::COLOR::ColorInfo>> colors;
  ASSERT_TRUE(colorManager.LoadColorsListFromXML(CreateFile(COLORS), colors, false));
  ASSERT_EQ(2u, colors.size());
  EXPECT_EQ("red", colors[0].first);
  EXPECT_EQ(0xffff0000, colors[0].second.colorARGB);
  EXPECT_EQ("blue", colors[1].first);
  EXPECT_EQ(0xff0000ff, colors[1].second.colorARGB);
}

TEST_F(TestGUIColorManager, LoadColorsListFromXMLMissingFile)
{
  CGUIColorManager colorManager;
  std::vector<std::pair<std::string, KODI::UTILS::COLOR::ColorInfo>> colors;
  EXPECT_FALSE(colorManager.LoadColorsListFromXML("/nonexistent/colors.xml", colors, false));
}
