/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IClient.h"
#include "ITransportLayer.h"

#include <map>
#include <memory>
#include <string>

class CFileItem;
class CVariant;
class CVideoInfoTag;

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
  typedef JSONRPC_STATUS (*MethodCall) (const std::string &method, ITransportLayer *transport, IClient *client, const CVariant& parameterObject, CVariant &result);

  /*!
   \ingroup jsonrpc
   \brief Permission categories for json rpc methods

   A JSON-RPC method will only be called if the caller
   has the correct permissions to execute the method.
   The method call needs to be perfectly threadsafe.
  */
  enum OperationPermission
  {
    ReadData        =    0x1,
    ControlPlayback =    0x2,
    ControlNotify   =    0x4,
    ControlPower    =    0x8,
    UpdateData      =   0x10,
    RemoveData      =   0x20,
    Navigate        =   0x40,
    WriteFile       =   0x80,
    ControlSystem   =  0x100,
    ControlGUI      =  0x200,
    ManageAddon     =  0x400,
    ExecuteAddon    =  0x800,
    ControlPVR      = 0x1000
  };

  const int OPERATION_PERMISSION_ALL = (ReadData | ControlPlayback | ControlNotify | ControlPower |
                                        UpdateData | RemoveData | Navigate | WriteFile | ControlSystem |
                                        ControlGUI | ManageAddon | ExecuteAddon | ControlPVR);

  const int OPERATION_PERMISSION_NOTIFICATION = (ControlPlayback | ControlNotify | ControlPower | UpdateData |
                                                 RemoveData | Navigate | WriteFile | ControlSystem |
                                                 ControlGUI | ManageAddon | ExecuteAddon | ControlPVR);

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
    case ControlSystem:
      return "ControlSystem";
    case ControlGUI:
      return "ControlGUI";
    case ManageAddon:
      return "ManageAddon";
    case ExecuteAddon:
      return "ExecuteAddon";
    case ControlPVR:
      return "ControlPVR";
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
  inline OperationPermission StringToPermission(const std::string& permission)
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
    if (permission.compare("ControlSystem") == 0)
      return ControlSystem;
    if (permission.compare("ControlGUI") == 0)
      return ControlGUI;
    if (permission.compare("ManageAddon") == 0)
      return ManageAddon;
    if (permission.compare("ExecuteAddon") == 0)
      return ExecuteAddon;
    if (permission.compare("ControlPVR") == 0)
      return ControlPVR;

    return ReadData;
  }

  class CJSONRPCUtils
  {
  public:
    static void NotifyItemUpdated();
    static void NotifyItemUpdated(const std::shared_ptr<CFileItem>& item);
    static void NotifyItemUpdated(const CVideoInfoTag& info,
                                  const std::map<std::string, std::string>& artwork);
  };
}
