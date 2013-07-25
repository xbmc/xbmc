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
/*
 * Copyright (c) 2011-2012 Dmitry Moskalchuk <dm@crystax.net>.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY Dmitry Moskalchuk ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Dmitry Moskalchuk OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Dmitry Moskalchuk.
 */

#define DBG(fmt, ...)
#include <string>
#include "jutils/jutils-details.hpp"

namespace jni
{

namespace details
{

std::string jcast_helper<std::string, jstring>::cast(jstring const &v)
{
    JNIEnv *env = xbmc_jnienv();
    std::string ret;
    if (!v)
      return ret;

    const char *s = env->GetStringUTFChars(v, JNI_FALSE);
    if (s)
    {
      ret = s;
      env->ReleaseStringUTFChars(v, s);
    }
    return ret;
}

jhstring jcast_helper<jhstring, std::string>::cast(const std::string &s)
{
    JNIEnv *env = xbmc_jnienv();
    jstring ret = NULL;
    if (!s.empty())
    {
      ret = env->NewStringUTF(s.c_str());
    }
    return jhstring(ret);
}

jhobjectArray jcast_helper<jhobjectArray, std::vector<std::string> >::cast(const std::vector<std::string> &s)
{
  JNIEnv *env = xbmc_jnienv();
  jobjectArray ret = NULL;
  if (!s.empty())
  {
    ret = env->NewObjectArray(s.size(), env->FindClass("java/lang/String"), NULL);
    for (unsigned int i = 0; i < s.size(); i++)
    env->SetObjectArrayElement(ret, i, env->NewStringUTF(s[i].c_str()));
  }
  return jhobjectArray(ret);
}

std::vector<std::string> jcast_helper<std::vector<std::string>, jobjectArray >::cast(const jobjectArray &s)
{
  JNIEnv *env = xbmc_jnienv();
  std::vector<std::string> ret;
  jstring element;
  const char* newString = NULL;
  if (!s)
    return ret;

  unsigned int arraySize = env->GetArrayLength(s);
  ret.reserve(arraySize);
  for (unsigned int i = 0; i < arraySize; ++i)
  {
    element = (jstring) env->GetObjectArrayElement(s, i);
    newString = env->GetStringUTFChars(element, JNI_FALSE);
    if (newString)
    {
      ret.push_back(newString);
      env->ReleaseStringUTFChars(element, newString);
    }
  }
  return ret;
}

#define CRYSTAX_PP_CAT(a, b, c) CRYSTAX_PP_CAT_IMPL(a, b, c)
#define CRYSTAX_PP_CAT_IMPL(a, b, c) a ## b ## c

#define CRYSTAX_PP_STRINGIZE(a) CRYSTAX_PP_STRINGIZE_IMPL(a)
#define CRYSTAX_PP_STRINGIZE_IMPL(a) #a

#define JNI_MAP_void Void
#define JNI_MAP_jboolean Boolean
#define JNI_MAP_jbyte Byte
#define JNI_MAP_jchar Char
#define JNI_MAP_jshort Short
#define JNI_MAP_jint Int
#define JNI_MAP_jlong Long
#define JNI_MAP_jfloat Float
#define JNI_MAP_jdouble Double
#define JNI_MAP_jhobject Object
#define JNI_MAP_jhclass Object
#define JNI_MAP_jhstring Object
#define JNI_MAP_jhthrowable Object
#define JNI_MAP_jharray Object
#define JNI_MAP_jhbooleanArray Object
#define JNI_MAP_jhbyteArray Object
#define JNI_MAP_jhshortArray Object
#define JNI_MAP_jhintArray Object
#define JNI_MAP_jhlongArray Object
#define JNI_MAP_jhfloatArray Object
#define JNI_MAP_jhdoubleArray Object
#define JNI_MAP_jhobjectArray Object

#define JNI_MAP(type) JNI_MAP_ ## type

template <typename T>
struct jni_base_type
{
    typedef T type_t;
};

template <typename T>
struct jni_base_type<jholder<T> >
{
    typedef T type_t;
};

void call_void_method(JNIEnv *env, jobject obj, jmethodID mid, ...)
{
    va_list vl;
    va_start(vl, mid);
    env->CallVoidMethodV(obj, mid, vl);
    va_end(vl);
}

void call_void_method(JNIEnv *env, jclass cls, jmethodID mid, ...)
{
    va_list vl;
    va_start(vl, mid);
    env->CallStaticVoidMethodV(cls, mid, vl);
    va_end(vl);
}

jhobject new_object(JNIEnv *env, jclass cls, jmethodID mid, ...)
{
  va_list vl;
  va_start(vl,mid);
  jhobject ret;
  if (env && cls && mid)
    ret = jholder<jobject>(env->NewObjectV(cls, mid, vl));
  va_end(vl);
  return ret;
}

template <typename T>
struct result_helper
{
    static T make_result(JNIEnv *env, T obj) {(void)env; return obj;}
};

template <typename T>
struct result_helper<jholder<T> >
{
    static jholder<T> make_result(JNIEnv *env, T obj) {return jholder<T>(env->ExceptionCheck() ? 0 : obj);}
};

#define CRYSTAX_PP_STEP(type) \
    type CRYSTAX_PP_CAT(get_, type, _field)(JNIEnv *env, jobject obj, jfieldID fid) \
    { \
        DBG("calling Get" CRYSTAX_PP_STRINGIZE(JNI_MAP(type)) "Field"); \
        return type((jni_base_type<type>::type_t)CRYSTAX_PP_CAT(env->Get, JNI_MAP(type), Field)(obj, fid)); \
    } \
    type CRYSTAX_PP_CAT(get_static_, type, _field)(JNIEnv *env, jclass cls, jfieldID fid) \
    { \
        DBG("calling GetStatic" CRYSTAX_PP_STRINGIZE(JNI_MAP(type)) "Field"); \
        return type((jni_base_type<type>::type_t)CRYSTAX_PP_CAT(env->GetStatic, JNI_MAP(type), Field)(cls, fid)); \
    } \
    void CRYSTAX_PP_CAT(set_, type, _field)(JNIEnv *env, jobject obj, jfieldID fid, type const &arg) \
    { \
        DBG("calling Set" CRYSTAX_PP_STRINGIZE(JNI_MAP(type)) "Field"); \
        CRYSTAX_PP_CAT(env->Set, JNI_MAP(type), Field)(obj, fid, (jni_base_type<type>::type_t)raw_arg(arg)); \
    } \
    void CRYSTAX_PP_CAT(set_, type, _field)(JNIEnv *env, jclass cls, jfieldID fid, type const &arg) \
    { \
        DBG("calling SetStatic" CRYSTAX_PP_STRINGIZE(JNI_MAP(type)) "Field"); \
        CRYSTAX_PP_CAT(env->SetStatic, JNI_MAP(type), Field)(cls, fid, (jni_base_type<type>::type_t)raw_arg(arg)); \
    } \
    type CRYSTAX_PP_CAT(call_, type, _method)(JNIEnv *env, jobject obj, jmethodID mid, ...) \
    { \
        DBG("calling Call" CRYSTAX_PP_STRINGIZE(JNI_MAP(type)) "MethodV"); \
        va_list vl; \
        va_start(vl, mid); \
        typedef jni_base_type<type>::type_t result_t; \
        result_t result = (result_t)CRYSTAX_PP_CAT(env->Call, JNI_MAP(type), MethodV)(obj, mid, vl); \
        va_end(vl); \
        return result_helper<type>::make_result(env, result); \
    } \
    type CRYSTAX_PP_CAT(call_, type, _method)(JNIEnv *env, jclass cls, jmethodID mid, ...) \
    { \
        DBG("calling CallStatic" CRYSTAX_PP_STRINGIZE(JNI_MAP(type)) "MethodV"); \
        va_list vl; \
        va_start(vl, mid); \
        typedef jni_base_type<type>::type_t result_t; \
        result_t result = (result_t)CRYSTAX_PP_CAT(env->CallStatic, JNI_MAP(type), MethodV)(cls, mid, vl); \
        va_end(vl); \
        return result_helper<type>::make_result(env, result); \
    }
#include "jutils/jni.inc"
#undef CRYSTAX_PP_STEP

template <> const char *jni_signature<jboolean>::signature = "Z";
template <> const char *jni_signature<jbyte>::signature = "B";
template <> const char *jni_signature<jchar>::signature = "C";
template <> const char *jni_signature<jshort>::signature = "S";
template <> const char *jni_signature<jint>::signature = "I";
template <> const char *jni_signature<jlong>::signature = "J";
template <> const char *jni_signature<jfloat>::signature = "F";
template <> const char *jni_signature<jdouble>::signature = "D";
template <> const char *jni_signature<jhobject>::signature = "Ljava/lang/Object;";
template <> const char *jni_signature<jhclass>::signature = "Ljava/lang/Class;";
template <> const char *jni_signature<jhstring>::signature = "Ljava/lang/String;";
template <> const char *jni_signature<jhthrowable>::signature = "Ljava/lang/Throwable;";
template <> const char *jni_signature<jhbooleanArray>::signature = "[Z";
template <> const char *jni_signature<jhbyteArray>::signature = "[B";
template <> const char *jni_signature<jhcharArray>::signature = "[C";
template <> const char *jni_signature<jhshortArray>::signature = "[S";
template <> const char *jni_signature<jhintArray>::signature = "[I";
template <> const char *jni_signature<jhlongArray>::signature = "[J";
template <> const char *jni_signature<jhfloatArray>::signature = "[F";
template <> const char *jni_signature<jhdoubleArray>::signature = "[D";
template <> const char *jni_signature<jhobjectArray>::signature = "[Ljava/lang/Object;";

} // namespace details
} // namespace jni
