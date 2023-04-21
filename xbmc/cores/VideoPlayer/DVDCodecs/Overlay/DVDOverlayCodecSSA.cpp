/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDOverlayCodecSSA.h"

#include "DVDCodecs/DVDCodecs.h"
#include "DVDOverlaySSA.h"
#include "DVDStreamInfo.h"
#include "ServiceBroker.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "settings/SettingsComponent.h"
#include "settings/SubtitlesSettings.h"
#include "utils/StringUtils.h"

#include <memory>

using namespace KODI;

CDVDOverlayCodecSSA::CDVDOverlayCodecSSA()
  : CDVDOverlayCodec("SSA Subtitle Decoder"),
    m_libass(std::make_shared<CDVDSubtitlesLibass>()),
    m_pOverlay(nullptr)
{
  m_order = 0;
  m_libass->Configure();
}

bool CDVDOverlayCodecSSA::Open(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  if (hints.codec != AV_CODEC_ID_SSA && hints.codec != AV_CODEC_ID_ASS)
    return false;

  m_pOverlay.reset();

  return m_libass->DecodeHeader(reinterpret_cast<char*>(hints.extradata.GetData()),
                                hints.extradata.GetSize());
}

OverlayMessage CDVDOverlayCodecSSA::Decode(DemuxPacket* pPacket)
{
  if (!pPacket)
    return OverlayMessage::OC_ERROR;

  double pts = pPacket->dts != DVD_NOPTS_VALUE ? pPacket->dts : pPacket->pts;
  // libass only has a precision of msec
  pts = round(pts / 1000) * 1000;

  uint8_t* data = pPacket->pData;
  int size = pPacket->iSize;
  double duration = pPacket->duration;
  if (duration == DVD_NOPTS_VALUE)
    duration = 0.0;

  if (strncmp((const char*)data, "Dialogue:", 9) == 0)
  {
    int sh, sm, ss, sc, eh, em, es, ec;
    double beg, end;
    size_t pos;
    std::string line, line2;
    std::vector<std::string> lines;
    StringUtils::Tokenize((const char*)data, lines, "\r\n");
    for (size_t i = 0; i < lines.size(); i++)
    {
      line = lines[i];
      StringUtils::Trim(line);
      std::unique_ptr<char[]> layer(new char[line.length() + 1]);

      if (sscanf(line.c_str(), "%*[^:]:%[^,],%d:%d:%d%*c%d,%d:%d:%d%*c%d", layer.get(), &sh, &sm,
                 &ss, &sc, &eh, &em, &es, &ec) != 9)
        continue;

      end = 10000 * ((eh * 360000.0) + (em * 6000.0) + (es * 100.0) + ec);
      beg = 10000 * ((sh * 360000.0) + (sm * 6000.0) + (ss * 100.0) + sc);

      pos = line.find_first_of(',', 0);
      pos = line.find_first_of(',', pos + 1);
      pos = line.find_first_of(',', pos + 1);
      if (pos == std::string::npos)
        continue;

      line2 = StringUtils::Format("{},{},{}", m_order++, layer.get(), line.substr(pos + 1));

      m_libass->DecodeDemuxPkt(line2.c_str(), static_cast<int>(line2.length()), beg, end - beg);
    }
  }
  else
  {
    m_libass->DecodeDemuxPkt((char*)data, size, pts, duration);
  }

  return m_pOverlay ? OverlayMessage::OC_DONE : OverlayMessage::OC_OVERLAY;
}

void CDVDOverlayCodecSSA::Reset()
{
  Flush();
}

void CDVDOverlayCodecSSA::Flush()
{
  m_pOverlay.reset();
  m_order = 0;
}

std::shared_ptr<CDVDOverlay> CDVDOverlayCodecSSA::GetOverlay()
{
  if (m_pOverlay)
    return nullptr;
  m_pOverlay = std::make_shared<CDVDOverlaySSA>(m_libass);
  m_pOverlay->iPTSStartTime = 0;
  m_pOverlay->iPTSStopTime = DVD_NOPTS_VALUE;
  auto overrideStyles{
      CServiceBroker::GetSettingsComponent()->GetSubtitlesSettings()->GetOverrideStyles()};
  m_pOverlay->SetForcedMargins(overrideStyles != SUBTITLES::OverrideStyles::STYLES_POSITIONS &&
                               overrideStyles != SUBTITLES::OverrideStyles::POSITIONS);
  return m_pOverlay;
}
