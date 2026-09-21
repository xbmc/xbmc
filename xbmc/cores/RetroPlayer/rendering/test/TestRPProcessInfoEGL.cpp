/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifdef HAS_EGL

#include "cores/RetroPlayer/playback/test/PlaybackTestEnvironment.h"
#include "cores/RetroPlayer/process/egl/RPProcessInfoEGL.h"

#ifdef TARGET_ANDROID
#include "cores/RetroPlayer/process/android/RPProcessInfoAndroid.h"
#endif

#include <gtest/gtest.h>

#include <EGL/egl.h>

using namespace KODI::RETRO;

namespace
{
std::unique_ptr<CRPProcessInfo> CreateProcessInfo()
{
#ifdef TARGET_ANDROID
  return CRPProcessInfoAndroid::Create();
#else
  return std::make_unique<CRPProcessInfoEGL>("test");
#endif
}
} // namespace

TEST(TestRPProcessInfoEGL, NullSymbolIsUnavailable)
{
  CPlaybackTestEnvironment environment;
  const auto processInfo = CreateProcessInfo();
  EXPECT_EQ(processInfo->GetHwProcedureAddress(nullptr), nullptr);
}

TEST(TestRPProcessInfoEGL, UnsupportedSymbolIsUnavailable)
{
  CPlaybackTestEnvironment environment;
  const auto processInfo = CreateProcessInfo();
  EXPECT_EQ(processInfo->GetHwProcedureAddress("kodiUnsupportedProcedure"), nullptr);
}

TEST(TestRPProcessInfoEGL, UsesEGLProcedureLookup)
{
  CPlaybackTestEnvironment environment;
  const auto processInfo = CreateProcessInfo();
  EXPECT_EQ(processInfo->GetHwProcedureAddress("glGetError"),
            static_cast<HwProcedureAddress>(eglGetProcAddress("glGetError")));
}

#if defined(TARGET_ANDROID) && HAS_GLES == 3
TEST(TestRPProcessInfoEGL, ResolvesGLES3FramebufferTextureAndSyncProcedures)
{
  CPlaybackTestEnvironment environment;
  const auto processInfo = CreateProcessInfo();
  for (const char* symbol :
       {"glGenFramebuffers", "glBindFramebuffer", "glFramebufferTexture2D", "glGenTextures",
        "glBindTexture", "glTexImage2D", "glFenceSync", "glClientWaitSync", "glDeleteSync"})
    EXPECT_NE(processInfo->GetHwProcedureAddress(symbol), nullptr) << symbol;
}
#endif

#endif
