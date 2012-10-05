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
  iTunes Visualization Wrapper for XBMC
*/


#include <GL/glew.h>
#include <string>

#include "itunes_vis.h"
#include "../../addons/include/xbmc_vis_dll.h"

int g_tex_width     = 512;
int g_tex_height    = 512;
int g_window_width  = 512;
int g_window_height = 512;
int g_window_xpos   = 0;
int g_window_ypos   = 0;

short       g_audio_data[2][512];
float       g_freq_data[2][512];
bool        g_new_audio;
std::string g_sub_module;
std::string g_vis_name;
GLuint      g_tex_id          = 0;
GLbyte*     g_tex_buffer      = NULL;
long        g_tex_buffer_size = 0;
ITunesVis*  g_plugin          = NULL;

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* visProps)
{
  if (!visProps)
    return ADDON_STATUS_UNKNOWN;

  VIS_PROPS* props = (VIS_PROPS*) visProps;

  if (!props->submodule)
    return ADDON_STATUS_UNKNOWN;
  
  g_vis_name = props->name;
  g_sub_module = props->submodule;

  /* copy window dimensions */
  g_window_width  = g_tex_width  = props->width;
  g_window_height = g_tex_height = props->height;
  g_window_xpos   = props->x;
  g_window_ypos   = props->y;

  /* create texture buffer */
  g_tex_buffer_size = g_tex_width * g_tex_height * 4;
  g_tex_buffer      = (GLbyte*)malloc( g_tex_buffer_size );

  if ( !g_tex_buffer )
    return ADDON_STATUS_UNKNOWN;

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
    return ADDON_STATUS_UNKNOWN;
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

  return ADDON_STATUS_OK;
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
extern "C" void ADDON_Stop()
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
extern "C" void AudioData(const short* pAudioData, int iAudioDataLength,
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
extern "C" bool OnAction(long flags, const void *param)
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
extern "C" unsigned int GetPresets(char ***pPresets)
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
// Return a list of names and paths for submodules
//-----------------------------------------------------------------------------
extern "C" unsigned int GetSubModules(char ***modules)
{
  char **path;
  unsigned int num_plugins;
  
  num_plugins = ivis_get_visualisations(modules, &path);
  free(path);
  return num_plugins;
}

//-- Destroy-------------------------------------------------------------------
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

