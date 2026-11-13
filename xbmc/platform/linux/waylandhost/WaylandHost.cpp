/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WaylandHost.h"

#include "WaylandBuffer.h"
#include "WaylandSurface.h"
#include "utils/log.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <mutex>
#include <signal.h>

#include <drm_fourcc.h>
#include <fcntl.h>
#include <kodi-web-v1-server-protocol.h>
#include <linux-dmabuf-v1-server-protocol.h>
#include <poll.h>
#include <presentation-time-server-protocol.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <text-input-unstable-v3-server-protocol.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wayland-server-protocol.h>
#include <xdg-shell-server-protocol.h>
#include <xkbcommon/xkbcommon.h>

extern char** environ;

using namespace KODI::WAYLANDHOST;

namespace
{

constexpr uint32_t COMPOSITOR_VERSION = 5;
constexpr uint32_t OUTPUT_VERSION = 2;
constexpr uint32_t SEAT_VERSION = 7;
constexpr uint32_t XDG_WM_BASE_VERSION = 1;
constexpr uint32_t LINUX_DMABUF_VERSION = 4;
constexpr uint32_t PRESENTATION_VERSION = 1;
constexpr uint32_t TEXT_INPUT_VERSION = 1;
constexpr uint32_t WEB_MANAGER_VERSION = 1;
// kodi_web_manager_v1: a profile identifier is 1 to 255 bytes
constexpr size_t WEB_PROFILE_MAX = 255;
// a client blocked in teardown, e.g. on a stale network mount, is killed after this
constexpr std::chrono::seconds CLIENT_EXIT_TIMEOUT{90};

// evdev key codes map to xkb key codes with this offset (xkbcommon convention)
constexpr uint32_t XKB_KEYCODE_OFFSET = 8;

// one wheel click is 120 in axis_value120 units and 15 on the wl_pointer axis
// (the libinput convention every compositor follows)
constexpr int32_t AXIS_UNITS_PER_VALUE120 = 8;

constexpr uint32_t DMABUF_FORMATS[] = {DRM_FORMAT_ARGB8888, DRM_FORMAT_XRGB8888,
                                       DRM_FORMAT_ABGR8888, DRM_FORMAT_XBGR8888};

struct FormatTableEntry
{
  uint32_t format;
  uint32_t padding;
  uint64_t modifier;
};

timespec Now()
{
  timespec now{};
  clock_gettime(CLOCK_MONOTONIC, &now);
  return now;
}

uint32_t Milliseconds(const timespec& now)
{
  return static_cast<uint32_t>(now.tv_sec * 1000 + now.tv_nsec / 1000000);
}

CWaylandHost& HostOf(wl_resource* resource)
{
  return *static_cast<CWaylandHost*>(wl_resource_get_user_data(resource));
}

CWaylandSurface* SurfaceOf(wl_resource* resource)
{
  return static_cast<CWaylandSurface*>(wl_resource_get_user_data(resource));
}

void DestroyResource(wl_client*, wl_resource* resource)
{
  wl_resource_destroy(resource);
}

void Ignore(wl_client*, wl_resource*)
{
}

void Ignore(wl_client*, wl_resource*, uint32_t)
{
}

void Ignore(wl_client*, wl_resource*, int32_t)
{
}

void Ignore(wl_client*, wl_resource*, int32_t, int32_t)
{
}

void Ignore(wl_client*, wl_resource*, wl_resource*)
{
}

void Ignore(wl_client*, wl_resource*, const char*)
{
}

void Ignore(wl_client*, wl_resource*, int32_t, int32_t, int32_t, int32_t)
{
}

void Ignore(wl_client*, wl_resource*, wl_resource*, uint32_t)
{
}

void Ignore(wl_client*, wl_resource*, wl_resource*, uint32_t, uint32_t)
{
}

void Ignore(wl_client*, wl_resource*, wl_resource*, uint32_t, int32_t, int32_t)
{
}

void Ignore(wl_client*, wl_resource*, uint32_t, wl_resource*, int32_t, int32_t)
{
}

// wl_surface

void SurfaceAttach(wl_client*, wl_resource* resource, wl_resource* buffer, int32_t, int32_t)
{
  if (auto* surface = SurfaceOf(resource))
    surface->Attach(buffer);
}

void SurfaceFrame(wl_client* client, wl_resource* resource, uint32_t id)
{
  wl_resource* callback = wl_resource_create(client, &wl_callback_interface, 1, id);
  if (!callback)
  {
    wl_client_post_no_memory(client);
    return;
  }

  if (auto* surface = SurfaceOf(resource))
    surface->Frame(callback);
  else
    wl_resource_destroy(callback);
}

void SurfaceCommit(wl_client*, wl_resource* resource)
{
  if (auto* surface = SurfaceOf(resource))
    surface->Commit();
}

// filled by name: newer protocol headers add requests above the version advertised
constexpr struct wl_surface_interface SURFACE_IMPL = []
{
  struct wl_surface_interface impl = {};
  impl.destroy = DestroyResource;
  impl.attach = SurfaceAttach;
  impl.damage = Ignore;
  impl.frame = SurfaceFrame;
  impl.set_opaque_region = Ignore;
  impl.set_input_region = Ignore;
  impl.commit = SurfaceCommit;
  impl.set_buffer_transform = Ignore;
  impl.set_buffer_scale = Ignore;
  impl.damage_buffer = Ignore;
  impl.offset = Ignore;
  return impl;
}();

void SurfaceResourceDestroyed(wl_resource* resource)
{
  auto* surface = SurfaceOf(resource);
  if (!surface)
    return;

  surface->OnResourceDestroyed();
  delete surface;
}

const struct wl_region_interface REGION_IMPL = {
    DestroyResource,
    Ignore,
    Ignore,
};

// wl_compositor

void CompositorCreateSurface(wl_client* client, wl_resource* resource, uint32_t id)
{
  wl_resource* surfaceResource =
      wl_resource_create(client, &wl_surface_interface, wl_resource_get_version(resource), id);
  if (!surfaceResource)
  {
    wl_client_post_no_memory(client);
    return;
  }

  auto* surface = new CWaylandSurface(HostOf(resource), surfaceResource);
  wl_resource_set_implementation(surfaceResource, &SURFACE_IMPL, surface, SurfaceResourceDestroyed);
}

void CompositorCreateRegion(wl_client* client, wl_resource* resource, uint32_t id)
{
  wl_resource* region =
      wl_resource_create(client, &wl_region_interface, wl_resource_get_version(resource), id);
  if (!region)
  {
    wl_client_post_no_memory(client);
    return;
  }

  wl_resource_set_implementation(region, &REGION_IMPL, nullptr, nullptr);
}

constexpr struct wl_compositor_interface COMPOSITOR_IMPL = []
{
  struct wl_compositor_interface impl = {};
  impl.create_surface = CompositorCreateSurface;
  impl.create_region = CompositorCreateRegion;
  return impl;
}();

// xdg_wm_base

const struct xdg_positioner_interface POSITIONER_IMPL = {
    DestroyResource, Ignore, Ignore, Ignore, Ignore, Ignore, Ignore, Ignore, Ignore, Ignore,
};

void ToplevelSetTitle(wl_client*, wl_resource* resource, const char* title)
{
  if (auto* surface = SurfaceOf(resource))
    surface->SetTitle(title ? title : "");
}

const struct xdg_toplevel_interface TOPLEVEL_IMPL = {
    DestroyResource, Ignore, ToplevelSetTitle, Ignore, Ignore, Ignore, Ignore,
    Ignore,          Ignore, Ignore,           Ignore, Ignore, Ignore, Ignore,
};

void ToplevelResourceDestroyed(wl_resource* resource)
{
  if (auto* surface = SurfaceOf(resource))
    surface->SetXdgToplevel(nullptr);
}

void XdgSurfaceGetToplevel(wl_client* client, wl_resource* resource, uint32_t id)
{
  auto* surface = SurfaceOf(resource);
  wl_resource* toplevel =
      wl_resource_create(client, &xdg_toplevel_interface, wl_resource_get_version(resource), id);
  if (!toplevel)
  {
    wl_client_post_no_memory(client);
    return;
  }

  wl_resource_set_implementation(toplevel, &TOPLEVEL_IMPL, surface, ToplevelResourceDestroyed);
  if (surface)
    surface->SetXdgToplevel(toplevel);
}

void XdgSurfaceGetPopup(wl_client*, wl_resource* resource, uint32_t, wl_resource*, wl_resource*)
{
  wl_resource_post_error(resource, XDG_WM_BASE_ERROR_ROLE, "popups are not supported");
}

void XdgSurfaceAckConfigure(wl_client*, wl_resource* resource, uint32_t serial)
{
  if (auto* surface = SurfaceOf(resource))
    surface->AckConfigure(serial);
}

const struct xdg_surface_interface XDG_SURFACE_IMPL = {
    DestroyResource, XdgSurfaceGetToplevel, XdgSurfaceGetPopup, Ignore, XdgSurfaceAckConfigure,
};

void XdgSurfaceResourceDestroyed(wl_resource* resource)
{
  if (auto* surface = SurfaceOf(resource))
    surface->SetXdgSurface(nullptr);
}

void WmBaseCreatePositioner(wl_client* client, wl_resource* resource, uint32_t id)
{
  wl_resource* positioner =
      wl_resource_create(client, &xdg_positioner_interface, wl_resource_get_version(resource), id);
  if (!positioner)
  {
    wl_client_post_no_memory(client);
    return;
  }

  wl_resource_set_implementation(positioner, &POSITIONER_IMPL, nullptr, nullptr);
}

void WmBaseGetXdgSurface(wl_client* client,
                         wl_resource* resource,
                         uint32_t id,
                         wl_resource* surfaceResource)
{
  auto* surface = SurfaceOf(surfaceResource);
  wl_resource* xdgSurface =
      wl_resource_create(client, &xdg_surface_interface, wl_resource_get_version(resource), id);
  if (!xdgSurface)
  {
    wl_client_post_no_memory(client);
    return;
  }

  wl_resource_set_implementation(xdgSurface, &XDG_SURFACE_IMPL, surface,
                                 XdgSurfaceResourceDestroyed);
  if (surface)
    surface->SetXdgSurface(xdgSurface);
}

const struct xdg_wm_base_interface WM_BASE_IMPL = {
    DestroyResource,
    WmBaseCreatePositioner,
    WmBaseGetXdgSurface,
    Ignore,
};

// zwp_linux_dmabuf_v1

struct BufferParams
{
  CWaylandHost* host{nullptr};
  std::array<DmaBufPlane, CWaylandBuffer::MAX_PLANES> planes;
  uint64_t modifier{DRM_FORMAT_MOD_INVALID};
  unsigned int planeCount{0};
  bool used{false};
};

void BufferResourceDestroyed(wl_resource* resource)
{
  auto* holder = static_cast<std::shared_ptr<CWaylandBuffer>*>(wl_resource_get_user_data(resource));
  if (!holder)
    return;

  (*holder)->SetResource(nullptr);
  delete holder;
}

const struct wl_buffer_interface BUFFER_IMPL = {
    DestroyResource,
};

void ParamsAdd(wl_client*,
               wl_resource* resource,
               int32_t fd,
               uint32_t planeIdx,
               uint32_t offset,
               uint32_t stride,
               uint32_t modifierHi,
               uint32_t modifierLo)
{
  auto* params = static_cast<BufferParams*>(wl_resource_get_user_data(resource));
  if (params->used)
  {
    close(fd);
    wl_resource_post_error(resource, ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_ALREADY_USED,
                           "params were already used to create a wl_buffer");
    return;
  }
  if (planeIdx >= CWaylandBuffer::MAX_PLANES)
  {
    close(fd);
    wl_resource_post_error(resource, ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_PLANE_IDX,
                           "plane index %u is too large", planeIdx);
    return;
  }
  if (params->planes[planeIdx].fd >= 0)
  {
    close(fd);
    wl_resource_post_error(resource, ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_PLANE_SET,
                           "plane %u was already set", planeIdx);
    return;
  }

  params->planes[planeIdx] = {fd, offset, stride};
  params->modifier = (static_cast<uint64_t>(modifierHi) << 32) | modifierLo;
  if (planeIdx >= params->planeCount)
    params->planeCount = planeIdx + 1;
}

bool FormatSupported(uint32_t format)
{
  return std::find(std::begin(DMABUF_FORMATS), std::end(DMABUF_FORMATS), format) !=
         std::end(DMABUF_FORMATS);
}

wl_resource* ParamsCreateBuffer(wl_client* client,
                                wl_resource* resource,
                                uint32_t bufferId,
                                int32_t width,
                                int32_t height,
                                uint32_t format,
                                uint32_t flags)
{
  auto* params = static_cast<BufferParams*>(wl_resource_get_user_data(resource));
  if (params->used)
  {
    wl_resource_post_error(resource, ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_ALREADY_USED,
                           "params were already used to create a wl_buffer");
    return nullptr;
  }
  params->used = true;

  for (unsigned int i = 0; i < params->planeCount; ++i)
  {
    if (params->planes[i].fd < 0)
    {
      wl_resource_post_error(resource, ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INCOMPLETE,
                             "plane %u is missing", i);
      return nullptr;
    }
  }
  if (params->planeCount == 0)
  {
    wl_resource_post_error(resource, ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INCOMPLETE,
                           "no planes were added");
    return nullptr;
  }
  if (width <= 0 || height <= 0)
  {
    wl_resource_post_error(resource, ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INVALID_DIMENSIONS,
                           "invalid buffer size %dx%d", width, height);
    return nullptr;
  }
  if (!FormatSupported(format) || flags != 0)
  {
    zwp_linux_buffer_params_v1_send_failed(resource);
    return nullptr;
  }

  wl_resource* buffer = wl_resource_create(client, &wl_buffer_interface, 1, bufferId);
  if (!buffer)
  {
    wl_client_post_no_memory(client);
    return nullptr;
  }

  auto* holder = new std::shared_ptr<CWaylandBuffer>(
      std::make_shared<CWaylandBuffer>(params->host->GetTaskQueue(), width, height, format,
                                       params->modifier, params->planes, params->planeCount));
  for (auto& plane : params->planes)
    plane.fd = -1;
  (*holder)->SetResource(buffer);
  wl_resource_set_implementation(buffer, &BUFFER_IMPL, holder, BufferResourceDestroyed);
  return buffer;
}

void ParamsCreate(wl_client* client,
                  wl_resource* resource,
                  int32_t width,
                  int32_t height,
                  uint32_t format,
                  uint32_t flags)
{
  wl_resource* buffer = ParamsCreateBuffer(client, resource, 0, width, height, format, flags);
  if (buffer)
    zwp_linux_buffer_params_v1_send_created(resource, buffer);
}

void ParamsCreateImmed(wl_client* client,
                       wl_resource* resource,
                       uint32_t bufferId,
                       int32_t width,
                       int32_t height,
                       uint32_t format,
                       uint32_t flags)
{
  ParamsCreateBuffer(client, resource, bufferId, width, height, format, flags);
}

constexpr struct zwp_linux_buffer_params_v1_interface PARAMS_IMPL = []
{
  struct zwp_linux_buffer_params_v1_interface impl = {};
  impl.destroy = DestroyResource;
  impl.add = ParamsAdd;
  impl.create = ParamsCreate;
  impl.create_immed = ParamsCreateImmed;
  return impl;
}();

void ParamsResourceDestroyed(wl_resource* resource)
{
  auto* params = static_cast<BufferParams*>(wl_resource_get_user_data(resource));
  for (auto& plane : params->planes)
  {
    if (plane.fd >= 0)
      close(plane.fd);
  }
  delete params;
}

void DmabufCreateParams(wl_client* client, wl_resource* resource, uint32_t id)
{
  wl_resource* params = wl_resource_create(client, &zwp_linux_buffer_params_v1_interface,
                                           wl_resource_get_version(resource), id);
  if (!params)
  {
    wl_client_post_no_memory(client);
    return;
  }

  auto* state = new BufferParams;
  state->host = &HostOf(resource);
  wl_resource_set_implementation(params, &PARAMS_IMPL, state, ParamsResourceDestroyed);
}

const struct zwp_linux_dmabuf_feedback_v1_interface FEEDBACK_IMPL = {
    DestroyResource,
};

void DmabufGetFeedback(wl_client* client, wl_resource* resource, uint32_t id, wl_resource*)
{
  wl_resource* feedback = wl_resource_create(client, &zwp_linux_dmabuf_feedback_v1_interface,
                                             wl_resource_get_version(resource), id);
  if (!feedback)
  {
    wl_client_post_no_memory(client);
    return;
  }

  wl_resource_set_implementation(feedback, &FEEDBACK_IMPL, nullptr, nullptr);
  HostOf(resource).SendDmabufFeedback(feedback);
}

void DmabufGetDefaultFeedback(wl_client* client, wl_resource* resource, uint32_t id)
{
  DmabufGetFeedback(client, resource, id, nullptr);
}

const struct zwp_linux_dmabuf_v1_interface DMABUF_IMPL = {
    DestroyResource,
    DmabufCreateParams,
    DmabufGetDefaultFeedback,
    DmabufGetFeedback,
};

// wl_seat

const struct wl_pointer_interface POINTER_IMPL = {
    Ignore,
    DestroyResource,
};

const struct wl_keyboard_interface KEYBOARD_IMPL = {
    DestroyResource,
};

const struct wl_touch_interface TOUCH_IMPL = {
    DestroyResource,
};

void PointerResourceDestroyed(wl_resource* resource)
{
  HostOf(resource).RemovePointer(resource);
}

void KeyboardResourceDestroyed(wl_resource* resource)
{
  HostOf(resource).RemoveKeyboard(resource);
}

void TouchResourceDestroyed(wl_resource* resource)
{
  HostOf(resource).RemoveTouch(resource);
}

void SeatGetPointer(wl_client* client, wl_resource* resource, uint32_t id)
{
  HostOf(resource).AddPointer(client, wl_resource_get_version(resource), id);
}

void SeatGetKeyboard(wl_client* client, wl_resource* resource, uint32_t id)
{
  HostOf(resource).AddKeyboard(client, wl_resource_get_version(resource), id);
}

void SeatGetTouch(wl_client* client, wl_resource* resource, uint32_t id)
{
  HostOf(resource).AddTouch(client, wl_resource_get_version(resource), id);
}

const struct wl_seat_interface SEAT_IMPL = {
    SeatGetPointer,
    SeatGetKeyboard,
    SeatGetTouch,
    DestroyResource,
};

// wp_presentation

void PresentationFeedback(wl_client* client,
                          wl_resource* resource,
                          wl_resource* surfaceResource,
                          uint32_t id)
{
  wl_resource* feedback = wl_resource_create(client, &wp_presentation_feedback_interface,
                                             wl_resource_get_version(resource), id);
  if (!feedback)
  {
    wl_client_post_no_memory(client);
    return;
  }

  auto* surface = SurfaceOf(surfaceResource);
  if (surface)
    surface->AddPresentationFeedback(feedback);
  else
  {
    wp_presentation_feedback_send_discarded(feedback);
    wl_resource_destroy(feedback);
  }
}

const struct wp_presentation_interface PRESENTATION_IMPL = {
    DestroyResource,
    PresentationFeedback,
};

// zwp_text_input_v3

CWaylandHost::TextInput* TextInputOf(wl_resource* resource)
{
  return static_cast<CWaylandHost::TextInput*>(wl_resource_get_user_data(resource));
}

void TextInputEnable(wl_client*, wl_resource* resource)
{
  if (auto* input = TextInputOf(resource))
    input->host->OnTextInputEnable(*input, true);
}

void TextInputDisable(wl_client*, wl_resource* resource)
{
  if (auto* input = TextInputOf(resource))
    input->host->OnTextInputEnable(*input, false);
}

void TextInputSetSurroundingText(
    wl_client*, wl_resource* resource, const char* text, int32_t cursor, int32_t)
{
  if (auto* input = TextInputOf(resource))
    input->host->OnTextInputSurroundingText(*input, text, cursor);
}

void TextInputSetContentType(wl_client*, wl_resource* resource, uint32_t hint, uint32_t purpose)
{
  if (auto* input = TextInputOf(resource))
    input->host->OnTextInputContentType(*input, hint, purpose);
}

void TextInputSetCursorRectangle(
    wl_client*, wl_resource* resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
  if (auto* input = TextInputOf(resource))
    input->host->OnTextInputCursorRectangle(*input, x, y, width, height);
}

void TextInputCommit(wl_client*, wl_resource* resource)
{
  if (auto* input = TextInputOf(resource))
    input->host->OnTextInputCommit(*input);
}

constexpr struct zwp_text_input_v3_interface TEXT_INPUT_IMPL = []
{
  struct zwp_text_input_v3_interface impl = {};
  impl.destroy = DestroyResource;
  impl.enable = TextInputEnable;
  impl.disable = TextInputDisable;
  impl.set_surrounding_text = TextInputSetSurroundingText;
  impl.set_text_change_cause = Ignore;
  impl.set_content_type = TextInputSetContentType;
  impl.set_cursor_rectangle = TextInputSetCursorRectangle;
  impl.commit = TextInputCommit;
  return impl;
}();

void TextInputResourceDestroyed(wl_resource* resource)
{
  if (auto* input = TextInputOf(resource))
    input->host->OnTextInputResourceDestroyed(*input);
}

void TextInputManagerGetTextInput(wl_client* client,
                                  wl_resource* resource,
                                  uint32_t id,
                                  wl_resource*)
{
  HostOf(resource).AddTextInput(client, wl_resource_get_version(resource), id);
}

const struct zwp_text_input_manager_v3_interface TEXT_INPUT_MANAGER_IMPL = {
    DestroyResource,
    TextInputManagerGetTextInput,
};

// kodi_web_v1

CWaylandHost::WebView* ViewOf(wl_resource* resource)
{
  return static_cast<CWaylandHost::WebView*>(wl_resource_get_user_data(resource));
}

void WebViewUrlChanged(wl_client*, wl_resource* resource, const char* url)
{
  if (auto* view = ViewOf(resource))
    view->host->OnWebViewUrl(*view, url);
}

void WebViewTitleChanged(wl_client*, wl_resource* resource, const char* title)
{
  if (auto* view = ViewOf(resource))
    view->host->OnWebViewTitle(*view, title);
}

void WebViewFavicon(wl_client*, wl_resource* resource, wl_resource* buffer)
{
  if (auto* view = ViewOf(resource))
    view->host->OnWebViewFavicon(*view, buffer);
}

void WebViewLoadProgress(wl_client*, wl_resource* resource, uint32_t percent)
{
  if (auto* view = ViewOf(resource))
    view->host->OnWebViewProgress(*view, percent);
}

void WebViewNavigationState(wl_client*,
                            wl_resource* resource,
                            uint32_t canGoBack,
                            uint32_t canGoForward)
{
  if (auto* view = ViewOf(resource))
    view->host->OnWebViewNavigationState(*view, canGoBack, canGoForward);
}

void WebViewError(wl_client*, wl_resource* resource, uint32_t code, const char* message)
{
  if (auto* view = ViewOf(resource))
    view->host->OnWebViewError(*view, code, message);
}

void WebViewExit(wl_client*, wl_resource* resource)
{
  if (auto* view = ViewOf(resource))
    view->host->OnWebViewExit(*view);
}

void WebViewMediaState(
    wl_client*, wl_resource* resource, uint32_t video, uint32_t audio, uint32_t paused)
{
  if (auto* view = ViewOf(resource))
    view->host->OnWebViewMediaState(*view, video != 0, audio != 0, paused != 0);
}

const struct kodi_web_view_v1_interface WEB_VIEW_IMPL = {
    DestroyResource, WebViewUrlChanged,   WebViewTitleChanged,
    WebViewFavicon,  WebViewLoadProgress, WebViewNavigationState,
    WebViewError,    WebViewExit,         WebViewMediaState,
};

void WebViewResourceDestroyed(wl_resource* resource)
{
  if (auto* view = ViewOf(resource))
    view->host->OnWebViewResourceDestroyed(*view);
}

void WebManagerCreateView(wl_client* client,
                          wl_resource* resource,
                          uint32_t id,
                          const char* profile,
                          wl_resource* surface)
{
  HostOf(resource).OnWebViewCreate(client, resource, id, profile, surface);
}

void WebManagerReady(wl_client*, wl_resource* resource)
{
  HostOf(resource).OnWebClientReady();
}

void WebManagerViewUnavailable(wl_client*,
                               wl_resource* resource,
                               const char* profile,
                               const char* message)
{
  HostOf(resource).OnWebViewUnavailable(profile, message);
}

void WebManagerContentFilterListPrepared(wl_client*,
                                         wl_resource* resource,
                                         const char* name,
                                         uint32_t prepared)
{
  HostOf(resource).OnWebFilterListPrepared(name, prepared != 0);
}

const struct kodi_web_manager_v1_interface WEB_MANAGER_IMPL = {
    DestroyResource,
    WebManagerCreateView,
    WebManagerReady,
    WebManagerViewUnavailable,
    WebManagerContentFilterListPrepared,
};

void WebManagerResourceDestroyed(wl_resource* resource)
{
  HostOf(resource).OnWebManagerUnbound(resource);
}

wl_resource* BindGlobal(wl_client* client,
                        const wl_interface& interface,
                        uint32_t version,
                        uint32_t maxVersion,
                        uint32_t id,
                        const void* implementation,
                        void* data)
{
  wl_resource* resource = wl_resource_create(client, &interface, std::min(version, maxVersion), id);
  if (!resource)
  {
    wl_client_post_no_memory(client);
    return nullptr;
  }

  wl_resource_set_implementation(resource, implementation, data, nullptr);
  return resource;
}

} // unnamed namespace

CWaylandTaskQueue::~CWaylandTaskQueue()
{
  Close();
}

int CWaylandTaskQueue::Open()
{
  std::unique_lock lock(m_lock);
  if (m_wakeFd < 0)
    m_wakeFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  return m_wakeFd;
}

void CWaylandTaskQueue::Close()
{
  std::unique_lock lock(m_lock);
  m_tasks.clear();
  if (m_wakeFd >= 0)
    close(m_wakeFd);
  m_wakeFd = -1;
}

void CWaylandTaskQueue::Post(std::function<void()> task)
{
  std::unique_lock lock(m_lock);
  if (m_wakeFd < 0)
    return;

  m_tasks.push_back(std::move(task));
  const uint64_t one = 1;
  if (write(m_wakeFd, &one, sizeof(one)) != sizeof(one))
    CLog::Log(LOGERROR, "CWaylandTaskQueue::{}: wake failed: {}", __FUNCTION__, strerror(errno));
}

void CWaylandTaskQueue::Run()
{
  std::list<std::function<void()>> tasks;
  {
    std::unique_lock lock(m_lock);
    tasks.swap(m_tasks);
  }

  for (auto& task : tasks)
    task();
}

CWaylandHost::CWaylandHost(IWaylandHostObserver& observer, WaylandHostConfig config)
  : CThread("waylandhost"),
    m_observer(observer),
    m_config(std::move(config))
{
}

CWaylandHost::~CWaylandHost()
{
  Stop();
}

bool CWaylandHost::Start()
{
  if (m_display)
    return true;

  m_display = wl_display_create();
  if (!m_display)
  {
    CLog::Log(LOGERROR, "CWaylandHost::{}: failed to create the display", __FUNCTION__);
    return false;
  }
  m_loop = wl_display_get_event_loop(m_display);

  const int wakeFd = m_tasks->Open();
  if (wakeFd < 0)
  {
    CLog::Log(LOGERROR, "CWaylandHost::{}: eventfd failed: {}", __FUNCTION__, strerror(errno));
    DestroyAll();
    return false;
  }
  m_wakeSource = wl_event_loop_add_fd(m_loop, wakeFd, WL_EVENT_READABLE, OnWake, this);

  if (!CreateKeymap() || !CreateFormatTable() || !CreateGlobals())
  {
    DestroyAll();
    return false;
  }

  Create();
  return true;
}

void CWaylandHost::Stop()
{
  if (!m_display)
    return;

  m_bStop = true;
  Post([]() {});
  StopThread(true);
  DestroyAll();
}

void CWaylandHost::Post(std::function<void()> task)
{
  m_tasks->Post(std::move(task));
}

void CWaylandHost::SpawnClient(std::vector<std::string> argv,
                               std::map<std::string, std::string> env)
{
  Post([this, argv = std::move(argv), env = std::move(env)]() { DoSpawnClient(argv, env); });
}

void CWaylandHost::TerminateClients()
{
  Post([this]() { DoTerminateClients(); });
}

void CWaylandHost::KeyboardKey(uint32_t evdevCode, bool pressed)
{
  Post([this, evdevCode, pressed]() { DoKeyboardKey(evdevCode, pressed); });
}

void CWaylandHost::PointerMotion(int32_t x, int32_t y)
{
  Post([this, x, y]() { DoPointerMotion(x, y); });
}

void CWaylandHost::PointerButton(uint32_t evdevButton, bool pressed)
{
  Post([this, evdevButton, pressed]() { DoPointerButton(evdevButton, pressed); });
}

void CWaylandHost::PointerAxis(int32_t value120)
{
  Post([this, value120]() { DoPointerAxis(value120); });
}

void CWaylandHost::PointerScroll(int32_t x, int32_t y, int32_t pixels)
{
  Post([this, x, y, pixels]() { DoPointerScroll(x, y, pixels); });
}

void CWaylandHost::TouchDown(int32_t id, int32_t x, int32_t y)
{
  Post([this, id, x, y]() { DoTouchDown(id, x, y); });
}

void CWaylandHost::TouchMotion(int32_t id, int32_t x, int32_t y)
{
  Post([this, id, x, y]() { DoTouchMotion(id, x, y); });
}

void CWaylandHost::TouchUp(int32_t id)
{
  Post([this, id]() { DoTouchUp(id); });
}

void CWaylandHost::TouchFrame()
{
  Post([this]() { DoTouchFrame(); });
}

void CWaylandHost::TouchCancel()
{
  Post([this]() { DoTouchCancel(); });
}

void CWaylandHost::FrameDone()
{
  Post([this]() { DoFrameDone(); });
}

uint32_t CWaylandHost::NextSerial()
{
  return ++m_serial;
}

void CWaylandHost::Process()
{
  while (!m_bStop)
  {
    wl_display_flush_clients(m_display);
    if (wl_event_loop_dispatch(m_loop, -1) < 0 && errno != EINTR)
    {
      CLog::Log(LOGERROR, "CWaylandHost::{}: event loop failed: {}", __FUNCTION__, strerror(errno));
      break;
    }
  }
}

int CWaylandHost::OnWake(int fd, uint32_t, void* data)
{
  uint64_t count;
  while (read(fd, &count, sizeof(count)) == sizeof(count))
  {
  }

  static_cast<CWaylandHost*>(data)->m_tasks->Run();
  return 0;
}

bool CWaylandHost::CreateKeymap()
{
  m_xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  if (!m_xkbContext)
    return false;

  xkb_rule_names names{};
  names.layout = m_config.keyboardLayout.empty() ? nullptr : m_config.keyboardLayout.c_str();
  m_xkbKeymap = xkb_keymap_new_from_names(m_xkbContext, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
  if (!m_xkbKeymap)
  {
    CLog::Log(LOGERROR, "CWaylandHost::{}: no keymap for layout \"{}\"", __FUNCTION__,
              m_config.keyboardLayout);
    return false;
  }
  m_xkbState = xkb_state_new(m_xkbKeymap);

  char* keymapString = xkb_keymap_get_as_string(m_xkbKeymap, XKB_KEYMAP_FORMAT_TEXT_V1);
  if (!keymapString)
    return false;

  m_keymapSize = strlen(keymapString) + 1;
  m_keymapFd = SealedMemfd("kodi-keymap", keymapString, m_keymapSize);
  free(keymapString);
  return m_keymapFd >= 0;
}

bool CWaylandHost::CreateFormatTable()
{
  struct stat st = {};
  if (stat(m_config.renderDevicePath.c_str(), &st) != 0)
  {
    CLog::Log(LOGERROR, "CWaylandHost::{}: cannot stat render device {}: {}", __FUNCTION__,
              m_config.renderDevicePath, strerror(errno));
    return false;
  }
  m_renderDevice = st.st_rdev;

  std::vector<FormatTableEntry> table;
  for (uint32_t format : DMABUF_FORMATS)
    table.push_back({format, 0, DRM_FORMAT_MOD_LINEAR});
  m_formatTableSize = table.size() * sizeof(FormatTableEntry);
  m_formatTableFd = SealedMemfd("kodi-dmabuf-formats", table.data(), m_formatTableSize);
  return m_formatTableFd >= 0;
}

int CWaylandHost::SealedMemfd(const char* name, const void* data, size_t size)
{
  const int fd = memfd_create(name, MFD_CLOEXEC | MFD_ALLOW_SEALING);
  if (fd >= 0 && write(fd, data, size) == static_cast<ssize_t>(size) &&
      fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_SEAL) == 0)
    return fd;

  CLog::Log(LOGERROR, "CWaylandHost::{}: {} memfd failed: {}", __FUNCTION__, name, strerror(errno));
  if (fd >= 0)
    close(fd);
  return -1;
}

bool CWaylandHost::CreateGlobals()
{
  if (wl_display_init_shm(m_display) != 0)
    return false;

  m_globals.push_back(wl_global_create(m_display, &wl_compositor_interface, COMPOSITOR_VERSION,
                                       this, BindCompositor));
  m_globals.push_back(
      wl_global_create(m_display, &wl_output_interface, OUTPUT_VERSION, this, BindOutput));
  m_globals.push_back(
      wl_global_create(m_display, &wl_seat_interface, SEAT_VERSION, this, BindSeat));
  m_globals.push_back(wl_global_create(m_display, &xdg_wm_base_interface, XDG_WM_BASE_VERSION, this,
                                       BindXdgWmBase));
  m_globals.push_back(wl_global_create(m_display, &zwp_linux_dmabuf_v1_interface,
                                       LINUX_DMABUF_VERSION, this, BindLinuxDmabuf));
  m_globals.push_back(wl_global_create(m_display, &wp_presentation_interface, PRESENTATION_VERSION,
                                       this, BindPresentation));
  m_globals.push_back(wl_global_create(m_display, &zwp_text_input_manager_v3_interface,
                                       TEXT_INPUT_VERSION, this, BindTextInputManager));
  m_globals.push_back(wl_global_create(m_display, &kodi_web_manager_v1_interface,
                                       WEB_MANAGER_VERSION, this, BindWebManager));

  return std::all_of(m_globals.begin(), m_globals.end(), [](wl_global* g) { return g; });
}

void CWaylandHost::DestroyAll()
{
  DoTerminateClients();
  if (m_display)
    wl_display_destroy_clients(m_display);
  for (Client& client : m_clients)
  {
    if (client.exitSource)
      wl_event_source_remove(client.exitSource);
    if (client.pid > 0)
      ReapClient(client);
    if (client.pidfd >= 0)
      close(client.pidfd);
  }
  m_clients.clear();
  m_views.clear();
  m_textInputs.clear();
  m_webManager = nullptr;

  for (wl_global* global : m_globals)
  {
    if (global)
      wl_global_destroy(global);
  }
  m_globals.clear();
  if (m_wakeSource)
  {
    wl_event_source_remove(m_wakeSource);
    m_wakeSource = nullptr;
  }
  if (m_display)
  {
    wl_display_destroy(m_display);
    m_display = nullptr;
    m_loop = nullptr;
  }
  m_tasks->Close();
  for (int* fd : {&m_keymapFd, &m_formatTableFd})
  {
    if (*fd >= 0)
      close(*fd);
    *fd = -1;
  }
  if (m_xkbState)
    xkb_state_unref(m_xkbState);
  if (m_xkbKeymap)
    xkb_keymap_unref(m_xkbKeymap);
  if (m_xkbContext)
    xkb_context_unref(m_xkbContext);
  m_xkbState = nullptr;
  m_xkbKeymap = nullptr;
  m_xkbContext = nullptr;
}

void CWaylandHost::DoSpawnClient(const std::vector<std::string>& argv,
                                 const std::map<std::string, std::string>& env)
{
  if (argv.empty())
    return;

  int fds[2];
  if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds) != 0)
  {
    CLog::Log(LOGERROR, "CWaylandHost::{}: socketpair failed: {}", __FUNCTION__, strerror(errno));
    m_observer.OnClientExited(-1, -1);
    return;
  }

  // everything the child needs is prepared before fork(), only
  // async-signal-safe calls happen between fork() and exec
  std::vector<std::string> envStrings;
  for (char** entry = environ; entry && *entry; ++entry)
  {
    std::string pair = *entry;
    const std::string name = pair.substr(0, pair.find('='));
    if (name == "WAYLAND_DISPLAY" || name == "WAYLAND_SOCKET" || env.count(name))
      continue;
    envStrings.push_back(std::move(pair));
  }
  for (const auto& [name, value] : env)
    envStrings.push_back(name + "=" + value);
  envStrings.push_back("WAYLAND_SOCKET=3");

  std::vector<char*> envp;
  for (auto& entry : envStrings)
    envp.push_back(entry.data());
  envp.push_back(nullptr);

  std::vector<std::string> argStrings = argv;
  std::vector<char*> args;
  for (auto& entry : argStrings)
    args.push_back(entry.data());
  args.push_back(nullptr);

  const pid_t pid = fork();
  if (pid < 0)
  {
    CLog::Log(LOGERROR, "CWaylandHost::{}: fork failed: {}", __FUNCTION__, strerror(errno));
    close(fds[0]);
    close(fds[1]);
    m_observer.OnClientExited(-1, -1);
    return;
  }
  if (pid == 0)
  {
    if (dup2(fds[1], 3) == 3)
      execvpe(args[0], args.data(), envp.data());
    _exit(127);
  }

  close(fds[1]);

  m_clients.emplace_back();
  Client& client = m_clients.back();
  client.host = this;
  client.pid = pid;
  client.client = wl_client_create(m_display, fds[0]);
  if (!client.client)
  {
    CLog::Log(LOGERROR, "CWaylandHost::{}: wl_client_create failed", __FUNCTION__);
    close(fds[0]);
    kill(pid, SIGTERM);
  }

  client.pidfd = static_cast<int>(syscall(SYS_pidfd_open, pid, 0));
  if (client.pidfd < 0)
  {
    // a client whose exit goes unnoticed could never be replaced
    CLog::Log(LOGERROR, "CWaylandHost::{}: pidfd_open failed: {}", __FUNCTION__, strerror(errno));
    kill(pid, SIGKILL);
    waitpid(pid, nullptr, 0);
    m_clients.pop_back();
    m_observer.OnClientExited(-1, -1);
    return;
  }
  client.exitSource =
      wl_event_loop_add_fd(m_loop, client.pidfd, WL_EVENT_READABLE, OnClientExitStatic, &client);

  CLog::Log(LOGINFO, "CWaylandHost::{}: started {} as pid {}", __FUNCTION__, argv[0], pid);
}

int CWaylandHost::OnClientExitStatic(int, uint32_t, void* data)
{
  auto* client = static_cast<Client*>(data);
  client->host->OnClientExit(*client);
  return 0;
}

void CWaylandHost::OnClientExit(Client& client)
{
  int status = -1;
  pid_t reaped;
  do
    reaped = waitpid(client.pid, &status, WNOHANG);
  while (reaped < 0 && errno == EINTR);
  if (reaped == 0)
    return;
  // a readable pidfd means the client has ended; another waitpid() may have
  // reaped it, which leaves its status unknown
  if (reaped < 0)
    status = -1;

  CLog::Log(LOGINFO, "CWaylandHost::{}: client pid {} exited with status {}", __FUNCTION__,
            client.pid, status);
  wl_event_source_remove(client.exitSource);
  close(client.pidfd);
  const pid_t pid = client.pid;
  m_clients.remove_if([&client](const Client& entry) { return &entry == &client; });
  m_observer.OnClientExited(pid, status);
}

void CWaylandHost::ReapClient(const Client& client)
{
  struct pollfd exited = {client.pidfd, POLLIN, 0};
  const int timeout = static_cast<int>(std::chrono::milliseconds(CLIENT_EXIT_TIMEOUT).count());

  if (client.pidfd < 0 || poll(&exited, 1, timeout) <= 0)
  {
    CLog::Log(LOGWARNING, "CWaylandHost::{}: client pid {} did not exit, killing it", __FUNCTION__,
              client.pid);
    kill(client.pid, SIGKILL);
  }
  waitpid(client.pid, nullptr, 0);
}

void CWaylandHost::DoTerminateClients()
{
  for (Client& client : m_clients)
  {
    if (client.pid > 0)
      kill(client.pid, SIGTERM);
  }
}

void CWaylandHost::BindCompositor(wl_client* client, void* data, uint32_t version, uint32_t id)
{
  BindGlobal(client, wl_compositor_interface, version, COMPOSITOR_VERSION, id, &COMPOSITOR_IMPL,
             data);
}

void CWaylandHost::BindOutput(wl_client* client, void* data, uint32_t version, uint32_t id)
{
  auto* host = static_cast<CWaylandHost*>(data);
  wl_resource* resource =
      BindGlobal(client, wl_output_interface, version, OUTPUT_VERSION, id, nullptr, data);
  if (!resource)
    return;

  wl_resource_set_destructor(resource, OutputDestroyed);
  host->m_outputs.push_back(resource);
  wl_output_send_geometry(resource, 0, 0, host->m_config.physicalWidth,
                          host->m_config.physicalHeight, WL_OUTPUT_SUBPIXEL_UNKNOWN, "Kodi", "Kodi",
                          WL_OUTPUT_TRANSFORM_NORMAL);
  host->SendOutputMode(resource);
}

void CWaylandHost::OutputDestroyed(wl_resource* resource)
{
  static_cast<CWaylandHost*>(wl_resource_get_user_data(resource))->m_outputs.remove(resource);
}

void CWaylandHost::SendOutputMode(wl_resource* output) const
{
  wl_output_send_mode(output, WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED, m_config.width,
                      m_config.height, m_config.refreshMilliHz);
  if (wl_resource_get_version(output) >= WL_OUTPUT_SCALE_SINCE_VERSION)
    wl_output_send_scale(output, 1);
  if (wl_resource_get_version(output) >= WL_OUTPUT_DONE_SINCE_VERSION)
    wl_output_send_done(output);
}

void CWaylandHost::SetOutputMode(int32_t width, int32_t height, int32_t refreshMilliHz)
{
  Post([this, width, height, refreshMilliHz] { DoSetOutputMode(width, height, refreshMilliHz); });
}

void CWaylandHost::DoSetOutputMode(int32_t width, int32_t height, int32_t refreshMilliHz)
{
  if (width == m_config.width && height == m_config.height &&
      refreshMilliHz == m_config.refreshMilliHz)
    return;

  CLog::Log(LOGINFO, "CWaylandHost::{}: {}x{} at {} mHz", __FUNCTION__, width, height,
            refreshMilliHz);
  m_config.width = width;
  m_config.height = height;
  m_config.refreshMilliHz = refreshMilliHz;
  for (wl_resource* output : m_outputs)
    SendOutputMode(output);
  for (CWaylandSurface* surface : m_surfaces)
    surface->SendConfigure(width, height);
  wl_display_flush_clients(m_display);
}

void CWaylandHost::BindSeat(wl_client* client, void* data, uint32_t version, uint32_t id)
{
  wl_resource* resource =
      BindGlobal(client, wl_seat_interface, version, SEAT_VERSION, id, &SEAT_IMPL, data);
  if (!resource)
    return;

  wl_seat_send_capabilities(resource, WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD |
                                          WL_SEAT_CAPABILITY_TOUCH);
  if (wl_resource_get_version(resource) >= WL_SEAT_NAME_SINCE_VERSION)
    wl_seat_send_name(resource, "seat0");
}

void CWaylandHost::BindXdgWmBase(wl_client* client, void* data, uint32_t version, uint32_t id)
{
  BindGlobal(client, xdg_wm_base_interface, version, XDG_WM_BASE_VERSION, id, &WM_BASE_IMPL, data);
}

void CWaylandHost::BindLinuxDmabuf(wl_client* client, void* data, uint32_t version, uint32_t id)
{
  wl_resource* resource = BindGlobal(client, zwp_linux_dmabuf_v1_interface, version,
                                     LINUX_DMABUF_VERSION, id, &DMABUF_IMPL, data);
  if (!resource)
    return;

  if (wl_resource_get_version(resource) < ZWP_LINUX_DMABUF_V1_GET_DEFAULT_FEEDBACK_SINCE_VERSION)
  {
    for (uint32_t format : DMABUF_FORMATS)
      zwp_linux_dmabuf_v1_send_format(resource, format);
  }
}

void CWaylandHost::BindPresentation(wl_client* client, void* data, uint32_t version, uint32_t id)
{
  wl_resource* resource = BindGlobal(client, wp_presentation_interface, version,
                                     PRESENTATION_VERSION, id, &PRESENTATION_IMPL, data);
  if (resource)
    wp_presentation_send_clock_id(resource, CLOCK_MONOTONIC);
}

void CWaylandHost::BindTextInputManager(wl_client* client,
                                        void* data,
                                        uint32_t version,
                                        uint32_t id)
{
  BindGlobal(client, zwp_text_input_manager_v3_interface, version, TEXT_INPUT_VERSION, id,
             &TEXT_INPUT_MANAGER_IMPL, data);
}

void CWaylandHost::BindWebManager(wl_client* client, void* data, uint32_t version, uint32_t id)
{
  wl_resource* resource = BindGlobal(client, kodi_web_manager_v1_interface, version,
                                     WEB_MANAGER_VERSION, id, &WEB_MANAGER_IMPL, data);
  if (!resource)
    return;

  wl_resource_set_destructor(resource, WebManagerResourceDestroyed);
  static_cast<CWaylandHost*>(data)->OnWebManagerBound(resource);
}

void CWaylandHost::OnWebManagerBound(wl_resource* manager)
{
  if (m_webManager)
    CLog::Log(LOGWARNING, "CWaylandHost::{}: a second web client bound the manager", __FUNCTION__);
  m_webManager = manager;
  SendLayoutDirection();
  for (const auto& [name, path] : m_filterLists)
    SendContentFilterList(name, path);
}

void CWaylandHost::SendLayoutDirection()
{
  kodi_web_manager_v1_send_layout_direction(
      m_webManager, m_rightToLeft ? KODI_WEB_MANAGER_V1_LAYOUT_DIRECTION_RIGHT_TO_LEFT
                                  : KODI_WEB_MANAGER_V1_LAYOUT_DIRECTION_LEFT_TO_RIGHT);
}

void CWaylandHost::OnWebManagerUnbound(wl_resource* manager)
{
  if (m_webManager == manager)
    m_webManager = nullptr;
}

void CWaylandHost::OnWebClientReady()
{
  CLog::Log(LOGINFO, "CWaylandHost::{}: the web client is ready", __FUNCTION__);
  m_observer.OnWebClientReady();
}

void CWaylandHost::OnWebViewCreate(wl_client* client,
                                   wl_resource* manager,
                                   uint32_t id,
                                   const char* profile,
                                   wl_resource* surfaceResource)
{
  if (!IsValidWebProfile(profile))
  {
    wl_resource_post_error(manager, KODI_WEB_MANAGER_V1_ERROR_INVALID_PROFILE,
                           "invalid profile identifier");
    return;
  }

  if (FindWebView(profile))
  {
    wl_resource_post_error(manager, KODI_WEB_MANAGER_V1_ERROR_DUPLICATE_PROFILE,
                           "a view for profile %s exists", profile);
    return;
  }

  auto* surface = SurfaceOf(surfaceResource);
  if (!surface || WebViewOf(surface))
  {
    wl_resource_post_error(manager, KODI_WEB_MANAGER_V1_ERROR_INVALID_SURFACE,
                           "the surface belongs to another view");
    return;
  }

  wl_resource* resource =
      wl_resource_create(client, &kodi_web_view_v1_interface, wl_resource_get_version(manager), id);
  if (!resource)
  {
    wl_client_post_no_memory(client);
    return;
  }

  m_views.push_back({this, resource, profile, surface});
  wl_resource_set_implementation(resource, &WEB_VIEW_IMPL, &m_views.back(),
                                 WebViewResourceDestroyed);
  CLog::Log(LOGINFO, "CWaylandHost::{}: view for profile \"{}\" created", __FUNCTION__, profile);
  m_observer.OnWebViewCreated(profile, *surface);
}

void CWaylandHost::OnWebViewUnavailable(const char* profile, const char* message)
{
  CLog::Log(LOGWARNING, "CWaylandHost::{}: no view for profile \"{}\": {}", __FUNCTION__, profile,
            message);
  m_observer.OnWebViewUnavailable(profile, message);
}

void CWaylandHost::OnWebFilterListPrepared(const char* name, bool prepared)
{
  CLog::Log(prepared ? LOGINFO : LOGWARNING, "CWaylandHost::{}: content filter list {} {}",
            __FUNCTION__, name, prepared ? "is prepared" : "cannot be prepared");
  m_observer.OnWebFilterListPrepared(name, prepared);
}

void CWaylandHost::OnWebViewUrl(WebView& view, const char* url)
{
  m_observer.OnWebViewUrl(view.profile, url);
}

void CWaylandHost::OnWebViewTitle(WebView& view, const char* title)
{
  m_observer.OnWebViewTitle(view.profile, title);
}

void CWaylandHost::OnWebViewFavicon(WebView& view, wl_resource* buffer)
{
  wl_shm_buffer* shm = wl_shm_buffer_get(buffer);
  CLog::Log(LOGDEBUG, "CWaylandHost::{}: profile \"{}\" icon {}x{}", __FUNCTION__, view.profile,
            shm ? wl_shm_buffer_get_width(shm) : 0, shm ? wl_shm_buffer_get_height(shm) : 0);
  // the icon pixels are not consumed yet; give the buffer straight back
  wl_buffer_send_release(buffer);
}

void CWaylandHost::OnWebViewProgress(WebView& view, uint32_t percent)
{
  m_observer.OnWebViewProgress(view.profile, percent);
}

void CWaylandHost::OnWebViewNavigationState(WebView& view,
                                            uint32_t canGoBack,
                                            uint32_t canGoForward)
{
  m_observer.OnWebViewNavigationState(view.profile, canGoBack != 0, canGoForward != 0);
}

void CWaylandHost::OnWebViewError(WebView& view, uint32_t code, const char* message)
{
  CLog::Log(LOGERROR, "CWaylandHost::{}: view for profile \"{}\" failed with code {}: {}",
            __FUNCTION__, view.profile, code, message);
  m_observer.OnWebViewError(view.profile, code, message);
}

void CWaylandHost::OnWebViewExit(WebView& view)
{
  CLog::Log(LOGINFO, "CWaylandHost::{}: the page of profile \"{}\" asks to leave", __FUNCTION__,
            view.profile);
  m_observer.OnWebViewExit(view.profile);
}

void CWaylandHost::OnWebViewMediaState(WebView& view, bool video, bool audio, bool paused)
{
  m_observer.OnWebViewMediaState(view.profile, video, audio, paused);
}

void CWaylandHost::OnWebViewResourceDestroyed(WebView& view)
{
  const std::string profile = view.profile;
  m_views.remove_if([&view](const WebView& entry) { return &entry == &view; });
  m_observer.OnWebViewDestroyed(profile);
}

CWaylandHost::WebView* CWaylandHost::FindWebView(const std::string& profile)
{
  auto it = std::find_if(m_views.begin(), m_views.end(),
                         [&profile](const WebView& view) { return view.profile == profile; });
  return it == m_views.end() ? nullptr : &*it;
}

CWaylandHost::WebView* CWaylandHost::WebViewOf(const CWaylandSurface* surface)
{
  auto it = std::find_if(m_views.begin(), m_views.end(),
                         [surface](const WebView& view) { return view.surface == surface; });
  return it == m_views.end() ? nullptr : &*it;
}

void CWaylandHost::WebWantView(std::string profile, std::string uplink)
{
  Post(
      [this, profile = std::move(profile), uplink = std::move(uplink)]()
      {
        if (m_webManager)
          kodi_web_manager_v1_send_want_view(m_webManager, profile.c_str(), uplink.c_str());
        else
          m_observer.OnWebViewUnavailable(profile, "no web client is connected");
      });
}

void CWaylandHost::WebViewLoadUrl(std::string profile, std::string url)
{
  Post(
      [this, profile = std::move(profile), url = std::move(url)]()
      {
        if (auto* view = FindWebView(profile))
          kodi_web_view_v1_send_load_url(view->resource, url.c_str());
      });
}

void CWaylandHost::WebViewNavigate(std::string profile, WebNavigation action)
{
  Post(
      [this, profile = std::move(profile), action]()
      {
        if (auto* view = FindWebView(profile))
          kodi_web_view_v1_send_navigate(view->resource, static_cast<uint32_t>(action));
      });
}

void CWaylandHost::WebViewSetActive(std::string profile, bool active)
{
  Post(
      [this, profile = std::move(profile), active]()
      {
        if (auto* view = FindWebView(profile))
          DoWebViewSetActive(*view, active);
      });
}

void CWaylandHost::DoWebViewSetActive(WebView& view, bool active)
{
  view.active = active;
  kodi_web_view_v1_send_set_active(view.resource, active ? 1 : 0);
  if (!view.surface)
    return;

  if (active && view.surface->IsMapped())
    SetKeyboardFocus(view.surface);
  else if (!active && m_keyboardFocus == view.surface)
    SetKeyboardFocus(nullptr);
}

void CWaylandHost::WebViewClose(std::string profile)
{
  Post(
      [this, profile = std::move(profile)]()
      {
        if (auto* view = FindWebView(profile))
          kodi_web_view_v1_send_close(view->resource);
      });
}

void CWaylandHost::WebViewSetUserAgent(std::string profile, std::string userAgent)
{
  Post(
      [this, profile = std::move(profile), userAgent = std::move(userAgent)]()
      {
        if (auto* view = FindWebView(profile))
          kodi_web_view_v1_send_set_user_agent(view->resource, userAgent.c_str());
      });
}

void CWaylandHost::WebViewSetViewport(std::string profile, uint32_t width, uint32_t height)
{
  Post(
      [this, profile = std::move(profile), width, height]()
      {
        auto* view = FindWebView(profile);
        if (view)
          kodi_web_view_v1_send_set_viewport(view->resource, width, height);
      });
}

void CWaylandHost::WebViewSetSoftwareVideoDecoding(std::string profile, bool allowed)
{
  Post(
      [this, profile = std::move(profile), allowed]()
      {
        auto* view = FindWebView(profile);
        if (view)
          kodi_web_view_v1_send_set_software_video_decoding(view->resource, allowed ? 1 : 0);
      });
}

void CWaylandHost::WebViewAddUserScript(std::string profile,
                                        std::string name,
                                        int fd,
                                        uint32_t size)
{
  Post([this, profile = std::move(profile), name = std::move(name), fd, size]()
       { SendWebViewFile(profile, name, fd, size, true); });
}

void CWaylandHost::WebViewAddContentFilter(std::string profile,
                                           std::string name,
                                           int fd,
                                           uint32_t size)
{
  Post([this, profile = std::move(profile), name = std::move(name), fd, size]()
       { SendWebViewFile(profile, name, fd, size, false); });
}

void CWaylandHost::WebViewAddContentFilterList(std::string profile, std::string name)
{
  Post(
      [this, profile = std::move(profile), name = std::move(name)]()
      {
        if (auto* view = FindWebView(profile))
          kodi_web_view_v1_send_add_content_filter_list(view->resource, name.c_str());
      });
}

void CWaylandHost::WebSetLayoutDirection(bool rightToLeft)
{
  Post(
      [this, rightToLeft]()
      {
        if (rightToLeft == m_rightToLeft)
          return;
        m_rightToLeft = rightToLeft;
        if (m_webManager)
          SendLayoutDirection();
      });
}

void CWaylandHost::WebSetContentFilterList(std::string name, std::string path)
{
  Post(
      [this, name = std::move(name), path = std::move(path)]()
      {
        if (path.empty())
        {
          if (m_filterLists.erase(name) && m_webManager)
            kodi_web_manager_v1_send_remove_content_filter_list(m_webManager, name.c_str());
          return;
        }
        m_filterLists[name] = path;
        if (m_webManager)
          SendContentFilterList(name, path);
      });
}

void CWaylandHost::SendContentFilterList(const std::string& name, const std::string& path)
{
  const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
  struct stat st = {};
  if (fd < 0 || fstat(fd, &st) != 0)
  {
    CLog::Log(LOGERROR, "CWaylandHost::{}: cannot read {}", __FUNCTION__, path);
    if (fd >= 0)
      close(fd);
    return;
  }

  kodi_web_manager_v1_send_content_filter_list(m_webManager, name.c_str(), fd,
                                               static_cast<uint32_t>(st.st_size));
  close(fd);
}

bool CWaylandHost::IsValidWebProfile(const std::string& profile)
{
  if (profile.empty() || profile.size() > WEB_PROFILE_MAX || profile == "." || profile == "..")
    return false;
  return std::all_of(profile.begin(), profile.end(),
                     [](char c)
                     {
                       return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                              (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
                     });
}

void CWaylandHost::SendWebViewFile(
    const std::string& profile, const std::string& name, int fd, uint32_t size, bool script)
{
  if (auto* view = FindWebView(profile))
  {
    if (script)
      kodi_web_view_v1_send_add_user_script(view->resource, name.c_str(), fd, size);
    else
      kodi_web_view_v1_send_add_content_filter(view->resource, name.c_str(), fd, size);
  }
  close(fd);
}

void CWaylandHost::SendDmabufFeedback(wl_resource* feedback)
{
  zwp_linux_dmabuf_feedback_v1_send_format_table(feedback, m_formatTableFd, m_formatTableSize);

  wl_array device;
  wl_array_init(&device);
  memcpy(wl_array_add(&device, sizeof(m_renderDevice)), &m_renderDevice, sizeof(m_renderDevice));
  zwp_linux_dmabuf_feedback_v1_send_main_device(feedback, &device);
  zwp_linux_dmabuf_feedback_v1_send_tranche_target_device(feedback, &device);
  wl_array_release(&device);

  zwp_linux_dmabuf_feedback_v1_send_tranche_flags(
      feedback, ZWP_LINUX_DMABUF_FEEDBACK_V1_TRANCHE_FLAGS_SCANOUT);

  wl_array indices;
  wl_array_init(&indices);
  const size_t count = m_formatTableSize / sizeof(FormatTableEntry);
  auto* index = static_cast<uint16_t*>(wl_array_add(&indices, count * sizeof(uint16_t)));
  for (size_t i = 0; i < count; ++i)
    index[i] = static_cast<uint16_t>(i);
  zwp_linux_dmabuf_feedback_v1_send_tranche_formats(feedback, &indices);
  wl_array_release(&indices);

  zwp_linux_dmabuf_feedback_v1_send_tranche_done(feedback);
  zwp_linux_dmabuf_feedback_v1_send_done(feedback);
}

void CWaylandHost::AddKeyboard(wl_client* client, uint32_t version, uint32_t id)
{
  wl_resource* keyboard = wl_resource_create(client, &wl_keyboard_interface, version, id);
  if (!keyboard)
  {
    wl_client_post_no_memory(client);
    return;
  }

  wl_resource_set_implementation(keyboard, &KEYBOARD_IMPL, this, KeyboardResourceDestroyed);
  m_keyboards.push_back(keyboard);

  wl_keyboard_send_keymap(keyboard, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1, m_keymapFd, m_keymapSize);
  // Kodi delivers its own key repeats, the client must not add any
  if (version >= WL_KEYBOARD_REPEAT_INFO_SINCE_VERSION)
    wl_keyboard_send_repeat_info(keyboard, 0, 0);

  if (m_keyboardFocus && wl_resource_get_client(m_keyboardFocus->GetResource()) == client)
    SendKeyboardEnterTo(keyboard, *m_keyboardFocus);
}

void CWaylandHost::RemoveKeyboard(wl_resource* keyboard)
{
  m_keyboards.remove(keyboard);
}

void CWaylandHost::AddPointer(wl_client* client, uint32_t version, uint32_t id)
{
  wl_resource* pointer = wl_resource_create(client, &wl_pointer_interface, version, id);
  if (!pointer)
  {
    wl_client_post_no_memory(client);
    return;
  }

  wl_resource_set_implementation(pointer, &POINTER_IMPL, this, PointerResourceDestroyed);
  m_pointers.push_back(pointer);

  if (m_pointerFocus && wl_resource_get_client(m_pointerFocus->GetResource()) == client)
    SendPointerEnterTo(pointer, *m_pointerFocus);
}

void CWaylandHost::RemovePointer(wl_resource* pointer)
{
  m_pointers.remove(pointer);
}

void CWaylandHost::AddTouch(wl_client* client, uint32_t version, uint32_t id)
{
  wl_resource* touch = wl_resource_create(client, &wl_touch_interface, version, id);
  if (!touch)
  {
    wl_client_post_no_memory(client);
    return;
  }

  wl_resource_set_implementation(touch, &TOUCH_IMPL, this, TouchResourceDestroyed);
  m_touches.push_back(touch);
}

void CWaylandHost::RemoveTouch(wl_resource* touch)
{
  m_touches.remove(touch);
}

void CWaylandHost::OnSurfaceCreated(CWaylandSurface* surface)
{
  m_surfaces.push_back(surface);
}

void CWaylandHost::OnSurfaceDestroyed(CWaylandSurface* surface)
{
  m_surfaces.remove(surface);
  for (WebView& view : m_views)
  {
    if (view.surface == surface)
      view.surface = nullptr;
  }
  if (m_keyboardFocus == surface)
    m_keyboardFocus = nullptr;
  if (m_pointerFocus == surface)
    m_pointerFocus = nullptr;
  if (m_touchFocus == surface)
  {
    m_touchFocus = nullptr;
    m_touchIds.clear();
  }
}

void CWaylandHost::OnSurfaceMapped(CWaylandSurface& surface)
{
  const WebView* view = WebViewOf(&surface);
  if (!view || view->active)
    SetKeyboardFocus(&surface);
  m_observer.OnSurfaceMapped(surface);
}

void CWaylandHost::OnSurfaceUnmapped(CWaylandSurface& surface)
{
  if (m_keyboardFocus == &surface)
    SetKeyboardFocus(nullptr);
  if (m_pointerFocus == &surface)
    SendPointerLeave();
  if (m_touchFocus == &surface)
    DoTouchCancel();
  m_observer.OnSurfaceUnmapped(surface);
}

void CWaylandHost::OnSurfaceReleased(CWaylandSurface& surface)
{
  m_observer.OnSurfaceDestroyed(surface);
}

void CWaylandHost::OnSurfaceFrame(CWaylandSurface& surface, std::shared_ptr<CWaylandBuffer> buffer)
{
  m_observer.OnSurfaceFrame(surface, std::move(buffer));
}

void CWaylandHost::SetKeyboardFocus(CWaylandSurface* surface)
{
  if (surface == m_keyboardFocus)
    return;

  if (m_keyboardFocus)
    SendKeyboardLeave(*m_keyboardFocus);
  // the pointer follows the keyboard focus on its next motion
  SendPointerLeave();
  m_keyboardFocus = surface;
  if (surface)
    SendKeyboardEnter(*surface);
}

void CWaylandHost::SendKeyboardEnter(CWaylandSurface& surface)
{
  wl_client* client = wl_resource_get_client(surface.GetResource());
  for (wl_resource* keyboard : m_keyboards)
  {
    if (wl_resource_get_client(keyboard) == client)
      SendKeyboardEnterTo(keyboard, surface);
  }
  SendTextInputFocus(&surface, true);
}

void CWaylandHost::SendTextInputFocus(CWaylandSurface* surface, bool entered)
{
  if (!surface)
    return;

  wl_client* client = wl_resource_get_client(surface->GetResource());
  for (TextInput& input : m_textInputs)
  {
    if (wl_resource_get_client(input.resource) != client)
      continue;
    if (entered)
      zwp_text_input_v3_send_enter(input.resource, surface->GetResource());
    else
      zwp_text_input_v3_send_leave(input.resource, surface->GetResource());
  }
}

void CWaylandHost::AddTextInput(wl_client* client, uint32_t version, uint32_t id)
{
  wl_resource* resource = wl_resource_create(client, &zwp_text_input_v3_interface, version, id);
  if (!resource)
  {
    wl_client_post_no_memory(client);
    return;
  }

  TextInput input;
  input.host = this;
  input.resource = resource;
  m_textInputs.push_back(std::move(input));
  wl_resource_set_implementation(resource, &TEXT_INPUT_IMPL, &m_textInputs.back(),
                                 TextInputResourceDestroyed);
  if (m_keyboardFocus && wl_resource_get_client(m_keyboardFocus->GetResource()) == client)
    zwp_text_input_v3_send_enter(resource, m_keyboardFocus->GetResource());
}

void CWaylandHost::OnTextInputEnable(TextInput& input, bool enable)
{
  input.pending.enabled = enable;
  if (!enable)
  {
    input.enableRequests = 0;
    return;
  }

  ++input.enableRequests;
  input.pending.surrounding.clear();
  input.pending.cursor = 0;
  input.pending.hint = 0;
  input.pending.purpose = 0;
  input.pending.caretX = 0;
  input.pending.caretY = 0;
  input.pending.caretWidth = 0;
  input.pending.caretHeight = 0;
}

void CWaylandHost::OnTextInputSurroundingText(TextInput& input, const char* text, int32_t cursor)
{
  input.pending.surrounding = text ? text : "";
  input.pending.cursor = cursor;
}

void CWaylandHost::OnTextInputContentType(TextInput& input, uint32_t hint, uint32_t purpose)
{
  input.pending.hint = hint;
  input.pending.purpose = purpose;
}

void CWaylandHost::OnTextInputCursorRectangle(
    TextInput& input, int32_t x, int32_t y, int32_t width, int32_t height)
{
  input.pending.caretX = x;
  input.pending.caretY = y;
  input.pending.caretWidth = width;
  input.pending.caretHeight = height;
}

void CWaylandHost::OnTextInputCommit(TextInput& input)
{
  input.current = input.pending;
  ++input.serial;
  CLog::Log(LOGDEBUG,
            "CWaylandHost::{}: text input {} after {} enable requests, caret {},{} {}x{}, "
            "cursor {} in \"{}\"",
            __FUNCTION__, input.current.enabled ? "enabled" : "disabled", input.enableRequests,
            input.current.caretX, input.current.caretY, input.current.caretWidth,
            input.current.caretHeight, input.current.cursor, input.current.surrounding);
  if (!input.current.enabled)
  {
    if (input.keyboardShown)
      m_observer.OnTextInputDisabled();
    input.keyboardShown = false;
    return;
  }

  // a page drawing its own keyboard enables the text input once; one that
  // wants the compositor's keyboard enables it again, as Mutter expects
  if (input.keyboardShown)
    m_observer.OnTextInputUpdated(input.current);
  else if (input.enableRequests >= 2)
  {
    input.keyboardShown = true;
    m_observer.OnTextInputEnabled(input.current);
  }
}

void CWaylandHost::OnTextInputResourceDestroyed(TextInput& input)
{
  const bool keyboardShown = input.keyboardShown;
  m_textInputs.remove_if([&input](const TextInput& entry) { return &entry == &input; });
  if (keyboardShown)
    m_observer.OnTextInputDisabled();
}

bool TextInputState::IsHidden() const
{
  return (hint & ZWP_TEXT_INPUT_V3_CONTENT_HINT_HIDDEN_TEXT) != 0 ||
         purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PASSWORD ||
         purpose == ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PIN;
}

void CWaylandHost::TextInputCommit(std::string text, uint32_t deleteBefore, uint32_t deleteAfter)
{
  Post(
      [this, text = std::move(text), deleteBefore, deleteAfter]()
      {
        for (TextInput& input : m_textInputs)
        {
          if (!input.current.enabled)
            continue;
          if (deleteBefore || deleteAfter)
            zwp_text_input_v3_send_delete_surrounding_text(input.resource, deleteBefore,
                                                           deleteAfter);
          zwp_text_input_v3_send_commit_string(input.resource, text.c_str());
          zwp_text_input_v3_send_done(input.resource, input.serial);
        }
      });
}

void CWaylandHost::SendKeyboardEnterTo(wl_resource* keyboard, CWaylandSurface& surface)
{
  wl_array keys;
  wl_array_init(&keys);
  wl_keyboard_send_enter(keyboard, NextSerial(), surface.GetResource(), &keys);
  wl_array_release(&keys);
  wl_keyboard_send_modifiers(keyboard, NextSerial(), m_modsDepressed, m_modsLatched, m_modsLocked,
                             m_modsGroup);
}

void CWaylandHost::SendKeyboardLeave(CWaylandSurface& surface)
{
  wl_client* client = wl_resource_get_client(surface.GetResource());
  const uint32_t serial = NextSerial();
  for (wl_resource* keyboard : m_keyboards)
  {
    if (wl_resource_get_client(keyboard) == client)
      wl_keyboard_send_leave(keyboard, serial, surface.GetResource());
  }
  SendTextInputFocus(&surface, false);
}

void CWaylandHost::SendPointerEnter(CWaylandSurface& surface)
{
  m_pointerFocus = &surface;
  wl_client* client = wl_resource_get_client(surface.GetResource());
  for (wl_resource* pointer : m_pointers)
  {
    if (wl_resource_get_client(pointer) == client)
      SendPointerEnterTo(pointer, surface);
  }
}

void CWaylandHost::SendPointerEnterTo(wl_resource* pointer, CWaylandSurface& surface)
{
  wl_pointer_send_enter(pointer, NextSerial(), surface.GetResource(), wl_fixed_from_int(m_pointerX),
                        wl_fixed_from_int(m_pointerY));
  if (wl_resource_get_version(pointer) >= WL_POINTER_FRAME_SINCE_VERSION)
    wl_pointer_send_frame(pointer);
}

void CWaylandHost::SendPointerLeave()
{
  if (!m_pointerFocus)
    return;

  wl_client* client = wl_resource_get_client(m_pointerFocus->GetResource());
  const uint32_t serial = NextSerial();
  for (wl_resource* pointer : m_pointers)
  {
    if (wl_resource_get_client(pointer) != client)
      continue;
    wl_pointer_send_leave(pointer, serial, m_pointerFocus->GetResource());
    if (wl_resource_get_version(pointer) >= WL_POINTER_FRAME_SINCE_VERSION)
      wl_pointer_send_frame(pointer);
  }
  m_pointerFocus = nullptr;
}

void CWaylandHost::SendModifiers()
{
  const uint32_t depressed = xkb_state_serialize_mods(m_xkbState, XKB_STATE_MODS_DEPRESSED);
  const uint32_t latched = xkb_state_serialize_mods(m_xkbState, XKB_STATE_MODS_LATCHED);
  const uint32_t locked = xkb_state_serialize_mods(m_xkbState, XKB_STATE_MODS_LOCKED);
  const uint32_t group = xkb_state_serialize_layout(m_xkbState, XKB_STATE_LAYOUT_EFFECTIVE);
  if (depressed == m_modsDepressed && latched == m_modsLatched && locked == m_modsLocked &&
      group == m_modsGroup)
    return;

  m_modsDepressed = depressed;
  m_modsLatched = latched;
  m_modsLocked = locked;
  m_modsGroup = group;
  if (!m_keyboardFocus)
    return;

  wl_client* client = wl_resource_get_client(m_keyboardFocus->GetResource());
  const uint32_t serial = NextSerial();
  for (wl_resource* keyboard : m_keyboards)
  {
    if (wl_resource_get_client(keyboard) == client)
      wl_keyboard_send_modifiers(keyboard, serial, depressed, latched, locked, group);
  }
}

void CWaylandHost::DoKeyboardKey(uint32_t evdevCode, bool pressed)
{
  xkb_state_update_key(m_xkbState, evdevCode + XKB_KEYCODE_OFFSET,
                       pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
  if (m_keyboardFocus)
  {
    wl_client* client = wl_resource_get_client(m_keyboardFocus->GetResource());
    const uint32_t serial = NextSerial();
    const uint32_t time = Milliseconds(Now());
    const uint32_t state = pressed ? WL_KEYBOARD_KEY_STATE_PRESSED : WL_KEYBOARD_KEY_STATE_RELEASED;
    for (wl_resource* keyboard : m_keyboards)
    {
      if (wl_resource_get_client(keyboard) == client)
        wl_keyboard_send_key(keyboard, serial, time, evdevCode, state);
    }
  }
  SendModifiers();
}

void CWaylandHost::DoPointerMotion(int32_t x, int32_t y)
{
  m_pointerX = x;
  m_pointerY = y;
  if (!m_keyboardFocus)
    return;

  if (m_pointerFocus != m_keyboardFocus)
  {
    SendPointerEnter(*m_keyboardFocus);
    return;
  }

  wl_client* client = wl_resource_get_client(m_pointerFocus->GetResource());
  const uint32_t time = Milliseconds(Now());
  for (wl_resource* pointer : m_pointers)
  {
    if (wl_resource_get_client(pointer) != client)
      continue;
    wl_pointer_send_motion(pointer, time, wl_fixed_from_int(x), wl_fixed_from_int(y));
    if (wl_resource_get_version(pointer) >= WL_POINTER_FRAME_SINCE_VERSION)
      wl_pointer_send_frame(pointer);
  }
}

void CWaylandHost::DoPointerButton(uint32_t evdevButton, bool pressed)
{
  if (!m_pointerFocus)
    return;

  wl_client* client = wl_resource_get_client(m_pointerFocus->GetResource());
  const uint32_t serial = NextSerial();
  const uint32_t time = Milliseconds(Now());
  const uint32_t state =
      pressed ? WL_POINTER_BUTTON_STATE_PRESSED : WL_POINTER_BUTTON_STATE_RELEASED;
  for (wl_resource* pointer : m_pointers)
  {
    if (wl_resource_get_client(pointer) != client)
      continue;
    wl_pointer_send_button(pointer, serial, time, evdevButton, state);
    if (wl_resource_get_version(pointer) >= WL_POINTER_FRAME_SINCE_VERSION)
      wl_pointer_send_frame(pointer);
  }
}

void CWaylandHost::DoPointerAxis(int32_t value120)
{
  if (!m_pointerFocus)
    return;

  wl_client* client = wl_resource_get_client(m_pointerFocus->GetResource());
  const uint32_t time = Milliseconds(Now());
  for (wl_resource* pointer : m_pointers)
  {
    if (wl_resource_get_client(pointer) != client)
      continue;
    const uint32_t version = wl_resource_get_version(pointer);
    if (version >= WL_POINTER_AXIS_SOURCE_SINCE_VERSION)
      wl_pointer_send_axis_source(pointer, WL_POINTER_AXIS_SOURCE_WHEEL);
    if (version >= WL_POINTER_AXIS_DISCRETE_SINCE_VERSION)
      wl_pointer_send_axis_discrete(pointer, WL_POINTER_AXIS_VERTICAL_SCROLL, value120 / 120);
    wl_pointer_send_axis(pointer, time, WL_POINTER_AXIS_VERTICAL_SCROLL,
                         wl_fixed_from_int(value120 / AXIS_UNITS_PER_VALUE120));
    if (version >= WL_POINTER_FRAME_SINCE_VERSION)
      wl_pointer_send_frame(pointer);
  }
}

void CWaylandHost::DoPointerScroll(int32_t x, int32_t y, int32_t pixels)
{
  DoPointerMotion(x, y);
  if (!m_pointerFocus)
    return;

  wl_client* client = wl_resource_get_client(m_pointerFocus->GetResource());
  const uint32_t time = Milliseconds(Now());
  for (wl_resource* pointer : m_pointers)
  {
    if (wl_resource_get_client(pointer) != client)
      continue;
    const uint32_t version = wl_resource_get_version(pointer);
    // without a discrete step the client takes the value as pixels
    if (version >= WL_POINTER_AXIS_SOURCE_SINCE_VERSION)
      wl_pointer_send_axis_source(pointer, WL_POINTER_AXIS_SOURCE_FINGER);
    wl_pointer_send_axis(pointer, time, WL_POINTER_AXIS_VERTICAL_SCROLL, wl_fixed_from_int(pixels));
    if (version >= WL_POINTER_FRAME_SINCE_VERSION)
      wl_pointer_send_frame(pointer);
  }
}

std::vector<wl_resource*> CWaylandHost::TouchesOfFocus() const
{
  std::vector<wl_resource*> touches;
  if (!m_touchFocus)
    return touches;

  wl_client* client = wl_resource_get_client(m_touchFocus->GetResource());
  for (wl_resource* touch : m_touches)
  {
    if (wl_resource_get_client(touch) == client)
      touches.push_back(touch);
  }
  return touches;
}

void CWaylandHost::DoTouchDown(int32_t id, int32_t x, int32_t y)
{
  if (m_touchIds.empty())
    m_touchFocus = m_keyboardFocus;
  if (!m_touchFocus || !m_touchIds.insert(id).second)
    return;

  const uint32_t serial = NextSerial();
  const uint32_t time = Milliseconds(Now());
  for (wl_resource* touch : TouchesOfFocus())
    wl_touch_send_down(touch, serial, time, m_touchFocus->GetResource(), id, wl_fixed_from_int(x),
                       wl_fixed_from_int(y));
}

void CWaylandHost::DoTouchMotion(int32_t id, int32_t x, int32_t y)
{
  if (!m_touchIds.contains(id))
    return;

  const uint32_t time = Milliseconds(Now());
  for (wl_resource* touch : TouchesOfFocus())
    wl_touch_send_motion(touch, time, id, wl_fixed_from_int(x), wl_fixed_from_int(y));
}

void CWaylandHost::DoTouchUp(int32_t id)
{
  if (m_touchIds.erase(id) == 0)
    return;

  const uint32_t serial = NextSerial();
  const uint32_t time = Milliseconds(Now());
  for (wl_resource* touch : TouchesOfFocus())
    wl_touch_send_up(touch, serial, time, id);
}

void CWaylandHost::DoTouchFrame()
{
  for (wl_resource* touch : TouchesOfFocus())
    wl_touch_send_frame(touch);
  if (m_touchIds.empty())
    m_touchFocus = nullptr;
}

void CWaylandHost::DoTouchCancel()
{
  for (wl_resource* touch : TouchesOfFocus())
    wl_touch_send_cancel(touch);
  m_touchIds.clear();
  m_touchFocus = nullptr;
}

void CWaylandHost::DoFrameDone()
{
  const timespec now = Now();
  const uint32_t time = Milliseconds(now);
  const uint32_t refreshNs = m_config.refreshMilliHz > 0
                                 ? static_cast<uint32_t>(1000000000000ULL / m_config.refreshMilliHz)
                                 : 0;
  for (CWaylandSurface* surface : m_surfaces)
    surface->FrameDone(time, now, refreshNs);
}
