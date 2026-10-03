/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUIImage.h"
#include "guilib/GUITexture.h"

#include <gtest/gtest.h>

namespace
{

class CTestGUITexture : public CGUITexture
{
public:
  CTestGUITexture(float posX, float posY, float width, float height, const CTextureInfo& texture)
    : CGUITexture(posX, posY, width, height, texture)
  {
  }
  CGUITexture* Clone() const override { return new CTestGUITexture(*this); }
  bool UsesCache() const { return m_use_cache; }

protected:
  void Begin(KODI::UTILS::COLOR::Color color) override {}
  void Draw(float* x,
            float* y,
            float* z,
            const CRect& texture,
            const CRect& diffuse,
            int orientation) override
  {
  }
  void End() override {}
};

class CTestGUIImage : public CGUIImage
{
public:
  using CGUIImage::CGUIImage;
  using CGUIImage::ProcessState;

  const CTestGUITexture& NextTexture() const
  {
    return static_cast<const CTestGUITexture&>(*m_textureNext);
  }
};

class TestGUIImage : public ::testing::Test
{
protected:
  void SetUp() override
  {
    CGUITexture::Register(
        [](float posX, float posY, float width, float height,
           const CTextureInfo& texture) -> CGUITexture*
        { return new CTestGUITexture(posX, posY, width, height, texture); },
        [](const CRect&, KODI::UTILS::COLOR::Color, CTexture*, const CRect*, float, bool) {});
  }
};

} // namespace

TEST_F(TestGUIImage, SetFileNameAppliesUseCache)
{
  CTestGUIImage image(0, 1, 0, 0, 100, 100, CTextureInfo());

  image.SetFileName("http://localhost/photo.jpg", false, false);
  image.ProcessState();

  EXPECT_EQ(image.NextTexture().GetFileName(), "http://localhost/photo.jpg");
  EXPECT_FALSE(image.NextTexture().UsesCache());
}

TEST_F(TestGUIImage, SetFileNameAppliesUseCacheToPendingTexture)
{
  CTestGUIImage image(0, 1, 0, 0, 100, 100, CTextureInfo());

  image.SetFileName("http://localhost/photo.jpg");
  image.ProcessState();
  image.SetFileName("http://localhost/photo.jpg", false, false);
  image.ProcessState();

  EXPECT_EQ(image.NextTexture().GetFileName(), "http://localhost/photo.jpg");
  EXPECT_FALSE(image.NextTexture().UsesCache());
}

TEST_F(TestGUIImage, SetFileNameUsesCacheByDefault)
{
  CTestGUIImage image(0, 1, 0, 0, 100, 100, CTextureInfo());

  image.SetFileName("http://localhost/photo.jpg", false, false);
  image.ProcessState();
  image.SetFileName("http://localhost/other.jpg");
  image.ProcessState();

  EXPECT_EQ(image.NextTexture().GetFileName(), "http://localhost/other.jpg");
  EXPECT_TRUE(image.NextTexture().UsesCache());
}
