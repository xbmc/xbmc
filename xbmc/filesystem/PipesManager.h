/*
 * Many concepts and protocol are taken from
 * the Boxee project. http://www.boxee.tv
 *
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "utils/RingBuffer.h"

#include <map>
#include <string>
#include <vector>

#define PIPE_DEFAULT_MAX_SIZE (6 * 1024 * 1024)

namespace XFILE
{

class IPipeListener
{
public:
  virtual ~IPipeListener() = default;
  virtual void OnPipeOverFlow() = 0;
  virtual void OnPipeUnderFlow() = 0;
};

class Pipe
  {
  public:
    Pipe(const std::string &name, int nMaxSize = PIPE_DEFAULT_MAX_SIZE );
    virtual ~Pipe();
    const std::string &GetName();

    void AddRef();
    void DecRef();   // a pipe does NOT delete itself with ref-count 0.
    int  RefCount();

    bool IsEmpty();

    /**
     * Read into the buffer from the Pipe the num of bytes asked for
     * blocking forever (which happens to be 5 minutes in this case).
     *
     * In the case where nWaitMillis is provided block for that number
     * of milliseconds instead.
     */
    int  Read(char *buf, int nMaxSize, int nWaitMillis = -1);

    /**
     * Write into the Pipe from the buffer the num of bytes asked for
     * blocking forever.
     *
     * In the case where nWaitMillis is provided block for that number
     * of milliseconds instead.
     */
    bool Write(const char *buf, int nSize, int nWaitMillis = -1);

    void Flush();

    void CheckStatus();
    void Close();

    void AddListener(IPipeListener *l);
    void RemoveListener(IPipeListener *l);

    void SetEof();
    bool IsEof();

    int	GetAvailableRead();
    void SetOpenThreshold(int threshold);

  protected:

    bool        m_bOpen;
    bool        m_bReadyForRead;

    bool        m_bEof;
    CRingBuffer m_buffer;
    std::string  m_strPipeName;
    int         m_nRefCount;
    int         m_nOpenThreshold;

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

  std::string   GetUniquePipeName();
  XFILE::Pipe *CreatePipe(const std::string &name="", int nMaxPipeSize = PIPE_DEFAULT_MAX_SIZE);
  XFILE::Pipe *OpenPipe(const std::string &name);
  void         ClosePipe(XFILE::Pipe *pipe);
  bool         Exists(const std::string &name);

protected:
  int    m_nGenIdHelper = 1;
  std::map<std::string, XFILE::Pipe *> m_pipes;

  CCriticalSection m_lock;
};

}

