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
 *  Ported to GLES by gimli
 */



#include <string.h>
#include <math.h>
#if defined(__APPLE__)
  #include <OpenGLES/ES2/gl.h>
  #include <OpenGLES/ES2/glext.h>
#else
  #include <GLES2/gl2.h>
  #include <GLES2/gl2ext.h>
#endif

#include "addons/include/xbmc_vis_dll.h"
#include "VisGUIShader.h"

#define NUM_BANDS 16

#ifndef M_PI
#define M_PI       3.141592654f
#endif
#define DEG2RAD(d) ( (d) * M_PI/180.0f )

/*GLfloat x_angle = 20.0f, x_speed = 0.0f;
GLfloat y_angle = 45.0f, y_speed = 0.5f;
GLfloat z_angle = 0.0f, z_speed = 0.0f;
GLfloat heights[16][16], cHeights[16][16], scale;
GLfloat hSpeed = 0.025f;
GLenum  g_mode = GL_TRIANGLES;
*/
float g_fWaveform[2][512];

const char *frag = "precision mediump float; \n"
                   "varying lowp vec4 m_colour; \n"
                   "void main () \n"
                   "{ \n"
                   "  gl_FragColor = m_colour; \n"
                   "}\n";

const char *vert = "attribute vec4 m_attrpos;\n"
                   "attribute vec4 m_attrcol;\n"
                   "attribute vec4 m_attrcord0;\n"
                   "attribute vec4 m_attrcord1;\n"
                   "varying vec4   m_cord0;\n"
                   "varying vec4   m_cord1;\n"
                   "varying lowp   vec4 m_colour;\n"
                   "uniform mat4   m_proj;\n"
                   "uniform mat4   m_model;\n"
                   "void main ()\n"
                   "{\n"
                   "  mat4 mvp    = m_proj * m_model;\n"
                   "  gl_Position = mvp * m_attrpos;\n"
                   "  m_colour    = m_attrcol;\n"
                   "  m_cord0     = m_attrcord0;\n"
                   "  m_cord1     = m_attrcord1;\n"
                   "}\n";

CVisGUIShader  *vis_shader = NULL;

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  vis_shader = new CVisGUIShader(vert, frag);

  if(!vis_shader)
    return ADDON_STATUS_UNKNOWN;

  if(!vis_shader->CompileAndLink())
  {
    delete vis_shader;
    return ADDON_STATUS_UNKNOWN;
  }

  return ADDON_STATUS_NEED_SETTINGS;
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  GLfloat col[256][3];
  GLfloat ver[256][3];
  GLubyte idx[256];

  glDisable(GL_BLEND);

  vis_shader->MatrixMode(MM_PROJECTION);
  vis_shader->PushMatrix();
  vis_shader->LoadIdentity();
  //vis_shader->Frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.5f, 10.0f);
  vis_shader->MatrixMode(MM_MODELVIEW);
  vis_shader->PushMatrix();
  vis_shader->LoadIdentity();

  vis_shader->PushMatrix();
  vis_shader->Translatef(0.0f ,0.0f ,-1.0f);
  vis_shader->Rotatef(0.0f, 1.0f, 0.0f, 0.0f);
  vis_shader->Rotatef(0.0f, 0.0f, 1.0f, 0.0f);
  vis_shader->Rotatef(0.0f, 0.0f, 0.0f, 1.0f);

  vis_shader->Enable();

  GLint   posLoc = vis_shader->GetPosLoc();
  GLint   colLoc = vis_shader->GetColLoc();

  glVertexAttribPointer(colLoc, 3, GL_FLOAT, 0, 0, col);
  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, 0, ver);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);

  for (int i = 0; i < 256; i++)
  {
    col[i][0] = 128;
    col[i][1] = 128;
    col[i][2] = 128;
    //ver[i][0] = g_viewport.X + ((i / 255.0f) * g_viewport.Width);
    //ver[i][1] = g_viewport.Y + g_viewport.Height * 0.33f + (g_fWaveform[0][i] * g_viewport.Height * 0.15f);
    ver[i][0] = -1.0f + ((i / 255.0f) * 2.0f);
    ver[i][1] = 0.5f + g_fWaveform[0][i];
    ver[i][2] = 1.0f;
    idx[i] = i;
  }

  glDrawElements(GL_LINE_STRIP, 256, GL_UNSIGNED_BYTE, idx);

  // Right channel
  for (int i = 0; i < 256; i++)
  {
    col[i][0] = 128;
    col[i][1] = 128;
    col[i][2] = 128;
    //ver[i][0] = g_viewport.X + ((i / 255.0f) * g_viewport.Width);
    //ver[i][1] = g_viewport.Y + g_viewport.Height * 0.66f + (g_fWaveform[1][i] * g_viewport.Height * 0.15f);
    ver[i][0] = -1.0f + ((i / 255.0f) * 2.0f);
    ver[i][1] = -0.5f + g_fWaveform[1][i];
    ver[i][2] = 1.0f;
    idx[i] = i;

  }

  glDrawElements(GL_LINE_STRIP, 256, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);

  vis_shader->Disable();

  vis_shader->PopMatrix();

  vis_shader->PopMatrix();
  vis_shader->MatrixMode(MM_PROJECTION);
  vis_shader->PopMatrix();

  glEnable(GL_BLEND);
  
}

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
}

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
// This dll must cease all runtime activities
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
  if (vis_shader) 
  {
    vis_shader->Free();
    delete vis_shader;
    vis_shader = NULL;
  }
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
//    return ADDON_STATUS_OK;
  return ADDON_STATUS_UNKNOWN;
}
