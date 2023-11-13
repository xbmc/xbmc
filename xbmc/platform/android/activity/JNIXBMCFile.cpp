/*
 *  Copyright (C) 2018 Christian Browet
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JNIXBMCFile.h"

#include "CompileInfo.h"
#include "utils/FileUtils.h"
#include "utils/log.h"

#include <memory>

#include <androidjni/jutils-details.hpp>

#define BUFFSIZE 8192

using namespace jni;

static std::string s_className = std::string(CCompileInfo::GetClass()) + "/XBMCFile";

CJNIXBMCFile::CJNIXBMCFile()
  : CJNIBase()
{
}

void CJNIXBMCFile::RegisterNatives(JNIEnv *env)
{
  jclass cClass = env->FindClass(s_className.c_str());
  if(cClass)
  {
    JNINativeMethod methods[] =
    {
      {"_open", "(Ljava/lang/String;)Z", (void*)&CJNIXBMCFile::_open},
      {"_close", "()V", (void*)&CJNIXBMCFile::_close},
      {"_read", "()[B", (void*)&CJNIXBMCFile::_read},
      {"_eof", "()Z", (void*)&CJNIXBMCFile::_eof},
    };

    env->RegisterNatives(cClass, methods, sizeof(methods)/sizeof(methods[0]));
  }
}

jboolean CJNIXBMCFile::_open(JNIEnv *env, jobject thiz, jstring path)
{
  std::string strPath = jcast<std::string>(jhstring::fromJNI(path));

  if (find_instance(thiz))
    return false;

  if (!XFILE::CFile::Exists(strPath))
    return false;

  CJNIXBMCFile* file = new CJNIXBMCFile();
  file->m_file = std::make_unique<XFILE::CFile>();
  bool ret = file->m_file->Open(strPath);
  if (!ret)
  {
    delete file;
    return false;
  }

  jhobject jo = jhobject::fromJNI(thiz);
  jo.setGlobal();
  add_instance(jo, file);
  file->m_eof = false;
  return true;
}

void CJNIXBMCFile::_close(JNIEnv *env, jobject thiz)
{
  CJNIXBMCFile *inst = find_instance(thiz);
  if (inst)
  {
    inst->m_file->Close();
    remove_instance(inst);
    delete inst;
  }
}

jbyteArray CJNIXBMCFile::_read(JNIEnv *env, jobject thiz)
{
  ssize_t sz = 0;
  char buffer[BUFFSIZE];

  CJNIXBMCFile *inst = find_instance(thiz);
  if (inst && inst->m_file)
  {
    sz = inst->m_file->Read((void*)buffer, BUFFSIZE);
    if (sz <= 0)
    {
      inst->m_eof = true;
      sz = 0;
    }
  }

  jbyteArray jba = NULL;
  char*   pArray;
  jba = env->NewByteArray(sz);
  if ((pArray = (char*)env->GetPrimitiveArrayCritical(jba, NULL)))
  {
    memcpy(pArray, buffer, sz);
    env->ReleasePrimitiveArrayCritical(jba, pArray, 0);
  }

  return jba;
}

jboolean CJNIXBMCFile::_eof(JNIEnv *env, jobject thiz)
{
  CJNIXBMCFile *inst = find_instance(thiz);
  if (inst)
    return inst->m_eof;

  return true;
}




