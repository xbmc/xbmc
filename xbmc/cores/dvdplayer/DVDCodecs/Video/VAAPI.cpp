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
#ifdef HAVE_LIBVA
#include "WindowingFactory.h"
#include "Settings.h"
#include "VAAPI.h"
#include "DVDVideoCodec.h"
#include <boost/scoped_array.hpp>

#define CHECK(a) \
do { \
  VAStatus res = a; \
  if(res != VA_STATUS_SUCCESS) \
  { \
    CLog::Log(LOGERROR, "VAAPI - failed executing "#a" at line %d with error %x:%s", __LINE__, res, vaErrorStr(res)); \
    return false; \
  } \
} while(0);

#define WARN(a) \
do { \
  VAStatus res = a; \
  if(res != VA_STATUS_SUCCESS) \
    CLog::Log(LOGWARNING, "VAAPI - failed executing "#a" at line %d with error %x:%s", __LINE__, res, vaErrorStr(res)); \
} while(0);


using namespace boost;
using namespace VAAPI;

static void RelBufferS(AVCodecContext *avctx, AVFrame *pic)
{ ((CDecoder*)((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetHardware())->RelBuffer(avctx, pic); }

static int GetBufferS(AVCodecContext *avctx, AVFrame *pic) 
{  return ((CDecoder*)((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetHardware())->GetBuffer(avctx, pic); }

static inline VASurfaceID GetSurfaceID(AVFrame *pic)
{ return (VASurfaceID)(uintptr_t)pic->data[3]; }


namespace
{
  struct VASurfaceHolder
  {
    VASurfaceHolder(VASurfaceID id, VADisplay display)
     : m_id(id)
     , m_display(display)
    {
    }

   ~VASurfaceHolder()
    {
      WARN(vaDestroySurfaces(m_display, &m_id, 1))
    }
    VASurfaceID m_id;
    VADisplay   m_display;
  };

  shared_ptr<VASurfaceID const> VASurfaceIDPtr( VASurfaceID id, VADisplay display )
  {
    shared_ptr<VASurfaceHolder> sw( new VASurfaceHolder(id, display) );
    return shared_ptr<VASurfaceID const>(sw,&sw->m_id);
  }

}


CDecoder::CDecoder()
{
  m_display  = NULL;
  m_config   = NULL;
  m_context  = NULL;
  m_hwaccel  = (vaapi_context*)calloc(1, sizeof(vaapi_context));
}

CDecoder::~CDecoder()
{
  Close();
  free(m_hwaccel);
}

void CDecoder::RelBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  VASurfaceID surface = GetSurfaceID(pic);
  
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
  VASurfaceID surface = GetSurfaceID(pic);
  if(surface)
  {
    /* reget call */
    std::list<VASurfaceID>::iterator it = m_surfaces_free.begin();
    for(; it != m_surfaces_free.end(); it++)
    {    
      if(*it == surface)
      {
        m_surfaces_used.push_back(surface);
        m_surfaces_free.erase(it);
        break;
      }
    }
    if(it == m_surfaces_free.end())
    {
      CLog::Log(LOGERROR, "VAAPI - unable to find requested surface");
      return -1;      
    }
  }
  else
  {
    if(m_surfaces_free.empty())
    {
      CLog::Log(LOGERROR, "VAAPI - unable to find free surface");
      return -1;
    }
    /* getbuffer call */
    surface = m_surfaces_free.front();
    m_surfaces_free.pop_front();
    m_surfaces_used.push_back(surface);
  }
  
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
  if(m_context)
    WARN(vaDestroyContext(m_display, m_context))
  m_context = NULL;

  if(m_config)
    WARN(vaDestroyConfig(m_display, m_config))
  m_config = NULL;
  
  for(std::list<VASurfaceID>::iterator it = m_surfaces_free.begin(); it != m_surfaces_free.end(); it++)
    WARN(vaDestroySurfaces(m_display, &(*it), 1))
  m_surfaces_free.clear();

  for(std::list<VASurfaceID>::iterator it = m_surfaces_used.begin(); it != m_surfaces_used.end(); it++)
    WARN(vaDestroySurfaces(m_display, &(*it), 1))
  m_surfaces_used.clear();
  
  if(m_display)
    WARN(vaTerminate(m_display))
  m_display = NULL;
}

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

  //shared_ptr<VASurfaceID const> test = VASurfaceIDPtr(m_surfaces[0], m_display);
  
  CHECK(vaCreateConfig(m_display, profile, entrypoint, &attrib, 1, &m_config))


  CHECK(vaCreateContext(m_display
                      , m_config
                      , avctx->width
                      , avctx->height
                      , VA_PROGRESSIVE
                      , m_surfaces
                      , m_surfaces_count
                      , &m_context))


  m_hwaccel->display     = m_display;
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
  if(frame)
    return VC_BUFFER | VC_PICTURE;
  else
    return VC_BUFFER;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture)
{
  picture->format        = DVDVideoPicture::FMT_VAAPI;
  picture->vaapi_object  = this;
  picture->vaapi_surface = GetSurfaceID(frame);
  return true;
}

int CDecoder::Check(AVCodecContext* avctx)
{
  return 0;
}

#endif
