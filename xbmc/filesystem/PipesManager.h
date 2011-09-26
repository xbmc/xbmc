/*
 * Many concepts and protocol are taken from
 * the Boxee project. http://www.boxee.tv
 * 
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

#ifndef __PIPES_MANAGER__H__
#define __PIPES_MANAGER__H__

#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "utils/StdString.h"
#include "utils/RingBuffer.h"

#include <map>

#define PIPE_DEFAULT_MAX_SIZE (6 * 1024 * 1024)

namespace XFILE
{
 
class IPipeListener
{
public:
  virtual ~IPipeListener() {}
  virtual void OnPipeOverFlow() = 0;
  virtual void OnPipeUnderFlow() = 0;
};
  
class Pipe
  {
  public:
    Pipe(const CStdString &name, int nMaxSize = PIPE_DEFAULT_MAX_SIZE ); 
    virtual ~Pipe();
    const CStdString &GetName();
    
    void AddRef();
    void DecRef();   // a pipe does NOT delete itself with ref-count 0. 
    int  RefCount(); 
    
    bool IsEmpty();
    int  Read(char *buf, int nMaxSize, int nWaitMillis);
    bool Write(const char *buf, int nSize, int nWaitMillis);

    void Flush();
    
    void CheckStatus();
    void Close();
    
    void AddListener(IPipeListener *l);
    void RemoveListener(IPipeListener *l);
    
    void SetEof();
    bool IsEof();
    
    int	GetAvailableRead();
    void SetOpenThreashold(int threashold);

  protected:
    
    bool        m_bOpen;
    bool        m_bReadyForRead;

    bool        m_bEof;
    CRingBuffer m_buffer;
    CStdString  m_strPipeName;  
    int         m_nRefCount;
    int         m_nOpenThreashold;

    CEvent     m_readEvent;
    CEvent     m_writeEvent;
    
    std::vector<XFILE::IPipeListener *> m_listeners;
    
    CCriticalSection m_lock;
  };

  
class PipesManager
{
public:
  virtual ~PipesManager();
  static PipesManager &GetInstance();

  CStdString   GetUniquePipeName();
  XFILE::Pipe *CreatePipe(const CStdString &name="", int nMaxPipeSize = PIPE_DEFAULT_MAX_SIZE);
  XFILE::Pipe *OpenPipe(const CStdString &name);
  void         ClosePipe(XFILE::Pipe *pipe);
  bool         Exists(const CStdString &name);
  
protected:
  PipesManager();
  int    m_nGenIdHelper;
  std::map<CStdString, XFILE::Pipe *> m_pipes;  
  
  CCriticalSection m_lock;
};

}

#endif

