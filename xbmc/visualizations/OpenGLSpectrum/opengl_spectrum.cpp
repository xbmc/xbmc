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
 *
 *  Ported to GLES 2.0 by Gimli
 */

#define __STDC_LIMIT_MACROS

#include "addons/include/xbmc_vis_dll.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#if defined(HAS_GLES)
#include "VisGUIShader.h"

#ifndef M_PI
#define M_PI       3.141592654f
#endif
#define DEG2RAD(d) ( (d) * M_PI/180.0f )

//OpenGL wrapper - allows us to use same code of functions draw_bars and render
#define GL_PROJECTION             MM_PROJECTION
#define GL_MODELVIEW              MM_MODELVIEW

#define glPushMatrix()            vis_shader->PushMatrix()
#define glPopMatrix()             vis_shader->PopMatrix()
#define glTranslatef(x,y,z)       vis_shader->Translatef(x,y,z)
#define glRotatef(a,x,y,z)        vis_shader->Rotatef(DEG2RAD(a),x,y,z)
#define glPolygonMode(a,b)        ;
#define glBegin(a)                vis_shader->Enable()
#define glEnd()                   vis_shader->Disable()
#define glMatrixMode(a)           vis_shader->MatrixMode(a)
#define glLoadIdentity()          vis_shader->LoadIdentity()
#define glFrustum(a,b,c,d,e,f)    vis_shader->Frustum(a,b,c,d,e,f)

static GLenum  g_mode = GL_TRIANGLES;
static const char *frag = "precision mediump float; \n"
                   "varying lowp vec4 m_colour; \n"
                   "void main () \n"
                   "{ \n"
                   "  gl_FragColor = m_colour; \n"
                   "}\n";

static const char *vert = "attribute vec4 m_attrpos;\n"
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

static CVisGUIShader *vis_shader = NULL;

#elif defined(HAS_GL)
#include <GL/glew.h>
static GLenum  g_mode = GL_FILL;

#endif

#define NUM_BANDS 16

static GLfloat x_angle = 20.0, x_speed = 0.0;
static GLfloat y_angle = 45.0, y_speed = 0.5;
static GLfloat z_angle = 0.0, z_speed = 0.0;
static GLfloat heights[16][16], cHeights[16][16], scale;
static GLfloat hSpeed = 0.05;

#if defined(HAS_GL)
static void draw_rectangle(GLfloat x1, GLfloat y1, GLfloat z1, GLfloat x2, GLfloat y2, GLfloat z2)
{
  if(y1 == y2)
  {
    glVertex3f(x1, y1, z1);
    glVertex3f(x2, y1, z1);
    glVertex3f(x2, y2, z2);

    glVertex3f(x2, y2, z2);
    glVertex3f(x1, y2, z2);
    glVertex3f(x1, y1, z1);
  }
  else
  {
    glVertex3f(x1, y1, z1);
    glVertex3f(x2, y1, z2);
    glVertex3f(x2, y2, z2);

    glVertex3f(x2, y2, z2);
    glVertex3f(x1, y2, z1);
    glVertex3f(x1, y1, z1);
  }
}

static void draw_bar(GLfloat x_offset, GLfloat z_offset, GLfloat height, GLfloat red, GLfloat green, GLfloat blue, int index)
{
  GLfloat width = 0.1;

  if (g_mode == GL_POINT)
    glColor3f(0.2, 1.0, 0.2);

  if (g_mode != GL_POINT)
  {
    glColor3f(red,green,blue);
    draw_rectangle(x_offset, height, z_offset, x_offset + width, height, z_offset + 0.1);
  }
  draw_rectangle(x_offset, 0, z_offset, x_offset + width, 0, z_offset + 0.1);

  if (g_mode != GL_POINT)
  {
    glColor3f(0.5 * red, 0.5 * green, 0.5 * blue);
    draw_rectangle(x_offset, 0.0, z_offset + 0.1, x_offset + width, height, z_offset + 0.1);
  }
  draw_rectangle(x_offset, 0.0, z_offset, x_offset + width, height, z_offset );

  if (g_mode != GL_POINT)
  {
    glColor3f(0.25 * red, 0.25 * green, 0.25 * blue);
    draw_rectangle(x_offset, 0.0, z_offset , x_offset, height, z_offset + 0.1);
  }
  draw_rectangle(x_offset + width, 0.0, z_offset , x_offset + width, height, z_offset + 0.1);
}

#elif defined(HAS_GLES)

static GLfloat  *m_col;
static GLfloat  *m_ver;
static GLushort *m_idx;

static void draw_bar(GLfloat x_offset, GLfloat z_offset, GLfloat height, GLfloat red, GLfloat green, GLfloat blue, int index)
{
  if (!m_col || !m_ver || !m_idx)
    return;

  // avoid zero sized bars, which results in overlapping triangles of same depth and display artefacts
  height = std::max(height, 1e-3f);
  GLfloat *ver = m_ver + 3 * 8 * index;
  // just need to update the height vertex, all else is the same
  for (int i=0; i<8; i++)
  {
    ver[1] = (((i+0)>>2)&1) * height;
    ver += 3;
  }
  // on last index, draw the object
  if (index == 16*16-1)
  {
    GLint   posLoc = vis_shader->GetPosLoc();
    GLint   colLoc = vis_shader->GetColLoc();

    glVertexAttribPointer(colLoc, 3, GL_FLOAT, 0, 0, m_col);
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, 0, 0, m_ver);

    glEnableVertexAttribArray(posLoc);
    glEnableVertexAttribArray(colLoc);

    glDrawElements(g_mode, 16*16*36, GL_UNSIGNED_SHORT, m_idx);

    glDisableVertexAttribArray(posLoc);
    glDisableVertexAttribArray(colLoc);
  }
}

static void init_bars(void)
{
  if (!m_col || !m_ver || !m_idx)
    return;

  GLfloat x_offset, z_offset, r_base, b_base;
  for(int y = 0; y < 16; y++)
  {
    z_offset = -1.6 + ((15 - y) * 0.2);

    b_base = y * (1.0 / 15);
    r_base = 1.0 - b_base;

    for(int x = 0; x < 16; x++)
    {
      x_offset = -1.6 + ((float)x * 0.2);

      GLfloat red = r_base - (float(x) * (r_base / 15.0));
      GLfloat green = (float)x * (1.0 / 15);
      GLfloat blue = b_base;
      int index = 16*y+x;
      GLfloat *col = m_col + 3 * 8 * index;
      for (int i=0; i<8; i++)
      {
        float scale = 0.1f * i;
        *col++ = red   * scale;
        *col++ = green * scale;
        *col++ = blue  * scale;
      }
      GLfloat *ver = m_ver + 3 * 8 * index;
      for (int i=0; i<8; i++)
      {
        *ver++ = x_offset + (((i+1)>>1)&1) * 0.1f;
        *ver++ = 0; // height - filled in later
        *ver++ = z_offset + (((i+0)>>1)&1) * 0.1f;
      }
      GLushort *idx = m_idx + 36 * index;
      GLushort startidx = 8 * index;
      // Bottom
      *idx++ = startidx + 0; *idx++ = startidx + 1; *idx++ = startidx + 2;
      *idx++ = startidx + 0; *idx++ = startidx + 2; *idx++ = startidx + 3;
      // Left
      *idx++ = startidx + 0; *idx++ = startidx + 4; *idx++ = startidx + 7;
      *idx++ = startidx + 0; *idx++ = startidx + 7; *idx++ = startidx + 3;
      // Back
      *idx++ = startidx + 3; *idx++ = startidx + 7; *idx++ = startidx + 6;
      *idx++ = startidx + 3; *idx++ = startidx + 6; *idx++ = startidx + 2;
      // Right
      *idx++ = startidx + 1; *idx++ = startidx + 5; *idx++ = startidx + 6;
      *idx++ = startidx + 1; *idx++ = startidx + 6; *idx++ = startidx + 2;
      // Front
      *idx++ = startidx + 0; *idx++ = startidx + 4; *idx++ = startidx + 5;
      *idx++ = startidx + 0; *idx++ = startidx + 5; *idx++ = startidx + 1;
      // Top
      *idx++ = startidx + 4; *idx++ = startidx + 5; *idx++ = startidx + 6;
      *idx++ = startidx + 4; *idx++ = startidx + 6; *idx++ = startidx + 7;
    }
  }
}
#endif

static void draw_bars(void)
{
  int x,y;
  GLfloat x_offset, z_offset, r_base, b_base;

  glClear(GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
  glTranslatef(0.0,-0.5,-5.0);
  glRotatef(x_angle,1.0,0.0,0.0);
  glRotatef(y_angle,0.0,1.0,0.0);
  glRotatef(z_angle,0.0,0.0,1.0);
  
  glPolygonMode(GL_FRONT_AND_BACK, g_mode);
  glBegin(GL_TRIANGLES);
  
  for(y = 0; y < 16; y++)
  {
    z_offset = -1.6 + ((15 - y) * 0.2);

    b_base = y * (1.0 / 15);
    r_base = 1.0 - b_base;

    for(x = 0; x < 16; x++)
    {
      x_offset = -1.6 + ((float)x * 0.2);
      if (::fabs(cHeights[y][x]-heights[y][x])>hSpeed)
      {
        if (cHeights[y][x]<heights[y][x])
          cHeights[y][x] += hSpeed;
        else
          cHeights[y][x] -= hSpeed;
      }
      draw_bar(x_offset, z_offset,
        cHeights[y][x], r_base - (float(x) * (r_base / 15.0)),
        (float)x * (1.0 / 15), b_base, 16*y+x);
    }
  }
  glEnd();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glPopMatrix();
}

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  scale = 1.0 / log(256.0);

#if defined(HAS_GLES)
  vis_shader = new CVisGUIShader(vert, frag);

  if(!vis_shader)
    return ADDON_STATUS_UNKNOWN;

  if(!vis_shader->CompileAndLink())
  {
    delete vis_shader;
    return ADDON_STATUS_UNKNOWN;
  }  
  m_col = (GLfloat  *)malloc(16*16*3*8 * sizeof(GLfloat *));
  m_ver = (GLfloat  *)malloc(16*16*3*8 * sizeof(GLfloat *));
  m_idx = (GLushort *)malloc(16*16*36  * sizeof(GLushort *));
  init_bars();
#endif

  scale = 1.0 / log(256.0);

  return ADDON_STATUS_NEED_SETTINGS;
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  glDisable(GL_BLEND);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glFrustum(-1, 1, -1, 1, 1.5, 10);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glPolygonMode(GL_FRONT, GL_FILL);
  //glPolygonMode(GL_BACK, GL_FILL);
  x_angle += x_speed;
  if(x_angle >= 360.0)
    x_angle -= 360.0;

  y_angle += y_speed;
  if(y_angle >= 360.0)
    y_angle -= 360.0;

  z_angle += z_speed;
  if(z_angle >= 360.0)
    z_angle -= 360.0;

  draw_bars();
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
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
      cHeights[y][x] = 0.0;
    }
  }

  x_speed = 0.0;
  y_speed = 0.5;
  z_speed = 0.0;
  x_angle = 20.0;
  y_angle = 45.0;
  z_angle = 0.0;
}

extern "C" void AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  int i,c;
  int y=0;
  GLfloat val;

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
        if((int)(pAudioData[c] * (INT16_MAX)) > y)
          y = (int)(pAudioData[c] * (INT16_MAX));
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
#if defined(HAS_GLES)
  if(vis_shader) 
  {
    vis_shader->Free();
    delete vis_shader;
  }
  free(m_col);
  free(m_ver);
  free(m_idx);
  m_col = NULL;
  m_ver = NULL;
  m_idx = NULL;
#endif
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
    case 1://standard
      scale = 1.f / log(256.f);
      break;

    case 2://big
      scale = 2.f / log(256.f);
      break;

    case 3://real big
      scale = 3.f / log(256.f);
      break;

    case 4://unused
      scale = 0.33f / log(256.f);
      break;

    case 0://small
    default:
      scale = 0.5f / log(256.f);
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
  else if (strcmp(strSetting, "mode")==0)
  {
#if defined(HAS_GL)
    switch (*(int*) value)
    {
      case 1:
        g_mode = GL_LINE;
        break;

      case 2:
        g_mode = GL_POINT;
        break;

      case 0:
      default:
        g_mode = GL_FILL;
        break;
    }
#else
    switch (*(int*) value)
    {
      case 1:
        g_mode = GL_LINE_LOOP;
        break;

      case 2:
        g_mode = GL_LINES; //no points on gles!
        break;

      case 0:
      default:
        g_mode = GL_TRIANGLES;
        break;
    }

#endif

    return ADDON_STATUS_OK;
  }

  return ADDON_STATUS_UNKNOWN;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
}
