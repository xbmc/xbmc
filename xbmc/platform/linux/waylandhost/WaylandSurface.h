/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <ctime>
#include <list>
#include <memory>
#include <string>

struct wl_resource;

namespace KODI
{
namespace WAYLANDHOST
{

class CWaylandBuffer;
class CWaylandHost;

/*!
 * \brief A client's wl_surface together with its xdg-shell toplevel role.
 *
 * Every method runs on the host thread. The surface is mapped once the
 * client has acknowledged the fullscreen configure and committed a buffer.
 */
class CWaylandSurface
{
public:
  CWaylandSurface(CWaylandHost& host, wl_resource* resource);
  ~CWaylandSurface();

  CWaylandSurface(const CWaylandSurface&) = delete;
  CWaylandSurface& operator=(const CWaylandSurface&) = delete;

  wl_resource* GetResource() const { return m_resource; }
  wl_resource* GetToplevel() const { return m_xdgToplevel; }
  bool IsMapped() const { return m_mapped; }
  const std::string& GetTitle() const { return m_title; }
  std::shared_ptr<CWaylandBuffer> GetCurrentBuffer() const { return m_current; }

  // wl_surface requests
  void Attach(wl_resource* buffer);
  void Frame(wl_resource* callback);
  void Commit();
  void OnResourceDestroyed();

  // xdg_surface and xdg_toplevel
  void SetXdgSurface(wl_resource* resource);
  void SetXdgToplevel(wl_resource* resource);
  void AckConfigure(uint32_t serial);
  void SetTitle(std::string title) { m_title = std::move(title); }
  void SendConfigure(int32_t width, int32_t height);

  // wp_presentation
  void AddPresentationFeedback(wl_resource* feedback);

  /*!
   * \brief The last committed content has been shown: complete the frame
   * callbacks and presentation feedback the client asked for
   */
  void FrameDone(uint32_t timeMs, const timespec& presented, uint32_t refreshNs);

private:
  void Map();
  void Unmap();

  CWaylandHost& m_host;
  wl_resource* m_resource;
  wl_resource* m_xdgSurface{nullptr};
  wl_resource* m_xdgToplevel{nullptr};
  std::string m_title;

  std::shared_ptr<CWaylandBuffer> m_pending;
  bool m_pendingAttach{false};
  bool m_pendingDetach{false};
  std::list<wl_resource*> m_pendingCallbacks;

  std::shared_ptr<CWaylandBuffer> m_current;
  std::list<wl_resource*> m_frameCallbacks;
  std::list<wl_resource*> m_presentationFeedback;

  uint32_t m_configureSerial{0};
  bool m_configured{false};
  bool m_acked{false};
  bool m_mapped{false};
};

} // namespace WAYLANDHOST
} // namespace KODI
