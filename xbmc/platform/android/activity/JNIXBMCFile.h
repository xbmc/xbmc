#pragma once
/*
 *      Copyright (C) 2018 Christian Browet
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

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
