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
#include <android/log.h>
#include <vector>

JNIEnv *xbmc_jnienv();

namespace jni
{
template <typename T>
class jholder
{
    typedef void (jholder::*safe_bool_type)();
    void non_null_object() {}

public:
    explicit jholder(T obj = 0)
        :object(obj)
        ,global(false)
    {
    }

    jholder(jholder const &c)
        :object(c.object ? (T)xbmc_jnienv()->NewLocalRef(c.object) : 0)
        ,global(false)
    {
    }


    template <typename U>
    explicit jholder(jholder<U> const &c)
        :object(c.object ? (T)xbmc_jnienv()->NewLocalRef(c.object) : 0)
        ,global(false)
    {
    }

    ~jholder()
    {
      reset();
    }

    jholder &operator=(jholder const &c)
    {
        jholder tmp(c);
        swap(tmp);
        return *this;
    }

    void reset(T obj = 0)
    {
        if (object)
        {
          if(global)
            xbmc_jnienv()->DeleteGlobalRef(object);
          else
            xbmc_jnienv()->DeleteLocalRef(object);
        }
        object = obj;
        global = false;
    }

    const T& get() const {return object;}

    T release()
    {
        T ret = object;
        object = 0;
        global = false;
        return ret;
    }

    void setGlobal()
    {
      if(object)
      {
        T globalRef = (T)xbmc_jnienv()->NewGlobalRef(object);
        reset(globalRef);
        global = true;
      }
    }

    bool operator!() const {return !object;}
    operator safe_bool_type () const
    {
        return object ? &jholder::non_null_object : 0;
    }

    operator const T&() const
    {
      return object;
    }

private:
    void swap(jholder &c)
    {
        T tmp = object;
        object = c.object;
        c.object = tmp;
    }

    T object;
    bool global;
};

typedef jholder<jclass> jhclass;
typedef jholder<jobject> jhobject;
typedef jholder<jstring> jhstring;
typedef jholder<jthrowable> jhthrowable;
typedef jholder<jarray> jharray;
typedef jholder<jbyteArray> jhbyteArray;
typedef jholder<jbooleanArray> jhbooleanArray;
typedef jholder<jcharArray> jhcharArray;
typedef jholder<jshortArray> jhshortArray;
typedef jholder<jintArray> jhintArray;
typedef jholder<jlongArray> jhlongArray;
typedef jholder<jfloatArray> jhfloatArray;
typedef jholder<jdoubleArray> jhdoubleArray;
typedef jholder<jobjectArray> jhobjectArray;

} // namespace jni

