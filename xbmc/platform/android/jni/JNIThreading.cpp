/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *      Some code borrowed from Dmitry Moskalchuk.
 *
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
/*
  This code is mostly borrowed from libcrystax. The functions and namespaces
  were renamed to avoid collisions when linking against the originals.
*/

#include <jni.h>
#include <pthread.h>
#include <stdlib.h>
#include <android/log.h>
namespace xbmcjni
{

static JavaVM *s_jvm = NULL;
static pthread_key_t s_jnienv_key;
static pthread_once_t s_jnienv_key_once = PTHREAD_ONCE_INIT;

JavaVM *jvm()
{
    return s_jvm;
}

static void jnienv_detach_thread(void *arg)
{
  (void)arg;
  if (!jvm())
    return;
  __android_log_print(ANDROID_LOG_VERBOSE, "XBMC","detaching thread");
  jvm()->DetachCurrentThread();
}

static void jnienv_key_create()
{
    if (::pthread_key_create(&s_jnienv_key, &jnienv_detach_thread) != 0)
        ::abort();
}

static bool save_jnienv(JNIEnv *env)
{

    ::pthread_once(&s_jnienv_key_once, &jnienv_key_create);

    if (::pthread_setspecific(s_jnienv_key, env) != 0)
        return false;
    return true;
}

JNIEnv *jnienv()
{
    ::pthread_once(&s_jnienv_key_once, &jnienv_key_create);

    JNIEnv *env = reinterpret_cast<JNIEnv *>(::pthread_getspecific(s_jnienv_key));
    if (!env && jvm())
    {
        jvm()->AttachCurrentThread(&env, NULL);
        if (!save_jnienv(env))
            ::abort();
    }
    return env;
}

} // namespace xbmcjni

JavaVM *xbmc_jvm()
{
    return ::xbmcjni::jvm();
}

JNIEnv *xbmc_jnienv()
{
    return ::xbmcjni::jnienv();
}

void xbmc_save_jnienv(JNIEnv *env)
{
    ::xbmcjni::save_jnienv(env);
}

jint xbmc_jni_on_load(JavaVM *vm, JNIEnv *env)
{
    jint jversion = JNI_VERSION_1_4;

    if (!env)
        return -1;

    ::xbmcjni::s_jvm = vm;

    // The main thread hands us an env. Attach and store it.
    ::xbmcjni::jvm()->AttachCurrentThread(&env, NULL);
    if (!::xbmcjni::save_jnienv(env))
        return -1;

    return jversion;
}

void xbmc_jni_on_unload()
{
  ::xbmcjni::jnienv_detach_thread(NULL);
  ::xbmcjni::s_jvm = NULL;
}
