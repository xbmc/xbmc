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

#include <dlfcn.h>                  /* for dlopen, dlclose */
#include <string.h>
#include <QuickTime/QuickTime.h>
#include <Accelerate/Accelerate.h>  /* for doing FFT */
#include <sys/time.h>
#include <AGL/agl.h>

#include "common.h"
#include "itunes_vis.h"
#include "qview.h"

/***********************************************************************/
/* ivis_open                                                           */
/* open a file as an iTunes visualizer and return a handle             */
/***********************************************************************/
int ivis_get_visualisations( char ***names, char ***paths )
{
  /* call the platform specific implementation */
  return _get_visualisations( names, paths );
}

/***********************************************************************/
/* ivis_open                                                           */
/* open a file as an iTunes visualizer and return a handle             */
/***********************************************************************/
ITunesVis* ivis_open( const char *name )
{
  ITunesVis* vis = (ITunesVis*) malloc( sizeof(ITunesVis) );

  if ( vis != NULL )
  {
    /* initialize to 0 */
    memset( vis, 0, sizeof(ITunesVis) );
    vis->track_info.recordLength = sizeof(ITTrackInfo);

    /* get path to visualiser from name */
    char *plugin_path = _get_visualisation_path( name );

    if ( plugin_path )
    {
      /* check if it's a composition or regular visualiser */
      if ( strstr( plugin_path, ".qtz" ) != NULL )
      {
        vis->vis_type = ITunesVisTypeComposition;
        strncpy( (char*)vis->filename, (const char*)plugin_path, sizeof(vis->filename) );
      }
      else
      {
        vis->vis_type = ITunesVisTypeNormal;
        char *exe_path = _get_executable_path( plugin_path );

        if ( exe_path )
        {
          vis->handle = dlopen( exe_path, RTLD_NOW );

          if ( vis->handle == NULL )
          {
            free( vis );
            vis = NULL;
          }
          else
          {
            strncpy( (char*)vis->bundle_path, (const char*)plugin_path, sizeof(vis->bundle_path) );
            strncpy( (char*)vis->filename, (const char*)exe_path, sizeof(vis->filename) );
          }
          free( exe_path );
        }
      }
      free( plugin_path );
    }
    else
    {
      free( vis );
      vis = NULL;
    }
  }
  return vis;
}

/***********************************************************************/
/* ivis_init                                                           */
/* initialize the iTunes visulizer with the specified widht & height   */
/***********************************************************************/
bool ivis_init( ITunesVis* plugin, int width, int height )
{
  if ( plugin == NULL )
    return false;

  /* load imports */
  plugin->imports.main = (iTunesPluginMainMachOPtr) dlsym( plugin->handle, "iTunesPluginMainMachO" );
  if ( ! plugin->imports.main )
    return false;

  /* configure kPluginInitMessage message */
  memset( &plugin->message.u.initMessage, 0, sizeof(plugin->message.u.initMessage) );
  plugin->message.u.initMessage.majorVersion = 7;
  plugin->message.u.initMessage.minorVersion = 4;
  plugin->message.u.initMessage.appCookie    = (void*)plugin;
  plugin->message.u.initMessage.appProc      = XBMCITAppProc;

  /* send the visualizer the kPluginInitMessage */
  plugin->imports.main( kPluginInitMessage, &( plugin->message ), NULL );

  /* keep track of ref */
  plugin->main_ref = plugin->message.u.initMessage.refCon;

  /* ensure we got a visual handler */
  if ( ! plugin->imports.visual_handler )
    return false;

  /* configure kVisualPluginInitMessage message */
  memset( &plugin->visual_message.u.initMessage, 0, sizeof(plugin->visual_message.u.initMessage) );
  plugin->visual_message.u.initMessage.messageMajorVersion = kITPluginMajorMessageVersion;
  plugin->visual_message.u.initMessage.messageMinorVersion = kITPluginMinorMessageVersion;
  plugin->visual_message.u.initMessage.appCookie           = (void*)plugin;
  plugin->visual_message.u.initMessage.appProc             = XBMCITAppProc;
  plugin->visual_message.u.initMessage.appVersion.majorRev       = 7;
  plugin->visual_message.u.initMessage.appVersion.minorAndBugRev = 4;
  plugin->visual_message.u.initMessage.appVersion.stage          = finalStage;
  plugin->visual_message.u.initMessage.appVersion.nonRelRev      = 0;

  /* send the plugin the kVisualPluginInitMessage message */
  plugin->imports.visual_handler( kVisualPluginInitMessage,
                                  &( plugin->visual_message ),
                                  plugin->vis_ref );

  /* update our ref pointer */
  if ( plugin->visual_message.u.initMessage.refCon )
    plugin->vis_ref = plugin->visual_message.u.initMessage.refCon;

  /* set the render rect */
  SetRect( &(plugin->rect), 0, 0, width, height );

  /* create FFT object (use what iTunes uses for doing FFT) */
  plugin->fft_setup = create_fftsetup( 9, FFT_RADIX2 );

  return true;
}

/***********************************************************************/
/* ivis_start                                                          */
/* TODO                                                                */
/***********************************************************************/
void ivis_start( ITunesVis* plugin )
{
  /* make sure we have a plugin and a visual handler */
  if ( !plugin || !plugin->imports.visual_handler )
    return;

  /* if we don't have an offscreen buffer, create one */
  if ( ! plugin->screen )
    plugin->screen = get_view( plugin->rect.right, plugin->rect.bottom );

  /* customize the show window message */
  memset( &plugin->visual_message.u.showWindowMessage, 0,
          sizeof(plugin->visual_message.u.showWindowMessage) );
  plugin->visual_message.u.showWindowMessage.GRAPHICS_DEVICE_NAME = plugin->screen;
  plugin->visual_message.u.showWindowMessage.drawRect             = plugin->rect;
  plugin->visual_message.u.showWindowMessage.options              = 0;
  plugin->visual_message.u.showWindowMessage.totalVisualizerRect  = plugin->rect;

  /* set our start time */
  plugin->start_time = ivis_current_time();

  /* send the show window message */
  CGLContextObj currentContext = CGLGetCurrentContext();
  plugin->imports.visual_handler( kVisualPluginEnableMessage, &( plugin->visual_message ),
                                  plugin->vis_ref );
  plugin->imports.visual_handler( kVisualPluginShowWindowMessage, &( plugin->visual_message ),
                                  plugin->vis_ref );
  plugin->imports.visual_handler( kVisualPluginUpdateMessage, &( plugin->visual_message ),
                                  plugin->vis_ref );
  plugin->gl_context = (void*)aglGetCurrentContext();
  CGLSetCurrentContext( currentContext );
}

unsigned long ivis_current_time()
{
  struct timeval current_time;
  gettimeofday( &current_time, NULL );
  return ( current_time.tv_sec * 1000 + current_time.tv_usec / 1000 );
}

/***********************************************************************/
/* ivis_render                                                         */
/* process audio data and update frame                                 */
/***********************************************************************/
ITunesPixelFormat ivis_render( ITunesVis* plugin, short audio_data[][512], float freq_data[][512],
                               void* buffer, long buffer_size, bool idle )
{
  ITunesPixelFormat format = ITunesPixelFormatUnknown;

  /* make sure we have a plugin and a visual handler */
  if ( !plugin || !plugin->imports.visual_handler )
    return format;

  int i=0, w=0;
  RenderVisualData visual_data;
  DSPSplitComplex splitComplex[2];
  float *data[2];

  /* perform FFT if we're not idling */
  if ( ! idle )
  {
    /* allocate some complex vars */
    for ( i = 0 ; i < 2 ; i++ )
    {
      splitComplex[i].realp = calloc( 512, sizeof(float) );
      splitComplex[i].imagp = calloc( 512, sizeof(float) );
      data[i] = calloc( 512, sizeof(float) );
    }

    /* 2 channels for spectrum and waveform data */
    visual_data.numWaveformChannels = 2;
    visual_data.numSpectrumChannels = 2;

    /* copy spectrum audio data to visual data strucure */
    for ( w = 0 ; w < 512 ; w++ )
    {
      /* iTunes visualizers expect waveform data from 0 - 255, with level 0 at 128 */
      visual_data.waveformData[0][w] = (UInt8)( (long)(audio_data[0][w]) / 128 + 128 );
      visual_data.waveformData[1][w] = (UInt8)( (long)(audio_data[1][w]) / 128 + 128 );

      /* scale to -1, +1 */
      *( data[0] + w ) = (float)(( audio_data[0][w]) / (2.0 * 8192.0) );
      *( data[1] + w ) = (float)(( audio_data[1][w]) / (2.0 * 8192.0) );
    }

    /* FFT scaler */
    float scale = ( 1.0 / 1024.0 ) ; /* scale by length of input * 2 (due to how vDSP does FFTs) */
    float nyq=0, dc=0, freq=0;

    for ( i = 0 ; i < 2 ; i++ )
    {
      /* pack data into format fft_zrip expects it */
      vDSP_ctoz( (COMPLEX*)( data[i] ), 2, &( splitComplex[i] ), 1, 256 );

      /* perform FFT on normalized audio data */
      fft_zrip( plugin->fft_setup, &( splitComplex[i] ), 1, 9, FFT_FORWARD );

      /* scale the values */
      vDSP_vsmul( splitComplex[i].realp, 1, &scale, splitComplex[i].realp, 1, 256 );
      vDSP_vsmul( splitComplex[i].imagp, 1, &scale, splitComplex[i].imagp, 1, 256 );

      /* unpack data */
      vDSP_ztoc( &splitComplex[i], 1, (COMPLEX*)( data[i] ), 2, 256 );

      /* ignore phase */
      dc = *(data[i]) = fabs( *(data[i]) );
      nyq = fabs( *(data[i] + 1) );

      for ( w = 1 ; w < 256 ; w++ )
      {
        /* don't use vDSP for this since there's some overflow */
        freq = hypot( *(data[i] + w * 2), *(data[i] + w * 2 + 1) ) * 256 * 16;
        freq = MAX( 0, freq );
        freq = MIN( 255, freq );
        visual_data.spectrumData[i][ w - 1 ] = (UInt8)( freq );
      }
      visual_data.spectrumData[i][256] = nyq;
    }

    /* deallocate complex vars */
    for ( i = 0 ; i < 2 ; i++ )
    {
      free( splitComplex[i].realp );
      free( splitComplex[i].imagp );
      free( data[i] );
    }

    /* update the render message with the new visual data and timestamp */
    plugin->visual_message.u.renderMessage.renderData = &visual_data;
    plugin->visual_message.u.renderMessage.timeStampID++;
  }

  /* update time */
  plugin->visual_message.u.renderMessage.currentPositionInMS =
    ivis_current_time() - plugin->start_time; // FIXME: real time

  /* save our GL context and send the vis a render message */
  CGLContextObj currentContext = CGLGetCurrentContext();
  if ( plugin->gl_context )
    aglSetCurrentContext( (AGLContext)(plugin->gl_context ) );

  /* call the plugin's render method */
  if ( idle )
  {
    /* idle message */
    if ( plugin->wants_idle )
      plugin->imports.visual_handler( kVisualPluginIdleMessage,
                                      &( plugin->visual_message ),
                                      plugin->vis_ref );
  }
  else
  {
    /* render message */
    plugin->imports.visual_handler( kVisualPluginRenderMessage,
                                    &( plugin->visual_message ),
                                    plugin->vis_ref );

    /* set position message */
    plugin->visual_message.u.setPositionMessage.positionTimeInMS
      = plugin->visual_message.u.renderMessage.currentPositionInMS;
    plugin->imports.visual_handler( kVisualPluginSetPositionMessage, &( plugin->visual_message ),
                                    plugin->vis_ref );
  }
  /* update message */
  plugin->imports.visual_handler( kVisualPluginUpdateMessage, NULL,
                                  plugin->vis_ref );

  /* read pixels and restore our GL context */
  CGLLockContext( CGLGetCurrentContext() );

  switch ( get_pixels( buffer, buffer_size, CGLGetCurrentContext() != currentContext ) )
  {
  case 3:
    format = ITunesPixelFormatRGB24;
    break;

  case 4:
    format = ITunesPixelFormatRGBA32;
    break;

  default:
    break;
  }

  CGLUnlockContext ( CGLGetCurrentContext() );

  /* restore our GL context */
  CGLSetCurrentContext( currentContext );
  return format;
}

/***********************************************************************/
/* ivis_set_track_info                                                 */
/* sets track metadata for the currently playing track                 */
/***********************************************************************/
void ivis_set_track_info( ITunesVis* plugin, ITunesTrack* track_info )
{
  if ( !plugin || !track_info )
    return;

  ITTrackInfo    *info      = &( plugin->track_info );
  ITTrackInfoV1  *info_v1   = &( plugin->track_info_v1 );
  ITStreamInfo   *stream    = &( plugin->stream_info );
  ITStreamInfoV1 *stream_v1 = &( plugin->stream_info_v1 );

  /* reset valid fields */
  info->validFields    = 0;
  info_v1->validFields = 0;
  memset( info,    0, sizeof(ITTrackInfo)   );
  memset( info_v1, 0, sizeof(ITTrackInfoV1) );

  if ( is_valid_field( track_info->title ) )
  {
    _copy_to_pascal_string( info_v1->name, track_info->title, sizeof(info_v1->name) );
    _copy_to_unicode_string( info->name, track_info->title, sizeof(info->name) );
    info_v1->validFields = info->validFields |= kITTINameFieldMask;
  }

  if ( is_valid_field( track_info->artist ) )
  {
    _copy_to_pascal_string( info_v1->artist, track_info->artist, sizeof(info_v1->artist) );
    _copy_to_unicode_string( info->artist, track_info->artist, sizeof(info->artist) );
    info_v1->validFields = info->validFields |= kITTIArtistFieldMask;
  }

  if ( is_valid_field( track_info->album ) )
  {
    _copy_to_pascal_string( info_v1->album, track_info->album, sizeof(info_v1->album) );
    _copy_to_unicode_string( info->album, track_info->album, sizeof(info->album) );
    info_v1->validFields = info->validFields |= kITTIAlbumFieldMask;
  }

  if ( is_valid_field( track_info->genre ) )
  {
    _copy_to_pascal_string( info_v1->genre, track_info->genre, sizeof(info_v1->genre) );
    _copy_to_unicode_string( info->genre, track_info->genre, sizeof(info->genre) );
    info_v1->validFields = info->validFields |= kITTIGenreFieldMask;
  }

  /* iTunes sends a 'stop and play' instead of a 'change track' */
  if ( 1 /*! plugin->playing*/ )
  {
    /* send a play message */
    plugin->playing = true;

    memset( &(plugin->visual_message.u.playMessage), 0,
            sizeof(plugin->visual_message.u.playMessage) );

    plugin->visual_message.u.playMessage.trackInfo         = info_v1;
    plugin->visual_message.u.playMessage.trackInfoUnicode  = info;
    plugin->visual_message.u.playMessage.streamInfo        = stream_v1;
    plugin->visual_message.u.playMessage.streamInfoUnicode = stream;
    plugin->visual_message.u.playMessage.volume            = 100;
    plugin->imports.visual_handler( kVisualPluginStopMessage,
                                    &( plugin->visual_message ), plugin->vis_ref );
    plugin->imports.visual_handler( kVisualPluginPlayMessage,
                                    &( plugin->visual_message ), plugin->vis_ref );
  }
  else
  {
    /* send a change track message */
    memset( &(plugin->visual_message.u.changeTrackMessage), 0,
            sizeof(plugin->visual_message.u.changeTrackMessage) );

    plugin->visual_message.u.changeTrackMessage.trackInfo        = info_v1;
    plugin->visual_message.u.changeTrackMessage.trackInfoUnicode = info;
    plugin->visual_message.u.changeTrackMessage.streamInfo        = stream_v1;
    plugin->visual_message.u.changeTrackMessage.streamInfoUnicode = stream;
    plugin->imports.visual_handler( kVisualPluginChangeTrackMessage,
                                    &( plugin->visual_message ), plugin->vis_ref );
  }
}

/***********************************************************************/
/* ivis_set_album_art                                                  */
/* sets album art for the currently playing track                      */
/***********************************************************************/
void ivis_set_album_art( ITunesVis* plugin, const char* filename )
{
  if ( !plugin || !filename)
    return;
  strncpy( plugin->album_art, filename, sizeof( plugin->album_art ) );
}

/***********************************************************************/
/* ivis_wants_idle                                                     */
/* returns true if the plugin wants idle messages                      */
/***********************************************************************/
bool ivis_wants_idle( ITunesVis* plugin )
{
  return plugin->wants_idle;
}

/***********************************************************************/
/* ivis_close                                                          */
/* close an iTunes visualizer                                          */
/***********************************************************************/
void ivis_close( ITunesVis* plugin )
{
  if ( plugin != NULL )
  {
    /* destroy FFT setup */
    destroy_fftsetup( plugin->fft_setup );

    /* send the plugin, visual hide and cleanup messages */
    if ( plugin->imports.visual_handler )
    {
      plugin->imports.visual_handler( kVisualPluginStopMessage, NULL, plugin->vis_ref );
      plugin->imports.visual_handler( kVisualPluginDisableMessage, NULL, plugin->vis_ref );
      plugin->imports.visual_handler( kVisualPluginHideWindowMessage, NULL, plugin->vis_ref );
      plugin->imports.visual_handler( kVisualPluginCleanupMessage,    NULL, plugin->vis_ref );
    }

    /* send the plugin a cleanup message */
    // FIXME: iTunes plugins don't expect to be unloaded until app terminates
    //if ( plugin->imports.main )
    //  plugin->imports.main( kPluginCleanupMessage, NULL, plugin->main_ref );

    /* free our screen if it was allocated */
    if ( plugin->screen )
      release_view( plugin->screen );

    /* if we have an open handle, then close it */
    if ( plugin->handle )
    {
      // FIXME: iTunes plugins don't expect to be unloaded until app terminates
      //dlclose( plugin->handle );
      plugin->handle = NULL;
    }

    /* deallocate */
    free( plugin );
  }
}

/***********************************************************************/
/***********************************************************************/
static OSStatus XBMCITAppProc(void *appCookie, OSType message, struct PlayerMessageInfo *messageInfo)
{
  /* cast app cookie into ITunesVis object */
  ITunesVis* plugin = (ITunesVis*) appCookie;
  FSRef ref;

  if ( plugin == NULL )
    return noErr;

  /* initial registration response */
  switch (message)
  {
  case kPlayerRegisterVisualPluginMessage:
    printf( "REGISTER: visual plugin\n" );
    /* copy the supplied ref from the plugin */
    plugin->vis_ref
      = messageInfo->u.registerVisualPluginMessage.registerRefCon;

    /* keep track of the visual handler so we can send it messages */
    plugin->imports.visual_handler
      = messageInfo->u.registerVisualPluginMessage.handler;

    /* check if plugin wants idle */
    if ( messageInfo->u.registerVisualPluginMessage.options & kVisualWantsIdleMessages )
    {
      printf( "REGISTER: plugin wants idle message\n" );
      plugin->wants_idle = true;
    }
    break;


  case kPlayerGetPluginFileSpecMessage:
    printf( "GET: plugin file spec\n" );
    /* get the plugin's FSSpec */
    memset( &ref, 0, sizeof(FSRef) );
    FSPathMakeRef( (UInt8 *)(plugin->bundle_path), &ref, NULL );
    FSGetCatalogInfo( &ref, kFSCatInfoNone, NULL, NULL,
                      messageInfo->u.getPluginFileSpecMessage.fileSpec,
                      NULL );
    break;

  case kPlayerSetPluginDataMessage:            /* Set plugin preferences */
    printf( "SET: plugin data\n" );
    break;

  case kPlayerGetPluginDataMessage:            /* Get plugin preferences */
    printf( "GET: plugin data message\n" );
    messageInfo->u.getPluginDataMessage.dataSize = 0;
    break;

  case kPlayerGetPluginNamedDataMessage:       /* Get plugin preferences */
    printf( "GET: plugin named data message\n" );
    messageInfo->u.getPluginNamedDataMessage.dataSize = 0;
    break;

  case kPlayerGetCurrentTrackCoverArtMessage:
    printf( "GET: current track cover art message\n" );
    messageInfo->u.getCurrentTrackCoverArtMessage.coverArt       = 0;
    messageInfo->u.getCurrentTrackCoverArtMessage.coverArtFormat = 0;
    if ( strlen(plugin->album_art) > 0 )
    {
      printf( "GET: %s\n", plugin->album_art );
      _get_album_art_from_file( (const char *)(plugin->album_art),
                                &( messageInfo->u.getCurrentTrackCoverArtMessage.coverArt ),
                                &( messageInfo->u.getCurrentTrackCoverArtMessage.coverArtFormat ) );
    }
    break;

  case kPlayerGetFileTrackInfoMessage:	       /* Query iTunes for information about a file  */
    printf( "GET: file track info\n" );
    break;

  case kPlayerSetFileTrackInfoMessage:         /* Ask iTunes to set information about a file */
    printf( "SET: file track info\n" );
    break;

  case kPlayerGetPluginITFileSpecMessage:      /* Get the location of the plugin executable (iTunes 4.1 or later) */
    printf( "GET: plugin it file spec\n" );
    break;

  case kPlayerGetITTrackInfoSizeMessage:       /* Query iTunes for the sizeof(ITTrackInfo).  */
                                               /* allows newer plugins to work with older versions of iTunes. */
    printf( "GET: IT track info size\n" );
    break;


    /* These messages should probably return error free */
  case kPlayerSetFullScreenMessage:
    break;

  case kPlayerSetFullScreenOptionsMessage:
    break;

  default:
    printf( "unknown message received\n" );
    break;
  }

  return noErr;
}
