/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUIFontManager.h"
#include "guilib/GUIFontParsing.h"

#include <stdexcept>
#include <string>
#include <thread>

#include <gtest/gtest.h>

TEST(TestGUIFontManager, MakeFontIdentDistinguishesSameBasenameDifferentPath)
{
  // Two addons each ship "MyFont.ttf" at the same size. Keyed on the basename
  // they collide in the shared CGUIFontTTF pool and the second renders the
  // first one's glyphs.
  const std::string a =
      GUIFontManager::MakeFontIdent("/addons/a/fonts/MyFont.ttf", 24.0f, 1.0f, false);
  const std::string b =
      GUIFontManager::MakeFontIdent("/addons/b/fonts/MyFont.ttf", 24.0f, 1.0f, false);

  EXPECT_NE(a, b);
}

TEST(TestGUIFontManager, MakeFontIdentSharesIdentForIdenticalPathSizeAspect)
{
  const std::string a =
      GUIFontManager::MakeFontIdent("/addons/a/fonts/MyFont.ttf", 24.0f, 1.0f, false);
  const std::string b =
      GUIFontManager::MakeFontIdent("/addons/a/fonts/MyFont.ttf", 24.0f, 1.0f, false);

  EXPECT_EQ(a, b);
}

TEST(TestGUIFontManager, MakeFontIdentDistinguishesBorder)
{
  const std::string plain = GUIFontManager::MakeFontIdent("/f/MyFont.ttf", 24.0f, 1.0f, false);
  const std::string border = GUIFontManager::MakeFontIdent("/f/MyFont.ttf", 24.0f, 1.0f, true);

  EXPECT_NE(plain, border);
}

TEST(TestGUIFontManager, MakeFontIdentDistinguishesSizeAndAspect)
{
  const std::string s24 = GUIFontManager::MakeFontIdent("/f/MyFont.ttf", 24.0f, 1.0f, false);
  const std::string s30 = GUIFontManager::MakeFontIdent("/f/MyFont.ttf", 30.0f, 1.0f, false);
  const std::string a15 = GUIFontManager::MakeFontIdent("/f/MyFont.ttf", 24.0f, 1.5f, false);

  EXPECT_NE(s24, s30);
  EXPECT_NE(s24, a15);
}

// GUIFontManager's font-loading paths need a CWinSystemBase, which kodi-test
// does not register. Scope bookkeeping does not, so it is tested directly.
//
// Constructing one is safe here: the ctor is `= default`, and the dtor's
// Clear() reaches CGUIFontTTFGL::DestroyStaticVertexBuffers(), which
// early-returns because m_staticVertexBufferCreated is false without a GL
// context (GUIFontTTFGL.cpp:474,482). Verified, not assumed.
// Subclass to reach the protected scope seams. This mirrors CGFTestable at
// TestGUIControlFactory.cpp:31 in this same directory, which is guilib's
// established way of testing protected members.
class CFontManagerTestable : public GUIFontManager
{
public:
  using GUIFontManager::FontScopeDepth;
  using GUIFontManager::InnermostFontScopeKey;
  using GUIFontManager::MarkFontScopeLoaded;
};

class TestFontScope : public ::testing::Test
{
protected:
  CFontManagerTestable m_mgr;
};

TEST_F(TestFontScope, PushAndPopReturnStackToEmpty)
{
  EXPECT_EQ(m_mgr.FontScopeDepth(), 0U);
  m_mgr.PushFontScope("/addon/MyWindow.xml");
  EXPECT_EQ(m_mgr.FontScopeDepth(), 1U);
  m_mgr.PopFontScope();
  EXPECT_EQ(m_mgr.FontScopeDepth(), 0U);
}

TEST_F(TestFontScope, NestedScopesResolveInnermostFirst)
{
  m_mgr.PushFontScope("/addon/Outer.xml");
  m_mgr.PushFontScope("/addon/Inner.xml");
  EXPECT_EQ(m_mgr.FontScopeDepth(), 2U);
  EXPECT_EQ(m_mgr.InnermostFontScopeKey(), "/addon/Inner.xml");
  m_mgr.PopFontScope();
  EXPECT_EQ(m_mgr.InnermostFontScopeKey(), "/addon/Outer.xml");
  m_mgr.PopFontScope();
}

TEST_F(TestFontScope, ScopeGuardPopsOnException)
{
  try
  {
    GUIFontManager::CScopeGuard guard(m_mgr, "/addon/MyWindow.xml");
    EXPECT_EQ(m_mgr.FontScopeDepth(), 1U);
    throw std::runtime_error("python blew up");
  }
  catch (const std::runtime_error&)
  {
  }

  EXPECT_EQ(m_mgr.FontScopeDepth(), 0U);
}

TEST_F(TestFontScope, PopOnEmptyStackIsAnErrorNotACrash)
{
  m_mgr.PopFontScope(); // logs an error
  EXPECT_EQ(m_mgr.FontScopeDepth(), 0U);

  // A zero depth alone would also hold if the underflow had corrupted the
  // container. Prove the stack still works. Task 6 makes this path reachable:
  // doAddControl pushes an empty-key guard for every non-addon window.
  m_mgr.PushFontScope("/addon/After.xml");
  EXPECT_EQ(m_mgr.InnermostFontScopeKey(), "/addon/After.xml");
  m_mgr.PopFontScope();
  EXPECT_EQ(m_mgr.FontScopeDepth(), 0U);
}

TEST_F(TestFontScope, UnloadAbsentScopeIsSilentNoOp)
{
  m_mgr.UnloadFontScope("/never/loaded.xml"); // must not throw or log an error
  EXPECT_FALSE(m_mgr.IsFontScopeLoaded("/never/loaded.xml"));
}

TEST_F(TestFontScope, TwoWindowsOfOneAddonGetDistinctScopes)
{
  // F1 regression. Keying on the addon script path collapsed a window and its
  // dialog onto one scope, so tearing down either freed the other's fonts.
  m_mgr.MarkFontScopeLoaded("/addon/MyWindow.xml");

  EXPECT_TRUE(m_mgr.IsFontScopeLoaded("/addon/MyWindow.xml"));
  EXPECT_FALSE(m_mgr.IsFontScopeLoaded("/addon/MyDialog.xml"));
}

TEST_F(TestFontScope, ScopeStackIsPerThread)
{
  // F2 regression. Control::Create resolves fonts on the Python thread while
  // the app thread may be pushing a scope for a different window.
  m_mgr.PushFontScope("/addon/AppThread.xml");

  std::thread other(
      [this]
      {
        EXPECT_EQ(m_mgr.FontScopeDepth(), 0U);
        m_mgr.PushFontScope("/addon/PythonThread.xml");
        EXPECT_EQ(m_mgr.InnermostFontScopeKey(), "/addon/PythonThread.xml");
        m_mgr.PopFontScope();
      });
  other.join();

  EXPECT_EQ(m_mgr.InnermostFontScopeKey(), "/addon/AppThread.xml");
  m_mgr.PopFontScope();
}

TEST_F(TestFontScope, EmptyScopeKeyResolvesAgainstGlobalSetOnly)
{
  // A non-WindowXML window (a plain xbmcgui.Window) has no scope key.
  // doAddControl still pushes a guard for it: the empty key must push a null
  // scope, not index m_scopedFonts[""], and resolving must skip it, not crash.
  GUIFontManager::CScopeGuard guard(m_mgr, "");
  EXPECT_EQ(m_mgr.FontScopeDepth(), 1U);
  EXPECT_EQ(m_mgr.InnermostFontScopeKey(), "");
  EXPECT_EQ(m_mgr.GetFont("font12", false), nullptr); // no crash on null scope
}

TEST_F(TestFontScope, DistinctResolvedPathsOfOneAddonOwnFontsIndependently)
{
  // F1 fix, pinned at the reachable level. The key derivation lives in
  // WindowXML's constructor (m_fontScopeKey = strSkinPath, the RESOLVED window
  // XML path, not the shared script path). WindowXML cannot be constructed
  // under kodi-test (it needs a skin, the window manager and Python), so the
  // string derivation itself is verified manually. What IS reachable, and what
  // actually makes the fix safe, is that two windows of one addon that resolve
  // to different XML paths own and tear down their fonts independently: closing
  // one must not free the other's. Keying on the shared script path would make
  // these one scope and reintroduce the use-after-free.
  const std::string window = "/addon/resources/skins/Default/720p/MyWindow.xml";
  const std::string dialog = "/addon/resources/skins/Default/720p/MyDialog.xml";

  m_mgr.MarkFontScopeLoaded(window);
  m_mgr.MarkFontScopeLoaded(dialog);
  ASSERT_TRUE(m_mgr.IsFontScopeLoaded(window));
  ASSERT_TRUE(m_mgr.IsFontScopeLoaded(dialog));

  // Tear down the dialog, as an ordinary forced unload would.
  m_mgr.UnloadFontScope(dialog);

  // The window's scope, and thus its fonts, survive.
  EXPECT_TRUE(m_mgr.IsFontScopeLoaded(window));
  EXPECT_FALSE(m_mgr.IsFontScopeLoaded(dialog));
}
