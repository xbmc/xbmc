/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/GUIControlGroup.h"

#include <memory>

#include <gtest/gtest.h>

namespace
{
class CTestGroup : public CGUIControlGroup
{
public:
  using CGUIControlLookup::IsValidControl;
};
} // namespace

TEST(TestGUIControlGroup, ClearAllRemovesDescendantsFromLiveAncestors)
{
  CGUIControlGroup ancestor;
  auto* group = new CGUIControlGroup(0, 1, 0, 0, 100, 100);
  auto* nested = new CGUIControlGroup(0, 2, 0, 0, 100, 100);
  group->AddControl(nested);
  ancestor.AddControl(group);
  auto* child = new CGUIControlGroup(0, 3, 0, 0, 100, 100);
  nested->AddControl(child);
  auto* sibling = new CGUIControlGroup(0, 3, 0, 0, 100, 100);
  ancestor.AddControl(sibling);
  ASSERT_EQ(ancestor.GetControl(3), child);

  group->ClearAll();

  EXPECT_EQ(ancestor.GetControl(1), group);
  EXPECT_EQ(ancestor.GetControl(2), nullptr);
  EXPECT_EQ(ancestor.GetControl(3), sibling);
  EXPECT_EQ(group->GetControl(2), nullptr);
  EXPECT_EQ(group->GetControl(3), nullptr);
  EXPECT_EQ(group->GetParentControl(), &ancestor);
}

TEST(TestGUIControlGroup, RemovedSubtreeCanOutliveItsAncestors)
{
  auto ancestor = std::make_unique<CGUIControlGroup>();
  auto* group = new CGUIControlGroup(0, 1, 0, 0, 100, 100);
  auto* removed = new CGUIControlGroup(0, 2, 0, 0, 100, 100);
  auto* child = new CGUIControlGroup(0, 3, 0, 0, 100, 100);
  removed->AddControl(child);
  group->AddControl(removed);
  ancestor->AddControl(group);
  ASSERT_EQ(ancestor->GetControl(3), child);

  ASSERT_TRUE(ancestor->RemoveControl(removed));
  std::unique_ptr<CGUIControlGroup> retained(removed);

  EXPECT_EQ(ancestor->GetControl(1), group);
  EXPECT_EQ(ancestor->GetControl(2), nullptr);
  EXPECT_EQ(ancestor->GetControl(3), nullptr);
  EXPECT_EQ(group->GetControl(2), nullptr);
  EXPECT_EQ(group->GetControl(3), nullptr);
  EXPECT_EQ(retained->GetControl(3), child);
  EXPECT_EQ(retained->GetParentControl(), nullptr);
  ancestor.reset();
  retained->ClearAll();
  EXPECT_EQ(retained->GetControl(3), nullptr);
}

TEST(TestGUIControlGroup, CloneRegistersOnlyWithItsNewParent)
{
  CTestGroup ancestor;
  auto* source = new CGUIControlGroup(0, 1, 0, 0, 100, 100);
  auto* nested = new CGUIControlGroup(0, 2, 0, 0, 100, 100);
  auto* child = new CGUIControlGroup(0, 3, 0, 0, 100, 100);
  nested->AddControl(child);
  source->AddControl(nested);
  ancestor.AddControl(source);

  std::unique_ptr<CGUIControlGroup> clone(source->Clone());
  CGUIControl* clonedNested = clone->GetControl(2);
  CGUIControl* clonedChild = clone->GetControl(3);
  ASSERT_NE(clonedNested, nullptr);
  ASSERT_NE(clonedChild, nullptr);
  EXPECT_NE(clonedNested, nested);
  EXPECT_NE(clonedChild, child);
  EXPECT_EQ(clone->GetParentControl(), nullptr);
  EXPECT_FALSE(ancestor.IsValidControl(clonedNested));
  EXPECT_FALSE(ancestor.IsValidControl(clonedChild));

  CGUIControlGroup destination;
  CGUIControlGroup* attached = clone.release();
  destination.AddControl(attached);
  EXPECT_EQ(destination.GetControl(2), clonedNested);
  EXPECT_EQ(destination.GetControl(3), clonedChild);
  attached->ClearAll();
  EXPECT_EQ(destination.GetControl(1), attached);
  EXPECT_EQ(destination.GetControl(2), nullptr);
  EXPECT_EQ(destination.GetControl(3), nullptr);
  EXPECT_EQ(ancestor.GetControl(2), nested);
  EXPECT_EQ(ancestor.GetControl(3), child);
}
