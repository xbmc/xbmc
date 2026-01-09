/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDDemuxCC.h"

#include "DVDDemuxCC/CCBitstreamParserFactory.h"
#include "DVDDemuxCC/CaptionBlock.h"
#include "DVDDemuxUtils.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/DVDCodecs/Overlay/contrib/cc_decoder708.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ColorUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>

namespace COLOR = KODI::UTILS::COLOR;

namespace
{
/*!
 * \brief Color formats for CC color conversion
 */
enum class ColorFormat
{
  RGB,
  ARGB
};

/*!
 * \brief Given the color code from the close caption decoder, returns the corresponding
 * color in rgb format (striping the alpha channel from argb)
 * 
 * \param[in] ccColor - a given color from the cc decoder
 * \param[in] format - the color format
 * \return the corresponding Color in rgb
 */
constexpr COLOR::Color CCColorConversion(const uint8_t ccColor, ColorFormat format)
{
  COLOR::Color color = COLOR::NONE;
  switch (ccColor)
  {
    case WHITE:
      color = COLOR::WHITE;
      break;
    case GREEN:
      color = COLOR::GREEN;
      break;
    case BLUE:
      color = COLOR::BLUE;
      break;
    case CYAN:
      color = COLOR::CYAN;
      break;
    case RED:
      color = COLOR::RED;
      break;
    case YELLOW:
      color = COLOR::YELLOW;
      break;
    case MAGENTA:
      color = COLOR::MAGENTA;
      break;
    case BLACK:
      color = COLOR::BLACK;
      break;
    default:
      break;
  }

  if (format == ColorFormat::RGB)
    return color & ~0xFF000000;

  return color;
}

/*!
 * \brief Given the current buffer cc text and cc style attributes, apply the modifiers
 * \param[in,out] ccText - the text to display in the caption
 * \param[in] ccAttributes - The attributes (italic, color, underline) for the text
 */
void ApplyStyleModifiers(std::string& ccText, const cc_attribute_t& ccAttributes)
{
  // Apply style modifiers to CC text
  if (ccAttributes.italic > 0)
  {
    ccText = StringUtils::Format("<i>{}</i>", ccText);
  }
  if (ccAttributes.underline > 0)
  {
    ccText = StringUtils::Format("<u>{}</u>", ccText);
  }
  if (ccAttributes.foreground != WHITE)
  {
    ccText = StringUtils::Format(
        "<font color=#{}>{}</u>",
        COLOR::ConvertToHexRGB(CCColorConversion(ccAttributes.foreground, ColorFormat::RGB)),
        ccText);
  }
}
} // namespace

bool reorder_sort (CCaptionBlock *lhs, CCaptionBlock *rhs)
{
  return (lhs->m_pts > rhs->m_pts);
}

CDVDDemuxCC::CDVDDemuxCC(AVCodecID codec, const uint8_t* extradata, int extrasize)
{
  m_hasData = false;
  m_curPts = 0.0;
  m_parser = CCBitstreamParserFactory::CreateParser(codec, extradata, extrasize);
}

CDVDDemuxCC::~CDVDDemuxCC()
{
  Dispose();
}

CDemuxStream* CDVDDemuxCC::GetStream(int iStreamId) const
{
  for (int i=0; i<GetNrOfStreams(); i++)
  {
    if (m_streams[i].uniqueId == iStreamId)
      return const_cast<CDemuxStreamSubtitle*>(&m_streams[i]);
  }
  return nullptr;
}

std::vector<CDemuxStream*> CDVDDemuxCC::GetStreams() const
{
  std::vector<CDemuxStream*> streams;

  int num = GetNrOfStreams();
  streams.reserve(num);
  for (int i = 0; i < num; ++i)
  {
    streams.push_back(const_cast<CDemuxStreamSubtitle*>(&m_streams[i]));
  }

  return streams;
}

int CDVDDemuxCC::GetNrOfStreams() const
{
  return m_streams.size();
}

DemuxPacket* CDVDDemuxCC::Read(DemuxPacket *pSrcPacket)
{
  DemuxPacket* pPacket = nullptr;

  if (!pSrcPacket)
  {
    pPacket = Decode();
    return pPacket;
  }

  if (pSrcPacket->pts == DVD_NOPTS_VALUE)
  {
    return pPacket;
  }

  // Move temp buffer to reorder buffer
  while (!m_ccTempBuffer.empty())
  {
    m_ccReorderBuffer.push_back(m_ccTempBuffer.back());
    m_ccTempBuffer.pop_back();
  }

  if (!m_parser)
  {
    CLog::Log(LOGERROR, "CDVDDemuxCC::Read - No parser available");
    return pPacket;
  }

  CCPictureType picType = m_parser->ParsePacket(pSrcPacket, m_ccTempBuffer, m_ccReorderBuffer);

  // Decode CC data when we encounter an I-frame or P-frame (reference frames).
  // Reference frames ensure correct temporal ordering of CC data,
  // as B-frames (CCPictureType::OTHER) may be reordered during decoding.
  if ((picType == CCPictureType::I_FRAME || picType == CCPictureType::P_FRAME) &&
      !m_ccReorderBuffer.empty())
  {
    if (!m_ccDecoder)
    {
      if (!OpenDecoder())
        return nullptr;
    }
    std::sort(m_ccReorderBuffer.begin(), m_ccReorderBuffer.end(), reorder_sort);
    pPacket = Decode();
  }

  return pPacket;
}

void CDVDDemuxCC::Handler(int service, void *userdata)
{
  CDVDDemuxCC *ctx = static_cast<CDVDDemuxCC*>(userdata);

  unsigned int idx;

  // switch back from 608 fallback if we got 708
  if (ctx->m_ccDecoder->m_seen608 && ctx->m_ccDecoder->m_seen708)
  {
    for (idx = 0; idx < ctx->m_streamdata.size(); idx++)
    {
      if (ctx->m_streamdata[idx].service == 0)
        break;
    }
    if (idx < ctx->m_streamdata.size())
    {
      ctx->m_streamdata.erase(ctx->m_streamdata.begin() + idx);
      ctx->m_ccDecoder->m_seen608 = false;
    }
    if (service == 0)
      return;
  }

  for (idx = 0; idx < ctx->m_streamdata.size(); idx++)
  {
    if (ctx->m_streamdata[idx].service == service)
      break;
  }
  if (idx >= ctx->m_streamdata.size())
  {
    CDemuxStreamSubtitle stream;
    stream.source = STREAM_SOURCE_VIDEOMUX;
    stream.name = g_localizeStrings.Get(39206); // Closed Caption "CC"
    stream.language = "und";

    auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
    if (settings->GetBool(CSettings::SETTING_SUBTITLES_CAPTIONSIMPAIRED))
      stream.flags = FLAG_HEARING_IMPAIRED;

    stream.codec = AV_CODEC_ID_TEXT;
    stream.uniqueId = service;
    ctx->m_streams.push_back(std::move(stream));

    streamdata data;
    data.streamIdx = idx;
    data.service = service;
    ctx->m_streamdata.push_back(data);

    if (service == 0)
      ctx->m_ccDecoder->m_seen608 = true;
    else
      ctx->m_ccDecoder->m_seen708 = true;
  }

  ctx->m_streamdata[idx].pts = ctx->m_curPts;
  ctx->m_streamdata[idx].hasData = true;
  ctx->m_hasData = true;
}

bool CDVDDemuxCC::OpenDecoder()
{
  m_ccDecoder = std::make_unique<CDecoderCC708>();
  m_ccDecoder->Init(Handler, this);
  return true;
}

void CDVDDemuxCC::Dispose()
{
  m_streams.clear();
  m_streamdata.clear();
  m_ccDecoder.reset();

  while (!m_ccReorderBuffer.empty())
  {
    delete m_ccReorderBuffer.back();
    m_ccReorderBuffer.pop_back();
  }
  while (!m_ccTempBuffer.empty())
  {
    delete m_ccTempBuffer.back();
    m_ccTempBuffer.pop_back();
  }
}

DemuxPacket* CDVDDemuxCC::Decode()
{
  DemuxPacket* pPacket = nullptr;

  while(!m_hasData && !m_ccReorderBuffer.empty())
  {
    CCaptionBlock *cc = m_ccReorderBuffer.back();
    m_ccReorderBuffer.pop_back();
    m_curPts = cc->m_pts;
    m_ccDecoder->Decode(cc->m_data.data(), static_cast<int>(cc->m_data.size()));
    delete cc;
  }

  if (m_hasData)
  {
    for (unsigned int i=0; i<m_streamdata.size(); i++)
    {
      if (m_streamdata[i].hasData)
      {
        int service = m_streamdata[i].service;

        std::string data;
        // CEA-608
        if (service == 0)
        {
          data = m_ccDecoder->m_cc608decoder->text;
          ApplyStyleModifiers(data, m_ccDecoder->m_cc608decoder->textattr);
        }
        // CEA-708
        else
        {
          data = m_ccDecoder->m_cc708decoders[service].text;
        }

        pPacket = CDVDDemuxUtils::AllocateDemuxPacket(data.size());
        pPacket->iSize = data.size();
        if (pPacket->iSize)
          memcpy(pPacket->pData, data.c_str(), pPacket->iSize);

        pPacket->iStreamId = service;
        pPacket->pts = m_streamdata[i].pts;
        pPacket->duration = 0;
        m_streamdata[i].hasData = false;
        break;
      }
      m_hasData = false;
    }
  }
  return pPacket;
}
