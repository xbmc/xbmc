/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/rendering/contexts/HwRenderingContextEGLUtils.h"

#include <gtest/gtest.h>

using namespace KODI::RETRO;

TEST(TestHwRenderingContextEGL, EGL14RequiresBothExtensions)
{
  EXPECT_FALSE(
      SupportsEGLHardwareRendering("1.3", "EGL_KHR_surfaceless_context EGL_KHR_create_context"));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.4", ""));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.4", "EGL_KHR_create_context"));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.4", "EGL_KHR_surfaceless_context"));
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.4 vendor",
                                           "EGL_KHR_surfaceless_context EGL_KHR_create_context"));
}

TEST(TestHwRenderingContextEGL, EGL15DoesNotRequireExtensionStrings)
{
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.5", ""));
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.5 vendor", nullptr));
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.10", ""));
  EXPECT_TRUE(SupportsEGLHardwareRendering("2.0", ""));
}

TEST(TestHwRenderingContextEGL, FailedQueriesAndMalformedVersionsAreRejected)
{
  EXPECT_FALSE(SupportsEGLHardwareRendering(nullptr, nullptr));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.4", nullptr));
  for (const char* version : {"", "garbage", "1", "1.", "1.5junk", "-1.5", "1.999999999999999"})
    EXPECT_FALSE(
        SupportsEGLHardwareRendering(version, "EGL_KHR_surfaceless_context EGL_KHR_create_context"))
        << version;
}

TEST(TestHwRenderingContextEGL, ExtensionNamesMustBeCompleteTokens)
{
  EXPECT_FALSE(SupportsEGLHardwareRendering(
      "1.4", "EGL_KHR_surfaceless_context_extra EGL_KHR_create_context"));
  EXPECT_FALSE(SupportsEGLHardwareRendering(
      "1.4", "EGL_KHR_surfaceless_context not_EGL_KHR_create_context"));
  EXPECT_TRUE(SupportsEGLHardwareRendering(
      "1.4",
      " EGL_KHR_surfaceless_context_extra EGL_KHR_create_context  EGL_KHR_surfaceless_context "));
}

#if defined(HAS_EGL)
TEST(TestHwRenderingContextEGL, OrdinaryContextPreservesVersionAndProfile)
{
  HwContextProperties properties;
  EXPECT_EQ(BuildEGLContextAttributes(properties, 4, 1, "1.5"),
            (std::vector<EGLint>{EGL_CONTEXT_MAJOR_VERSION_KHR, 4, EGL_CONTEXT_MINOR_VERSION_KHR, 1,
                                 EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
                                 EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR, EGL_NONE}));
  properties.coreProfile = false;
  EXPECT_EQ(BuildEGLContextAttributes(properties, 3, 2, "1.4"),
            (std::vector<EGLint>{EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 2,
                                 EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
                                 EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR, EGL_NONE}));
}

TEST(TestHwRenderingContextEGL, EGL15DebugUsesBooleanAttribute)
{
  HwContextProperties properties;
  properties.debugContext = true;
  EXPECT_EQ(BuildEGLContextAttributes(properties, 4, 1, "1.5 vendor"),
            (std::vector<EGLint>{EGL_CONTEXT_MAJOR_VERSION_KHR, 4, EGL_CONTEXT_MINOR_VERSION_KHR, 1,
                                 EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
                                 EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR, EGL_CONTEXT_OPENGL_DEBUG,
                                 EGL_TRUE, EGL_NONE}));
}

TEST(TestHwRenderingContextEGL, EGL14DebugUsesKHRFlags)
{
  HwContextProperties properties;
  properties.debugContext = true;
  EXPECT_EQ(BuildEGLContextAttributes(properties, 3, 2, "1.4 vendor"),
            (std::vector<EGLint>{EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 2,
                                 EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
                                 EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR, EGL_CONTEXT_FLAGS_KHR,
                                 EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR, EGL_NONE}));
}

TEST(TestHwRenderingContextEGL, EmbeddedDebugContextDoesNotRequestDesktopProfile)
{
  HwContextProperties properties;
  properties.embedded = true;
  properties.debugContext = true;
  for (const char* version : {"1.4", "1.5"})
  {
    const bool coreAttributes = SupportsEGLHardwareRendering(version, nullptr);
    EXPECT_EQ(BuildEGLContextAttributes(properties, 3, 0, version),
              (std::vector<EGLint>{
                  EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_CONTEXT_MINOR_VERSION_KHR, 0,
                  coreAttributes ? EGL_CONTEXT_OPENGL_DEBUG : EGL_CONTEXT_FLAGS_KHR,
                  coreAttributes ? EGL_TRUE : EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR, EGL_NONE}));
  }
}
#endif
