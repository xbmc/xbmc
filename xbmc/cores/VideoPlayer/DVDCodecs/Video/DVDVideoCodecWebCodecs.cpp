/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDVideoCodecWebCodecs.h"

#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDStreamInfo.h"
#include "DVDVideoCodecFFmpeg.h"
#include "DVDVideoCodecWebCodecsBitstream.h"
#include "DVDVideoCodecWebCodecsBridge.h"
#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "cores/VideoPlayer/Interface/TimingConstants.h"
#include "utils/log.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <optional>
#include <string>

#include <emscripten/bind.h>
#include <emscripten/threading.h>

extern "C"
{
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

namespace
{
// WebCodecs VideoPixelFormat values with their FFmpeg equivalent, or
// AV_PIX_FMT_NONE where FFmpeg has none, and their bit depth. The names are
// the WebCodecs strings and double as the Embind enumerator names.
struct WebCodecsPixelFormatInfo
{
  WebCodecsPixelFormat id;
  const char* name;
  AVPixelFormat avFormat;
  int bitDepth;
};

constexpr WebCodecsPixelFormatInfo PIXEL_FORMATS[] = {
    {WEBCODECS_PIXFMT_I420, "I420", AV_PIX_FMT_YUV420P, 8},
    {WEBCODECS_PIXFMT_I420P10, "I420P10", AV_PIX_FMT_YUV420P10, 10},
    {WEBCODECS_PIXFMT_I420P12, "I420P12", AV_PIX_FMT_YUV420P12, 12},
    {WEBCODECS_PIXFMT_I420A, "I420A", AV_PIX_FMT_YUVA420P, 8},
    {WEBCODECS_PIXFMT_I420AP10, "I420AP10", AV_PIX_FMT_YUVA420P10, 10},
    {WEBCODECS_PIXFMT_I420AP12, "I420AP12", AV_PIX_FMT_NONE, 12},
    {WEBCODECS_PIXFMT_I422, "I422", AV_PIX_FMT_YUV422P, 8},
    {WEBCODECS_PIXFMT_I422P10, "I422P10", AV_PIX_FMT_YUV422P10, 10},
    {WEBCODECS_PIXFMT_I422P12, "I422P12", AV_PIX_FMT_YUV422P12, 12},
    {WEBCODECS_PIXFMT_I422A, "I422A", AV_PIX_FMT_YUVA422P, 8},
    {WEBCODECS_PIXFMT_I422AP10, "I422AP10", AV_PIX_FMT_YUVA422P10, 10},
    {WEBCODECS_PIXFMT_I422AP12, "I422AP12", AV_PIX_FMT_YUVA422P12, 12},
    {WEBCODECS_PIXFMT_I444, "I444", AV_PIX_FMT_YUV444P, 8},
    {WEBCODECS_PIXFMT_I444P10, "I444P10", AV_PIX_FMT_YUV444P10, 10},
    {WEBCODECS_PIXFMT_I444P12, "I444P12", AV_PIX_FMT_YUV444P12, 12},
    {WEBCODECS_PIXFMT_I444A, "I444A", AV_PIX_FMT_YUVA444P, 8},
    {WEBCODECS_PIXFMT_I444AP10, "I444AP10", AV_PIX_FMT_YUVA444P10, 10},
    {WEBCODECS_PIXFMT_I444AP12, "I444AP12", AV_PIX_FMT_YUVA444P12, 12},
    {WEBCODECS_PIXFMT_NV12, "NV12", AV_PIX_FMT_NV12, 8},
    {WEBCODECS_PIXFMT_RGBA, "RGBA", AV_PIX_FMT_RGBA, 8},
    {WEBCODECS_PIXFMT_RGBX, "RGBX", AV_PIX_FMT_RGB0, 8},
    {WEBCODECS_PIXFMT_BGRA, "BGRA", AV_PIX_FMT_BGRA, 8},
    {WEBCODECS_PIXFMT_BGRX, "BGRX", AV_PIX_FMT_BGR0, 8},
    {WEBCODECS_PIXFMT_OPAQUE, "opaque", AV_PIX_FMT_NONE, 0},
};

const WebCodecsPixelFormatInfo* FindPixelFormat(int pixelFormat)
{
  for (const auto& format : PIXEL_FORMATS)
  {
    if (format.id == pixelFormat)
      return &format;
  }
  return nullptr;
}
} // namespace

// Expose the bridge enums to JavaScript via Embind so the JS bridge can read
// the canonical C++ values (Module.WebCodecsPixelFormat.NV12 etc.) instead of
// duplicating them. Must live at global scope, hence outside any namespace.
EMSCRIPTEN_BINDINGS(kodi_webcodecs_bridge)
{
  emscripten::enum_<WebCodecsPixelFormat> pixelFormats("WebCodecsPixelFormat");
  pixelFormats.value("UNKNOWN", WEBCODECS_PIXFMT_UNKNOWN);
  for (const auto& format : PIXEL_FORMATS)
    pixelFormats.value(format.name, format.id);

  emscripten::enum_<WebCodecsCopyResult>("WebCodecsCopyResult")
      .value("OK", WEBCODECS_COPY_OK)
      .value("FAILED", WEBCODECS_COPY_FAILED)
      .value("DST_TOO_SMALL", WEBCODECS_COPY_DST_TOO_SMALL)
      .value("NO_FRAME", WEBCODECS_COPY_NO_FRAME);

  // The VideoColorSpace values WebCodecs defines, as FFmpeg numbers the frame
  // ring carries.
  emscripten::enum_<AVColorSpace>("AVColorSpace")
      .value("UNSPECIFIED", AVCOL_SPC_UNSPECIFIED)
      .value("RGB", AVCOL_SPC_RGB)
      .value("BT709", AVCOL_SPC_BT709)
      .value("BT470BG", AVCOL_SPC_BT470BG)
      .value("SMPTE170M", AVCOL_SPC_SMPTE170M)
      .value("BT2020_NCL", AVCOL_SPC_BT2020_NCL);

  emscripten::enum_<AVColorPrimaries>("AVColorPrimaries")
      .value("UNSPECIFIED", AVCOL_PRI_UNSPECIFIED)
      .value("BT709", AVCOL_PRI_BT709)
      .value("BT470BG", AVCOL_PRI_BT470BG)
      .value("SMPTE170M", AVCOL_PRI_SMPTE170M)
      .value("BT2020", AVCOL_PRI_BT2020)
      .value("SMPTE432", AVCOL_PRI_SMPTE432);

  emscripten::enum_<AVColorTransferCharacteristic>("AVColorTransferCharacteristic")
      .value("UNSPECIFIED", AVCOL_TRC_UNSPECIFIED)
      .value("BT709", AVCOL_TRC_BT709)
      .value("SMPTE170M", AVCOL_TRC_SMPTE170M)
      .value("IEC61966_2_1", AVCOL_TRC_IEC61966_2_1)
      .value("LINEAR", AVCOL_TRC_LINEAR)
      .value("SMPTE2084", AVCOL_TRC_SMPTE2084)
      .value("ARIB_STD_B67", AVCOL_TRC_ARIB_STD_B67);
}

namespace
{
constexpr int INVALID_DECODER_HANDLE = 0;
constexpr int DEFAULT_ALIGNMENT = 64;
constexpr int DECODER_ERROR_BUFFER_SIZE = 512;
constexpr int DISPLAY_WIDTH_ALIGN_MASK = -3;
constexpr double FRAME_WAIT_MS = 20.0;
constexpr auto DRAIN_TIMEOUT = std::chrono::milliseconds(1000);
constexpr auto DRAIN_SETTLE_TIME = std::chrono::milliseconds(100);
constexpr auto COPY_TIMEOUT = std::chrono::milliseconds(500);
constexpr auto CONFIGURE_TIMEOUT = std::chrono::milliseconds(5000);
constexpr int MAX_DECODER_RECOVERIES = 3;
// A replaced decoder counts as recovered once it has produced this many frames.
constexpr int FRAMES_TO_RECOVER = 30;
constexpr int DROPPED_FRAMES_LOG_THRESHOLD = 8;
constexpr int PICTURE_COLOR_BITS = 8;
constexpr int YUV_STRIDE_ALIGNMENT = 32;

int AlignUp(int value, int alignment)
{
  return ((value + alignment - 1) / alignment) * alignment;
}

struct Yuv420pLayout
{
  int yStride;
  int uvStride;
  int ySize;
  int uvSize;
  int Size() const { return ySize + 2 * uvSize; }
};

Yuv420pLayout Yuv420pLayoutFor(int width, int height)
{
  const int yStride = AlignUp(width, YUV_STRIDE_ALIGNMENT);
  const int uvStride = yStride / 2;
  return {yStride, uvStride, yStride * height, uvStride * ((height + 1) / 2)};
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

AVPixelFormat PixelFormatFromWebCodecs(int pixelFormat)
{
  const WebCodecsPixelFormatInfo* format = FindPixelFormat(pixelFormat);
  return format ? format->avFormat : AV_PIX_FMT_NONE;
}

const char* WebCodecsPixelFormatName(int pixelFormat)
{
  const WebCodecsPixelFormatInfo* format = FindPixelFormat(pixelFormat);
  return format ? format->name : "unknown";
}

bool IsPackedRgb(AVPixelFormat format)
{
  return format == AV_PIX_FMT_RGBA || format == AV_PIX_FMT_RGB0 || format == AV_PIX_FMT_BGRA ||
         format == AV_PIX_FMT_BGR0;
}

int SwsColorspace(AVColorSpace colorSpace)
{
  switch (colorSpace)
  {
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
      return SWS_CS_ITU601;
    case AVCOL_SPC_BT2020_NCL:
    case AVCOL_SPC_BT2020_CL:
      return SWS_CS_BT2020;
    default:
      return SWS_CS_ITU709;
  }
}
} // namespace

CVideoBufferWebCodecs::CVideoBufferWebCodecs(int id) : CVideoBuffer(id)
{
}

void CVideoBufferWebCodecs::SetFrame(int decoderHandle, int32_t sequence, AVPixelFormat format)
{
  m_decoderHandle = decoderHandle;
  m_sequence = sequence;
  m_pixFormat = format;
}

CVideoBuffer* CVideoBufferPoolWebCodecs::Get()
{
  std::unique_lock lock(m_critSection);

  CVideoBufferWebCodecs* buffer;
  if (!m_free.empty())
  {
    buffer = m_all[m_free.front()].get();
    m_free.pop_front();
  }
  else
  {
    m_all.emplace_back(std::make_unique<CVideoBufferWebCodecs>(static_cast<int>(m_all.size())));
    buffer = m_all.back().get();
  }

  buffer->Acquire(GetPtr());
  return buffer;
}

// The last reference is gone, so the frame is either uploaded already or will
// never be; the release is a no-op for a frame the bridge has closed.
void CVideoBufferPoolWebCodecs::Return(int id)
{
  std::unique_lock lock(m_critSection);

  CVideoBufferWebCodecs& buffer = *m_all[id];
  if (buffer.GetSequence() >= 0)
    webcodecs_release_frame(buffer.GetDecoderHandle(), buffer.GetSequence());
  buffer.SetFrame(0, -1, AV_PIX_FMT_NONE);
  m_free.push_back(id);
}

CDVDVideoCodecWebCodecs::CDVDVideoCodecWebCodecs(CProcessInfo& processInfo)
  : CDVDVideoCodec(processInfo)
{
  m_framePool = std::make_shared<CVideoBufferPoolWebCodecs>();
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
  return hints.codec == AV_CODEC_ID_H264 || hints.codec == AV_CODEC_ID_HEVC ||
         hints.codec == AV_CODEC_ID_VP8 || hints.codec == AV_CODEC_ID_VP9 ||
         hints.codec == AV_CODEC_ID_AV1;
}

bool CDVDVideoCodecWebCodecs::BuildCodecConfiguration(const CDVDStreamInfo& hints)
{
  const std::optional<WEBCODECS::CodecConfiguration> config =
      WEBCODECS::GetCodecConfiguration(hints);
  if (!config)
    return false;

  m_codecString = config->codecString;
  m_annexB = config->annexB;
  m_prependExtradata = config->prependExtradata;
  m_nalLengthSize = config->nalLengthSize;
  m_hasDescription = config->hasDescription;
  return true;
}

bool CDVDVideoCodecWebCodecs::CreateDecoder()
{
  const uint8_t* description = m_hasDescription ? m_hints.extradata.GetData() : nullptr;
  const int descriptionSize = m_hasDescription ? static_cast<int>(m_hints.extradata.GetSize()) : 0;

  m_shared = {};
  m_pushCount = 0;
  m_copyId = 0;
  m_decoderHandle =
      webcodecs_create_decoder(m_codecString.c_str(), m_hints.width, m_hints.height, description,
                               descriptionSize, m_annexB ? 1 : 0, &m_shared);
  if (m_decoderHandle == INVALID_DECODER_HANDLE)
  {
    CLog::Log(LOGDEBUG, "CDVDVideoCodecWebCodecs::CreateDecoder - VideoDecoder unavailable for {}",
              m_codecString);
    return false;
  }

  if (!WaitForConfigured())
  {
    const std::string error = ReadDecoderError(m_decoderHandle);
    CLog::Log(LOGINFO,
              "CDVDVideoCodecWebCodecs::CreateDecoder - unable to configure decoder for {} "
              "(annexB={}, descriptionSize={}, {}x{}): {}",
              m_codecString, m_annexB, descriptionSize, m_hints.width, m_hints.height,
              error.empty() ? "<no js error>" : error);
    webcodecs_destroy_decoder(m_decoderHandle);
    m_decoderHandle = INVALID_DECODER_HANDLE;
    return false;
  }

  return true;
}

// The bridge configures the decoder once isConfigSupported() has answered;
// true when it did, false when no configuration was accepted or the answer
// never came.
bool CDVDVideoCodecWebCodecs::WaitForConfigured()
{
  const auto deadline = std::chrono::steady_clock::now() + CONFIGURE_TIMEOUT;
  while (true)
  {
    const auto seen = static_cast<uint32_t>(SharedLoad(m_shared.signal));
    if (SharedLoad(m_shared.configured))
      return true;
    if (SharedLoad(m_shared.failed))
      return false;
    if (std::chrono::steady_clock::now() >= deadline)
    {
      CLog::Log(LOGWARNING,
                "CDVDVideoCodecWebCodecs::WaitForConfigured - no answer from the browser within "
                "{} ms",
                CONFIGURE_TIMEOUT.count());
      return false;
    }
    WaitForDecoderSignal(seen, FRAME_WAIT_MS);
  }
}

void CDVDVideoCodecWebCodecs::Dispose()
{
  ReleaseCopyBuffer();
  sws_freeContext(m_swsContext);
  m_swsContext = nullptr;
  m_swsFormat = AV_PIX_FMT_NONE;
  if (m_decoderHandle != INVALID_DECODER_HANDLE)
  {
    webcodecs_destroy_decoder(m_decoderHandle);
    m_decoderHandle = INVALID_DECODER_HANDLE;
  }
  m_opened = false;
  m_waitingForKeyFrame = true;
  m_drained = false;
  m_lastLoggedDroppedFrames = 0;
  m_highWaterMark = 0;
  m_recoveries = 0;
}

bool CDVDVideoCodecWebCodecs::Open(CDVDStreamInfo& hints, CDVDCodecOptions& options)
{
  if (!SupportsCodec(hints))
    return false;

  // Interlaced streams need FFmpeg's deinterlacing.
  if (hints.interlaced)
    return false;

  // The heap copy takes 8-bit 4:2:0 frames only; FFmpeg decodes the rest.
  m_textureUpload = webcodecs_probe_texture_upload() != 0;
  if (!m_textureUpload && (hints.bitdepth > 8 || !WEBCODECS::IsChroma420Profile(hints)))
    return false;

  Dispose();
  m_options = options;

  m_hints = hints;
  if (!BuildCodecConfiguration(hints))
    return false;

  if (!CreateDecoder())
    return false;

  m_name = "webcodecs-" + m_codecString;
  m_hardware = SharedLoad(m_shared.hardware) != 0;
  m_opened = true;
  m_waitingForKeyFrame = true;
  m_drained = false;
  m_codecControlFlags = 0;
  m_lastLoggedDroppedFrames = 0;
  m_highWaterMark = 0;
  m_reportedPixelFormat.clear();
  m_processInfo.SetVideoDecoderName(m_name, m_hardware);
  m_processInfo.SetVideoDeintMethod("none");
  m_processInfo.SetVideoDimensions(hints.width, hints.height);
  CLog::Log(LOGINFO,
            "CDVDVideoCodecWebCodecs::Open - Using WebCodecs for video decoding: {} ({}x{}, "
            "annexB={}, {} decoder, frames {})",
            m_codecString, hints.width, hints.height, m_annexB,
            m_hardware ? "hardware" : "browser-chosen",
            m_textureUpload ? "imported as textures" : "copied through the heap");
  return true;
}

bool CDVDVideoCodecWebCodecs::AddData(const DemuxPacket& packet)
{
  if (m_fallback)
    return m_fallback->AddData(packet);

  if (!m_opened || m_decoderHandle == INVALID_DECODER_HANDLE)
    return false;

  if (packet.iSize <= 0 || packet.pData == nullptr)
    return true;

  const bool isKeyFrame = WEBCODECS::PacketIsKeyFrame(m_hints, packet, m_nalLengthSize);

  // Feeding a delta before the first IDR would permanently fail the decoder.
  if (m_waitingForKeyFrame && !isKeyFrame)
    return true;

  // Backpressure: decoder is saturated, let VideoPlayer re-queue this packet.
  if (DecoderBusy())
    return false;

  double pts = m_hints.ptsinvalid ? DVD_NOPTS_VALUE : packet.pts;
  if (pts == DVD_NOPTS_VALUE)
    pts = packet.dts;
  double ptsSeconds = std::numeric_limits<double>::quiet_NaN();
  if (pts != DVD_NOPTS_VALUE && std::isfinite(pts))
    ptsSeconds = pts / static_cast<double>(DVD_TIME_BASE);

  double durationSeconds = 0.0;
  if (packet.duration > 0.0 && std::isfinite(packet.duration))
    durationSeconds = packet.duration / static_cast<double>(DVD_TIME_BASE);

  // The push runs on the main thread after this call has returned, so it gets
  // its own copy of the packet and frees it.
  const int extradataSize = m_waitingForKeyFrame && m_prependExtradata
                                ? static_cast<int>(m_hints.extradata.GetSize())
                                : 0;
  const int size = extradataSize + packet.iSize;
  auto* data = static_cast<uint8_t*>(std::malloc(size));
  if (!data)
    return false;
  if (extradataSize > 0)
    std::memcpy(data, m_hints.extradata.GetData(), extradataSize);
  std::memcpy(data + extradataSize, packet.pData, packet.iSize);
  ++m_pushCount;
  webcodecs_push_packet(m_decoderHandle, data, size, isKeyFrame ? 1 : 0, ptsSeconds,
                        durationSeconds);

  m_drained = false;
  if (isKeyFrame)
    m_waitingForKeyFrame = false;
  return true;
}

void CDVDVideoCodecWebCodecs::Reset()
{
  if (m_fallback)
  {
    m_fallback->Reset();
    return;
  }

  if (m_decoderHandle == INVALID_DECODER_HANDLE)
    return;

  ReleaseCopyBuffer();
  webcodecs_reset_decoder(m_decoderHandle);
  // The reset closed the frames not taken yet; sequence numbers keep counting.
  PublishFramesTaken(SharedLoad(m_shared.framesProduced));
  m_waitingForKeyFrame = true;
  m_drained = false;
  m_lastLoggedDroppedFrames = 0;
  m_highWaterMark = 0;
  CLog::Log(LOGDEBUG, "CDVDVideoCodecWebCodecs::Reset - decoder reset after seek/flush");
}

void CDVDVideoCodecWebCodecs::SetCodecControl(int flags)
{
  m_codecControlFlags = flags;
  if (m_fallback)
    m_fallback->SetCodecControl(flags);
}

unsigned CDVDVideoCodecWebCodecs::GetConvergeCount()
{
  return m_fallback ? m_fallback->GetConvergeCount() : CDVDVideoCodec::GetConvergeCount();
}

bool CDVDVideoCodecWebCodecs::GetCodecStats(double& pts, int& droppedFrames, int& skippedPics)
{
  return m_fallback ? m_fallback->GetCodecStats(pts, droppedFrames, skippedPics)
                    : CDVDVideoCodec::GetCodecStats(pts, droppedFrames, skippedPics);
}

void CDVDVideoCodecWebCodecs::Reopen()
{
  if (m_fallback)
    m_fallback->Reopen();
}

bool CDVDVideoCodecWebCodecs::FallBackToFFmpeg()
{
  if (m_fallbackTried)
    return false;
  m_fallbackTried = true;

  auto codec = std::make_unique<CDVDVideoCodecFFmpeg>(m_processInfo);
  if (!codec->Open(m_hints, m_options))
  {
    CLog::Log(LOGERROR, "CDVDVideoCodecWebCodecs::FallBackToFFmpeg - FFmpeg cannot decode {}",
              m_codecString);
    return false;
  }

  CLog::Log(LOGWARNING, "CDVDVideoCodecWebCodecs::FallBackToFFmpeg - decoding {} with FFmpeg",
            m_codecString);
  Dispose();
  m_fallback = std::move(codec);
  return true;
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

int32_t CDVDVideoCodecWebCodecs::QueuedFrames() const
{
  return SharedLoad(m_shared.framesProduced) - SharedLoad(m_shared.framesTaken);
}

// Once published, the output callback may reuse the ring slot of every earlier
// sequence number, so callers copy the slot out first.
void CDVDVideoCodecWebCodecs::PublishFramesTaken(int32_t sequence)
{
  __atomic_store_n(&m_shared.framesTaken, sequence, __ATOMIC_RELEASE);
}

// Pushes the main thread has not run yet are invisible to the shared counters
// and have to be counted here. Every chunk the decoder holds and every frame
// it has output but nobody has closed occupies one of its buffers.
bool CDVDVideoCodecWebCodecs::DecoderBusy() const
{
  const int32_t pendingPushes = m_pushCount - SharedLoad(m_shared.pushesProcessed);
  return pendingPushes + SharedLoad(m_shared.decoding) + SharedLoad(m_shared.openFrames) >=
         WEBCODECS_MAX_INFLIGHT;
}

// Callers sample `seenSignal` before checking the shared state, so a change that
// lands in between makes the futex wait return immediately.
void CDVDVideoCodecWebCodecs::WaitForDecoderSignal(uint32_t seenSignal, double maxWaitMs)
{
  emscripten_futex_wait(&m_shared.signal, seenSignal, maxWaitMs);
}

// Draining never calls VideoDecoder.flush(): a flushed decoder demands a key
// chunk, and VideoPlayerVideo drains on every still frame and stalled stream, so
// playback would freeze until the next IDR each time. Instead wait until nothing
// is pending and the decoder has stayed silent for DRAIN_SETTLE_TIME. Frames a
// decoder holds back until it sees more input are lost at end of stream.
// Returns true once the drain is over, false while frames are still arriving.
bool CDVDVideoCodecWebCodecs::WaitForDrain()
{
  const auto deadline = std::chrono::steady_clock::now() + DRAIN_TIMEOUT;
  std::optional<std::chrono::steady_clock::time_point> settleDeadline;
  while (true)
  {
    const auto seen = static_cast<uint32_t>(SharedLoad(m_shared.signal));
    if (QueuedFrames() > 0 || SharedLoad(m_shared.failed))
      return false;

    const auto now = std::chrono::steady_clock::now();
    const int32_t pendingPushes = m_pushCount - SharedLoad(m_shared.pushesProcessed);
    const int32_t inflight = SharedLoad(m_shared.inflight) + pendingPushes;
    if (now >= deadline)
    {
      PollDecoderStats();
      CLog::Log(LOGWARNING,
                "CDVDVideoCodecWebCodecs::WaitForDrain - drain timed out with {} pending "
                "WebCodecs operations",
                inflight);
      return true;
    }

    if (inflight > 0)
      settleDeadline.reset();
    else if (!settleDeadline)
      settleDeadline = now + DRAIN_SETTLE_TIME;
    else if (now >= *settleDeadline)
    {
      CLog::Log(LOGDEBUG, "CDVDVideoCodecWebCodecs::WaitForDrain - drain completed");
      return true;
    }

    WaitForDecoderSignal(seen, FRAME_WAIT_MS);
  }
}

// Returns the WebCodecsCopyResult of request copyId, or 0 if it has not
// finished within COPY_TIMEOUT.
int32_t CDVDVideoCodecWebCodecs::WaitForCopy(int copyId)
{
  const auto deadline = std::chrono::steady_clock::now() + COPY_TIMEOUT;
  while (true)
  {
    const auto seen = static_cast<uint32_t>(SharedLoad(m_shared.signal));
    if (SharedLoad(m_shared.copyDone) == copyId)
      return SharedLoad(m_shared.copyResult);
    if (std::chrono::steady_clock::now() >= deadline)
      return 0;
    WaitForDecoderSignal(seen, FRAME_WAIT_MS);
  }
}

void CDVDVideoCodecWebCodecs::ReleaseCopyBuffer()
{
  if (!m_copyBuffer)
    return;
  // copyTo() cannot be cancelled, so a buffer it may still write into is
  // never returned to its pool.
  if (WaitForCopy(m_copyId) == 0)
    CLog::Log(LOGWARNING,
              "CDVDVideoCodecWebCodecs::ReleaseCopyBuffer - abandoning the buffer of "
              "unfinished copy {}",
              m_copyId);
  else
    m_copyBuffer->Release();
  m_copyBuffer = nullptr;
}

// A pool's free buffers keep the format and size they were allocated with, so
// a change gets a new pool. Buffers still in use keep the old one alive.
CVideoBuffer* CDVDVideoCodecWebCodecs::AcquirePictureBuffer(
    std::shared_ptr<CVideoBufferPoolSysMem>& pool, AVPixelFormat pixelFormat, int bufferSize)
{
  const int size = AlignUp(bufferSize, DEFAULT_ALIGNMENT);
  if (!pool || !pool->IsCompatible(pixelFormat, size))
  {
    pool = std::make_shared<CVideoBufferPoolSysMem>();
    pool->Configure(pixelFormat, size);
  }
  CVideoBuffer* buffer = pool->Get();
  if (buffer && !buffer->GetMemPtr())
  {
    buffer->Release();
    return nullptr;
  }
  return buffer;
}

// The renderer only takes planar YUV. The colour matrix and range used here are
// the ones FillPictureMetadata() reports, so the renderer undoes this exactly.
// Releases rgbBuffer and rewrites info to describe the YUV420P result.
CVideoBuffer* CDVDVideoCodecWebCodecs::ConvertToYuv420p(CVideoBuffer* rgbBuffer,
                                                        WebCodecsFrameInfo& info)
{
  const AVPixelFormat srcFormat = PixelFormatFromWebCodecs(info.pixelFormat);
  const int width = info.width;
  const int height = info.height;
  const Yuv420pLayout layout = Yuv420pLayoutFor(width, height);

  if (!m_loggedRgbConversion)
  {
    m_loggedRgbConversion = true;
    CLog::Log(LOGINFO,
              "CDVDVideoCodecWebCodecs::GetPicture - decoder outputs {} frames, converting to "
              "YUV420P in software",
              av_get_pix_fmt_name(srcFormat));
  }

  // sws_getCachedContext() would rebuild the context every frame, because the
  // range set by sws_setColorspaceDetails() differs from what it derives from
  // the formats.
  if (!m_swsContext || width != m_swsWidth || height != m_swsHeight || srcFormat != m_swsFormat)
  {
    sws_freeContext(m_swsContext);
    m_swsContext = sws_getContext(width, height, srcFormat, width, height, AV_PIX_FMT_YUV420P,
                                  SWS_POINT, nullptr, nullptr, nullptr);
    if (!m_swsContext)
    {
      rgbBuffer->Release();
      CLog::Log(LOGERROR,
                "CDVDVideoCodecWebCodecs::GetPicture - cannot create swscale context for {}x{} {}",
                width, height, av_get_pix_fmt_name(srcFormat));
      return nullptr;
    }

    const int fullRange = m_hints.colorRange == AVCOL_RANGE_JPEG ? 1 : 0;
    sws_setColorspaceDetails(m_swsContext, sws_getCoefficients(SWS_CS_DEFAULT), 1,
                             sws_getCoefficients(SwsColorspace(m_hints.colorSpace)), fullRange, 0,
                             1 << 16, 1 << 16);
    m_swsWidth = width;
    m_swsHeight = height;
    m_swsFormat = srcFormat;
  }

  CVideoBuffer* yuvBuffer =
      AcquirePictureBuffer(m_videoBufferPool, AV_PIX_FMT_YUV420P, layout.Size());
  if (!yuvBuffer)
  {
    rgbBuffer->Release();
    return nullptr;
  }

  uint8_t* dst = yuvBuffer->GetMemPtr();
  // sws_scale() reads four entries from each array, whatever the format.
  const uint8_t* src[4] = {rgbBuffer->GetMemPtr(), nullptr, nullptr, nullptr};
  const int srcStride[4] = {info.yStride, 0, 0, 0};
  uint8_t* dstPlanes[4] = {dst, dst + layout.ySize, dst + layout.ySize + layout.uvSize, nullptr};
  const int dstStride[4] = {layout.yStride, layout.uvStride, layout.uvStride, 0};
  sws_scale(m_swsContext, src, srcStride, 0, height, dstPlanes, dstStride);
  rgbBuffer->Release();

  info.pixelFormat = WEBCODECS_PIXFMT_I420;
  info.yStride = layout.yStride;
  info.uStride = layout.uvStride;
  info.vStride = layout.uvStride;
  info.uOffset = layout.ySize;
  info.vOffset = layout.ySize + layout.uvSize;
  info.payloadSize = layout.Size();
  return yuvBuffer;
}

// The colour fields come from the stream hints, as for every codec, unless
// colourFromFrame asks for the frame's own colour space; the fields the frame
// leaves unspecified fall back to the hints either way. The sysmem path keeps
// the hints because its RGB to YUV conversion was made with them.
void CDVDVideoCodecWebCodecs::FillPictureMetadata(VideoPicture* pVideoPicture,
                                                  CVideoBuffer* videoBuffer,
                                                  AVPixelFormat pixelFormat,
                                                  const WebCodecsFrameInfo& info,
                                                  bool colourFromFrame) const
{
  const int width = info.width;
  const int height = info.height;

  pVideoPicture->Reset();
  pVideoPicture->videoBuffer = videoBuffer;
  pVideoPicture->pixelFormat = pixelFormat;
  pVideoPicture->iWidth = width;
  pVideoPicture->iHeight = height;

  // The container's aspect wins; without one the frame's own display size
  // carries the decoder's sample aspect ratio.
  double aspect = height > 0 ? static_cast<double>(width) / height : 1.0;
  if (m_hints.aspect > 0.0)
    aspect = m_hints.aspect;
  else if (info.displayWidth > 0 && info.displayHeight > 0)
    aspect = static_cast<double>(info.displayWidth) / info.displayHeight;
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
  pVideoPicture->pts = !m_hints.ptsinvalid && std::isfinite(info.ptsSeconds)
                           ? info.ptsSeconds * DVD_TIME_BASE
                           : DVD_NOPTS_VALUE;
  pVideoPicture->dts = DVD_NOPTS_VALUE;
  pVideoPicture->iDuration =
      info.durationSeconds > 0.0 ? info.durationSeconds * DVD_TIME_BASE : 0.0;
  pVideoPicture->iRepeatPicture = 0.0;
  pVideoPicture->iFlags = 0;
  pVideoPicture->iFrameType = info.keyFrame != 0 ? FRAME_TYPE_I : FRAME_TYPE_P;

  pVideoPicture->color_space =
      m_hints.colorSpace == AVCOL_SPC_UNSPECIFIED ? AVCOL_SPC_BT709 : m_hints.colorSpace;
  pVideoPicture->color_primaries =
      m_hints.colorPrimaries == AVCOL_PRI_UNSPECIFIED ? AVCOL_PRI_BT709 : m_hints.colorPrimaries;
  pVideoPicture->color_transfer = m_hints.colorTransferCharacteristic == AVCOL_TRC_UNSPECIFIED
                                      ? AVCOL_TRC_BT709
                                      : m_hints.colorTransferCharacteristic;
  pVideoPicture->color_range = m_hints.colorRange == AVCOL_RANGE_JPEG;
  pVideoPicture->colorBits = PICTURE_COLOR_BITS;

  if (colourFromFrame)
  {
    if (info.colorMatrix != AVCOL_SPC_UNSPECIFIED)
      pVideoPicture->color_space = static_cast<AVColorSpace>(info.colorMatrix);
    if (info.colorPrimaries != AVCOL_PRI_UNSPECIFIED)
      pVideoPicture->color_primaries = static_cast<AVColorPrimaries>(info.colorPrimaries);
    if (info.colorTransfer != AVCOL_TRC_UNSPECIFIED)
      pVideoPicture->color_transfer =
          static_cast<AVColorTransferCharacteristic>(info.colorTransfer);
    if (info.fullRange >= 0)
      pVideoPicture->color_range = info.fullRange == 1;
    const WebCodecsPixelFormatInfo* format = FindPixelFormat(info.pixelFormat);
    if (format && format->bitDepth > 0)
      pVideoPicture->colorBits = format->bitDepth;
    else if (m_hints.bitdepth > 0)
      pVideoPicture->colorBits = m_hints.bitdepth;
  }

  pVideoPicture->m_originalColorPrimaries = pVideoPicture->color_primaries;
  pVideoPicture->chroma_position = AVCHROMA_LOC_LEFT;

  pVideoPicture->hdrType = m_hints.hdrType;
  if (m_hints.masteringMetadata)
  {
    pVideoPicture->displayMetadata = *m_hints.masteringMetadata;
    pVideoPicture->hasDisplayMetadata = true;
  }
  if (m_hints.contentLightMetadata)
  {
    pVideoPicture->lightMetadata = *m_hints.contentLightMetadata;
    pVideoPicture->hasLightMetadata = true;
  }
}

void CDVDVideoCodecWebCodecs::ReportPixelFormat(const char* name)
{
  if (!name)
    name = "";
  if (m_reportedPixelFormat == name)
    return;
  m_reportedPixelFormat = name;
  m_processInfo.SetVideoPixelFormat(m_reportedPixelFormat);
}

// A picture flagged DVP_FLAG_DROPPED is never rendered, but VideoPlayerVideo
// still configures the renderer from it, so it carries a buffer of the format the
// displayed pictures have.
CDVDVideoCodec::VCReturn CDVDVideoCodecWebCodecs::DroppedPicture(const WebCodecsFrameInfo& info,
                                                                 AVPixelFormat pixelFormat,
                                                                 VideoPicture* pVideoPicture)
{
  int bufferSize = info.payloadSize;
  if (IsPackedRgb(pixelFormat))
  {
    pixelFormat = AV_PIX_FMT_YUV420P;
    bufferSize = Yuv420pLayoutFor(info.width, info.height).Size();
  }

  CVideoBuffer* videoBuffer = AcquirePictureBuffer(m_videoBufferPool, pixelFormat, bufferSize);
  if (!videoBuffer)
    return VC_NOBUFFER;
  videoBuffer->SetPixelFormat(pixelFormat);

  FillPictureMetadata(pVideoPicture, videoBuffer, pixelFormat, info, false);
  pVideoPicture->iFlags |= DVP_FLAG_DROPPED;
  return VC_PICTURE;
}

// Zero-copy path: the picture only names the frame. CRendererWebCodecs
// imports it into a texture on the main thread and releasing the buffer
// closes it, so a dropped picture releases it here and is never rendered.
CDVDVideoCodec::VCReturn CDVDVideoCodecWebCodecs::TakeFrame(int32_t sequence,
                                                            const WebCodecsFrameInfo& info,
                                                            VideoPicture* pVideoPicture)
{
  auto* videoBuffer = static_cast<CVideoBufferWebCodecs*>(m_framePool->Get());
  videoBuffer->SetFrame(m_decoderHandle, sequence, PixelFormatFromWebCodecs(info.pixelFormat));
  PublishFramesTaken(sequence + 1);
  ReportPixelFormat(WebCodecsPixelFormatName(info.pixelFormat));

  const bool drop = (m_codecControlFlags & (DVD_CODEC_CTRL_DROP | DVD_CODEC_CTRL_DROP_ANY)) != 0;
  if (drop)
    webcodecs_release_frame(m_decoderHandle, sequence);

  FillPictureMetadata(pVideoPicture, videoBuffer, videoBuffer->GetFormat(), info, true);
  if (drop)
    pVideoPicture->iFlags |= DVP_FLAG_DROPPED;
  return VC_PICTURE;
}

// Sysmem path: the frame is copied into a CVideoBuffer, converted to YUV420P
// when the decoder emits packed RGB, and handed to the generic renderer.
CDVDVideoCodec::VCReturn CDVDVideoCodecWebCodecs::CopyFrame(int32_t sequence,
                                                            const WebCodecsFrameInfo& info,
                                                            VideoPicture* pVideoPicture)
{
  // A copy an earlier call gave up on has to settle before another starts.
  ReleaseCopyBuffer();

  AVPixelFormat pixelFormat = PixelFormatFromWebCodecs(info.pixelFormat);
  if (pixelFormat == AV_PIX_FMT_NONE || info.payloadSize <= 0)
  {
    webcodecs_release_frame(m_decoderHandle, sequence);
    PublishFramesTaken(sequence + 1);
    CLog::Log(LOGERROR,
              "CDVDVideoCodecWebCodecs::GetPicture - {} frames cannot be copied through the heap",
              WebCodecsPixelFormatName(info.pixelFormat));
    return FallBackToFFmpeg() ? VC_FLUSHED : VC_ERROR;
  }
  ReportPixelFormat(av_get_pix_fmt_name(pixelFormat));

  if (m_codecControlFlags & (DVD_CODEC_CTRL_DROP | DVD_CODEC_CTRL_DROP_ANY))
  {
    webcodecs_release_frame(m_decoderHandle, sequence);
    PublishFramesTaken(sequence + 1);
    return DroppedPicture(info, pixelFormat, pVideoPicture);
  }

  const bool packedRgb = IsPackedRgb(pixelFormat);
  CVideoBuffer* videoBuffer = AcquirePictureBuffer(packedRgb ? m_rgbBufferPool : m_videoBufferPool,
                                                   pixelFormat, info.payloadSize);
  if (!videoBuffer)
    return VC_NOBUFFER;

  m_copyBuffer = videoBuffer;
  webcodecs_copy_frame(m_decoderHandle, sequence, ++m_copyId, videoBuffer->GetMemPtr(),
                       info.payloadSize);
  PublishFramesTaken(sequence + 1);
  const int32_t copyResult = WaitForCopy(m_copyId);
  if (copyResult == 0)
  {
    CLog::Log(LOGERROR,
              "CDVDVideoCodecWebCodecs::GetPicture - frame copy did not complete within {} ms",
              COPY_TIMEOUT.count());
    return VC_ERROR;
  }
  m_copyBuffer = nullptr;

  if (copyResult != WEBCODECS_COPY_OK)
  {
    videoBuffer->Release();
    if (copyResult == WEBCODECS_COPY_NO_FRAME)
      return VC_BUFFER;

    const std::string error = ReadDecoderError(m_decoderHandle);
    CLog::Log(LOGERROR,
              "CDVDVideoCodecWebCodecs::GetPicture - frame copy failed (result={}, "
              "payloadSize={}): {}",
              copyResult, info.payloadSize, error.empty() ? "<no js error>" : error);
    return VC_ERROR;
  }

  WebCodecsFrameInfo layout = info;
  if (packedRgb)
  {
    videoBuffer = ConvertToYuv420p(videoBuffer, layout);
    if (!videoBuffer)
      return VC_ERROR;
    pixelFormat = AV_PIX_FMT_YUV420P;
  }

  const int strides[YuvImage::MAX_PLANES] = {layout.yStride, layout.uStride, layout.vStride};
  const int planeOffsets[YuvImage::MAX_PLANES] = {0, layout.uOffset, layout.vOffset};
  videoBuffer->SetPixelFormat(pixelFormat);
  videoBuffer->SetDimensions(info.width, info.height, strides, planeOffsets);

  FillPictureMetadata(pVideoPicture, videoBuffer, pixelFormat, info, false);
  return VC_PICTURE;
}

CDVDVideoCodec::VCReturn CDVDVideoCodecWebCodecs::GetPicture(VideoPicture* pVideoPicture)
{
  if (m_fallback)
    return m_fallback->GetPicture(pVideoPicture);

  if (!m_opened || m_decoderHandle == INVALID_DECODER_HANDLE)
    return VC_ERROR;

  const bool draining = (m_codecControlFlags & DVD_CODEC_CTRL_DRAIN) != 0;
  if (draining)
  {
    if (!m_drained)
      m_drained = WaitForDrain();
  }
  else
  {
    m_drained = false;
    const auto seen = static_cast<uint32_t>(SharedLoad(m_shared.signal));
    if (QueuedFrames() == 0 && DecoderBusy())
      WaitForDecoderSignal(seen, FRAME_WAIT_MS);
  }

  if (SharedLoad(m_shared.failed))
  {
    const std::string error = ReadDecoderError(m_decoderHandle);
    CLog::Log(LOGERROR, "CDVDVideoCodecWebCodecs::GetPicture - decoder entered failed state: {}",
              error.empty() ? "<no js error>" : error);
    // VideoPlayer keeps feeding a codec that returns VC_ERROR, so replace the
    // decoder and resume at the next key frame. When it fails again and again
    // before producing FRAMES_TO_RECOVER frames, FFmpeg decodes the rest of the
    // stream.
    if (m_recoveries < MAX_DECODER_RECOVERIES)
    {
      ++m_recoveries;
      m_framesSinceRecovery = 0;
      Reset();
      return VC_BUFFER;
    }
    return FallBackToFFmpeg() ? VC_FLUSHED : VC_ERROR;
  }

  if (QueuedFrames() <= 0)
    return draining ? VC_EOF : VC_BUFFER;

  if (m_recoveries > 0 && ++m_framesSinceRecovery >= FRAMES_TO_RECOVER)
    m_recoveries = 0;

  // The slot is complete: the bridge writes it before it publishes framesProduced.
  const int32_t sequence = SharedLoad(m_shared.framesTaken);
  const WebCodecsFrameInfo info = m_shared.ring[sequence % WEBCODECS_FRAME_RING];
  return m_textureUpload ? TakeFrame(sequence, info, pVideoPicture)
                         : CopyFrame(sequence, info, pVideoPicture);
}
