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
#include <D3D9.h>
#include <d3dx9math.h>

#define NUM_BANDS 16

float y_angle = 45.0f, y_speed = 0.5f;
float x_angle = 20.0f, x_speed = 0.0f;
float z_angle = 0.0f, z_speed = 0.0f;
float heights[16][16], cHeights[16][16], scale;
float hSpeed = 0.05f;
DWORD g_mode = D3DFILL_SOLID;
LPDIRECT3DDEVICE9 g_device;

typedef struct
{
  float x, y, z;
  D3DCOLOR  col;
} Vertex_t;

#define VERTEX_FORMAT (D3DFVF_XYZ | D3DFVF_DIFFUSE)

void draw_vertex(Vertex_t * pVertex, float x, float y, float z, D3DCOLOR color) {
	pVertex->col = color;
    pVertex->x = x;
    pVertex->y = y;
    pVertex->z = z;
}

int draw_rectangle(Vertex_t * verts, float x1, float y1, float z1, float x2, float y2, float z2, D3DCOLOR color)
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
  Vertex_t  verts[36];
  int verts_idx = 0;

  float width = 0.1f;
  D3DCOLOR color;

  if (g_mode == D3DFILL_POINT)
    color = D3DXCOLOR(0.2f, 1.0f, 0.2f, 1.0f);

  if (g_mode != D3DFILL_POINT)
  {
    color = D3DXCOLOR(red, green, blue, 1.0f);
    verts_idx += draw_rectangle(&verts[verts_idx], x_offset, height, z_offset, x_offset + width, height, z_offset + 0.1f, color);
  }
  verts_idx += draw_rectangle(&verts[verts_idx], x_offset, 0.0f, z_offset, x_offset + width, 0.0f, z_offset + 0.1f, color);

  if (g_mode != D3DFILL_POINT)
  {
    color = D3DXCOLOR(0.5f * red, 0.5f * green, 0.5f * blue, 1.0f);
    verts_idx += draw_rectangle(&verts[verts_idx], x_offset, 0.0f, z_offset + 0.1f, x_offset + width, height, z_offset + 0.1f, color);
  }
  verts_idx += draw_rectangle(&verts[verts_idx], x_offset, 0.0f, z_offset, x_offset + width, height, z_offset, color);

  if (g_mode != D3DFILL_POINT)
  {
    color = D3DXCOLOR(0.25f * red, 0.25f * green, 0.25f * blue, 1.0f);
    verts_idx += draw_rectangle(&verts[verts_idx], x_offset, 0.0f, z_offset , x_offset, height, z_offset + 0.1f, color);
  }
  verts_idx += draw_rectangle(&verts[verts_idx], x_offset + width, 0.0f, z_offset , x_offset + width, height, z_offset + 0.1f, color);

  g_device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, verts_idx / 3, verts, sizeof(Vertex_t));
}

void draw_bars(void)
{
  int x,y;
  float x_offset, z_offset, r_base, b_base;
  D3DXMATRIX matRotationX, matRotationY, matRotationZ, matTranslation, matWorld;

  D3DXMatrixIdentity(&matWorld);
  D3DXMatrixRotationZ(&matRotationZ, D3DXToRadian(z_angle));
  D3DXMatrixRotationY(&matRotationY, -D3DXToRadian(y_angle));
  D3DXMatrixRotationX(&matRotationX, -D3DXToRadian(x_angle));
  D3DXMatrixTranslation(&matTranslation, 0.0f, -0.5f, 5.0f);
  matWorld = matRotationZ * matRotationY * matRotationX * matTranslation;
  g_device->SetTransform(D3DTS_WORLD, &matWorld);

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

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* visProps)
{
  if (!visProps)
    return ADDON_STATUS_UNKNOWN;

  VIS_PROPS* props = (VIS_PROPS*) visProps;
  g_device = (LPDIRECT3DDEVICE9) props->device;

  return ADDON_STATUS_NEED_SETTINGS;
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  bool configured = true; //FALSE;

  g_device->SetRenderState(D3DRS_SRCBLEND , D3DBLEND_ONE);
  g_device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
  g_device->SetRenderState(D3DRS_AMBIENT, 0xffffffff);
  g_device->SetRenderState(D3DRS_LIGHTING, FALSE);
  g_device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  g_device->SetRenderState(D3DRS_ZENABLE, TRUE);
  g_device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  g_device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
  g_device->SetRenderState(D3DRS_FILLMODE, g_mode);
  g_device->SetFVF(VERTEX_FORMAT);
  g_device->SetPixelShader(NULL);
  g_device->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f), 1.0f, 0);

  D3DXMATRIX matProjection;
  D3DXMatrixPerspectiveOffCenterLH(&matProjection, -1.0f, 1.0f, -1.0f, 1.0f, 1.5f, 10.0f);
  g_device->SetTransform(D3DTS_PROJECTION, &matProjection);

  D3DXMATRIX matView;
  D3DXMatrixIdentity(&matView);
  g_device->SetTransform(D3DTS_VIEW, &matView);

  if(configured)
  {
    x_angle += x_speed;
    if(x_angle >= 360.0f)
      x_angle -= 360.0f;

    y_angle += y_speed;
    if(y_angle >= 360.0f)
      y_angle -= 360.0f;

    z_angle += z_speed;
    if(z_angle >= 360.0f)
      z_angle -= 360.0f;

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
        if(pAudioData[c] > y)
          y = (int)pAudioData[c];
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
      g_mode = D3DFILL_SOLID;
      break;

    case 1:
      g_mode = D3DFILL_WIREFRAME;
      break;

    case 2:
      g_mode = D3DFILL_POINT;
      break;
    }
    return ADDON_STATUS_OK;
  }
  return ADDON_STATUS_UNKNOWN;
}

