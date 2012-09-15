#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#pragma once

#include <vector>
#include "guilib/GUIDialog.h"
#include "IConnection.h"
#include "utils/Job.h"


const std::string EncodeAccessPointParam(const std::string name, const CIPConfig &ipconfig);

class CFileItemList;

class CGUIDialogAccessPoints : public CGUIDialog, public IJobCallback
{
public:
  CGUIDialogAccessPoints(void);
  virtual ~CGUIDialogAccessPoints(void);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnBack(int actionID);

  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);
private:
  void UpdateConnectionList();
  void DecodeAccessPointParam(const std::string &param);

  static const char *ConnectionStateToString(ConnectionState state);
  static const char *ConnectionTypeToString(ConnectionType type);
  static const char *EncryptionToString(EncryptionType type);

  std::string   m_ipname;
  CIPConfig     m_ipconfig;
  bool          m_use_ipconfig;
  bool          m_doing_connection;
  CFileItemList *m_connectionsFileList;
};
