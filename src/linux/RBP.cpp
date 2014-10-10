/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "RBP.h"
#if defined(TARGET_RASPBERRY_PI)

#include "settings/Settings.h"
#include "utils/log.h"

#include "cores/omxplayer/OMXImage.h"

CRBP::CRBP()
{
  m_initialized     = false;
  m_omx_initialized = false;
  m_DllBcmHost      = new DllBcmHost();
  m_OMX             = new COMXCore();
  m_element = 0;
}

CRBP::~CRBP()
{
  Deinitialize();
  delete m_OMX;
  delete m_DllBcmHost;
}

bool CRBP::Initialize()
{
  CSingleLock lock (m_critSection);
  if (m_initialized)
    return true;

  m_initialized = m_DllBcmHost->Load();
  if(!m_initialized)
    return false;

  m_DllBcmHost->bcm_host_init();

  uint32_t vc_image_ptr;
  m_resource = vc_dispmanx_resource_create( VC_IMAGE_RGB565, 1, 1, &vc_image_ptr );

  m_omx_initialized = m_OMX->Initialize();
  if(!m_omx_initialized)
    return false;

  char response[80] = "";
  m_arm_mem = 0;
  m_gpu_mem = 0;
  m_codec_mpg2_enabled = false;
  m_codec_wvc1_enabled = false;

  if (vc_gencmd(response, sizeof response, "get_mem arm") == 0)
    vc_gencmd_number_property(response, "arm", &m_arm_mem);
  if (vc_gencmd(response, sizeof response, "get_mem gpu") == 0)
    vc_gencmd_number_property(response, "gpu", &m_gpu_mem);

  if (vc_gencmd(response, sizeof response, "codec_enabled MPG2") == 0)
    m_codec_mpg2_enabled = strcmp("MPG2=enabled", response) == 0;
  if (vc_gencmd(response, sizeof response, "codec_enabled WVC1") == 0)
    m_codec_wvc1_enabled = strcmp("WVC1=enabled", response) == 0;

  if (m_gpu_mem < 128)
    setenv("V3D_DOUBLE_BUFFER", "1", 1);

  m_gui_resolution_limit = CSettings::Get().GetInt("videoscreen.limitgui");
  if (!m_gui_resolution_limit)
    m_gui_resolution_limit = m_gpu_mem < 128 ? 720:1080;

  g_OMXImage.Initialize();
  m_omx_image_init = true;
  return true;
}

void CRBP::LogFirmwareVerison()
{
  char  response[1024];
  m_DllBcmHost->vc_gencmd(response, sizeof response, "version");
  response[sizeof(response) - 1] = '\0';
  CLog::Log(LOGNOTICE, "Raspberry PI firmware version: %s", response);
  CLog::Log(LOGNOTICE, "ARM mem: %dMB GPU mem: %dMB MPG2:%d WVC1:%d", m_arm_mem, m_gpu_mem, m_codec_mpg2_enabled, m_codec_wvc1_enabled);
  m_DllBcmHost->vc_gencmd(response, sizeof response, "get_config int");
  response[sizeof(response) - 1] = '\0';
  CLog::Log(LOGNOTICE, "Config:\n%s", response);
  m_DllBcmHost->vc_gencmd(response, sizeof response, "get_config str");
  response[sizeof(response) - 1] = '\0';
  CLog::Log(LOGNOTICE, "Config:\n%s", response);
}

void CRBP::GetDisplaySize(int &width, int &height)
{
  DISPMANX_DISPLAY_HANDLE_T display;
  DISPMANX_MODEINFO_T info;

  display = vc_dispmanx_display_open( 0 /*screen*/ );
  if (vc_dispmanx_display_get_info(display, &info) == 0)
  {
    width = info.width;
    height = info.height;
  }
  else
  {
    width = 0;
    height = 0;
  }
  vc_dispmanx_display_close(display );
}

unsigned char *CRBP::CaptureDisplay(int width, int height, int *pstride, bool swap_red_blue, bool video_only)
{
  DISPMANX_DISPLAY_HANDLE_T display;
  DISPMANX_RESOURCE_HANDLE_T resource;
  VC_RECT_T rect;
  unsigned char *image = NULL;
  uint32_t vc_image_ptr;
  int stride;
  uint32_t flags = 0;

  if (video_only)
    flags |= DISPMANX_SNAPSHOT_NO_RGB|DISPMANX_SNAPSHOT_FILL;
  if (swap_red_blue)
    flags |= DISPMANX_SNAPSHOT_SWAP_RED_BLUE;
  if (!pstride)
    flags |= DISPMANX_SNAPSHOT_PACK;

  display = vc_dispmanx_display_open( 0 /*screen*/ );
  stride = ((width + 15) & ~15) * 4;
  image = new unsigned char [height * stride];

  if (image)
  {
    resource = vc_dispmanx_resource_create( VC_IMAGE_RGBA32, width, height, &vc_image_ptr );

    vc_dispmanx_snapshot(display, resource, (DISPMANX_TRANSFORM_T)flags);

    vc_dispmanx_rect_set(&rect, 0, 0, width, height);
    vc_dispmanx_resource_read_data(resource, &rect, image, stride);
    vc_dispmanx_resource_delete( resource );
    vc_dispmanx_display_close(display );
  }
  if (pstride)
    *pstride = stride;
  return image;
}

void CRBP::WaitVsync(void)
{
  DISPMANX_DISPLAY_HANDLE_T display = vc_dispmanx_display_open( 0 /*screen*/ );
  DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);

  VC_DISPMANX_ALPHA_T alpha = { (DISPMANX_FLAGS_ALPHA_T)(DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS), 120, /*alpha 0->255*/ 0 };
  VC_RECT_T       src_rect;
  VC_RECT_T       dst_rect;
  vc_dispmanx_rect_set( &src_rect, 0, 0, 1 << 16, 1 << 16 );
  vc_dispmanx_rect_set( &dst_rect, 0, 0, 1, 1 );

  if (m_element)
    vc_dispmanx_element_remove( update, m_element );

  m_element = vc_dispmanx_element_add( update,
                                            display,
                                            2000,               // layer
                                            &dst_rect,
                                            m_resource,
                                            &src_rect,
                                            DISPMANX_PROTECTION_NONE,
                                            &alpha,
                                            NULL,             // clamp
                                            (DISPMANX_TRANSFORM_T)0 );

  vc_dispmanx_update_submit_sync(update);
  vc_dispmanx_display_close( display );
}


void CRBP::Deinitialize()
{
  if (m_omx_image_init)
    g_OMXImage.Deinitialize();

  if(m_omx_initialized)
    m_OMX->Deinitialize();

  m_DllBcmHost->bcm_host_deinit();

  if(m_initialized)
    m_DllBcmHost->Unload();

  m_omx_image_init  = false;
  m_initialized     = false;
  m_omx_initialized = false;
}
#endif
