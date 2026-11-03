/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WaylandSurface.h"

#include "WaylandBuffer.h"
#include "WaylandHost.h"
#include "utils/log.h"

#include <presentation-time-server-protocol.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <xdg-shell-server-protocol.h>

using namespace KODI::WAYLANDHOST;

CWaylandSurface::CWaylandSurface(CWaylandHost& host, wl_resource* resource)
  : m_host(host),
    m_resource(resource)
{
  m_host.OnSurfaceCreated(this);
}

CWaylandSurface::~CWaylandSurface()
{
  // forgotten before unmapping: a destroyed surface is sent no leave events
  m_host.OnSurfaceDestroyed(this);
  Unmap();
  m_host.OnSurfaceReleased(*this);

  if (m_xdgToplevel)
    wl_resource_set_user_data(m_xdgToplevel, nullptr);
  if (m_xdgSurface)
    wl_resource_set_user_data(m_xdgSurface, nullptr);

  if (m_current)
  {
    m_current->SetCurrent(false);
    m_current->SendRelease();
  }

  for (wl_resource* callback : m_pendingCallbacks)
    wl_resource_destroy(callback);
  for (wl_resource* callback : m_frameCallbacks)
    wl_resource_destroy(callback);
  for (wl_resource* feedback : m_presentationFeedback)
  {
    wp_presentation_feedback_send_discarded(feedback);
    wl_resource_destroy(feedback);
  }
}

void CWaylandSurface::Attach(wl_resource* buffer)
{
  m_pendingAttach = true;
  m_pending.reset();
  m_pendingDetach = (buffer == nullptr);
  if (!buffer)
    return;

  if (wl_shm_buffer_get(buffer))
  {
    // only dmabuf content reaches the screen; give shm buffers straight back
    wl_buffer_send_release(buffer);
    m_pendingDetach = true;
    return;
  }

  auto* holder = static_cast<std::shared_ptr<CWaylandBuffer>*>(wl_resource_get_user_data(buffer));
  if (holder)
    m_pending = *holder;
}

void CWaylandSurface::Frame(wl_resource* callback)
{
  m_pendingCallbacks.push_back(callback);
}

void CWaylandSurface::Commit()
{
  bool newContent = false;

  if (m_pendingAttach)
  {
    std::shared_ptr<CWaylandBuffer> previous = std::move(m_current);
    m_current = std::move(m_pending);
    m_pendingAttach = false;
    m_pendingDetach = false;

    if (m_current)
    {
      m_current->SetCurrent(true);
      newContent = true;
    }
    if (previous && previous != m_current)
    {
      previous->SetCurrent(false);
      previous->SendRelease();
    }
  }

  m_frameCallbacks.splice(m_frameCallbacks.end(), m_pendingCallbacks);

  if (m_xdgToplevel && !m_configured)
    SendConfigure(m_host.GetConfig().width, m_host.GetConfig().height);

  if (!m_current)
  {
    Unmap();
    return;
  }

  if (!m_mapped && m_xdgToplevel && m_acked)
    Map();

  if (m_mapped && newContent)
  {
    m_current->Lock();
    m_host.OnSurfaceFrame(*this, m_current);
  }
}

void CWaylandSurface::OnResourceDestroyed()
{
  m_resource = nullptr;
}

void CWaylandSurface::SetXdgSurface(wl_resource* resource)
{
  m_xdgSurface = resource;
  if (!resource)
    SetXdgToplevel(nullptr);
}

void CWaylandSurface::SetXdgToplevel(wl_resource* resource)
{
  m_xdgToplevel = resource;
  if (resource)
    return;

  Unmap();
  m_configured = false;
  m_acked = false;
}

void CWaylandSurface::AckConfigure(uint32_t serial)
{
  if (serial == m_configureSerial)
    m_acked = true;
}

void CWaylandSurface::SendConfigure(int32_t width, int32_t height)
{
  if (!m_xdgSurface || !m_xdgToplevel)
    return;

  wl_array states;
  wl_array_init(&states);
  auto* state = static_cast<uint32_t*>(wl_array_add(&states, 2 * sizeof(uint32_t)));
  state[0] = XDG_TOPLEVEL_STATE_FULLSCREEN;
  state[1] = XDG_TOPLEVEL_STATE_ACTIVATED;
  xdg_toplevel_send_configure(m_xdgToplevel, width, height, &states);
  wl_array_release(&states);

  m_configureSerial = m_host.NextSerial();
  m_configured = true;
  m_acked = false;
  xdg_surface_send_configure(m_xdgSurface, m_configureSerial);
}

void CWaylandSurface::AddPresentationFeedback(wl_resource* feedback)
{
  m_presentationFeedback.push_back(feedback);
}

void CWaylandSurface::FrameDone(uint32_t timeMs, const timespec& presented, uint32_t refreshNs)
{
  for (wl_resource* callback : m_frameCallbacks)
  {
    wl_callback_send_done(callback, timeMs);
    wl_resource_destroy(callback);
  }
  m_frameCallbacks.clear();

  const auto seconds = static_cast<uint64_t>(presented.tv_sec);
  for (wl_resource* feedback : m_presentationFeedback)
  {
    wp_presentation_feedback_send_presented(
        feedback, static_cast<uint32_t>(seconds >> 32), static_cast<uint32_t>(seconds),
        static_cast<uint32_t>(presented.tv_nsec), refreshNs, 0, 0,
        WP_PRESENTATION_FEEDBACK_KIND_VSYNC | WP_PRESENTATION_FEEDBACK_KIND_HW_CLOCK);
    wl_resource_destroy(feedback);
  }
  m_presentationFeedback.clear();
}

void CWaylandSurface::Map()
{
  m_mapped = true;
  CLog::Log(LOGDEBUG, "CWaylandSurface::{}: surface mapped, title \"{}\"", __FUNCTION__, m_title);
  m_host.OnSurfaceMapped(*this);
}

void CWaylandSurface::Unmap()
{
  if (!m_mapped)
    return;

  m_mapped = false;
  m_host.OnSurfaceUnmapped(*this);
}
