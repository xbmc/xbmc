/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "cores/RetroPlayer/rendering/contexts/HwRenderingContextEGLUtils.h"

#if defined(HAS_EGL)
#include "cores/RetroPlayer/rendering/contexts/EGLClientContext.h"

#include <algorithm>
#include <array>
#endif

#include <gtest/gtest.h>

using namespace KODI::RETRO;

TEST(TestHwRenderingContextEGL, EGL14AcceptsPbufferFallback)
{
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.4 Android META-EGL", "EGL_KHR_create_context", true,
                                           3, 0, true));
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.4", "EGL_KHR_create_context", true, 3, 0, true));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.4", "EGL_KHR_create_context", true, 3, 0, false));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.4", "", true, 3, 0, true));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.3", "EGL_KHR_create_context", true, 3, 0, true));
}

TEST(TestHwRenderingContextEGL, BindingAndContextAttributesAreIndependent)
{
  const auto core = GetEGLCapabilities("1.5", nullptr);
  EXPECT_TRUE(core.createContext);
  EXPECT_TRUE(core.surfaceless);
  EXPECT_TRUE(core.coreContextAttributes);
  const auto pbuffer = GetEGLCapabilities("1.4", "EGL_KHR_create_context");
  EXPECT_TRUE(pbuffer.createContext);
  EXPECT_FALSE(pbuffer.surfaceless);
  EXPECT_FALSE(pbuffer.coreContextAttributes);
  const auto extensions =
      GetEGLCapabilities("1.4", "EGL_KHR_create_context EGL_KHR_surfaceless_context");
  EXPECT_TRUE(extensions.createContext);
  EXPECT_TRUE(extensions.surfaceless);
  EXPECT_FALSE(extensions.coreContextAttributes);
}

TEST(TestHwRenderingContextEGL, ActualES2GuiRejectsHardwareRendering)
{
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.5", nullptr, true, 2, 0));
  EXPECT_FALSE(SupportsEGLHardwareRendering(
      "1.4", "EGL_KHR_surfaceless_context EGL_KHR_create_context", true, 2, 0));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.4", "EGL_KHR_create_context", true, 2, 0, true));
}

TEST(TestHwRenderingContextEGL, ActualES3GuiAcceptsSurfacelessHardwareRendering)
{
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.5", nullptr, true, 3, 0));
  EXPECT_TRUE(SupportsEGLHardwareRendering(
      "1.4", "EGL_KHR_surfaceless_context EGL_KHR_create_context", true, 3, 0));
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.5", nullptr, true, 3, 2));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.4", nullptr, true, 3, 0));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.3", nullptr, true, 3, 0));
  EXPECT_FALSE(SupportsEGLHardwareRendering(nullptr, nullptr, true, 3, 0));
}

TEST(TestHwRenderingContextEGL, DesktopGuiRequirementsAreUnchanged)
{
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.5", nullptr, false, 2, 0));
  EXPECT_FALSE(SupportsEGLHardwareRendering("1.5", nullptr, false, 3, 1));
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.5", nullptr, false, 3, 2));
  EXPECT_TRUE(SupportsEGLHardwareRendering("1.5", nullptr, false, 4, 1));
}

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
namespace
{
struct FakeEGLConfigs
{
  static inline FakeEGLConfigs* current{};
  EGLint guiRenderable{EGL_OPENGL_ES3_BIT}, guiSurfaces{EGL_WINDOW_BIT | EGL_PBUFFER_BIT};
  EGLint requiredSurfaces{-1};
  std::array<EGLint, 4> colorSizes{8, 8, 8, 8};
  std::vector<EGLConfig> matches{reinterpret_cast<EGLConfig>(2), reinterpret_cast<EGLConfig>(3)};
  std::vector<EGLint> requestedAttributes;
  bool failQuery{false}, failChoose{false};
  int chooseCalls{0};
  EGLConfigFunctions Functions()
  {
    current = this;
    return {[](EGLDisplay, EGLConfig, EGLint attribute, EGLint* value) -> EGLBoolean
            {
              if (current->failQuery)
                return EGL_FALSE;
              *value =
                  attribute == EGL_RENDERABLE_TYPE ? current->guiRenderable : current->guiSurfaces;
              return EGL_TRUE;
            },
            [](EGLDisplay, const EGLint* attributes, EGLConfig* configs, EGLint size,
               EGLint* count) -> EGLBoolean
            {
              ++current->chooseCalls;
              bool matchesColor = true;
              current->requestedAttributes.clear();
              for (int i = 0; attributes[i] != EGL_NONE; i += 2)
              {
                current->requestedAttributes.insert(current->requestedAttributes.end(),
                                                    {attributes[i], attributes[i + 1]});
                if (attributes[i] == EGL_SURFACE_TYPE)
                  current->requiredSurfaces = attributes[i + 1];
                if (attributes[i] == EGL_RENDERABLE_TYPE)
                {
                  EXPECT_EQ(attributes[i + 1], EGL_OPENGL_ES3_BIT);
                }
                const EGLint colorAttributes[]{EGL_RED_SIZE, EGL_GREEN_SIZE, EGL_BLUE_SIZE,
                                               EGL_ALPHA_SIZE};
                for (size_t channel = 0; channel < current->colorSizes.size(); ++channel)
                {
                  if (attributes[i] == colorAttributes[channel])
                    matchesColor &= attributes[i + 1] <= current->colorSizes[channel];
                }
              }
              current->requestedAttributes.push_back(EGL_NONE);
              if (current->failChoose)
                return EGL_FALSE;
              *count = matchesColor ? static_cast<EGLint>(current->matches.size()) : 0;
              if (configs)
              {
                *count = std::min(size, *count);
                std::copy_n(current->matches.begin(), *count, configs);
              }
              return EGL_TRUE;
            }};
  }
};
} // namespace

TEST(TestHwRenderingContextEGL, ReusesGUIConfigWithES3AndPbuffer)
{
  FakeEGLConfigs egl;
  EXPECT_EQ(GetEGLClientConfigs(reinterpret_cast<EGLDisplay>(1), reinterpret_cast<EGLConfig>(1),
                                EGL_OPENGL_ES3_BIT, false, egl.Functions()),
            (std::vector<EGLConfig>{reinterpret_cast<EGLConfig>(1), reinterpret_cast<EGLConfig>(2),
                                    reinterpret_cast<EGLConfig>(3)}));
  EXPECT_EQ(egl.chooseCalls, 2);
}

TEST(TestHwRenderingContextEGL, SurfacelessAcceptsWindowOnlyGBMConfig)
{
  FakeEGLConfigs egl;
  egl.guiSurfaces = EGL_WINDOW_BIT;
  EXPECT_EQ(GetEGLClientConfigs(reinterpret_cast<EGLDisplay>(1), reinterpret_cast<EGLConfig>(1),
                                EGL_OPENGL_ES3_BIT, true, egl.Functions()),
            (std::vector<EGLConfig>{reinterpret_cast<EGLConfig>(1), reinterpret_cast<EGLConfig>(2),
                                    reinterpret_cast<EGLConfig>(3)}));
  EXPECT_EQ(egl.requiredSurfaces, 0);
}

TEST(TestHwRenderingContextEGL, GUIConfigWithoutClientAPIBitEnumeratesAlternatives)
{
  FakeEGLConfigs egl;
  egl.guiRenderable = EGL_OPENGL_ES2_BIT;
  EXPECT_EQ(
      GetEGLClientConfigs(reinterpret_cast<EGLDisplay>(1), reinterpret_cast<EGLConfig>(1),
                          EGL_OPENGL_ES3_BIT, false, egl.Functions()),
      (std::vector<EGLConfig>{reinterpret_cast<EGLConfig>(2), reinterpret_cast<EGLConfig>(3)}));
  EXPECT_EQ(egl.requiredSurfaces, EGL_PBUFFER_BIT);
}

TEST(TestHwRenderingContextEGL, SurfacelessSelectionDoesNotRequirePbuffer)
{
  FakeEGLConfigs egl;
  egl.guiRenderable = EGL_OPENGL_ES2_BIT;
  EXPECT_FALSE(GetEGLClientConfigs(reinterpret_cast<EGLDisplay>(1), reinterpret_cast<EGLConfig>(1),
                                   EGL_OPENGL_ES3_BIT, true, egl.Functions())
                   .empty());
  EXPECT_EQ(egl.requiredSurfaces, 0);
  EXPECT_EQ(egl.requestedAttributes, (std::vector<EGLint>{EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                                                          EGL_SURFACE_TYPE, 0, EGL_NONE}));
}

TEST(TestHwRenderingContextEGL, GUIConfigAndAlternatesAreNotDuplicated)
{
  FakeEGLConfigs egl;
  egl.matches = {reinterpret_cast<EGLConfig>(2), reinterpret_cast<EGLConfig>(1),
                 reinterpret_cast<EGLConfig>(3), reinterpret_cast<EGLConfig>(1),
                 reinterpret_cast<EGLConfig>(2)};
  EXPECT_EQ(GetEGLClientConfigs(reinterpret_cast<EGLDisplay>(1), reinterpret_cast<EGLConfig>(1),
                                EGL_OPENGL_ES3_BIT, false, egl.Functions()),
            (std::vector<EGLConfig>{reinterpret_cast<EGLConfig>(1), reinterpret_cast<EGLConfig>(2),
                                    reinterpret_cast<EGLConfig>(3)}));
}

TEST(TestHwRenderingContextEGL, FailedOrEmptyEnumerationRetainsSuitableGUIConfig)
{
  FakeEGLConfigs egl;
  egl.matches.clear();
  EXPECT_EQ(GetEGLClientConfigs(reinterpret_cast<EGLDisplay>(1), reinterpret_cast<EGLConfig>(1),
                                EGL_OPENGL_ES3_BIT, false, egl.Functions()),
            (std::vector<EGLConfig>{reinterpret_cast<EGLConfig>(1)}));
  egl.failChoose = true;
  EXPECT_EQ(GetEGLClientConfigs(reinterpret_cast<EGLDisplay>(1), reinterpret_cast<EGLConfig>(1),
                                EGL_OPENGL_ES3_BIT, false, egl.Functions()),
            (std::vector<EGLConfig>{reinterpret_cast<EGLConfig>(1)}));
}

TEST(TestHwRenderingContextEGL, RGB565PbufferConfigRemainsEligible)
{
  FakeEGLConfigs egl;
  egl.guiRenderable = EGL_OPENGL_ES2_BIT;
  egl.colorSizes = {5, 6, 5, 0};
  egl.matches = {reinterpret_cast<EGLConfig>(3)};
  EXPECT_EQ(GetEGLClientConfigs(reinterpret_cast<EGLDisplay>(1), reinterpret_cast<EGLConfig>(1),
                                EGL_OPENGL_ES3_BIT, false, egl.Functions()),
            (std::vector<EGLConfig>{reinterpret_cast<EGLConfig>(3)}));
  EXPECT_EQ(egl.requestedAttributes,
            (std::vector<EGLint>{EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT, EGL_SURFACE_TYPE,
                                 EGL_PBUFFER_BIT, EGL_NONE}));
}

TEST(TestHwRenderingContextEGL, MissingPbufferAndFailedConfigQueriesRejectCleanly)
{
  FakeEGLConfigs egl;
  egl.guiSurfaces = EGL_WINDOW_BIT;
  egl.matches.clear();
  EXPECT_TRUE(GetEGLClientConfigs(reinterpret_cast<EGLDisplay>(1), reinterpret_cast<EGLConfig>(1),
                                  EGL_OPENGL_ES3_BIT, false, egl.Functions())
                  .empty());
  egl.failQuery = egl.failChoose = true;
  EXPECT_TRUE(GetEGLClientConfigs(reinterpret_cast<EGLDisplay>(1), reinterpret_cast<EGLConfig>(1),
                                  EGL_OPENGL_ES3_BIT, false, egl.Functions())
                  .empty());
}

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

#if defined(HAS_EGL)
namespace
{
struct FakeEGL
{
  static inline FakeEGL* current{};
  EGLenum api{EGL_OPENGL_API};
  EGLDisplay display{reinterpret_cast<EGLDisplay>(1)};
  EGLContext context{reinterpret_cast<EGLContext>(2)};
  EGLSurface draw{reinterpret_cast<EGLSurface>(3)}, read{reinterpret_cast<EGLSurface>(4)};
  EGLSurface clientSurface{reinterpret_cast<EGLSurface>(5)};
  EGLContext clientContext{reinterpret_cast<EGLContext>(6)};
  bool failSurface{false}, failContext{false}, failBind{false}, failRestore{false};
  bool failDestroy{false}, failUnbind{false};
  EGLConfig failSurfaceConfig{}, failContextConfig{};
  int surfaces{0}, contexts{0}, releases{0};
  std::vector<char> destroyed;

  EGLClientFunctions Functions()
  {
    current = this;
    return {
        []() -> EGLint { return EGL_BAD_MATCH; },
        []() -> EGLenum { return current->api; },
        [](EGLenum api) -> EGLBoolean
        {
          current->api = api;
          return EGL_TRUE;
        },
        []() -> EGLDisplay { return current->display; },
        [](EGLint which) -> EGLSurface
        { return which == EGL_DRAW ? current->draw : current->read; },
        []() -> EGLContext { return current->context; },
        [](EGLDisplay, EGLConfig config, const EGLint* attributes) -> EGLSurface
        {
          EXPECT_EQ(attributes[0], EGL_WIDTH);
          EXPECT_EQ(attributes[1], 1);
          EXPECT_EQ(attributes[2], EGL_HEIGHT);
          EXPECT_EQ(attributes[3], 1);
          if (current->failSurface || config == current->failSurfaceConfig)
            return EGL_NO_SURFACE;
          ++current->surfaces;
          return current->clientSurface;
        },
        [](EGLDisplay, EGLConfig config, EGLContext shared, const EGLint*) -> EGLContext
        {
          EXPECT_EQ(shared, reinterpret_cast<EGLContext>(2));
          EXPECT_EQ(current->api, EGL_OPENGL_ES_API);
          if (current->failContext || config == current->failContextConfig)
            return EGL_NO_CONTEXT;
          ++current->contexts;
          return current->clientContext;
        },
        [](EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context) -> EGLBoolean
        {
          if ((context == current->clientContext && current->failBind) ||
              (context == reinterpret_cast<EGLContext>(2) && current->failRestore) ||
              (context == EGL_NO_CONTEXT && current->failUnbind))
            return EGL_FALSE;
          current->display = context == EGL_NO_CONTEXT ? EGL_NO_DISPLAY : display;
          current->context = context;
          current->draw = draw;
          current->read = read;
          return EGL_TRUE;
        },
        [](EGLDisplay, EGLContext context) -> EGLBoolean
        {
          EXPECT_EQ(context, current->clientContext);
          EXPECT_NE(current->context, context);
          --current->contexts;
          current->destroyed.push_back('c');
          return current->failDestroy ? EGL_FALSE : EGL_TRUE;
        },
        [](EGLDisplay, EGLSurface surface) -> EGLBoolean
        {
          EXPECT_EQ(surface, current->clientSurface);
          EXPECT_NE(current->draw, surface);
          EXPECT_NE(current->read, surface);
          --current->surfaces;
          current->destroyed.push_back('s');
          return current->failDestroy ? EGL_FALSE : EGL_TRUE;
        },
        []() -> EGLBoolean
        {
          ++current->releases;
          current->context = EGL_NO_CONTEXT;
          current->display = EGL_NO_DISPLAY;
          current->draw = current->read = EGL_NO_SURFACE;
          current->api = EGL_OPENGL_ES_API;
          return EGL_TRUE;
        }};
  }
};

class TestEGLClientContext : public testing::Test
{
protected:
  FakeEGL egl;
  CEGLClientContext client{EGL_OPENGL_ES_API, egl.Functions()};
  bool Create(bool surfaceless = false)
  {
    const EGLint attributes[]{EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_NONE};
    return client.Create(reinterpret_cast<EGLDisplay>(1), reinterpret_cast<EGLConfig>(7),
                         reinterpret_cast<EGLContext>(2), attributes, surfaceless);
  }
  void ExpectGUI()
  {
    EXPECT_EQ(egl.api, EGL_OPENGL_API);
    EXPECT_EQ(egl.display, reinterpret_cast<EGLDisplay>(1));
    EXPECT_EQ(egl.context, reinterpret_cast<EGLContext>(2));
    EXPECT_EQ(egl.draw, reinterpret_cast<EGLSurface>(3));
    EXPECT_EQ(egl.read, reinterpret_cast<EGLSurface>(4));
  }
};
} // namespace

TEST_F(TestEGLClientContext, PbufferBindRestoresDistinctGUISurfaces)
{
  ASSERT_TRUE(Create());
  ExpectGUI();
  ASSERT_TRUE(client.MakeCurrent());
  EXPECT_EQ(egl.draw, egl.clientSurface);
  EXPECT_EQ(egl.read, egl.clientSurface);
  EXPECT_EQ(egl.context, egl.clientContext);
  EXPECT_TRUE(client.RestoreCurrent());
  ExpectGUI();
  client.Destroy();
  EXPECT_EQ(egl.surfaces, 0);
  EXPECT_EQ(egl.contexts, 0);
  EXPECT_EQ(egl.destroyed, (std::vector<char>{'c', 's'}));
  ExpectGUI();
}

TEST_F(TestEGLClientContext, SurfacelessNeverAllocatesASurface)
{
  ASSERT_TRUE(Create(true));
  ASSERT_TRUE(client.MakeCurrent());
  EXPECT_EQ(egl.draw, EGL_NO_SURFACE);
  EXPECT_EQ(egl.read, EGL_NO_SURFACE);
  EXPECT_TRUE(client.RestoreCurrent());
  client.Destroy();
  EXPECT_EQ(egl.destroyed, (std::vector<char>{'c'}));
  ExpectGUI();
}

TEST_F(TestEGLClientContext, SurfaceCreationFailureCanRecover)
{
  egl.failSurface = true;
  EXPECT_FALSE(Create());
  EXPECT_FALSE(client.IsCreated());
  ExpectGUI();
  egl.failSurface = false;
  EXPECT_TRUE(Create());
}

TEST_F(TestEGLClientContext, ContextCreationFailureReleasesPbuffer)
{
  egl.failContext = true;
  EXPECT_FALSE(Create());
  EXPECT_FALSE(client.IsCreated());
  EXPECT_EQ(egl.surfaces, 0);
  EXPECT_EQ(egl.destroyed, (std::vector<char>{'s'}));
  ExpectGUI();
  egl.failContext = false;
  EXPECT_TRUE(Create());
}

TEST_F(TestEGLClientContext, FailedGUICandidateCanUseAlternateConfig)
{
  FakeEGLConfigs configs;
  const auto candidates =
      GetEGLClientConfigs(reinterpret_cast<EGLDisplay>(1), reinterpret_cast<EGLConfig>(1),
                          EGL_OPENGL_ES3_BIT, false, configs.Functions());
  for (const bool failSurface : {true, false})
  {
    SCOPED_TRACE(failSurface ? "pbuffer creation failure" : "shared context creation failure");
    egl.failSurfaceConfig = failSurface ? reinterpret_cast<EGLConfig>(1) : nullptr;
    egl.failContextConfig = failSurface ? nullptr : reinterpret_cast<EGLConfig>(1);
    const EGLint attributes[]{EGL_CONTEXT_MAJOR_VERSION_KHR, 3, EGL_NONE};
    EGLConfig selected{};
    for (const auto config : candidates)
    {
      if (client.Create(reinterpret_cast<EGLDisplay>(1), config, reinterpret_cast<EGLContext>(2),
                        attributes, false))
      {
        selected = config;
        break;
      }
    }
    ASSERT_EQ(selected, reinterpret_cast<EGLConfig>(2));
    ASSERT_TRUE(client.MakeCurrent());
    EXPECT_TRUE(client.RestoreCurrent());
    ExpectGUI();
    client.Destroy();
    EXPECT_EQ(egl.surfaces, 0);
    EXPECT_EQ(egl.contexts, 0);
  }
}

TEST_F(TestEGLClientContext, BindFailurePreservesGUIAndCanRecover)
{
  ASSERT_TRUE(Create());
  egl.failBind = true;
  EXPECT_FALSE(client.MakeCurrent());
  ExpectGUI();
  client.Destroy();
  ExpectGUI();
  EXPECT_EQ(egl.surfaces, 0);
  egl.failBind = false;
  ASSERT_TRUE(Create());
  EXPECT_TRUE(client.MakeCurrent());
}

TEST_F(TestEGLClientContext, RestoreFailureUnbindsClient)
{
  ASSERT_TRUE(Create());
  ASSERT_TRUE(client.MakeCurrent());
  egl.failRestore = true;
  EXPECT_FALSE(client.RestoreCurrent());
  EXPECT_EQ(egl.context, EGL_NO_CONTEXT);
  EXPECT_EQ(egl.draw, EGL_NO_SURFACE);
  EXPECT_EQ(egl.api, EGL_OPENGL_API);
}

TEST_F(TestEGLClientContext, FailedUnbindReleasesThread)
{
  ASSERT_TRUE(Create());
  ASSERT_TRUE(client.MakeCurrent());
  egl.failRestore = egl.failUnbind = true;
  EXPECT_FALSE(client.RestoreCurrent());
  EXPECT_EQ(egl.context, EGL_NO_CONTEXT);
  EXPECT_GT(egl.releases, 0);
  EXPECT_EQ(egl.api, EGL_OPENGL_API);
}

TEST_F(TestEGLClientContext, RepeatedCreateDestroysCurrentClientAndRestoresGUI)
{
  for (int i = 0; i < 3; ++i)
  {
    ASSERT_TRUE(Create());
    ExpectGUI();
    EXPECT_TRUE(client.MakeCurrent());
  }
  client.Destroy();
  ExpectGUI();
  EXPECT_EQ(egl.surfaces, 0);
  EXPECT_EQ(egl.contexts, 0);
}

TEST_F(TestEGLClientContext, DuplicateBindCannotSaveClientAsPreviousContext)
{
  ASSERT_TRUE(Create());
  ASSERT_TRUE(client.MakeCurrent());
  EXPECT_FALSE(client.MakeCurrent());
  EXPECT_TRUE(client.RestoreCurrent());
  EXPECT_TRUE(client.RestoreCurrent());
  ExpectGUI();
}

TEST_F(TestEGLClientContext, RestoresContextOnAnotherDisplay)
{
  egl.display = reinterpret_cast<EGLDisplay>(9);
  ASSERT_TRUE(Create());
  ASSERT_TRUE(client.MakeCurrent());
  EXPECT_TRUE(client.RestoreCurrent());
  EXPECT_EQ(egl.display, reinterpret_cast<EGLDisplay>(9));
  EXPECT_EQ(egl.draw, reinterpret_cast<EGLSurface>(3));
  EXPECT_EQ(egl.read, reinterpret_cast<EGLSurface>(4));
}

TEST_F(TestEGLClientContext, SameAPIRestorationFailureUnbindsClient)
{
  egl.api = EGL_OPENGL_ES_API;
  ASSERT_TRUE(Create());
  ASSERT_TRUE(client.MakeCurrent());
  egl.failRestore = true;
  EXPECT_FALSE(client.RestoreCurrent());
  EXPECT_EQ(egl.context, EGL_NO_CONTEXT);
  EXPECT_EQ(egl.draw, EGL_NO_SURFACE);
  EXPECT_EQ(egl.read, EGL_NO_SURFACE);
}

TEST_F(TestEGLClientContext, ThreadWithoutPreviousContextIsUnboundAfterRestore)
{
  egl.context = EGL_NO_CONTEXT;
  egl.display = EGL_NO_DISPLAY;
  egl.draw = egl.read = EGL_NO_SURFACE;
  ASSERT_TRUE(Create());
  ASSERT_TRUE(client.MakeCurrent());
  EXPECT_TRUE(client.RestoreCurrent());
  EXPECT_EQ(egl.context, EGL_NO_CONTEXT);
  EXPECT_EQ(egl.display, EGL_NO_DISPLAY);
  EXPECT_EQ(egl.api, EGL_OPENGL_API);
}

TEST_F(TestEGLClientContext, InvalidSharedContextNeverCreatesSurface)
{
  const EGLint attributes[]{EGL_NONE};
  EXPECT_FALSE(client.Create(reinterpret_cast<EGLDisplay>(1), reinterpret_cast<EGLConfig>(7),
                             EGL_NO_CONTEXT, attributes, false));
  EXPECT_EQ(egl.surfaces, 0);
  EXPECT_EQ(egl.contexts, 0);
  ExpectGUI();
}

TEST_F(TestEGLClientContext, DestroyFailuresAbandonHandlesAndAllowAnotherSession)
{
  ASSERT_TRUE(Create());
  egl.failDestroy = true;
  client.Destroy();
  EXPECT_FALSE(client.IsCreated());
  EXPECT_EQ(egl.destroyed, (std::vector<char>{'c', 's'}));
  egl.failDestroy = false;
  ASSERT_TRUE(Create());
  EXPECT_TRUE(client.MakeCurrent());
}
#endif
