#ifndef __COREAUDIOAEEVENTTHREAD_H__
#define __COREAUDIOAEEVENTTHREAD_H__
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

#include "CoreAudioAE.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"

class CPulseAEStream;
class CCoreAudioAEEventThread : public IRunnable
{
public:
  virtual void Run();
  CCoreAudioAEEventThread(CCoreAudioAE *engine);
  virtual ~CCoreAudioAEEventThread();
  void Trigger();

private:
  bool                 m_run;
  CCoreAudioAE        *m_engine;
  CEvent               m_event;
  CThread              m_thread;
  CCriticalSection     m_lock;
  CCriticalSection     m_lockEvent;

};

#endif
