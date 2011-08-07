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


#include "system.h"

#if HAS_GLES == 2

#include "addons/include/xbmc_vis_dll.h"
#include <string.h>
#include <math.h>
#if defined(__APPLE__)
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#else
#include <GLES/gl.h>
#endif

#include "MatrixGLES.h"
#include "GUIShader.h"

#define NUM_BANDS 16

#ifndef M_PI
#define M_PI       3.141592654f
#endif
#define DEG2RAD(d) ( (d) * M_PI/180.0f )

GLfloat x_angle = 20.0f, x_speed = 0.0f;
GLfloat y_angle = 45.0f, y_speed = 0.5f;
GLfloat z_angle = 0.0f, z_speed = 0.0f;
GLfloat heights[16][16], cHeights[16][16], scale;
GLfloat hSpeed = 0.025f;
GLenum  g_mode = GL_TRIANGLES;
float g_fWaveform[2][512];

enum VIS
{
  VIS_3D_SPECTRUM = 0,
  VIS_WAVEFORM,
};

VIS g_vis = VIS_3D_SPECTRUM;

std::string frag = "precision mediump float; \n"
                   "varying lowp vec4 m_colour; \n"
                   "void main () \n"
                   "{ \n"
                   "  gl_FragColor = m_colour; \n"
                   "}\n";

std::string vert = "attribute vec4 m_attrpos;\n"
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

CGUIShader *m_shader = NULL;

#if 0
void draw_rectangle(GLfloat x1, GLfloat y1, GLfloat z1, GLfloat x2, GLfloat y2, GLfloat z2)
{
  GLint   posLoc = m_shader->GetPosLoc();
  GLint   colLoc = m_shader->GetColLoc();

  glVertexAttribPointer(colLoc, 4, GL_FLOAT, 0, 0, col);
  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, 0, ver);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);

  if(y1 == y2)
  {
    ver[0][0] = x1; ver[0][1] = y1; ver[0][2] = z1;
    ver[1][0] = x2; ver[1][1] = y1; ver[1][2] = z1;
    ver[2][0] = x2; ver[2][1] = y2; ver[2][2] = z2;

    glDrawArrays (GL_TRIANGLES, 0, 3);

    ver[0][0] = x2; ver[0][1] = y2; ver[0][2] = z2;
    ver[1][0] = x1; ver[1][1] = y2; ver[1][2] = z2;
    ver[2][0] = x1; ver[2][1] = y1; ver[2][2] = z1;

    glDrawArrays (GL_TRIANGLES, 0, 3);
  }
  else
  {
    ver[0][0] = x1; ver[0][1] = y1; ver[0][2] = z1;
    ver[1][0] = x2; ver[1][1] = y1; ver[1][2] = z2;
    ver[2][0] = x2; ver[2][1] = y2; ver[2][2] = z2;

    glDrawArrays (GL_TRIANGLES, 0, 3);

    ver[0][0] = x2; ver[0][1] = y2; ver[0][2] = z2;
    ver[1][0] = x1; ver[1][1] = y2; ver[1][2] = z1;
    ver[2][0] = x1; ver[2][1] = y1; ver[2][2] = z1;

    glDrawArrays (GL_TRIANGLES, 0, 3);
  }

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);

}

void set_color(GLfloat red, GLfloat blue, GLfloat green, GLfloat alpha)
{
  for (int i=0; i<4; i++)
  {
    // Setup Colour Values
    col[i][0] = red;
    col[i][1] = blue;
    col[i][2] = green;
    col[i][3] = alpha;
  }
}


void draw_bar(GLfloat x_offset, GLfloat z_offset, GLfloat height, GLfloat red, GLfloat green, GLfloat blue )
{
  GLfloat width = 0.1;

  if (g_mode == GL_POINTS)
    set_color(0.2f, 1.0f, 0.2f, 1.0f);

  if (g_mode != GL_POINTS)
  {
    set_color(red, green, blue, 1.0f);
    draw_rectangle(x_offset, height, z_offset, x_offset + width, height, z_offset + 0.1f);
  }
  draw_rectangle(x_offset, 0.0f, z_offset, x_offset + width, 0.0f, z_offset + 0.1f);

  if (g_mode != GL_POINTS)
  {
    set_color(0.5f * red, 0.5f * green, 0.5f * blue, 1.0f);
    draw_rectangle(x_offset, 0.0f, z_offset + 0.1f, x_offset + width, height, z_offset + 0.1f);
  }
  draw_rectangle(x_offset, 0.0f, z_offset, x_offset + width, height, z_offset );

  if (g_mode != GL_POINTS)
  {
    set_color(0.25f * red, 0.25f * green, 0.25f * blue, 1.0f);
    draw_rectangle(x_offset, 0.0f, z_offset , x_offset, height, z_offset + 0.1f);
  }
  draw_rectangle(x_offset + width, 0.0f, z_offset , x_offset + width, height, z_offset + 0.1f);
}
#else

void draw_bar(GLfloat x_offset, GLfloat z_offset, GLfloat height, GLfloat red, GLfloat green, GLfloat blue )
{
  GLfloat col[] =  {
                      red * 0.1f, green * 0.1f, blue * 0.1f,
                      red * 0.2f, green * 0.2f, blue * 0.2f,
                      red * 0.3f, green * 0.3f, blue * 0.3f,
                      red * 0.4f, green * 0.4f, blue * 0.4f,
                      red * 0.5f, green * 0.5f, blue * 0.5f,
                      red * 0.6f, green * 0.6f, blue * 0.6f,
                      red * 0.7f, green * 0.7f, blue * 0.7f,
                      red * 0.8f, green * 0.8f, blue *0.8f
                   };
  GLfloat ver[] =  {
                      x_offset + 0.0f, 0.0f,    z_offset + 0.0f,
                      x_offset + 0.1f, 0.0f,    z_offset + 0.0f,
                      x_offset + 0.1f, 0.0f,    z_offset + 0.1f,
                      x_offset + 0.0f, 0.0f,    z_offset + 0.1f,
                      x_offset + 0.0f, height,  z_offset + 0.0f,
                      x_offset + 0.1f, height,  z_offset + 0.0f,
                      x_offset + 0.1f, height,  z_offset + 0.1f,
                      x_offset + 0.0f, height,  z_offset + 0.1f
                   };

  GLubyte idx[] =  {
                      // Bottom
                      0, 1, 2,
                      0, 2, 3,
                      // Left
                      0, 4, 7,
                      0, 7, 3,
                      // Back
                      3, 7, 6,
                      3, 6, 2,
                      // Right
                      1, 5, 6,
                      1, 6, 2,
                      // Front
                      0, 4, 5,
                      0, 5, 1,
                      // Top
                      4, 5, 6,
                      4, 6, 7
                   };

  GLint   posLoc = m_shader->GetPosLoc();
  GLint   colLoc = m_shader->GetColLoc();

  glVertexAttribPointer(colLoc, 3, GL_FLOAT, 0, 0, col);
  glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, 0, ver);

  glEnableVertexAttribArray(posLoc);
  glEnableVertexAttribArray(colLoc);

  glDrawElements(g_mode, 36, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);
}
#endif

void draw_bars(void)
{
  int x,y;
  GLfloat x_offset, z_offset, r_base, b_base;

  glClear(GL_DEPTH_BUFFER_BIT);
  g_matricesSpectrum.PushMatrix();
  g_matricesSpectrum.Translatef(0.0f ,-0.5f, -5.0f);
  g_matricesSpectrum.Rotatef(DEG2RAD(x_angle), 1.0f, 0.0f, 0.0f);
  g_matricesSpectrum.Rotatef(DEG2RAD(y_angle), 0.0f, 1.0f, 0.0f);
  g_matricesSpectrum.Rotatef(DEG2RAD(z_angle), 0.0f, 0.0f, 1.0f);

  m_shader->Enable();

  for(y = 0; y < 16; y++)
  {
    z_offset = -1.6f + ((15.0f - y) * 0.2f);

    b_base = y * (1.0f / 15.0f);
    r_base = 1.0f - b_base;

    for(x = 0; x < 16; x++)
    {
      x_offset = -1.6f + ((float)x * 0.2f);
      if (::fabs(cHeights[y][x]-heights[y][x])>hSpeed)
      {
        if (cHeights[y][x]<heights[y][x])
          cHeights[y][x] += hSpeed;
        else
          cHeights[y][x] -= hSpeed;
      }
      draw_bar(x_offset, z_offset,
        cHeights[y][x], r_base - (float(x) * (r_base / 15.0f)),
        (float)x * (1.0f / 15.0f), b_base);
    }
  }

  m_shader->Disable();

  g_matricesSpectrum.PopMatrix();
}

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  m_shader = new CGUIShader(vert, frag);

  if(!m_shader)
    return ADDON_STATUS_UNKNOWN;

  if(!m_shader->CompileAndLink())
  {
    delete m_shader;
    return ADDON_STATUS_UNKNOWN;
  }

  return ADDON_STATUS_NEED_SETTINGS;
}

void render_waveform()
{
  GLfloat col[256][3];
  GLfloat ver[256][3];
  GLubyte idx[256];

  glDisable(GL_BLEND);

  g_matricesSpectrum.MatrixMode(MM_PROJECTION);
  g_matricesSpectrum.PushMatrix();
  g_matricesSpectrum.LoadIdentity();
  //g_matricesSpectrum.Frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.5f, 10.0f);
  g_matricesSpectrum.MatrixMode(MM_MODELVIEW);
  g_matricesSpectrum.PushMatrix();
  g_matricesSpectrum.LoadIdentity();

  g_matricesSpectrum.PushMatrix();
  g_matricesSpectrum.Translatef(0.0f ,0.0f ,-1.0f);
  g_matricesSpectrum.Rotatef(0.0f, 1.0f, 0.0f, 0.0f);
  g_matricesSpectrum.Rotatef(0.0f, 0.0f, 1.0f, 0.0f);
  g_matricesSpectrum.Rotatef(0.0f, 0.0f, 0.0f, 1.0f);

  m_shader->Enable();

  GLint   posLoc = m_shader->GetPosLoc();
  GLint   colLoc = m_shader->GetColLoc();

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
    ver[i][1] = 0.5f + (g_fWaveform[0][i] * 0.000015f);
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
    ver[i][1] = -0.5f + (g_fWaveform[1][i] * 0.000015f);
    ver[i][2] = 1.0f;
    idx[i] = i;

  }

  glDrawElements(GL_LINE_STRIP, 256, GL_UNSIGNED_BYTE, idx);

  glDisableVertexAttribArray(posLoc);
  glDisableVertexAttribArray(colLoc);

  m_shader->Disable();

  g_matricesSpectrum.PopMatrix();

  g_matricesSpectrum .PopMatrix();
  g_matricesSpectrum.MatrixMode(MM_PROJECTION);
  g_matricesSpectrum.PopMatrix();

  glEnable(GL_BLEND);
  
}

void render_3d_spectrum()
{
  glDisable(GL_BLEND);

  g_matricesSpectrum.MatrixMode(MM_PROJECTION);
  g_matricesSpectrum.PushMatrix();
  g_matricesSpectrum.LoadIdentity();
  g_matricesSpectrum.Frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.5f, 10.0f);
  g_matricesSpectrum.MatrixMode(MM_MODELVIEW);
  g_matricesSpectrum.PushMatrix();
  g_matricesSpectrum.LoadIdentity();

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

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

  g_matricesSpectrum .PopMatrix();
  g_matricesSpectrum.MatrixMode(MM_PROJECTION);
  g_matricesSpectrum.PopMatrix();

  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
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

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  switch(g_vis)
  {
    case VIS_3D_SPECTRUM:
      render_3d_spectrum();
      break;
    case VIS_WAVEFORM:
      render_waveform();
      break;
  }
}
extern "C" void AudioData(const short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  int i,c;
  int y=0;
  GLfloat val;

  int xscale[] = {0, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 255};

  if(g_vis == VIS_WAVEFORM) 
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

  } else if(g_vis == VIS_3D_SPECTRUM) {
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
            y = (int)(pAudioData[c]);
        }
        else
          continue;
      }
      y >>= 7;
      if(y > 0)
        val = (logf(y) * scale);
      else
        val = 0;
      heights[0][i] = val;
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
  if(m_shader) 
  {
    m_shader->Free();
    delete m_shader;
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
  if (!strSetting || !value)
    return ADDON_STATUS_UNKNOWN;

  if (strcmp(strSetting, "vis")==0)
  {
    switch (*(int*) value)
    {
      case 1:
        g_vis = VIS_WAVEFORM;
        break;
      default:
        g_vis = VIS_3D_SPECTRUM;
        break;
    }
    return ADDON_STATUS_OK;
  } 
  /*
  else if (strcmp(strSetting, "mode")==0)
  {
    switch (*(int*) value)
    {
      case 1:
        g_mode = GL_LINES;
        break;

      case 2:
        g_mode = GL_LINES;
        break;

      case 0:
      default:
        g_mode = GL_TRIANGLES;
        break;
    }
    return ADDON_STATUS_OK;
  }
  else if (strcmp(strSetting, "bar_height")==0)
  {
    switch (*(int*) value)
    {
    case 1:
      scale = 2.0f / log(256.f);
      break;

    case 2:
      scale = 3.0f / log(256.f);
      break;

    case 3:
      scale = 0.5f / log(256.f);
      break;

    case 4:
      scale = 0.33f / log(256.f);
      break;

    case 0:
    default:
      scale = 1.0f / log(256.f);
      break;
    }
    return ADDON_STATUS_OK;
  }
  else if (strcmp(strSetting, "speed")==0)
  {
    switch (*(int*) value)
    {
    case 1:
      hSpeed = 0.025f;
      break;

    case 2:
      hSpeed = 0.0125f;
      break;

    case 3:
      hSpeed = 0.1f;
      break;

    case 4:
      hSpeed = 0.2f;
      break;

    case 0:
    default:
      hSpeed = 0.05f;
      break;
    }
    return ADDON_STATUS_OK;
  }
  */

  return ADDON_STATUS_UNKNOWN;
}

#endif
