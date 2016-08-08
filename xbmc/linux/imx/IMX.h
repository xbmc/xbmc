#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "guilib/DispResource.h"
#include "utils/log.h"

#include <mutex>
#include <queue>
#include <condition_variable>
#include <algorithm>
#include <atomic>
#include <thread>
#include <map>

#define DIFFRINGSIZE 60

class CIMX;
extern CIMX g_IMX;

class CIMX : public CThread, IDispResource
{
public:
  CIMX(void);
  ~CIMX(void);

  bool          Initialize();
  void          Deinitialize();

  int           WaitVsync();
  virtual void  OnResetDisplay();

private:
  virtual void  Process();
  bool          UpdateDCIC();

  int           m_fddcic;
  bool          m_change;
  unsigned long m_counter;
  unsigned long m_counterLast;
  CEvent        m_VblankEvent;

  double        m_frameTime;
  CCriticalSection m_critSection;

  uint32_t      m_lastSyncFlag;
};

// A blocking FIFO buffer
template <typename T>
class lkFIFO
{
public:
  lkFIFO() { m_size = queue.max_size(); queue.clear(); m_abort = false; }

public:
  T pop()
  {
    std::unique_lock<std::mutex> m_lock(lkqueue);
    m_abort = false;
    while (!queue.size() && !m_abort)
      read.wait(m_lock);

    T val;
    if (!queue.empty())
    {
      val = queue.front();
      queue.pop_front();
    }

    m_lock.unlock();
    write.notify_one();
    return val;
  }

  bool push(const T& item)
  {
    std::unique_lock<std::mutex> m_lock(lkqueue);
    m_abort = false;
    while (queue.size() >= m_size && !m_abort)
      write.wait(m_lock);

    if (m_abort)
      return false;

    queue.push_back(item);
    m_lock.unlock();
    read.notify_one();
    return true;
  }

  void signal()
  {
    m_abort = true;
    read.notify_one();
    write.notify_one();
  }

  void setquotasize(size_t newsize)
  {
    m_size = newsize;
    write.notify_one();
  }

  size_t getquotasize()
  {
    return m_size;
  }

  void for_each(void (*fn)(T &t), bool clear = true)
  {
    std::unique_lock<std::mutex> m_lock(lkqueue);
    std::for_each(queue.begin(), queue.end(), fn);

    if (clear)
      queue.clear();

    write.notify_one();
  }

  size_t size()
  {
    return queue.size();
  }

  void clear()
  {
    std::unique_lock<std::mutex> m_lock(lkqueue);
    queue.clear();
    write.notify_one();
  }

  bool full()  { return queue.size() >= m_size; }

private:
  std::deque<T>           queue;
  std::mutex              lkqueue;
  std::condition_variable write;
  std::condition_variable read;

  size_t                  m_size;
  volatile bool           m_abort;
};

// Generell description of a buffer used by
// the IMX context, e.g. for blitting
class CIMXBuffer {
public:
  CIMXBuffer() : m_iRefs(0) {}

  // Shared pointer interface
  virtual void Lock() = 0;
  virtual long Release() = 0;

  int          GetFormat()  { return iFormat; }

public:
  uint32_t     iWidth;
  uint32_t     iHeight;
  int          pPhysAddr;
  uint8_t     *pVirtAddr;
  int          iFormat;
  double       m_fps;

protected:
  std::atomic<long> m_iRefs;
};

class CIMXFps
{
  public:
    CIMXFps()       { Flush(); }
    void   Add(double pts);
    void   Flush(); //flush the saved pattern and the ringbuffer

    double GetFrameDuration() { return m_frameDuration;             }
    bool   HasFullBuffer()    { return m_ts.size() == DIFFRINGSIZE; }

    bool   Recalc();

  private:
    std::deque<double>   m_ts;
    std::map<double,int> m_hgraph;
    double               m_frameDuration;
    bool                 m_hasPattern;
    double               m_ptscorrection;
};
