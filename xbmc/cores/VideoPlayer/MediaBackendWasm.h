/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDCodecs/Audio/DVDAudioCodec.h"
#include "DVDStreamInfo.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

struct DemuxPacket;

/*!
 * \brief A media player of the browser or the device that CMediaPipelineWasm feeds: it
 * decodes, keeps audio and video in sync and shows the video in a <video> element under
 * the GUI canvas.
 *
 * Timestamps are in seconds. Every call comes from one of the pipeline's threads.
 */
class IMediaBackendWasm
{
public:
  enum class AppendResult
  {
    OK,
    FULL, //!< offer the packet again later
    STALE, //!< dropped, it belonged to the position before a seek
    NEEDS_KEY_FRAME,
    FAILED,
  };

  enum class AudioPath
  {
    NONE,
    NATIVE, //!< the backend plays the stream as it is
    CONVERTED, //!< FFmpeg decodes it and the backend takes the decoded frames
  };

  struct Packet
  {
    const uint8_t* data{nullptr};
    size_t size{0};
    double pts{0.0};
    double dts{0.0};
    double duration{0.0};
    bool keyFrame{false};
  };

  virtual ~IMediaBackendWasm() = default;

  //! The backend for this device and settings, or nullptr.
  static std::unique_ptr<IMediaBackendWasm> Create();

  virtual std::string Name() const = 0;
  //! Looks at the codec, profile, bit depth, size and frame rate only.
  virtual bool SupportsVideo(const CDVDStreamInfo& hint) = 0;
  //! Looks at the codec, profile, channels and sample rate only.
  virtual AudioPath GetAudioPath(const CDVDStreamInfo& hint) = 0;

  /*!
   * \brief Creates the media source with its tracks and waits until it takes packets.
   * \param audio nullptr for no audio.
   * \param duration infinity when unknown.
   * \return A description of the tracks for the log, empty on failure.
   */
  virtual std::string Open(const CDVDStreamInfo& video,
                           const CDVDStreamInfo* audio,
                           AudioPath audioPath,
                           double duration) = 0;
  virtual void Close() = 0;
  virtual std::string VideoCodecName() const = 0;
  virtual std::string AudioCodecName() const = 0;
  //! How far ahead of the playback position audio can be appended.
  virtual double MaxAudioAhead() const = 0;

  //! \return false when the packet is not to be appended, such as before the first
  //! parameter sets. \p out stays valid until the next call.
  virtual bool PrepareVideo(DemuxPacket& packet, Packet& out) = 0;
  virtual AppendResult AppendVideo(const Packet& packet) = 0;
  virtual AppendResult AppendAudio(const Packet& packet) = 0;
  virtual void EndVideo() = 0;
  virtual void EndAudio() = 0;

  //! For AudioPath::CONVERTED: takes decoded frames and returns them as packets to append.
  virtual void ConvertAudio(const DVDAudioFrame& frame) = 0;
  //! Turns the frames held back for a bigger packet into one.
  virtual void FinishConvertedAudio() = 0;
  virtual bool PeekConvertedAudio(Packet& out) = 0;
  virtual void PopConvertedAudio() = 0;
  //! Drops what the backend holds for the position before a flush.
  virtual void Flush() = 0;

  virtual void Play() = 0;
  virtual void Pause() = 0;
  virtual void SetRate(double rate) = 0;
  //! Whether SetVolume() applies Kodi's volume; otherwise the device's own volume does.
  virtual bool HasVolume() const = 0;
  virtual void SetVolume(float volume) = 0;
  //! Moves playback to \p seconds and returns once the tracks take packets for it.
  virtual void Seek(double seconds) = 0;
  //! Where a fresh source reports to be until it plays.
  virtual void SetStartPosition(double seconds) = 0;

  //! 0 while playback has no error.
  virtual int Error() const = 0;
  virtual bool CanPlay() const = 0;
  virtual bool Ended() const = 0;
  virtual double Position() const = 0;
};

namespace WASM_MEDIA
{
//! How far ahead of playback packets are appended. Samsung's player takes them up to
//! about three seconds ahead.
constexpr double MAX_FEED_AHEAD = 2.5;

//! The RFC 6381 codec string of an H.264, HEVC, VP9 or AV1 stream, such as "avc1.640028".
std::string VideoCodecString(const CDVDStreamInfo& hint);
//! \param nalLengthSize 0 for Annex B, otherwise the size of the NAL unit lengths.
bool IsKeyFrame(AVCodecID codec, const uint8_t* data, size_t size, int nalLengthSize);
} // namespace WASM_MEDIA
