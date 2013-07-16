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
#include <sstream>
#include <stdexcept>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>

#include <wayland-client.h>

#include "guilib/Resolution.h"
#include "guilib/gui3d.h"

#include "windowing/DllWaylandClient.h"
#include "windowing/DllXKBCommon.h"

#include "Callback.h"
#include "Compositor.h"
#include "Display.h"
#include "Registry.h"
#include "Region.h"
#include "Shell.h"

#include "windowing/WaylandProtocol.h"
#include "XBMCConnection.h"

namespace xbmc
{
namespace wayland
{
class XBMCConnection::Private :
  public IWaylandRegistration
{
public:

  Private(IDllWaylandClient &clientLibrary,
          IDllXKBCommon &xkbCommonLibrary,
          EventInjector &eventInjector);
  ~Private();

  IDllWaylandClient &m_clientLibrary;
  IDllXKBCommon &m_xkbCommonLibrary;
  
  EventInjector &m_eventInjector;

  boost::scoped_ptr<Display> m_display;
  boost::scoped_ptr<Registry> m_registry;
  boost::scoped_ptr<Compositor> m_compositor;
  boost::scoped_ptr<Shell> m_shell;

  /* Synchronization logic - these variables should not be touched
   * outside the scope of WaitForSynchronize() */
  bool synchronized;
  boost::scoped_ptr<Callback> synchronizeCallback;

  /* Synchronization entry point - call this function to issue a
   * wl_display.sync request to the server. All this does is cause
   * the server to send back an event that acknowledges the receipt
   * of the request. However, it is useful in a number of circumstances
   * - all request processing in wayland is sequential and guarunteed
   * to be in the same order as requests were made. That means that
   * once the event is received from the server, it is guarunteed
   * that all requests made prior to the sync request have finished
   * processing on the server and events have been issued to us. */
  void WaitForSynchronize();
  
  /* Callback function for synchronize event, do not call directly */
  void Synchronize();

  bool OnCompositorAvailable(struct wl_compositor *);
  bool OnShellAvailable(struct wl_shell *);
  bool OnSeatAvailable(struct wl_seat *);
  bool OnOutputAvailable(struct wl_output *);
};
}
}

namespace xw = xbmc::wayland;

/* Creating a new xbmc::wayland::XBMCConnection effectively creates
 * a new xbmc::wayland::Display object, which in turn will connect
 * to the running wayland compositor and encapsulate the return value
 * from the client library. Then it creates a new
 * xbmc::wayland::Registry object which is responsible for managing
 * all of the global objects on the wayland connection that we might
 * want to use. On creation of this object, a request is sent to
 * the compositor to send back an event for every available global
 * object. Once we know which objects exist, we can easily
 * bind to them.
 * 
 * The WaitForSynchronize call at the end of the constructor is
 * important. Once we make a request to the server for all of the
 * available global objects, we need to know what they all are
 * by the time this constructor finishes running so that the
 * object will be complete. The only way to do that is to know
 * when our wl_registry.add_listener request has finished processing
 * on both the server and client side
 */
xw::XBMCConnection::Private::Private(IDllWaylandClient &clientLibrary,
                                     IDllXKBCommon &xkbCommonLibrary,
                                     EventInjector &eventInjector) :
  m_clientLibrary(clientLibrary),
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_eventInjector(eventInjector),
  m_display(new xw::Display(clientLibrary)),
  m_registry(new xw::Registry(clientLibrary,
                              m_display->GetWlDisplay(),
                              *this))
{
  /* Tell CWinEvents what our display is. That way
   * CWinEvents::MessagePump is now able to dispatch events from
   * the display whenever it is called */ 
  (*m_eventInjector.setDisplay)(&clientLibrary,
                                m_display->GetWlDisplay());
	
  WaitForSynchronize();
}

xw::XBMCConnection::Private::~Private()
{
  (*m_eventInjector.destroyDisplay)();
}

xw::XBMCConnection::XBMCConnection(IDllWaylandClient &clientLibrary,
                                   IDllXKBCommon &xkbCommonLibrary,
                                   EventInjector &eventInjector) :
  priv(new Private (clientLibrary, xkbCommonLibrary, eventInjector))
{
}

/* A defined destructor is required such that
 * boost::scoped_ptr<Private>::~scoped_ptr is generated here */
xw::XBMCConnection::~XBMCConnection()
{
}

/* These are all registry callbacks. Once an object becomes available
 * (generally speaking at construction time) then we need to
 * create an internal representation for it so that we can use it
 * later */
bool xw::XBMCConnection::Private::OnCompositorAvailable(struct wl_compositor *c)
{
  m_compositor.reset(new xw::Compositor(m_clientLibrary, c));
  return true;
}

bool xw::XBMCConnection::Private::OnShellAvailable(struct wl_shell *s)
{
  m_shell.reset(new xw::Shell(m_clientLibrary, s));
  return true;
}

bool xw::XBMCConnection::Private::OnSeatAvailable(struct wl_seat *s)
{
  return true;
}

void xw::XBMCConnection::Private::WaitForSynchronize()
{
  boost::function<void(uint32_t)> func(boost::bind(&Private::Synchronize,
                                                   this));
  
  synchronized = false;
  synchronizeCallback.reset(new xw::Callback(m_clientLibrary,
                                             m_display->Sync(),
                                             func));
  while (!synchronized)
    (*m_eventInjector.messagePump)();
}

void xw::XBMCConnection::Private::Synchronize()
{
  synchronized = true;
  synchronizeCallback.reset();
}

void
xw::XBMCConnection::CurrentResolution(RESOLUTION_INFO &res) const
{
  res.iWidth = 640;
  res.iHeight = 480;
  res.fRefreshRate = 60;
  res.dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
  res.iScreen = 0;
  res.bFullScreen = true;
  res.iSubtitles = static_cast<int>(0.965 * res.iHeight);
  res.fPixelRatio = 1.0f;
  res.iScreenWidth = res.iWidth;
  res.iScreenHeight = res.iHeight;
  res.strMode.Format("%dx%d @ %.2fp",
                     res.iScreenWidth,
                     res.iScreenHeight,
                     res.fRefreshRate);
}

void
xw::XBMCConnection::PreferredResolution(RESOLUTION_INFO &res) const
{
  CurrentResolution(res);
}

void
xw::XBMCConnection::AvailableResolutions(std::vector<RESOLUTION_INFO> &res) const
{
  RESOLUTION_INFO resolution;
  CurrentResolution(resolution);
  res.push_back(resolution);
}

EGLNativeDisplayType *
xw::XBMCConnection::NativeDisplay() const
{
  return priv->m_display->GetEGLNativeDisplay();
}

const boost::scoped_ptr<xw::Compositor> &
xw::XBMCConnection::GetCompositor() const
{
  return priv->m_compositor;
}

const boost::scoped_ptr<xw::Shell> &
xw::XBMCConnection::GetShell() const
{
  return priv->m_shell;
}
