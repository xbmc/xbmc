/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GLESShader.h"
#include "rendering/RenderSystem.h"
#include "utils/Color.h"
#include "utils/log.h"

#include <array>
#include <unordered_set>

#include "system_gl.h"

enum ESHADERMETHOD
{
  SM_DEFAULT,
  SM_TEXTURE,
  SM_MULTI,
  SM_FONTS,
  SM_TEXTURE_NOBLEND,
  SM_MULTI_BLENDCOLOR,
  SM_TEXTURE_RGBA,
  SM_TEXTURE_RGBA_OES,
  SM_TEXTURE_RGBA_BLENDCOLOR,
  SM_TEXTURE_RGBA_BOB,
  SM_TEXTURE_RGBA_BOB_OES,
  SM_TEXTURE_NOALPHA,
  SM_MAX
};

class CShaderCache
{
public:
  CShaderCache() = default;
  ~CShaderCache()
  {
    for (auto& shader : m_shaderCache)
    {
      CLog::Log(LOGDEBUG, "deleting shader with hash: {}", (*shader).GetHash());
      delete shader;
    }
  }

  Shaders::CGLSLShaderProgram* Find(Shaders::CGLSLShaderProgram* s)
  {
    CLog::Log(LOGDEBUG, "looking for hash: {}", s->GetHash());

    CLog::Log(LOGDEBUG, "shaders in cache:");
    for (auto shader : m_shaderCache)
      CLog::Log(LOGDEBUG, "\t\thash: {}", (*shader).GetHash());

    auto shader = m_shaderCache.find(s);
    if (shader != m_shaderCache.end())
    {
      CLog::Log(LOGDEBUG, "found shader in cache: {} hash: {}", fmt::ptr(*shader),
                (*shader)->GetHash());
      return *shader;
    }

    return nullptr;
  }

  void Add(Shaders::CGLSLShaderProgram* const s)
  {
    m_shaderCache.emplace(s);
    CLog::Log(LOGDEBUG, "new shader added to cache: {} hash: {}", fmt::ptr(s), s->GetHash());
  }

private:
  struct ShaderHash
  {
    std::size_t operator()(Shaders::CGLSLShaderProgram* const s) const noexcept
    {
      return s->GetHash();
    }
  };

  struct ShaderEquals
  {
    bool operator()(Shaders::CGLSLShaderProgram const* lhs,
                    Shaders::CGLSLShaderProgram const* rhs) const
    {
      return lhs->GetHash() == rhs->GetHash();
    }
  };

  std::unordered_set<Shaders::CGLSLShaderProgram*, ShaderHash, ShaderEquals> m_shaderCache;
};

class CRenderSystemGLES : public CRenderSystemBase
{
public:
  CRenderSystemGLES();
  ~CRenderSystemGLES() override = default;

  bool InitRenderSystem() override;
  bool DestroyRenderSystem() override;
  bool ResetRenderSystem(int width, int height) override;

  bool BeginRender() override;
  bool EndRender() override;
  void PresentRender(bool rendered, bool videoLayer) override;
  bool ClearBuffers(UTILS::Color color) override;
  bool IsExtSupported(const char* extension) const override;

  void SetVSync(bool vsync);
  void ResetVSync() { m_bVsyncInit = false; }

  void SetViewPort(const CRect& viewPort) override;
  void GetViewPort(CRect& viewPort) override;

  bool ScissorsCanEffectClipping() override;
  CRect ClipRectToScissorRect(const CRect &rect) override;
  void SetScissors(const CRect& rect) override;
  void ResetScissors() override;

  void CaptureStateBlock() override;
  void ApplyStateBlock() override;

  void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor = 0.0f) override;

  bool SupportsStereo(RENDER_STEREO_MODE mode) const override;

  void Project(float &x, float &y, float &z) override;

  std::string GetShaderPath(const std::string &filename) override { return "GLES/2.0/"; }

  void InitialiseShaders();
  void ReleaseShaders();
  void EnableGUIShader(ESHADERMETHOD method);
  void DisableGUIShader();

  GLint GUIShaderGetPos();
  GLint GUIShaderGetCol();
  GLint GUIShaderGetCoord0();
  GLint GUIShaderGetCoord1();
  GLint GUIShaderGetUniCol();
  GLint GUIShaderGetCoord0Matrix();
  GLint GUIShaderGetField();
  GLint GUIShaderGetStep();
  GLint GUIShaderGetContrast();
  GLint GUIShaderGetBrightness();
  GLint GUIShaderGetModel();

  CShaderCache* GetShaderCache() { return m_shaderCache.get(); }

protected:
  virtual void SetVSyncImpl(bool enable) = 0;
  virtual void PresentRenderImpl(bool rendered) = 0;
  void CalculateMaxTexturesize();

  bool m_bVsyncInit{false};
  int m_width;
  int m_height;

  std::string m_RenderExtensions;

  std::array<std::unique_ptr<CGLESShader>, SM_MAX> m_pShader;
  ESHADERMETHOD m_method = SM_DEFAULT;

  GLint      m_viewPort[4];

  std::unique_ptr<CShaderCache> m_shaderCache;
};

