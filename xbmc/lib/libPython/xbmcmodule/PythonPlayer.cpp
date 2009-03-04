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

#include "stdafx.h"
#include "pyutil.h"
#include "PythonPlayer.h"
#include "lib/libPython/XBPython.h"

using namespace PYXBMC;

CPythonPlayer::CPythonPlayer()
{
  pCallback = NULL;
}

CPythonPlayer::~CPythonPlayer(void)
{
  g_pythonParser.UnregisterPythonPlayerCallBack(this);
}

void CPythonPlayer::OnPlayBackStarted()
{
  // aquire lock?
  Py_INCREF(pCallback);
  Py_AddPendingCall(Py_XBMC_Event_OnPlayBackStarted, pCallback);
  g_pythonParser.PulseGlobalEvent();
}

void CPythonPlayer::OnPlayBackEnded()
{
  // aquire lock?
  Py_INCREF(pCallback);
  Py_AddPendingCall(Py_XBMC_Event_OnPlayBackEnded, pCallback);
  g_pythonParser.PulseGlobalEvent();
}

void CPythonPlayer::OnPlayBackStopped()
{
  // aquire lock?
  Py_INCREF(pCallback);
  Py_AddPendingCall(Py_XBMC_Event_OnPlayBackStopped, pCallback);
  g_pythonParser.PulseGlobalEvent();
}

void CPythonPlayer::SetCallback(PyObject *object)
{
  pCallback = object;
  g_pythonParser.RegisterPythonPlayerCallBack(this);
}

/*
 * called from python library!
 */
int Py_XBMC_Event_OnPlayBackStarted(void* playerObject)
{
  if (playerObject != NULL) PyObject_CallMethod((PyObject*)playerObject, "onPlayBackStarted", NULL);
  Py_DECREF((PyObject*)playerObject);  
  return 0;
}

/*
 * called from python library!
 */
int Py_XBMC_Event_OnPlayBackEnded(void* playerObject)
{
  if (playerObject != NULL) PyObject_CallMethod((PyObject*)playerObject, "onPlayBackEnded", NULL);
  Py_DECREF((PyObject*)playerObject);
  return 0;
}

/*
 * called from python library!
 */
int Py_XBMC_Event_OnPlayBackStopped(void* playerObject)
{
  if (playerObject != NULL) PyObject_CallMethod((PyObject*)playerObject, "onPlayBackStopped", NULL );
  Py_DECREF((PyObject*)playerObject);
  return 0;
}

