/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMUtils.h"

#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/DRMHelpers.h"
#include "utils/StringUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include "PlatformDefs.h"

using namespace KODI::WINDOWING::GBM;

namespace
{
const std::string SETTING_VIDEOSCREEN_LIMITGUISIZE = "videoscreen.limitguisize";

void DrmFbDestroyCallback(gbm_bo* bo, void* data)
{
  drm_fb* fb = static_cast<drm_fb*>(data);

  if (fb->fb_id > 0)
  {
    CLog::LogF(LOGDEBUG, "Removing framebuffer: {}", fb->fb_id);
    int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
    drmModeRmFB(drm_fd, fb->fb_id);
  }

  delete fb;
}
} // namespace

CDRMUtils::~CDRMUtils()
{
  DestroyDrm();
}

bool CDRMUtils::SetMode(const RESOLUTION_INFO& res)
{
  if (!m_connector->CheckConnector())
    return false;

  m_mode = m_connector->GetModeForIndex(std::atoi(res.strId.c_str()));
  m_width = res.iWidth;
  m_height = res.iHeight;

  CLog::LogF(LOGDEBUG, "Found crtc mode: {}x{}{} @ {} Hz", m_mode->hdisplay, m_mode->vdisplay,
             m_mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "", m_mode->vrefresh);

  return true;
}

drm_fb* CDRMUtils::DrmFbGetFromBo(struct gbm_bo* bo)
{
  {
    struct drm_fb* fb = static_cast<drm_fb*>(gbm_bo_get_user_data(bo));
    if (fb)
      return fb;
  }

  struct drm_fb* fb = new drm_fb;
  fb->bo = bo;
  fb->format = gbm_bo_get_format(bo);

  uint32_t width, height, handles[4] = {0}, strides[4] = {0}, offsets[4] = {0};

  uint64_t modifiers[4] = {0};

  width = gbm_bo_get_width(bo);
  height = gbm_bo_get_height(bo);

#if defined(HAS_GBM_MODIFIERS)
  for (int i = 0; i < gbm_bo_get_plane_count(bo); i++)
  {
    handles[i] = gbm_bo_get_handle_for_plane(bo, i).u32;
    strides[i] = gbm_bo_get_stride_for_plane(bo, i);
    offsets[i] = gbm_bo_get_offset(bo, i);
    modifiers[i] = gbm_bo_get_modifier(bo);
  }
#else
  handles[0] = gbm_bo_get_handle(bo).u32;
  strides[0] = gbm_bo_get_stride(bo);
  memset(offsets, 0, 16);
#endif

  uint32_t flags = 0;

  if (modifiers[0] && modifiers[0] != DRM_FORMAT_MOD_INVALID)
  {
    flags |= DRM_MODE_FB_MODIFIERS;
    CLog::LogF(LOGDEBUG, "Using modifier: {}", DRMHELPERS::ModifierToString(modifiers[0]));
  }

  int ret = drmModeAddFB2WithModifiers(m_fd, width, height, fb->format, handles, strides, offsets,
                                       modifiers, &fb->fb_id, flags);

  if (ret < 0)
  {
    ret = drmModeAddFB2(m_fd, width, height, fb->format, handles, strides, offsets, &fb->fb_id,
                        flags);

    if (ret < 0)
    {
      delete (fb);
      CLog::LogF(LOGDEBUG, "Failed to add framebuffer: {} ({})", strerror(errno), errno);
      return nullptr;
    }
  }

  gbm_bo_set_user_data(bo, fb, DrmFbDestroyCallback);

  return fb;
}

bool CDRMUtils::FindPreferredMode()
{
  if (m_mode)
    return true;

  for (int i = 0, area = 0; i < m_connector->GetModesCount(); i++)
  {
    drmModeModeInfo* current_mode = m_connector->GetModeForIndex(i);

    if (current_mode->type & DRM_MODE_TYPE_PREFERRED)
    {
      m_mode = current_mode;
      CLog::LogF(LOGDEBUG, "Found preferred mode: {}x{}{} @ {} Hz", m_mode->hdisplay,
                 m_mode->vdisplay, m_mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "",
                 m_mode->vrefresh);
      break;
    }

    auto current_area = current_mode->hdisplay * current_mode->vdisplay;
    if (current_area > area)
    {
      m_mode = current_mode;
      area = current_area;
    }
  }

  if (!m_mode)
  {
    CLog::LogF(LOGDEBUG, "Failed to find preferred mode");
    return false;
  }

  return true;
}

bool CDRMUtils::FindGuiPlane(uint32_t format, uint64_t modifier)
{
  const auto gui_format = std::ranges::find(m_gui_formats, format, &guiformat::drm);

  if (gui_format == m_gui_formats.end())
  {
    CLog::LogF(LOGERROR, "CDRMUtils::{} - Requested format {} for gui plane is not supported",
               __FUNCTION__, DRMHELPERS::FourCCToString(format));
    return false;
  }

  const PlaneType gui_type = HasQuirk(QUIRK_NEEDSPRIMARY) ? PLANE_TYPE_PRIMARY : PLANE_TYPE_ANY;
  const RESOLUTION_INFO res = GetCurrentMode();

  for (auto& crtc : m_encoder->GetPossibleCrtcs(m_crtcs, m_crtc))
  {
    for (auto& plane : m_planes)
    {
      if (!plane->Check(res.iWidth, res.iHeight, format, modifier, crtc, gui_type))
        continue;

      plane->SetFormat(format);
      plane->SetModifier(modifier);
      m_old_crtc = m_crtc;
      m_crtc = crtc;
      m_gui_plane = plane.get();
      CLog::LogF(
          LOGINFO,
          "CDRMUtils::{} - Using gui plane [{}x{}] id:{}, format:{}, modifier:{}, crtc id:{}",
          __FUNCTION__, res.iWidth, res.iHeight, m_gui_plane->GetId(),
          DRMHELPERS::FourCCToString(format), DRMHELPERS::ModifierToString(modifier),
          m_crtc->GetId());
      return true;
    }
  }

  CLog::LogF(LOGERROR,
             "CDRMUtils::{} - Requested format {} and modifier {} for gui plane is not supported",
             __FUNCTION__, DRMHELPERS::FourCCToString(format),
             DRMHELPERS::ModifierToString(modifier));

  return false;
}

bool CDRMUtils::FindVideoAndGuiPlane(uint32_t format,
                                     uint64_t modifier,
                                     uint64_t width,
                                     uint64_t height)
{
  if (m_gui_plane == nullptr)
    return false;

  for (auto& gui_format : m_gui_formats)
  {
    if (gui_format.drm != m_gui_plane->GetFormat())
      continue;
    if (!gui_format.alpha)
    {
      CLog::LogF(LOGWARNING,
                 "CDRMUtils::{} - GUI plane format {} can not do alpha blending, "
                 "video will be rendered through EGL import over the gui plane",
                 __FUNCTION__, DRMHELPERS::FourCCToString(m_gui_plane->GetFormat()));
      m_video_plane = nullptr;
      return false;
    }
    break;
  }

  const PlaneType gui_type = HasQuirk(QUIRK_NEEDSPRIMARY) ? PLANE_TYPE_PRIMARY : PLANE_TYPE_ANY;
  const RESOLUTION_INFO res = GetCurrentMode();

  // check if current config already satisfies
  if (m_video_plane != nullptr &&
      m_video_plane->Check(width, height, format, modifier, m_crtc, PLANE_TYPE_ANY))
    return true;

  for (auto& crtc : m_encoder->GetPossibleCrtcs(m_crtcs, m_crtc))
  {
    for (auto& gui_plane : m_planes)
    {
      if (!gui_plane->Check(res.iWidth, res.iHeight, m_gui_plane->GetFormat(),
                            m_gui_plane->GetModifier(), crtc, gui_type))
        continue;

      for (auto& video_plane : m_planes)
      {
        if (video_plane->GetId() == gui_plane->GetId() ||
            !video_plane->Check(width, height, format, modifier, crtc, PLANE_TYPE_ANY) ||
            !gui_plane->MoveOnTopOf(video_plane.get()))
          continue;

        gui_plane->SetFormat(m_gui_plane->GetFormat());
        gui_plane->SetModifier(m_gui_plane->GetModifier());
        video_plane->SetFormat(format);
        video_plane->SetModifier(modifier);

        m_old_crtc = m_crtc;
        m_crtc = crtc;

        m_gui_plane = gui_plane.get();
        m_video_plane = video_plane.get();

        CLog::LogF(LOGINFO,
                   "CDRMUtils::{} - Using gui plane [{}x{}] id:{}, video plane [{}x{}] id:{} on "
                   "crtc id:{} for video format:{}, video modifier:{}",
                   __FUNCTION__, res.iWidth, res.iHeight, m_gui_plane->GetId(), width, height,
                   m_video_plane->GetId(), m_crtc->GetId(), DRMHELPERS::FourCCToString(format),
                   DRMHELPERS::ModifierToString(modifier));
        return true;
      }
    }
  }

  CLog::LogF(LOGWARNING,
             "CDRMUtils::{} - Rendering will be done through EGL. "
             "Can not find a Video Plane [{}x{}] format:{}, modifier:{} "
             "together with a Gui Plane [{}x{}] format:{}, modifier:{}.",
             __FUNCTION__, res.iWidth, res.iHeight, DRMHELPERS::FourCCToString(format),
             DRMHELPERS::ModifierToString(modifier), width, height,
             DRMHELPERS::FourCCToString(m_gui_plane->GetFormat()),
             DRMHELPERS::ModifierToString(m_gui_plane->GetModifier()));
  return false;
}

void CDRMUtils::PrintDrmDeviceInfo(drmDevicePtr device)
{
  std::string message;

  // clang-format off
  message.append(fmt::format("DRM Device Info:"));
  message.append(fmt::format("\n  available_nodes: {:#04x}", device->available_nodes));
  message.append("\n  nodes:");

  for (int i = 0; i < DRM_NODE_MAX; i++)
  {
    if (device->available_nodes & 1 << i)
      message.append(fmt::format("\n    nodes[{}]: {}", i, device->nodes[i]));
  }

  message.append(fmt::format("\n  bustype: {:#04x}", device->bustype));

  if (device->bustype == DRM_BUS_PCI)
  {
    message.append("\n    pci:");
    message.append(fmt::format("\n      domain: {:#04x}", device->businfo.pci->domain));
    message.append(fmt::format("\n      bus:    {:#02x}", device->businfo.pci->bus));
    message.append(fmt::format("\n      dev:    {:#02x}", device->businfo.pci->dev));
    message.append(fmt::format("\n      func:   {:#1}", device->businfo.pci->func));

    message.append("\n  deviceinfo:");
    message.append("\n    pci:");
    message.append(fmt::format("\n      vendor_id:    {:#04x}", device->deviceinfo.pci->vendor_id));
    message.append(fmt::format("\n      device_id:    {:#04x}", device->deviceinfo.pci->device_id));
    message.append(fmt::format("\n      subvendor_id: {:#04x}", device->deviceinfo.pci->subvendor_id));
    message.append(fmt::format("\n      subdevice_id: {:#04x}", device->deviceinfo.pci->subdevice_id));
  }
  else if (device->bustype == DRM_BUS_USB)
  {
    message.append("\n    usb:");
    message.append(fmt::format("\n      bus: {:#03x}", device->businfo.usb->bus));
    message.append(fmt::format("\n      dev: {:#03x}", device->businfo.usb->dev));

    message.append("\n  deviceinfo:");
    message.append("\n    usb:");
    message.append(fmt::format("\n      vendor:  {:#04x}", device->deviceinfo.usb->vendor));
    message.append(fmt::format("\n      product: {:#04x}", device->deviceinfo.usb->product));
  }
  else if (device->bustype == DRM_BUS_PLATFORM)
  {
    message.append("\n    platform:");
    message.append(fmt::format("\n      fullname: {}", device->businfo.platform->fullname));
  }
  else if (device->bustype == DRM_BUS_HOST1X)
  {
    message.append("\n    host1x:");
    message.append(fmt::format("\n      fullname: {}", device->businfo.host1x->fullname));
  }
  else
    message.append("\n    unhandled bus type");
  // clang-format on

  CLog::LogF(LOGDEBUG, "{}", message);
}

bool CDRMUtils::OpenDrm(bool needConnector)
{
  int numDevices = drmGetDevices2(0, nullptr, 0);
  if (numDevices <= 0)
  {
    CLog::LogF(LOGERROR, "No drm devices found: ({})", strerror(errno));
    return false;
  }

  CLog::LogF(LOGDEBUG, "drm devices found: {}", numDevices);

  std::vector<drmDevicePtr> devices(numDevices);

  int ret = drmGetDevices2(0, devices.data(), devices.size());
  if (ret < 0)
  {
    CLog::LogF(LOGERROR, "drmGetDevices2 return an error: ({})", strerror(errno));
    return false;
  }

  for (const auto device : devices)
  {
    if (!(device->available_nodes & 1 << DRM_NODE_PRIMARY))
      continue;

    close(m_fd);
    m_fd = open(device->nodes[DRM_NODE_PRIMARY], O_RDWR | O_CLOEXEC);
    if (m_fd < 0)
      continue;

    if (needConnector)
    {
      auto resources = drmModeGetResources(m_fd);
      if (!resources)
        continue;

      m_connectors.clear();
      for (int i = 0; i < resources->count_connectors; i++)
        m_connectors.emplace_back(std::make_unique<CDRMConnector>(m_fd, resources->connectors[i]));

      drmModeFreeResources(resources);

      if (!FindConnector())
        continue;
    }

    CLog::LogF(LOGDEBUG, "Opened device: {}", device->nodes[DRM_NODE_PRIMARY]);

    drmVersionPtr version = drmGetVersion(m_fd);
    if (version)
    {
      if (strcmp(version->name, "amdgpu") == 0)
        m_drm_quirks |= QUIRK_NEEDSPRIMARY;
      drmFreeVersion(version);
    }

    PrintDrmDeviceInfo(device);

    const char* renderPath = drmGetRenderDeviceNameFromFd(m_fd);

    if (!renderPath)
      renderPath = drmGetDeviceNameFromFd2(m_fd);

    if (!renderPath)
      renderPath = drmGetDeviceNameFromFd(m_fd);

    if (renderPath)
    {
      m_renderDevicePath = renderPath;
      m_renderFd = open(renderPath, O_RDWR | O_CLOEXEC);
      if (m_renderFd != 0)
        CLog::LogF(LOGDEBUG, "Opened render node: {}", renderPath);
    }

    drmFreeDevices(devices.data(), devices.size());
    return true;
  }

  drmFreeDevices(devices.data(), devices.size());
  return false;
}

bool CDRMUtils::InitDrm()
{
  if (m_fd < 0)
    return false;

  /* caps need to be set before allocating connectors, encoders, crtcs, and planes */
  int ret = drmSetClientCap(m_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
  if (ret)
  {
    CLog::LogF(LOGERROR, "Failed to set universal planes capability: {}", strerror(errno));
    return false;
  }

  ret = drmSetClientCap(m_fd, DRM_CLIENT_CAP_STEREO_3D, 1);
  if (ret)
  {
    CLog::LogF(LOGERROR, "Failed to set stereo 3d capability: {}", strerror(errno));
    return false;
  }

#if defined(DRM_CLIENT_CAP_ASPECT_RATIO)
  ret = drmSetClientCap(m_fd, DRM_CLIENT_CAP_ASPECT_RATIO, 0);
  if (ret != 0)
    CLog::LogF(LOGERROR, "Aspect ratio capability is not supported: {}", strerror(errno));
#endif

  auto resources = drmModeGetResources(m_fd);
  if (!resources)
  {
    CLog::LogF(LOGERROR, "Failed to get drm resources: {}", strerror(errno));
    return false;
  }

  m_connectors.clear();
  for (int i = 0; i < resources->count_connectors; i++)
    m_connectors.emplace_back(std::make_unique<CDRMConnector>(m_fd, resources->connectors[i]));

  m_encoders.clear();
  for (int i = 0; i < resources->count_encoders; i++)
    m_encoders.emplace_back(std::make_unique<CDRMEncoder>(m_fd, resources->encoders[i]));

  m_crtcs.clear();
  for (int i = 0; i < resources->count_crtcs; i++)
    m_crtcs.emplace_back(std::make_unique<CDRMCrtc>(m_fd, resources->crtcs[i], i));

  drmModeFreeResources(resources);

  auto planeResources = drmModeGetPlaneResources(m_fd);
  if (!planeResources)
  {
    CLog::LogF(LOGERROR, "Failed to get drm plane resources: {}", strerror(errno));
    return false;
  }

  m_planes.clear();
  for (uint32_t i = 0; i < planeResources->count_planes; i++)
  {
    m_planes.emplace_back(std::make_unique<CDRMPlane>(m_fd, planeResources->planes[i]));
    m_planes[i]->FindModifiers();
  }

  drmModeFreePlaneResources(planeResources);

  if (!FindConnector())
    return false;

  if (!FindEncoder())
    return false;

  if (!FindCrtc())
    return false;

  if (!FindPreferredMode())
    return false;

  ret = drmSetMaster(m_fd);
  if (ret < 0)
  {
    CLog::LogF(LOGDEBUG, "Failed to set drm master, will try to authorize instead: {}",
               strerror(errno));

    drm_magic_t magic;

    ret = drmGetMagic(m_fd, &magic);
    if (ret < 0)
    {
      CLog::LogF(LOGERROR, "Failed to get drm magic: {}", strerror(errno));
      return false;
    }

    ret = drmAuthMagic(m_fd, magic);
    if (ret < 0)
    {
      CLog::LogF(LOGERROR, "Failed to authorize drm magic: {}", strerror(errno));
      return false;
    }

    CLog::Log(LOGINFO, "CDRMUtils: Successfully authorized drm magic");
  }

  const PlaneType gui_type = HasQuirk(QUIRK_NEEDSPRIMARY) ? PLANE_TYPE_PRIMARY : PLANE_TYPE_ANY;
  const RESOLUTION_INFO res = GetCurrentMode();

  for (auto& plane : m_planes)
  {
    for (auto& format : m_gui_formats)
    {
      if (!plane->Check(res.iWidth, res.iHeight, format.drm, DRM_FORMAT_MOD_INVALID, nullptr,
                        gui_type))
        continue;

      if (!format.active)
        format.active = true;

      for (uint64_t modifier : plane->GetModifiersForFormat(format.drm))
      {
        auto it = std::ranges::find(format.modifiers, modifier);
        if (it == format.modifiers.end())
          format.modifiers.push_back(modifier);
      }
    }
  }
  return true;
}

bool CDRMUtils::FindConnector()
{
  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return false;

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return false;

  std::vector<std::unique_ptr<CDRMConnector>>::iterator connector;

  std::string connectorName = settings->GetString(CSettings::SETTING_VIDEOSCREEN_MONITOR);
  if (connectorName != "Default")
  {
    connector = std::find_if(m_connectors.begin(), m_connectors.end(),
                             [&connectorName](auto& connector)
                             {
                               return connector->GetEncoderId() > 0 && connector->IsConnected() &&
                                      connector->GetName() == connectorName;
                             });

    if (connector == m_connectors.end())
    {
      CLog::LogF(LOGDEBUG, "Failed to find specified connector: {}, trying default", connectorName);
      connectorName = "Default";
    }
  }

  if (connectorName == "Default")
  {
    connector = std::ranges::find_if(m_connectors, [](auto& conn)
                                     { return conn->GetEncoderId() > 0 && conn->IsConnected(); });
  }

  if (connector == m_connectors.end())
  {
    CLog::LogF(LOGDEBUG, "Failed to find connected connector");
    return false;
  }

  CLog::Log(LOGINFO, "CDRMUtils: Using connector: {}", connector->get()->GetName());

  m_connector = connector->get();
  return true;
}

bool CDRMUtils::FindEncoder()
{
  auto encoder = std::ranges::find_if(
      m_encoders, [this](auto& enc) { return enc->GetEncoderId() == m_connector->GetEncoderId(); });

  if (encoder == m_encoders.end())
  {
    CLog::LogF(LOGDEBUG, "Failed to find encoder for connector id: {}",
               *m_connector->GetConnectorId());
    return false;
  }

  CLog::Log(LOGINFO, "CDRMUtils: Using encoder: {}", encoder->get()->GetEncoderId());

  m_encoder = encoder->get();
  return true;
}

bool CDRMUtils::FindCrtc()
{
  for (auto& crtc : m_encoder->GetPossibleCrtcs(m_crtcs))
  {
    if (crtc->GetCrtcId() != m_encoder->GetCrtcId())
      continue;
    m_orig_crtc = crtc;
    m_crtc = m_orig_crtc;
    if (m_orig_crtc->GetModeValid())
    {
      m_mode = m_orig_crtc->GetMode();
      CLog::LogF(LOGDEBUG, "Original crtc id: {}, mode: {}x{}{} @ {} Hz", m_crtc->GetCrtcId(),
                 m_mode->hdisplay, m_mode->vdisplay,
                 m_mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "", m_mode->vrefresh);
    }
    return true;
  }

  return false;
}

bool CDRMUtils::RestoreOriginalMode()
{
  if (!m_orig_crtc)
  {
    return false;
  }

  auto ret = drmModeSetCrtc(m_fd, m_orig_crtc->GetCrtcId(), m_orig_crtc->GetBufferId(),
                            m_orig_crtc->GetX(), m_orig_crtc->GetY(), m_connector->GetConnectorId(),
                            1, m_orig_crtc->GetMode());

  if (ret)
  {
    CLog::LogF(LOGERROR, "Failed to set original crtc mode");
    return false;
  }

  CLog::LogF(LOGDEBUG, "Set original crtc mode");

  return true;
}

void CDRMUtils::DestroyDrm()
{
  RestoreOriginalMode();

  if (drmAuthMagic(m_fd, 0) == EINVAL)
    drmDropMaster(m_fd);

  close(m_renderFd);
  close(m_fd);

  m_connector = nullptr;
  m_encoder = nullptr;
  m_crtc = nullptr;
  m_orig_crtc = nullptr;
  m_video_plane = nullptr;
  m_gui_plane = nullptr;
}

RESOLUTION_INFO CDRMUtils::GetResolutionInfo(drmModeModeInfoPtr mode)
{
  RESOLUTION_INFO res;
  res.iScreenWidth = mode->hdisplay;
  res.iScreenHeight = mode->vdisplay;
  res.iWidth = res.iScreenWidth;
  res.iHeight = res.iScreenHeight;

  int limit = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      SETTING_VIDEOSCREEN_LIMITGUISIZE);
  if (limit > 0 && res.iScreenWidth > 1920 && res.iScreenHeight > 1080)
  {
    switch (limit)
    {
      case 1: // 720p
        res.iWidth = 1280;
        res.iHeight = 720;
        break;
      case 2: // 1080p / 720p (>30hz)
        res.iWidth = mode->vrefresh > 30 ? 1280 : 1920;
        res.iHeight = mode->vrefresh > 30 ? 720 : 1080;
        break;
      case 3: // 1080p
        res.iWidth = 1920;
        res.iHeight = 1080;
        break;
      case 4: // Unlimited / 1080p (>30hz)
        res.iWidth = mode->vrefresh > 30 ? 1920 : res.iScreenWidth;
        res.iHeight = mode->vrefresh > 30 ? 1080 : res.iScreenHeight;
        break;
    }
  }

  if (mode->clock % 5 != 0)
    res.fRefreshRate = static_cast<float>(mode->vrefresh) * (1000.0f / 1001.0f);
  else
    res.fRefreshRate = mode->vrefresh;
  res.iSubtitles = res.iHeight;
  res.fPixelRatio = 1.0f;
  res.bFullScreen = true;

  if (mode->flags & DRM_MODE_FLAG_3D_MASK)
  {
    if (mode->flags & DRM_MODE_FLAG_3D_TOP_AND_BOTTOM)
      res.dwFlags = D3DPRESENTFLAG_MODE3DTB;
    else if (mode->flags & DRM_MODE_FLAG_3D_SIDE_BY_SIDE_HALF)
      res.dwFlags = D3DPRESENTFLAG_MODE3DSBS;
  }
  else if (mode->flags & DRM_MODE_FLAG_INTERLACE)
    res.dwFlags = D3DPRESENTFLAG_INTERLACED;
  else
    res.dwFlags = D3DPRESENTFLAG_PROGRESSIVE;

  res.strMode =
      StringUtils::Format("{}x{}{} @ {:.6f} Hz", res.iScreenWidth, res.iScreenHeight,
                          res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "", res.fRefreshRate);
  return res;
}

RESOLUTION_INFO CDRMUtils::GetCurrentMode()
{
  return GetResolutionInfo(m_mode);
}

std::vector<RESOLUTION_INFO> CDRMUtils::GetModes()
{
  std::vector<RESOLUTION_INFO> resolutions;
  resolutions.reserve(m_connector->GetModesCount());

  for (auto i = 0; i < m_connector->GetModesCount(); i++)
  {
    RESOLUTION_INFO res = GetResolutionInfo(m_connector->GetModeForIndex(i));
    res.strId = std::to_string(i);
    resolutions.push_back(res);
  }

  return resolutions;
}

std::vector<std::string> CDRMUtils::GetConnectedConnectorNames()
{
  std::vector<std::string> connectorNames;
  for (const auto& connector : m_connectors)
  {
    if (connector->IsConnected())
      connectorNames.emplace_back(connector->GetName());
  }

  return connectorNames;
}
