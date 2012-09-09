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
#include "system.h"
#ifdef HAVE_LIBVA
#include "windowing/WindowingFactory.h"
#include "settings/Settings.h"
#include "VAAPI.h"
#include "DVDVideoCodec.h"
#include <boost/scoped_array.hpp>
#include <boost/weak_ptr.hpp>

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

#ifndef VA_SURFACE_ATTRIB_SETTABLE
#define vaCreateSurfaces(d, f, w, h, s, ns, a, na) \
    vaCreateSurfaces(d, w, h, f, ns, s)
#endif

using namespace std;
using namespace boost;
using namespace VAAPI;

static int compare_version(int major_l, int minor_l, int micro_l, int major_r, int minor_r, int micro_r)
{
  if(major_l < major_r) return -1;
  if(major_l > major_r) return  1;
  if(minor_l < minor_r) return -1;
  if(minor_l > minor_r) return  1;
  if(micro_l < micro_r) return -1;
  if(micro_l > micro_r) return  1;
  return 0;
}

static void RelBufferS(AVCodecContext *avctx, AVFrame *pic)
{ ((CDecoder*)((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetHardware())->RelBuffer(avctx, pic); }

static int GetBufferS(AVCodecContext *avctx, AVFrame *pic) 
{  return ((CDecoder*)((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetHardware())->GetBuffer(avctx, pic); }

static inline VASurfaceID GetSurfaceID(AVFrame *pic)
{ return (VASurfaceID)(uintptr_t)pic->data[3]; }

static CDisplayPtr GetGlobalDisplay()
{
  static weak_ptr<CDisplay> display_global;

  CDisplayPtr display(display_global.lock());
  if(display)
  {
    if(display->lost())
    {
      CLog::Log(LOGERROR, "VAAPI - vaapi display is in lost state");
      display.reset();
    }    
    return display;
  }

  VADisplay disp;
  disp = vaGetDisplayGLX(g_Windowing.GetDisplay());

  int major_version, minor_version;
  VAStatus res = vaInitialize(disp, &major_version, &minor_version);

  CLog::Log(LOGDEBUG, "VAAPI - initialize version %d.%d", major_version, minor_version);

  if(res != VA_STATUS_SUCCESS)
  {
    CLog::Log(LOGERROR, "VAAPI - unable to initialize display %d - %s", res, vaErrorStr(res));
    return display;
  }

  const char* vendor = vaQueryVendorString(disp);
  CLog::Log(LOGDEBUG, "VAAPI - vendor: %s", vendor);

  bool deinterlace = true;
  int major, minor, micro;
  if(sscanf(vendor,  "Intel i965 driver - %d.%d.%d", &major, &minor, &micro) == 3)
  {
    /* older version will crash and burn */
    if(compare_version(major, minor, micro, 1, 0, 17) < 0)
    {
      CLog::Log(LOGDEBUG, "VAAPI - deinterlace not support on this intel driver version");
      deinterlace = false;
    }
  }

  display = CDisplayPtr(new CDisplay(disp, deinterlace));
  display_global = display;
  return display;
}

CDisplay::~CDisplay()
{
  CLog::Log(LOGDEBUG, "VAAPI - destroying display %p", m_display);
  WARN(vaTerminate(m_display))
}

CSurface::~CSurface()
{
  CLog::Log(LOGDEBUG, "VAAPI - destroying surface 0x%x", (int)m_id);
  CSingleLock lock(*m_display);
  WARN(vaDestroySurfaces(m_display->get(), &m_id, 1))
}

CSurfaceGL::~CSurfaceGL()
{
  CLog::Log(LOGDEBUG, "VAAPI - destroying glx surface %p", m_id);
  CSingleLock lock(*m_display);
  WARN(vaDestroySurfaceGLX(m_display->get(), m_id))
}

CDecoder::CDecoder()
{
  m_refs            = 0;
  m_surfaces_count  = 0;
  m_config          = 0;
  m_context         = 0;
  m_hwaccel         = (vaapi_context*)calloc(1, sizeof(vaapi_context));
  memset(m_surfaces, 0, sizeof(*m_surfaces));
}

CDecoder::~CDecoder()
{
  Close();
  free(m_hwaccel);
}

void CDecoder::RelBuffer(AVCodecContext *avctx, AVFrame *pic)
{
  VASurfaceID surface = GetSurfaceID(pic);

  for(std::list<CSurfacePtr>::iterator it = m_surfaces_used.begin(); it != m_surfaces_used.end(); it++)
  {    
    if((*it)->m_id == surface)
    {
      m_surfaces_free.push_back(*it);
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
  CSurface*   wrapper = NULL;
  std::list<CSurfacePtr>::iterator it = m_surfaces_free.begin();
  if(surface)
  {
    /* reget call */
    for(; it != m_surfaces_free.end(); it++)
    {
      if((*it)->m_id == surface)
      {
        wrapper = it->get();
        m_surfaces_used.push_back(*it);
        m_surfaces_free.erase(it);
        break;
      }
    }
    if(!wrapper)
    {
      CLog::Log(LOGERROR, "VAAPI - unable to find requested surface");
      return -1; 
    }
  }
  else
  {
    // To avoid stutter, we scan the free surface pool (provided by decoder) for surfaces
    // that are 100% not in use by renderer. The pointers to these surfaces have a use_count of 1.
    for (; it != m_surfaces_free.end() && it->use_count() > 1; it++) {}

    // If we have zero free surface from decoder OR all free surfaces are in use by renderer, we allocate a new surface
    if (it == m_surfaces_free.end())
    {
      if (!m_surfaces_free.empty()) CLog::Log(LOGERROR, "VAAPI - renderer still using all freed up surfaces by decoder");
      CLog::Log(LOGERROR, "VAAPI - unable to find free surface, trying to allocate a new one");
      if(!EnsureSurfaces(avctx, m_surfaces_count+1) || m_surfaces_free.empty())
      {
        CLog::Log(LOGERROR, "VAAPI - unable to find free surface");
        return -1;
      }
      // Set itarator position to the newly allocated surface (end-1)
      it = m_surfaces_free.end(); it--;
    }
    /* getbuffer call */
    wrapper = it->get();
    surface = wrapper->m_id;
    m_surfaces_used.push_back(*it);
    m_surfaces_free.erase(it);
  }

  pic->type           = FF_BUFFER_TYPE_USER;
  pic->data[0]        = (uint8_t*)wrapper;
  pic->data[1]        = NULL;
  pic->data[2]        = NULL;
  pic->data[3]        = (uint8_t*)surface;
  pic->linesize[0]    = 0;
  pic->linesize[1]    = 0;
  pic->linesize[2]    = 0;
  pic->linesize[3]    = 0;
  pic->reordered_opaque= avctx->reordered_opaque;
  return 0;
}

void CDecoder::Close()
{ 
  if(m_context)
    WARN(vaDestroyContext(m_display->get(), m_context))
  m_context = 0;

  if(m_config)
    WARN(vaDestroyConfig(m_display->get(), m_config))
  m_config = 0;
  
  m_surfaces_free.clear();
  m_surfaces_used.clear();
  m_surfaces_count = 0;
  m_refs           = 0;
  memset(m_hwaccel , 0, sizeof(*m_hwaccel));
  memset(m_surfaces, 0, sizeof(*m_surfaces));
  m_display.reset();
  m_holder.surface.reset();
}

bool CDecoder::Open(AVCodecContext *avctx, enum PixelFormat fmt, unsigned int surfaces)
{
  VAEntrypoint entrypoint = VAEntrypointVLD;
  VAProfile    profile;

  CLog::Log(LOGDEBUG, "VAAPI - attempting to open codec %d with profile %d at level %d with %d reference frames", avctx->codec_id, avctx->profile, avctx->level, avctx->refs);

  vector<VAProfile> accepted;
  switch (avctx->codec_id) {
    case CODEC_ID_MPEG2VIDEO:
      accepted.push_back(VAProfileMPEG2Main);
      break;
    case CODEC_ID_MPEG4:
    case CODEC_ID_H263:
      accepted.push_back(VAProfileMPEG4AdvancedSimple);
      break;
    case CODEC_ID_H264:
    {
#ifdef FF_PROFILE_H264_BASELINE
      if  (avctx->profile == FF_PROFILE_H264_BASELINE)
        accepted.push_back(VAProfileH264Baseline);
      else
      {
        if(avctx->profile == FF_PROFILE_H264_MAIN)
          accepted.push_back(VAProfileH264Main); 
#else
      {
        // fallback to high profile if libavcodec is too old to export
        // profile information; it will likely work
#endif
        // fallback to high profile if main profile is not available
        accepted.push_back(VAProfileH264High);
      }
      break;
    }
    case CODEC_ID_WMV3:
      accepted.push_back(VAProfileVC1Main);
      break;
    case CODEC_ID_VC1:
      accepted.push_back(VAProfileVC1Advanced);
      break;
    default:
      return false;
  }

  m_display = GetGlobalDisplay();
  if(!m_display)
    return false;

  int num_display_attrs = 0;
  scoped_array<VADisplayAttribute> display_attrs(new VADisplayAttribute[vaMaxNumDisplayAttributes(m_display->get())]);

  CHECK(vaQueryDisplayAttributes(m_display->get(), display_attrs.get(), &num_display_attrs))

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
  scoped_array<VAProfile> profiles(new VAProfile[vaMaxNumProfiles(m_display->get())]);
  CHECK(vaQueryConfigProfiles(m_display->get(), profiles.get(), &num_profiles))

  for(int i = 0; i < num_profiles; i++)
    CLog::Log(LOGDEBUG, "VAAPI - profile %d", profiles[i]);

  vector<VAProfile>::iterator selected = find_first_of(accepted.begin()
                                                     , accepted.end()
                                                     , profiles.get()
                                                     , profiles.get()+num_profiles);
  if(selected == accepted.end())
  {
    CLog::Log(LOGDEBUG, "VAAPI - unable to find a suitable profile");
    return false;
  }

  profile = *selected;

  VAConfigAttrib attrib;
  attrib.type = VAConfigAttribRTFormat;
  CHECK(vaGetConfigAttributes(m_display->get(), profile, entrypoint, &attrib, 1))

  if ((attrib.value & VA_RT_FORMAT_YUV420) == 0)
  {
    CLog::Log(LOGERROR, "VAAPI - invalid yuv format %x", attrib.value);
    return false;
  }

  CHECK(vaCreateConfig(m_display->get(), profile, entrypoint, &attrib, 1, &m_hwaccel->config_id))
  m_config = m_hwaccel->config_id;

  if (!EnsureContext(avctx))
    return false;

  m_hwaccel->display     = m_display->get();

  avctx->hwaccel_context = m_hwaccel;
  avctx->thread_count    = 1;
  avctx->get_buffer      = GetBufferS;
  avctx->reget_buffer    = GetBufferS;
  avctx->release_buffer  = RelBufferS;
  avctx->draw_horiz_band = NULL;
  avctx->slice_flags     = SLICE_FLAG_CODED_ORDER|SLICE_FLAG_ALLOW_FIELD;
  return true;
}

bool CDecoder::EnsureContext(AVCodecContext *avctx)
{
  if(avctx->refs && avctx->refs <= m_refs)
    return true;

  if(m_refs > 0)
    CLog::Log(LOGWARNING, "VAAPI - reference frame count increasing, reiniting decoder");

  m_refs = avctx->refs;
  if(m_refs == 0)
  {
    if(avctx->codec_id == CODEC_ID_H264)
      m_refs = 16;
    else
      m_refs = 2;
  }
  return EnsureSurfaces(avctx, m_refs + 3);
}

bool CDecoder::EnsureSurfaces(AVCodecContext *avctx, unsigned n_surfaces_count)
{
  CLog::Log(LOGDEBUG, "VAAPI - making sure %d surfaces are allocated for given %d references", n_surfaces_count, avctx->refs);

  if(n_surfaces_count <= m_surfaces_count)
    return true;

  const unsigned old_surfaces_count = m_surfaces_count;
  m_surfaces_count = n_surfaces_count;

  CHECK(vaCreateSurfaces(m_display->get()
                       , VA_RT_FORMAT_YUV420
                       , avctx->width
                       , avctx->height
                       , &m_surfaces[old_surfaces_count]
                       , m_surfaces_count - old_surfaces_count
                       , NULL
                       , 0))

  for(unsigned i = old_surfaces_count; i < m_surfaces_count; i++)
    m_surfaces_free.push_back(CSurfacePtr(new CSurface(m_surfaces[i], m_display)));

  //shared_ptr<VASurfaceID const> test = VASurfaceIDPtr(m_surfaces[0], m_display);

  if(m_context)
    WARN(vaDestroyContext(m_display->get(), m_context))
  m_context = 0;

  CHECK(vaCreateContext(m_display->get()
                      , m_config
                      , avctx->width
                      , avctx->height
                      , VA_PROGRESSIVE
                      , m_surfaces
                      , m_surfaces_count
                      , &m_hwaccel->context_id))
  m_context = m_hwaccel->context_id;
  return true;
}

int CDecoder::Decode(AVCodecContext* avctx, AVFrame* frame)
{
  int status = Check(avctx);
  if(status)
    return status;

  if(frame)
    return VC_BUFFER | VC_PICTURE;
  else
    return VC_BUFFER;
}

bool CDecoder::GetPicture(AVCodecContext* avctx, AVFrame* frame, DVDVideoPicture* picture)
{
  ((CDVDVideoCodecFFmpeg*)avctx->opaque)->GetPictureCommon(picture);
  VASurfaceID surface = GetSurfaceID(frame);


  m_holder.surface.reset();

  std::list<CSurfacePtr>::iterator it;
  for(it = m_surfaces_used.begin(); it != m_surfaces_used.end() && !m_holder.surface; it++)
  {    
    if((*it)->m_id == surface)
    {
      m_holder.surface = *it;
      break;
    }
  }

  for(it = m_surfaces_free.begin(); it != m_surfaces_free.end() && !m_holder.surface; it++)
  {    
    if((*it)->m_id == surface)
    {
      m_holder.surface = *it;
      break;
    }
  }
  if(!m_holder.surface)
  {
    CLog::Log(LOGERROR, "VAAPI - Unable to find surface");
    return false;
  }

  picture->format = RENDER_FMT_VAAPI;
  picture->vaapi  = &m_holder;
  return true;
}

int CDecoder::Check(AVCodecContext* avctx)
{
  if (m_display == NULL)
  {
    if(!Open(avctx, avctx->pix_fmt))
    {
      CLog::Log(LOGERROR, "VAAPI - Unable to recover device after display was closed");
      Close();
      return VC_ERROR;
    }
  }

  if (m_display->lost())
  {
    Close();
    if(!Open(avctx, avctx->pix_fmt))
    {
      CLog::Log(LOGERROR, "VAAPI - Unable to recover device after display was lost");
      Close();
      return VC_ERROR;
    }
    return VC_FLUSHED;
  }

  if (!EnsureContext(avctx))
    return VC_ERROR;

  m_holder.surface.reset();
  return 0;
}

#endif
