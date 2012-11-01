#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "threads/SingleLock.h"

//WARNING: since this will unlock/lock the python global interpreter lock,
//         it will not work recursively

//this is basically a scoped version of a Py_BEGIN_ALLOW_THREADS .. Py_END_ALLOW_THREADS block
class CPyThreadState
{
  public:
    CPyThreadState(bool save = true)
    {
      m_threadState = NULL;

      if (save)
        Save();
    }

    ~CPyThreadState()
    {
      Restore();
    }

    void Save()
    {
      if (!m_threadState)
        m_threadState = PyEval_SaveThread(); //same as Py_BEGIN_ALLOW_THREADS
    }

    void Restore()
    {
      if (m_threadState)
      {
        PyEval_RestoreThread(m_threadState); //same as Py_END_ALLOW_THREADS
        m_threadState = NULL;
      }
    }

  private:
    PyThreadState* m_threadState;
};

/**
 * A CSingleLock that will relinquish the GIL during the time
 *  it takes to obtain the CriticalSection
 */
class GilSafeSingleLock : public CPyThreadState, public CSingleLock
{
public:
  GilSafeSingleLock(const CCriticalSection& critSec) : CPyThreadState(true), CSingleLock(critSec) { CPyThreadState::Restore(); }
};

