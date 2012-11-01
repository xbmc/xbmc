#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/GUIDialog.h"

class CGUIDialogNetworkSetup :
      public CGUIDialog
{
public:
  enum NET_PROTOCOL { NET_PROTOCOL_SMB = 0,
                      NET_PROTOCOL_XBMSP,
                      NET_PROTOCOL_FTP,
                      NET_PROTOCOL_HTTP,
                      NET_PROTOCOL_HTTPS,
                      NET_PROTOCOL_DAV,
                      NET_PROTOCOL_DAVS,
                      NET_PROTOCOL_DAAP,
                      NET_PROTOCOL_UPNP,
                      NET_PROTOCOL_RSS,
                      NET_PROTOCOL_HTSP,
                      NET_PROTOCOL_VTP,
                      NET_PROTOCOL_MYTH,
                      NET_PROTOCOL_TUXBOX,
                      NET_PROTOCOL_SFTP,
                      NET_PROTOCOL_NFS, 
                      NET_PROTOCOL_AFP};
  CGUIDialogNetworkSetup(void);
  virtual ~CGUIDialogNetworkSetup(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnBack(int actionID);
  virtual void OnInitWindow();
  virtual void OnWindowLoaded();
  virtual void OnDeinitWindow(int nextWindowID);

  static bool ShowAndGetNetworkAddress(CStdString &path);

  CStdString ConstructPath() const;
  void SetPath(const CStdString &path);
  bool IsConfirmed() const { return m_confirmed; };

protected:
  void OnProtocolChange();
  void OnServerBrowse();
  void OnOK();
  void OnCancel();
  void UpdateButtons();

  NET_PROTOCOL m_protocol;
  CStdString m_server;
  CStdString m_path;
  CStdString m_username;
  CStdString m_password;
  CStdString m_port;

  bool m_confirmed;
};
