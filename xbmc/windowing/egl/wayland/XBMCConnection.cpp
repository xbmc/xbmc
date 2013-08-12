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
#include <boost/shared_ptr.hpp>

#include <wayland-client.h>

#include "guilib/Resolution.h"
#include "guilib/gui3d.h"

#include "windowing/DllWaylandClient.h"
#include "windowing/DllXKBCommon.h"

#include "Callback.h"
#include "Compositor.h"
#include "Display.h"
#include "Output.h"
#include "Registry.h"
#include "Region.h"
#include "Shell.h"

#include "windowing/WaylandProtocol.h"
#include "XBMCConnection.h"

#include "windowing/wayland/Wayland11EventQueueStrategy.h"
#include "windowing/wayland/Wayland12EventQueueStrategy.h"

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
  
  EventInjector m_eventInjector;

  boost::scoped_ptr<Display> m_display;
  boost::scoped_ptr<Registry> m_registry;
  boost::scoped_ptr<Compositor> m_compositor;
  boost::scoped_ptr<Shell> m_shell;
  
  std::vector<boost::shared_ptr<Output> > m_outputs;

  /* Synchronization logic - these variables should not be touched
   * outside the scope of WaitForSynchronize() */
  bool synchronized;
  boost::scoped_ptr<Callback> synchronizeCallback;
  
  boost::scoped_ptr<events::IEventQueueStrategy> m_eventQueue;

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
  
  /* Do not call this from a non-main thread. The main thread may be
   * waiting for a wl_display.sync event to be coming through and this
   * function will merely spin until synchronized == true, for which
   * a non-main thread may be responsible for setting as true */
  void Synchronize();

  bool OnCompositorAvailable(struct wl_compositor *);
  bool OnShellAvailable(struct wl_shell *);
  bool OnSeatAvailable(struct wl_seat *);
  bool OnOutputAvailable(struct wl_output *);
};
}
}

namespace xw = xbmc::wayland;
namespace xwe = xbmc::wayland::events;

namespace
{
/* A deficiency in the client library in wayland versions prior to
 * 1.2 means that there is divergent behaviour between versions here
 * and this is explicitly expressed and encapsulated in these two
 * strategies.
 * 
 * Because xbmc uses a game-loop, it is expected that no operation
 * should block the main thread. This includes any operations to
 * read the window system event queue. The main thread might be blocked
 * for a prolonged period in the situation where the main xbmc surface
 * is not visible, because the screen lock is active or another
 * surface is obstructing it. When the main thread becomes blocked,
 * it means that xbmc isn't able to start or stop any background jobs,
 * which could interrupt library updates which occurr on idle or
 * other such operations.
 * 
 * However, wayland versions prior to 1.2 had the expectation that
 * clients expected to block if there were no incoming compositor
 * events because it is part of wayland's design that the compositor
 * is responsible for sending the events to drive a client's render
 * and input loop. As such, on wayland <= 1.1, the expectation is that
 * compositor event read and dispatch occurrs in the same thread and
 * on wayland >= 1.2 the expectation is that these operations can
 * occurr in multiple threads.
 * 
 * The following table illustrates these differences:
 * 
 * ---------------------------------------------------------------------
 * | Wayland | Thread that  | Thread that | Thread that   | Strategy   |
 * | Version | Reads happen | wrappers    | flush happens | Object     |
 * |         | in           | operate in  |               |            |
 * |         |              | in          |               |            |
 * ---------------------------------------------------------------------
 * | <= 1.1  | Poll Thread  | Poll Thread | Main Thread   | xw::versio-|
 * |         |              |             |               | n11::Event-|
 * |         |              |             |               | QueueStrat-|
 * |         |              |             |               | egy        |
 * ---------------------------------------------------------------------
 * | >= 1.2  | Poll Thread  | Main Thread | Main Thread   | xw::versio-|
 * |         |              |             |               | n12::Event-|
 * |         |              |             |               | QueueStrat-|
 * |         |              |             |               | egy        |
 * ---------------------------------------------------------------------
 * 
 * The reason why it is different between the two versions it that it
 * is generally desirable that the operation of all the wrapper objects
 * occurr in the main thread, because there's less overhead in having
 * to allocate temporary storage for their results in a queue so that
 * they can be re-dispatched later. The plan is to eventually deprecate
 * and remove support for wayland versions <= 1.1.
 */
xwe::IEventQueueStrategy *
EventQueueForClientVersion(IDllWaylandClient &clientLibrary,
                           struct wl_display *display)
{
  /* TODO: Test for wl_display_read_events / wl_display_prepare_read */
  const bool version12 =
    clientLibrary.wl_display_read_events_proc() &&
    clientLibrary.wl_display_prepare_read_proc();
  if (version12)
    return new xw::version_12::EventQueueStrategy(clientLibrary,
                                                  display);
  else
    return new xw::version_11::EventQueueStrategy(clientLibrary,
                                                  display);
}
}

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
                              *this)),
  m_eventQueue(EventQueueForClientVersion(m_clientLibrary,
                                          m_display->GetWlDisplay()))
{
  /* Tell CWinEvents what our display is. That way
   * CWinEvents::MessagePump is now able to dispatch events from
   * the display whenever it is called */ 
  (*m_eventInjector.setDisplay)(clientLibrary,
                                *(m_eventQueue.get()),
                                m_display->GetWlDisplay());
	
  /* Wait once for the globals to appear and then once
   * for the globals to be initialized (i.e, outputs) */
  unsigned int waitCount = 2;
  while (waitCount--)
    WaitForSynchronize();

  if (m_outputs.empty())
  {
    std::stringstream ss;
    throw std::runtime_error(ss.str());
  }
}

xw::XBMCConnection::Private::~Private()
{
  (*m_eventInjector.destroyWaylandSeat)();
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
  /* When the seat becomes available and bound, let CWinEventsWayland
   * know about it so that it can wrap it and query it for more
   * information about input devices */
  (*m_eventInjector.setWaylandSeat)(m_clientLibrary,
                                    m_xkbCommonLibrary,
                                    s);
  return true;
}

bool xw::XBMCConnection::Private::OnOutputAvailable(struct wl_output *o)
{
  /* Even though wl_output is a global interface, there can be multiple
   * outputs available, so we put them into a list */
  m_outputs.push_back(boost::shared_ptr<xw::Output>(new xw::Output(m_clientLibrary,
                                                                   o)));

  /* It is the responsibility of the caller to ensure that we have
   * recieved all the events telling us the output modes by
   * calling WaitForSynchronize() in the main thread */
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

  /* For version 1.1 event queues the effect of this is going to be
   * a spin-wait. That's not exactly ideal, but we do need to
   * continuously flush the event queue */
  while (!synchronized)
    (*m_eventInjector.messagePump)();
}

void xw::XBMCConnection::Private::Synchronize()
{
  synchronized = true;
  synchronizeCallback.reset();
}

namespace
{
void ResolutionInfoForMode(const xw::Output::ModeGeometry &mode,
                           RESOLUTION_INFO &res)
{
  res.iWidth = mode.width;
  res.iHeight = mode.height;
  
  /* The refresh rate is given as an integer in the second exponent
   * so we need to divide by 100.0f to get a floating point value */
  res.fRefreshRate = mode.refresh / 100.0f;
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
}

void
xw::XBMCConnection::CurrentResolution(RESOLUTION_INFO &res) const
{
  /* Supporting only the first output device at the moment */
  const xw::Output::ModeGeometry &current(priv->m_outputs[0]->CurrentMode());
  
  ResolutionInfoForMode(current, res);
}

void
xw::XBMCConnection::PreferredResolution(RESOLUTION_INFO &res) const
{
  /* Supporting only the first output device at the moment */
  const xw::Output::ModeGeometry &preferred(priv->m_outputs[0]->PreferredMode());
  ResolutionInfoForMode(preferred, res);
}

void
xw::XBMCConnection::AvailableResolutions(std::vector<RESOLUTION_INFO> &resolutions) const
{
  /* Supporting only the first output device at the moment */
  const boost::shared_ptr <xw::Output> &output(priv->m_outputs[0]);
  const std::vector<xw::Output::ModeGeometry> &m_modes(output->AllModes());

  for (std::vector<xw::Output::ModeGeometry>::const_iterator it = m_modes.begin();
       it != m_modes.end();
       ++it)
  {
    resolutions.push_back(RESOLUTION_INFO());
    RESOLUTION_INFO &back(resolutions.back());
    
    ResolutionInfoForMode(*it, back);
  }
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

const boost::shared_ptr<xw::Output> &
xw::XBMCConnection::GetFirstOutput() const
{
  return priv->m_outputs[0];
}
