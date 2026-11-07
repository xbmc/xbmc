/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>

// C interface to the browser's Media Source Extensions, implemented in mse_bridge.js. The
// MediaSource, its SourceBuffers and the <video> element it plays in live on the browser
// main thread, and every call below is proxied there. Each track takes fragmented MP4.

extern "C"
{
  // Published by the bridge for the lifetime of a source; Kodi's threads only read it,
  // with atomic loads, and wait on `signal`, which is bumped after every change.
  struct MseState
  {
    int32_t signal;
    int32_t open; // the source takes tracks and data
    int32_t canPlay; // the media element has enough data to start
    int32_t ended;
    int32_t error; // MseError, 0 when none
    int32_t videoSession; // to pass with every video append
    int32_t audioSession;
    int32_t reserved;
    double position; // playback position in seconds
  };

  enum MseError
  {
    MSE_NO_ERROR = 0,
    MSE_APPEND_ERROR = 1,
    MSE_MEDIA_ERROR = 2,
  };

  enum MseAppendResult
  {
    MSE_APPEND_OK = 0,
    MSE_APPEND_FULL = 1, // offer the data again later
    MSE_APPEND_STALE = 2, // belongs to the position before a seek, dropped
    MSE_APPEND_FAILED = 3,
  };

  // Whether this browser has Media Source Extensions.
  int mse_available(void);
  // MediaSource.isTypeSupported() of a MIME type with codecs.
  int mse_is_type_supported(const char* type);

  // Creates a source attached to a <video> element of WasmMediaLayer.h. Returns a handle,
  // or -1.
  int mse_create(MseState* state);
  void mse_destroy(int handle);

  // Adds the video (video != 0) or audio track once the source is open. The bridge adds
  // timestampOffset to the timestamps of the data. Returns 0, or -1.
  int mse_add_track(int handle, int video, const char* type, double timestampOffset);
  // Duration in seconds; a non-finite value is an unbounded (live) stream.
  int mse_set_duration(int handle, double seconds);
  // Queues data for a track; the bridge appends it in order.
  int mse_append(int handle, int video, const uint8_t* data, int size, int session);
  // No more data for the track at this session; the source ends once every track has.
  void mse_end_track(int handle, int video, int session);

  void mse_play(int handle);
  void mse_pause(int handle);
  void mse_set_playback_rate(int handle, double rate);
  void mse_set_volume(int handle, double volume);
  // Moves playback to `seconds`: queued data is dropped and both sessions change.
  void mse_seek(int handle, double seconds);
}
