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

#include "system.h" // for HANDLE and SOCKET
#include <stdlib.h>

namespace AUTOPTR
{
class CAutoPtrHandle
{
public:
  CAutoPtrHandle(HANDLE hHandle);
  virtual ~CAutoPtrHandle(void);
  operator HANDLE();
  void attach(HANDLE hHandle);
  HANDLE release();
  bool isValid() const;
  void reset();
protected:
  virtual void Cleanup();
  HANDLE m_hHandle;
};

class CAutoPtrFind : public CAutoPtrHandle
{
public:
  CAutoPtrFind(HANDLE hHandle);
  virtual ~CAutoPtrFind(void);
protected:
  virtual void Cleanup();
};


class CAutoPtrSocket
{
public:
  CAutoPtrSocket(SOCKET hSocket);
  virtual ~CAutoPtrSocket(void);
  operator SOCKET();
  void attach(SOCKET hSocket);
  SOCKET release();
  bool isValid() const;
  void reset();
protected:
  virtual void Cleanup();
  SOCKET m_hSocket;
};

}
