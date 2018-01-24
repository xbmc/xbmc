/*
 *      Copyright (C) 2018 Christian Browet
 *      http://kodi.tv
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

#include "JNIXBMCJsonHandler.h"

#include "CompileInfo.h"
#include "interfaces/json-rpc/JSONRPC.h"
#include "interfaces/json-rpc/JSONServiceDescription.h"
#include "interfaces/json-rpc/JSONUtils.h"

#include <androidjni/jutils-details.hpp>
#include <androidjni/Context.h>

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCJsonRPC";

void CJNIXBMCJsonHandler::RegisterNatives(JNIEnv *env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_requestJSON", "(Ljava/lang/String;)Ljava/lang/String;", (void*)&CJNIXBMCJsonHandler::_requestJSON},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

jstring CJNIXBMCJsonHandler::_requestJSON(JNIEnv *env, jobject thiz, jstring request)
{
  CJNIClient client;
  CJNITransportLayer transportLayer;

  std::string strRequest = jcast<std::string>(jhstring::fromJNI(request));
  std::string responseData = JSONRPC::CJSONRPC::MethodCall(strRequest, &transportLayer, &client);

  jstring jres = env->NewStringUTF(responseData.c_str());
  return jres;
}

/**********************************/

bool CJNIXBMCJsonHandler::CJNITransportLayer::PrepareDownload(const char *path, CVariant &details, std::string &protocol)
{
  return false;
}

bool CJNIXBMCJsonHandler::CJNITransportLayer::Download(const char *path, CVariant &result)
{
  return false;
}

int CJNIXBMCJsonHandler::CJNITransportLayer::GetCapabilities()
{
  return JSONRPC::Response;
}

int CJNIXBMCJsonHandler::CJNIClient::GetPermissionFlags()
{
  return JSONRPC::OPERATION_PERMISSION_ALL;
}

int CJNIXBMCJsonHandler::CJNIClient::GetAnnouncementFlags()
{
  // Does not support broadcast
  return 0;
}

bool CJNIXBMCJsonHandler::CJNIClient::SetAnnouncementFlags(int flags)
{
  return false;
}
