/*
 *      Copyright (C) 2011 Team XBMC
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

#include "FilePipe.h"
#include "threads/SingleLock.h"
#include "PipesManager.h"
#include "utils/StringUtils.h"

using namespace XFILE;

CFilePipe::CFilePipe() : m_pos(0), m_length(-1), m_pipe(NULL)
{
}

CFilePipe::~CFilePipe()
{
  Close();
}

int64_t CFilePipe::GetPosition()
{
  return m_pos;
}

int64_t CFilePipe::GetLength()
{
  return m_length;
}

void CFilePipe::SetLength(int64_t len)
{
  m_length = len;
}

bool CFilePipe::Open(const CURL& url)
{
  CStdString name = url.Get();
  m_pipe = PipesManager::GetInstance().OpenPipe(name);
  if (m_pipe)
    m_pipe->AddListener(this);
  return (m_pipe != NULL);
}

bool CFilePipe::Exists(const CURL& url)
{
  CStdString name = url.Get();
  return PipesManager::GetInstance().Exists(name);
}

int CFilePipe::Stat(const CURL& url, struct __stat64* buffer)
{
  return -1;
}

int CFilePipe::Stat(struct __stat64* buffer)
{
  memset(buffer,0,sizeof(struct __stat64));
  buffer->st_size = m_length;
  return 0;
}

unsigned int CFilePipe::Read(void* lpBuf, int64_t uiBufSize)
{
  if (!m_pipe)
    return -1;
  
  return m_pipe->Read((char *)lpBuf,(int)uiBufSize,INFINITE);
}

int CFilePipe::Write(const void* lpBuf, int64_t uiBufSize)
{
  if (!m_pipe)
    return -1;
  
  return (int)(m_pipe->Write((const char *)lpBuf,(int)uiBufSize,INFINITE)); // its not the size. its bool. either all was written or not.
}

void CFilePipe::SetEof()
{
  if (!m_pipe)
    return ;
  m_pipe->SetEof();
}

bool CFilePipe::IsEof()
{
  if (!m_pipe)
    return true;
  return m_pipe->IsEof();
}

bool CFilePipe::IsEmpty()
{
  if (!m_pipe)
    return true;
  return m_pipe->IsEmpty();
}

int64_t CFilePipe::Seek(int64_t iFilePosition, int iWhence)
{
  return -1;
}

void CFilePipe::Close()
{
  if (m_pipe)
  {
    m_pipe->RemoveListener(this);
    PipesManager::GetInstance().ClosePipe(m_pipe);    
  }
  m_pipe = NULL;
}

bool CFilePipe::IsClosed()
{
  return (m_pipe == NULL);
}

void CFilePipe::Flush()
{
  if (m_pipe)
    m_pipe->Flush();
}

bool CFilePipe::OpenForWrite(const CURL& url, bool bOverWrite)
{
  CStdString name = url.Get();

  m_pipe = PipesManager::GetInstance().CreatePipe(name);
  if (m_pipe)
    m_pipe->AddListener(this);
  return (m_pipe != NULL);
}

bool CFilePipe::Delete(const CURL& url)
{
  return false;
}

bool CFilePipe::Rename(const CURL& url, const CURL& urlnew)
{
  return false;
}

int CFilePipe::IoControl(int request, void* param)
{
  return -1;
}

CStdString CFilePipe::GetName() const
{
  if (!m_pipe)
    return StringUtils::EmptyString;
  return m_pipe->GetName();
}

void CFilePipe::OnPipeOverFlow()
{
  CSingleLock lock(m_lock);
  for (size_t l=0; l<m_listeners.size(); l++)
    m_listeners[l]->OnPipeOverFlow();
}

__int64	CFilePipe::GetAvailableRead()
{
  return m_pipe->GetAvailableRead();
}

void CFilePipe::OnPipeUnderFlow()
{
  for (size_t l=0; l<m_listeners.size(); l++)
    m_listeners[l]->OnPipeUnderFlow();
}

void CFilePipe::AddListener(IPipeListener *l)
{
  CSingleLock lock(m_lock);
  for (size_t i=0; i<m_listeners.size(); i++)
  {
    if (m_listeners[i] == l)
      return;
  }
  m_listeners.push_back(l);
}

void CFilePipe::RemoveListener(IPipeListener *l)
{
  CSingleLock lock(m_lock);
  std::vector<XFILE::IPipeListener *>::iterator i = m_listeners.begin();
  while(i != m_listeners.end())
  {
    if ( (*i) == l)
      i = m_listeners.erase(i);
    else
      i++;
  }
}

void CFilePipe::SetOpenThreashold(int threashold)
{
  m_pipe->SetOpenThreashold(threashold);
}

