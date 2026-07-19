/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMAtomic.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "settings/Settings.h"
#include "utils/log.h"

#include <chrono>
#include <errno.h>
#include <string.h>
#include <thread>

#include <drm_fourcc.h>
#include <drm_mode.h>
#include <fcntl.h>
#include <unistd.h>

using namespace KODI::WINDOWING::GBM;

void CDRMAtomic::DrmAtomicCommit(int fb_id, int flags, bool rendered, bool videoLayer)
{
  // Declared at function scope so the blob outlives drmModeAtomicCommit.
  // DRM requires the blob to remain alive for the duration of the commit;
  // destroying it before the commit returns leaves the atomic request
  // referencing an invalid id and the kernel rejects with EINVAL.
  CDRMPropertyBlob modeBlob;

  if (m_old_crtc != nullptr)
  {
    if (m_old_crtc->GetId() != m_crtc->GetId())
    {
      AddProperty(m_old_crtc, "ACTIVE", 0);
      AddProperty(m_old_crtc, "MODE_ID", 0);
      flags |= DRM_MODE_ATOMIC_ALLOW_MODESET;
    }

    for (const auto& plane : m_planes)
    {
      if (m_gui_plane != nullptr && m_gui_plane->GetId() == plane->GetId())
        continue;
      if (m_video_plane != nullptr && m_video_plane->GetId() == plane->GetId())
        continue;

      uint64_t planeid = plane->GetPropertyValue("CRTC_ID").value_or(0);
      if (planeid == m_crtc->GetId() || planeid == m_old_crtc->GetId())
      {
        AddProperty(plane.get(), "CRTC_ID", 0);
        AddProperty(plane.get(), "FB_ID", 0);
      }

      // below disables the planes which are not in our crtcs, in other words
      // crts attached to other connectors (ie: 2nd monitor), amdgpu requires at least
      // one primary plane to enable crtcs, if we disable rest of the planes in amdgpu
      // atomic commit will fail
      if (!(HasQuirk(QUIRK_NEEDSPRIMARY)))
      {
        AddProperty(plane.get(), "CRTC_ID", 0);
        AddProperty(plane.get(), "FB_ID", 0);
      }
    }
    m_old_crtc = nullptr;
  }

  if (flags & DRM_MODE_ATOMIC_ALLOW_MODESET)
  {
    if (!AddProperty(m_connector, "CRTC_ID", m_crtc->GetCrtcId()))
      return;

    modeBlob = CDRMPropertyBlob(m_fd, m_mode, sizeof(*m_mode));
    if (!modeBlob.IsValid())
      return;

    if (!AddProperty(m_crtc, "MODE_ID", modeBlob.Get()))
      return;

    if (!AddProperty(m_crtc, "ACTIVE", m_active ? 1 : 0))
      return;
  }

  // In Direct-To-Plane (dual plane) mode m_gui_plane is the output
  // (gui overlay on top of m_video_plane). In single-plane flip-flop mode
  // FindVideoPlane has made m_video_plane the output and nulled m_gui_plane.
  // Pick whichever is live.
  CDRMPlane* outputPlane = m_gui_plane ? m_gui_plane : m_video_plane;

  if (rendered)
  {
    AddProperty(outputPlane, "FB_ID", fb_id);
    AddProperty(outputPlane, "CRTC_ID", m_crtc->GetCrtcId());
    AddProperty(outputPlane, "SRC_X", 0);
    AddProperty(outputPlane, "SRC_Y", 0);
    AddProperty(outputPlane, "SRC_W", m_width << 16);
    AddProperty(outputPlane, "SRC_H", m_height << 16);
    AddProperty(outputPlane, "CRTC_X", 0);
    AddProperty(outputPlane, "CRTC_Y", 0);
    AddProperty(outputPlane, "CRTC_W", m_mode->hdisplay);
    AddProperty(outputPlane, "CRTC_H", m_mode->vdisplay);

    if (m_inFenceFd != -1)
    {
      AddProperty(m_crtc, "OUT_FENCE_PTR", reinterpret_cast<uint64_t>(&m_outFenceFd));
      AddProperty(outputPlane, "IN_FENCE_FD", m_inFenceFd);
    }
  }
  //! @todo Reaching out to the window manager and application player
  //! singletons from inside the DRM layer is a layering violation. The
  //! "should the GUI plane be attached this frame" decision is GUI/player
  //! policy and should be computed at the WinSystem caller and passed in
  //! as a parameter on FlipPage. Until that refactor lands, do the lookups
  //! in place to match the surrounding master code pattern.
  else if (m_gui_plane && m_video_plane &&
           !CServiceBroker::GetGUI()->GetWindowManager().HasVisibleControls() &&
           !CServiceBroker::GetAppComponents()
                .GetComponent<CApplicationPlayer>()
                ->HasVisibleOverlay() &&
           !HasQuirk(QUIRK_NEEDSPRIMARY))
  {
    AddProperty(m_gui_plane, "FB_ID", 0);
    AddProperty(m_gui_plane, "CRTC_ID", 0);
  }

  if (CServiceBroker::GetLogging().CanLogComponent(LOGWINDOWING))
    m_req->LogAtomicRequest();

  auto ret = drmModeAtomicCommit(m_fd, m_req->Get(), flags | DRM_MODE_ATOMIC_TEST_ONLY, nullptr);
  if (ret < 0)
  {
    CLog::LogF(LOGERROR,
               "test commit failed: ({}) - falling back to last successful atomic "
               "request",
               strerror(errno));

    auto oldRequest = m_atomicRequestQueue.front().get();
    CDRMAtomicRequest::LogAtomicDiff(m_req, oldRequest);
    m_req = oldRequest;

    // update the old atomic request with the new fb id to avoid tearing
    if (rendered)
      AddProperty(outputPlane, "FB_ID", fb_id);
  }

  ret = drmModeAtomicCommit(m_fd, m_req->Get(), flags, nullptr);
  if (ret < 0)
  {
    CLog::LogF(LOGERROR, "atomic commit failed: {}", strerror(errno));
    m_atomicRequestQueue.pop_back();
  }
  else
  {
    // Sync the property cache with values the kernel accepted.
    // This must happen after a successful commit so that
    // GetPropertyValue() returns current state (e.g. CRTC_ID=0
    // after Disable()). Without this, stale cached values cause
    // incorrect plane cleanup on subsequent video playback.
    m_req->CacheProperties();

    if (m_atomicRequestQueue.size() > 1)
      m_atomicRequestQueue.pop_front();
  }

  if (m_inFenceFd != -1)
  {
    close(m_inFenceFd);
    m_inFenceFd = -1;
  }

  m_atomicRequestQueue.emplace_back(std::make_unique<CDRMAtomicRequest>());
  m_req = m_atomicRequestQueue.back().get();
}

void CDRMAtomic::FlipPage(struct gbm_bo* bo, bool rendered, bool videoLayer, bool async)
{
  struct drm_fb* drm_fb = nullptr;
  uint32_t flags = 0;

  if (rendered)
  {
    drm_fb = CDRMUtils::DrmFbGetFromBo(bo);
    if (!drm_fb)
    {
      CLog::LogF(LOGERROR, "Failed to get a new FBO");
      return;
    }

    if (async && !m_need_modeset)
      flags |= DRM_MODE_ATOMIC_NONBLOCK;
  }

  if (m_need_modeset)
  {
    flags |= DRM_MODE_ATOMIC_ALLOW_MODESET;
    m_need_modeset = false;
    CLog::LogF(LOGDEBUG, "Execute modeset at next commit");
  }

  DrmAtomicCommit(!drm_fb ? 0 : drm_fb->fb_id, flags, rendered, videoLayer);
}

bool CDRMAtomic::InitDrm()
{
  bool opened = CDRMUtils::OpenDrm(true);

  // At cold boot a connector's EDID/HPD probe may not have completed, so
  // OpenDrm(true) transiently fails with no connector even though an
  // atomic-capable display is attached. Retry a few times with a short delay,
  // but only when the hardware is atomic capable so a not-yet-ready display is
  // given time to appear. Genuinely non-atomic hardware skips the retry and
  // returns immediately, so a legacy fall-through is never delayed.
  if (!opened && SupportsAtomicModesetting())
  {
    constexpr int maxPasses = 3;
    constexpr auto retryDelay = std::chrono::milliseconds(500);

    for (int pass = 1; !opened && pass < maxPasses; ++pass)
    {
      CLog::LogF(LOGWARNING,
                 "atomic-capable device present but no connector ready, re-scanning ({}/{})", pass,
                 maxPasses);
      std::this_thread::sleep_for(retryDelay);
      opened = CDRMUtils::OpenDrm(true);
    }
  }

  if (!opened)
    return false;

  auto ret = drmSetClientCap(m_fd, DRM_CLIENT_CAP_ATOMIC, 1);
  if (ret)
  {
    CLog::LogF(LOGERROR, "no atomic modesetting support: {}", strerror(errno));
    return false;
  }

  m_atomicRequestQueue.emplace_back(std::make_unique<CDRMAtomicRequest>());
  m_req = m_atomicRequestQueue.back().get();

  if (!CDRMUtils::InitDrm())
    return false;

  CLog::LogF(LOGDEBUG, "initialized atomic DRM");

  return true;
}

void CDRMAtomic::DestroyDrm()
{
  CDRMUtils::DestroyDrm();
}

bool CDRMAtomic::SupportsAtomicModesetting()
{
  int numDevices = drmGetDevices2(0, nullptr, 0);
  if (numDevices <= 0)
    return false;

  std::vector<drmDevicePtr> devices(numDevices);
  if (drmGetDevices2(0, devices.data(), devices.size()) < 0)
    return false;

  bool atomic = false;
  for (const auto device : devices)
  {
    if (!(device->available_nodes & 1 << DRM_NODE_PRIMARY))
      continue;

    int fd = open(device->nodes[DRM_NODE_PRIMARY], O_RDWR | O_CLOEXEC);
    if (fd < 0)
      continue;

    // The atomic client cap is a device property gated on the driver's atomic
    // support, not on any connector, so testing it needs only the fd.
    if (drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1) == 0)
      atomic = true;

    close(fd);

    if (atomic)
      break;
  }

  drmFreeDevices(devices.data(), devices.size());
  return atomic;
}

bool CDRMAtomic::SetVideoMode(const RESOLUTION_INFO& res, struct gbm_bo* bo)
{
  m_need_modeset = true;

  return true;
}

bool CDRMAtomic::SetActive(bool active)
{
  m_need_modeset = true;
  m_active = active;

  return true;
}

bool CDRMAtomic::AddProperty(CDRMObject* object, const char* name, uint64_t value)
{
  if (!object)
    return false;
  return m_req->AddProperty(object, name, value);
}

CDRMAtomic::CDRMAtomicRequest::CDRMAtomicRequest() : m_atomicRequest(drmModeAtomicAlloc())
{
}

bool CDRMAtomic::CDRMAtomicRequest::AddProperty(CDRMObject* object,
                                                const char* name,
                                                uint64_t value)
{
  uint32_t propertyId = object->GetPropertyId(name);
  if (propertyId == 0)
    return false;

  int ret = drmModeAtomicAddProperty(m_atomicRequest.get(), object->GetId(), propertyId, value);
  if (ret < 0)
    return false;

  m_atomicRequestItems[object][propertyId] = value;
  return true;
}

void CDRMAtomic::CDRMAtomicRequest::CacheProperties()
{
  for (const auto& [object, properties] : m_atomicRequestItems)
  {
    for (const auto& [propertyId, value] : properties)
    {
      object->CachePropertyValue(propertyId, value);
    }
  }
}

void CDRMAtomic::CDRMAtomicRequest::LogAtomicDiff(CDRMAtomicRequest* current,
                                                  CDRMAtomicRequest* old)
{
  std::map<CDRMObject*, std::map<uint32_t, uint64_t>> atomicDiff;

  for (const auto& object : current->m_atomicRequestItems)
  {
    auto sameObject = old->m_atomicRequestItems.find(object.first);
    if (sameObject != old->m_atomicRequestItems.end())
    {
      std::map<uint32_t, uint64_t> propertyDiff;

      std::ranges::set_difference(current->m_atomicRequestItems[object.first],
                                  old->m_atomicRequestItems[object.first],
                                  std::inserter(propertyDiff, propertyDiff.begin()));

      atomicDiff[object.first] = propertyDiff;
    }
    else
    {
      atomicDiff[object.first] = current->m_atomicRequestItems[object.first];
    }
  }

  CLog::LogF(LOGDEBUG, "DRM Atomic Request Diff:");

  LogAtomicRequest(LOGERROR, atomicDiff);
}

void CDRMAtomic::CDRMAtomicRequest::LogAtomicRequest()
{
  CLog::LogF(LOGDEBUG, "DRM Atomic Request:");
  LogAtomicRequest(LOGDEBUG, m_atomicRequestItems);
}

void CDRMAtomic::CDRMAtomicRequest::LogAtomicRequest(
    uint8_t logLevel, std::map<CDRMObject*, std::map<uint32_t, uint64_t>>& atomicRequestItems)
{
  std::string message;
  for (const auto& object : atomicRequestItems)
  {
    message.append("\nObject: " + object.first->GetTypeName() +
                   "\tID: " + std::to_string(object.first->GetId()));
    for (const auto& property : object.second)
      message.append("\n  Property: " + object.first->GetPropertyName(property.first) +
                     "\tID: " + std::to_string(property.first) +
                     "\tValue: " + std::to_string(property.second));
  }

  CLog::LogF(logLevel, "{}", message);
}

void CDRMAtomic::CDRMAtomicRequest::DrmModeAtomicReqDeleter::operator()(drmModeAtomicReqPtr p) const
{
  drmModeAtomicFree(p);
};
