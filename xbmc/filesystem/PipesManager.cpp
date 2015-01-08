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

#include "PipesManager.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

using namespace XFILE;


Pipe::Pipe(const std::string &name, int nMaxSize)
{
  m_buffer.Create(nMaxSize);
  m_nRefCount = 1;
  m_readEvent.Reset();
  m_writeEvent.Set();
  m_strPipeName = name;
  m_bOpen = true;
  m_bEof = false;
  m_nOpenThreashold = PIPE_DEFAULT_MAX_SIZE / 2;
  m_bReadyForRead = true; // open threashold disabled atm
}

Pipe::~Pipe()
{
}

void Pipe::SetOpenThreashold(int threashold)
{
  m_nOpenThreashold = threashold;
}

const std::string &Pipe::GetName()
{
  return m_strPipeName;
}

void Pipe::AddRef()
{
  CSingleLock lock(m_lock);
  m_nRefCount++;
}

void Pipe::DecRef()
{
  CSingleLock lock(m_lock);
  m_nRefCount--;
}

int  Pipe::RefCount()
{
  CSingleLock lock(m_lock);
  return m_nRefCount;
}

void Pipe::SetEof()
{
  m_bEof = true;
}

bool Pipe::IsEof()
{
  return m_bEof;
}

bool Pipe::IsEmpty()
{
  return (m_buffer.getMaxReadSize() == 0);
}

void Pipe::Flush()
{
  CSingleLock lock(m_lock);

  if (!m_bOpen || !m_bReadyForRead || m_bEof)
  {
    return;
  }
  m_buffer.Clear();
  CheckStatus();
}

int  Pipe::Read(char *buf, int nMaxSize, int nWaitMillis)
{
  CSingleLock lock(m_lock);
  
  if (!m_bOpen)
  {
    return -1;
  }

  while (!m_bReadyForRead && !m_bEof)
    m_readEvent.WaitMSec(100);

  int nResult = 0;
  if (!IsEmpty())
  {
    int nToRead = min((int)m_buffer.getMaxReadSize(), nMaxSize);
    m_buffer.ReadData(buf, nToRead);
    nResult = nToRead;
  }
  else if (m_bEof)
  {
    nResult = 0;
  }
  else
  {
    // we're leaving the guard - add ref to make sure we are not getting erased.
    // at the moment we leave m_listeners unprotected which might be a problem in future
    // but as long as we only have 1 listener attaching at startup and detaching on close we're fine
    AddRef();
    lock.Leave();

    bool bHasData = false;
    int nMillisLeft = nWaitMillis;
    if (nMillisLeft < 0)
      nMillisLeft = 5*60*1000; // arbitrary. 5 min.

    do
    {
      for (size_t l=0; l<m_listeners.size(); l++)
        m_listeners[l]->OnPipeUnderFlow();

      bHasData = m_readEvent.WaitMSec(min(200,nMillisLeft));
      nMillisLeft -= 200;
    } while (!bHasData && nMillisLeft > 0 && !m_bEof);

    lock.Enter();
    DecRef();
    
    if (!m_bOpen)
      return -1;
    
    if (bHasData)
    {
      int nToRead = min((int)m_buffer.getMaxReadSize(), nMaxSize);
      m_buffer.ReadData(buf, nToRead);
      nResult = nToRead;
    }
  }
  
  CheckStatus();
  
  return nResult;
}

bool Pipe::Write(const char *buf, int nSize, int nWaitMillis)
{
  CSingleLock lock(m_lock);
  if (!m_bOpen)
    return false;
  bool bOk = false;
  int writeSize = m_buffer.getMaxWriteSize();
  if (writeSize > nSize)
  {
    m_buffer.WriteData((char*)buf, nSize);
    bOk = true;
  }
  else
  {
    while ( (int)m_buffer.getMaxWriteSize() < nSize && m_bOpen )
    {
      lock.Leave();
      for (size_t l=0; l<m_listeners.size(); l++)
        m_listeners[l]->OnPipeOverFlow();

      bool bClear = nWaitMillis < 0 ? m_writeEvent.Wait() : m_writeEvent.WaitMSec(nWaitMillis);
      lock.Enter();
      if (bClear && (int)m_buffer.getMaxWriteSize() >= nSize)
      {
        m_buffer.WriteData((char*)buf, nSize);
        bOk = true;
        break;
      }

      // FIXME: is this right? Shouldn't we see if the time limit has been reached?
      if (nWaitMillis > 0)
        break;
    }
  }

  CheckStatus();
  
  return bOk && m_bOpen;
}

void Pipe::CheckStatus()
{
  if (m_bEof)
  {
    m_writeEvent.Set();
    m_readEvent.Set();  
    return;
  }
  
  if (m_buffer.getMaxWriteSize() == 0)
    m_writeEvent.Reset();
  else
    m_writeEvent.Set();
  
  if (m_buffer.getMaxReadSize() == 0)
    m_readEvent.Reset();
  else
  {
    if (!m_bReadyForRead  && (int)m_buffer.getMaxReadSize() >= m_nOpenThreashold)
      m_bReadyForRead = true;
    m_readEvent.Set();  
  }
}

void Pipe::Close()
{
  CSingleLock lock(m_lock);
  m_bOpen = false;
  m_readEvent.Set();
  m_writeEvent.Set();
}

void Pipe::AddListener(IPipeListener *l)
{
  CSingleLock lock(m_lock);
  for (size_t i=0; i<m_listeners.size(); i++)
  {
    if (m_listeners[i] == l)
      return;
  }
  m_listeners.push_back(l);
}

void Pipe::RemoveListener(IPipeListener *l)
{
  CSingleLock lock(m_lock);
  std::vector<XFILE::IPipeListener *>::iterator i = m_listeners.begin();
  while(i != m_listeners.end())
  {
    if ( (*i) == l)
      i = m_listeners.erase(i);
    else
      ++i;
  }
}

int	Pipe::GetAvailableRead()
{
  CSingleLock lock(m_lock);
  return m_buffer.getMaxReadSize();
}

PipesManager::PipesManager() : m_nGenIdHelper(1)
{
}

PipesManager::~PipesManager()
{
}

PipesManager &PipesManager::GetInstance()
{
  static PipesManager instance;
  return instance;
}

std::string   PipesManager::GetUniquePipeName()
{
  CSingleLock lock(m_lock);
  return StringUtils::Format("pipe://%d/", m_nGenIdHelper++);
}

XFILE::Pipe *PipesManager::CreatePipe(const std::string &name, int nMaxPipeSize)
{
  std::string pName = name;
  if (pName.empty())
    pName = GetUniquePipeName();
  
  CSingleLock lock(m_lock);
  if (m_pipes.find(pName) != m_pipes.end())
    return NULL;
  
  XFILE::Pipe *p = new XFILE::Pipe(pName, nMaxPipeSize);
  m_pipes[pName] = p;
  return p;
}

XFILE::Pipe *PipesManager::OpenPipe(const std::string &name)
{
  CSingleLock lock(m_lock);
  if (m_pipes.find(name) == m_pipes.end())
    return NULL;
  m_pipes[name]->AddRef();
  return m_pipes[name];
}

void         PipesManager::ClosePipe(XFILE::Pipe *pipe)
{
  CSingleLock lock(m_lock);
  if (!pipe)
    return ;
  
  pipe->DecRef();
  if (pipe->RefCount() == 0)
  {
    pipe->Close();
    m_pipes.erase(pipe->GetName());
    delete pipe;
  }
}

bool         PipesManager::Exists(const std::string &name)
{
  CSingleLock lock(m_lock);
  return (m_pipes.find(name) != m_pipes.end());
}

