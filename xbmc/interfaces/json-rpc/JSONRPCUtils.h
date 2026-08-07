/*
 *  Copyright (C) 2012-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IClient.h"
#include "ITransportLayer.h"
#include "utils/Artwork.h"

#include <array>
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
 \brief Possible status codes of a response
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
  //-32100..-32000 Reserved for implementation-defined server-errors.
  BadPermission = -32099,
  NotFound = -32098,
  Unavailable = -32097,
  FailedToExecute = -32100
};

/*!
 \ingroup jsonrpc
 \brief Describes a JSONRPC_STATUS that is reported to a client as an error

 The message is what CJSONRPC::BuildResponse puts in "error.message"; the description is
 served through JSONRPC.Introspect so that a client can discover how a call may fail
 without reading this header.
 */
struct JsonRpcStatusDescription
{
  JSONRPC_STATUS status;
  const char* name;
  const char* message;
  const char* description;
  //! Whether responses with this status populate the optional "error.data" member
  bool hasData;
};

/*!
 \ingroup jsonrpc
 \brief The error taxonomy of the JSON-RPC API

 Every JSONRPC_STATUS that reaches a client as an error appears here exactly once. OK and
 ACK are absent because they produce a result rather than an error.
 */
inline constexpr std::array<JsonRpcStatusDescription, 9> JSONRPC_STATUS_DESCRIPTIONS{{
    {ParseError, "ParseError", "Parse error.", "The request could not be parsed as JSON.", false},
    {InvalidRequest, "InvalidRequest", "Invalid request.",
     "The request parsed as JSON but is not a well-formed JSON-RPC 2.0 request object.", false},
    {MethodNotFound, "MethodNotFound", "Method not found.",
     "The requested method does not exist, or the client lacks the permission to see it.", false},
    {InvalidParams, "InvalidParams", "Invalid params.",
     "The given parameters do not validate against the schema of the method. The \"data\" member "
     "names the offending parameter and the constraint it failed.",
     true},
    {InternalError, "InternalError", "Internal error.",
     "The method failed for a reason that no other status describes.", false},
    {FailedToExecute, "FailedToExecute", "Failed to execute method.",
     "The method was called correctly but the operation it requested did not succeed. Note that "
     "-32100 sits one code point below the -32099..-32000 range that JSON-RPC 2.0 reserves for "
     "implementation-defined server errors; this is long-standing and is retained for "
     "compatibility with existing clients.",
     false},
    {BadPermission, "BadPermission", "Bad client permission.",
     "The client does not hold every permission the method requires.", false},
    {NotFound, "NotFound", "Not found.", "The requested item does not exist.", false},
    {Unavailable, "Unavailable", "Requested item is unavailable.",
     "The requested item exists but cannot be provided at the moment.", false},
}};

/*!
 \brief Returns the description of the given JSONRPC_STATUS
 \param status Specific JSONRPC_STATUS
 \return Description of the given status, or nullptr if it is not reported as an error
 */
inline const JsonRpcStatusDescription* StatusToDescription(JSONRPC_STATUS status)
{
  for (const auto& description : JSONRPC_STATUS_DESCRIPTIONS)
  {
    if (description.status == status)
      return &description;
  }

  return nullptr;
}

/*!
 \brief Function pointer for JSON-RPC methods
 */
typedef JSONRPC_STATUS (*MethodCall)(const std::string& method,
                                     ITransportLayer* transport,
                                     IClient* client,
                                     const CVariant& parameterObject,
                                     CVariant& result);

/*!
 \ingroup jsonrpc
 \brief Permission categories for json rpc methods

 A JSON-RPC method will only be called if the caller
 has the correct permissions to execute the method.
 The method call needs to be perfectly threadsafe.
 */
enum OperationPermission
{
  ReadData = 0x1,
  ControlPlayback = 0x2,
  ControlNotify = 0x4,
  ControlPower = 0x8,
  UpdateData = 0x10,
  RemoveData = 0x20,
  Navigate = 0x40,
  WriteFile = 0x80,
  ControlSystem = 0x100,
  ControlGUI = 0x200,
  ManageAddon = 0x400,
  ExecuteAddon = 0x800,
  ControlPVR = 0x1000
};

const int OPERATION_PERMISSION_ALL =
    (ReadData | ControlPlayback | ControlNotify | ControlPower | UpdateData | RemoveData |
     Navigate | WriteFile | ControlSystem | ControlGUI | ManageAddon | ExecuteAddon | ControlPVR);

const int OPERATION_PERMISSION_NOTIFICATION =
    (ControlPlayback | ControlNotify | ControlPower | UpdateData | RemoveData | Navigate |
     WriteFile | ControlSystem | ControlGUI | ManageAddon | ExecuteAddon | ControlPVR);

/*!
 \brief Returns a string representation for the
 given OperationPermission
 \param permission Specific OperationPermission
 \return String representation of the given OperationPermission
 */
inline const char* PermissionToString(const OperationPermission& permission)
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
    static void NotifyItemUpdated(const CVideoInfoTag& info, const KODI::ART::Artwork& artwork);
  };
}
