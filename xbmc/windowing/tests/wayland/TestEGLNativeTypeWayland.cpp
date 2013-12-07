/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#include "system.h"

#define WL_EGL_PLATFORM

#include <string>
#include <vector>

#include <boost/bind.hpp>

#include <gtest/gtest.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>

#if defined(HAVE_WAYLAND_XBMC_PROTO)
#include "xbmc_wayland_test_client_protocol.h"
#endif

#include "windowing/egl/wayland/Display.h"
#include "windowing/egl/wayland/Registry.h"
#include "windowing/egl/wayland/Surface.h"
#include "windowing/egl/EGLNativeTypeWayland.h"

#include "TmpEnv.h"
#include "WestonTest.h"
#include "XBMCWayland.h"

using ::testing::Values;
using ::testing::WithParamInterface;

namespace xt = xbmc::test;
namespace xw = xbmc::wayland;
namespace xtw = xbmc::test::wayland;

class EGLNativeTypeWaylandWestonTest :
  public WestonTest
{
protected:

  CEGLNativeTypeWayland m_nativeType;
};

TEST_F(EGLNativeTypeWaylandWestonTest, TestCheckCompatibilityWithEnvSet)
{
  TmpEnv env("WAYLAND_DISPLAY", TempSocketName().c_str());
  EXPECT_TRUE(m_nativeType.CheckCompatibility());
}

TEST_F(EGLNativeTypeWaylandWestonTest, TestCheckCompatibilityWithEnvNotSet)
{
  EXPECT_FALSE(m_nativeType.CheckCompatibility());
}

class CompatibleEGLNativeTypeWaylandWestonTest :
  public EGLNativeTypeWaylandWestonTest
{
public:

  CompatibleEGLNativeTypeWaylandWestonTest();
  virtual void SetUp();
  
private:

  TmpEnv m_waylandDisplayEnv;
};

CompatibleEGLNativeTypeWaylandWestonTest::CompatibleEGLNativeTypeWaylandWestonTest() :
  m_waylandDisplayEnv("WAYLAND_DISPLAY",
                      TempSocketName().c_str())
{
}

void
CompatibleEGLNativeTypeWaylandWestonTest::SetUp()
{
  WestonTest::SetUp();
  ASSERT_TRUE(m_nativeType.CheckCompatibility());
}

TEST_F(CompatibleEGLNativeTypeWaylandWestonTest, TestConnection)
{
  EXPECT_TRUE(m_nativeType.CreateNativeDisplay());
}

class ConnectedEGLNativeTypeWaylandWestonTest :
  public CompatibleEGLNativeTypeWaylandWestonTest
{
public:

  ConnectedEGLNativeTypeWaylandWestonTest();
  ~ConnectedEGLNativeTypeWaylandWestonTest();
  virtual void SetUp();

protected:

  xw::Display *m_display;
  struct wl_surface *m_mostRecentSurface;

private:

  void Global(struct wl_registry *, uint32_t, const char *, uint32_t);
  void DisplayAvailable(xw::Display &display);
  void SurfaceCreated(xw::Surface &surface);
};

ConnectedEGLNativeTypeWaylandWestonTest::ConnectedEGLNativeTypeWaylandWestonTest()
{
  xw::WaylandDisplayListener &displayListener(xw::WaylandDisplayListener::GetInstance());
  displayListener.SetHandler(boost::bind(&ConnectedEGLNativeTypeWaylandWestonTest::DisplayAvailable,
                                         this, _1));

  xw::WaylandSurfaceListener &surfaceListener(xw::WaylandSurfaceListener::GetInstance());
  surfaceListener.SetHandler(boost::bind(&ConnectedEGLNativeTypeWaylandWestonTest::SurfaceCreated,
                                         this, _1));
}

ConnectedEGLNativeTypeWaylandWestonTest::~ConnectedEGLNativeTypeWaylandWestonTest()
{
  xw::WaylandDisplayListener &displayListener(xw::WaylandDisplayListener::GetInstance());
  displayListener.SetHandler(xw::WaylandDisplayListener::Handler());
  
  xw::WaylandSurfaceListener &surfaceListener(xw::WaylandSurfaceListener::GetInstance());
  surfaceListener.SetHandler(xw::WaylandSurfaceListener::Handler());
}

void
ConnectedEGLNativeTypeWaylandWestonTest::SetUp()
{
  CompatibleEGLNativeTypeWaylandWestonTest::SetUp();
  ASSERT_TRUE(m_nativeType.CreateNativeDisplay());
}

void
ConnectedEGLNativeTypeWaylandWestonTest::DisplayAvailable(xw::Display &display)
{
  m_display = &display;
}

void
ConnectedEGLNativeTypeWaylandWestonTest::SurfaceCreated(xw::Surface &surface)
{
  m_mostRecentSurface = surface.GetWlSurface();
}

TEST_F(ConnectedEGLNativeTypeWaylandWestonTest, CreateNativeWindowSuccess)
{
  EXPECT_TRUE(m_nativeType.CreateNativeWindow());
}

TEST_F(ConnectedEGLNativeTypeWaylandWestonTest, ProbeResolutionsSuccess)
{
  std::vector<RESOLUTION_INFO> info;
  EXPECT_TRUE(m_nativeType.ProbeResolutions(info));
}

TEST_F(ConnectedEGLNativeTypeWaylandWestonTest, PreferredResolutionSuccess)
{
  RESOLUTION_INFO info;
  EXPECT_TRUE(m_nativeType.GetPreferredResolution(&info));
}

TEST_F(ConnectedEGLNativeTypeWaylandWestonTest, CurrentNativeSuccess)
{
  RESOLUTION_INFO info;
  EXPECT_TRUE(m_nativeType.GetNativeResolution(&info));
}

TEST_F(ConnectedEGLNativeTypeWaylandWestonTest, GetMostRecentSurface)
{
  m_nativeType.CreateNativeWindow();
  EXPECT_TRUE(m_mostRecentSurface != NULL);
}

#if defined(HAVE_WAYLAND_XBMC_PROTO)

class AssistedEGLNativeTypeWaylandTest :
  public ConnectedEGLNativeTypeWaylandWestonTest
{
protected:

  AssistedEGLNativeTypeWaylandTest();
  ~AssistedEGLNativeTypeWaylandTest();

  virtual void SetUp();
  
  void Global(struct wl_registry *registry,
              uint32_t name,
              const char *interface,
              uint32_t version);
  
  boost::scoped_ptr<xtw::XBMCWayland> m_xbmcWayland;
};

AssistedEGLNativeTypeWaylandTest::AssistedEGLNativeTypeWaylandTest()
{
  xw::ExtraWaylandGlobals &extra(xw::ExtraWaylandGlobals::GetInstance());
  extra.SetHandler(boost::bind(&AssistedEGLNativeTypeWaylandTest::Global,
                               this, _1, _2, _3, _4));
}

AssistedEGLNativeTypeWaylandTest::~AssistedEGLNativeTypeWaylandTest()
{
  xw::ExtraWaylandGlobals &extra(xw::ExtraWaylandGlobals::GetInstance());
  extra.SetHandler(xw::ExtraWaylandGlobals::GlobalHandler());
}

void
AssistedEGLNativeTypeWaylandTest::SetUp()
{
  ConnectedEGLNativeTypeWaylandWestonTest::SetUp();
  ASSERT_TRUE(m_xbmcWayland.get());
}

void
AssistedEGLNativeTypeWaylandTest::Global(struct wl_registry *registry,
                                         uint32_t name,
                                         const char *interface,
                                         uint32_t version)
{
  if (std::string(interface) == "xbmc_wayland")
    m_xbmcWayland.reset(new xtw::XBMCWayland(static_cast<xbmc_wayland *>(wl_registry_bind(registry,
                                                                                          name,
                                                                                          &xbmc_wayland_interface,
                                                                                          version))));
}

TEST_F(AssistedEGLNativeTypeWaylandTest, TestGotXBMCWayland)
{
  EXPECT_TRUE(m_xbmcWayland.get() != NULL);
}

TEST_F(AssistedEGLNativeTypeWaylandTest, AdditionalResolutions)
{
  m_xbmcWayland->AddMode(2, 2, 2, static_cast<enum wl_output_mode>(0));
  std::vector<RESOLUTION_INFO> resolutions;
  m_nativeType.ProbeResolutions(resolutions);
  EXPECT_TRUE(resolutions.size() == 2);
}

TEST_F(AssistedEGLNativeTypeWaylandTest, PreferredResolutionChange)
{
  m_xbmcWayland->AddMode(2, 2, 2, static_cast<enum wl_output_mode>(WL_OUTPUT_MODE_PREFERRED));
  RESOLUTION_INFO res;
  m_nativeType.GetPreferredResolution(&res);
  EXPECT_EQ(res.iWidth, 2);
  EXPECT_EQ(res.iHeight, 2);
}

TEST_F(AssistedEGLNativeTypeWaylandTest, CurrentResolutionChange)
{
  m_xbmcWayland->AddMode(2, 2, 2, static_cast<enum wl_output_mode>(WL_OUTPUT_MODE_CURRENT));
  RESOLUTION_INFO res;
  m_nativeType.GetNativeResolution(&res);
  EXPECT_EQ(res.iWidth, 2);
  EXPECT_EQ(res.iHeight, 2);
}

#endif
