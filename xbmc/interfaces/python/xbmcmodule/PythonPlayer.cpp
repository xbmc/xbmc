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

#include "pyutil.h"
#include "PythonPlayer.h"
#include "pythreadstate.h"
#include "../XBPython.h"
#include "threads/Atomics.h"

using namespace PYXBMC;

struct SPyEvent
{
  SPyEvent(CPythonPlayer *player, const char *function, std::vector<int> &params=std::vector<int>())
  {
    m_player   = player;
    m_player->Acquire();
    m_function = function;
    m_params = params;
  }

  ~SPyEvent()
  {
    m_player->Release();
  }

  const char*    m_function;
  CPythonPlayer* m_player;
  std::vector<int> m_params;

};

/*
 * called from python library!
 */
static int SPyEvent_Function(void* e)
{
  SPyEvent* object = (SPyEvent*)e;
  PyObject* ret    = NULL;

  if (object->m_player->m_callback)
   {
    if (object->m_params.size() == 2)
      ret = PyObject_CallMethod(object->m_player->m_callback, (char*)object->m_function, "ii", object->m_params[0], object->m_params[1]);
    else if (object->m_params.size() == 1)
      ret = PyObject_CallMethod(object->m_player->m_callback, (char*)object->m_function, "i", object->m_params[0]);
    else
      ret = PyObject_CallMethod(object->m_player->m_callback, (char*)object->m_function, NULL);
   }

  if(ret)
  {
    Py_DECREF(ret);
  }

  CPyThreadState pyState;
  delete object;

  return 0;

}

CPythonPlayer::CPythonPlayer()
{
  m_callback = NULL;
  m_refs     = 1;
  g_pythonParser.RegisterPythonPlayerCallBack(this);
}

void CPythonPlayer::Release()
{
  if(AtomicDecrement(&m_refs) == 0)
    delete this;
}

void CPythonPlayer::Acquire()
{
  AtomicIncrement(&m_refs);
}

CPythonPlayer::~CPythonPlayer(void)
{
  g_pythonParser.UnregisterPythonPlayerCallBack(this);
}

void CPythonPlayer::OnPlayBackStarted()
{
  PyXBMC_AddPendingCall(m_state, SPyEvent_Function, new SPyEvent(this, "onPlayBackStarted"));
  g_pythonParser.PulseGlobalEvent();
}

void CPythonPlayer::OnPlayBackEnded()
{
  PyXBMC_AddPendingCall(m_state, SPyEvent_Function, new SPyEvent(this, "onPlayBackEnded"));
  g_pythonParser.PulseGlobalEvent();
}

void CPythonPlayer::OnPlayBackStopped()
{
  PyXBMC_AddPendingCall(m_state, SPyEvent_Function, new SPyEvent(this, "onPlayBackStopped"));
  g_pythonParser.PulseGlobalEvent();
}

void CPythonPlayer::OnPlayBackPaused()
{
  PyXBMC_AddPendingCall(m_state, SPyEvent_Function, new SPyEvent(this, "onPlayBackPaused"));
  g_pythonParser.PulseGlobalEvent();
}

void CPythonPlayer::OnPlayBackResumed()
{
  PyXBMC_AddPendingCall(m_state, SPyEvent_Function, new SPyEvent(this, "onPlayBackResumed"));
  g_pythonParser.PulseGlobalEvent();
}

void CPythonPlayer::SetCallback(PyThreadState *state, PyObject *object)
{
  /* python lock should be held */
  m_callback = object;
  m_state    = state;
}
