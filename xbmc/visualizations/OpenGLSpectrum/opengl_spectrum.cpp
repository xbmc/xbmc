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


#ifdef HAS_XBOX_HARDWARE
#include <xtl.h>
#endif
#include "../../../visualisations/xbmc_vis.h"
#include <math.h>
#include <GL/glew.h>

#define NUM_BANDS 16

GLfloat y_angle = 45.0, y_speed = 0.5;
GLfloat x_angle = 20.0, x_speed = 0.0;
GLfloat z_angle = 0.0, z_speed = 0.0;
GLfloat heights[16][16], cHeights[16][16], scale;
GLfloat hSpeed = 0.05;
GLenum  g_mode = GL_FILL;
vector<VisSetting> g_vecSettings;

extern "C" void Create(void* pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName,
                       float fPixelRatio, const char *szSubModuleName)
{
  g_vecSettings.clear();
  m_uiVisElements = 0;
  VisSetting scale(VisSetting::SPIN, "Bar Height");
  scale.AddEntry("Default");
  scale.AddEntry("Big");
  scale.AddEntry("Very Big");
  scale.AddEntry("Small");

  VisSetting mode(VisSetting::SPIN, "Mode");
  mode.AddEntry("Default");
  mode.AddEntry("Wireframe");
  mode.AddEntry("Points");

  VisSetting speed(VisSetting::SPIN, "Speed");
  speed.AddEntry("Default");
  speed.AddEntry("Slow");
  speed.AddEntry("Very Slow");
  speed.AddEntry("Fast");
  speed.AddEntry("Very Fast");

  g_vecSettings.push_back( scale );
  g_vecSettings.push_back( mode );  
  g_vecSettings.push_back( speed );
}

void draw_rectangle(GLfloat x1, GLfloat y1, GLfloat z1, GLfloat x2, GLfloat y2, GLfloat z2)
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

void draw_bar(GLfloat x_offset, GLfloat z_offset, GLfloat height, GLfloat red, GLfloat green, GLfloat blue )
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

void draw_bars(void)
{
  int x,y;
  GLfloat x_offset, z_offset, r_base, b_base;

  //glClearColor(0,0,0,0);
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
          x_offset = -1.6 + (x * 0.2);			
          if (::fabs(cHeights[y][x]-heights[y][x])>hSpeed) {
            if (cHeights[y][x]<heights[y][x]) {
              cHeights[y][x] += hSpeed;
            } else {
              cHeights[y][x] -= hSpeed;
            }
          }
          draw_bar(x_offset, z_offset, 
		   cHeights[y][x], r_base - (x * (r_base / 15.0)), 
		   x * (1.0 / 15), b_base);
        }
    }
  glEnd();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glPopMatrix();
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  bool configured = true; //FALSE;

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
  if(configured)
    {
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
    }
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

  scale = 1.0 / log(256.0);

  x_speed = 0.0;
  y_speed = 0.5;
  z_speed = 0.0;
  x_angle = 20.0;
  y_angle = 45.0;
  z_angle = 0.0;
}

extern "C" void Stop()
{

}

extern "C" void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
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
          if (c<iAudioDataLength) {
            if(pAudioData[c] > y)
              y = (int)pAudioData[c];
          } else {
            continue;
          }
        }
      y >>= 7;
      if(y > 0)
#ifdef _WIN32PC
        val = (logf(y) * scale);
#else
        val = (logf(y) * scale);
#endif
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


//-- OnAction -----------------------------------------------------------------
// Handle XBMC actions such as next preset, lock preset, album art changed etc
//-----------------------------------------------------------------------------
extern "C" bool OnAction(long flags, void *param)
{
  bool ret = false;
  return ret;
}

//-- GetPresets ---------------------------------------------------------------
// Return a list of presets to XBMC for display
//-----------------------------------------------------------------------------
extern "C" void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{

}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
//-----------------------------------------------------------------------------
extern "C" unsigned int GetSettings(StructSetting*** sSet)
{ 
  m_uiVisElements = VisUtils::VecToStruct(g_vecSettings, &m_structSettings);
  *sSet = m_structSettings;
  return m_uiVisElements;
}

extern "C" void FreeSettings()
{
  VisUtils::FreeStruct(m_uiVisElements, &m_structSettings);
}

//-- UpdateSetting ------------------------------------------------------------
// Handle setting change request from XBMC
//-----------------------------------------------------------------------------
extern "C" void UpdateSetting(int num, StructSetting*** sSet)
{
  VisUtils::StructToVec(m_uiVisElements, sSet, &g_vecSettings);

  if ( (int)g_vecSettings.size() <= num || num < 0 )
    return;

  if (strcmp(g_vecSettings[num].name, "Size")==0)
  {
    switch (g_vecSettings[num].current)
      {
      case 0:
	scale = 1.0 / log(256.0);
	break;

      case 1:
	scale = 2.0 / log(256.0);
	break;

      case 2:
	scale = 3.0 / log(256.0);
	break;

      case 3:
	scale = 0.5 / log(256.0);
	break;

      case 4:
	scale = 0.33 / log(256.0);
	break;
      }
  }

  if (strcmp(g_vecSettings[num].name, "Speed")==0)
  {
    switch (g_vecSettings[num].current)
      {
      case 0:
	hSpeed = 0.05;
	break;

      case 1:
	hSpeed = 0.025;
	break;

      case 2:
	hSpeed = 0.0125;
	break;

      case 3:
	hSpeed = 0.10;
	break;

      case 4:
	hSpeed = 0.20;
	break;
      }
  }

  if (strcmp(g_vecSettings[num].name, "Mode")==0)
  {
    switch (g_vecSettings[num].current)
      {
      case 0:
	g_mode = GL_FILL;
	break;

      case 1:
	g_mode = GL_LINE;
	break;

      case 2:
	g_mode = GL_POINT;
	break;
      }
  }
}

//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" int GetSubModules(char ***names, char ***paths)
{
  return 0; // this vis supports 0 sub modules
}
