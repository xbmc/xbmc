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

#include "PipeFile.h"
#include "threads/SingleLock.h"
#include "PipesManager.h"
#include "URL.h"

using namespace XFILE;

CPipeFile::CPipeFile() : m_pos(0), m_length(-1), m_pipe(NULL)
{
}

CPipeFile::~CPipeFile()
{
  Close();
}

int64_t CPipeFile::GetPosition()
{
  return m_pos;
}

int64_t CPipeFile::GetLength()
{
  return m_length;
}

void CPipeFile::SetLength(int64_t len)
{
  m_length = len;
}

bool CPipeFile::Open(const CURL& url)
{
  std::string name = url.Get();
  m_pipe = PipesManager::GetInstance().OpenPipe(name);
  if (m_pipe)
    m_pipe->AddListener(this);
  return (m_pipe != NULL);
}

bool CPipeFile::Exists(const CURL& url)
{
  std::string name = url.Get();
  return PipesManager::GetInstance().Exists(name);
}

int CPipeFile::Stat(const CURL& url, struct __stat64* buffer)
{
  return -1;
}

int CPipeFile::Stat(struct __stat64* buffer)
{
  memset(buffer,0,sizeof(struct __stat64));
  buffer->st_size = m_length;
  return 0;
}

ssize_t CPipeFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (!m_pipe)
    return -1;
  
  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  return m_pipe->Read((char *)lpBuf,(int)uiBufSize);
}

ssize_t CPipeFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (!m_pipe)
    return -1;
  
  // m_pipe->Write return bool. either all was written or not.
  return m_pipe->Write((const char *)lpBuf,uiBufSize) ? uiBufSize : -1;
}

void CPipeFile::SetEof()
{
  if (!m_pipe)
    return ;
  m_pipe->SetEof();
}

bool CPipeFile::IsEof()
{
  if (!m_pipe)
    return true;
  return m_pipe->IsEof();
}

bool CPipeFile::IsEmpty()
{
  if (!m_pipe)
    return true;
  return m_pipe->IsEmpty();
}

int64_t CPipeFile::Seek(int64_t iFilePosition, int iWhence)
{
  return -1;
}

void CPipeFile::Close()
{
  if (m_pipe)
  {
    m_pipe->RemoveListener(this);
    PipesManager::GetInstance().ClosePipe(m_pipe);    
  }
  m_pipe = NULL;
}

bool CPipeFile::IsClosed()
{
  return (m_pipe == NULL);
}

void CPipeFile::Flush()
{
  if (m_pipe)
    m_pipe->Flush();
}

bool CPipeFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  std::string name = url.Get();

  m_pipe = PipesManager::GetInstance().CreatePipe(name);
  if (m_pipe)
    m_pipe->AddListener(this);
  return (m_pipe != NULL);
}

bool CPipeFile::Delete(const CURL& url)
{
  return false;
}

bool CPipeFile::Rename(const CURL& url, const CURL& urlnew)
{
  return false;
}

int CPipeFile::IoControl(int request, void* param)
{
  return -1;
}

std::string CPipeFile::GetName() const
{
  if (!m_pipe)
    return "";
  return m_pipe->GetName();
}

void CPipeFile::OnPipeOverFlow()
{
  CSingleLock lock(m_lock);
  for (size_t l=0; l<m_listeners.size(); l++)
    m_listeners[l]->OnPipeOverFlow();
}

int64_t	CPipeFile::GetAvailableRead()
{
  return m_pipe->GetAvailableRead();
}

void CPipeFile::OnPipeUnderFlow()
{
  for (size_t l=0; l<m_listeners.size(); l++)
    m_listeners[l]->OnPipeUnderFlow();
}

void CPipeFile::AddListener(IPipeListener *l)
{
  CSingleLock lock(m_lock);
  for (size_t i=0; i<m_listeners.size(); i++)
  {
    if (m_listeners[i] == l)
      return;
  }
  m_listeners.push_back(l);
}

void CPipeFile::RemoveListener(IPipeListener *l)
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

void CPipeFile::SetOpenThreashold(int threashold)
{
  m_pipe->SetOpenThreashold(threashold);
}

