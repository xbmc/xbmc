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

#pragma once

#include <androidjni/JNIBase.h>

#include "interfaces/json-rpc/IClient.h"
#include "interfaces/json-rpc/ITransportLayer.h"

namespace jni
{

  class CJNIXBMCJsonHandler : public CJNIBase
  {
  public:
    CJNIXBMCJsonHandler(const jni::jhobject &object) : CJNIBase(object) {}

    static void RegisterNatives(JNIEnv* env);


  protected:
    virtual ~CJNIXBMCJsonHandler() {}

    static jstring _requestJSON(JNIEnv* env, jobject thiz, jstring request);

    class CJNITransportLayer : public JSONRPC::ITransportLayer
    {
    public:
      CJNITransportLayer() = default;
      ~CJNITransportLayer() = default;

      // implementations of JSONRPC::ITransportLayer
      bool PrepareDownload(const char *path, CVariant &details, std::string &protocol) override;
      bool Download(const char *path, CVariant &result) override;
      int GetCapabilities() override;
    };

    class CJNIClient : public JSONRPC::IClient
    {
    public:
      virtual int  GetPermissionFlags();
      virtual int  GetAnnouncementFlags();
      virtual bool SetAnnouncementFlags(int flags);
    };

  };

}
