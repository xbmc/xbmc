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
 *  the Free Software Foundation, 51 Franklin Street, Suite 500, Boston, MA 02110, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "IClient.h"
#include "ITransportLayer.h"
#include "interfaces/IAnnouncer.h"
#include "utils/StdString.h"
#include "utils/Variant.h"

namespace JSONRPC
{
  /*!
   \ingroup jsonrpc
   \brief Possible statuc codes of a response
   to a JSON-RPC request
   */
  enum JSONRPC_STATUS
  {
    OK = 0,
    ACK = -1,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    ParseError = -32700,
    //-32099..-32000 Reserved for implementation-defined server-errors.
    BadPermission = -32099,
    FailedToExecute = -32100
  };

  /*!
   \brief Function pointer for JSON-RPC methods
   */
  typedef JSONRPC_STATUS (*MethodCall) (const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);

  /*!
   \ingroup jsonrpc
   \brief Permission categories for json rpc methods
   
   A JSON-RPC method will only be called if the caller 
   has the correct permissions to exectue the method.
   The method call needs to be perfectly threadsafe.
  */
  enum OperationPermission
  {
    ReadData        =   0x1,
    ControlPlayback =   0x2,
    ControlNotify   =   0x4,
    ControlPower    =   0x8,
    UpdateData      =  0x10,
    RemoveData      =  0x20,
    Navigate        =  0x40,
    WriteFile       =  0x80
  };

  const int OPERATION_PERMISSION_ALL = (ReadData | ControlPlayback | ControlNotify | ControlPower | UpdateData | RemoveData | Navigate | WriteFile);

  const int OPERATION_PERMISSION_NOTIFICATION = (ControlPlayback | ControlNotify | ControlPower | UpdateData | RemoveData | Navigate | WriteFile);

  /*!
    \brief Returns a string representation for the 
    given OperationPermission
    \param permission Specific OperationPermission
    \return String representation of the given OperationPermission
    */
  inline const char *PermissionToString(const OperationPermission &permission)
  {
    switch (permission)
    {
    case ReadData:
      return "ReadData";
    case ControlPlayback:
      return "ControlPlayback";
    case ControlNotify:
      return "ControlNotify";
    case ControlPower:
      return "ControlPower";
    case UpdateData:
      return "UpdateData";
    case RemoveData:
      return "RemoveData";
    case Navigate:
      return "Navigate";
    case WriteFile:
      return "WriteFile";
    default:
      return "Unknown";
    }
  }

  /*!
    \brief Returns a OperationPermission value for the given
    string representation
    \param permission String representation of the OperationPermission
    \return OperationPermission value of the given string representation
    */
  inline OperationPermission StringToPermission(std::string permission)
  {
    if (permission.compare("ControlPlayback") == 0)
      return ControlPlayback;
    if (permission.compare("ControlNotify") == 0)
      return ControlNotify;
    if (permission.compare("ControlPower") == 0)
      return ControlPower;
    if (permission.compare("UpdateData") == 0)
      return UpdateData;
    if (permission.compare("RemoveData") == 0)
      return RemoveData;
    if (permission.compare("Navigate") == 0)
      return Navigate;
    if (permission.compare("WriteFile") == 0)
      return WriteFile;

    return ReadData;
  }
}
