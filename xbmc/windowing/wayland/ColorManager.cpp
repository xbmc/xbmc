/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ColorManager.h"

#include "Connection.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "Registry.h"
#include "ServiceBroker.h"
#include "commons/ilog.h"
#include "utils/Map.h"
#include "utils/log.h"
#include "windowing/WinSystem.h"

#include <optional>

#ifdef HAS_WAYLAND_COLOR_MANAGEMENT

namespace
{
constexpr auto ffmpegToWaylandTFMap =
    make_map<AVColorTransferCharacteristic, wayland::color_manager_v1_transfer_function>(
        // clang-format off
        {
          //{AVCOL_TRC_BT709,        ???},
          //{AVCOL_TRC_UNSPECIFIED,  not needed},
          //{AVCOL_TRC_RESERVED,     not needed},
            {AVCOL_TRC_GAMMA22,      wayland::color_manager_v1_transfer_function::gamma22},
            {AVCOL_TRC_GAMMA28,      wayland::color_manager_v1_transfer_function::gamma28},
          //{AVCOL_TRC_SMPTE170M,    ???},
          //{AVCOL_TRC_SMPTE240M,    ???},
            {AVCOL_TRC_LINEAR,       wayland::color_manager_v1_transfer_function::ext_linear},
            {AVCOL_TRC_LOG,          wayland::color_manager_v1_transfer_function::log_100},
            {AVCOL_TRC_LOG_SQRT,     wayland::color_manager_v1_transfer_function::log_316},
            {AVCOL_TRC_IEC61966_2_4, wayland::color_manager_v1_transfer_function::xvycc},
          //{AVCOL_TRC_BT1361_ECG,   ???},
            {AVCOL_TRC_IEC61966_2_1, wayland::color_manager_v1_transfer_function::srgb},
            {AVCOL_TRC_BT2020_10,    wayland::color_manager_v1_transfer_function::bt1886},
            {AVCOL_TRC_BT2020_12,    wayland::color_manager_v1_transfer_function::bt1886},
            {AVCOL_TRC_SMPTE2084,    wayland::color_manager_v1_transfer_function::st2084_pq},
          //{AVCOL_TRC_SMPTEST2084,  alias of AVCOL_TRC_SMPTE2084},
            {AVCOL_TRC_SMPTE428,     wayland::color_manager_v1_transfer_function::st428},
          //{AVCOL_TRC_SMPTEST428_1, alias of AVCOL_TRC_SMPTE428},
            {AVCOL_TRC_ARIB_STD_B67, wayland::color_manager_v1_transfer_function::hlg},
          //{AVCOL_TRC_NB,           not needed}
        } // clang-format on
    );

constexpr auto ffmpegToWaylandPrimariesMap =
    make_map<AVColorPrimaries, wayland::color_manager_v1_primaries>({
        // clang-format off
        //{AVCOL_PRI_RESERVED0,    unused},
          {AVCOL_PRI_BT709,        wayland::color_manager_v1_primaries::srgb},
        //{AVCOL_PRI_UNSPECIFIED,  unused},
        //{AVCOL_PRI_RESERVED,     unused},
          {AVCOL_PRI_BT470M,       wayland::color_manager_v1_primaries::pal_m},
          {AVCOL_PRI_BT470BG,      wayland::color_manager_v1_primaries::pal},
          {AVCOL_PRI_SMPTE170M,    wayland::color_manager_v1_primaries::ntsc},
          {AVCOL_PRI_SMPTE240M,    wayland::color_manager_v1_primaries::ntsc},
          {AVCOL_PRI_FILM,         wayland::color_manager_v1_primaries::generic_film},
          {AVCOL_PRI_BT2020,       wayland::color_manager_v1_primaries::bt2020},
          {AVCOL_PRI_SMPTE428,     wayland::color_manager_v1_primaries::cie1931_xyz},
        //{AVCOL_PRI_SMPTEST428_1, alias of AVCOL_PRI_SMPTE428},
          {AVCOL_PRI_SMPTE431,     wayland::color_manager_v1_primaries::dci_p3},
          {AVCOL_PRI_SMPTE432,     wayland::color_manager_v1_primaries::display_p3},
        //{AVCOL_PRI_EBU3213,      ???},
        //{AVCOL_PRI_JEDEC_P22,    ???},
        //{AVCOL_PRI_NB,           unused},
        // clang-format on
    });

constexpr double COLOR_FACTOR = 1'000'000.0;
constexpr double MIN_LUM_FACTOR = 10'000.0;

} // namespace

#define CASE(VAL) \
  case VAL: \
    return std::string_view(#VAL)

template<>
struct fmt::formatter<wayland::color_manager_v1_render_intent> : fmt::formatter<std::string_view>
{
public:
  template<typename FormatContext>
  constexpr auto format(const wayland::color_manager_v1_render_intent& intent,
                        FormatContext& ctx) const
  {
    using namespace std::literals;

    const std::string_view sv = [&]
    {
      using enum wayland::color_manager_v1_render_intent;

      switch (intent)
      {
        CASE(perceptual);
        CASE(relative);
        CASE(saturation);
        CASE(absolute);
        CASE(relative_bpc);
        default:
          return "Unknown"sv;
      }
    }();

    return fmt::formatter<string_view>::format(sv, ctx);
  }
};

template<>
struct fmt::formatter<wayland::color_manager_v1_feature> : fmt::formatter<std::string_view>
{
public:
  template<typename FormatContext>
  constexpr auto format(const wayland::color_manager_v1_feature& feature, FormatContext& ctx) const
  {
    using namespace std::literals;

    const std::string_view sv = [&]
    {
      using enum wayland::color_manager_v1_feature;

      switch (feature)
      {
        CASE(icc_v2_v4);
        CASE(parametric);
        CASE(set_primaries);
        CASE(set_tf_power);
        CASE(set_luminances);
        CASE(set_mastering_display_primaries);
        CASE(extended_target_volume);
        CASE(windows_scrgb);
        default:
          return "Unknown"sv;
      }
    }();

    return fmt::formatter<string_view>::format(sv, ctx);
  }
};

template<>
struct fmt::formatter<wayland::color_manager_v1_transfer_function>
  : fmt::formatter<std::string_view>
{
public:
  template<typename FormatContext>
  constexpr auto format(const wayland::color_manager_v1_transfer_function& tf,
                        FormatContext& ctx) const
  {
    using namespace std::literals;

    const std::string_view sv = [&]
    {
      using enum wayland::color_manager_v1_transfer_function;

      switch (tf)
      {
        CASE(bt1886);
        CASE(gamma22);
        CASE(gamma28);
        CASE(st240);
        CASE(ext_linear);
        CASE(log_100);
        CASE(log_316);
        CASE(xvycc);
        CASE(srgb);
        CASE(ext_srgb);
        CASE(st2084_pq);
        CASE(st428);
        CASE(hlg);
        default:
          return "Unknown"sv;
      }
    }();

    return fmt::formatter<string_view>::format(sv, ctx);
  }
};

template<>
struct fmt::formatter<wayland::color_manager_v1_primaries> : fmt::formatter<std::string_view>
{
public:
  template<typename FormatContext>
  constexpr auto format(const wayland::color_manager_v1_primaries& primaries,
                        FormatContext& ctx) const
  {
    using namespace std::literals;

    const std::string_view sv = [&]
    {
      using enum wayland::color_manager_v1_primaries;

      switch (primaries)
      {
        CASE(srgb);
        CASE(pal_m);
        CASE(pal);
        CASE(ntsc);
        CASE(generic_film);
        CASE(bt2020);
        CASE(cie1931_xyz);
        CASE(dci_p3);
        CASE(display_p3);
        CASE(adobe_rgb);
        default:
          return "Unknown"sv;
      }
    }();

    return fmt::formatter<string_view>::format(sv, ctx);
  }
};

#undef CASE

using namespace KODI::WINDOWING::WAYLAND;

CColorManager::CColorManager(CConnection& connection)
{
  CRegistry registry{connection};
  registry.RequestSingleton(m_colorManager, 1, 1, false);
  registry.Bind();

  if (!m_colorManager)
  {
    CLog::Log(LOGINFO, "Color management protocol not supported");
    return;
  }

  m_colorManager.on_supported_intent() = [&](wayland::color_manager_v1_render_intent intent)
  {
    CLog::Log(LOGDEBUG, "Supported render intent: {}", intent);
    m_compositorIntents.SetSupported(intent);
  };
  m_colorManager.on_supported_feature() = [&](wayland::color_manager_v1_feature feature)
  {
    CLog::Log(LOGDEBUG, "Supported feature: {}", feature);
    m_compositorFeatures.SetSupported(feature);
  };
  m_colorManager.on_supported_tf_named() = [&](wayland::color_manager_v1_transfer_function tf)
  {
    CLog::Log(LOGDEBUG, "Supported transfer function: {}", tf);
    m_compositorTFs.SetSupported(tf);
  };
  m_colorManager.on_supported_primaries_named() = [&](wayland::color_manager_v1_primaries primaries)
  {
    CLog::Log(LOGDEBUG, "Supported primaries: {}", primaries);
    m_compositorPrimaries.SetSupported(primaries);
  };
}

CColorManager::~CColorManager()
{
  UnsetSurface();
}

void CColorManager::SetSurface(const wayland::surface_t& surface)
{
  UnsetSurface();
  if (m_colorManager)
    m_colorManagementSurface = m_colorManager.get_surface(surface);
}

void CColorManager::UnsetSurface()
{
  if (m_colorManagementSurface)
    m_colorManagementSurface.unset_image_description();

  m_colorManagementSurface = {};
}

bool CColorManager::SetHDR(const VideoPicture* videoPicture)
{
  if (!m_colorManager)
    return false;

  if (!m_colorManagementSurface)
  {
    CLog::Log(LOGERROR, "No surface set");
    return false;
  }

  m_colorManagementSurface.unset_image_description();

  if (!videoPicture)
    return false;

  if (!CServiceBroker::GetWinSystem()->IsHDRDisplaySettingEnabled())
  {
    CLog::Log(LOGINFO, "HDR is disabled in the settings");
    return false;
  }

  if (!m_compositorFeatures.IsSupported(wayland::color_manager_v1_feature::parametric))
    return false;

  wayland::image_description_creator_params_v1_t descriptionCreator =
      m_colorManager.create_parametric_creator();

  const auto transferFunction = ffmpegToWaylandTFMap.get(videoPicture->color_transfer);
  if (transferFunction)
  {
    if (m_compositorTFs.IsSupported(*transferFunction))
    {
      CLog::Log(LOGDEBUG, "Set transfer function {}", *transferFunction);
      descriptionCreator.set_tf_named(*transferFunction);
    }
    else
    {
      CLog::Log(LOGINFO, "Compositor doesn't support transfer function {}, HDR disabled",
                *transferFunction);
      return false;
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "Unknown transfer function {}", videoPicture->color_transfer);
    return false;
  }

  const auto primaries = ffmpegToWaylandPrimariesMap.get(videoPicture->color_primaries);
  if (primaries)
  {
    if (m_compositorPrimaries.IsSupported(*primaries))
    {
      CLog::Log(LOGDEBUG, "Set primaries {}", *primaries);
      descriptionCreator.set_primaries_named(*primaries);
    }
    else
    {
      CLog::Log(LOGINFO, "Compositor doesn't support primaries {}, HDR disabled", *primaries);
      return false;
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "Unknown primaries {}", videoPicture->color_primaries);
    return false;
  }

  SetDisplayMetadata(descriptionCreator, videoPicture);
  SetLightMetadata(descriptionCreator, videoPicture);

  m_colorManagementSurface.set_image_description(
      descriptionCreator.create(), wayland::color_manager_v1_render_intent::perceptual);

  return true;
}

bool CColorManager::IsHDRDisplay() const
{
  const CHDRCapabilities caps = GetDisplayHDRCapabilities();
  return caps.SupportsHDR10() || caps.SupportsHLG();
}

CHDRCapabilities CColorManager::GetDisplayHDRCapabilities() const
{
  CHDRCapabilities caps;
  if (m_compositorFeatures.IsSupported(wayland::color_manager_v1_feature::parametric) &&
      m_compositorPrimaries.IsSupported(wayland::color_manager_v1_primaries::bt2020))
  {
    if (m_compositorTFs.IsSupported(wayland::color_manager_v1_transfer_function::st2084_pq))
      caps.SetHDR10();

    if (m_compositorTFs.IsSupported(wayland::color_manager_v1_transfer_function::hlg))
      caps.SetHLG();
  }
  return caps;
}

void CColorManager::SetDisplayMetadata(
    wayland::image_description_creator_params_v1_t& descriptionCreator,
    const VideoPicture* videoPicture)
{
  if (videoPicture->hasDisplayMetadata &&
      m_compositorFeatures.IsSupported(
          wayland::color_manager_v1_feature::set_mastering_display_primaries))
  {
    if (videoPicture->displayMetadata.has_primaries)
    {
      // convert colors

      auto convertColor = [](AVRational r) { return std::round(av_q2d(r) * COLOR_FACTOR); };
      const auto& primaries = videoPicture->displayMetadata.display_primaries;

      uint32_t c[3][2] = {};
      for (int i = 0; i < 3; ++i)
      {
        c[i][0] = convertColor(primaries[i][0]);
        c[i][1] = convertColor(primaries[i][1]);
      }

      // convert white point

      const auto& whitePoint = videoPicture->displayMetadata.white_point;

      int32_t wp[2] = {};
      wp[0] = convertColor(whitePoint[0]);
      wp[1] = convertColor(whitePoint[1]);

      CLog::Log(LOGDEBUG,
                "Set mastering display primaries: red: ({}, {}), green: ({}, {}), blue: ({}, {}), "
                "white point: ({}, {})",
                c[0][0], c[0][1], c[1][0], c[1][1], c[2][0], c[2][1], wp[0], wp[1]);

      descriptionCreator.set_mastering_display_primaries(c[0][0], c[0][1], c[1][0], c[1][1],
                                                         c[2][0], c[2][1], wp[0], wp[1]);
    }

    if (videoPicture->displayMetadata.has_luminance)
    {
      const auto& metadata = videoPicture->displayMetadata;

      const uint32_t min = std::round(av_q2d(metadata.min_luminance) * MIN_LUM_FACTOR);
      const uint32_t max = std::round(av_q2d(metadata.max_luminance));

      CLog::Log(LOGDEBUG, "Set mastering luminance: min: {}, max: {})", min, max);

      descriptionCreator.set_mastering_luminance(min, max);
    }
  }
}

void CColorManager::SetLightMetadata(
    wayland::image_description_creator_params_v1_t& descriptionCreator,
    const VideoPicture* videoPicture)
{
  if (videoPicture->hasLightMetadata)
  {
    const auto& metadata = videoPicture->lightMetadata;

    CLog::Log(LOGDEBUG, "max_cll: {}, max_fall: {}", metadata.MaxCLL, metadata.MaxFALL);

    descriptionCreator.set_max_cll(metadata.MaxCLL);
    descriptionCreator.set_max_fall(metadata.MaxFALL);
  }
}

#else

using namespace KODI::WINDOWING::WAYLAND;

CColorManager::CColorManager(CConnection& /*connection*/)
{
}

CColorManager::~CColorManager() = default;

void CColorManager::SetSurface(const wayland::surface_t& /*surface*/)
{
}

void CColorManager::UnsetSurface()
{
}

bool CColorManager::SetHDR(const VideoPicture* /*videoPicture*/)
{
  return false;
}

bool CColorManager::IsHDRDisplay() const
{
  return false;
}

CHDRCapabilities CColorManager::GetDisplayHDRCapabilities() const
{
  return {};
}

#endif
