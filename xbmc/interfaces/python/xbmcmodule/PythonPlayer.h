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

#pragma once

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if (defined USE_EXTERNAL_PYTHON)
#include <Python.h>
#else
  #include "python/Include/Python.h"
#endif
#include "cores/IPlayer.h"


int Py_XBMC_Event_OnPlayBackStarted(void* arg);
int Py_XBMC_Event_OnPlayBackEnded(void* arg);
int Py_XBMC_Event_OnPlayBackStopped(void* arg);
int Py_XBMC_Event_OnPlayBackPaused(void* arg);
int Py_XBMC_Event_OnPlayBackResumed(void* arg);

class CPythonPlayer : public IPlayerCallback
{
public:
  CPythonPlayer();
  void    SetCallback(PyThreadState *state, PyObject *object);
  void    OnPlayBackEnded();
  void    OnPlayBackStarted();
  void    OnPlayBackPaused();
  void    OnPlayBackResumed();
  void    OnPlayBackStopped();
  void    OnQueueNextItem() {}; // unimplemented

  void    Acquire();
  void    Release();

  PyObject* m_callback;
  PyThreadState *m_state;
protected:
  virtual ~CPythonPlayer(void);
  long   m_refs;
};
