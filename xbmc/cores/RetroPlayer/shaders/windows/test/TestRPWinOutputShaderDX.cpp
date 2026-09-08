/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/shaders/windows/RPWinOutputShader.h"
#include "cores/RetroPlayer/shaders/windows/ShaderDX.h"
#include "cores/RetroPlayer/shaders/windows/ShaderTextureDX.h"
#include "cores/RetroPlayer/shaders/windows/ShaderTypesDX.h"

#include <array>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <windows.h>

using namespace KODI;
using namespace KODI::SHADER;

namespace
{
class CTestOutputShader : public CRPWinOutputShader
{
public:
  bool createVertexBufferResult{true};
  bool lockVertexBufferResult{true};
  bool unlockVertexBufferResult{true};

  unsigned int createVertexBufferCalls{0};
  unsigned int loadEffectCalls{0};
  unsigned int createInputLayoutCalls{0};
  unsigned int lockVertexBufferCalls{0};
  unsigned int unlockVertexBufferCalls{0};
  unsigned int executeCalls{0};

protected:
  bool CreateVertexBuffer(unsigned int vertCount, unsigned int vertSize) override
  {
    ++createVertexBufferCalls;
    return createVertexBufferResult;
  }

  bool LoadEffect(const std::string& filename, DefinesMap* defines) override
  {
    ++loadEffectCalls;
    return true;
  }

  bool CreateInputLayout(D3D11_INPUT_ELEMENT_DESC* layout,
                         unsigned int numElements,
                         const char* techniqueName) override
  {
    ++createInputLayoutCalls;
    return true;
  }

  bool LockVertexBuffer(void** data) override
  {
    ++lockVertexBufferCalls;
    if (!lockVertexBufferResult)
      return false;

    *data = m_vertices.data();
    return true;
  }

  bool UnlockVertexBuffer() override
  {
    ++unlockVertexBufferCalls;
    return unlockVertexBufferResult;
  }

  bool Execute(const std::vector<CD3DTexture*>& targets, unsigned int vertexIndexStep) override
  {
    ++executeCalls;
    return true;
  }

private:
  std::array<CUSTOMVERTEX, 4> m_vertices{};
};

class CTestShaderDX : public CShaderDX
{
public:
  bool lockVertexBufferResult{true};
  bool unlockVertexBufferResult{true};
  bool updateInputBufferResult{true};

  unsigned int lockVertexBufferCalls{0};
  unsigned int unlockVertexBufferCalls{0};
  unsigned int updateInputBufferCalls{0};

protected:
  bool LockVertexBuffer(void** data) override
  {
    ++lockVertexBufferCalls;
    if (!lockVertexBufferResult)
      return false;

    *data = m_vertices.data();
    return true;
  }

  bool UnlockVertexBuffer() override
  {
    ++unlockVertexBufferCalls;
    return unlockVertexBufferResult;
  }

  bool UpdateInputBuffer(uint64_t frameCount) override
  {
    ++updateInputBufferCalls;
    return updateInputBufferResult;
  }

private:
  std::array<CUSTOMVERTEX, 4> m_vertices{};
};
} // namespace

TEST(TestRPWinOutputShaderDX, CreateStopsWhenVertexBufferCreationFails)
{
  CTestOutputShader shader;
  shader.createVertexBufferResult = false;

  EXPECT_FALSE(shader.Create(RETRO::SCALINGMETHOD::LINEAR));
  EXPECT_EQ(1u, shader.createVertexBufferCalls);
  EXPECT_EQ(0u, shader.loadEffectCalls);
  EXPECT_EQ(0u, shader.createInputLayoutCalls);
}

TEST(TestRPWinOutputShaderDX, RenderStopsAndRetriesWhenVertexBufferLockFails)
{
  CTestOutputShader shader;
  shader.lockVertexBufferResult = false;

  CD3DTexture sourceTexture;
  CD3DTexture targetTexture;
  RETRO::ViewportCoordinates points{};
  points[0].x = 1.0f;
  CRect viewPort;

  shader.Render(sourceTexture, CRect{}, points, viewPort, targetTexture);
  EXPECT_EQ(0u, shader.executeCalls);
  EXPECT_EQ(0u, shader.unlockVertexBufferCalls);

  shader.lockVertexBufferResult = true;
  shader.Render(sourceTexture, CRect{}, points, viewPort, targetTexture);
  shader.Render(sourceTexture, CRect{}, points, viewPort, targetTexture);

  EXPECT_EQ(2u, shader.lockVertexBufferCalls);
  EXPECT_EQ(1u, shader.unlockVertexBufferCalls);
  EXPECT_EQ(2u, shader.executeCalls);
}

TEST(TestRPWinOutputShaderDX, RenderStopsAndRetriesWhenVertexBufferUnlockFails)
{
  CTestOutputShader shader;
  shader.unlockVertexBufferResult = false;

  CD3DTexture sourceTexture;
  CD3DTexture targetTexture;
  RETRO::ViewportCoordinates points{};
  points[0].x = 1.0f;
  CRect viewPort;

  shader.Render(sourceTexture, CRect{}, points, viewPort, targetTexture);
  shader.Render(sourceTexture, CRect{}, points, viewPort, targetTexture);

  EXPECT_EQ(2u, shader.lockVertexBufferCalls);
  EXPECT_EQ(2u, shader.unlockVertexBufferCalls);
  EXPECT_EQ(0u, shader.executeCalls);

  shader.unlockVertexBufferResult = true;
  shader.Render(sourceTexture, CRect{}, points, viewPort, targetTexture);
  EXPECT_EQ(3u, shader.lockVertexBufferCalls);
  EXPECT_EQ(3u, shader.unlockVertexBufferCalls);
  EXPECT_EQ(1u, shader.executeCalls);

  shader.Render(sourceTexture, CRect{}, points, viewPort, targetTexture);
  EXPECT_EQ(3u, shader.lockVertexBufferCalls);
  EXPECT_EQ(3u, shader.unlockVertexBufferCalls);
  EXPECT_EQ(2u, shader.executeCalls);
}

TEST(TestShaderDX, PrepareParametersFailsWhenVertexBufferLockFails)
{
  CTestShaderDX shader;
  shader.SetSizes({320.0f, 240.0f});
  shader.lockVertexBufferResult = false;

  CShaderTextureDX source(std::make_shared<CD3DTexture>());
  std::vector<std::unique_ptr<IShaderTexture>> shaderTextures;
  std::vector<std::unique_ptr<IShader>> shaders;

  EXPECT_FALSE(shader.PrepareParameters(source, shaderTextures, shaders, 1));
  EXPECT_EQ(1u, shader.lockVertexBufferCalls);
  EXPECT_EQ(0u, shader.unlockVertexBufferCalls);
  EXPECT_EQ(0u, shader.updateInputBufferCalls);
}

TEST(TestShaderDX, PrepareParametersFailsWhenVertexBufferUnlockFails)
{
  CTestShaderDX shader;
  shader.SetSizes({320.0f, 240.0f});
  shader.unlockVertexBufferResult = false;

  CShaderTextureDX source(std::make_shared<CD3DTexture>());
  std::vector<std::unique_ptr<IShaderTexture>> shaderTextures;
  std::vector<std::unique_ptr<IShader>> shaders;

  EXPECT_FALSE(shader.PrepareParameters(source, shaderTextures, shaders, 1));
  EXPECT_EQ(1u, shader.lockVertexBufferCalls);
  EXPECT_EQ(1u, shader.unlockVertexBufferCalls);
  EXPECT_EQ(0u, shader.updateInputBufferCalls);
}

TEST(TestShaderDX, PrepareParametersFailsWhenInputBufferUpdateFails)
{
  CTestShaderDX shader;
  shader.SetSizes({320.0f, 240.0f});
  shader.updateInputBufferResult = false;

  CShaderTextureDX source(std::make_shared<CD3DTexture>());
  std::vector<std::unique_ptr<IShaderTexture>> shaderTextures;
  std::vector<std::unique_ptr<IShader>> shaders;

  EXPECT_FALSE(shader.PrepareParameters(source, shaderTextures, shaders, 1));
  EXPECT_EQ(1u, shader.lockVertexBufferCalls);
  EXPECT_EQ(1u, shader.unlockVertexBufferCalls);
  EXPECT_EQ(1u, shader.updateInputBufferCalls);
}

TEST(TestShaderDX, PrepareParametersSucceedsWhenAllUpdatesSucceed)
{
  CTestShaderDX shader;
  shader.SetSizes({320.0f, 240.0f});

  CShaderTextureDX source(std::make_shared<CD3DTexture>());
  std::vector<std::unique_ptr<IShaderTexture>> shaderTextures;
  std::vector<std::unique_ptr<IShader>> shaders;

  EXPECT_TRUE(shader.PrepareParameters(source, shaderTextures, shaders, 1));
  EXPECT_EQ(1u, shader.lockVertexBufferCalls);
  EXPECT_EQ(1u, shader.unlockVertexBufferCalls);
  EXPECT_EQ(1u, shader.updateInputBufferCalls);
}
