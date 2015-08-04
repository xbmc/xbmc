/*
 *      Copyright (C) 2008-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

// Waveform.vis
// A simple visualisation example by MrC

#include "addons/include/xbmc_vis_dll.h"
#include <stdio.h>
#ifdef HAS_GL
#include <GL/glew.h>
#else
#ifdef _WIN32
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <stdio.h>
#endif
#endif

char g_visName[512];
#ifndef HAS_GL
ID3D11Device*             g_device = NULL;
ID3D11DeviceContext*      g_context = NULL;
ID3D11VertexShader*       g_vShader = NULL;
ID3D11PixelShader*        g_pShader = NULL;
ID3D11InputLayout*        g_inputLayout = NULL;
ID3D11Buffer*             g_vBuffer = NULL;
ID3D11Buffer*             g_cViewPort = NULL;

using namespace DirectX;
using namespace DirectX::PackedVector;

// Include the precompiled shader code.
namespace
{
  #include "DefaultPixelShader.inc"
  #include "DefaultVertexShader.inc"
}

struct cbViewPort
{
  float g_viewPortWidth;
  float g_viewPortHeigh;
  float align1, align2;
};

#else
void* g_device;
#endif
float g_fWaveform[2][512];

#ifdef HAS_GL
typedef struct {
  int TopLeftX;
  int TopLeftY;
  int Width;
  int Height;
  int MinDepth;
  int MaxDepth;
} D3D11_VIEWPORT;
typedef unsigned long D3DCOLOR;
#endif

D3D11_VIEWPORT g_viewport;

struct Vertex_t
{
  float x, y, z;
#ifdef HAS_GL
  D3DCOLOR  col;
#else
  XMFLOAT4 col;
#endif
};

#ifndef HAS_GL
bool init_renderer_objs()
{
  // Create vertex shader
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
  CD3D11_BUFFER_DESC desc(sizeof(Vertex_t) * 512, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
  if (S_OK != g_device->CreateBuffer(&desc, NULL, &g_vBuffer))
    return false;

  desc.ByteWidth = sizeof(cbViewPort);
  desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.CPUAccessFlags = 0;

  cbViewPort viewPort = { (float)g_viewport.Width, (float)g_viewport.Height, 0.0f, 0.0f };
  D3D11_SUBRESOURCE_DATA initData;
  initData.pSysMem = &viewPort;

  if (S_OK != g_device->CreateBuffer(&desc, &initData, &g_cViewPort))
    return false;

  // we are ready
  return true;
}
#endif // !HAS_GL

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  VIS_PROPS* visProps = (VIS_PROPS*)props;

#ifdef HAS_GL
  g_device = visProps->device;
#endif
  g_viewport.TopLeftX = visProps->x;
  g_viewport.TopLeftY = visProps->y;
  g_viewport.Width = visProps->width;
  g_viewport.Height = visProps->height;
  g_viewport.MinDepth = 0;
  g_viewport.MaxDepth = 1;
#ifndef HAS_GL  
  g_context = (ID3D11DeviceContext*)visProps->device;
  g_context->GetDevice(&g_device);
  if (!init_renderer_objs())
    return ADDON_STATUS_PERMANENT_FAILURE;
#endif

  return ADDON_STATUS_OK;
}

//-- Start --------------------------------------------------------------------
// Called when a new soundtrack is played
//-----------------------------------------------------------------------------
extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
  //printf("Got Start Command\n");
}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
extern "C" void AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  int ipos=0;
  while (ipos < 512)
  {
    for (int i=0; i < iAudioDataLength; i+=2)
    {
      g_fWaveform[0][ipos] = pAudioData[i  ]; // left channel
      g_fWaveform[1][ipos] = pAudioData[i+1]; // right channel
      ipos++;
      if (ipos >= 512) break;
    }
  }
}


//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  Vertex_t  verts[512];

#ifndef HAS_GL
  unsigned stride = sizeof(Vertex_t), offset = 0;
  g_context->IASetVertexBuffers(0, 1, &g_vBuffer, &stride, &offset);
  g_context->IASetInputLayout(g_inputLayout);
  g_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
  g_context->VSSetShader(g_vShader, 0, 0);
  g_context->VSSetConstantBuffers(0, 1, &g_cViewPort);
  g_context->PSSetShader(g_pShader, 0, 0);
  float xcolor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
#endif

  // Left channel
#ifdef HAS_GL
  GLenum errcode;
  glColor3f(1.0, 1.0, 1.0);
  glDisable(GL_BLEND);
  glPushMatrix();
  glTranslatef(0,0,-1.0);
  glBegin(GL_LINE_STRIP);
#endif
  for (int i = 0; i < 256; i++)
  {
#ifdef HAS_GL
    verts[i].col = 0xffffffff;
#else
    verts[i].col = XMFLOAT4(xcolor);;
#endif
    verts[i].x = g_viewport.TopLeftX + ((i / 255.0f) * g_viewport.Width);
    verts[i].y = g_viewport.TopLeftY + g_viewport.Height * 0.33f + (g_fWaveform[0][i] * g_viewport.Height * 0.15f);
    verts[i].z = 1.0;
#ifdef HAS_GL
    glVertex2f(verts[i].x, verts[i].y);
#endif
  }

#ifdef HAS_GL
  glEnd();
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
#endif

  // Right channel
#ifdef HAS_GL
  glBegin(GL_LINE_STRIP);
  for (int i = 0; i < 256; i++)
#else
  for (int i = 256; i < 512; i++)
#endif
  {
#ifdef HAS_GL
    verts[i].col = 0xffffffff;
    verts[i].x = g_viewport.TopLeftX + ((i / 255.0f) * g_viewport.Width);
#else
    verts[i].col = XMFLOAT4(xcolor);
    verts[i].x = g_viewport.TopLeftX + (((i - 256) / 255.0f) * g_viewport.Width);
#endif
    verts[i].y = g_viewport.TopLeftY + g_viewport.Height * 0.66f + (g_fWaveform[1][i] * g_viewport.Height * 0.15f);
    verts[i].z = 1.0;
#ifdef HAS_GL
    glVertex2f(verts[i].x, verts[i].y);
#endif
  }

#ifdef HAS_GL
  glEnd();
  glEnable(GL_BLEND);
  glPopMatrix();
  if ((errcode=glGetError())!=GL_NO_ERROR) {
    printf("Houston, we have a GL problem: %s\n", gluErrorString(errcode));
  }
#elif !defined(HAS_GL)
  // a little optimization: generate and send all vertecies for both channels
  D3D11_MAPPED_SUBRESOURCE res;
  if (S_OK == g_context->Map(g_vBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &res))
  {
    memcpy(res.pData, verts, sizeof(Vertex_t) * 512);
    g_context->Unmap(g_vBuffer, 0);
  }
  // draw left channel
  g_context->Draw(256, 0);
  // draw right channel
  g_context->Draw(256, 256);
#endif

}

//-- GetInfo ------------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
extern "C" void GetInfo(VIS_INFO* pInfo)
{
  pInfo->bWantsFreq = false;
  pInfo->iSyncDelay = 0;
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

//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" unsigned int GetSubModules(char ***names)
{
  return 0; // this vis supports 0 sub modules
}

//-- Stop ---------------------------------------------------------------------
// This dll must stop all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Stop()
{
}

//-- Detroy -------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Destroy()
{
#ifndef HAS_GL
  if (g_cViewPort)
    g_cViewPort->Release();
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
#endif
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" bool ADDON_HasSettings()
{
  return false;
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
  return ADDON_STATUS_OK;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}
