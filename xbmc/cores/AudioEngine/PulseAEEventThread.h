#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "PulseAEStream.h"
#include "utils/Event.h"
#include "utils/Thread.h"
#include "utils/CriticalSection.h"

class CPulseAEStream;
class CPulseAEEventThread : public IRunnable
{
public:
  virtual void Run();

private:
  friend class CPulseAEStream;

  bool                 m_run;
  CPulseAEStream        *m_stream;
  CEvent               m_event;
  CThread              m_thread;
  IAEStream::AECBFunc *m_cbFunc;
  void                *m_arg;
  CCriticalSection     m_lock;
  CCriticalSection     m_lockEvent;

  CPulseAEEventThread(CPulseAEStream *stream);
  virtual ~CPulseAEEventThread();

  void Trigger();
  void SetCallback(IAEStream::AECBFunc *cbFunc, void *arg);
};

