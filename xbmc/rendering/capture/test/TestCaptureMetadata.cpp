/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "rendering/capture/CaptureMetadata.h"
#include "utils/DisplayInfo.h"
#include "windowing/WinSystem.h"

extern "C"
{
#include <libavutil/pixfmt.h>
}

#include <gtest/gtest.h>

using namespace KODI::RENDERING::CAPTURE;

namespace
{
class CTestWinSystem : public CWinSystemBase
{
public:
  explicit CTestWinSystem(KODI::UTILS::Eotf eotf) : m_eotf(eotf) {}

  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override
  {
    return true;
  }
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override { return true; }
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override
  {
    return true;
  }
  void Register(IDispResource* resource) override {}
  void Unregister(IDispResource* resource) override {}
  bool UseLimitedColor() override { return false; }
  KODI::UTILS::Eotf GetEotf() const override { return m_eotf; }

private:
  const KODI::UTILS::Eotf m_eotf;
};
} // namespace

TEST(TestCaptureMetadata, TagsSdrAsSrgb)
{
  CTestWinSystem winSystem(KODI::UTILS::Eotf::TRADITIONAL_SDR);
  const ImageColorMetadata color = GetOutputColorMetadata(winSystem);
  EXPECT_EQ(color.primaries, AVCOL_PRI_BT709);
  EXPECT_EQ(color.transfer, AVCOL_TRC_IEC61966_2_1);
  EXPECT_EQ(color.range, AVCOL_RANGE_JPEG);
}

TEST(TestCaptureMetadata, TagsPqAsPq)
{
  CTestWinSystem winSystem(KODI::UTILS::Eotf::PQ);
  EXPECT_EQ(GetOutputColorMetadata(winSystem).transfer, AVCOL_TRC_SMPTE2084);
}
