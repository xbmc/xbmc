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

#include <assert.h>
#include "settings/Settings.h"
#include "utils/log.h"

#include "cores/omxplayer/OMXImage.h"

#include <sys/ioctl.h>
#include "rpi/rpi_user_vcsm.h"
#include "utils/TimeUtils.h"

#define MAJOR_NUM 100
#define IOCTL_MBOX_PROPERTY _IOWR(MAJOR_NUM, 0, char *)
#define DEVICE_FILE_NAME "/dev/vcio"

static int mbox_open();
static void mbox_close(int file_desc);

CRBP::CRBP()
{
  m_initialized     = false;
  m_omx_initialized = false;
  m_DllBcmHost      = new DllBcmHost();
  m_OMX             = new COMXCore();
  m_display = DISPMANX_NO_HANDLE;
  m_mb = mbox_open();
  vcsm_init();
  m_vsync_count = 0;
  m_vsync_time = 0;
}

CRBP::~CRBP()
{
  Deinitialize();
  delete m_OMX;
  delete m_DllBcmHost;
}

bool CRBP::Initialize()
{
  CSingleLock lock(m_critSection);
  if (m_initialized)
    return true;

  m_initialized = m_DllBcmHost->Load();
  if(!m_initialized)
    return false;

  m_DllBcmHost->bcm_host_init();

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

  m_gui_resolution_limit = CSettings::GetInstance().GetInt("videoscreen.limitgui");
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

static void vsync_callback_static(DISPMANX_UPDATE_HANDLE_T u, void *arg)
{
  CRBP *rbp = reinterpret_cast<CRBP*>(arg);
  rbp->VSyncCallback();
}

DISPMANX_DISPLAY_HANDLE_T CRBP::OpenDisplay(uint32_t device)
{
  CSingleLock lock(m_critSection);
  if (m_display == DISPMANX_NO_HANDLE)
  {
    m_display = vc_dispmanx_display_open( 0 /*screen*/ );
    int s = vc_dispmanx_vsync_callback(m_display, vsync_callback_static, (void *)this);
    assert(s == 0);
  }
  return m_display;
}

void CRBP::CloseDisplay(DISPMANX_DISPLAY_HANDLE_T display)
{
  CSingleLock lock(m_critSection);
  assert(display == m_display);
  int s = vc_dispmanx_vsync_callback(m_display, NULL, NULL);
  assert(s == 0);
  vc_dispmanx_display_close(m_display);
  m_display = DISPMANX_NO_HANDLE;
}

void CRBP::GetDisplaySize(int &width, int &height)
{
  CSingleLock lock(m_critSection);
  DISPMANX_MODEINFO_T info;
  if (m_display != DISPMANX_NO_HANDLE && vc_dispmanx_display_get_info(m_display, &info) == 0)
  {
    width = info.width;
    height = info.height;
  }
  else
  {
    width = 0;
    height = 0;
  }
}

unsigned char *CRBP::CaptureDisplay(int width, int height, int *pstride, bool swap_red_blue, bool video_only)
{
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

  stride = ((width + 15) & ~15) * 4;

  CSingleLock lock(m_critSection);
  if (m_display != DISPMANX_NO_HANDLE)
  {
    image = new unsigned char [height * stride];
    resource = vc_dispmanx_resource_create( VC_IMAGE_RGBA32, width, height, &vc_image_ptr );

    vc_dispmanx_snapshot(m_display, resource, (DISPMANX_TRANSFORM_T)flags);

    vc_dispmanx_rect_set(&rect, 0, 0, width, height);
    vc_dispmanx_resource_read_data(resource, &rect, image, stride);
    vc_dispmanx_resource_delete( resource );
  }
  if (pstride)
    *pstride = stride;
  return image;
}

void CRBP::VSyncCallback()
{
  CSingleLock lock(m_vsync_lock);
  m_vsync_count++;
  m_vsync_time = CurrentHostCounter();
  m_vsync_cond.notifyAll();
}

uint32_t CRBP::WaitVsync(uint32_t target)
{
  CSingleLock vlock(m_vsync_lock);
  DISPMANX_DISPLAY_HANDLE_T display = m_display;
  XbmcThreads::EndTime delay(50);
  if (target == ~0U)
    target = m_vsync_count+1;
  while (!delay.IsTimePast())
  {
    if ((signed)(m_vsync_count - target) >= 0)
      break;
    if (!m_vsync_cond.wait(vlock, delay.MillisLeft()))
      break;
  }
  if ((signed)(m_vsync_count - target) < 0)
    CLog::Log(LOGDEBUG, "CRBP::%s no  vsync %d/%d display:%x(%x) delay:%d", __FUNCTION__, m_vsync_count, target, m_display, display, delay.MillisLeft());

  return m_vsync_count;
}

uint32_t CRBP::LastVsync(int64_t &time)
{
  CSingleLock lock(m_vsync_lock);
  time = m_vsync_time;
  return m_vsync_count;
}

uint32_t CRBP::LastVsync()
{
  int64_t time = 0;
  return LastVsync(time);
}

void CRBP::Deinitialize()
{
  if (m_omx_image_init)
    g_OMXImage.Deinitialize();

  if(m_omx_initialized)
    m_OMX->Deinitialize();

  if (m_display)
    CloseDisplay(m_display);

  m_DllBcmHost->bcm_host_deinit();

  if(m_initialized)
    m_DllBcmHost->Unload();

  m_omx_image_init  = false;
  m_initialized     = false;
  m_omx_initialized = false;
  if (m_mb)
    mbox_close(m_mb);
  m_mb = 0;
  vcsm_exit();
}

static int mbox_property(int file_desc, void *buf)
{
   int ret_val = ioctl(file_desc, IOCTL_MBOX_PROPERTY, buf);

   if (ret_val < 0)
   {
     CLog::Log(LOGERROR, "%s: ioctl_set_msg failed:%d", __FUNCTION__, ret_val);
   }
   return ret_val;
}

static int mbox_open()
{
   int file_desc;

   // open a char device file used for communicating with kernel mbox driver
   file_desc = open(DEVICE_FILE_NAME, 0);
   if (file_desc < 0)
     CLog::Log(LOGERROR, "%s: Can't open device file: %s (%d)", __FUNCTION__, DEVICE_FILE_NAME, file_desc);

   return file_desc;
}

static void mbox_close(int file_desc)
{
  close(file_desc);
}

static unsigned mem_lock(int file_desc, unsigned handle)
{
   int i=0;
   unsigned p[32];
   p[i++] = 0; // size
   p[i++] = 0x00000000; // process request

   p[i++] = 0x3000d; // (the tag id)
   p[i++] = 4; // (size of the buffer)
   p[i++] = 4; // (size of the data)
   p[i++] = handle;

   p[i++] = 0x00000000; // end tag
   p[0] = i*sizeof *p; // actual size

   mbox_property(file_desc, p);
   return p[5];
}

unsigned mem_unlock(int file_desc, unsigned handle)
{
   int i=0;
   unsigned p[32];
   p[i++] = 0; // size
   p[i++] = 0x00000000; // process request

   p[i++] = 0x3000e; // (the tag id)
   p[i++] = 4; // (size of the buffer)
   p[i++] = 4; // (size of the data)
   p[i++] = handle;

   p[i++] = 0x00000000; // end tag
   p[0] = i*sizeof *p; // actual size

   mbox_property(file_desc, p);
   return p[5];
}

CGPUMEM::CGPUMEM(unsigned int numbytes, bool cached)
{
  m_numbytes = numbytes;
  m_vcsm_handle = vcsm_malloc_cache(numbytes, cached ? VCSM_CACHE_TYPE_HOST : VCSM_CACHE_TYPE_NONE, (char *)"CGPUMEM");
  assert(m_vcsm_handle);
  m_vc_handle = vcsm_vc_hdl_from_hdl(m_vcsm_handle);
  assert(m_vc_handle);
  m_arm = vcsm_lock(m_vcsm_handle);
  assert(m_arm);
  m_vc = mem_lock(g_RBP.GetMBox(), m_vc_handle);
  assert(m_vc);
}

CGPUMEM::~CGPUMEM()
{
  mem_unlock(g_RBP.GetMBox(), m_vc_handle);
  vcsm_unlock_ptr(m_arm);
  vcsm_free(m_vcsm_handle);
}

// Call this to clean and invalidate a region of memory
void CGPUMEM::Flush()
{
  struct vcsm_user_clean_invalid_s iocache = {};
  iocache.s[0].handle = m_vcsm_handle;
  iocache.s[0].cmd = 3; // clean+invalidate
  iocache.s[0].addr = (int) m_arm;
  iocache.s[0].size  = m_numbytes;
  vcsm_clean_invalid( &iocache );
}

#endif
