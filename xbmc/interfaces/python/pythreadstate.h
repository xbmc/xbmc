/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once


//WARNING: since this will unlock/lock the python global interpreter lock,
//         it will not work recursively

//this is basically a scoped version of a Py_BEGIN_ALLOW_THREADS .. Py_END_ALLOW_THREADS block
class CPyThreadState
{
  public:
    explicit CPyThreadState(bool save = true)
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
 * A std::unique_lock<CCriticalSection> that will relinquish the GIL during the time
 *  it takes to obtain the CriticalSection
 */
class GilSafeSingleLock : public CPyThreadState, public std::unique_lock<CCriticalSection>
{
public:
  explicit GilSafeSingleLock(CCriticalSection& critSec)
    : CPyThreadState(true), std::unique_lock<CCriticalSection>(critSec)
  {
    CPyThreadState::Restore();
  }
};

