/*
 *  Copyright (C) 2018 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/json-rpc/IClient.h"
#include "interfaces/json-rpc/ITransportLayer.h"

#include <androidjni/JNIBase.h>

namespace jni
{

  class CJNIXBMCJsonHandler : public CJNIBase
  {
  public:
    CJNIXBMCJsonHandler(const jni::jhobject &object) : CJNIBase(object) {}

    static void RegisterNatives(JNIEnv* env);


  protected:
    ~CJNIXBMCJsonHandler() override = default;

    static jstring _requestJSON(JNIEnv* env, jobject thiz, jstring request);

    class CJNITransportLayer : public JSONRPC::ITransportLayer
    {
    public:
      CJNITransportLayer() = default;
      ~CJNITransportLayer() override = default;

      // implementations of JSONRPC::ITransportLayer
      bool PrepareDownload(const char *path, CVariant &details, std::string &protocol) override;
      bool Download(const char *path, CVariant &result) override;
      int GetCapabilities() override;
    };

    class CJNIClient : public JSONRPC::IClient
    {
    public:
      int GetPermissionFlags() override;
      int GetAnnouncementFlags() override;
      bool SetAnnouncementFlags(int flags) override;
    };

  };

}
