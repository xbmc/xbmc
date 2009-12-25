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

#define BOBLIGHT_DLOPEN
#include <libboblight/boblight.h>

#include "Boblight.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "GUISettings.h"
#include "Settings.h"
#include "utils/log.h"
#include "StdString.h"

#define WIDTH     64
#define HEIGHT    64
#define PRIORITY  64

CBoblightClient g_boblight;

void CBoblightClient::Initialize()
{
  m_haslib = true;

  char* boblight_error = boblight_loadlibrary(NULL);
  if (boblight_error)
  {
    CLog::Log(LOGERROR, "Error loading boblight library: %s", boblight_error);
    m_haslib = false;
  }

  m_captureandsend = false;
  m_enabled = false;
  m_boblight = NULL;
  m_pixels = new unsigned char[WIDTH * HEIGHT * 4];
  m_pbo = 0;
  m_query = 0;
}

void CBoblightClient::Start()
{
  if (!m_haslib)
    return;

  if (ThreadHandle() == NULL)
  {
    if (!glewIsSupported("GL_ARB_pixel_buffer_object"))
    {
      CLog::Log(LOGERROR, "CBoblightClient: GL_ARB_pixel_buffer_object not supported");
      return;
    }

    if (!glewIsSupported("GL_ARB_occlusion_query"))
    {
      CLog::Log(LOGERROR, "CBoblightClient: GL_ARB_occlusion_query not supported");
      return;
    }

    memset(m_pixels, 0, WIDTH * HEIGHT * 4);

    //hack hack, CBaseTexture::GetPixels() needs to return NULL for the pbo to work
    m_texture.Allocate(WIDTH, HEIGHT, XB_FMT_A8R8G8B8);
    m_texture.LoadToGPU();
    m_texture.DestroyTextureObject();

    glGenBuffersARB(1, &m_pbo);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);
    glBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, WIDTH * HEIGHT * 4, (GLvoid*)m_pixels, GL_STREAM_READ_ARB);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);

    glGenQueriesARB(1, &m_query);

    m_enabled = true;
    m_captureandsend = false;
    m_needsettingload = true;
    m_needsprocessing = false;
    m_captureevent.Set();

    Create();
  }
}

void CBoblightClient::Stop()
{
  if (!m_haslib)
    return;

  m_bStop = true;
  m_enabled = false;
  m_captureandsend = false;
  m_captureevent.Set();
  StopThread();

  if (m_pbo)
  {
    glDeleteBuffersARB(1, &m_pbo);
    m_pbo = 0;
  }
  if (m_query)
  {
    glDeleteQueriesARB(1, &m_query);
    m_query = 0;
  }
}

void CBoblightClient::CaptureVideo()
{
  ProcessVideo();

  if (m_enabled && g_renderManager.IsConfigured() && !m_needsprocessing)
  {
    glBeginQueryARB(GL_SAMPLES_PASSED_ARB, m_query);

    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);

    glReadBuffer(GL_AUX0);
    glDrawBuffer(GL_AUX0);
    g_renderManager.CreateThumbnail(&m_texture, WIDTH, HEIGHT);
    glReadBuffer(GL_BACK);
    glDrawBuffer(GL_BACK);

    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);

    glEndQueryARB(GL_SAMPLES_PASSED_ARB);

    m_needsprocessing = true;

    if (g_guiSettings.GetBool("videoplayer.boblighttestmode"))
    {
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, m_pbo);
      glRasterPos2i(WIDTH, HEIGHT * 2);
      glDrawPixels(WIDTH, HEIGHT, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
    }
  }
}

void CBoblightClient::ProcessVideo()
{
  if (m_enabled && m_needsprocessing)
  {
    GLuint queryresult;
    glGetQueryObjectuivARB(m_query, GL_QUERY_RESULT_AVAILABLE_ARB, &queryresult);
    if (!queryresult)
      return;

    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);
    unsigned char* pbodata = (unsigned char*)glMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
    memcpy(m_pixels, pbodata, WIDTH * HEIGHT * 4);
    glUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
    glBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);

    m_captureandsend = true;
    m_needsprocessing = false;
    m_captureevent.Set();
  }
}

void CBoblightClient::Disable()
{
  if (m_captureandsend)
  {
    memset(m_pixels, 0, WIDTH * HEIGHT * 4);

    m_needsprocessing = false;
    m_captureandsend = false;
    m_captureevent.Set();
  }
}

void CBoblightClient::Process()
{
  while(!m_bStop)
  {
    m_captureevent.Wait();

    if (!Connect())
    {
      Sleep(10000);
      continue;
    }

    if (m_captureandsend)
    {
      boblight_setscanrange(m_boblight, WIDTH, HEIGHT);

      int rgb[3];
      for (int y = 0; y < HEIGHT; y++)
      {
        unsigned char* pixelptr = m_pixels + y * WIDTH * 4;
        for (int x = 0; x < WIDTH; x++)
        {
          rgb[2] = *pixelptr++;
          rgb[1] = *pixelptr++;
          rgb[0] = *pixelptr++;
          pixelptr++;

          boblight_addpixelxy(m_boblight, x, y, rgb);
        }
      }

      boblight_sendrgb(m_boblight, 1, NULL);

      if (LoadSettings())
        SetBoblightSettings();

      if (m_priority != PRIORITY)
      {
        m_priority = PRIORITY;
        boblight_setpriority(m_boblight, m_priority);
      }
    }
    else
    {
      if (m_priority != 255)
      {
        m_priority = 255;
        boblight_setpriority(m_boblight, m_priority);
      }
    }
  }
}

bool CBoblightClient::LoadSettings()
{
  if (m_value         != g_settings.m_currentVideoSettings.m_BoblightValue ||
      m_valuerangemin != g_settings.m_currentVideoSettings.m_BoblightValueMin ||
      m_valuerangemax != g_settings.m_currentVideoSettings.m_BoblightValueMax ||
      m_saturation    != g_settings.m_currentVideoSettings.m_BoblightSaturation ||
      m_satrangemin   != g_settings.m_currentVideoSettings.m_BoblightSaturationMin ||
      m_satrangemax   != g_settings.m_currentVideoSettings.m_BoblightSaturationMax ||
      m_speed         != g_settings.m_currentVideoSettings.m_BoblightSpeed ||
      m_autospeed     != g_settings.m_currentVideoSettings.m_BoblightAutoSpeed ||
      m_needsettingload)
  {
    m_value         = g_settings.m_currentVideoSettings.m_BoblightValue;
    m_valuerangemin = g_settings.m_currentVideoSettings.m_BoblightValueMin;
    m_valuerangemax = g_settings.m_currentVideoSettings.m_BoblightValueMax;
    m_saturation    = g_settings.m_currentVideoSettings.m_BoblightSaturation;
    m_satrangemin   = g_settings.m_currentVideoSettings.m_BoblightSaturationMin;
    m_satrangemax   = g_settings.m_currentVideoSettings.m_BoblightSaturationMax;
    m_speed         = g_settings.m_currentVideoSettings.m_BoblightSpeed;
    m_autospeed     = g_settings.m_currentVideoSettings.m_BoblightAutoSpeed;

    m_needsettingload = false;
    return true;
  }

  return false;
}

bool CBoblightClient::Connect()
{
  if (!m_boblight)
  {
    m_boblight = boblight_init();

    if (!boblight_connect(m_boblight, NULL, -1, 5000000))
    {
      CLog::Log(LOGERROR, "CBoblightClient: error connecting to boblightd: %s", boblight_geterror(m_boblight));
      boblight_destroy(m_boblight);
      m_boblight = NULL;
      m_captureevent.Set();
      return false;
    }
  }

  return true;
}

bool CBoblightClient::SetBoblightSettings()
{
  CStdString setting;

  setting.Format("value %f", m_value);
  boblight_setoption(m_boblight, -1, setting.c_str());
  CLog::Log(LOGDEBUG, "CBoblightClient: %s\n", setting.c_str());

  setting.Format("valuemin %f", m_valuerangemin);
  boblight_setoption(m_boblight, -1, setting.c_str());
  CLog::Log(LOGDEBUG, "CBoblightClient: %s\n", setting.c_str());

  setting.Format("valuemax %f", m_valuerangemax);
  boblight_setoption(m_boblight, -1, setting.c_str());
  CLog::Log(LOGDEBUG, "CBoblightClient: %s\n", setting.c_str());

  setting.Format("saturation %f", m_saturation);
  boblight_setoption(m_boblight, -1, setting.c_str());
  CLog::Log(LOGDEBUG, "CBoblightClient: %s\n", setting.c_str());

  setting.Format("saturationmin %f", m_satrangemin);
  boblight_setoption(m_boblight, -1, setting.c_str());
  CLog::Log(LOGDEBUG, "CBoblightClient: %s\n", setting.c_str());

  setting.Format("saturationmax %f", m_satrangemax);
  boblight_setoption(m_boblight, -1, setting.c_str());
  CLog::Log(LOGDEBUG, "CBoblightClient: %s\n", setting.c_str());

  setting.Format("speed %f", m_speed);
  boblight_setoption(m_boblight, -1, setting.c_str());
  CLog::Log(LOGDEBUG, "CBoblightClient: %s\n", setting.c_str());

  setting.Format("autospeed %f", m_autospeed);
  boblight_setoption(m_boblight, -1, setting.c_str());
  CLog::Log(LOGDEBUG, "CBoblightClient: %s\n", setting.c_str());

  setting.Format("threshold 16");
  boblight_setoption(m_boblight, -1, setting.c_str());
  CLog::Log(LOGDEBUG, "CBoblightClient: %s\n", setting.c_str());

  return true;
}

