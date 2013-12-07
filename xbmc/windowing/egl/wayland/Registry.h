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
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>

#include <wayland-client.h>

#include "windowing/WaylandProtocol.h"

class IDllWaylandClient;

namespace xbmc
{
namespace wayland
{
/* This is effectively just a seam for testing purposes so that
 * we can listen for extra objects that the core implementation might
 * not necessarily be interested in */
class ExtraWaylandGlobals
{
public:

  typedef boost::function<void(struct wl_registry *,
                               uint32_t,
                               const char *,
                               uint32_t)> GlobalHandler;
  
  void SetHandler(const GlobalHandler &);
  void NewGlobal(struct wl_registry *,
                 uint32_t,
                 const char *,
                 uint32_t);

  static ExtraWaylandGlobals & GetInstance();
private:

  GlobalHandler m_handler;
  
  static boost::scoped_ptr<ExtraWaylandGlobals> m_instance;
};

class IWaylandRegistration
{
public:

  virtual ~IWaylandRegistration() {};

  virtual bool OnGlobalInterfaceAvailable(uint32_t,
                                          const char *,
                                          uint32_t) = 0;
};

class Registry :
  boost::noncopyable
{
public:

  Registry(IDllWaylandClient &clientLibrary,
           struct wl_display   *display,
           IWaylandRegistration &registration);
  ~Registry();

  struct wl_registry * GetWlRegistry();
  
  template<typename Create>
  Create Bind(uint32_t name,
              struct wl_interface **interface,
              uint32_t version)
  {
    Create object =
      protocol::CreateWaylandObject<Create,
                                    struct wl_registry *>(m_clientLibrary,
                                                          m_registry,
                                                          interface);

    /* This looks a bit funky - but it is correct. The dll returns
     * a ** to wl_interface when it is in fact just a pointer to
     * the static variable, so we need to remove one indirection */
    BindInternal(name,
                 reinterpret_cast<struct wl_interface *>(interface)->name,
                 version,
                 object);
    return object;
  }

private:

  static const struct wl_registry_listener m_listener;

  static void HandleGlobalCallback(void *, struct wl_registry *,
                                   uint32_t, const char *, uint32_t);
  static void HandleRemoveGlobalCallback(void *, struct wl_registry *,
                                         uint32_t name);

  void BindInternal(uint32_t name,
                    const char *interface,
                    uint32_t version,
                    void *proxy);

  IDllWaylandClient &m_clientLibrary;
  struct wl_registry *m_registry;
  IWaylandRegistration &m_registration;

  void HandleGlobal(uint32_t, const char *, uint32_t);
  void HandleRemoveGlobal(uint32_t);
};
}
}
