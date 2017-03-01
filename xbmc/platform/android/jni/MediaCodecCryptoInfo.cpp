/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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

#include "MediaCodecCryptoInfo.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIMediaCodecCryptoInfo::CJNIMediaCodecCryptoInfo()
  : CJNIBase("android/media/MediaCodec$CryptoInfo")
{
  m_object = new_object(GetClassName(), "<init>", "()V");
  m_object.setGlobal();
}

int CJNIMediaCodecCryptoInfo::numSubSamples() const
{
  return get_field<int>(m_object, "numSubSamples");
}

std::vector<int> CJNIMediaCodecCryptoInfo::numBytesOfClearData() const
{
  JNIEnv *env = xbmc_jnienv();

  jhintArray numBytesOfClearData = get_field<jhintArray>(m_object, "numBytesOfClearData");
  jsize size = env->GetArrayLength(numBytesOfClearData.get());
  std::vector<int> intarray;
  intarray.resize(size);
  env->GetIntArrayRegion(numBytesOfClearData.get(), 0, size, (jint*)intarray.data());

  return intarray;
}

std::vector<int> CJNIMediaCodecCryptoInfo::numBytesOfEncryptedData() const
{
  JNIEnv *env = xbmc_jnienv();

  jhintArray numBytesOfEncryptedData = get_field<jhintArray>(m_object, "numBytesOfEncryptedData");
  jsize size = env->GetArrayLength(numBytesOfEncryptedData.get());
  std::vector<int> intarray;
  intarray.resize(size);
  env->GetIntArrayRegion(numBytesOfEncryptedData.get(), 0, size, (jint*)intarray.data());

  return intarray;
}

std::vector<char> CJNIMediaCodecCryptoInfo::key() const
{
  JNIEnv *env = xbmc_jnienv();

  jhbyteArray key = get_field<jhbyteArray>(m_object, "key");
  jsize size = env->GetArrayLength(key.get());
  std::vector<char> chararray;
  chararray.resize(size);
  env->GetByteArrayRegion(key.get(), 0, size, (jbyte*)chararray.data());

  return chararray;
}

std::vector<char> CJNIMediaCodecCryptoInfo::iv() const
{
  JNIEnv *env = xbmc_jnienv();

  jhbyteArray iv = get_field<jhbyteArray>(m_object, "iv");
  jsize size = env->GetArrayLength(iv.get());
  std::vector<char> chararray;
  chararray.resize(size);
  env->GetByteArrayRegion(iv.get(), 0, size, (jbyte*)chararray.data());

  return chararray;
}

int CJNIMediaCodecCryptoInfo::mode() const
{
  return get_field<int>(m_object, "mode");
}

void CJNIMediaCodecCryptoInfo::set(int newNumSubSamples,
  const std::vector<int>  &newNumBytesOfClearData,
  const std::vector<int>  &newNumBytesOfEncryptedData,
  const std::vector<char> &newKey,
  const std::vector<char> &newIV,
  int newMode)
{
  jsize size;
  JNIEnv *env = xbmc_jnienv();

  size = newNumBytesOfClearData.size();
  jintArray numBytesOfClearData = env->NewIntArray(size);
  jint *intdata = (jint*)newNumBytesOfClearData.data();
  env->SetIntArrayRegion(numBytesOfClearData, 0, size, intdata);

  size = newNumBytesOfEncryptedData.size();
  jintArray numBytesOfEncryptedData = env->NewIntArray(size);
  intdata = (jint*)newNumBytesOfEncryptedData.data();
  env->SetIntArrayRegion(numBytesOfEncryptedData, 0, size, intdata);

  size = newKey.size();
  jbyteArray Key = env->NewByteArray(size);
  jbyte *bytedata = (jbyte*)newKey.data();
  env->SetByteArrayRegion(Key, 0, size, bytedata);

  size = newIV.size();
  jbyteArray IV = env->NewByteArray(size);
  bytedata = (jbyte*)newIV.data();
  env->SetByteArrayRegion(IV, 0, size, bytedata);

  call_method<void>(m_object,
    "set", "(I[I[I[B[BI)V",
    newNumSubSamples, numBytesOfClearData, numBytesOfEncryptedData, Key, IV, newMode);

  env->DeleteLocalRef(numBytesOfClearData);
  env->DeleteLocalRef(numBytesOfEncryptedData);
  env->DeleteLocalRef(Key);
  env->DeleteLocalRef(IV);
}
