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

#pragma once
#include <jni.h>
#include <string>
#include "jutils.hpp"
namespace jni
{

namespace details
{

template <typename T, typename U>
struct jcast_helper;

template <>
struct jcast_helper<std::string, jstring>
{
    static std::string cast(jstring const &v);
};

template <>
struct jcast_helper<std::vector<std::string>, jobjectArray>
{
    static std::vector<std::string> cast(jobjectArray const &v);
};

template <typename T>
struct jcast_helper<std::vector<T>, jobjectArray>
{
    static std::vector<T> cast(jobjectArray const &v);
};

template <>
struct jcast_helper<std::vector<float>, jfloatArray>
{
    static std::vector<float> cast(jfloatArray const &v)
    {
        JNIEnv *env = xbmc_jnienv();
        jsize size = 0;
        if(v)
            size = env->GetArrayLength(v);

        std::vector<float> vec;
        vec.reserve(size);

        float *elements = env->GetFloatArrayElements(v, NULL);
        for (int i = 0; i < size; i++)
        {
            vec.emplace_back(elements[i]);
        }
        env->ReleaseFloatArrayElements(v, elements, JNI_ABORT);
        return vec;
    }
};

template <>
struct jcast_helper<jhstring, std::string>
{
    static jhstring cast(const std::string &v);
};

template <>
struct jcast_helper<jhobjectArray, std::vector<std::string> >
{
    static jhobjectArray cast(const std::vector<std::string> &v);
};

template <>
struct jcast_helper<std::string, jhstring>
{
    static std::string cast(jhstring const &v)
    {
        return jcast_helper<std::string, jstring>::cast(v.get());
    }
};

template <>
struct jcast_helper<std::vector<std::string>, jhobjectArray>
{
    static std::vector<std::string> cast(jhobjectArray const &v)
    {
        return jcast_helper<std::vector<std::string>, jobjectArray>::cast(v.get());
    }
};

template <typename T>
struct jcast_helper<std::vector<T>, jhobjectArray>
{
    static std::vector<T> cast(jhobjectArray const &v)
    {
        return jcast_helper<std::vector<T>, jobjectArray>::cast(v.get());
    }
};

template <>
struct jcast_helper<std::vector<float>, jhfloatArray>
{
    static std::vector<float> cast(jhfloatArray const &v)
    {
        return jcast_helper<std::vector<float>, jfloatArray>::cast(v.get());
    }
};

template <typename T>
std::vector<T> jcast_helper<std::vector<T>, jobjectArray>::cast(jobjectArray const &v)
{
  JNIEnv *env = xbmc_jnienv();
  jsize size = 0;
  if(v)
    size = env->GetArrayLength(v);

  std::vector<T> vec;
  vec.reserve(size);

  for (int i = 0; i < size; i++)
  {
    T element((jhobject)env->GetObjectArrayElement(v, i));
    vec.emplace_back(element);
  }
  return vec;
}


} // namespace details

template <typename T, typename U>
T jcast(U const &v)
{
    return details::jcast_helper<T, U>::cast(v);
}

inline
jhclass find_class(JNIEnv *env, const char *clsname)
{
  return jhclass((jclass)env->FindClass(clsname));
}

inline
jhclass find_class(const char *clsname)
{
    return find_class(xbmc_jnienv(), clsname);
}

inline
jhclass get_class(JNIEnv *env, jhobject const &obj)
{
    return jhclass(env->GetObjectClass(obj.get()));
}

inline
jhclass get_class(jhobject const &obj)
{
    return get_class(xbmc_jnienv(), obj);
}

inline
jmethodID get_method_id(JNIEnv *env, jhclass const &cls, const char *name, const char *signature)
{
    return env->GetMethodID(cls.get(), name, signature);
}

inline
jmethodID get_method_id(jhclass const &cls, const char *name, const char *signature)
{
    return get_method_id(xbmc_jnienv(), cls, name, signature);
}

inline
jmethodID get_method_id(JNIEnv *env, jhobject const &obj, const char *name, const char *signature)
{
    return get_method_id(env, get_class(env, obj), name, signature);
}

template <typename T>
jmethodID get_method_id(jholder<T> const &obj, const char *name, const char *signature)
{
    return get_method_id(xbmc_jnienv(), obj, name, signature);
}

inline
jmethodID get_static_method_id(JNIEnv *env, jhclass const &cls, const char *name, const char *signature)
{
    return env->GetStaticMethodID(cls.get(), name, signature);
}

inline
jmethodID get_static_method_id(jhclass const &cls, const char *name, const char *signature)
{
    return get_static_method_id(xbmc_jnienv(), cls, name, signature);
}

template <typename T>
jmethodID get_static_method_id(JNIEnv *env, jholder<T> const &obj, const char *name, const char *signature)
{
    return get_static_method_id(env, get_class(env, obj), name, signature);
}

template <typename T>
jmethodID get_static_method_id(jholder<T> const &obj, const char *name, const char *signature)
{
    return get_static_method_id(xbmc_jnienv(), obj, name, signature);
}

inline
jfieldID get_field_id(JNIEnv *env, jhclass const &cls, const char *name, const char *signature)
{
    return env->GetFieldID(cls.get(), name, signature);
}

inline
jfieldID get_field_id(jhclass const &cls, const char *name, const char *signature)
{
    return get_field_id(xbmc_jnienv(), cls, name, signature);
}

template <typename T>
jfieldID get_field_id(JNIEnv *env, jholder<T> const &obj, const char *name, const char *signature)
{
    return get_field_id(env, get_class(env, obj), name, signature);
}

template <typename T>
jfieldID get_field_id(jholder<T> const &obj, const char *name, const char *signature)
{
    return get_field_id(xbmc_jnienv(), obj, name, signature);
}

inline
jfieldID get_static_field_id(JNIEnv *env, jhclass const &cls, const char *name, const char *signature)
{
    return env->GetStaticFieldID(cls.get(), name, signature);
}

inline
jfieldID get_static_field_id(jhclass const &cls, const char *name, const char *signature)
{
    return get_static_field_id(xbmc_jnienv(), cls, name, signature);
}

template <typename T>
jfieldID get_static_field_id(JNIEnv *env, jholder<T> const &obj, const char *name, const char *signature)
{
    return get_static_field_id(env, get_class(env, obj), name, signature);
}

template <typename T>
jfieldID get_static_field_id(jholder<T> const &obj, const char *name, const char *signature)
{
    return get_static_field_id(xbmc_jnienv(), obj, name, signature);
}

namespace details
{

void call_void_method(JNIEnv *env, jclass cls, jmethodID mid, ...);
void call_void_method(JNIEnv *env, jobject obj, jmethodID mid, ...);
jhobject new_object(JNIEnv *env, jclass cls, jmethodID mid, ...);
#define CRYSTAX_PP_STEP(type) \
    type get_ ## type ## _field(JNIEnv *env, jobject obj, jfieldID fid); \
    type get_ ## type ## _field(JNIEnv *env, jclass cls, jfieldID fid); \
    type get_static_ ## type ## _field(JNIEnv *env, jobject obj, jfieldID fid); \
    type get_static_ ## type ## _field(JNIEnv *env, jclass cls, jfieldID fid); \
    void set_ ## type ## _field(JNIEnv *env, jobject obj, jfieldID fid, type const &arg); \
    void set_ ## type ## _field(JNIEnv *env, jclass cls, jfieldID fid, type const &arg); \
    type call_ ## type ## _method(JNIEnv *env, jclass cls, jmethodID mid, ...); \
    type call_ ## type ## _method(JNIEnv *env, jobject obj, jmethodID mid, ...);
#include "jni.inc"
#undef CRYSTAX_PP_STEP

#define CRYSTAX_PP_STEP(type) inline type raw_arg(type arg) {return arg;}
CRYSTAX_PP_STEP(jboolean)
CRYSTAX_PP_STEP(jbyte)
CRYSTAX_PP_STEP(jchar)
CRYSTAX_PP_STEP(jshort)
CRYSTAX_PP_STEP(jint)
CRYSTAX_PP_STEP(jlong)
CRYSTAX_PP_STEP(jfloat)
CRYSTAX_PP_STEP(jdouble)
CRYSTAX_PP_STEP(jobject)
CRYSTAX_PP_STEP(jclass)
CRYSTAX_PP_STEP(jstring)
CRYSTAX_PP_STEP(jthrowable)
CRYSTAX_PP_STEP(jarray)
CRYSTAX_PP_STEP(jbooleanArray)
CRYSTAX_PP_STEP(jbyteArray)
CRYSTAX_PP_STEP(jshortArray)
CRYSTAX_PP_STEP(jintArray)
CRYSTAX_PP_STEP(jlongArray)
CRYSTAX_PP_STEP(jfloatArray)
CRYSTAX_PP_STEP(jdoubleArray)
CRYSTAX_PP_STEP(jobjectArray)
#undef CRYSTAX_PP_STEP

template <typename T>
T raw_arg(jholder<T> const &arg)
{
    return arg.get();
}

template <typename Ret>
struct jni_helper;

template <>
struct jni_helper<void>
{
    template <typename T, typename... Args>
    static void call_method(JNIEnv *env, T const &obj, jmethodID mid, Args&&... args)
    {
        details::call_void_method(env, raw_arg(obj), mid, raw_arg(args)...);
    }
};

struct jni_stripper
{
    template <typename... Args>
    static jhobject new_object(JNIEnv *env, const jhclass &cls, jmethodID mid, Args&&... args)
    {
        return details::new_object(env, raw_arg(cls), mid, raw_arg(args)...);
    }
};

#define CRYSTAX_PP_STEP(type) \
template <> \
struct jni_helper<type> \
{ \
    template <typename T> \
    static type get_field(JNIEnv *env, T const &obj, jfieldID fid) \
    { \
        return details::get_ ## type ## _field(env, raw_arg(obj), fid); \
    } \
    template <typename T> \
    static type get_static_field(JNIEnv *env, T const &obj, jfieldID fid) \
    { \
        return details::get_static_ ## type ## _field(env, raw_arg(obj), fid); \
    } \
    template <typename T> \
    static void set_field(JNIEnv *env, T const &obj, jfieldID fid, type const &arg) \
    { \
        details::set_ ## type ## _field(env, raw_arg(obj), fid, arg); \
    } \
    template <typename T, typename... Args> \
    static type call_method(JNIEnv *env, T const &obj, jmethodID mid, Args&&... args) \
    { \
        return details::call_ ## type ## _method(env, raw_arg(obj), mid, raw_arg(args)...); \
    } \
};
#include "jni.inc"
#undef CRYSTAX_PP_STEP

template <typename T>
struct jni_signature
{
    static const char *signature;
};

} // namespace details

// Get field

template <typename Ret, typename T>
Ret get_field(JNIEnv *env, jholder<T> const &obj, jfieldID fid)
{
    return details::jni_helper<Ret>::get_field(env, obj, fid);
}

template <typename Ret, typename T>
Ret get_field(jholder<T> const &obj, jfieldID fid)
{
    return get_field<Ret>(xbmc_jnienv(), obj, fid);
}

template <typename Ret>
Ret get_field(JNIEnv *env, const char *clsname, jfieldID fid)
{
    return get_field<Ret>(env, find_class(env, clsname), fid);
}

template <typename Ret>
Ret get_field(const char *clsname, jfieldID fid)
{
    return get_field<Ret>(xbmc_jnienv(), clsname, fid);
}

template <typename Ret, typename T>
Ret get_field(JNIEnv *env, jholder<T> const &obj, const char *name, const char *signature)
{
    return get_field<Ret>(env, obj, get_field_id(env, obj, name, signature));
}

template <typename Ret, typename T>
Ret get_field(jholder<T> const &obj, const char *name, const char *signature)
{
    return get_field<Ret>(xbmc_jnienv(), obj, name, signature);
}
/*
template <typename Ret>
Ret get_field(JNIEnv *env, const char *clsname, const char *name, const char *signature)
{
    return get_field<Ret>(env, find_class(clsname), name, signature);
}

template <typename Ret>
Ret get_field(const char *clsname, const char *name, const char *signature)
{
    return get_field<Ret>(xbmc_jnienv(), clsname, name, signature);
}
*/
template <typename Ret, typename T>
Ret get_field(JNIEnv *env, jholder<T> const &obj, const char *name)
{
    return get_field<Ret>(env, obj, name, details::jni_signature<Ret>::signature);
}

template <typename Ret, typename T>
Ret get_field(jholder<T> const &obj, const char *name)
{
    return get_field<Ret>(xbmc_jnienv(), obj, name);
}
/*
template <typename Ret>
Ret get_field(JNIEnv *env, const char *clsname, const char *name)
{
    return get_field<Ret>(env, find_class(env, clsname), name);
}

template <typename Ret>
Ret get_field(const char *clsname, const char *name)
{
    return get_field<Ret>(xbmc_jnienv(), clsname, name);
}
*/
// Get static field

template <typename Ret>
Ret get_static_field(JNIEnv *env, jhobject const &obj, const char *name, const char *signature)
{
    return details::jni_helper<Ret>::get_static_field(env, get_class(env, obj), get_static_field_id(env, obj, name, signature));
}

template <typename Ret>
Ret get_static_field(JNIEnv *env, jhclass const &cls, const char *name, const char *signature)
{
    return details::jni_helper<Ret>::get_static_field(env, cls, get_static_field_id(env, cls, name, signature));
}

template <typename Ret, typename T>
Ret get_static_field(jholder<T> const &obj, const char *name, const char *signature)
{
    return get_static_field<Ret>(xbmc_jnienv(), obj, name, signature);
}

template <typename Ret>
Ret get_static_field(JNIEnv *env, const char *clsname, const char *name, const char *signature)
{
    return get_static_field<Ret>(env, find_class(env, clsname), name, signature);
}

template <typename Ret>
Ret get_static_field(const char *clsname, const char *name, const char *signature)
{
    return get_static_field<Ret>(xbmc_jnienv(), clsname, name, signature);
}

template <typename Ret, typename T>
Ret get_static_field(JNIEnv *env, jholder<T> const &obj, const char *name)
{
    return get_static_field<Ret>(env, obj, name, details::jni_signature<Ret>::signature);
}

template <typename Ret, typename T>
Ret get_static_field(jholder<T> const &obj, const char *name)
{
    return get_static_field<Ret>(xbmc_jnienv(), obj, name);
}

template <typename Ret>
Ret get_static_field(JNIEnv *env, const char *clsname, const char *name)
{
    return get_static_field<Ret>(env, find_class(env, clsname), name);
}

template <typename Ret>
Ret get_static_field(const char *clsname, const char *name)
{
    return get_static_field<Ret>(xbmc_jnienv(), clsname, name);
}

// Set field

template <typename T, typename Arg>
void set_field(JNIEnv *env, jholder<T> const &obj, jfieldID fid, Arg const &arg)
{
    details::jni_helper<Arg>::set_field(env, obj, fid, arg);
}

template <typename T, typename Arg>
void set_field(jholder<T> const &obj, jfieldID fid, Arg const &arg)
{
    set_field(xbmc_jnienv(), obj, fid, arg);
}

template <typename Arg>
void set_field(JNIEnv *env, const char *clsname, jfieldID fid, Arg const &arg)
{
    set_field(env, find_class(env, clsname), fid, arg);
}

template <typename Arg>
void set_field(const char *clsname, jfieldID fid, Arg const &arg)
{
    set_field(xbmc_jnienv(), clsname, fid, arg);
}

template <typename T, typename Arg>
void set_field(JNIEnv *env, jholder<T> const &obj, const char *name, const char *signature, Arg const &arg)
{
    set_field(env, obj, get_field_id(env, obj, name, signature), arg);
}

template <typename T, typename Arg>
void set_field(jholder<T> const &obj, const char *name, const char *signature, Arg const &arg)
{
    set_field(xbmc_jnienv(), obj, name, signature, arg);
}

template <typename Arg>
void set_field(JNIEnv *env, const char *clsname, const char *name, const char *signature, Arg const &arg)
{
    set_field(env, find_class(env, clsname), name, signature, arg);
}

template <typename Arg>
void set_field(const char *clsname, const char *name, const char *signature, Arg const &arg)
{
    set_field(xbmc_jnienv(), clsname, name, signature, arg);
}

template <typename T, typename Arg>
void set_field(JNIEnv *env, jholder<T> const &obj, const char *name, Arg const &arg)
{
    set_field(env, obj, name, details::jni_signature<Arg>::signature, arg);
}

template <typename T, typename Arg>
void set_field(jholder<T> const &obj, const char *name, Arg const &arg)
{
    set_field(xbmc_jnienv(), obj, name, arg);
}

template <typename Arg>
void set_field(JNIEnv *env, const char *clsname, const char *name, Arg const &arg)
{
    set_field(env, find_class(env, clsname), name, arg);
}

template <typename Arg>
void set_field(const char *clsname, const char *name, Arg const &arg)
{
    set_field(xbmc_jnienv(), clsname, name, arg);
}

// Set static field

template <typename T, typename Arg>
void set_static_field(JNIEnv *env, jholder<T> const &obj, const char *name, const char *signature, Arg const &arg)
{
    set_field(env, obj, get_static_field_id(env, obj, name, signature), arg);
}

template <typename T, typename Arg>
void set_static_field(jholder<T> const &obj, const char *name, const char *signature, Arg const &arg)
{
    set_static_field(xbmc_jnienv(), obj, name, signature, arg);
}

template <typename Arg>
void set_static_field(JNIEnv *env, const char *clsname, const char *name, const char *signature, Arg const &arg)
{
    set_static_field(env, find_class(env, clsname), name, signature, arg);
}

template <typename Arg>
void set_static_field(const char *clsname, const char *name, const char *signature, Arg const &arg)
{
    set_static_field(xbmc_jnienv(), clsname, name, signature, arg);
}

template <typename T, typename Arg>
void set_static_field(JNIEnv *env, jholder<T> const &obj, const char *name, Arg const &arg)
{
    set_static_field(env, obj, name, details::jni_signature<Arg>::signature, arg);
}

template <typename T, typename Arg>
void set_static_field(jholder<T> const &obj, const char *name, Arg const &arg)
{
    set_static_field(xbmc_jnienv(), obj, name, arg);
}

template <typename Arg>
void set_static_field(JNIEnv *env, const char *clsname, const char *name, Arg const &arg)
{
    set_static_field(env, find_class(env, clsname), name, arg);
}

template <typename Arg>
void set_static_field(const char *clsname, const char *name, Arg const &arg)
{
    set_static_field(xbmc_jnienv(), clsname, name, arg);
}

// Warning!!! C++11 magic below: variadic templates and perfect forwarding!
// We need to define own implementation of std::forward because we don't have
// standard C++ library when building libcrystax.

namespace details
{

#if __GNUC__ == 4 && __GNUC_MINOR__ == 4

template <typename T>
struct identity {typedef T type;};

template <typename T>
inline T && forward(typename identity<T>::type && arg)
{
    return arg;
}

#else // __GNUC__ == 4 && __GNUC_MINOR__ == 4

template <typename T>
struct remove_reference
{
    typedef T type;
};

template <typename T>
struct remove_reference<T &>
{
    typedef T type;
};

template <typename T>
struct remove_reference<T &&>
{
    typedef T type;
};

template <typename T>
struct is_lvalue_reference
{
    enum {value = false};
};

template <typename T>
struct is_lvalue_reference<T &>
{
    enum {value = true};
};

template <typename T>
inline T && forward(typename remove_reference<T>::type &arg)
{
    return static_cast<T&&>(arg);
}

template <typename T>
inline T && forward(typename remove_reference<T>::type &&arg)
{
    static_assert(!is_lvalue_reference<T>::value, "template argument is an lvalue reference type");
    return static_cast<T&&>(arg);
}
#endif // __GNUC__ == 4 && __GNUC_MINOR__ == 4

} // namespace details

template <typename Ret, typename T, typename... Args>
Ret call_method(JNIEnv *env, T const &obj, jmethodID mid, Args&&... args)
{
    return details::jni_helper<Ret>::call_method(env, obj, mid, details::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
Ret call_static_method(JNIEnv *env, const jhclass &cls, const jmethodID mid, Args&&... args)
{
    return details::jni_helper<Ret>::call_static_method(env, cls, mid, details::forward<Args>(args)...);
}

template <typename... Args>
jhobject new_object(JNIEnv *env, const jhclass &cls, jmethodID mid, Args&&... args)
{
    return details::jni_stripper::new_object(env, cls, mid, details::forward<Args>(args)...);
}

template <typename Ret, typename T, typename... Args>
Ret call_method(T const &obj, jmethodID mid, Args&&... args)
{
    return call_method<Ret>(xbmc_jnienv(), obj, mid, details::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
Ret call_method(JNIEnv *env, const char *clsname, jmethodID mid, Args&&... args)
{
    return call_method<Ret>(env, find_class(env, clsname), mid, details::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
Ret call_method(const char *clsname, jmethodID mid, Args&&... args)
{
    return call_method<Ret>(xbmc_jnienv(), clsname, mid, details::forward<Args>(args)...);
}

template <typename Ret, typename T, typename... Args>
Ret call_method(JNIEnv *env, T const &obj, const char *name, const char *signature, Args&&... args)
{
    return call_method<Ret>(env, obj, get_method_id(env, obj, name, signature), details::forward<Args>(args)...);
}

template <typename Ret, typename T, typename... Args>
Ret call_method(T const &obj, const char *name, const char *signature, Args&&... args)
{
    return call_method<Ret>(xbmc_jnienv(), obj, name, signature, details::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
Ret call_method(JNIEnv *env, const char *clsname, const char *name, const char *signature, Args&&... args)
{
    return call_method<Ret>(env, find_class(env, clsname), name, signature, details::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
Ret call_method(const char *clsname, const char *name, const char *signature, Args&&... args)
{
    return call_method<Ret>(xbmc_jnienv(), clsname, name, signature, details::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
Ret call_static_method(JNIEnv *env, const jhclass &clazz, const char *name, const char *signature, Args&&... args)
{
    return call_method<Ret>(env, clazz, get_static_method_id(env, clazz, name, signature), details::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
Ret call_static_method(jhclass const &clazz, const char *name, const char *signature, Args&&... args)
{
    return call_static_method<Ret>(xbmc_jnienv(), clazz, name, signature, details::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
Ret call_static_method(JNIEnv *env, const char *clsname, const char *name, const char *signature, Args&&... args)
{
    return call_static_method<Ret>(env, find_class(env, clsname), name, signature, details::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
Ret call_static_method(const char *clsname, const char *name, const char *signature, Args&&... args)
{
    return call_static_method<Ret>(xbmc_jnienv(), clsname, name, signature, details::forward<Args>(args)...);
}

template <typename... Args>
jhobject new_object(JNIEnv *env, const jhclass &cls, const char *name, const char *signature, Args&&... args)
{
    return new_object(env, cls, get_method_id(env, cls, name, signature), details::forward<Args>(args)...);
}

template <typename... Args>
jhobject new_object(JNIEnv *env, const char *clsname, const char *name, const char *signature, Args&&... args)
{
    return new_object(env, find_class(env, clsname), name, signature, details::forward<Args>(args)...);
}

template <typename... Args>
jhobject new_object(const char *clsname, const char *name, const char *signature, Args&&... args)
{
    return new_object(xbmc_jnienv(), clsname, name, signature, details::forward<Args>(args)...);
}

template <typename... Args>
jhobject new_object(std::string clsname, const char *name, const char *signature, Args&&... args)
{
    return new_object(xbmc_jnienv(), clsname.c_str(), name, signature, details::forward<Args>(args)...);
}

template <typename... Args>
jhobject new_object(const char *clsname)
{
    return new_object(xbmc_jnienv(), clsname, "<init>", "()V");
}

template <typename... Args>
jhobject new_object(std::string clsname)
{
    return new_object(xbmc_jnienv(), clsname.c_str(), "<init>", "()V");
}

template <typename... Args>
jhobject new_object(const jhclass& cls, const char *name, const char *signature, Args&&... args)
{
    return new_object(xbmc_jnienv(), cls, name, signature, details::forward<Args>(args)...);
}

template <typename... Args>
jhobject new_object(const jhclass& cls)
{
    return new_object(xbmc_jnienv(), cls, "<init>", "()V");
}

inline void jthrow(JNIEnv *env, jhthrowable const &ex) {env->Throw(ex.get());}
inline void jthrow(jhthrowable const &ex) {jthrow(xbmc_jnienv(), ex);}

inline void jthrow(JNIEnv *env, jhclass const &cls, char const *what) {env->ThrowNew(cls.get(), what);}
inline void jthrow(jhclass const &cls, char const *what) {jthrow(xbmc_jnienv(), cls, what);}

inline void jthrow(JNIEnv *env, char const *clsname, char const *what) {jthrow(find_class(env, clsname), what);}
inline void jthrow(char const *clsname, char const *what) {jthrow(xbmc_jnienv(), clsname, what);}

bool jexcheck(JNIEnv *env);
inline bool jexcheck() {return jexcheck(xbmc_jnienv());}

} // namespace jni
