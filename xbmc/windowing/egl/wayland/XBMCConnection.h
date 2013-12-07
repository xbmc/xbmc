#pragma once

/*
 *      Copyright (C) 2011-2013 Team XBMC
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
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

class IDllWaylandClient;
class IDllXKBCommon;

struct wl_compositor;
struct wl_display;
struct wl_output;
struct wl_shell;
struct wl_seat;

typedef struct wl_display * EGLNativeDisplayType;

struct RESOLUTION_INFO;

namespace xbmc
{
namespace wayland
{
class Compositor;
class Output;
class Shell;

namespace events
{
class IEventQueueStrategy;
}

class XBMCConnection
{
public:

  struct EventInjector
  {
    typedef void (*SetEventQueue)(events::IEventQueueStrategy &strategy);
    typedef void (*DestroyEventQueue)();
    typedef void (*SetWaylandSeat)(IDllWaylandClient &clientLibrary,
                                   IDllXKBCommon &xkbCommonLibrary,
                                   struct wl_seat *seat);
    typedef void (*DestroyWaylandSeat)();
    typedef bool (*MessagePump)();
    
    SetEventQueue setEventQueue;
    DestroyEventQueue destroyEventQueue;
    SetWaylandSeat setWaylandSeat;
    DestroyWaylandSeat destroyWaylandSeat;
    MessagePump messagePump;
  };

  XBMCConnection(IDllWaylandClient &clientLibrary,
                 IDllXKBCommon &xkbCommonLibrary,
                 EventInjector &injector);
  ~XBMCConnection();
  
  void PreferredResolution(RESOLUTION_INFO &res) const;
  void CurrentResolution(RESOLUTION_INFO &res) const;
  void AvailableResolutions(std::vector<RESOLUTION_INFO> &res) const;
  
  EGLNativeDisplayType * NativeDisplay() const;
  
  Compositor & GetCompositor();
  Shell & GetShell();
  Output & GetFirstOutput();
  
private:

  class Private;
  boost::scoped_ptr<Private> priv;
};
}
}
