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
#include <tr1/tuple>

#include <gtest/gtest.h>

#include <wayland-client-protocol.h>

#include "windowing/wayland/Pointer.h"
#include "windowing/wayland/PointerProcessor.h"

#include "StubCursorManager.h"
#include "StubEventListener.h"

using ::testing::Values;
using ::testing::WithParamInterface;

namespace xw = xbmc::wayland;

class WaylandPointerProcessor :
  public ::testing::Test
{
public:

  WaylandPointerProcessor();

protected:

  StubCursorManager cursorManager;
  StubEventListener listener;
  xbmc::PointerProcessor processor;
};

WaylandPointerProcessor::WaylandPointerProcessor() :
  processor(listener, cursorManager)
{
}

class WaylandPointerProcessorButtons :
  public WaylandPointerProcessor,
  public WithParamInterface<std::tr1::tuple<uint32_t, uint32_t> >
{
protected:

  uint32_t WaylandButton();
  uint32_t XBMCButton();
};

uint32_t WaylandPointerProcessorButtons::WaylandButton()
{
  return std::tr1::get<0>(GetParam());
}

uint32_t WaylandPointerProcessorButtons::XBMCButton()
{
  return std::tr1::get<1>(GetParam());
}

TEST_P(WaylandPointerProcessorButtons, ButtonPress)
{
  xw::IPointerReceiver &receiver =
    static_cast<xw::IPointerReceiver &>(processor);
  receiver.Button(0, 0, WaylandButton(), WL_POINTER_BUTTON_STATE_PRESSED);
  
  XBMC_Event event(listener.FetchLastEvent());
  EXPECT_EQ(XBMC_MOUSEBUTTONDOWN, event.type);
  EXPECT_EQ(XBMCButton(), event.button.button);
}

TEST_P(WaylandPointerProcessorButtons, ButtonRelease)
{
  xw::IPointerReceiver &receiver =
    static_cast<xw::IPointerReceiver &>(processor);
  receiver.Button(0, 0, WaylandButton(), WL_POINTER_BUTTON_STATE_RELEASED);
  
  XBMC_Event event(listener.FetchLastEvent());
  EXPECT_EQ(XBMC_MOUSEBUTTONUP, event.type);
  EXPECT_EQ(XBMCButton(), event.button.button);
}

INSTANTIATE_TEST_CASE_P(ThreeButtonMouse,
                        WaylandPointerProcessorButtons,
                        Values(std::tr1::tuple<uint32_t, uint32_t>(272, 1),
                               std::tr1::tuple<uint32_t, uint32_t>(274, 2),
                               std::tr1::tuple<uint32_t, uint32_t>(273, 3)));

class WaylandPointerProcessorAxisButtons :
  public WaylandPointerProcessor,
  public WithParamInterface<std::tr1::tuple<float, uint32_t> >
{
protected:

  float Magnitude();
  uint32_t XBMCButton();
};

float WaylandPointerProcessorAxisButtons::Magnitude()
{
  return std::tr1::get<0>(GetParam());
}

uint32_t WaylandPointerProcessorAxisButtons::XBMCButton()
{
  return std::tr1::get<1>(GetParam());
}

TEST_P(WaylandPointerProcessorAxisButtons, Axis)
{
  xw::IPointerReceiver &receiver =
    static_cast<xw::IPointerReceiver &>(processor);
  receiver.Axis(0, WL_POINTER_AXIS_VERTICAL_SCROLL, Magnitude());
  
  XBMC_Event event(listener.FetchLastEvent());
  EXPECT_EQ(XBMC_MOUSEBUTTONDOWN, event.type);
  EXPECT_EQ(XBMCButton(), event.button.button);
  
  event = listener.FetchLastEvent();
  EXPECT_EQ(XBMC_MOUSEBUTTONUP, event.type);
  EXPECT_EQ(XBMCButton(), event.button.button);
}

INSTANTIATE_TEST_CASE_P(VerticalScrollWheel,
                        WaylandPointerProcessorAxisButtons,
                        Values(std::tr1::tuple<float, uint32_t>(-1.0, 4),
                               std::tr1::tuple<float, uint32_t>(1.0, 5)));

TEST_F(WaylandPointerProcessor, Motion)
{
  const float x = 5.0;
  const float y = 5.0;
  xw::IPointerReceiver &receiver =
    static_cast<xw::IPointerReceiver &>(processor);
  receiver.Motion(0, x, y);
  
  XBMC_Event event(listener.FetchLastEvent());
  EXPECT_EQ(XBMC_MOUSEMOTION, event.type);
  EXPECT_EQ(::round(x), event.motion.xrel);
  EXPECT_EQ(::round(y), event.motion.yrel);  
}

TEST_F(WaylandPointerProcessor, MotionThenButton)
{
  const float x = 5.0;
  const float y = 5.0;
  xw::IPointerReceiver &receiver =
    static_cast<xw::IPointerReceiver &>(processor);
  receiver.Motion(0, x, y);
  receiver.Button(0, 0, 272, WL_POINTER_BUTTON_STATE_PRESSED);
  
  listener.FetchLastEvent();
  XBMC_Event event(listener.FetchLastEvent());
  EXPECT_EQ(XBMC_MOUSEBUTTONDOWN, event.type);
  EXPECT_EQ(::round(x), event.button.y);
  EXPECT_EQ(::round(y), event.button.x);  
}
