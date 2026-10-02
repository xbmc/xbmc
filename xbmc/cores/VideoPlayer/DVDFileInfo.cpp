/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DVDFileInfo.h"

#include "DVDInputStreams/DVDInputStream.h"
#include "DVDStreamInfo.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "VideoDecodeSession.h"
#include "filesystem/StackDirectory.h"
#include "guilib/Texture.h"
#include "messaging/ApplicationMessenger.h"
#include "network/NetworkFileItemClassify.h"
#include "playlists/PlayListFileItemClassify.h"
#include "pvr/utils/PVRStreamUtils.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/Thread.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "video/VideoInfoTag.h"
#ifdef HAVE_LIBBLURAY
#include "DVDInputStreams/DVDInputStreamBluray.h"
#endif
#include "DVDCodecs/DVDFactoryCodec.h"
#include "DVDCodecs/Video/DVDVideoCodec.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDDemuxers/DVDDemuxVobsub.h"
#include "DVDDemuxers/DVDFactoryDemuxer.h"
#include "DVDInputStreams/DVDFactoryInputStream.h"
#include "Process/ProcessInfo.h"
#include "Util.h"
#include "filesystem/File.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <unordered_map>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mastering_display_metadata.h>
#include <libswscale/swscale.h>
}

using namespace KODI;

namespace
{
constexpr std::chrono::seconds PROBE_TIMEOUT{30};
constexpr std::chrono::seconds PROCESS_THREAD_TIMEOUT{5};
constexpr std::chrono::milliseconds CANCEL_POLL{100};
constexpr std::chrono::seconds SHUTDOWN_WAIT{5};
constexpr int MAX_PROBE_WORKERS = 8;

struct ProbeState
{
  bool finished{false};
  bool abandoned{false};
};

struct ProbeRegistry
{
  std::mutex mutex;
  std::condition_variable idle;
  std::unordered_map<std::string, int> stuck;
  int live{0};
  bool stopping{false};
};

ProbeRegistry& Registry()
{
  static auto* registry{new ProbeRegistry};
  return *registry;
}

enum class Reservation
{
  GRANTED,
  EXHAUSTED,
  STUCK,
};

Reservation ReserveWorker(const std::string& path)
{
  auto& registry{Registry()};
  std::unique_lock lock(registry.mutex);
  if (registry.stopping || registry.live >= MAX_PROBE_WORKERS)
    return Reservation::EXHAUSTED;
  if (registry.stuck.contains(path))
    return Reservation::STUCK;
  ++registry.live;
  return Reservation::GRANTED;
}

bool AbandonWorker(const std::string& path, ProbeState& state)
{
  auto& registry{Registry()};
  std::unique_lock lock(registry.mutex);
  if (state.finished)
    return false;
  state.abandoned = true;
  ++registry.stuck[path];
  return true;
}

void ReleaseWorker(const std::string& path, ProbeState& state)
{
  auto& registry{Registry()};
  {
    std::unique_lock lock(registry.mutex);
    state.finished = true;
    if (state.abandoned)
    {
      const auto it{registry.stuck.find(path)};
      if (--it->second == 0)
        registry.stuck.erase(it);
    }
    --registry.live;
  }
  registry.idle.notify_all();
}

std::chrono::seconds GetProbeTimeout()
{
  const auto messenger{CServiceBroker::GetAppMessenger()};
  return messenger && messenger->IsProcessThread() ? PROCESS_THREAD_TIMEOUT : PROBE_TIMEOUT;
}

bool IsStopping()
{
  auto& registry{Registry()};
  std::unique_lock lock(registry.mutex);
  return registry.stopping;
}

template<typename Fn>
auto RunWithTimeout(std::string_view what,
                    const std::string& path,
                    Fn&& fn) -> std::optional<std::invoke_result_t<std::decay_t<Fn>>>
{
  using Result = std::invoke_result_t<std::decay_t<Fn>>;
  using Promise = std::promise<std::optional<Result>>;

  switch (ReserveWorker(path))
  {
    case Reservation::GRANTED:
      break;
    case Reservation::STUCK:
      CLog::LogF(LOGDEBUG, "{}: an earlier probe of {} is still blocked, skipping", what,
                 CURL::GetRedacted(path));
      return std::nullopt;
    case Reservation::EXHAUSTED:
      CLog::LogF(LOGWARNING, "{}: probe workers exhausted or shutting down, skipping {}", what,
                 CURL::GetRedacted(path));
      return std::nullopt;
  }

  auto state{std::make_shared<ProbeState>()};
  Promise promise;
  std::future<std::optional<Result>> future = promise.get_future();

  try
  {
    std::thread(
        [promise = std::move(promise), fn = std::forward<Fn>(fn), path, state]() mutable
        {
          {
            auto work{std::move(fn)};
            try
            {
              if (IsStopping())
                promise.set_value(std::nullopt);
              else
                promise.set_value(work());
            }
            catch (...)
            {
              promise.set_exception(std::current_exception());
            }
          }
          ReleaseWorker(path, *state);
        })
        .detach();
  }
  catch (const std::system_error& e)
  {
    ReleaseWorker(path, *state);
    CLog::LogF(LOGERROR, "{}: unable to start worker for {}: {}", what, CURL::GetRedacted(path),
               e.what());
    return std::nullopt;
  }
  catch (...)
  {
    ReleaseWorker(path, *state);
    throw;
  }

  const auto timeout{GetProbeTimeout()};
  const auto deadline{std::chrono::steady_clock::now() + timeout};
  while (future.wait_until(std::min(deadline, std::chrono::steady_clock::now() + CANCEL_POLL)) !=
         std::future_status::ready)
  {
    const CThread* thread{CThread::GetCurrentThread()};
    const bool stopRequested{thread && thread->IsStopRequested()};
    const bool expired{std::chrono::steady_clock::now() >= deadline};
    if (!stopRequested && !expired)
      continue;

    if (!AbandonWorker(path, *state))
      break;

    if (expired)
    {
      CLog::LogF(LOGWARNING, "{}: no result after {}s, giving up on {}", what, timeout.count(),
                 CURL::GetRedacted(path));
    }
    else
    {
      CLog::LogF(LOGDEBUG, "{}: caller is stopping, giving up on {}", what,
                 CURL::GetRedacted(path));
    }
    return std::nullopt;
  }
  return future.get();
}
} // namespace

void CDVDFileInfo::StopProbes()
{
  auto& registry{Registry()};
  std::unique_lock lock(registry.mutex);
  registry.stopping = true;
  if (!registry.idle.wait_for(lock, SHUTDOWN_WAIT, [&registry] { return registry.live == 0; }))
  {
    CLog::LogF(LOGWARNING, "{} probe workers still blocked in I/O, continuing shutdown",
               registry.live);
  }
}

bool CDVDFileInfo::ProbeFileDuration(const std::string& path, int& duration)
{
  std::unique_ptr<CDVDDemux> demux;

  CFileItem item(path, false);
  auto input = CDVDFactoryInputStream::CreateInputStream(NULL, item);
  if (!input)
    return false;

  // A DVD can only be read through a navigator driven by a player, and the title that would
  // give a meaningful duration is not known here anyway
  if (input->IsStreamType(DVDSTREAM_TYPE_DVD) || !input->Open())
    return false;

  demux.reset(CDVDFactoryDemuxer::CreateDemuxer(input, true));
  if (!demux)
    return false;

  duration = demux->GetStreamLength();
  if (duration > 0)
    return true;
  else
    return false;
}

bool CDVDFileInfo::GetFileDuration(const std::string& path, int& duration)
{
  const auto result = RunWithTimeout("GetFileDuration", path,
                                     [path]
                                     {
                                       int length{0};
                                       const bool ok{ProbeFileDuration(path, length)};
                                       return std::pair{ok, length};
                                     });
  if (!result || !result->first)
    return false;

  duration = result->second;
  return true;
}

std::unique_ptr<CTexture> CDVDFileInfo::ExtractThumbToTexture(const CFileItem& fileItem,
                                                              int chapterNumber)
{
  if (!CanExtract(fileItem))
    return {};

  auto result = RunWithTimeout("ExtractThumbToTexture", fileItem.GetPath(),
                               [item = CFileItem(fileItem), chapterNumber]
                               { return ProbeThumbToTexture(item, chapterNumber); });
  return result ? std::move(*result) : nullptr;
}

bool CDVDFileInfo::GetFileStreamDetails(CFileItem* pItem)
{
  if (!pItem || !CanExtract(*pItem))
    return false;

  auto work = std::make_shared<CFileItem>(*pItem);
  const auto ok = RunWithTimeout("GetFileStreamDetails", pItem->GetPath(),
                                 [work] { return ProbeFileStreamDetails(work.get()); });
  if (!ok)
    return false;

  if (work->HasVideoInfoTag())
    pItem->GetVideoInfoTag()->m_streamDetails = work->GetVideoInfoTag()->m_streamDetails;
  return *ok;
}

int DegreeToOrientation(int degrees)
{
  switch(degrees)
  {
    case 90:
      return 5;
    case 180:
      return 2;
    case 270:
      return 7;
    default:
      return 0;
  }
}

namespace
{
//! Convert a decoded picture to a BGRA texture sized for the thumbnail cache.
std::unique_ptr<CTexture> PictureToTexture(const VideoPicture& picture, const CDVDStreamInfo& hint)
{
  const unsigned int nWidth =
      std::min(picture.iDisplayWidth,
               CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_imageRes);
  double aspect =
      static_cast<double>(picture.iDisplayWidth) / static_cast<double>(picture.iDisplayHeight);
  if (hint.forced_aspect && hint.aspect != 0)
    aspect = hint.aspect;
  const auto nHeight = static_cast<unsigned int>(static_cast<double>(nWidth) / aspect);

  std::unique_ptr<CTexture> result = CTexture::CreateTexture(nWidth, nHeight);
  result->SetAlpha(false);
  result->SetOrientation(DegreeToOrientation(hint.orientation));

  uint8_t* planes[YuvImage::MAX_PLANES];
  int stride[YuvImage::MAX_PLANES];
  picture.videoBuffer->GetPlanes(planes);
  picture.videoBuffer->GetStrides(stride);

  bool converted = false;

#if LIBSWSCALE_BUILD >= AV_VERSION_INT(9, 0, 100)
  {
    AVFrame* srcFrame = av_frame_alloc();
    AVFrame* dstFrame = av_frame_alloc();
    SwsContext* sws = sws_alloc_context();
    if (srcFrame && dstFrame && sws)
    {
      srcFrame->width = static_cast<int>(picture.iWidth);
      srcFrame->height = static_cast<int>(picture.iHeight);
      srcFrame->format = picture.videoBuffer->GetFormat();
      for (int i = 0; i < YuvImage::MAX_PLANES; i++)
      {
        srcFrame->data[i] = planes[i];
        srcFrame->linesize[i] = stride[i];
      }
      srcFrame->colorspace = picture.color_space;
      srcFrame->color_range = picture.color_range == 1 ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;
      srcFrame->color_primaries = picture.color_primaries;
      srcFrame->color_trc = picture.color_transfer;
      srcFrame->chroma_location = picture.chroma_position;

      if (picture.hasDisplayMetadata)
      {
        AVMasteringDisplayMetadata* mdm = av_mastering_display_metadata_create_side_data(srcFrame);
        if (mdm)
          *mdm = picture.displayMetadata;
      }
      if (picture.hasLightMetadata)
      {
        AVContentLightMetadata* clm = av_content_light_metadata_create_side_data(srcFrame);
        if (clm)
          *clm = picture.lightMetadata;
      }

      dstFrame->width = static_cast<int>(nWidth);
      dstFrame->height = static_cast<int>(nHeight);
      dstFrame->format = AV_PIX_FMT_BGRA;
      dstFrame->data[0] = result->GetPixels();
      dstFrame->linesize[0] = static_cast<int>(result->GetPitch());
      dstFrame->colorspace = AVCOL_SPC_RGB;
      dstFrame->color_range = AVCOL_RANGE_JPEG;
      dstFrame->color_primaries = AVCOL_PRI_BT709;
      dstFrame->color_trc = AVCOL_TRC_BT709;

      sws->flags = SWS_BILINEAR;
      sws->intent = SWS_INTENT_PERCEPTUAL;

      const int res = sws_scale_frame(sws, dstFrame, srcFrame);
      if (res < 0)
        CLog::LogF(LOGWARNING, "sws_scale_frame failed ({}), using legacy conversion", res);
      else
        converted = true;
    }
    sws_free_context(&sws);
    av_frame_free(&srcFrame);
    av_frame_free(&dstFrame);
  }
#endif

  if (!converted)
  {
    struct SwsContext* context =
        sws_getContext(picture.iWidth, picture.iHeight, picture.videoBuffer->GetFormat(), nWidth,
                       nHeight, AV_PIX_FMT_BGRA, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    if (context)
    {
      // dstRange is ignored for RGB destinations; the tables drive YUV->RGB
      sws_setColorspaceDetails(context, sws_getCoefficients(picture.color_space),
                               picture.color_range == 1 ? 1 : 0,
                               sws_getCoefficients(AVCOL_SPC_BT709), 1, 0, 1 << 16, 1 << 16);
      uint8_t* src[4] = {planes[0], planes[1], planes[2], 0};
      int srcStride[] = {stride[0], stride[1], stride[2], 0};
      uint8_t* dst[] = {result->GetPixels(), 0, 0, 0};
      int dstStride[] = {static_cast<int>(result->GetPitch()), 0, 0, 0};
      sws_scale(context, src, srcStride, 0, picture.iHeight, dst, dstStride);
      sws_freeContext(context);
      converted = true;
    }
  }

  if (!converted)
    result.reset();

  return result;
}

} // namespace

std::unique_ptr<CTexture> CDVDFileInfo::ProbeThumbToTexture(const CFileItem& fileItem,
                                                            int chapterNumber)
{
  if (!CanExtract(fileItem))
    return {};

  const std::string redactPath = CURL::GetRedacted(fileItem.GetPath());
  auto start = std::chrono::steady_clock::now();

  std::optional<VideoDecodeSession> session =
      OpenVideoDecodeSession(fileItem, CODEC_FORCE_SOFTWARE, redactPath);
  if (!session)
    return {};

  int packetsTried = 0;

  std::unique_ptr<CTexture> result{};
  // Thumbnail position: the chapter start, or one third in
  const int nTotalLen = session->demuxer->GetStreamLength();
  const bool seekToChapter = chapterNumber > 0 && session->demuxer->GetChapterCount() > 0;
  const int64_t nSeekTo =
      seekToChapter ? session->demuxer->GetChapterPos(chapterNumber).count() : nTotalLen / 3;

  VideoPicture picture = {};
  if (SeekAndDecodePictureAt(*session->demuxer, *session->codec, session->videoStream, nSeekTo,
                             redactPath, picture, packetsTried))
    result = PictureToTexture(picture, session->hint);
  else
    CLog::LogF(LOGDEBUG, "decode failed in {} after {} packets.", redactPath, packetsTried);

  auto end = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  CLog::LogF(LOGDEBUG, "measured {} ms to extract thumb from file <{}> in {} packets. ",
             duration.count(), redactPath, packetsTried);

  return result;
}

bool CDVDFileInfo::CanExtract(const CFileItem& fileItem)
{
  if (fileItem.IsFolder())
    return false;

  if ((URIUtils::IsPVR(fileItem.GetPath()) &&
       !PVR::UTILS::ProvidesStreamForMetaDataExtraction(fileItem)) ||
      // plugin path not fully resolved
      URIUtils::IsPlugin(fileItem.GetDynPath()) || URIUtils::IsUPnP(fileItem.GetPath()) ||
      NETWORK::IsInternetStream(fileItem) || VIDEO::IsDiscStub(fileItem) ||
      PLAYLIST::IsPlayList(fileItem))
    return false;

  // mostly can't extract from discs and files from discs.
  if (URIUtils::IsBlurayPath(fileItem.GetPath()) || VIDEO::IsBDFile(fileItem) || fileItem.IsDVD() ||
      fileItem.IsDiscImage() || VIDEO::IsDVDFile(fileItem, false, true))
    return false;

  // A resolved disc item keeps the disc file in its path while its dynpath is the bluray://
  // playlist, so the checks above (which look at one or the other) can miss it
  if (URIUtils::IsBlurayPath(fileItem.GetDynPath()) ||
      URIUtils::IsOpticalMediaFile(fileItem.GetPath()) || URIUtils::IsDiscImage(fileItem.GetPath()))
    return false;

  // ..nor from a stack still holding unresolved disc parts.
  // A stack of bluray:// playlists is extractable and DemuxerToStreamDetails() sums their durations
  if (URIUtils::IsDiscImageStack(fileItem.GetDynPath()))
    return false;

  // For HTTP/FTP we only allow extraction when on a LAN
  if (URIUtils::IsRemote(fileItem.GetPath()) && !URIUtils::IsOnLAN(fileItem.GetPath()) &&
      (URIUtils::IsFTP(fileItem.GetPath()) || URIUtils::IsHTTP(fileItem.GetPath())))
    return false;

  return true;
}

/**
 * \brief Open the item pointed to by pItem and extract streamdetails
 * \return true if the stream details have changed
 */
bool CDVDFileInfo::ProbeFileStreamDetails(CFileItem* pItem)
{
  if (!pItem)
    return false;

  if (!CanExtract(*pItem))
    return false;

  std::string strFileNameAndPath;
  if (pItem->HasVideoInfoTag())
    strFileNameAndPath = pItem->GetVideoInfoTag()->m_strFileNameAndPath;

  if (strFileNameAndPath.empty())
    strFileNameAndPath = pItem->GetDynPath();

  std::string playablePath = strFileNameAndPath;
  if (URIUtils::IsStack(playablePath))
    playablePath = XFILE::CStackDirectory::GetFirstStackedFile(playablePath);

  CFileItem item(playablePath, false);
  item.SetMimeTypeForInternetFile();
  auto pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, item);
  if (!pInputStream)
    return false;

  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_DVD) || !pInputStream->Open())
  {
    return false;
  }

  CDVDDemux* pDemuxer = CDVDFactoryDemuxer::CreateDemuxer(pInputStream, true);
  if (pDemuxer)
  {
    bool retVal = DemuxerToStreamDetails(
        pInputStream, pDemuxer, pItem->GetVideoInfoTag()->m_streamDetails, strFileNameAndPath);

    if (!pInputStream->IsStreamType(DVDSTREAM_TYPE_PVRMANAGER))
      ProcessExternalSubtitles(pItem);

    delete pDemuxer;
    return retVal;
  }
  else
  {
    return false;
  }
}

bool CDVDFileInfo::DemuxerToStreamDetails(const std::shared_ptr<CDVDInputStream>& pInputStream,
                                          CDVDDemux* pDemuxer,
                                          const std::vector<CStreamDetailSubtitle>& subs,
                                          CStreamDetails& details)
{
  bool result = DemuxerToStreamDetails(pInputStream, pDemuxer, details);
  for (unsigned int i = 0; i < subs.size(); i++)
  {
    CStreamDetailSubtitle* sub = new CStreamDetailSubtitle();
    sub->m_strLanguage = subs[i].m_strLanguage;
    sub->m_flags = subs[i].m_flags;
    sub->SetSource(CStreamDetail::MEDIA);
    details.AddStream(sub);
    result = true;
  }
  return result;
}

static bool GetDetailsFromFrame(CDemuxStreamVideo* stream,
                                CDVDDemux* demuxer,
                                const std::string& redactPath,
                                CStreamDetailVideo& vDetail)
{
  std::unique_ptr<CProcessInfo> processInfo(CProcessInfo::CreateInstance());
  std::vector<AVPixelFormat> pixFmts{AV_PIX_FMT_YUV420P};
  processInfo->SetPixFormats(pixFmts);

  CDVDStreamInfo hint(*stream, true);
  hint.codecOptions = CODEC_FORCE_SOFTWARE;

  std::unique_ptr<CDVDVideoCodec> videoCodec =
      CDVDFactoryCodec::CreateVideoCodec(hint, *processInfo);
  if (!videoCodec)
  {
    CLog::LogF(LOGERROR, "Unable to create video codec to retrieve HDR details");
    return false;
  }

  VideoPicture picture = {};
  int packetsTried = 0;
  if (!SeekAndDecodePictureAt(*demuxer, *videoCodec, stream->uniqueId,
                              demuxer->GetStreamLength() / 5, redactPath, picture, packetsTried))
  {
    CLog::LogF(LOGERROR, "no picture decoded after {} packets", packetsTried);
    return false;
  }

  vDetail.m_strHdrType = CStreamDetails::HdrTypeToString(picture.hdrType);
  vDetail.m_strHdrTypeAlt = CStreamDetails::HdrTypeToString(picture.hdrTypeAlt);
  if (vDetail.m_strHdrDetail.find("7") != std::string::npos)
    vDetail.m_strHdrDetail += picture.strDVELType;
  return true;
}

/* returns true if details have been added */
bool CDVDFileInfo::DemuxerToStreamDetails(const std::shared_ptr<CDVDInputStream>& pInputStream,
                                          CDVDDemux* pDemux,
                                          CStreamDetails& details,
                                          const std::string& path)
{
  bool retVal = false;
  details.Reset();

  const CURL pathToUrl(path);
  for (CDemuxStream* stream : pDemux->GetStreams())
  {
    if (stream->type == StreamType::VIDEO && !(stream->flags & AV_DISPOSITION_ATTACHED_PIC))
    {
      CStreamDetailVideo *p = new CStreamDetailVideo();
      CDemuxStreamVideo* vstream = static_cast<CDemuxStreamVideo*>(stream);
      p->m_iWidth = vstream->iWidth;
      p->m_iHeight = vstream->iHeight;
      p->m_fAspect = static_cast<float>(vstream->fAspect);
      if (p->m_fAspect == 0.0f && p->m_iHeight > 0)
        p->m_fAspect = (float)p->m_iWidth / p->m_iHeight;
      p->m_strCodec = pDemux->GetStreamCodecName(stream->demuxerId, stream->uniqueId);
      p->m_iDuration = pDemux->GetStreamLength();
      p->m_strStereoMode = vstream->stereo_mode;
      p->m_strLanguage = vstream->language.AsIso6392B();
      p->m_strHdrType = CStreamDetails::HdrTypeToString(vstream->hdr_type);
      if (vstream->hdr_type == StreamHdrType::HDR_TYPE_DOLBYVISION)
      {
        p->m_strHdrDetail = vstream->dovi.dv_profile == 0
                                ? ""
                                : std::to_string(static_cast<int>(vstream->dovi.dv_profile));
        // distinguish HDR10 from HLG base
        if (vstream->dovi.dv_profile == 8)
        {
          p->m_strHdrDetail += ".";
          p->m_strHdrDetail +=
              std::to_string(static_cast<int>(vstream->dovi.dv_bl_signal_compatibility_id));
          if (vstream->dovi.dv_bl_signal_compatibility_id == 4)
            p->m_strHdrTypeAlt = "hlg";
        }
      }
      // look for DV EL type and/or hdr10+
      if (vstream->hdr_type != StreamHdrType::HDR_TYPE_NONE &&
          vstream->hdr_type != StreamHdrType::HDR_TYPE_HLG && vstream->dovi.dv_profile != 5 &&
          vstream->dovi.dv_profile <= 10 && p->m_strHdrTypeAlt != "hlg")
      {
        if (!GetDetailsFromFrame(vstream, pDemux, CURL::GetRedacted(path), *p))
          CLog::LogF(LOGERROR, "Failed to get HDR details from frame");
      }
      p->SetSource(CStreamDetail::MEDIA);

      // stack handling
      if (URIUtils::IsStack(path))
      {
        CFileItemList files;
        XFILE::CStackDirectory stack;
        stack.GetDirectory(pathToUrl, files);

        // skip first path as we already know the duration
        for (int i = 1; i < files.Size(); i++)
        {
           int duration = 0;
           if (CDVDFileInfo::GetFileDuration(files[i]->GetDynPath(), duration))
             p->m_iDuration = p->m_iDuration + duration;
        }
      }

      // finally, calculate seconds
      if (p->m_iDuration > 0)
        p->m_iDuration = p->m_iDuration / 1000;

      details.AddStream(p);

      if (details.GetVideoHdrType(1, true).length() > 0)
      {
        // add a virtual stream for the alternate HDR type
        CStreamDetailVideo* q = new CStreamDetailVideo();
        *q = *p;
        q->m_strHdrType = q->m_strHdrTypeAlt;
        // atm we use hdrDetail only for DV
        if (q->m_strHdrType != "dolbyvision")
          q->m_strHdrDetail = "";
        if (p->m_strHdrType != "dolbyvision")
          p->m_strHdrDetail = "";
        details.AddStream(q);
      }

      retVal = true;
    }

    else if (stream->type == StreamType::AUDIO)
    {
      CStreamDetailAudio *p = new CStreamDetailAudio();
      p->m_iChannels = static_cast<CDemuxStreamAudio*>(stream)->iChannels;
      p->m_strLanguage = stream->language.AsIso6392B();
      p->m_strCodec = pDemux->GetStreamCodecName(stream->demuxerId, stream->uniqueId);
      p->m_flags = stream->flags;
      p->SetSource(CStreamDetail::MEDIA);
      details.AddStream(p);
      retVal = true;
    }

    else if (stream->type == StreamType::SUBTITLE)
    {
      CStreamDetailSubtitle *p = new CStreamDetailSubtitle();
      p->m_strLanguage = stream->language.AsIso6392B();
      p->m_flags = stream->flags;
      p->SetSource(CStreamDetail::MEDIA);
      details.AddStream(p);
      retVal = true;
    }
  }  /* for iStream */

  details.DetermineBestStreams();
#ifdef HAVE_LIBBLURAY
  // correct bluray runtime. we need the duration from the input stream, not the demuxer.
  if (pInputStream->IsStreamType(DVDSTREAM_TYPE_BLURAY))
  {
    if (std::static_pointer_cast<CDVDInputStreamBluray>(pInputStream)->GetTotalTime() > 0)
    {
      const CStreamDetailVideo* dVideo = static_cast<const CStreamDetailVideo*>(details.GetNthStream(CStreamDetail::VIDEO, 0));
      CStreamDetailVideo* detailVideo = const_cast<CStreamDetailVideo*>(dVideo);
      if (detailVideo)
        detailVideo->m_iDuration = std::static_pointer_cast<CDVDInputStreamBluray>(pInputStream)->GetTotalTime() / 1000;
    }
  }
#endif
  return retVal;
}

void CDVDFileInfo::ProcessExternalSubtitles(CFileItem* item)
{
  std::vector<std::string> externalSubtitles;
  const std::string videoPath = item->GetDynPath();

  CUtil::ScanForExternalSubtitles(videoPath, externalSubtitles);

  for (const auto& externalSubtitle : externalSubtitles)
  {
    // if vobsub subtitle:
    if (URIUtils::GetExtension(externalSubtitle) == ".idx")
    {
      std::string subFile;
      if (CUtil::FindVobSubPair(externalSubtitles, externalSubtitle, subFile))
        AddExternalSubtitleToDetails(videoPath, item->GetVideoInfoTag()->m_streamDetails,
                                     externalSubtitle, subFile);
    }
    else
    {
      if (!CUtil::IsVobSub(externalSubtitles, externalSubtitle))
      {
        AddExternalSubtitleToDetails(videoPath, item->GetVideoInfoTag()->m_streamDetails,
                                     externalSubtitle);
      }
    }
  }
}

bool CDVDFileInfo::AddExternalSubtitleToDetails(const std::string &path, CStreamDetails &details, const std::string& filename, const std::string& subfilename)
{
  std::string ext = URIUtils::GetExtension(filename);
  std::string vobsubfile = subfilename;
  if(ext == ".idx")
  {
    if (vobsubfile.empty())
      vobsubfile = URIUtils::ReplaceExtension(filename, ".sub");

    CDVDDemuxVobsub v;
    if (!v.Open(filename, STREAM_SOURCE_NONE, vobsubfile))
      return false;

    const ExternalStreamInfo idxInfo = CUtil::GetExternalStreamDetailsFromFilename(path, filename);

    for(CDemuxStream* stream : v.GetStreams())
    {
      CStreamDetailSubtitle *dsub = new CStreamDetailSubtitle();
      dsub->m_strLanguage = stream->language.AsIso6392B();
      // Mirror CVideoPlayer::AddSubtitleFile: a flag in the filename overrides the demuxer,
      // so the scanner and the player agree
      dsub->m_flags = static_cast<StreamFlags>(idxInfo.flag) != StreamFlags::FLAG_NONE
                          ? static_cast<StreamFlags>(idxInfo.flag)
                          : stream->flags;
      dsub->SetSource(CStreamDetail::MEDIA);
      details.AddStream(dsub);
    }
    return true;
  }
  if(ext == ".sub")
  {
    std::string strReplace(URIUtils::ReplaceExtension(filename,".idx"));
    if (XFILE::CFile::Exists(strReplace))
      return false;
  }

  CStreamDetailSubtitle *dsub = new CStreamDetailSubtitle();
  ExternalStreamInfo info = CUtil::GetExternalStreamDetailsFromFilename(path, filename);
  dsub->m_strLanguage = info.language.AsIso6392B();
  dsub->m_flags = static_cast<StreamFlags>(info.flag);
  dsub->SetSource(CStreamDetail::MEDIA);
  details.AddStream(dsub);

  return true;
}

