/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef __cplusplus

#if defined(WIN32) && defined(HAS_ANGLE)

#include <angle_gl.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <kodi/AddonBase.h>
#include <kodi/gui/General.h>
#include <wrl/client.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#pragma comment(lib, "d3dcompiler.lib")
#ifndef GL_CLIENT_VERSION
#define GL_CLIENT_VERSION 3
#endif

namespace kodi
{
namespace gui
{
namespace gl
{

class ATTR_DLL_LOCAL CGLonDX : public kodi::gui::IRenderHelper
{
public:
  explicit CGLonDX() : m_pContext(reinterpret_cast<ID3D11DeviceContext*>(kodi::gui::GetHWContext()))
  {
  }
  ~CGLonDX() override { destruct(); }

  bool Init() override
  {
    EGLint egl_display_attrs[] = {EGL_PLATFORM_ANGLE_TYPE_ANGLE,
                                  EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
                                  EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE,
                                  EGL_DONT_CARE,
                                  EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE,
                                  EGL_DONT_CARE,
                                  EGL_EXPERIMENTAL_PRESENT_PATH_ANGLE,
                                  EGL_EXPERIMENTAL_PRESENT_PATH_FAST_ANGLE,
                                  EGL_NONE};
    EGLint egl_config_attrs[] = {EGL_RED_SIZE,
                                 8,
                                 EGL_GREEN_SIZE,
                                 8,
                                 EGL_BLUE_SIZE,
                                 8,
                                 EGL_ALPHA_SIZE,
                                 8,
                                 EGL_BIND_TO_TEXTURE_RGBA,
                                 EGL_TRUE,
                                 EGL_RENDERABLE_TYPE,
                                 GL_CLIENT_VERSION == 3 ? EGL_OPENGL_ES3_BIT : EGL_OPENGL_ES2_BIT,
                                 EGL_SURFACE_TYPE,
                                 EGL_PBUFFER_BIT,
                                 EGL_NONE};
    EGLint egl_context_attrs[] = {EGL_CONTEXT_CLIENT_VERSION, GL_CLIENT_VERSION, EGL_NONE};

    m_eglDisplay =
        eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, egl_display_attrs);
    if (m_eglDisplay == EGL_NO_DISPLAY)
    {
      Log(ADDON_LOG_ERROR, "GLonDX: unable to get EGL display (%s)", eglGetErrorString());
      return false;
    }

    if (eglInitialize(m_eglDisplay, nullptr, nullptr) != EGL_TRUE)
    {
      Log(ADDON_LOG_ERROR, "GLonDX: unable to init EGL display (%s)", eglGetErrorString());
      return false;
    }

    EGLint numConfigs = 0;
    if (eglChooseConfig(m_eglDisplay, egl_config_attrs, &m_eglConfig, 1, &numConfigs) != EGL_TRUE ||
        numConfigs == 0)
    {
      Log(ADDON_LOG_ERROR, "GLonDX: unable to get EGL config (%s)", eglGetErrorString());
      return false;
    }

    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, nullptr, egl_context_attrs);
    if (m_eglContext == EGL_NO_CONTEXT)
    {
      Log(ADDON_LOG_ERROR, "GLonDX: unable to create EGL context (%s)", eglGetErrorString());
      return false;
    }

    if (!createD3DResources())
      return false;

    if (eglMakeCurrent(m_eglDisplay, m_eglBuffer, m_eglBuffer, m_eglContext) != EGL_TRUE)
    {
      Log(ADDON_LOG_ERROR, "GLonDX: unable to make current EGL (%s)", eglGetErrorString());
      return false;
    }
    return true;
  }

  void CheckGL(ID3D11DeviceContext* device)
  {
    if (m_pContext != device)
    {
      m_pSRView = nullptr;
      m_pVShader = nullptr;
      m_pPShader = nullptr;
      m_pContext = device;

      if (m_eglBuffer != EGL_NO_SURFACE)
      {
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(m_eglDisplay, m_eglBuffer);
        m_eglBuffer = EGL_NO_SURFACE;
      }

      // create new resources
      if (!createD3DResources())
        return;

      eglMakeCurrent(m_eglDisplay, m_eglBuffer, m_eglBuffer, m_eglContext);
    }
  }

  void Begin() override
  {
    // confirm on begin D3D context is correct
    CheckGL(reinterpret_cast<ID3D11DeviceContext*>(kodi::gui::GetHWContext()));

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  void End() override
  {
    glFlush();

    // set our primitive shaders
    m_pContext->VSSetShader(m_pVShader.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pPShader.Get(), nullptr, 0);
    m_pContext->PSSetShaderResources(0, 1, m_pSRView.GetAddressOf());
    // draw texture
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_pContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    m_pContext->IASetInputLayout(nullptr);
    m_pContext->Draw(4, 0);
    // unset shaders
    m_pContext->PSSetShader(nullptr, nullptr, 0);
    m_pContext->VSSetShader(nullptr, nullptr, 0);
    // unbind our view
    ID3D11ShaderResourceView* views[1] = {};
    m_pContext->PSSetShaderResources(0, 1, views);
  }

private:
  enum ShaderType
  {
    VERTEX_SHADER,
    PIXEL_SHADER
  };

  bool createD3DResources()
  {
    HANDLE sharedHandle;
    Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRTView;
    Microsoft::WRL::ComPtr<ID3D11Resource> pRTResource;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> pRTTexture;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> pOffScreenTexture;
    Microsoft::WRL::ComPtr<IDXGIResource> dxgiResource;

    m_pContext->GetDevice(&pDevice);
    m_pContext->OMGetRenderTargets(1, &pRTView, nullptr);
    if (!pRTView)
      return false;

    pRTView->GetResource(&pRTResource);
    if (FAILED(pRTResource.As(&pRTTexture)))
      return false;

    D3D11_TEXTURE2D_DESC texDesc;
    pRTTexture->GetDesc(&texDesc);
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    if (FAILED(pDevice->CreateTexture2D(&texDesc, nullptr, &pOffScreenTexture)))
    {
      Log(ADDON_LOG_ERROR, "GLonDX: unable to create intermediate texture");
      return false;
    }

    CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(pOffScreenTexture.Get(),
                                             D3D11_SRV_DIMENSION_TEXTURE2D);
    if (FAILED(pDevice->CreateShaderResourceView(pOffScreenTexture.Get(), &srvDesc, &m_pSRView)))
    {
      Log(ADDON_LOG_ERROR, "GLonDX: unable to create shader view");
      return false;
    }

    if (FAILED(pOffScreenTexture.As(&dxgiResource)) ||
        FAILED(dxgiResource->GetSharedHandle(&sharedHandle)))
    {
      Log(ADDON_LOG_ERROR, "GLonDX: unable get shared handle for texture");
      return false;
    }

    // initiate simple shaders
    if (FAILED(d3dCreateShader(VERTEX_SHADER, vs_out_shader_text, &m_pVShader)))
    {
      Log(ADDON_LOG_ERROR, "GLonDX: unable to create vertex shader view");
      return false;
    }

    if (FAILED(d3dCreateShader(PIXEL_SHADER, ps_out_shader_text, &m_pPShader)))
    {
      Log(ADDON_LOG_ERROR, "GLonDX: unable to create pixel shader view");
      return false;
    }

    // create EGL buffer from D3D shared texture
    EGLint egl_buffer_attrs[] = {EGL_WIDTH,
                                 static_cast<EGLint>(texDesc.Width),
                                 EGL_HEIGHT,
                                 static_cast<EGLint>(texDesc.Height),
                                 EGL_TEXTURE_TARGET,
                                 EGL_TEXTURE_2D,
                                 EGL_TEXTURE_FORMAT,
                                 EGL_TEXTURE_RGBA,
                                 EGL_NONE};

    m_eglBuffer =
        eglCreatePbufferFromClientBuffer(m_eglDisplay, EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE,
                                         sharedHandle, m_eglConfig, egl_buffer_attrs);

    if (m_eglBuffer == EGL_NO_SURFACE)
    {
      Log(ADDON_LOG_ERROR, "GLonDX: unable to create EGL buffer (%s)", eglGetErrorString());
      return false;
    }
    return true;
  }

  HRESULT d3dCreateShader(ShaderType shaderType,
                          const std::string& source,
                          IUnknown** ppShader) const
  {
    Microsoft::WRL::ComPtr<ID3DBlob> pBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> pErrors;

    auto hr = D3DCompile(source.c_str(), source.length(), nullptr, nullptr, nullptr, "main",
                         shaderType == PIXEL_SHADER ? "ps_4_0" : "vs_4_0", 0, 0, &pBlob, &pErrors);

    if (SUCCEEDED(hr))
    {
      Microsoft::WRL::ComPtr<ID3D11Device> pDevice;
      m_pContext->GetDevice(&pDevice);

      if (shaderType == PIXEL_SHADER)
      {
        hr = pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr,
                                        reinterpret_cast<ID3D11PixelShader**>(ppShader));
      }
      else
      {
        hr = pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr,
                                         reinterpret_cast<ID3D11VertexShader**>(ppShader));
      }

      if (FAILED(hr))
      {
        Log(ADDON_LOG_ERROR, "GLonDX: unable to create %s shader",
            shaderType == PIXEL_SHADER ? "pixel" : "vertex");
      }
    }
    else
    {
      Log(ADDON_LOG_ERROR, "GLonDX: unable to compile shader (%s)", pErrors->GetBufferPointer());
    }
    return hr;
  }

  static const char* eglGetErrorString()
  {
#define CASE_STR(value) \
  case value: \
    return #value
    switch (eglGetError())
    {
      CASE_STR(EGL_SUCCESS);
      CASE_STR(EGL_NOT_INITIALIZED);
      CASE_STR(EGL_BAD_ACCESS);
      CASE_STR(EGL_BAD_ALLOC);
      CASE_STR(EGL_BAD_ATTRIBUTE);
      CASE_STR(EGL_BAD_CONTEXT);
      CASE_STR(EGL_BAD_CONFIG);
      CASE_STR(EGL_BAD_CURRENT_SURFACE);
      CASE_STR(EGL_BAD_DISPLAY);
      CASE_STR(EGL_BAD_SURFACE);
      CASE_STR(EGL_BAD_MATCH);
      CASE_STR(EGL_BAD_PARAMETER);
      CASE_STR(EGL_BAD_NATIVE_PIXMAP);
      CASE_STR(EGL_BAD_NATIVE_WINDOW);
      CASE_STR(EGL_CONTEXT_LOST);
      default:
        return "Unknown";
    }
#undef CASE_STR
  }

  void destruct()
  {
    if (m_eglDisplay != EGL_NO_DISPLAY)
    {
      eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

      if (m_eglBuffer != EGL_NO_SURFACE)
      {
        eglDestroySurface(m_eglDisplay, m_eglBuffer);
        m_eglBuffer = EGL_NO_SURFACE;
      }

      if (m_eglContext != EGL_NO_CONTEXT)
      {
        eglDestroyContext(m_eglDisplay, m_eglContext);
        m_eglContext = EGL_NO_CONTEXT;
      }

      eglTerminate(m_eglDisplay);
      m_eglDisplay = EGL_NO_DISPLAY;
    }

    m_pSRView = nullptr;
    m_pVShader = nullptr;
    m_pPShader = nullptr;
    m_pContext = nullptr;
  }

  EGLConfig m_eglConfig = EGL_NO_CONFIG_KHR;
  EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
  EGLContext m_eglContext = EGL_NO_CONTEXT;
  EGLSurface m_eglBuffer = EGL_NO_SURFACE;

  ID3D11DeviceContext* m_pContext = nullptr; // don't hold context
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pSRView = nullptr;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVShader = nullptr;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pPShader = nullptr;

#define TO_STRING(...) #__VA_ARGS__
  std::string vs_out_shader_text = TO_STRING(void main(uint id
                                                       : SV_VertexId, out float2 tex
                                                       : TEXCOORD0, out float4 pos
                                                       : SV_POSITION) {
    tex = float2(id % 2, (id % 4) >> 1);
    pos = float4((tex.x - 0.5f) * 2, -(tex.y - 0.5f) * 2, 0, 1);
  });

  std::string ps_out_shader_text = TO_STRING(
  Texture2D texMain : register(t0);
  SamplerState Sampler
  {
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
    Comparison = NEVER;
  };

  float4 main(in float2 tex : TEXCOORD0) : SV_TARGET
  {
    return texMain.Sample(Sampler, tex);
  });
#undef TO_STRING
}; /* class CGLonDX */

} /* namespace gl */

using CRenderHelper = gl::CGLonDX;
} /* namespace gui */
} /* namespace kodi */

#else /* defined(WIN32) && defined(HAS_ANGLE) */
#pragma message("WARNING: GLonDX.h only be available on Windows by use of Angle as depend!")
#endif /* defined(WIN32) && defined(HAS_ANGLE) */

#endif /* __cplusplus */
