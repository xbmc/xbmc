#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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

#include <Python.h>
#include <string>

#include "system.h"
#include "interfaces/json-rpc/IJSONRPCAnnouncer.h"
#include "interfaces/json-rpc/ITransportLayer.h"
#include "interfaces/json-rpc/JSONRPC.h"

int Py_XBMC_Event_OnNotification(void* arg);

#ifdef HAS_JSONRPC
class CPythonJsonRpcTransport : public JSONRPC::ITransportLayer
{
public:
  virtual bool PrepareDownload(const char *path, CVariant &details, std::string &protocol) { return false; }
  virtual bool Download(const char *path, CVariant &result) { return false; }
  virtual int GetCapabilities() { return JSONRPC::Response | JSONRPC::Announcing; }
};

class CPythonJsonRpcClient : public JSONRPC::IClient, public JSONRPC::IJSONRPCAnnouncer
{
public:
  CPythonJsonRpcClient();

  void Acquire();
  void Release();
  void SetCallback(PyThreadState *state, PyObject *object);

  virtual int  GetPermissionFlags() { return JSONRPC::OPERATION_PERMISSION_ALL; }
  virtual int  GetAnnouncementFlags() { return m_announcementFlags; }
  virtual bool SetAnnouncementFlags(int flags) { m_announcementFlags = flags; return true; }

  virtual void Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data);
    
  PyObject *m_callback;
  PyThreadState *m_state;

protected:
  virtual ~CPythonJsonRpcClient();
  long m_refs;

private:
  int m_announcementFlags;
};
#endif