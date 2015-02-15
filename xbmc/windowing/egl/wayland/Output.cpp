/*
 *      Copyright (C) 2011-2013 Team XBMC
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
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <wayland-client.h>

#include "windowing/DllWaylandClient.h"
#include "windowing/WaylandProtocol.h"
#include "Output.h"

namespace xw = xbmc::wayland;

/* We only support version 1 of this interface, the other
 * struct members are impliedly set to NULL */
const wl_output_listener xw::Output::m_listener = 
{
  Output::GeometryCallback,
  Output::ModeCallback
};

xw::Output::Output(IDllWaylandClient &clientLibrary,
                   struct wl_output *output) :
  m_clientLibrary(clientLibrary),
  m_output(output),
  m_scaleFactor(1.0),
  m_currentValid(false),
  m_preferredValid(false)
{
  protocol::AddListenerOnWaylandObject(m_clientLibrary,
                                       m_output,
                                       &m_listener,
                                       reinterpret_cast<void *>(this));
}

xw::Output::~Output()
{
  protocol::DestroyWaylandObject(m_clientLibrary,
                                 m_output);
}

struct wl_output *
xw::Output::GetWlOutput()
{
  return m_output;
}

/* It is a precondition violation to use CurrentMode() and
 * PreferredMode() before output modes have arrived yet, use
 * a synchronization function to ensure that this is the case */
const xw::Output::ModeGeometry &
xw::Output::CurrentMode()
{
  if (!m_currentValid)
    throw std::logic_error("No current mode has been set by the server"
                           " yet");
  
  return m_current;
}

const xw::Output::ModeGeometry &
xw::Output::PreferredMode()
{
  if (!m_preferredValid)
    throw std::logic_error("No preferred mode has been set by the "
                           " server yet");

  return m_preferred;
}

const std::vector <xw::Output::ModeGeometry> &
xw::Output::AllModes()
{
  return m_modes;
}

const xw::Output::PhysicalGeometry &
xw::Output::Geometry()
{
  return m_geometry;
}

uint32_t
xw::Output::ScaleFactor()
{
  return m_scaleFactor;
}

void
xw::Output::GeometryCallback(void *data,
                             struct wl_output *output,
                             int32_t x,
                             int32_t y,
                             int32_t physicalWidth,
                             int32_t physicalHeight,
                             int32_t subpixelArrangement,
                             const char *make,
                             const char *model,
                             int32_t transform)
{
  return static_cast<xw::Output *>(data)->Geometry(x,
                                                   y,
                                                   physicalWidth,
                                                   physicalHeight,
                                                   subpixelArrangement,
                                                   make,
                                                   model,
                                                   transform);
}

void
xw::Output::ModeCallback(void *data,
                         struct wl_output *output,
                         uint32_t flags,
                         int32_t width,
                         int32_t height,
                         int32_t refresh)
{
  return static_cast<xw::Output *>(data)->Mode(flags,
                                               width,
                                               height,
                                               refresh);
}

void
xw::Output::DoneCallback(void *data,
                         struct wl_output *output)
{
  return static_cast<xw::Output *>(data)->Done();
}

void
xw::Output::ScaleCallback(void *data,
                          struct wl_output *output,
                          int32_t factor)
{
  return static_cast<xw::Output *>(data)->Scale(factor);
}

/* This function is called when the output geometry is determined.
 * 
 * The output geometry represents the actual geometry of the monitor.
 * As it is per-output, there is only one geometry.
 */
void
xw::Output::Geometry(int32_t x,
                     int32_t y,
                     int32_t physicalWidth,
                     int32_t physicalHeight,
                     int32_t subpixelArrangement,
                     const char *make,
                     const char *model,
                     int32_t transform)
{
  m_geometry.x = x;
  m_geometry.y = y;
  m_geometry.physicalWidth = physicalWidth;
  m_geometry.physicalHeight = physicalHeight;
  m_geometry.subpixelArrangement =
    static_cast<enum wl_output_subpixel>(subpixelArrangement);
  m_geometry.outputTransformation =
    static_cast<enum wl_output_transform>(transform);
}

/* This function is called when a new mode is available on this output
 * or a mode's state changes.
 * 
 * It is possible that the same mode can change its state, so we will
 * not add it twice. Instead, we will determine if the mode is the
 * same one, but its flags have been updated and if so, update
 * the pointers to modes having those flags.
 */
void
xw::Output::Mode(uint32_t flags,
                 int32_t width,
                 int32_t height,
                 int32_t refresh)
{
  xw::Output::ModeGeometry *update = NULL;
  
  for (std::vector<ModeGeometry>::iterator it = m_modes.begin();
       it != m_modes.end();
       ++it)
  { 
    if (it->width == width &&
        it->height == height &&
        it->refresh == refresh)
    {
      update = &(*it);
      break;
    }
  }
  
  enum wl_output_mode outputFlags =
    static_cast<enum wl_output_mode>(flags);
  
  if (!update)
  {
    /* New output created */
    m_modes.push_back(ModeGeometry());
    ModeGeometry &next(m_modes.back());
    
    next.width = width;
    next.height = height;
    next.refresh = refresh;
    
    update = &next;
  }
  
  /* We may get a mode notification for a new or
   * or existing mode. In both cases we need to
   * update the current and preferred modes */
  if (outputFlags & WL_OUTPUT_MODE_CURRENT)
  {
    m_current = *update;
    m_currentValid = true;
  }
  if (outputFlags & WL_OUTPUT_MODE_PREFERRED)
  {
    m_preferred = *update;
    m_preferredValid = true;
  }
}

void
xw::Output::Done()
{
}

/* This function is called whenever the scaling factor for this
 * output changes. It there for clients to support HiDPI displays,
 * although unused as of present */
void
xw::Output::Scale(int32_t factor)
{
  m_scaleFactor = factor;
}
