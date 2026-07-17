/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUILabel.h"

#include <gtest/gtest.h>

// Regression test for the bug reported where CGUIButtonControl::PythonSetLabel (and thus
// xbmcgui.ControlButton.setLabel()/ControlRadioButton.setLabel()) changed the font stored in the
// label's CLabelInfo but never reached the CGUITextLayout that actually performs the rendering.
// Because CGUITextLayout::Update() short-circuits when the text string itself hasn't changed, the
// font change was silently dropped whenever the label text stayed the same. CGUILabel::SetLabelFont
// must both update the label info and force the text layout to re-run on the next SetText() call,
// even for otherwise unchanged text.
TEST(TestGUILabel, SetLabelFontForcesRelayoutOfUnchangedText)
{
  CLabelInfo labelInfo;
  ASSERT_EQ(labelInfo.font, nullptr);

  CGUILabel label(0, 0, 100, 100, labelInfo, CGUILabel::OVER_FLOW_TRUNCATE);

  // Initial SetText() always triggers a layout since the label starts invalid.
  EXPECT_TRUE(label.SetText("hello"));

  // Setting the exact same text again should not require any relayout.
  EXPECT_FALSE(label.SetText("hello"));

  // Changing the label's font (as PythonSetLabel does) must invalidate the cached layout, so that
  // the very next SetText() call - even with unchanged text - is forced to relayout and pick up
  // the new font.
  label.SetLabelFont(nullptr);
  EXPECT_EQ(label.GetLabelInfo().font, nullptr);
  EXPECT_TRUE(label.SetText("hello"));

  // With no further font changes, the cache behaves normally again.
  EXPECT_FALSE(label.SetText("hello"));
}
