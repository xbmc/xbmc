/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlHelper.h"
#include <mfobjects.h>

#include "rendering/dx/RenderContext.h"


static void pl_log_cb(void*, enum pl_log_level level, const char* msg)
{
  switch (level) {
  case PL_LOG_FATAL:
    CLog::Log(LOGFATAL, "libPlacebo Fatal: {}", msg);
    break;
  case PL_LOG_ERR:
    CLog::Log(LOGERROR, "libPlacebo Error: {}", msg);

    break;
  case PL_LOG_WARN:
    CLog::Log(LOGWARNING, "libPlacebo Warning: {}", msg);
    break;
  case PL_LOG_INFO:
    CLog::Log(LOGINFO, "libPlacebo Info: {}", msg);
    break;
  case PL_LOG_DEBUG:
    CLog::Log(LOGDEBUG, "libPlacebo Debug: {}", msg);
    break;
  case PL_LOG_NONE:
  case PL_LOG_TRACE:
    CLog::Log(LOGNONE, "libPlacebo Trace: {}", msg);
    break;
  }
}

std::shared_ptr<PL::PLInstance> PL::PLInstance::Get()
{
  static std::shared_ptr<PLInstance> sPLResources(new PLInstance);
  return sPLResources;
}

PL::PLInstance::PLInstance()
  : m_plD3d11(nullptr),
  m_plLog(nullptr),
  m_plRenderer(nullptr),
  m_plSwapchain(nullptr),
  CurrentPrim(0),
  CurrentMatrix(0),
  Currenttransfer(0)
{

}

PL::PLInstance::~PLInstance() = default;

bool PL::PLInstance::Init()
{
  pl_log_params log_param{};
  log_param.log_cb = pl_log_cb;
  log_param.log_level = PL_LOG_DEBUG;
  m_plLog = pl_log_create(PL_API_VER, &log_param);
  //d3d device
  pl_d3d11_params d3d_param{};
  d3d_param.device = DX::DeviceResources::Get()->GetD3DDevice();
  d3d_param.adapter = DX::DeviceResources::Get()->GetAdapter();
  d3d_param.adapter_luid = DX::DeviceResources::Get()->GetAdapterDesc().AdapterLuid;
  d3d_param.allow_software = true;
  d3d_param.force_software = false;
  d3d_param.no_compute = false;
  d3d_param.debug = false;
  //libplacebo dont touch it if 0
  d3d_param.max_frame_latency = 0;
  //this was added to libplacebo to handle multi threaded rendering for kodi

  
  m_plD3d11 = pl_d3d11_create(m_plLog, &d3d_param);
  if (!m_plD3d11)
    return false;
  //swap chain
  pl_d3d11_swapchain_params swapchain_param{};
  swapchain_param.swapchain = DX::DeviceResources::Get()->GetSwapChain();
  //everything else is not used
  m_plSwapchain = pl_d3d11_create_swapchain(m_plD3d11, &swapchain_param);
  if (!m_plSwapchain)
    return false;

  m_plRenderer = pl_renderer_create(m_plLog, m_plD3d11->gpu);
}
void PL::PLInstance::Reset()
{ 
  m_plSwapchain = nullptr;
  m_plLog = nullptr;
  m_plD3d11 = nullptr;
  m_plRenderer = nullptr;
}

void PL::PLInstance::LogCurrent()
{
  if (CurrentPrim == PL_COLOR_PRIM_COUNT)
    CurrentPrim = 0;
  if (CurrentMatrix == PL_COLOR_SYSTEM_COUNT)
    CurrentMatrix = 0;
  if (Currenttransfer == PL_COLOR_TRC_COUNT)
    Currenttransfer = 0;
  std::string sSys =  pl_color_system_name((pl_color_system)CurrentMatrix);
  std::string sTrans = pl_color_transfer_name((pl_color_transfer)Currenttransfer);
  std::string sPrim = pl_color_primaries_name((pl_color_primaries)CurrentPrim);
  CLog::Log(LOGINFO, "LibPlaceboCurrent Color Settings: Primaries: {}", sPrim.c_str());
  CLog::Log(LOGINFO, "LibPlaceboCurrent Color Settings: Transfer: {}", sTrans.c_str());
  CLog::Log(LOGINFO, "LibPlaceboCurrent Color Settings: Matrix: {}", sSys.c_str());
}

#if 0

pl_hdr_metadata CPlHelper::GetHdrMetadata(AVMasteringDisplayMetadata* mdm, AVContentLightMetadata* clm, AVDynamicHDRPlus* dhp)
{
  pl_hdr_metadata out = {};
  if (mdm) {
    if (mdm->has_luminance) {
      out.max_luma = av_q2d(mdm->max_luminance);
      out.min_luma = av_q2d(mdm->min_luminance);
      if (out.max_luma < 5.0 || out.min_luma >= out.max_luma)
        out.max_luma = out.min_luma = 0; /* sanity */
    }

    if (mdm->has_primaries)
    {
      out.prim = pl_raw_primaries{
          .red = { static_cast<float>(av_q2d(mdm->display_primaries[0][0])),
                     static_cast<float>(av_q2d(mdm->display_primaries[0][1])) },
          .green = { static_cast<float>(av_q2d(mdm->display_primaries[1][0])),
                     static_cast<float>(av_q2d(mdm->display_primaries[1][1])) },
          .blue = { static_cast<float>(av_q2d(mdm->display_primaries[2][0])),
                     static_cast<float>(av_q2d(mdm->display_primaries[2][1])) },
          .white = { static_cast<float>(av_q2d(mdm->white_point[0])),
                     static_cast<float>(av_q2d(mdm->white_point[1])) },
      };
    }


    if (clm) {
      out.max_cll = clm->MaxCLL;
      out.max_fall = clm->MaxFALL;
    }

    if (dhp && dhp->application_version < 2) {
      float hist_max = 0;
      const AVHDRPlusColorTransformParams* pars = &dhp->params[0];
      assert(dhp->num_windows > 0);
      out.scene_max[0] = 10000 * av_q2d(pars->maxscl[0]);
      out.scene_max[1] = 10000 * av_q2d(pars->maxscl[1]);
      out.scene_max[2] = 10000 * av_q2d(pars->maxscl[2]);
      out.scene_avg = 10000 * av_q2d(pars->average_maxrgb);

      // Calculate largest value from histogram to use as fallback for clips
      // with missing MaxSCL information. Note that this may end up picking
      // the "reserved" value at the 5% percentile, which in practice appears
      // to track the brightest pixel in the scene.
      for (int i = 0; i < pars->num_distribution_maxrgb_percentiles; i++) {
        float hist_val = av_q2d(pars->distribution_maxrgb[i].percentile);
        if (hist_val > hist_max)
          hist_max = hist_val;
      }
      hist_max *= 10000;
      if (!out.scene_max[0])
        out.scene_max[0] = hist_max;
      if (!out.scene_max[1])
        out.scene_max[1] = hist_max;
      if (!out.scene_max[2])
        out.scene_max[2] = hist_max;

      if (pars->tone_mapping_flag == 1) {
        out.ootf.target_luma = av_q2d(dhp->targeted_system_display_maximum_luminance);
        out.ootf.knee_x = av_q2d(pars->knee_point_x);
        out.ootf.knee_y = av_q2d(pars->knee_point_y);
        assert(pars->num_bezier_curve_anchors < 16);
        for (int i = 0; i < pars->num_bezier_curve_anchors; i++)
          out.ootf.anchors[i] = av_q2d(pars->bezier_curve_anchors[i]);
        out.ootf.num_anchors = pars->num_bezier_curve_anchors;
      }
    }
  }
  return out;
}

void pl_map_dovi_metadata(pl_dovi_metadata* out, const AVDOVIMetadata* data)
{
  const AVDOVIRpuDataHeader* header;
  const AVDOVIDataMapping* mapping;
  const AVDOVIColorMetadata* color;
  if (!data)
    return;

  header = av_dovi_get_header(data);
  mapping = av_dovi_get_mapping(data);
  color = av_dovi_get_color(data);

  for (int i = 0; i < 3; i++)
    out->nonlinear_offset[i] = av_q2d(color->ycc_to_rgb_offset[i]);
  for (int i = 0; i < 9; i++) {
    float* nonlinear = &out->nonlinear.m[0][0];
    float* linear = &out->linear.m[0][0];
    nonlinear[i] = av_q2d(color->ycc_to_rgb_matrix[i]);
    linear[i] = av_q2d(color->rgb_to_lms_matrix[i]);
  }
  for (int c = 0; c < 3; c++) {
    const AVDOVIReshapingCurve* csrc = &mapping->curves[c];
    pl_dovi_metadata::pl_reshape_data* cdst = &out->comp[c];
    cdst->num_pivots = csrc->num_pivots;
    for (int i = 0; i < csrc->num_pivots; i++) {
      const float scale = 1.0f / ((1 << header->bl_bit_depth) - 1);
      cdst->pivots[i] = scale * csrc->pivots[i];
    }
    for (int i = 0; i < csrc->num_pivots - 1; i++) {
      const float scale = 1.0f / (1 << header->coef_log2_denom);
      cdst->method[i] = csrc->mapping_idc[i];
      switch (csrc->mapping_idc[i]) {
      case AV_DOVI_MAPPING_POLYNOMIAL:
        for (int k = 0; k < 3; k++) {
          cdst->poly_coeffs[i][k] = (k <= csrc->poly_order[i])
            ? scale * csrc->poly_coef[i][k]
            : 0.0f;
        }
        break;
      case AV_DOVI_MAPPING_MMR:
        cdst->mmr_order[i] = csrc->mmr_order[i];
        cdst->mmr_constant[i] = scale * csrc->mmr_constant[i];
        for (int j = 0; j < csrc->mmr_order[i]; j++) {
          for (int k = 0; k < 7; k++)
            cdst->mmr_coeffs[i][j][k] = scale * csrc->mmr_coef[i][j][k];
        }
        break;
      }
    }
  }
}

void CPlHelper::avdovi_metadata(pl_color_space* color, pl_color_repr* repr, pl_dovi_metadata* dovi, const AVDOVIMetadata* metadata)
{
  const AVDOVIRpuDataHeader* header;
  const AVDOVIColorMetadata* dovi_color;
  const AVDOVIDmData* dovi_ext;

  if (!color || !repr || !dovi)
    return;

  header = av_dovi_get_header(metadata);
  dovi_color = av_dovi_get_color(metadata);
  if (header->disable_residual_flag) {
    pl_map_dovi_metadata(dovi, metadata);

    repr->dovi = dovi;
    repr->sys = PL_COLOR_SYSTEM_DOLBYVISION;
    color->primaries = PL_COLOR_PRIM_BT_2020;
    color->transfer = PL_COLOR_TRC_PQ;
    color->hdr.min_luma =
      pl_hdr_rescale(PL_HDR_PQ, PL_HDR_NITS, dovi_color->source_min_pq / 4095.0f);
    color->hdr.max_luma =
      pl_hdr_rescale(PL_HDR_PQ, PL_HDR_NITS, dovi_color->source_max_pq / 4095.0f);


    if ((dovi_ext = av_dovi_find_level(metadata, 1))) {
      color->hdr.max_pq_y = dovi_ext->l1.max_pq / 4095.0f;
      color->hdr.avg_pq_y = dovi_ext->l1.avg_pq / 4095.0f;
    }

  }
}


#endif