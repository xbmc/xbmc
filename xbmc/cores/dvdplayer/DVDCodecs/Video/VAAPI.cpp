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
#include "system.h"
#include "WindowingFactory.h"
#include "Settings.h"
#include "VAAPI.h"
#include <boost/scoped_array.hpp>

using namespace boost;
using namespace VAAPI;

static void RelBufferS(AVCodecContext *avctx, AVFrame *pic)
{ ((CDecoder*)((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetHardware())->RelBuffer(avctx, pic); }

static int GetBufferS(AVCodecContext *avctx, AVFrame *pic) 
{  return ((CDecoder*)((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetHardware())->GetBuffer(avctx, pic); }

CDecoder::CDecoder()
{
  m_display  = NULL;
  m_config   = NULL;
  m_context  = NULL;
  m_hwaccel  = (vaapi_context*)calloc(1, sizeof(vaapi_context));
}

CDecoder::~CDecoder()
{
  free(m_hwaccel);
}

void CDecoder::RelBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  VASurfaceID surface = (VASurfaceID)pic->data[3];
  
  for(std::list<VASurfaceID>::iterator it = m_surfaces_used.begin(); it != m_surfaces_used.end(); it++)
  {    
    if(*it == surface)
    {
      m_surfaces_free.push_back(surface);
      m_surfaces_used.erase(it);
      break;
    }
  }
  pic->data[0] = NULL;
  pic->data[1] = NULL;
  pic->data[2] = NULL;
  pic->data[3] = NULL;
}

int CDecoder::GetBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  
  if(m_surfaces_free.empty())
    return -1;

  VASurfaceID surface = m_surfaces_free.front();
  m_surfaces_free.pop_front();
  m_surfaces_used.push_back(surface);
  
  pic->type           = FF_BUFFER_TYPE_USER;
  pic->age            = 1;
  pic->data[0]        = (uint8_t*)surface;
  pic->data[1]        = NULL;
  pic->data[2]        = NULL;
  pic->data[3]        = (uint8_t*)surface;
  pic->linesize[0]    = 0;
  pic->linesize[1]    = 0;
  pic->linesize[2]    = 0;
  pic->linesize[3]    = 0;
  return 0;
}

void CDecoder::Close()
{
}

#define CHECK(a) \
do { \
  VAStatus res = a; \
  if(res != VA_STATUS_SUCCESS) \
  { \
    CLog::Log(LOGERROR, "VAAPI - failed executing "#a" at line %d with error %x", __LINE__, res); \
    return false; \
  } \
} while(0);

bool CDecoder::Open(AVCodecContext *avctx, enum PixelFormat fmt)
{
  VAEntrypoint entrypoint = VAEntrypointVLD;
  VAProfile    profile;

  switch (avctx->codec_id) {
  case CODEC_ID_MPEG2VIDEO:
      profile = VAProfileMPEG2Main;           break;
  case CODEC_ID_MPEG4:
  case CODEC_ID_H263:
      profile = VAProfileMPEG4AdvancedSimple; break;
  case CODEC_ID_H264:
      profile = VAProfileH264High;            break;
  case CODEC_ID_WMV3:
      profile = VAProfileVC1Main;             break;
  case CODEC_ID_VC1:
      profile = VAProfileVC1Advanced;         break;
  default:
      return false;
  }
  
  if(avctx->codec_id == CODEC_ID_H264)
    m_surfaces_count = 16 + 1 + 1;
  else
    m_surfaces_count = 2  + 1 + 1;

  
  m_display = vaGetDisplayGLX(g_Windowing.GetDisplay());

  int major_version, minor_version;
  CHECK(vaInitialize(m_display, &major_version, &minor_version))

  int num_display_attrs = 0;
  scoped_array<VADisplayAttribute> display_attrs(new VADisplayAttribute[vaMaxNumDisplayAttributes(m_display)]);

  CHECK(vaQueryDisplayAttributes(m_display, display_attrs.get(), &num_display_attrs))

  for(int i = 0; i < num_display_attrs; i++)
  {
      VADisplayAttribute * const display_attr = &display_attrs[i];
      CLog::Log(LOGDEBUG, "VAAPI - attrib %d (%s/%s) min %d max %d value 0x%x\n"
              , display_attr->type
              ,(display_attr->flags & VA_DISPLAY_ATTRIB_GETTABLE) ? "get" : "---"
              ,(display_attr->flags & VA_DISPLAY_ATTRIB_SETTABLE) ? "set" : "---"
              , display_attr->min_value
              , display_attr->max_value
              , display_attr->value);
  }

  int num_profiles = 0;
  scoped_array<VAProfile> profiles(new VAProfile[vaMaxNumProfiles(m_display)]);
  CHECK(vaQueryConfigProfiles(m_display, profiles.get(), &num_profiles))

  for(int i = 0; i < num_profiles; i++)
    CLog::Log(LOGDEBUG, "VAAPI - profile %d", profiles[i]);

  VAConfigAttrib attrib;
  attrib.type = VAConfigAttribRTFormat;
  CHECK(vaGetConfigAttributes(m_display, profile, entrypoint, &attrib, 1))
  if ((attrib.value & VA_RT_FORMAT_YUV420) == 0)
      return false;
  
  CHECK(vaCreateSurfaces(m_display
                       , avctx->width
                       , avctx->height
                       , VA_RT_FORMAT_YUV420
                       , m_surfaces_count
                       , m_surfaces))

  for(unsigned i = 0; i < m_surfaces_count; i++)
    m_surfaces_free.push_back(m_surfaces[i]);

  CHECK(vaCreateConfig(m_display, profile, entrypoint, &attrib, 1, &m_config))


  CHECK(vaCreateContext(m_display
                      , m_config
                      , avctx->width
                      , avctx->height
                      , VA_PROGRESSIVE
                      , m_surfaces
                      , m_surfaces_count
                      , &m_context))


  //CHECK(vaCreateSurfaceGLX(va_context->display, GL_TEXTURE_2D, gl_texture, &gl_surface);


  m_hwaccel->config_id   = m_config;
  m_hwaccel->context_id  = m_context;

  avctx->hwaccel_context = m_hwaccel;
  avctx->thread_count    = 1;
  avctx->get_buffer      = GetBufferS;
  avctx->reget_buffer    = GetBufferS;
  avctx->release_buffer  = RelBufferS;
  avctx->draw_horiz_band = NULL;
  avctx->slice_flags     = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
  return true;
}

int CDecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
  return VC_BUFFER;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture)
{
  return true;
}

int CDecoder::Check(AVCodecContext* avctx)
{
  return 0;
}
