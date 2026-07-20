/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ServiceBroker.h"
#include "guilib/GUIControlGroupMask.h"
#include "guilib/GUIListItem.h"
#include "guilib/GUITexture.h"
#include "rendering/RenderSystem.h"
#include "windowing/WinSystem.h"

#include <vector>

#include <gtest/gtest.h>

namespace
{
class CTestRenderSystem : public CRenderSystemBase
{
public:
  bool InitRenderSystem() override { return true; }
  bool DestroyRenderSystem() override { return true; }
  bool ResetRenderSystem(int, int) override { return true; }
  bool BeginRender() override { return true; }
  bool EndRender() override { return true; }
  void PresentRender(bool, bool) override {}
  bool ClearBuffers(KODI::UTILS::COLOR::Color) override { return true; }
  bool IsExtSupported(const char*) const override { return false; }
  void SetViewPort(const CRect&) override {}
  void GetViewPort(CRect&) override {}
  void SetScissors(const CRect&) override {}
  void ResetScissors() override {}
  void CaptureStateBlock() override {}
  void ApplyStateBlock() override {}
  void SetCameraPosition(const CPoint&, int, int, float) override {}
};

class CTestWinSystem : public CWinSystemBase
{
public:
  CRenderSystemBase* GetRenderSystem() override { return &m_renderSystem; }
  bool CreateNewWindow(const std::string&, bool, RESOLUTION_INFO&) override { return false; }
  bool ResizeWindow(int, int, int, int) override { return false; }
  bool SetFullScreen(bool, RESOLUTION_INFO&, bool) override { return false; }
  void Register(IDispResource*) override {}
  void Unregister(IDispResource*) override {}

private:
  CTestRenderSystem m_renderSystem;
};

class TestGUIControlGroupMask : public testing::Test
{
protected:
  void SetUp() override { CServiceBroker::RegisterWinSystem(&m_winSystem); }
  void TearDown() override { CServiceBroker::UnregisterWinSystem(); }

private:
  CTestWinSystem m_winSystem;
};

class CTestControl : public CGUIControl
{
public:
  CTestControl() : CGUIControl(0, 0, 0.0f, 0.0f, 1.0f, 1.0f) {}
  CTestControl* Clone() const override { return new CTestControl(*this); }

  void UpdateInfo(const CGUIListItem* item) override { infoItem = item; }
  void UpdateVisibility(const CGUIListItem* item) override
  {
    visibilityItem = item;
    CGUIControl::UpdateVisibility(item);
  }
  void Render() override
  {
    stencilLayer = CServiceBroker::GetWinSystem()->GetGfxContext().GetStencilLayer();
    ++renderCount;
  }

  const CGUIListItem* infoItem{nullptr};
  const CGUIListItem* visibilityItem{nullptr};
  STENCIL_LAYER stencilLayer{STENCIL_LAYER::INVALID};
  unsigned int renderCount{0};
};

template<class Group>
class CRotatingGroup : public Group
{
public:
  using Group::Group;
  bool Animate(unsigned int) override
  {
    this->m_transform.SetZRotation(angle, 50.0f, 50.0f);
    return true;
  }
  float angle{0.0f};
};

class TestGUIControlGroupMaskRendering : public TestGUIControlGroupMask,
                                         public testing::WithParamInterface<bool>
{
protected:
  void SetUp() override
  {
    TestGUIControlGroupMask::SetUp();
    Context().SetScissors(CRect(0.0f, 0.0f, 200.0f, 200.0f));
    CGUITexture::Register(
        {}, {},
        [this](const CRect& rect, STENCIL_LAYER layer) {
          draws.push_back({rect, Context().GenerateAABB(rect), layer, Context().GetStencilLayer()});
        });
  }
  void TearDown() override
  {
    CGUITexture::Register({}, {}, {});
    TestGUIControlGroupMask::TearDown();
  }
  CGraphicContext& Context() { return CServiceBroker::GetWinSystem()->GetGfxContext(); }
  void Render(CGUIControl& group)
  {
    Context().SetRenderOrder(GetParam() ? RENDER_ORDER_FRONT_TO_BACK
                                        : RENDER_ORDER_ALL_BACK_TO_FRONT);
    group.DoRender();
    if (GetParam())
    {
      const size_t stencilDraws = draws.size();
      Context().SetRenderOrder(RENDER_ORDER_BACK_TO_FRONT);
      group.DoRender();
      EXPECT_EQ(stencilDraws, draws.size());
    }
    EXPECT_EQ(STENCIL_LAYER::NONE, Context().GetStencilLayer());
    EXPECT_EQ(CRect(0.0f, 0.0f, 200.0f, 200.0f), Context().GetScissors());
  }
  struct StencilDraw
  {
    CRect rect;
    CRect screenRect;
    STENCIL_LAYER layer;
    STENCIL_LAYER parent;
  };
  std::vector<StencilDraw> draws;
};
} // namespace

TEST_F(TestGUIControlGroupMask, UpdateInfoPropagatesItem)
{
  CGUIControlGroupMask group(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
  auto* child = new CTestControl;
  group.AddControl(child);
  CGUIListItem item;

  group.UpdateInfo(&item);

  EXPECT_EQ(&item, child->infoItem);
  EXPECT_EQ(&item, child->visibilityItem);
}

TEST_F(TestGUIControlGroupMask, ProcessPropagatesAndClearsItem)
{
  CGUIControlGroupMask group(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
  auto* child = new CTestControl;
  group.AddControl(child);
  CGUIListItem item;
  CDirtyRegionList dirtyRegions;

  group.UpdateVisibility(&item);
  group.Process(0, dirtyRegions);
  EXPECT_EQ(&item, child->visibilityItem);

  group.Process(0, dirtyRegions);
  EXPECT_EQ(nullptr, child->visibilityItem);
}

TEST_F(TestGUIControlGroupMask, NestedGroupMaskPropagatesItem)
{
  CGUIControlGroupMask group(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
  auto* nestedGroup = new CGUIControlGroupMask(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
  auto* child = new CTestControl;
  nestedGroup->AddControl(child);
  group.AddControl(nestedGroup);
  CGUIListItem item;
  CDirtyRegionList dirtyRegions;

  group.UpdateVisibility(&item);
  group.Process(0, dirtyRegions);

  EXPECT_EQ(&item, child->visibilityItem);
}

TEST_P(TestGUIControlGroupMaskRendering, RotationTransitionsReleaseLayer)
{
  CRotatingGroup<CGUIControlGroupMask> group(0, 0, 10.0f, 20.0f, 80.0f, 60.0f);
  auto* child = new CTestControl;
  group.AddControl(child);
  CDirtyRegionList dirtyRegions;

  for (const float angle : {0.0f, 0.5f, 0.0f, 0.5f})
  {
    Context().ClearStencilBuckets();
    draws.clear();
    group.angle = angle;
    group.DoProcess(0, dirtyRegions);
    if (angle == 0.0f)
      Context().AddStencilRectToBucket(group.GetRenderRegion());
    Render(group);

    if (angle == 0.0f)
    {
      EXPECT_TRUE(draws.empty());
      EXPECT_EQ(STENCIL_LAYER::NONE, child->stencilLayer);
    }
    else
    {
      ASSERT_EQ(1u, draws.size());
      EXPECT_EQ(CRect(10.0f, 20.0f, 90.0f, 80.0f), draws[0].rect);
      EXPECT_NE(group.GetRenderRegion(), draws[0].rect);
      EXPECT_EQ(group.GetRenderRegion(), draws[0].screenRect);
      EXPECT_EQ(draws[0].layer, child->stencilLayer);
    }
  }
}

TEST_P(TestGUIControlGroupMaskRendering, InheritedRotationAndNestedMasks)
{
  CRotatingGroup<CGUIControlGroup> parent(0, 0, 10.0f, 20.0f, 100.0f, 100.0f);
  parent.angle = 0.5f;
  auto* outer = new CGUIControlGroupMask(0, 0, 10.0f, 10.0f, 80.0f, 80.0f);
  auto* inner = new CGUIControlGroupMask(0, 0, 5.0f, 5.0f, 60.0f, 60.0f);
  auto* innerChild = new CTestControl;
  auto* outerChild = new CTestControl;
  inner->AddControl(innerChild);
  outer->AddControl(inner);
  outer->AddControl(outerChild);
  parent.AddControl(outer);
  CDirtyRegionList dirtyRegions;

  parent.DoProcess(0, dirtyRegions);
  Render(parent);

  ASSERT_EQ(2u, draws.size());
  EXPECT_EQ(outer->GetRenderRegion(), draws[0].screenRect);
  EXPECT_EQ(inner->GetRenderRegion(), draws[1].screenRect);
  EXPECT_EQ(STENCIL_LAYER::NONE, draws[0].parent);
  EXPECT_EQ(draws[0].layer, draws[1].parent);
  EXPECT_NE(draws[0].layer, draws[1].layer);
  EXPECT_EQ(draws[0].layer, outerChild->stencilLayer);
  EXPECT_EQ(draws[1].layer, innerChild->stencilLayer);

  Context().ClearStencilBuckets();
  draws.clear();
  parent.angle = 0.0f;
  parent.DoProcess(0, dirtyRegions);
  Render(parent);

  EXPECT_TRUE(draws.empty());
  EXPECT_EQ(STENCIL_LAYER::NONE, outerChild->stencilLayer);
  EXPECT_EQ(STENCIL_LAYER::NONE, innerChild->stencilLayer);
}

TEST_P(TestGUIControlGroupMaskRendering, UnrotatedMaskRecoversFromExhaustion)
{
  CRotatingGroup<CGUIControlGroupMask> group(0, 0, 10.0f, 20.0f, 80.0f, 60.0f);
  auto* child = new CTestControl;
  group.AddControl(child);
  CDirtyRegionList dirtyRegions;

  for (unsigned int i = 0; i < 8; ++i)
    Context().AddStencilRectToBucket(CRect(0.0f, 0.0f, 200.0f, 200.0f));
  group.angle = 0.5f;
  group.DoProcess(0, dirtyRegions);
  Render(group);
  EXPECT_EQ(0u, child->renderCount);

  Context().ClearStencilBuckets();
  group.angle = 0.0f;
  group.DoProcess(0, dirtyRegions);
  Render(group);
  EXPECT_GT(child->renderCount, 0u);
  EXPECT_EQ(STENCIL_LAYER::NONE, child->stencilLayer);
  EXPECT_TRUE(draws.empty());
}

INSTANTIATE_TEST_SUITE_P(RenderPasses, TestGUIControlGroupMaskRendering, testing::Bool());
