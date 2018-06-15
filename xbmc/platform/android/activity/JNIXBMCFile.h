/*
 *  Copyright (C) 2018 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <androidjni/JNIBase.h>

#include "filesystem/File.h"

#include <memory>

namespace jni
{

  class CJNIXBMCFile : public CJNIBase, public CJNIInterfaceImplem<CJNIXBMCFile>
  {
  public:
    CJNIXBMCFile();
    CJNIXBMCFile(const jni::jhobject &object) : CJNIBase(object) {}
    virtual ~CJNIXBMCFile() {}

    static void RegisterNatives(JNIEnv* env);

  protected:
    bool m_eof;
    std::unique_ptr<XFILE::CFile> m_file;

    static jboolean _open(JNIEnv* env, jobject thiz, jstring path);
    static void _close(JNIEnv* env, jobject thiz);
    static jbyteArray _read(JNIEnv* env, jobject thiz);
    static jboolean _eof(JNIEnv* env, jobject thiz);
  };

}
