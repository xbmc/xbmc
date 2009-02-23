/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


/* 

Goom Visualization Interface for XBMC
- Team XBMC

*/


#include "xbmc_vis.h"
extern "C" {
#include "goom.h"
}
#include "goom_config.h"
#include <GL/glew.h>
#include <string>
#ifdef _WIN32PC
#ifndef _MINGW
#include "win32-dirent.h"
#endif
#include <io.h>
#else
#include "system.h"
#include "FileSystem/SpecialProtocol.h"
#include <dirent.h>
#endif

#ifdef _WIN32PC
#define PRESETS_DIR "visualisations\\goom"
#define CONFIG_FILE "visualisations\\goom.conf"
#define strcasecmp  stricmp
#else
#define PRESETS_DIR "special://xbmc/visualisations/goom"
#define CONFIG_FILE "special://profile/visualisations/goom.conf"
#endif

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

// case-insensitive alpha sort from projectM's win32-dirent.cc
#ifndef _WIN32PC
int alphasort(const void* lhs, const void* rhs) 
{
  const struct dirent* lhs_ent = *(struct dirent**)lhs;
  const struct dirent* rhs_ent = *(struct dirent**)rhs;
  return strcasecmp(lhs_ent->d_name, rhs_ent->d_name);
}
#endif

// check for a valid preset extension
#ifdef __APPLE__
int check_valid_extension(struct dirent* ent) 
#else
int check_valid_extension(const struct dirent* ent) 
#endif
{
#ifndef _MINGW
  const char* ext = 0;
  
  if (!ent) return 0;
  
  ext = strrchr(ent->d_name, '.');
  if (!ext) ext = ent->d_name;
  
  if (0 == strcasecmp(ext, ".milk")) return 1;
  if (0 == strcasecmp(ext, ".prjm")) return 1;
#endif
  return 0;
}

//-- Create -------------------------------------------------------------------
// Called once when the visualisation is created by XBMC. Do any setup here.
//-----------------------------------------------------------------------------
#ifdef HAS_XBOX_HARDWARE
extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName,
                       float fPixelRatio, const char *szSubModuleName)
#else
extern "C" void Create(void* pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName,
                       float fPixelRatio, const char *szSubModuleName)
#endif
{
  strcpy(g_visName, szVisualisationName);
  m_vecSettings.clear();

  /** Initialise Goom */
  if (g_goom)
  {
    goom_close( g_goom );
    g_goom = NULL;
  }

  g_goom = goom_init(g_tex_width, g_tex_height);
  if (!g_goom)
    return;

  g_goom_buffer = (unsigned char*)malloc(g_tex_width * g_tex_height * 4);
  goom_set_screenbuffer( g_goom, g_goom_buffer );
  memset( g_audio_data, 0, sizeof(g_audio_data) );
  g_window_width = iWidth;
  g_window_height = iHeight;
  g_window_xpos = iPosX;
  g_window_ypos = iPosY;

#ifdef _WIN32PC
#ifndef _MINGW
  g_configFile = string(getenv("XBMC_PROFILE_USERDATA")) + "\\" + CONFIG_FILE;
  std::string presetsDir = string(getenv("XBMC_HOME")) + "\\" + PRESETS_DIR;
#endif
#else
  g_configFile = _P(CONFIG_FILE);
  std::string presetsDir = _P(PRESETS_DIR);
#endif

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
extern "C" void Stop()
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
  if (g_texid)
  {
    glDeleteTextures( 1, &g_texid );
  }
  m_vecSettings.clear(); 
}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
extern "C" void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  int copysize = iAudioDataLength < (int)sizeof( g_audio_data ) ? iAudioDataLength : (int)sizeof( g_audio_data );
  memcpy( g_audio_data, pAudioData, copysize );
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
extern "C" void GetSettings(vector<VisSetting> **vecSettings)
{
  if (!vecSettings)
    return;
  *vecSettings = &m_vecSettings;
}

//-- UpdateSetting ------------------------------------------------------------
// Handle setting change request from XBMC
//-----------------------------------------------------------------------------
extern "C" void UpdateSetting(int num)
{
  //VisSetting &setting = m_vecSettings[num];
}

//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" int GetSubModules(char ***names, char ***paths)
{
  return 0; // this vis supports 0 sub modules
}
