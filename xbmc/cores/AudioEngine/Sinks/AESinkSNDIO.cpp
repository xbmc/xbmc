/* -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 *  Copyright (C) 2016-2017 Tobias Kortkamp <t@tobik.me>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AESinkSNDIO.h"

#include "cores/AudioEngine/AESinkFactory.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/log.h"

#include <sys/param.h>

#ifndef nitems
#define nitems(x) (sizeof((x))/sizeof((x)[0]))
#endif

static enum AEChannel channelMap[] =
{
  AE_CH_FL,
  AE_CH_FR,
  AE_CH_BL,
  AE_CH_BR,
  AE_CH_FC,
  AE_CH_LFE,
  AE_CH_SL,
  AE_CH_SR,
};

struct sndio_formats
{
  AEDataFormat fmt;
  unsigned int bits;
  unsigned int bps;
  unsigned int sig;
  unsigned int le;
  unsigned int msb;
};

static struct sndio_formats formats[] =
{
  { AE_FMT_S32NE, 32, 4, 1, SIO_LE_NATIVE, 1 },
  { AE_FMT_S32LE, 32, 4, 1, 1, 1 },
  { AE_FMT_S32BE, 32, 4, 1, 0, 1 },

  { AE_FMT_S24NE4, 24, 4, 1, SIO_LE_NATIVE, 0 },
  { AE_FMT_S24NE4, 24, 4, 1, SIO_LE_NATIVE, 1 },
  { AE_FMT_S24NE3, 24, 3, 1, SIO_LE_NATIVE, 0 },
  { AE_FMT_S24NE3, 24, 3, 1, SIO_LE_NATIVE, 1 },

  { AE_FMT_S16NE, 16, 2, 1, SIO_LE_NATIVE, 1 },
  { AE_FMT_S16NE, 16, 2, 1, SIO_LE_NATIVE, 0 },
  { AE_FMT_S16LE, 16, 2, 1, 1, 1 },
  { AE_FMT_S16LE, 16, 2, 1, 1, 0 },
  { AE_FMT_S16BE, 16, 2, 1, 0, 1 },
  { AE_FMT_S16BE, 16, 2, 1, 0, 0 },

  { AE_FMT_U8, 8, 1, 0, 0, 0 },
  { AE_FMT_U8, 8, 1, 0, 0, 1 },
  { AE_FMT_U8, 8, 1, 0, 1, 0 },
  { AE_FMT_U8, 8, 1, 0, 1, 1 },
};

static AEDataFormat lookupDataFormat(unsigned int bits, unsigned int bps,
                                     unsigned int sig, unsigned int le, unsigned int msb)
{
  for (const sndio_formats& format : formats)
  {
    if (bits == format.bits &&
        bps == format.bps &&
        sig == format.sig &&
        le == format.le &&
        msb == format.msb)
    {
      return format.fmt;
    }
  }
  return AE_FMT_INVALID;
}

void CAESinkSNDIO::AudioFormatToPar(AEAudioFormat& format)
{
  sio_initpar(&m_par);

  m_par.rate = format.m_sampleRate;
  m_par.xrun = SIO_IGNORE;
  m_par.pchan = format.m_channelLayout.Count();

  for (const sndio_formats& f : formats)
  {
    if (f.fmt == format.m_dataFormat)
    {
      m_par.bits = f.bits;
      m_par.sig = f.sig;
      m_par.le = f.le;
      m_par.msb = f.msb;
      m_par.bps = f.bps;
      return;
    }
  }

  /* Default to AE_FMT_S16NE */
  m_par.bits = 16;
  m_par.bps = 2;
  m_par.sig = 1;
  m_par.le = SIO_LE_NATIVE;
}

bool CAESinkSNDIO::ParToAudioFormat(AEAudioFormat& format)
{
  AEDataFormat dataFormat = lookupDataFormat(m_par.bits, m_par.bps, m_par.sig, m_par.le, m_par.msb);
  if (dataFormat == AE_FMT_INVALID)
  {
    CLog::Log(LOGERROR, "CAESinkSNDIO::ParToAudioFormat - invalid data format");
    return false;
  }

  if (m_par.pchan > nitems(channelMap))
  {
    CLog::Log(LOGERROR, "CAESinkSNDIO::ParToAudioFormat - too many channels: {}", m_par.pchan);
    return false;
  }

  CAEChannelInfo info;
  for (unsigned int i = 0; i < m_par.pchan; i++)
      info += channelMap[i];
  format.m_channelLayout = info;
  format.m_dataFormat = dataFormat;
  format.m_sampleRate = m_par.rate;
  format.m_frameSize = m_par.bps * m_par.pchan;
  format.m_frames = m_par.bufsz / format.m_frameSize;

  return true;
}

CAESinkSNDIO::CAESinkSNDIO()
{
  m_hdl = nullptr;
}

CAESinkSNDIO::~CAESinkSNDIO()
{
  Deinitialize();
}

void CAESinkSNDIO::Register()
{
  AE::AESinkRegEntry entry;
  entry.sinkName = "SNDIO";
  entry.createFunc = CAESinkSNDIO::Create;
  entry.enumerateFunc = CAESinkSNDIO::EnumerateDevicesEx;
  AE::CAESinkFactory::RegisterSink(entry);
}

std::unique_ptr<IAESink> CAESinkSNDIO::Create(std::string& device, AEAudioFormat& desiredFormat)
{
  auto sink = std::make_unique<CAESinkSNDIO>();
  if (sink->Initialize(desiredFormat, device))
    return sink;

  return {};
}

bool CAESinkSNDIO::Initialize(AEAudioFormat &format, std::string &device)
{
  if ((m_hdl = sio_open(SIO_DEVANY, SIO_PLAY, 0)) == nullptr)
  {
    CLog::Log(LOGERROR, "CAESinkSNDIO::Initialize - Failed to open device");
    return false;
  }

  AudioFormatToPar(format);
  if (!sio_setpar(m_hdl, &m_par) ||
      !sio_getpar(m_hdl, &m_par) ||
      !ParToAudioFormat(format))
  {
    CLog::Log(LOGERROR, "CAESinkSNDIO::Initialize - could not negotiate parameters");
    return false;
  }

  m_played = m_written = 0;

  sio_onmove(m_hdl, CAESinkSNDIO::OnmoveCb, this);

  if (!sio_start(m_hdl))
  {
    CLog::Log(LOGERROR, "CAESinkSNDIO::Initialize - sio_start failed");
    return false;
  }

  return true;
}

void CAESinkSNDIO::Deinitialize()
{
  if (m_hdl != nullptr)
  {
    sio_close(m_hdl);
    m_hdl = nullptr;
  }
}

void CAESinkSNDIO::Stop()
{
  if (!m_hdl)
    return;

  if (!sio_stop(m_hdl))
    CLog::Log(LOGERROR, "CAESinkSNDIO::Stop - Failed");

  m_written = m_played = 0;
}

void CAESinkSNDIO::OnmoveCb(void *arg, int delta) {
  CAESinkSNDIO* self = static_cast<CAESinkSNDIO*>(arg);
  self->m_played += delta;
}

void CAESinkSNDIO::GetDelay(AEDelayStatus& status)
{
  unsigned int frameSize = m_par.bps * m_par.pchan;
  double delay = 1.0 * ((m_written / frameSize) - m_played) / m_par.rate;
  status.SetDelay(delay);
}

unsigned int CAESinkSNDIO::AddPackets(uint8_t **data, unsigned int frames, unsigned int offset)
{
  if (!m_hdl)
    return INT_MAX;

  unsigned int frameSize = m_par.bps * m_par.pchan;
  size_t size = frames * frameSize;
  void *buffer = data[0] + offset * frameSize;
  size_t wrote = sio_write(m_hdl, buffer, size);
  m_written += wrote;
  return wrote / frameSize;
}

void CAESinkSNDIO::Drain()
{
  if(!m_hdl)
    return;

  if (!sio_stop(m_hdl) || !sio_start(m_hdl))
    CLog::Log(LOGERROR, "CAESinkSNDIO::Drain - failed");

  m_written = m_played = 0;
}

void CAESinkSNDIO::EnumerateDevicesEx(AEDeviceInfoList &list, bool force)
{
  struct sio_hdl *hdl;
  struct sio_cap cap;

  if ((hdl = sio_open(SIO_DEVANY, SIO_PLAY, 0)) == nullptr)
  {
    CLog::Log(LOGERROR, "CAESinkSNDIO::EnumerateDevicesEx - sio_open");
    return;
  }

  if (!sio_getcap(hdl, &cap))
  {
    CLog::Log(LOGERROR, "CAESinkSNDIO::EnumerateDevicesEx - sio_getcap");
    return;
  }

  sio_close(hdl);
  hdl = nullptr;

  for (unsigned int i = 0; i < cap.nconf; i++)
  {
    CAEDeviceInfo info;
    sio_cap::sio_conf conf = cap.confs[i];

    info.m_deviceName = SIO_DEVANY;
    info.m_displayName = "sndio";
    info.m_displayNameExtra = "#" + std::to_string(i);
    info.m_deviceType = AE_DEVTYPE_PCM;
    info.m_wantsIECPassthrough = false;

    unsigned int maxchan = 0;
    for (unsigned int j = 0; j < SIO_NCHAN; j++)
    {
      if (conf.pchan & (1 << j))
        maxchan = MAX(maxchan, cap.pchan[j]);
    }

    maxchan = MIN(maxchan, nitems(channelMap));
    for (unsigned int j = 0; j < maxchan; j++)
      info.m_channels += channelMap[j];

    for (unsigned int j = 0; j < SIO_NRATE; j++)
    {
      if (conf.rate & (1 << j))
      {
        info.m_sampleRates.push_back(cap.rate[j]);
      }
    }

    for (unsigned int j = 0; j < SIO_NENC; j++)
    {
      if (conf.enc & (1 << j))
      {
        AEDataFormat format = lookupDataFormat(cap.enc[j].bits, cap.enc[j].bps, cap.enc[j].sig, cap.enc[j].le, cap.enc[j].msb);
        if (format != AE_FMT_INVALID)
          info.m_dataFormats.push_back(format);
      }
    }

    list.push_back(info);
  }
}
