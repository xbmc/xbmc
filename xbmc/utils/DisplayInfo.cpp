/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DisplayInfo.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

extern "C"
{
#include <libdisplay-info/cta.h>
#include <libdisplay-info/edid.h>
#include <libdisplay-info/info.h>
}

using namespace KODI;
using namespace UTILS;

void CDisplayInfo::DiInfoDeleter::operator()(di_info* p)
{
  di_info_destroy(p);
}

std::unique_ptr<CDisplayInfo> CDisplayInfo::Create(const std::vector<uint8_t>& edid)
{
  auto info = std::unique_ptr<CDisplayInfo>(new CDisplayInfo(edid));
  if (!info->IsValid())
    return {};

  info->Parse();

  info->LogInfo();

  return info;
}

CDisplayInfo::CDisplayInfo(const std::vector<uint8_t>& edid)
{
  m_info.reset(di_info_parse_edid(edid.data(), edid.size()));
}

CDisplayInfo::~CDisplayInfo() = default;

bool CDisplayInfo::IsValid() const
{
  if (!m_info)
    return false;

  const char* error = di_info_get_failure_msg(m_info.get());
  if (error)
  {
    CLog::Log(LOGERROR, "[display-info] Error parsing EDID:");
    CLog::Log(LOGERROR, "[display-info] ----------------------------------------------");

    std::vector<std::string> lines = StringUtils::Split(error, "\n");

    for (const auto& line : lines)
    {
      CLog::Log(LOGERROR, "[display-info] {}", line);
    }

    CLog::Log(LOGERROR, "[display-info] ----------------------------------------------");
  }

  return true;
}

void CDisplayInfo::Parse()
{
  char* str = nullptr;

  str = di_info_get_make(m_info.get());
  m_make = str ? str : "unknown";
  free(str);

  str = di_info_get_model(m_info.get());
  m_model = str ? str : "unknown";
  free(str);

  const di_edid* edid = di_info_get_edid(m_info.get());

  const di_edid_ext* const* extensions = di_edid_get_extensions(edid);
  for (const di_edid_ext* extension = *extensions; extension != nullptr;
       extension = *(++extensions))
  {
    di_edid_ext_tag tag = di_edid_ext_get_tag(extension);

    switch (tag)
    {
      case DI_EDID_EXT_CEA:
      {
        const di_edid_cta* cta = di_edid_ext_get_cta(extension);

        const di_cta_data_block* const* blocks = di_edid_cta_get_data_blocks(cta);
        for (const di_cta_data_block* block = *blocks; block != nullptr; block = *(++blocks))
        {
          di_cta_data_block_tag cta_tag = di_cta_data_block_get_tag(block);

          switch (cta_tag)
          {
            case DI_CTA_DATA_BLOCK_COLORIMETRY:
            {
              m_colorimetry = di_cta_data_block_get_colorimetry(block);

              break;
            }
            case DI_CTA_DATA_BLOCK_HDR_STATIC_METADATA:
            {
              m_hdr_static_metadata = di_cta_data_block_get_hdr_static_metadata(block);

              break;
            }
            default:
              break;
          }
        }

        break;
      }
      default:
        break;
    }
  }
}

void CDisplayInfo::LogInfo() const
{
  CLog::Log(LOGINFO, "[display-info] make: '{}' model: '{}'", m_make, m_model);

  if (m_hdr_static_metadata)
  {
    CLog::Log(LOGINFO, "[display-info] supports hdr static metadata type1: {}",
              m_hdr_static_metadata->descriptors->type1);

    CLog::Log(LOGINFO, "[display-info] supported eotf:");
    CLog::Log(LOGINFO, "[display-info]   traditional sdr: {}",
              m_hdr_static_metadata->eotfs->traditional_sdr);
    CLog::Log(LOGINFO, "[display-info]   traditional hdr: {}",
              m_hdr_static_metadata->eotfs->traditional_hdr);
    CLog::Log(LOGINFO, "[display-info]   pq:              {}", m_hdr_static_metadata->eotfs->pq);
    CLog::Log(LOGINFO, "[display-info]   hlg:             {}", m_hdr_static_metadata->eotfs->hlg);

    CLog::Log(LOGINFO, "[display-info] luma min: '{}' avg: '{}' max: '{}'",
              m_hdr_static_metadata->desired_content_min_luminance,
              m_hdr_static_metadata->desired_content_max_frame_avg_luminance,
              m_hdr_static_metadata->desired_content_max_luminance);
  }

  if (m_colorimetry)
  {
    CLog::Log(LOGINFO, "[display-info] supported colorimetry:");
    CLog::Log(LOGINFO, "[display-info]   xvycc_601:   {}", m_colorimetry->xvycc_601);
    CLog::Log(LOGINFO, "[display-info]   xvycc_709:   {}", m_colorimetry->xvycc_709);
    CLog::Log(LOGINFO, "[display-info]   sycc_601:    {}", m_colorimetry->sycc_601);
    CLog::Log(LOGINFO, "[display-info]   opycc_601:   {}", m_colorimetry->opycc_601);
    CLog::Log(LOGINFO, "[display-info]   oprgb:       {}", m_colorimetry->oprgb);
    CLog::Log(LOGINFO, "[display-info]   bt2020_cycc: {}", m_colorimetry->bt2020_cycc);
    CLog::Log(LOGINFO, "[display-info]   bt2020_ycc:  {}", m_colorimetry->bt2020_ycc);
    CLog::Log(LOGINFO, "[display-info]   bt2020_rgb:  {}", m_colorimetry->bt2020_rgb);
    CLog::Log(LOGINFO, "[display-info]   st2113_rgb:  {}", m_colorimetry->st2113_rgb);
    CLog::Log(LOGINFO, "[display-info]   ictcp:       {}", m_colorimetry->ictcp);
  }
}

bool CDisplayInfo::SupportsHDRStaticMetadataType1() const
{
  if (!m_hdr_static_metadata)
    return false;

  return m_hdr_static_metadata->descriptors->type1;
}

bool CDisplayInfo::SupportsEOTF(Eotf eotf) const
{
  if (!m_hdr_static_metadata)
    return false;

  switch (eotf)
  {
    case Eotf::TRADITIONAL_SDR:
      return m_hdr_static_metadata->eotfs->traditional_sdr;
    case Eotf::TRADITIONAL_HDR:
      return m_hdr_static_metadata->eotfs->traditional_hdr;
    case Eotf::PQ:
      return m_hdr_static_metadata->eotfs->pq;
    case Eotf::HLG:
      return m_hdr_static_metadata->eotfs->hlg;
    default:
      return false;
  }
}

bool CDisplayInfo::SupportsColorimetry(Colorimetry colorimetry) const
{
  if (!m_colorimetry)
    return false;

  switch (colorimetry)
  {
    case Colorimetry::XVYCC_601:
      return m_colorimetry->xvycc_601;
    case Colorimetry::XVYCC_709:
      return m_colorimetry->xvycc_709;
    case Colorimetry::SYCC_601:
      return m_colorimetry->sycc_601;
    case Colorimetry::OPYCC_601:
      return m_colorimetry->opycc_601;
    case Colorimetry::OPRGB:
      return m_colorimetry->oprgb;
    case Colorimetry::BT2020_CYCC:
      return m_colorimetry->bt2020_cycc;
    case Colorimetry::BT2020_YCC:
      return m_colorimetry->bt2020_ycc;
    case Colorimetry::BT2020_RGB:
      return m_colorimetry->bt2020_rgb;
    case Colorimetry::ST2113_RGB:
      return m_colorimetry->st2113_rgb;
    case Colorimetry::ICTCP:
      return m_colorimetry->ictcp;
    default:
      return false;
  }
}
