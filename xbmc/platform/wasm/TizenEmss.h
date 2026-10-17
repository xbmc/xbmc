/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>

// C interface to Samsung's Tizen WASM Player (tizentvwasm.ElementaryMediaStreamSource),
// implemented in tizen_emss.js. The source, its tracks and the <video> element it
// plays in live on the browser main thread; every call below is proxied there except
// the appends, which go to the TV from the calling pthread.

extern "C"
{
  // Published by the bridge for the lifetime of a source; Kodi's threads only read it,
  // with atomic loads, and wait on `signal`, which is bumped after every change.
  struct TizenEmssState
  {
    int32_t signal;
    int32_t readyState; // TizenEmssReadyState
    int32_t videoOpen; // the track takes packets
    int32_t videoSessionId; // to put in every video packet
    int32_t audioOpen;
    int32_t audioSessionId;
    int32_t canPlay; // the media element has enough data to start
    int32_t ended;
    int32_t error; // TizenEmssError, 0 when none
    int32_t videoDecodingMode; // TizenEmssDecodingMode: the decoder the TV gave the video track
    double position; // playback position in seconds
  };

  enum TizenEmssReadyState
  {
    TIZEN_EMSS_DETACHED = 0,
    TIZEN_EMSS_CLOSED = 1,
    TIZEN_EMSS_OPEN_PENDING = 2,
    TIZEN_EMSS_OPEN = 3,
    TIZEN_EMSS_ENDED = 4,
  };

  enum TizenEmssError
  {
    TIZEN_EMSS_NO_ERROR = 0,
    TIZEN_EMSS_CONFIG_ERROR = 1,
    TIZEN_EMSS_DRM_ERROR = 2,
    TIZEN_EMSS_PIPELINE_ERROR = 3,
    TIZEN_EMSS_OUT_OF_MEMORY = 4,
    TIZEN_EMSS_MEDIA_ERROR = 5,
    TIZEN_EMSS_UNKNOWN_ERROR = 6,
  };

  // Software is a fallback the TV may pick for a configuration its hardware decoder refuses.
  enum TizenEmssDecodingMode
  {
    TIZEN_EMSS_DECODE_UNKNOWN = 0,
    TIZEN_EMSS_DECODE_HARDWARE = 1,
    TIZEN_EMSS_DECODE_SOFTWARE = 2,
  };

  enum TizenEmssAppendResult
  {
    TIZEN_EMSS_APPEND_OK = 0,
    TIZEN_EMSS_APPEND_BUFFER_FULL = 1, // offer the packet again later
    TIZEN_EMSS_APPEND_IGNORED = 2, // stale session, drop the packet
    TIZEN_EMSS_APPEND_NEEDS_KEYFRAME = 3,
    TIZEN_EMSS_APPEND_FAILED = 4,
  };

  // Whether this device has the Tizen WASM Player.
  int tizen_emss_available(void);

  // Whether the TV decodes a stream: 1 yes, 0 no, -1 when this firmware cannot tell.
  // Sample formats and channel layouts are Samsung's names, such as "SampleFormatS16"
  // and "ChannelLayout5_1".
  int tizen_emss_validate_video(
      const char* mimeType, int width, int height, int framerateNum, int framerateDen);
  int tizen_emss_validate_audio(const char* mimeType,
                                const char* sampleFormat,
                                const char* channelLayout,
                                int samplesPerSecond);

  // Creates a source in normal latency mode, attached to a <video> element of
  // WasmMediaLayer.h. Returns a handle, or -1.
  int tizen_emss_create(TizenEmssState* state);
  void tizen_emss_destroy(int handle);

  // Tracks are added while the source is closed. Return a track id, or -1.
  int tizen_emss_add_video_track(int handle,
                                 const char* mimeType,
                                 const uint8_t* extradata,
                                 int extradataSize,
                                 int width,
                                 int height,
                                 int framerateNum,
                                 int framerateDen);
  int tizen_emss_add_audio_track(int handle,
                                 const char* mimeType,
                                 const uint8_t* extradata,
                                 int extradataSize,
                                 const char* sampleFormat,
                                 const char* channelLayout,
                                 int samplesPerSecond);

  // Duration in seconds; a non-finite value is an unbounded (live) stream.
  int tizen_emss_set_duration(int handle, double seconds);
  // Opens the source; the outcome arrives in TizenEmssState::readyState.
  int tizen_emss_open(int handle);
  void tizen_emss_play(int handle);
  void tizen_emss_pause(int handle);
  void tizen_emss_set_playback_rate(int handle, double rate);
  // Moves playback to `seconds`. The tracks close and reopen with new session ids;
  // the first packet after that has to be a key frame.
  void tizen_emss_seek(int handle, double seconds);

  // Not proxied: run on the calling pthread. Timestamps are in seconds. Width and
  // height are 0 unless the video size changes with this packet.
  int tizen_emss_append(int trackId,
                        const uint8_t* data,
                        int size,
                        double pts,
                        double dts,
                        double duration,
                        int keyFrame,
                        int sessionId,
                        int width,
                        int height);
  int tizen_emss_append_end(int trackId, int sessionId);
}
