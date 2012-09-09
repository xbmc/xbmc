/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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


/*

Goom Visualization Interface for XBMC
- Team XBMC

*/

#define __STDC_LIMIT_MACROS

#include "../../addons/include/xbmc_vis_dll.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>
extern "C" {
#include "goom.h"
}
#include "goom_config.h"
#include <GL/glew.h>

extern int  preset_index;
char        g_visName[512];
PluginInfo* g_goom  = NULL;

int g_tex_width     = GOOM_TEXTURE_WIDTH;
int g_tex_height    = GOOM_TEXTURE_HEIGHT;
int g_window_width  = 512;
int g_window_height = 512;
int g_window_xpos   = 0;
int g_window_ypos   = 0;

GLuint         g_texid       = 0;
unsigned char* g_goom_buffer = NULL;
short          g_audio_data[2][512];
std::string    g_configFile;

using namespace std;

//-- Create -------------------------------------------------------------------
// Called once when the visualisation is created by XBMC. Do any setup here.
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  if (!props)
    return ADDON_STATUS_UNKNOWN;

  VIS_PROPS* visprops = (VIS_PROPS*)props;

  strcpy(g_visName, visprops->name);
  g_configFile = string(visprops->profile) + string("/goom.conf");
  std::string presetsDir = string(visprops->presets) + string("/resources");

  /** Initialise Goom */
  if (g_goom)
  {
    goom_close( g_goom );
    g_goom = NULL;
  }

  g_goom = goom_init(g_tex_width, g_tex_height);
  if (!g_goom)
    return ADDON_STATUS_UNKNOWN;

  g_goom_buffer = (unsigned char*)malloc(g_tex_width * g_tex_height * 4);
  goom_set_screenbuffer( g_goom, g_goom_buffer );
  memset( g_audio_data, 0, sizeof(g_audio_data) );
  g_window_width = visprops->width;
  g_window_height = visprops->height;
  g_window_xpos = visprops->x;
  g_window_ypos = visprops->y;

  return ADDON_STATUS_OK;
}

//-- Destroy -------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Destroy()
{
  if ( g_goom )
  {
    goom_close( g_goom );
    g_goom = NULL;
  }
  if ( g_goom_buffer )
  {
    free( g_goom_buffer );
    g_goom_buffer = NULL;
  }
}

//-- Start --------------------------------------------------------------------
// Called when a new soundtrack is played
//-----------------------------------------------------------------------------
extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
  if ( g_goom )
  {
    goom_update( g_goom, g_audio_data, 0, 0, (char*)szSongName, (char*)"XBMC" );
  }
}

//-- Stop ---------------------------------------------------------------------
// Called when the visualisation is closed by XBMC
//-----------------------------------------------------------------------------
extern "C" void ADDON_Stop()
{
  if (g_texid)
  {
    glDeleteTextures( 1, &g_texid );
    g_texid = 0;
  }
}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
extern "C" void AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  int copysize = iAudioDataLength < (int)sizeof( g_audio_data ) >> 1 ? iAudioDataLength : (int)sizeof( g_audio_data ) >> 1;
  int ipos, i;
  for(ipos = 0, i = 0; i < copysize; i += 2, ++ipos)
  {
    g_audio_data[0][ipos] = (int)(pAudioData[i  ] * (INT16_MAX+.5f));
    g_audio_data[1][ipos] = (int)(pAudioData[i+1] * (INT16_MAX+.5f));
  }
}


//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  if ( g_goom )
  {
    goom_set_screenbuffer( g_goom, g_goom_buffer );
    if (!g_texid)
    {
      // initialize the texture we'll be using
      glGenTextures( 1, &g_texid );
      if (!g_texid)
        return;
      goom_update( g_goom, g_audio_data, 0, 0, NULL, (char*)"XBMC" );
      glEnable(GL_TEXTURE_2D);
      glBindTexture( GL_TEXTURE_2D, g_texid );
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D( GL_TEXTURE_2D, 0, 4, g_tex_width, g_tex_height, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, g_goom_buffer );
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    else
    {
      // update goom frame and copy to our texture
      goom_update( g_goom, g_audio_data, 0, 0, NULL, (char*)"XBMC" );
      glEnable(GL_TEXTURE_2D);
      glBindTexture( GL_TEXTURE_2D, g_texid );
      glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, g_tex_width, g_tex_height,
                       GL_RGBA, GL_UNSIGNED_BYTE, g_goom_buffer );
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    glDisable(GL_BLEND);
    glBegin( GL_QUADS );
    {
      glColor3f( 1.0, 1.0, 1.0 );
      glTexCoord2f( 0.0, 0.0 );
      glVertex2f( g_window_xpos, g_window_ypos );

      glTexCoord2f( 0.0, 1.0 );
      glVertex2f( g_window_xpos, g_window_ypos + g_window_height );

      glTexCoord2f( 1.0, 1.0 );
      glVertex2f( g_window_xpos + g_window_width, g_window_ypos + g_window_height );

      glTexCoord2f( 1.0, 0.0 );
      glVertex2f( g_window_xpos + g_window_width, g_window_ypos );
    }
    glEnd();
    glDisable( GL_TEXTURE_2D );
    glEnable(GL_BLEND);
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
