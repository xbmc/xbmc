/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// Shared ABI between the C++ video codec and the Emscripten JS library that
// drives WebCodecs. All functions declared here are implemented in
// webcodecs_bridge.js and linked via --js-library.
//
// The VideoDecoder lives on the browser main thread, so the JS library proxies
// every call there with Emscripten's __proxy attribute; callers may invoke
// these functions from any pthread. The per-frame calls are asynchronous and
// report through WebCodecsSharedState, the others are synchronous.
//
// Every output frame gets a per-decoder sequence number. The JS side keeps the
// VideoFrame in a map under that number and publishes its metadata in the
// ring below; the codec takes frames in sequence order without a call into JS
// and refers to them by number afterwards.

#include <cstdint>

#ifdef __cplusplus
extern "C"
{
#endif

  // VideoFrame.format values, named exactly like the WebCodecs strings so the JS
  // side maps a frame by name through the Embind table. Must stay in sync with
  // the table in DVDVideoCodecWebCodecs.cpp. OPAQUE is a frame whose format is
  // null: GPU memory in a layout WebCodecs has no name for (P010, for one),
  // which texImage2D still accepts but copyTo() cannot read.
  enum WebCodecsPixelFormat
  {
    WEBCODECS_PIXFMT_UNKNOWN = 0,
    WEBCODECS_PIXFMT_I420,
    WEBCODECS_PIXFMT_I420P10,
    WEBCODECS_PIXFMT_I420P12,
    WEBCODECS_PIXFMT_I420A,
    WEBCODECS_PIXFMT_I420AP10,
    WEBCODECS_PIXFMT_I420AP12,
    WEBCODECS_PIXFMT_I422,
    WEBCODECS_PIXFMT_I422P10,
    WEBCODECS_PIXFMT_I422P12,
    WEBCODECS_PIXFMT_I422A,
    WEBCODECS_PIXFMT_I422AP10,
    WEBCODECS_PIXFMT_I422AP12,
    WEBCODECS_PIXFMT_I444,
    WEBCODECS_PIXFMT_I444P10,
    WEBCODECS_PIXFMT_I444P12,
    WEBCODECS_PIXFMT_I444A,
    WEBCODECS_PIXFMT_I444AP10,
    WEBCODECS_PIXFMT_I444AP12,
    WEBCODECS_PIXFMT_NV12,
    WEBCODECS_PIXFMT_RGBA,
    WEBCODECS_PIXFMT_RGBX,
    WEBCODECS_PIXFMT_BGRA,
    WEBCODECS_PIXFMT_BGRX,
    WEBCODECS_PIXFMT_OPAQUE,
  };

  // Outcome of a webcodecs_copy_frame request, published in
  // WebCodecsSharedState::copyResult once copyDone carries the request's id.
  enum WebCodecsCopyResult
  {
    WEBCODECS_COPY_OK = 1,
    WEBCODECS_COPY_FAILED = -1,
    WEBCODECS_COPY_DST_TOO_SMALL = -2,
    WEBCODECS_COPY_NO_FRAME = -3,
  };

  enum
  {
    // Cap on decoder buffers in use at once: chunks pushed but not yet output
    // and output frames not yet closed, taken or not. The Samsung decoder
    // reuses the buffer of an open frame when it runs out; it ran the copy path
    // with 19 open without harm, and it holds about ten chunks before it
    // outputs anything.
    WEBCODECS_MAX_INFLIGHT = 16,
    // Slots in WebCodecsSharedState::ring. The output callback drops a frame
    // rather than overwrite a slot the codec has not taken.
    WEBCODECS_FRAME_RING = 32,
  };

  // Metadata of one output frame, written by the JS side into
  // ring[sequence % WEBCODECS_FRAME_RING] before framesProduced is published.
  // Field offsets are locked down by static_assert below and mirrored as byte
  // offsets in JS. The colour fields hold FFmpeg's AVCOL_* values, with the
  // *_UNSPECIFIED value where the frame carries none; fullRange is 1, 0 or -1
  // for unknown. The layout fields serve the sysmem copy path only.
  struct WebCodecsFrameInfo
  {
    int32_t width; // visibleRect
    int32_t height;
    int32_t displayWidth;
    int32_t displayHeight;
    int32_t pixelFormat; // WebCodecsPixelFormat
    int32_t keyFrame;
    int32_t colorMatrix;
    int32_t colorPrimaries;
    int32_t colorTransfer;
    int32_t fullRange;
    int32_t payloadSize; // tightly packed copy size; 0 on the texture path or if not copyable
    int32_t yStride;
    int32_t uStride;
    int32_t vStride;
    int32_t uOffset;
    int32_t vOffset;
    double ptsSeconds;
    double durationSeconds;
  };

  // Decoder state mirrored into wasm memory by the JS side with Atomics so the
  // codec can poll it without a main-thread round trip. `signal` is incremented
  // and Atomics.notify'd on every change, so callers can futex-wait on it.
  // framesTaken is the one field the codec writes: the sequence number
  // GetPicture will read next. Queued frames are framesProduced - framesTaken.
  struct WebCodecsSharedState
  {
    int32_t signal;
    int32_t framesProduced; // sequence number of the next output frame
    int32_t framesTaken;
    int32_t inflight; // decodeQueueSize + frame copies still in progress
    int32_t failed;
    int32_t pushesProcessed; // webcodecs_push_packet calls the main thread has run
    int32_t copyDone; // copyId of the last finished webcodecs_copy_frame
    int32_t copyResult; // WebCodecsCopyResult of that copy
    int32_t openFrames; // output frames not yet closed, taken by the codec or not
    int32_t decoding; // accepted chunks with no frame output yet, plus a copy in progress
    int32_t configured; // 1 once configure() has run; until then failed says whether it ever will
    int32_t hardware; // 1 when the browser accepted 'prefer-hardware', 0 when it chose the decoder
    struct WebCodecsFrameInfo ring[WEBCODECS_FRAME_RING];
  };

#ifdef __cplusplus
} // extern "C"

#include <cstddef>

static_assert(sizeof(WebCodecsFrameInfo) == 80, "WebCodecsFrameInfo must be 80 bytes");
static_assert(offsetof(WebCodecsFrameInfo, width) == 0, "width offset");
static_assert(offsetof(WebCodecsFrameInfo, height) == 4, "height offset");
static_assert(offsetof(WebCodecsFrameInfo, displayWidth) == 8, "displayWidth offset");
static_assert(offsetof(WebCodecsFrameInfo, displayHeight) == 12, "displayHeight offset");
static_assert(offsetof(WebCodecsFrameInfo, pixelFormat) == 16, "pixelFormat offset");
static_assert(offsetof(WebCodecsFrameInfo, keyFrame) == 20, "keyFrame offset");
static_assert(offsetof(WebCodecsFrameInfo, colorMatrix) == 24, "colorMatrix offset");
static_assert(offsetof(WebCodecsFrameInfo, colorPrimaries) == 28, "colorPrimaries offset");
static_assert(offsetof(WebCodecsFrameInfo, colorTransfer) == 32, "colorTransfer offset");
static_assert(offsetof(WebCodecsFrameInfo, fullRange) == 36, "fullRange offset");
static_assert(offsetof(WebCodecsFrameInfo, payloadSize) == 40, "payloadSize offset");
static_assert(offsetof(WebCodecsFrameInfo, yStride) == 44, "yStride offset");
static_assert(offsetof(WebCodecsFrameInfo, uStride) == 48, "uStride offset");
static_assert(offsetof(WebCodecsFrameInfo, vStride) == 52, "vStride offset");
static_assert(offsetof(WebCodecsFrameInfo, uOffset) == 56, "uOffset offset");
static_assert(offsetof(WebCodecsFrameInfo, vOffset) == 60, "vOffset offset");
static_assert(offsetof(WebCodecsFrameInfo, ptsSeconds) == 64, "ptsSeconds offset");
static_assert(offsetof(WebCodecsFrameInfo, durationSeconds) == 72, "durationSeconds offset");

static_assert(sizeof(WebCodecsSharedState) == 48 + WEBCODECS_FRAME_RING * 80,
              "WebCodecsSharedState size");
static_assert(offsetof(WebCodecsSharedState, signal) == 0, "signal offset");
static_assert(offsetof(WebCodecsSharedState, framesProduced) == 4, "framesProduced offset");
static_assert(offsetof(WebCodecsSharedState, framesTaken) == 8, "framesTaken offset");
static_assert(offsetof(WebCodecsSharedState, inflight) == 12, "inflight offset");
static_assert(offsetof(WebCodecsSharedState, failed) == 16, "failed offset");
static_assert(offsetof(WebCodecsSharedState, pushesProcessed) == 20, "pushesProcessed offset");
static_assert(offsetof(WebCodecsSharedState, copyDone) == 24, "copyDone offset");
static_assert(offsetof(WebCodecsSharedState, copyResult) == 28, "copyResult offset");
static_assert(offsetof(WebCodecsSharedState, openFrames) == 32, "openFrames offset");
static_assert(offsetof(WebCodecsSharedState, decoding) == 36, "decoding offset");
static_assert(offsetof(WebCodecsSharedState, configured) == 40, "configured offset");
static_assert(offsetof(WebCodecsSharedState, hardware) == 44, "hardware offset");
static_assert(offsetof(WebCodecsSharedState, ring) == 48, "ring offset");

extern "C"
{
#endif

  // Returns 1 if the page's WebGL context accepts a VideoFrame as a texImage2D
  // source, 0 otherwise. The first call runs the probe on the context Emscripten
  // holds on the main thread and must come from the thread that owns the GL
  // command stream, before it has queued anything (InitWindowSystem); later
  // calls return the cached answer.
  int webcodecs_probe_texture_upload(void);

  // Returns a positive decoder handle on success, 0 on failure. `description` is
  // the AVC or HEVC decoder configuration record when the stream has one, and
  // `annexB` selects the Annex B bitstream format for those two codecs. `shared`
  // must stay valid until webcodecs_destroy_decoder.
  //
  // The decoder is configured asynchronously: the bridge asks isConfigSupported()
  // for hardware decoding first and settles for whatever the browser offers when
  // that is refused. Callers wait on `shared` until either configured or failed
  // is set before pushing packets.
  int webcodecs_create_decoder(const char* codec,
                               int codedWidth,
                               int codedHeight,
                               const uint8_t* description,
                               int descriptionSize,
                               int annexB,
                               struct WebCodecsSharedState* shared);

  // Closes every frame and the decoder, and unregisters the handle. Later calls
  // with the handle are no-ops.
  void webcodecs_destroy_decoder(int handle);

  // reset() drops pending work and requires reconfigure(); the bridge handles
  // both, and closes the frames the codec has not taken (sequence numbers from
  // framesTaken up). Sequence numbers keep counting across resets.
  int webcodecs_reset_decoder(int handle);

  // Feeds one encoded packet. Runs asynchronously on the main thread and takes
  // ownership of `data`, which must come from malloc(). Each call is counted in
  // WebCodecsSharedState::pushesProcessed once it has run; a packet the decoder
  // rejects puts it into the failed state. A packet without timestamp passes a
  // NaN ptsSeconds, and so does the frame decoded from it.
  void webcodecs_push_packet(
      int handle, uint8_t* data, int size, int keyFrame, double ptsSeconds, double durationSeconds);

  // Uploads frame `sequence` into GL texture `glTexture` (an Emscripten texture
  // name from glGenTextures) with texImage2D on the main thread. Asynchronous;
  // ordered with the caller's own GL calls because both go through Emscripten's
  // system proxying queue. The frame stays open until webcodecs_release_frame(),
  // as VideoPlayer queues a still picture again into other render buffers.
  void webcodecs_upload_frame(int handle, int sequence, unsigned int glTexture);

  // Closes frame `sequence` if it is still open. Asynchronous; a no-op for a
  // frame already released or closed by a reset.
  void webcodecs_release_frame(int handle, int sequence);

  // Copies frame `sequence` into [dst, dst+dstSize) with the layout published in
  // its ring slot, then closes it. Runs asynchronously on the main thread; when
  // the copy has finished, WebCodecsSharedState::copyDone is set to copyId and
  // copyResult to a WebCodecsCopyResult. dst must stay valid until then.
  void webcodecs_copy_frame(int handle, int sequence, int copyId, uint8_t* dst, int dstSize);

  // Reads accumulated queue stats.
  int webcodecs_read_stats(int handle, int* droppedFrames, int* highWaterMark);

  // Writes a NUL-terminated UTF-8 diagnostic string ("<state>|<error>") of at
  // most (dstSize-1) bytes into dst. Returns the number of bytes written
  // (excluding the NUL). A return of 0 means "nothing to report".
  int webcodecs_take_error(int handle, char* dst, int dstSize);

#ifdef __cplusplus
}
#endif
