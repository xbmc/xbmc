/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "addons/Repository.h"
#include "addons/Skin.h"
#include "addons/addoninfo/AddonInfoBuilder.h"
#include "addons/addoninfo/AddonType.h"
#include "utils/XBMCTinyXML2.h"

#include <string>

#include <gtest/gtest.h>

using namespace ADDON;

namespace
{
class CSkinInfoTest : public CSkinInfo
{
public:
  using CSkinInfo::CSkinInfo;
  using CSkinInfo::m_resolutions;
};

AddonInfoPtr SkinWithAspect(const std::string& aspect)
{
  const std::string xml{R"xml(
<addon id="skin.test" name="Test" version="1.0.0" provider-name="Team Kodi">
  <extension point="xbmc.gui.skin">
    <res width="1920" height="1080" aspect=")xml" +
                        aspect + R"xml(" default="true" folder="xml" />
  </extension>
</addon>
)xml"};

  CXBMCTinyXML2 doc;
  EXPECT_TRUE(doc.Parse(xml));
  return CAddonInfoBuilder::Generate(doc.RootElement(), RepositoryDirInfo{});
}
} // namespace

TEST(TestSkin, ReadsTheDeclaredAspect)
{
  const CSkinInfoTest skin{SkinWithAspect("1.78")};

  ASSERT_EQ(skin.m_resolutions.size(), 1u);
  EXPECT_NEAR(skin.m_resolutions[0].DisplayRatio(), 16.0f / 9.0f, 0.001f);
}

TEST(TestSkin, AnAspectThatIsNoRatioLeavesThePixelShape)
{
  for (const char* aspect : {"-1.78", "0", "16:0", "-16:9"})
  {
    const CSkinInfoTest skin{SkinWithAspect(aspect)};

    ASSERT_EQ(skin.m_resolutions.size(), 1u) << aspect;
    EXPECT_FLOAT_EQ(skin.m_resolutions[0].fPixelRatio, 1.0f) << aspect;
  }
}
