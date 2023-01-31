/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoBufferDMA.h"

#include "ServiceBroker.h"
#include "utils/BufferObject.h"
#include "utils/DRMHelpers.h"
#include "utils/log.h"

extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
}

CVideoBufferDMA::CVideoBufferDMA(IVideoBufferPool& pool, int id, uint32_t fourcc, uint64_t size)
  : CVideoBufferDRMPRIMEFFmpeg(pool, id),
    m_bo(CBufferObject::GetBufferObject(true)),
    m_fourcc(fourcc),
    m_size(size)
{
}

CVideoBufferDMA::~CVideoBufferDMA()
{
  Destroy();
}

AVDRMFrameDescriptor* CVideoBufferDMA::GetDescriptor() const
{
  return const_cast<AVDRMFrameDescriptor*>(&m_descriptor);
}

uint8_t* CVideoBufferDMA::GetMemPtr()
{
  return m_addr;
}

void CVideoBufferDMA::GetPlanes(uint8_t* (&planes)[YuvImage::MAX_PLANES])
{
  for (uint32_t i = 0; i < YuvImage::MAX_PLANES; i++)
    planes[i] = m_addr + m_offsets[i];
}

void CVideoBufferDMA::GetStrides(int (&strides)[YuvImage::MAX_PLANES])
{
  for (uint32_t i = 0; i < YuvImage::MAX_PLANES; i++)
    strides[i] = m_strides[i];
}

void CVideoBufferDMA::SetDimensions(int width, int height)
{
  SetDimensions(width, height, m_strides, m_offsets);
}

void CVideoBufferDMA::SetDimensions(int width,
                                    int height,
                                    const int (&strides)[YuvImage::MAX_PLANES])
{
  SetDimensions(width, height, strides, m_offsets);
}

void CVideoBufferDMA::SetDimensions(int width,
                                    int height,
                                    const int (&strides)[YuvImage::MAX_PLANES],
                                    const int (&planeOffsets)[YuvImage::MAX_PLANES])
{
  m_width = width;
  m_height = height;

  AVDRMFrameDescriptor* descriptor = &m_descriptor;
  descriptor->nb_objects = 1;
  descriptor->objects[0].fd = m_fd;
  descriptor->nb_layers = 1;

  AVDRMLayerDescriptor* layer = &descriptor->layers[0];
  layer->format = m_fourcc;
  layer->nb_planes = m_planes;

  for (uint32_t i = 0; i < m_planes; i++)
  {
    layer->planes[i].offset = planeOffsets[i];
    layer->planes[i].pitch = strides[i];
  }

  if (CServiceBroker::GetLogging().CanLogComponent(LOGVIDEO))
  {
    std::string planeStr;
    for (uint32_t plane = 0; plane < m_planes; plane++)
      planeStr.append(fmt::format("\nplane[{}]: stride={}\toffset={}", plane, strides[plane],
                                  planeOffsets[plane]));

    CLog::Log(LOGDEBUG, LOGVIDEO, "CVideoBufferDMA::{} - frame layout id={} fourcc={}{}",
              __FUNCTION__, m_id, DRMHELPERS::FourCCToString(m_fourcc), planeStr);
  }
}

bool CVideoBufferDMA::Alloc()
{
  if (!m_bo->CreateBufferObject(m_size))
    return false;

  m_fd = m_bo->GetFd();
  m_addr = m_bo->GetMemory();
  m_planes = 3; // CAddonVideoCodec only requests AV_PIX_FMT_YUV420P for now

  CLog::Log(LOGDEBUG, LOGVIDEO, "CVideoBufferDMA::{} - id={} fourcc={} fd={} size={} addr={}",
            __FUNCTION__, m_id, DRMHELPERS::FourCCToString(m_fourcc), m_fd, m_size,
            fmt::ptr(m_addr));

  return true;
}

void CVideoBufferDMA::Export(AVFrame* frame, uint32_t width, uint32_t height)
{
  AVPixelFormat pix_fmt = static_cast<AVPixelFormat>(frame->format);
  m_planes = av_pix_fmt_count_planes(pix_fmt);
  int h_shift;
  int v_shift;

  if (av_pix_fmt_get_chroma_sub_sample(pix_fmt, &h_shift, &v_shift))
    throw std::runtime_error("unable to determine chroma_sub_sample: " +
                             std::string(av_get_pix_fmt_name(pix_fmt)));

  if (m_planes < 2 || m_planes > 3)
    throw std::runtime_error("only 2 or 3 plane formats supported: " +
                             std::string(av_get_pix_fmt_name(pix_fmt)));

  for (uint32_t plane = 0; plane < m_planes; plane++)
  {
    m_strides[plane] =
        av_image_get_linesize(static_cast<AVPixelFormat>(frame->format), width, plane);
  }
  m_offsets[0] = 0;
  m_offsets[1] = m_strides[0] * height;
  m_offsets[2] = m_offsets[1] + (m_strides[1] * height >> v_shift);

  if (CServiceBroker::GetLogging().CanLogComponent(LOGVIDEO))
  {
    std::string planeStr;
    for (uint32_t plane = 0; plane < m_planes; plane++)
      planeStr.append(fmt::format("\nplane[{}]: stride={}\toffset={}", plane, m_strides[plane],
                                  m_offsets[plane]));

    CLog::Log(LOGDEBUG, LOGVIDEO, "CVideoBufferDMA::{} - frame layout id={} fourcc={}{}",
              __FUNCTION__, m_id, DRMHELPERS::FourCCToString(m_fourcc), planeStr);
  }

  for (uint32_t i = 0; i < AV_NUM_DATA_POINTERS; i++)
  {
    frame->data[i] = i < m_planes ? m_addr + m_offsets[i] : nullptr;
    frame->linesize[i] = i < m_planes ? m_strides[i] : 0;
    frame->buf[i] = i == 0 ? frame->opaque_ref : nullptr;
  }

  frame->extended_data = frame->data;
  frame->opaque_ref = nullptr;
}

void CVideoBufferDMA::SyncStart()
{
  m_bo->SyncStart();
}

void CVideoBufferDMA::SyncEnd()
{
  m_bo->SyncEnd();
}

void CVideoBufferDMA::Destroy()
{
  m_bo->ReleaseMemory();
  m_bo->DestroyBufferObject();

  for (auto& offset : m_offsets)
    offset = 0;

  for (auto& stride : m_strides)
    stride = 0;

  m_planes = 0;
  m_width = 0;
  m_height = 0;
  m_fourcc = 0;
  m_size = 0;
  m_addr = nullptr;
  m_fd = -1;
}
