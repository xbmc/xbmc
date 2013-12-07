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

#include "windowing/DllWaylandClient.h"
#include "windowing/DllWaylandEgl.h"
#include "windowing/DllXKBCommon.h"

namespace xbmc
{
namespace wayland
{
template <class DllInterface, class Dll>
class AutoloadDll :
  boost::noncopyable
{
  public:

    AutoloadDll();
    ~AutoloadDll();
    DllInterface & Get();

  private:

    Dll m_dll;
};

class Libraries :
  boost::noncopyable
{
public:

  IDllWaylandClient & ClientLibrary();
  IDllWaylandEGL & EGLLibrary();
  IDllXKBCommon & XKBCommonLibrary();

private:

  AutoloadDll<IDllWaylandClient, DllWaylandClient> m_clientLibrary;
  AutoloadDll<IDllWaylandEGL, DllWaylandEGL> m_eglLibrary;
  AutoloadDll<IDllXKBCommon, DllXKBCommon> m_xkbCommonLibrary;
};

void LoadLibrary(DllDynamic &dll);

template <class DllInterface, class Dll>
AutoloadDll<DllInterface, Dll>::AutoloadDll()
{
  LoadLibrary(m_dll);
}

template <class DllInterface, class Dll>
DllInterface &
AutoloadDll<DllInterface, Dll>::Get()
{
  return m_dll;
}

template <class DllInterface, class Dll>
AutoloadDll<DllInterface, Dll>::~AutoloadDll()
{
  m_dll.Unload();
}
}
}
