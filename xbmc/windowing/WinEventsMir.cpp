/*
*      Copyright (C) 2005-2013 Team XBMC
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

#if defined (HAVE_MIR)

#include <algorithm>
#include <sstream>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scope_exit.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <sys/poll.h>
#include <sys/mman.h>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#include "Application.h"
#include "WindowingFactory.h"
#include "WinEvents.h"
#include "WinEventsMir.h"
#include "utils/Stopwatch.h"
#include "utils/log.h"

#include "DllMirClient.h"
#include "DllXKBCommon.h"

namespace xbmc
{
namespace mir
{
class IEventListener
{
public:

  virtual ~IEventListener() {}
  virtual bool OnEvent(XBMC_Event &) = 0;
  virtual bool OnFocused() = 0;
  virtual bool OnUnfocused() = 0;
};

class EventDispatch :
  public IEventListener
{
public:

  bool OnEvent(XBMC_Event &);
  bool OnFocused();
  bool OnUnfocused();
};
}
}

namespace xm = xbmc::mir;

namespace
{
class MirInput
{
public:

  MirInput(IDllMirClient &clientLibrary,
           IDllXKBCommon &xkbCommonLibrary,
           xm::IEventPipe &eventPipe);

  void PumpEvents();

private:

  IDllMirClient &m_clientLibrary;
  IDllXKBCommon &m_xkbCommonLibrary;
  xm::IEventPipe &m_eventPipe;
};

boost::scoped_ptr<MirInput> g_mirInput;
}

MirInput::MirInput(IDllMirClient &clientLibrary,
                   IDllXKBCommon &xkbCommonLibrary,
                   xm::IEventPipe &eventPipe) :
  m_clientLibrary(clientLibrary),
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_eventPipe(eventPipe)
{
}

void MirInput::PumpEvents()
{
  m_eventPipe.Pump();
}

CWinEventsMir::CWinEventsMir()
{
}

void CWinEventsMir::RefreshDevices()
{
}

bool CWinEventsMir::IsRemoteLowBattery()
{
  return false;
}

bool CWinEventsMir::MessagePump()
{
  if (g_mirInput.get())
  {
    g_mirInput->PumpEvents();
    return true;
  }
  
  return false;
}

void CWinEventsMir::SetEventPipe(IDllMirClient &clientLibrary,
                                 IDllXKBCommon &xkbCommonLibrary,
                                 xm::IEventPipe &eventPipe)
{
  g_mirInput.reset(new MirInput(clientLibrary,
                                xkbCommonLibrary,
                                eventPipe));
}

void CWinEventsMir::RemoveEventPipe()
{
  g_mirInput.reset();
}

#endif
