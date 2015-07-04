/*  XMMS - Cross-platform multimedia player
 *  Copyright (C) 1998-2000  Peter Alm, Mikael Alm, Olle Hallnas, Thomas Nilsson and 4Front Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 *  Wed May 24 10:49:37 CDT 2000
 *  Fixes to threading/context creation for the nVidia X4 drivers by
 *  Christian Zander <phoenix@minion.de>
 */

/*
 *  Ported to XBMC by d4rk
 *  Also added 'hSpeed' to animate transition between bar heights
 */


#include "../../addons/include/xbmc_vis_dll.h"
#include <math.h>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <stdio.h>

#define NUM_BANDS 16
#define NUM_VERTICIES 36

float y_angle = 45.0f, y_speed = 0.5f;
float x_angle = 20.0f, x_speed = 0.0f;
float z_angle = 0.0f, z_speed = 0.0f;
float heights[16][16], cHeights[16][16], scale;
float hSpeed = 0.05f;
DWORD g_mode = 3; // D3DFILL_SOLID;

ID3D11Device*             g_device = NULL;
ID3D11DeviceContext*      g_context = NULL;
ID3D11VertexShader*       g_vShader = NULL;
ID3D11PixelShader*        g_pShader = NULL;
ID3D11InputLayout*        g_inputLayout = NULL;
ID3D11Buffer*             g_vBuffer = NULL;
ID3D11Buffer*             g_cViewProj = NULL;
ID3D11Buffer*             g_cWorld = NULL;
ID3D11RasterizerState*    g_rsStateSolid = NULL;
ID3D11RasterizerState*    g_rsStateWire = NULL;
ID3D11BlendState*         g_omBlend = NULL;
ID3D11DepthStencilState*  g_omDepth = NULL;

using namespace DirectX;
using namespace DirectX::PackedVector;

// Include the precompiled shader code.
namespace
{
  #include "DefaultPixelShader.inc"
  #include "DefaultVertexShader.inc"
}

typedef struct
{
  XMFLOAT3 pos;
  XMFLOAT4 col;
} Vertex_t;

typedef struct
{
  XMFLOAT4X4 view;
  XMFLOAT4X4 proj;
} cbViewProj;

typedef struct
{
  XMFLOAT4X4 world;
} cbWorld;

#define VERTEX_FORMAT (D3DFVF_XYZ | D3DFVF_DIFFUSE)

void draw_vertex(Vertex_t * pVertex, float x, float y, float z, XMFLOAT4 color) {
  pVertex->col = XMFLOAT4(color);
  pVertex->pos = XMFLOAT3(x, y, z);
}

int draw_rectangle(Vertex_t * verts, float x1, float y1, float z1, float x2, float y2, float z2, XMFLOAT4 color)
{
  if(y1 == y2)
  {
    draw_vertex(&verts[0], x1, y1, z1, color);
    draw_vertex(&verts[1], x2, y1, z1, color);
    draw_vertex(&verts[2], x2, y2, z2, color);

    draw_vertex(&verts[3], x2, y2, z2, color);
    draw_vertex(&verts[4], x1, y2, z2, color);
    draw_vertex(&verts[5], x1, y1, z1, color);
  }
  else
  {
    draw_vertex(&verts[0], x1, y1, z1, color);
    draw_vertex(&verts[1], x2, y1, z2, color);
    draw_vertex(&verts[2], x2, y2, z2, color);

    draw_vertex(&verts[3], x2, y2, z2, color);
    draw_vertex(&verts[4], x1, y2, z1, color);
    draw_vertex(&verts[5], x1, y1, z1, color);
  }
  return 6;
}

void draw_bar(float x_offset, float z_offset, float height, float red, float green, float blue)
{
  Vertex_t  verts[NUM_VERTICIES];
  int verts_idx = 0;

  float width = 0.1f;
  XMFLOAT4 color;

  if (1 == g_mode /*== D3DFILL_POINT*/)
    color = XMFLOAT4(0.2f, 1.0f, 0.2f, 1.0f);

  if (1 != g_mode /*!= D3DFILL_POINT*/)
  {
    color = XMFLOAT4(red, green, blue, 1.0f);
    verts_idx += draw_rectangle(&verts[verts_idx], x_offset, height, z_offset, x_offset + width, height, z_offset + 0.1f, color);
  }
  verts_idx += draw_rectangle(&verts[verts_idx], x_offset, 0.0f, z_offset, x_offset + width, 0.0f, z_offset + 0.1f, color);

  if (1 != g_mode /*!= D3DFILL_POINT*/)
  {
    color = XMFLOAT4(0.5f * red, 0.5f * green, 0.5f * blue, 1.0f);
    verts_idx += draw_rectangle(&verts[verts_idx], x_offset, 0.0f, z_offset + 0.1f, x_offset + width, height, z_offset + 0.1f, color);
  }
  verts_idx += draw_rectangle(&verts[verts_idx], x_offset, 0.0f, z_offset, x_offset + width, height, z_offset, color);

  if (1 != g_mode /*!= D3DFILL_POINT*/)
  {
    color = XMFLOAT4(0.25f * red, 0.25f * green, 0.25f * blue, 1.0f);
    verts_idx += draw_rectangle(&verts[verts_idx], x_offset, 0.0f, z_offset , x_offset, height, z_offset + 0.1f, color);
  }
  verts_idx += draw_rectangle(&verts[verts_idx], x_offset + width, 0.0f, z_offset , x_offset + width, height, z_offset + 0.1f, color);

  D3D11_MAPPED_SUBRESOURCE res;
  if (S_OK == g_context->Map(g_vBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res))
  {
    memcpy(res.pData, verts, sizeof(Vertex_t) * NUM_VERTICIES);
    g_context->Unmap(g_vBuffer, 0);
  }

  g_context->IASetPrimitiveTopology(g_mode != 1 /*D3DFILL_POINT*/ ? D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST : D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
  g_context->Draw(verts_idx, 0);
}

void draw_bars(void)
{
  int x,y;
  float x_offset, z_offset, r_base, b_base;

  for(y = 0; y < 16; y++)
  {
    z_offset = -1.6f + ((15 - y) * 0.2f);

    b_base = y * (1.0f / 15);
    r_base = 1.0f - b_base;

    for(x = 0; x < 16; x++)
    {
      x_offset = -1.6f + (x * 0.2f);
      if (::fabs(cHeights[y][x]-heights[y][x])>hSpeed)
      {
        if (cHeights[y][x]<heights[y][x])
          cHeights[y][x] += hSpeed;
        else
          cHeights[y][x] -= hSpeed;
      }
      draw_bar(x_offset, z_offset,
               cHeights[y][x], r_base - (x * (r_base / 15.0f)),
               x * (1.0f / 15), b_base);
    }
  }
}

bool init_renderer_objs()
{
  if (S_OK != g_device->CreateVertexShader(DefaultVertexShaderCode, sizeof(DefaultVertexShaderCode), nullptr, &g_vShader))
    return false;

  // Create input layout
  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };
  if (S_OK != g_device->CreateInputLayout(layout, ARRAYSIZE(layout), DefaultVertexShaderCode, sizeof(DefaultVertexShaderCode), &g_inputLayout))
    return false;

  // Create pixel shader
  if (S_OK != g_device->CreatePixelShader(DefaultPixelShaderCode, sizeof(DefaultPixelShaderCode), nullptr, &g_pShader))
    return false;

  // create buffers
  CD3D11_BUFFER_DESC desc(sizeof(Vertex_t) * NUM_VERTICIES, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
  if (S_OK != g_device->CreateBuffer(&desc, NULL, &g_vBuffer))
    return false;

  desc.ByteWidth = sizeof(cbWorld);
  desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  if (S_OK != g_device->CreateBuffer(&desc, NULL, &g_cWorld))
    return false;

  cbViewProj cViewProj;
  XMStoreFloat4x4(&cViewProj.view, XMMatrixTranspose(XMMatrixIdentity()));
  XMStoreFloat4x4(&cViewProj.proj, XMMatrixTranspose(XMMatrixPerspectiveOffCenterLH(-1.0f, 1.0f, -1.0f, 1.0f, 1.5f, 10.0f)));

  desc.ByteWidth = sizeof(cbViewProj);
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.CPUAccessFlags = 0;
  D3D11_SUBRESOURCE_DATA initData = { 0 };
  initData.pSysMem = &cViewProj;
  if (S_OK != g_device->CreateBuffer(&desc, &initData, &g_cViewProj))
    return false;

  // create blend state
  D3D11_BLEND_DESC blendState = { 0 };
  ZeroMemory(&blendState, sizeof(D3D11_BLEND_DESC));
  blendState.RenderTarget[0].BlendEnable = true;
  blendState.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE; 
  blendState.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
  blendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
  blendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
  blendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
  blendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
  blendState.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

  if (S_OK != g_device->CreateBlendState(&blendState, &g_omBlend))
    return false;

  // create depth state
  D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
  ZeroMemory(&depthStencilDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

  // Set up the description of the stencil state.
  depthStencilDesc.DepthEnable = true;
  depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
  depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
  depthStencilDesc.StencilEnable = true;
  depthStencilDesc.StencilReadMask = 0xFF;
  depthStencilDesc.StencilWriteMask = 0xFF;

  // Stencil operations if pixel is front-facing.
  depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
  depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

  // Stencil operations if pixel is back-facing.
  depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
  depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
  depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

  if (S_OK != g_device->CreateDepthStencilState(&depthStencilDesc, &g_omDepth))
    return false;

  // create raster states
  D3D11_RASTERIZER_DESC rasterizerState;
  rasterizerState.CullMode = D3D11_CULL_NONE;
  rasterizerState.FillMode = D3D11_FILL_SOLID;
  rasterizerState.FrontCounterClockwise = false;
  rasterizerState.DepthBias = 0;
  rasterizerState.DepthBiasClamp = 0.0f;
  rasterizerState.DepthClipEnable = true;
  rasterizerState.SlopeScaledDepthBias = 0.0f;
  rasterizerState.ScissorEnable = false;
  rasterizerState.MultisampleEnable = false;
  rasterizerState.AntialiasedLineEnable = false;

  if (S_OK != g_device->CreateRasterizerState(&rasterizerState, &g_rsStateSolid))
    return false;

  rasterizerState.FillMode = D3D11_FILL_WIREFRAME;
  if (S_OK != g_device->CreateRasterizerState(&rasterizerState, &g_rsStateWire))
    return false;

  // we are ready
  return true;
}

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* visProps)
{
  if (!visProps)
    return ADDON_STATUS_UNKNOWN;

  VIS_PROPS* props = (VIS_PROPS*) visProps;
  g_context = (ID3D11DeviceContext*)props->device;
  g_context->GetDevice(&g_device);

  if (!init_renderer_objs())
    return ADDON_STATUS_PERMANENT_FAILURE;

  return ADDON_STATUS_NEED_SETTINGS;
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  bool configured = true; //FALSE;

  float factors[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
  g_context->OMSetBlendState(g_omBlend, factors, 0xFFFFFFFF);
  g_context->OMSetDepthStencilState(g_omDepth, 0);
  switch (g_mode)
  {
  case 1: // D3DFILL_POINT:
  case 2: // D3DFILL_WIREFRAME:
    g_context->RSSetState(g_rsStateWire);
    break;
  case 3: // D3DFILL_SOLID:
    g_context->RSSetState(g_rsStateSolid);
    break;
  }

  unsigned stride = sizeof(Vertex_t), offset = 0;
  g_context->IASetVertexBuffers(0, 1, &g_vBuffer, &stride, &offset);
  g_context->IASetInputLayout(g_inputLayout);
  g_context->VSSetShader(g_vShader, 0, 0);
  g_context->VSSetConstantBuffers(0, 1, &g_cViewProj);
  g_context->VSSetConstantBuffers(1, 1, &g_cWorld);
  g_context->PSSetShader(g_pShader, 0, 0);

  if(configured)
  {
    x_angle += x_speed;
    if (x_angle >= 360.0f)
      x_angle -= 360.0f;

    y_angle += y_speed;
    if (y_angle >= 360.0f)
      y_angle -= 360.0f;

    z_angle += z_speed;
    if (z_angle >= 360.0f)
      z_angle -= 360.0f;

    D3D11_MAPPED_SUBRESOURCE res;
    if (S_OK == g_context->Map(g_cWorld, 0, D3D11_MAP_WRITE_DISCARD, 0, &res))
    {
      cbWorld *cWorld = (cbWorld*)res.pData;
      XMMATRIX
        matRotationX = XMMatrixRotationX(-XMConvertToRadians(x_angle)),
        matRotationY = XMMatrixRotationY(-XMConvertToRadians(y_angle)),
        matRotationZ = XMMatrixRotationZ(XMConvertToRadians(z_angle)),
        matTranslation = XMMatrixTranslation(0.0f, -0.5f, 5.0f),
        matWorld = matRotationZ * matRotationY * matRotationX * matTranslation;
      XMStoreFloat4x4(&cWorld->world, XMMatrixTranspose(matWorld));

      g_context->Unmap(g_cWorld, 0);
    }

    draw_bars();
  }
}

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
  int x, y;

  for(x = 0; x < 16; x++)
  {
    for(y = 0; y < 16; y++)
    {
      cHeights[y][x] = 0.0f;
    }
  }

  scale = 1.0f / log(256.0f);

  x_speed = 0.0f;
  y_speed = 0.5f;
  z_speed = 0.0f;
  x_angle = 20.0f;
  y_angle = 45.0f;
  z_angle = 0.0f;
}

extern "C" void AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  int i,c;
  int y=0;
  float val;

  int xscale[] = {0, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 255};

  for(y = 15; y > 0; y--)
  {
    for(i = 0; i < 16; i++)
    {
      heights[y][i] = heights[y - 1][i];
    }
  }

  for(i = 0; i < NUM_BANDS; i++)
  {
    for(c = xscale[i], y = 0; c < xscale[i + 1]; c++)
    {
      if (c<iAudioDataLength)
      {
        if((int)(pAudioData[c] * (0x07fff+.5f) > y))
          y = (int)(pAudioData[c] * (0x07fff+.5f));
      }
      else
        continue;
    }
    y >>= 7;
    if(y > 0)
      val = (logf((float)y) * scale);
    else
      val = 0;
    heights[0][i] = val;
  }
}


//-- GetInfo ------------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
extern "C" void GetInfo(VIS_INFO* pInfo)
{
  pInfo->bWantsFreq = false;
  pInfo->iSyncDelay = 0;
}

//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" unsigned int GetSubModules(char ***names)
{
  return 0; // this vis supports 0 sub modules
}

//-- OnAction -----------------------------------------------------------------
// Handle XBMC actions such as next preset, lock preset, album art changed etc
//-----------------------------------------------------------------------------
extern "C" bool OnAction(long flags, const void *param)
{
  bool ret = false;
  return ret;
}

//-- GetPresets ---------------------------------------------------------------
// Return a list of presets to XBMC for display
//-----------------------------------------------------------------------------
extern "C" unsigned int GetPresets(char ***presets)
{
  return 0;
}

//-- GetPreset ----------------------------------------------------------------
// Return the index of the current playing preset
//-----------------------------------------------------------------------------
extern "C" unsigned GetPreset()
{
  return 0;
}

//-- IsLocked -----------------------------------------------------------------
// Returns true if this add-on use settings
//-----------------------------------------------------------------------------
extern "C" bool IsLocked()
{
  return false;
}

//-- Stop ---------------------------------------------------------------------
// This dll must stop all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Stop()
{
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Destroy()
{
  if (g_cViewProj)
    g_cViewProj->Release();
  if (g_cWorld)
    g_cWorld->Release();
  if (g_rsStateSolid)
    g_rsStateSolid->Release();
  if (g_rsStateWire)
    g_rsStateWire->Release();
  if (g_omBlend)
    g_omBlend->Release();
  if (g_omDepth)
    g_omDepth->Release();
  if (g_vBuffer)
    g_vBuffer->Release();
  if (g_inputLayout)
    g_inputLayout->Release();
  if (g_vShader)
    g_vShader->Release();
  if (g_pShader)
    g_pShader->Release();
  if (g_device)
    g_device->Release();
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" bool ADDON_HasSettings()
{
  return true;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

extern "C" void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  if (!strSetting || !value)
    return ADDON_STATUS_UNKNOWN;

  if (strcmp(strSetting, "bar_height")==0)
  {
    switch (*(int*) value)
    {
    case 0:
      scale = 1.0f / log(256.0f);
      break;

    case 1:
      scale = 2.0f / log(256.0f);
      break;

    case 2:
      scale = 3.0f / log(256.0f);
      break;

    case 3:
      scale = 0.5f / log(256.0f);
      break;

    case 4:
      scale = 0.33f / log(256.0f);
      break;
    }
    return ADDON_STATUS_OK;
  }

  else if (strcmp(strSetting, "speed")==0)
  {
    switch (*(int*) value)
    {
    case 0:
      hSpeed = 0.05f;
      break;

    case 1:
      hSpeed = 0.025f;
      break;

    case 2:
      hSpeed = 0.0125f;
      break;

    case 3:
      hSpeed = 0.10f;
      break;

    case 4:
      hSpeed = 0.20f;
      break;
    }
    return ADDON_STATUS_OK;
  }

  else if (strcmp(strSetting, "mode")==0)
  {
    switch (*(int*) value)
    {
    case 0:
      g_mode = 3; // D3DFILL_SOLID;
      break;

    case 1:
      g_mode = 2; // D3DFILL_WIREFRAME;
      break;

    case 2:
      g_mode = 1; // D3DFILL_POINT;
      break;
    }
    return ADDON_STATUS_OK;
  }
  return ADDON_STATUS_UNKNOWN;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
//-----------------------------------------------------------------------------

extern "C" void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}
