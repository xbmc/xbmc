/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "DVDVideoCodecWebCodecs.h"

#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecWebCodecsBridge.h"
#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/log.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

#include <emscripten/bind.h>
#include <emscripten/threading.h>

// Expose the bridge enums to JavaScript via Embind so the JS bridge can read
// the canonical C++ values (Module.WebCodecsPixelFormat.NV12 etc.) instead of
// duplicating them. Must live at global scope, hence outside any namespace.
EMSCRIPTEN_BINDINGS(kodi_webcodecs_bridge)
{
  emscripten::enum_<WebCodecsPixelFormat>("WebCodecsPixelFormat")
      .value("UNKNOWN", WEBCODECS_PIXFMT_UNKNOWN)
      .value("YUV420P", WEBCODECS_PIXFMT_YUV420P)
      .value("NV12", WEBCODECS_PIXFMT_NV12);

  emscripten::enum_<WebCodecsPushStatus>("WebCodecsPushStatus")
      .value("QUEUED", WEBCODECS_PUSH_QUEUED)
      .value("EMPTY", WEBCODECS_PUSH_EMPTY)
      .value("HANDLE_NOT_FOUND", WEBCODECS_PUSH_HANDLE_NOT_FOUND)
      .value("DECODER_FAILED", WEBCODECS_PUSH_DECODER_FAILED)
      .value("NOT_CONFIGURED", WEBCODECS_PUSH_NOT_CONFIGURED)
      .value("DECODE_THREW", WEBCODECS_PUSH_DECODE_THREW)
      .value("BUSY", WEBCODECS_PUSH_BUSY);
}

namespace
{
constexpr int INVALID_DECODER_HANDLE = 0;
constexpr int DEFAULT_ALIGNMENT = 64;
constexpr int DECODER_ERROR_BUFFER_SIZE = 512;
constexpr int DISPLAY_WIDTH_ALIGN_MASK = -3;
constexpr double FRAME_WAIT_MS = 20.0;
constexpr auto DRAIN_TIMEOUT = std::chrono::milliseconds(1000);
constexpr auto COPY_TIMEOUT = std::chrono::milliseconds(500);
constexpr int DROPPED_FRAMES_LOG_THRESHOLD = 8;
constexpr int PICTURE_COLOR_BITS = 8;

constexpr int CODEC_STRING_BUFFER_SIZE = 24;
constexpr int H264_AVCC_MIN_EXTRADATA_SIZE = 7;
constexpr uint8_t H264_AVCC_CONFIG_VERSION = 1;
constexpr int H264_AVCC_PROFILE_OFFSET = 1;
constexpr int H264_AVCC_COMPAT_OFFSET = 2;
constexpr int H264_AVCC_LEVEL_OFFSET = 3;
constexpr int H264_AVCC_LENGTH_SIZE_OFFSET = 4;
constexpr uint8_t H264_AVCC_LENGTH_SIZE_MASK = 0x03;

constexpr uint8_t H264_NAL_TYPE_MASK = 0x1F;
constexpr uint8_t H264_NAL_TYPE_IDR = 5;

constexpr uint8_t ANNEXB_START_CODE_BYTE = 0x01;

constexpr int VP9_FRAME_MARKER = 2;
constexpr int VP9_PROFILE_WITH_RESERVED_BIT = 3;
constexpr int VP9_FRAME_TYPE_KEY = 0;

int AlignUp(int value, int alignment)
{
  return ((value + alignment - 1) / alignment) * alignment;
}

// An AVCDecoderConfigurationRecord is only complete once numOfSequenceParameterSets,
// its last fixed field, is present at byte 6.
bool HasAVCCExtradata(const CDVDStreamInfo& hints)
{
  return hints.extradata.GetSize() >= H264_AVCC_MIN_EXTRADATA_SIZE &&
         hints.extradata.GetData()[0] == H264_AVCC_CONFIG_VERSION;
}

std::string BuildH264CodecString(const CDVDStreamInfo& hints)
{
  const auto& extradata = hints.extradata;
  if (HasAVCCExtradata(hints))
  {
    const uint8_t profile = extradata.GetData()[H264_AVCC_PROFILE_OFFSET];
    const uint8_t compat = extradata.GetData()[H264_AVCC_COMPAT_OFFSET];
    const uint8_t level = extradata.GetData()[H264_AVCC_LEVEL_OFFSET];

    char buffer[CODEC_STRING_BUFFER_SIZE];
    std::snprintf(buffer, sizeof(buffer), "avc1.%02X%02X%02X", profile, compat, level);
    return buffer;
  }

  return "avc1.42E01E";
}

int ClampTwoDigitCodecValue(int value, int fallback)
{
  if (value < 0)
    value = fallback;
  return std::clamp(value, 0, 99);
}

int VP9ProfileFromHints(const CDVDStreamInfo& hints)
{
  switch (hints.profile)
  {
    case AV_PROFILE_VP9_0:
      return 0;
    case AV_PROFILE_VP9_1:
      return 1;
    case AV_PROFILE_VP9_2:
      return 2;
    case AV_PROFILE_VP9_3:
      return 3;
    default:
      return hints.bitdepth > 8 ? 2 : 0;
  }
}

std::string BuildVP9CodecString(const CDVDStreamInfo& hints)
{
  const int profile = ClampTwoDigitCodecValue(VP9ProfileFromHints(hints), 0);
  const int level = ClampTwoDigitCodecValue(hints.level, 10);
  const int bitDepth = ClampTwoDigitCodecValue(hints.bitdepth > 0 ? hints.bitdepth : 8, 8);

  char buffer[CODEC_STRING_BUFFER_SIZE];
  std::snprintf(buffer, sizeof(buffer), "vp09.%02d.%02d.%02d", profile, level, bitDepth);
  return buffer;
}

// Returns true if the H.264 sample contains an IDR NAL unit.
bool H264SampleContainsIDR(const uint8_t* data, int size, int nalLengthSize)
{
  if (!data || size <= 0)
    return false;

  if (nalLengthSize > 0)
  {
    int offset = 0;
    while (offset + nalLengthSize <= size)
    {
      uint32_t nalSize = 0;
      for (int i = 0; i < nalLengthSize; ++i)
        nalSize = (nalSize << 8) | data[offset + i];
      offset += nalLengthSize;
      if (nalSize == 0 || offset + static_cast<int>(nalSize) > size)
        break;
      if ((data[offset] & H264_NAL_TYPE_MASK) == H264_NAL_TYPE_IDR)
        return true;
      offset += nalSize;
    }
    return false;
  }

  for (int i = 0; i + 3 < size; ++i)
  {
    const bool threeByteStart = data[i] == 0x00 && data[i + 1] == 0x00 &&
                                data[i + 2] == ANNEXB_START_CODE_BYTE;
    const bool fourByteStart = data[i] == 0x00 && data[i + 1] == 0x00 && data[i + 2] == 0x00 &&
                               data[i + 3] == ANNEXB_START_CODE_BYTE;
    if (!threeByteStart && !fourByteStart)
      continue;

    const int nalStart = threeByteStart ? i + 3 : i + 4;
    if (nalStart < size && (data[nalStart] & H264_NAL_TYPE_MASK) == H264_NAL_TYPE_IDR)
      return true;
  }
  return false;
}

bool VP8SampleIsKeyFrame(const uint8_t* data, int size)
{
  return data && size > 0 && (data[0] & 0x01) == 0;
}

bool VP9SampleIsKeyFrame(const uint8_t* data, int size)
{
  if (!data || size <= 0)
    return false;

  // The VP9 uncompressed header is a plain f(n) bit stream, read MSB first, so the
  // first field starts at bit 7. Everything needed to classify the frame fits in the
  // first byte.
  const uint8_t firstByte = data[0];
  int bit = 7;
  const auto readBit = [&firstByte, &bit]() { return (firstByte >> bit--) & 0x01; };

  const int markerHigh = readBit();
  const int markerLow = readBit();
  if (((markerHigh << 1) | markerLow) != VP9_FRAME_MARKER)
    return false;

  const int profileLowBit = readBit();
  const int profileHighBit = readBit();
  if (((profileHighBit << 1) | profileLowBit) == VP9_PROFILE_WITH_RESERVED_BIT)
    readBit(); // reserved_zero

  const bool showExistingFrame = readBit() != 0;
  if (showExistingFrame)
    return false;

  return readBit() == VP9_FRAME_TYPE_KEY;
}

// WebCodecs needs 'key' to mean "decodable on its own". The demuxer flag is not
// that: FFmpeg sets AV_PKT_FLAG_KEY on non-IDR I-frames (open GOP), whose
// leading pictures reference frames the decoder never saw. Parse the bitstream
// where we can and only trust the demuxer for codecs we don't parse.
bool PacketIsKeyFrame(const CDVDStreamInfo& hints, const DemuxPacket& packet, int nalLengthSize)
{
  switch (hints.codec)
  {
    case AV_CODEC_ID_H264:
      return H264SampleContainsIDR(packet.pData, packet.iSize, nalLengthSize);
    case AV_CODEC_ID_VP8:
      return VP8SampleIsKeyFrame(packet.pData, packet.iSize);
    case AV_CODEC_ID_VP9:
      return VP9SampleIsKeyFrame(packet.pData, packet.iSize);
    default:
      return packet.m_keyFrame;
  }
}

// Reads any pending diagnostic string from the JS decoder side.
std::string ReadDecoderError(int decoderHandle)
{
  if (decoderHandle == INVALID_DECODER_HANDLE)
    return {};
  char buffer[DECODER_ERROR_BUFFER_SIZE];
  const int written = webcodecs_take_error(decoderHandle, buffer, sizeof(buffer));
  if (written <= 0)
    return {};
  buffer[sizeof(buffer) - 1] = '\0';
  return std::string(buffer);
}

const char* PushStatusToString(int status)
{
  switch (status)
  {
    case WEBCODECS_PUSH_QUEUED:
      return "queued";
    case WEBCODECS_PUSH_EMPTY:
      return "empty packet";
    case WEBCODECS_PUSH_HANDLE_NOT_FOUND:
      return "decoder handle not found";
    case WEBCODECS_PUSH_DECODER_FAILED:
      return "decoder in failed state";
    case WEBCODECS_PUSH_NOT_CONFIGURED:
      return "decoder not configured";
    case WEBCODECS_PUSH_DECODE_THREW:
      return "decode threw";
    case WEBCODECS_PUSH_BUSY:
      return "decoder busy (backpressure)";
  }
  return "unknown";
}

AVPixelFormat PixelFormatFromWebCodecs(int pixelFormat)
{
  switch (pixelFormat)
  {
    case WEBCODECS_PIXFMT_YUV420P:
      return AV_PIX_FMT_YUV420P;
    case WEBCODECS_PIXFMT_NV12:
      return AV_PIX_FMT_NV12;
    case WEBCODECS_PIXFMT_UNKNOWN:
    default:
      return AV_PIX_FMT_NONE;
  }
}
} // namespace

CDVDVideoCodecWebCodecs::CDVDVideoCodecWebCodecs(CProcessInfo& processInfo)
  : CDVDVideoCodec(processInfo)
{
  m_videoBufferPool = std::make_shared<CVideoBufferPoolSysMem>();
}

CDVDVideoCodecWebCodecs::~CDVDVideoCodecWebCodecs()
{
  Dispose();
}

std::unique_ptr<CDVDVideoCodec> CDVDVideoCodecWebCodecs::Create(CProcessInfo& processInfo)
{
  return std::make_unique<CDVDVideoCodecWebCodecs>(processInfo);
}

bool CDVDVideoCodecWebCodecs::Register()
{
  CDVDFactoryCodec::RegisterHWVideoCodec("webcodecs_dec", CDVDVideoCodecWebCodecs::Create);
  return true;
}

bool CDVDVideoCodecWebCodecs::SupportsCodec(const CDVDStreamInfo& hints) const
{
  return hints.codec == AV_CODEC_ID_H264 || hints.codec == AV_CODEC_ID_VP8 ||
         hints.codec == AV_CODEC_ID_VP9;
}

bool CDVDVideoCodecWebCodecs::BuildCodecConfiguration(const CDVDStreamInfo& hints)
{
  m_codecString.clear();
  m_annexB = false;
  m_nalLengthSize = 0;

  if (hints.codec == AV_CODEC_ID_H264)
  {
    m_codecString = BuildH264CodecString(hints);
    if (HasAVCCExtradata(hints))
    {
      m_annexB = false;
      m_nalLengthSize =
          (hints.extradata.GetData()[H264_AVCC_LENGTH_SIZE_OFFSET] & H264_AVCC_LENGTH_SIZE_MASK) +
          1;
    }
    else
    {
      m_annexB = true;
      m_nalLengthSize = 0;
    }
    return true;
  }

  if (hints.codec == AV_CODEC_ID_VP9)
  {
    m_codecString = BuildVP9CodecString(hints);
    return true;
  }

  if (hints.codec == AV_CODEC_ID_VP8)
  {
    m_codecString = "vp8";
    return true;
  }

  return false;
}

bool CDVDVideoCodecWebCodecs::CreateDecoder()
{
  const uint8_t* extraData = m_hints.extradata.GetData();
  const int extraDataSize = static_cast<int>(m_hints.extradata.GetSize());

  m_shared = {};
  m_decoderHandle = webcodecs_create_decoder(m_codecString.c_str(), m_hints.width, m_hints.height,
                                             extraData, extraDataSize, m_annexB ? 1 : 0, &m_shared);
  if (m_decoderHandle == INVALID_DECODER_HANDLE)
  {
    CLog::Log(LOGDEBUG,
              "CDVDVideoCodecWebCodecs::CreateDecoder - unable to configure decoder for {} "
              "(annexB={}, extradataSize={}, {}x{}): check that VideoDecoder is available and "
              "that the codec/description match the stream",
              m_codecString, m_annexB, extraDataSize, m_hints.width, m_hints.height);
    return false;
  }

  return true;
}

void CDVDVideoCodecWebCodecs::Dispose()
{
  ReleaseCopyBuffer();
  if (m_decoderHandle != INVALID_DECODER_HANDLE)
  {
    webcodecs_destroy_decoder(m_decoderHandle);
    m_decoderHandle = INVALID_DECODER_HANDLE;
  }
  m_opened = false;
  m_waitingForKeyFrame = true;
  m_drainSubmitted = false;
  m_lastLoggedDroppedFrames = 0;
  m_highWaterMark = 0;
}

bool CDVDVideoCodecWebCodecs::Open(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  if (!SupportsCodec(hints))
    return false;

  (void)options;
  Dispose();

  m_hints = hints;
  if (!BuildCodecConfiguration(hints))
    return false;

  if (!CreateDecoder())
    return false;

  m_name = "webcodecs-" + m_codecString;
  m_opened = true;
  m_waitingForKeyFrame = true;
  m_drainSubmitted = false;
  m_codecControlFlags = 0;
  m_lastLoggedDroppedFrames = 0;
  m_highWaterMark = 0;
  m_processInfo.SetVideoDecoderName(m_name, true);
  m_processInfo.SetVideoDimensions(hints.width, hints.height);
  CLog::Log(LOGINFO,
            "CDVDVideoCodecWebCodecs::Open - Using WebCodecs for video decoding: {} ({}x{}, annexB={})",
            m_codecString, hints.width, hints.height, m_annexB);
  return true;
}

bool CDVDVideoCodecWebCodecs::AddData(const DemuxPacket& packet)
{
  if (!m_opened || m_decoderHandle == INVALID_DECODER_HANDLE)
    return false;

  if (packet.iSize <= 0 || packet.pData == nullptr)
    return true;

  const bool isKeyFrame = PacketIsKeyFrame(m_hints, packet, m_nalLengthSize);

  // Feeding a delta before the first IDR would permanently fail the decoder.
  if (m_waitingForKeyFrame && !isKeyFrame)
    return true;

  if (SharedLoad(m_shared.busy))
    return false;

  double ptsSeconds = 0.0;
  if (packet.pts != DVD_NOPTS_VALUE && std::isfinite(packet.pts))
    ptsSeconds = packet.pts / static_cast<double>(DVD_TIME_BASE);

  double durationSeconds = 0.0;
  if (packet.duration > 0.0 && std::isfinite(packet.duration))
    durationSeconds = packet.duration / static_cast<double>(DVD_TIME_BASE);

  const int status = webcodecs_push_packet(m_decoderHandle, packet.pData, packet.iSize,
                                           isKeyFrame ? 1 : 0, ptsSeconds, durationSeconds);

  // Backpressure: decoder is saturated, let VideoPlayer re-queue this packet.
  if (status == WEBCODECS_PUSH_BUSY)
    return false;

  m_drainSubmitted = false;

  if (status <= 0)
  {
    if (status == WEBCODECS_PUSH_EMPTY)
      return true;

    const std::string error = ReadDecoderError(m_decoderHandle);
    CLog::Log(LOGDEBUG,
              "CDVDVideoCodecWebCodecs::AddData - decode rejected packet (size={}, pts={:.6f}s, "
              "key={}, status={} [{}]): {}",
              packet.iSize, ptsSeconds, isKeyFrame, status, PushStatusToString(status),
              error.empty() ? "<no js error>" : error);
    return false;
  }

  if (isKeyFrame)
    m_waitingForKeyFrame = false;

  return true;
}

void CDVDVideoCodecWebCodecs::Reset()
{
  if (m_decoderHandle == INVALID_DECODER_HANDLE)
    return;

  ReleaseCopyBuffer();
  webcodecs_reset_decoder(m_decoderHandle);
  m_waitingForKeyFrame = true;
  m_drainSubmitted = false;
  m_lastLoggedDroppedFrames = 0;
  m_highWaterMark = 0;
  CLog::Log(LOGDEBUG, "CDVDVideoCodecWebCodecs::Reset - decoder reset after seek/flush");
}

void CDVDVideoCodecWebCodecs::SetCodecControl(int flags)
{
  m_codecControlFlags = flags;
}

void CDVDVideoCodecWebCodecs::PollDecoderStats()
{
  if (m_decoderHandle == INVALID_DECODER_HANDLE)
    return;

  int droppedFrames = 0;
  int highWaterMark = 0;
  if (!webcodecs_read_stats(m_decoderHandle, &droppedFrames, &highWaterMark))
    return;

  if (highWaterMark > m_highWaterMark)
    m_highWaterMark = highWaterMark;

  if ((droppedFrames - m_lastLoggedDroppedFrames) < DROPPED_FRAMES_LOG_THRESHOLD)
    return;

  CLog::Log(LOGWARNING,
            "CDVDVideoCodecWebCodecs::GetPicture - dropped {} queued WebCodecs frames "
            "(totalDropped={}, queueHighWater={})",
            droppedFrames - m_lastLoggedDroppedFrames, droppedFrames, m_highWaterMark);
  m_lastLoggedDroppedFrames = droppedFrames;
}

int32_t CDVDVideoCodecWebCodecs::SharedLoad(const int32_t& field)
{
  return __atomic_load_n(&field, __ATOMIC_ACQUIRE);
}

// Callers sample `seenSignal` before checking the shared state, so a change that
// lands in between makes the futex wait return immediately.
void CDVDVideoCodecWebCodecs::WaitForDecoderSignal(uint32_t seenSignal, double maxWaitMs)
{
  emscripten_futex_wait(&m_shared.signal, seenSignal, maxWaitMs);
}

// VideoPlayerVideo stops draining at the first VC_BUFFER, so in-flight decodes
// have to be waited for here rather than reported as "need more data".
void CDVDVideoCodecWebCodecs::WaitForDrain()
{
  const auto deadline = std::chrono::steady_clock::now() + DRAIN_TIMEOUT;
  while (std::chrono::steady_clock::now() < deadline)
  {
    const auto seen = static_cast<uint32_t>(SharedLoad(m_shared.signal));
    if (SharedLoad(m_shared.queuedFrames) > 0 || SharedLoad(m_shared.inflight) <= 0 ||
        SharedLoad(m_shared.failed))
      return;
    WaitForDecoderSignal(seen, FRAME_WAIT_MS);
  }
}

bool CDVDVideoCodecWebCodecs::WaitForCopy()
{
  const auto deadline = std::chrono::steady_clock::now() + COPY_TIMEOUT;
  while (true)
  {
    const auto seen = static_cast<uint32_t>(SharedLoad(m_shared.signal));
    const int32_t result = SharedLoad(m_shared.copyResult);
    if (result != 0)
      return result > 0;
    if (std::chrono::steady_clock::now() >= deadline)
      return false;
    WaitForDecoderSignal(seen, FRAME_WAIT_MS);
  }
}

void CDVDVideoCodecWebCodecs::ReleaseCopyBuffer()
{
  if (!m_copyBuffer)
    return;
  WaitForCopy();
  m_copyBuffer->Release();
  m_copyBuffer = nullptr;
}

CVideoBuffer* CDVDVideoCodecWebCodecs::AcquirePictureBuffer(AVPixelFormat pixelFormat,
                                                            int bufferSize)
{
  m_videoBufferPool->Configure(pixelFormat, AlignUp(bufferSize, DEFAULT_ALIGNMENT));
  CVideoBuffer* buffer = m_videoBufferPool->Get();
  if (buffer && !buffer->GetMemPtr())
  {
    buffer->Release();
    return nullptr;
  }
  return buffer;
}

void CDVDVideoCodecWebCodecs::FillPictureMetadata(VideoPicture* pVideoPicture,
                                                  CVideoBuffer* videoBuffer,
                                                  AVPixelFormat pixelFormat,
                                                  int width,
                                                  int height,
                                                  bool keyFrame,
                                                  double ptsSeconds,
                                                  double durationSeconds) const
{
  pVideoPicture->Reset();
  pVideoPicture->videoBuffer = videoBuffer;
  pVideoPicture->pixelFormat = pixelFormat;
  pVideoPicture->iWidth = width;
  pVideoPicture->iHeight = height;

  double aspect = m_hints.aspect > 0.0 ? m_hints.aspect
                                       : (height > 0 ? static_cast<double>(width) / height : 1.0);
  pVideoPicture->iDisplayWidth =
      static_cast<int>(std::lrint(height * aspect)) & DISPLAY_WIDTH_ALIGN_MASK;
  pVideoPicture->iDisplayHeight = height;
  if (pVideoPicture->iDisplayWidth > static_cast<unsigned int>(width))
  {
    pVideoPicture->iDisplayWidth = width;
    pVideoPicture->iDisplayHeight =
        static_cast<int>(std::lrint(width / aspect)) & DISPLAY_WIDTH_ALIGN_MASK;
  }

  // VideoPicture::pts / iDuration are in DVD_TIME_BASE units (microseconds).
  pVideoPicture->pts = std::isfinite(ptsSeconds) ? ptsSeconds * DVD_TIME_BASE : DVD_NOPTS_VALUE;
  pVideoPicture->dts = DVD_NOPTS_VALUE;
  pVideoPicture->iDuration = durationSeconds > 0.0 ? durationSeconds * DVD_TIME_BASE : 0.0;
  pVideoPicture->iRepeatPicture = 0.0;
  pVideoPicture->iFlags = 0;
  pVideoPicture->iFrameType = keyFrame ? FRAME_TYPE_I : FRAME_TYPE_P;

  pVideoPicture->color_space = m_hints.colorSpace == AVCOL_SPC_UNSPECIFIED
                                   ? AVCOL_SPC_BT709
                                   : m_hints.colorSpace;
  pVideoPicture->color_primaries = m_hints.colorPrimaries == AVCOL_PRI_UNSPECIFIED
                                       ? AVCOL_PRI_BT709
                                       : m_hints.colorPrimaries;
  pVideoPicture->color_transfer =
      m_hints.colorTransferCharacteristic == AVCOL_TRC_UNSPECIFIED
          ? AVCOL_TRC_BT709
          : m_hints.colorTransferCharacteristic;
  pVideoPicture->m_originalColorPrimaries = pVideoPicture->color_primaries;
  pVideoPicture->color_range = m_hints.colorRange == AVCOL_RANGE_JPEG;
  pVideoPicture->chroma_position = AVCHROMA_LOC_LEFT;
  pVideoPicture->colorBits = PICTURE_COLOR_BITS;
}

CDVDVideoCodec::VCReturn CDVDVideoCodecWebCodecs::GetPicture(VideoPicture* pVideoPicture)
{
  if (!m_opened || m_decoderHandle == INVALID_DECODER_HANDLE)
    return VC_ERROR;

  const bool draining = (m_codecControlFlags & DVD_CODEC_CTRL_DRAIN) != 0;
  if (draining && !m_drainSubmitted)
  {
    m_drainSubmitted = true;
    m_waitingForKeyFrame = true;
    webcodecs_flush_decoder(m_decoderHandle);
    CLog::Log(LOGDEBUG, "CDVDVideoCodecWebCodecs::GetPicture - drain requested, flushing");
  }
  else if (!draining)
    m_drainSubmitted = false;

  if (draining)
    WaitForDrain();
  else
  {
    const auto seen = static_cast<uint32_t>(SharedLoad(m_shared.signal));
    if (SharedLoad(m_shared.queuedFrames) == 0 && SharedLoad(m_shared.busy))
      WaitForDecoderSignal(seen, FRAME_WAIT_MS);
  }

  if (SharedLoad(m_shared.failed))
  {
    const std::string error = ReadDecoderError(m_decoderHandle);
    CLog::Log(LOGERROR, "CDVDVideoCodecWebCodecs::GetPicture - decoder entered failed state: {}",
              error.empty() ? "<no js error>" : error);
    return VC_ERROR;
  }

  if (SharedLoad(m_shared.queuedFrames) == 0)
  {
    if (!draining)
      return VC_BUFFER;

    PollDecoderStats();
    const int32_t inflight = SharedLoad(m_shared.inflight);
    if (inflight > 0)
      CLog::Log(LOGWARNING,
                "CDVDVideoCodecWebCodecs::GetPicture - drain timed out with {} pending "
                "WebCodecs operations",
                inflight);
    else
      CLog::Log(LOGDEBUG, "CDVDVideoCodecWebCodecs::GetPicture - drain completed (EOF)");
    return VC_EOF;
  }

  const AVPixelFormat pixelFormat = PixelFormatFromWebCodecs(SharedLoad(m_shared.nextPixelFormat));
  const int payloadSize = SharedLoad(m_shared.nextPayloadSize);
  if (pixelFormat == AV_PIX_FMT_NONE || payloadSize <= 0)
  {
    CLog::Log(LOGERROR,
              "CDVDVideoCodecWebCodecs::GetPicture - invalid queued frame metadata "
              "(pixelFormat={}, payloadSize={})",
              SharedLoad(m_shared.nextPixelFormat), payloadSize);
    return VC_ERROR;
  }

  CVideoBuffer* videoBuffer = AcquirePictureBuffer(pixelFormat, payloadSize);
  if (!videoBuffer)
    return VC_NOBUFFER;

  WebCodecsFrameInfo info{};
  const int copyStatus =
      webcodecs_copy_next_frame(m_decoderHandle, videoBuffer->GetMemPtr(), payloadSize, &info);
  if (copyStatus != 1)
  {
    videoBuffer->Release();
    if (copyStatus == 0)
      return VC_BUFFER;

    const std::string error = ReadDecoderError(m_decoderHandle);
    CLog::Log(LOGERROR,
              "CDVDVideoCodecWebCodecs::GetPicture - cannot start frame copy (status={}, "
              "payloadSize={} vs published {}): {}",
              copyStatus, info.payloadSize, payloadSize,
              error.empty() ? "<no js error>" : error);
    return VC_ERROR;
  }

  m_copyBuffer = videoBuffer;
  if (!WaitForCopy())
  {
    if (SharedLoad(m_shared.copyResult) == 0)
    {
      CLog::Log(LOGERROR, "CDVDVideoCodecWebCodecs::GetPicture - frame copy did not complete "
                          "within {} ms",
                COPY_TIMEOUT.count());
      return VC_ERROR;
    }

    ReleaseCopyBuffer();
    const std::string error = ReadDecoderError(m_decoderHandle);
    CLog::Log(LOGERROR, "CDVDVideoCodecWebCodecs::GetPicture - frame copy failed: {}",
              error.empty() ? "<no js error>" : error);
    return VC_ERROR;
  }
  m_copyBuffer = nullptr;

  const AVPixelFormat framePixelFormat = PixelFormatFromWebCodecs(info.pixelFormat);
  if (framePixelFormat == AV_PIX_FMT_NONE)
  {
    videoBuffer->Release();
    CLog::Log(LOGERROR, "CDVDVideoCodecWebCodecs::GetPicture - unsupported pixel format id {}",
              info.pixelFormat);
    return VC_ERROR;
  }

  const int strides[YuvImage::MAX_PLANES] = {info.yStride, info.uStride, info.vStride};
  const int planeOffsets[YuvImage::MAX_PLANES] = {0, info.uOffset, info.vOffset};
  videoBuffer->SetPixelFormat(framePixelFormat);
  videoBuffer->SetDimensions(info.width, info.height, strides, planeOffsets);

  FillPictureMetadata(pVideoPicture, videoBuffer, framePixelFormat, info.width, info.height,
                      info.keyFrame != 0, info.ptsSeconds, info.durationSeconds);
  return VC_PICTURE;
}
