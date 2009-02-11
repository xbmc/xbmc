/*
 *      Copyright (C) 2005-2009 Team XBMC
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
  iTunes Visualization Wrapper for XBMC
*/


#include "xbmc_vis.h"
#include <GL/glew.h>
#include <string>
#ifdef _WIN32PC
#ifndef _MINGW
#include "win32-dirent.h"
#endif
#include <io.h>
#else
#include "system.h"
#include "Util.h"
#include <dirent.h>
#endif
#ifdef _LINUX
#include <dlfcn.h>
#endif
#ifdef __APPLE__
#include "itunes_vis.h"
#endif

using namespace std;

int g_tex_width     = 512;
int g_tex_height    = 512;
int g_window_width  = 512;
int g_window_height = 512;
int g_window_xpos   = 0;
int g_window_ypos   = 0;

short       g_audio_data[2][512];
float       g_freq_data[2][512];
bool        g_new_audio;
string      g_sub_module;
string      g_vis_name;
GLuint      g_tex_id          = 0;
GLbyte*     g_tex_buffer      = NULL;
long        g_tex_buffer_size = 0;
ITunesVis*  g_plugin          = NULL;

//-- Create -------------------------------------------------------------------
// Called once when the visualisation is created by XBMC. Do any setup here.
//-----------------------------------------------------------------------------
extern "C" void Create(void* pd3dDevice, int iPosX, int iPosY, int iWidth,
                       int iHeight, const char* szVisualisationName,
                       float fPixelRatio, const char *szSubModuleName)
{
  if ( szVisualisationName )
    g_vis_name = szVisualisationName;
  m_vecSettings.clear();

  /* copy window dimensions */
  g_window_width  = g_tex_width  = iWidth;
  g_window_height = g_tex_height = iHeight;
  g_window_xpos   = iPosX;
  g_window_ypos   = iPosY;
  g_sub_module    = szSubModuleName;

  /* create texture buffer */
  g_tex_buffer_size = g_tex_width * g_tex_height * 4;
  g_tex_buffer      = (GLbyte*)malloc( g_tex_buffer_size );

  if ( !g_tex_buffer )
    return;

  if ( g_plugin )
  {
    ivis_close( g_plugin );
    g_plugin = NULL;
  }

  /* load the plugin */
  g_plugin = ivis_open( g_sub_module.c_str() );

  if ( g_plugin == NULL )
  {
    printf( "Error loading %s\n", g_vis_name.c_str() );
    return;
  }

  /* initialize and start the plugin */
  if ( ivis_init( g_plugin, g_tex_width, g_tex_height ) == false )
  {
    printf( "Error initializing %s\n", g_vis_name.c_str() );
    ivis_close( g_plugin );
    g_plugin = NULL;
  }
  else
  {
    ivis_start( g_plugin );
  }

  return;
}

//-- Start --------------------------------------------------------------------
// Called when a new soundtrack is played
//-----------------------------------------------------------------------------
extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample,
                      const char* szSongName)
{

}

//-- Stop ---------------------------------------------------------------------
// Called when the visualisation is closed by XBMC
//-----------------------------------------------------------------------------
extern "C" void Stop()
{
  if ( g_tex_id )
  {
    glDeleteTextures( 1, &g_tex_id );
  }
  ivis_close( g_plugin );
  free( g_tex_buffer );
  g_tex_buffer = NULL;
  g_tex_buffer_size = 0;
  g_plugin = NULL;
  g_new_audio = false;
}

//-- Audiodata ----------------------------------------------------------------
// Called by XBMC to pass new audio data to the vis
//-----------------------------------------------------------------------------
extern "C" void AudioData(short* pAudioData, int iAudioDataLength,
                          float *pFreqData, int iFreqDataLength)
{
  int copysize = iAudioDataLength  < (int)sizeof( g_audio_data ) ? iAudioDataLength  : (int)sizeof( g_audio_data );
  for (int i = 0 ; i < copysize*2 ; i+=2 )
  {
    g_audio_data[0][i/2] = *(pAudioData + i);
    g_audio_data[1][i/2] = *(pAudioData + i + 1);
  }
  if ( pFreqData )
  {
    copysize = iFreqDataLength  < (int)sizeof( g_freq_data ) ? iFreqDataLength  : (int)sizeof( g_freq_data );
    memcpy( g_freq_data[0], pFreqData, copysize );
    memcpy( g_freq_data[1], pFreqData, copysize );
  }
  g_new_audio = true;
}


//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  static ITunesPixelFormat format;
  static float yflip = 0.0;
  if ( ! g_tex_id )
  {
    // initialize the texture we'll be using
    glGenTextures( 1, &g_tex_id );
    if ( ! g_tex_id )
      return;

    /* render the vislualization */
    format = ivis_render( g_plugin, g_audio_data, g_freq_data, (void*)g_tex_buffer,
                          g_tex_buffer_size, !g_new_audio );
    glActiveTextureARB( GL_TEXTURE3_ARB );
    glDisable( GL_TEXTURE_2D );
    glActiveTextureARB( GL_TEXTURE2_ARB );
    glDisable( GL_TEXTURE_2D );
    glActiveTextureARB( GL_TEXTURE1_ARB );
    glDisable( GL_TEXTURE_2D );

    /* update texture */
    glActiveTextureARB( GL_TEXTURE0_ARB );
    glEnable(GL_TEXTURE_2D);
    glBindTexture( GL_TEXTURE_2D, g_tex_id );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if ( format == ITunesPixelFormatRGBA32 )
    {
      glTexImage2D( GL_TEXTURE_2D, 0, 4, g_tex_width, g_tex_height, 0,
                    GL_BGRA, GL_UNSIGNED_BYTE, g_tex_buffer );
    }
    else if ( format == ITunesPixelFormatRGB24 )
    {
      glTexImage2D( GL_TEXTURE_2D, 0, 4, g_tex_width, g_tex_height, 0,
                    GL_RGB, GL_UNSIGNED_BYTE, g_tex_buffer );
      yflip = 1.0;
    }
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  }
  else
  {
    /* render the vislualization */
    format = ivis_render( g_plugin, g_audio_data, g_freq_data, (void*)g_tex_buffer,
                          g_tex_buffer_size, !g_new_audio );
    glActiveTextureARB( GL_TEXTURE3_ARB );
    glDisable( GL_TEXTURE_2D );
    glActiveTextureARB( GL_TEXTURE2_ARB );
    glDisable( GL_TEXTURE_2D );
    glActiveTextureARB( GL_TEXTURE1_ARB );
    glDisable( GL_TEXTURE_2D );

    /* update texture */
    glActiveTextureARB( GL_TEXTURE0_ARB );
    glEnable(GL_TEXTURE_2D);
    glBindTexture( GL_TEXTURE_2D, g_tex_id );
    if ( format == ITunesPixelFormatRGBA32 )
      glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, g_tex_width, g_tex_height,
                       GL_BGRA, GL_UNSIGNED_BYTE, g_tex_buffer );
    else if ( format == ITunesPixelFormatRGB24 )
      glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, g_tex_width, g_tex_height,
                       GL_RGB, GL_UNSIGNED_BYTE, g_tex_buffer );
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  }
  g_new_audio = false;

  /* draw a quad with the updated texture */
  glDisable(GL_BLEND);
  glBegin( GL_QUADS );
  {
    glColor3f( 1.0, 1.0, 1.0 );
    glTexCoord2f( 0.0, 1.0 - yflip );
    glVertex2f( g_window_xpos, g_window_ypos );

    glTexCoord2f( 0.0, yflip - 0.0 );
    glVertex2f( g_window_xpos, g_window_ypos + g_window_height );

    glTexCoord2f( 1.0, yflip - 0.0 );
    glVertex2f( g_window_xpos + g_window_width, g_window_ypos + g_window_height );

    glTexCoord2f( 1.0, 1.0 - yflip );
    glVertex2f( g_window_xpos + g_window_width, g_window_ypos );
  }
  glEnd();
  glDisable( GL_TEXTURE_2D );
  glEnable(GL_BLEND);
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

  switch( flags )
  {
  case VIS_ACTION_UPDATE_ALBUMART:
    if ( param )
      ivis_set_album_art( g_plugin, (const char*)param );
    break;

  case VIS_ACTION_UPDATE_TRACK:
    if ( param )
    {
      VisTrack* info_param = (VisTrack*)param;
      ITunesTrack track_info;
      memset( &track_info, 0, sizeof(track_info) );

      track_info.title        = info_param->title;
      track_info.artist       = info_param->artist;
      track_info.album        = info_param->album;
      track_info.album_artist = info_param->albumArtist;
      track_info.track_number = info_param->trackNumber;
      track_info.disc_number  = info_param->discNumber;
      track_info.duration     = info_param->duration;
      track_info.year         = info_param->year;
      track_info.genre        = info_param->genre;
      track_info.rating       = info_param->rating * 20;

      ivis_set_track_info( g_plugin, &track_info );
    }
  }
  return ret;
}

//-- GetPresets ---------------------------------------------------------------
// Return a list of presets to XBMC for display
//-----------------------------------------------------------------------------
extern "C" void GetPresets(char ***pPresets, int *currentPreset, int *numPresets,
                           bool *locked)
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

extern "C" int GetSubModules(char ***names, char ***paths)
{
  return ivis_get_visualisations( names, paths );
}
