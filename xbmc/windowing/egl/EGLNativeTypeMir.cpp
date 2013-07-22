/*
 *      Copyright (C) 2011-2013 Team XBMC
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
#include <deque>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <cstdlib>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>

#include "../DllMirClient.h"
#include "../DllXKBCommon.h"

#include "system.h"
#include "guilib/gui3d.h"
#include "utils/log.h"
#include "EGLNativeTypeMir.h"
#include "../WinEvents.h"
#include "Application.h"

#if defined(HAVE_MIR)

#include <mir_toolkit/mir_client_library.h>

namespace xbmc
{
namespace mir
{
class Surface;

class Connection :
  public IEventPipe,
  boost::noncopyable
{
public:

  Connection(IDllMirClient &clientLibrary,
             const char *server,
             const char *applicationName);
  ~Connection();

  MirConnection * GetMirConnection();
  
  void FetchDisplayInfo(MirDisplayInfo &);
  MirSurface * CreateSurface(const char *surfaceName,
                             int width,
                             int height,
                             MirPixelFormat pixelFormat,
                             MirBufferUsage bufferUsage);
  MirEGLNativeDisplayType * GetEGLDisplay();

private:

  IDllMirClient &m_clientLibrary;

  MirConnection *m_connection;
  MirEGLNativeDisplayType m_nativeDisplay;
  
  std::vector<IEventPipe *> m_Dispatchables;
  
  void Pump();
};

class Surface :
  public IEventPipe,
  boost::noncopyable
{
public:

  Surface(IDllMirClient &clientLibrary,
          MirSurface *surface);
  ~Surface();
  
  MirSurface * GetMirSurface();
  MirEGLNativeWindowType * GetEGLWindow();

private:

  /* Called from event handler threads */
  static void HandleEventFromThreadCallback(MirSurface *,
                                            MirEvent const *,
                                            void *);
  void HandleEventFromThread(MirEvent const *);
  
  /* Reads the message pipe */
  void Pump();
  void HandleEvent(const MirEvent &event);
  
  IDllMirClient &m_clientLibrary;
  
  MirSurface *m_surface;
  MirEGLNativeWindowType m_eglWindow;

  pthread_mutex_t m_mutex;
  std::deque<MirEvent> m_events;
};
}
}

namespace xm = xbmc::mir;

xm::Connection::Connection(IDllMirClient &clientLibrary,
                           const char *server,
                           const char *applicationName) :
  m_clientLibrary(clientLibrary),
  m_connection(m_clientLibrary.mir_connect_sync(server, applicationName)),
  m_nativeDisplay(NULL)
{
  if (!m_clientLibrary.mir_connection_is_valid(m_connection))
  {
    std::stringstream ss;
    ss << "Failed to connect to server :"
       << std::string(server ? server : " default ")
       << " with application name : "
       << std::string(applicationName);
    throw std::runtime_error(ss.str());
  }
}

xm::Connection::~Connection()
{
  m_clientLibrary.mir_connection_release(m_connection);
}

void
xm::Connection::FetchDisplayInfo(MirDisplayInfo &info)
{ 
  m_clientLibrary.mir_connection_get_display_info(m_connection, &info);
}

MirSurface *
xm::Connection::CreateSurface(const char *surfaceName,
                              int width,
                              int height,
                              MirPixelFormat format,
                              MirBufferUsage usage)
{
  MirSurfaceParameters param =
  {
    surfaceName,
    width,
    height,
    format,
    usage
  };
  
  return m_clientLibrary.mir_connection_create_surface_sync(m_connection,
                                                            &param);
}

MirEGLNativeDisplayType *
xm::Connection::GetEGLDisplay()
{
  if (!m_nativeDisplay)
  {
    m_nativeDisplay =
      m_clientLibrary.mir_connection_get_egl_native_display(m_connection);
  }
  
  return &m_nativeDisplay;
}

void
xm::Connection::Pump()
{
}

xm::Surface::Surface(IDllMirClient &clientLibrary,
                     MirSurface *surface) :
  m_clientLibrary(clientLibrary),
  m_surface(surface),
  m_eglWindow(NULL)
{
  if (!m_surface)
    throw std::runtime_error("Returned surface was not valid");

  pthread_mutex_init(&m_mutex, NULL);
  
  MirEventDelegate delegate =
  {
    xm::Surface::HandleEventFromThreadCallback,
    reinterpret_cast<void *>(this)
  };
  
  m_clientLibrary.mir_surface_set_event_handler(m_surface,
                                                &delegate);
}

namespace
{
class MutexLock :
  boost::noncopyable
{
public:

  MutexLock(pthread_mutex_t *mutex) :
    m_mutex(mutex)
  {
    pthread_mutex_lock(m_mutex);
  }
  
  ~MutexLock()
  {
    pthread_mutex_unlock(m_mutex);
  }
  
private:
  
  pthread_mutex_t *m_mutex;
};
}

void
xm::Surface::HandleEvent(const MirEvent &event)
{
  switch (event.type)
  {
    case mir_event_type_key:
      printf("key event\n");
      break;
    case mir_event_type_motion:
      printf("motion event\n");
      break;
    case mir_event_type_surface:
      printf("surface event\n");
      break;
    default:
      break;
  }
}

void
xm::Surface::Pump()
{
  MutexLock lock(&m_mutex);
  while(!m_events.empty())
  {
    HandleEvent(m_events.front());
    m_events.pop_front();
  }
}

void
xm::Surface::HandleEventFromThreadCallback(MirSurface *surface,
                                           const MirEvent *event,
                                           void *context)
{
  reinterpret_cast<Surface *>(context)->HandleEventFromThread(event);
}

void
xm::Surface::HandleEventFromThread(const MirEvent *event)
{
  MutexLock lock(&m_mutex);
  m_events.push_back(*event);
}

xm::Surface::~Surface()
{
  pthread_mutex_destroy(&m_mutex);
  m_clientLibrary.mir_surface_release_sync(m_surface);
}

MirEGLNativeWindowType *
xm::Surface::GetEGLWindow()
{
  if (!m_eglWindow)
  {
    m_eglWindow =
      m_clientLibrary.mir_surface_get_egl_native_window(m_surface);
  }
  
  return &m_eglWindow;
}

class CEGLNativeTypeMir::Private
{
public:

  DllMirClient m_clientLibrary;
  DllXKBCommon m_xkbCommonLibrary;
  
  boost::scoped_ptr<xm::Connection> m_connection;
  boost::scoped_ptr<xm::Surface> m_surface;

private:
};
#else
class CEGLNativeTypeMir::Private
{
};
#endif

CEGLNativeTypeMir::CEGLNativeTypeMir() :
  priv(new Private())
{
}

CEGLNativeTypeMir::~CEGLNativeTypeMir()
{
} 


bool CEGLNativeTypeMir::CheckCompatibility()
{
#if defined(HAVE_MIR)
  struct stat socketStat;
  if (stat("/tmp/mir_socket", &socketStat) == -1)
  {
    CLog::Log(LOGWARNING, "%s:, stat (/tmp/mir_socket): %s",
              __FUNCTION__, strerror(errno));
    return false;
  }
  
  /* FIXME:
   * There appears to be a bug in DllDynamic::CanLoad() which causes
   * it to always return false. We are just loading the library 
   * directly at CheckCompatibility time now */
  const struct LibraryStatus
  {
    const char *library;
    bool status;
  } libraryStatus[] =
  {
    { priv->m_clientLibrary.GetFile().c_str(), priv->m_clientLibrary.Load() },
    { priv->m_xkbCommonLibrary.GetFile().c_str(), priv->m_xkbCommonLibrary.Load() }
  };
  
  const size_t libraryStatusSize = sizeof(libraryStatus) /
                                   sizeof(libraryStatus[0]);
  bool loadFailure = false;

  for (size_t i = 0; i < libraryStatusSize; ++i)
  {
    if (!libraryStatus[i].status)
    {
      CLog::Log(LOGWARNING, "%s: Unable to load library: %s\n",
                __FUNCTION__, libraryStatus[i].library);
      loadFailure = true;
    }
  }

  if (loadFailure)
  {
    priv->m_clientLibrary.Unload();
    priv->m_xkbCommonLibrary.Unload();
    return false;
  }

  return true;
#else
  return false
#endif
}

void CEGLNativeTypeMir::Initialize()
{
}

void CEGLNativeTypeMir::Destroy()
{
#if defined(HAVE_MIR)
  priv->m_clientLibrary.Unload();
  priv->m_xkbCommonLibrary.Unload();
#endif
}

bool CEGLNativeTypeMir::CreateNativeDisplay()
{
#if defined(HAVE_MIR)
  try
  {
    priv->m_connection.reset(new xm::Connection(priv->m_clientLibrary,
                                                NULL,
                                                "xbmc"));
  }
  catch (const std::runtime_error &err)
  {
    CLog::Log(LOGERROR, "%s: Failed to get Mir connection: %s",
              __FUNCTION__, err.what());
    return false;
  }

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::CreateNativeWindow()
{
#if defined(HAVE_MIR)

  try
  {
    MirDisplayInfo info;
    priv->m_connection->FetchDisplayInfo(info);
    
    MirSurface *surface =
      priv->m_connection->CreateSurface("xbmc.mainwindow",
                                        info.width,
                                        info.height,
                                        mir_pixel_format_argb_8888,
                                        mir_buffer_usage_hardware);
    priv->m_surface.reset(new xm::Surface (priv->m_clientLibrary,
                                           surface));
  }
  catch (const std::runtime_error &err)
  {
    CLog::Log(LOGERROR, "%s: Failed to create surface: %s", 
              __FUNCTION__, err.what());
    return false;
  }
  
  CWinEventsMir::SetEventPipe(priv->m_clientLibrary,
                              priv->m_xkbCommonLibrary,
                              *priv->m_surface);

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
#if defined(HAVE_MIR)

  *nativeDisplay =
    reinterpret_cast<XBNativeDisplayType *>(priv->m_connection->GetEGLDisplay());

  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::GetNativeWindow(XBNativeDisplayType **nativeWindow) const
{
#if defined(HAVE_MIR)

  *nativeWindow =
    reinterpret_cast<XBNativeWindowType *>(priv->m_surface->GetEGLWindow());

  return true;
#else
  return false;
#endif
}  

bool CEGLNativeTypeMir::DestroyNativeDisplay()
{
#if defined(HAVE_MIR)
  priv->m_connection.reset();
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::DestroyNativeWindow()
{
#if defined(HAVE_MIR)
  CWinEventsMir::RemoveEventPipe();
  priv->m_surface.reset();
  return true;  
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::GetNativeResolution(RESOLUTION_INFO *res) const
{
#if defined(HAVE_MIR)
  MirDisplayInfo info;
    priv->m_connection->FetchDisplayInfo(info);
    
  res->iWidth = info.width;
  res->iHeight = info.height;
  
  /* The refresh rate is given as an integer in the second exponent
   * so we need to divide by 100.0f to get a floating point value */
  res->fRefreshRate = 60.0f;
  res->dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  res->iScreen = 0;
  res->bFullScreen = true;
  res->iSubtitles = static_cast<int>(0.965 * res->iHeight);
  res->fPixelRatio = 1.0f;
  res->iScreenWidth = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->strMode.Format("%dx%d @ %.2fp",
                      res->iScreenWidth,
                      res->iScreenHeight,
                      res->fRefreshRate);
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::SetNativeResolution(const RESOLUTION_INFO &res)
{
#if defined(HAVE_MIR)
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
#if defined(HAVE_MIR)
  RESOLUTION_INFO info;
  GetNativeResolution(&info);
  resolutions.push_back(info);
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::GetPreferredResolution(RESOLUTION_INFO *res) const
{
#if defined(HAVE_MIR)
  GetNativeResolution(res);
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeMir::ShowWindow(bool show)
{
#if defined(HAVE_MIR)
  return true;
#else
  return false;
#endif
}
