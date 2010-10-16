/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#ifndef SERVER_H
#define SERVER_H

#include <list>
#include <vdr/thread.h>

#include "config.h"
//#include "cmdcontrol.h"

class cConnection;

typedef std::list<cConnection*> Connections;

class cServer : public cThread
{
protected:
  virtual void Action(void);
  void NewClientConnected(int fd);

  int           m_ServerPort;
  int           m_ServerId;
  int           m_ServerFD;
  cString       m_AllowedHostsFile;
  Connections   m_Connections;
  //cCmdControl   m_CmdControl;
  static unsigned int m_IdCnt;

public:
  cServer(int listenPort);
  virtual ~cServer();

  //cCmdControl *GetCmdControl() { return &m_CmdControl; }
};

#endif /* SERVER_H */
