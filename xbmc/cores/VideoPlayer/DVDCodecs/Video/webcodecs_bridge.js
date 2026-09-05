/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

//
// Emscripten JS library implementing the WebCodecs bridge declared in
// DVDVideoCodecWebCodecsBridge.h. Linked via --js-library (see
// cmake/scripts/wasm/ArchSetup.cmake).
//
// Every exported function has __proxy:'sync' so calls from any pthread are
// automatically marshalled to the main browser thread, where the VideoDecoder
// lives. Decoder state the C++ side polls is mirrored into WebCodecsSharedState
// with Atomics instead (see publishState) so polling needs no round trip.
//
// Function signatures must match the C prototypes; mismatched __sig will
// silently corrupt arguments on threaded builds.
//

mergeInto(LibraryManager.library, {

  // ---------------------------------------------------------------------------
  // Internal module: shared state + helpers, attached only if any bridge
  // function is used (Emscripten dead-strips unreferenced $-symbols).
  // ---------------------------------------------------------------------------
  $WebCodecsBridge: {
    MICROSECONDS_PER_SECOND: 1000000.0,
    FRAME_QUEUE_HIGH_WATER: 24,
    // Cap on decoder frames alive at once: queued for decode, held open
    // awaiting copy, or being copied. Beyond this push_packet reports BUSY so
    // VideoPlayer re-queues the packet; hardware decoders stall when too many
    // output frames stay open.
    MAX_INFLIGHT: 12,

    // Enum values below are pulled from the C++-owned Embind registrations
    // (Module.WebCodecsPixelFormat / Module.WebCodecsPushStatus) on first use
    // by syncEnumsFromEmbind(). The C++ header (DVDVideoCodecWebCodecsBridge.h)
    // is the single source of truth; do not hardcode these here.
    PIXFMT_UNKNOWN: 0,
    PIXFMT_YUV420P: 0,
    PIXFMT_NV12: 0,
    PUSH_QUEUED: 0,
    PUSH_EMPTY: 0,
    PUSH_HANDLE_NOT_FOUND: 0,
    PUSH_DECODER_FAILED: 0,
    PUSH_NOT_CONFIGURED: 0,
    PUSH_DECODE_THREW: 0,
    PUSH_BUSY: 0,
    _enumsReady: false,

    // Byte offsets inside struct WebCodecsFrameInfo (kept in sync with the
    // static_asserts in DVDVideoCodecWebCodecsBridge.h).
    FI_PIXFMT: 0,
    FI_WIDTH: 4,
    FI_HEIGHT: 8,
    FI_Y_STRIDE: 12,
    FI_U_STRIDE: 16,
    FI_V_STRIDE: 20,
    FI_U_OFFSET: 24,
    FI_V_OFFSET: 28,
    FI_KEYFRAME: 32,
    FI_PAYLOAD_SIZE: 36,
    FI_PTS: 40,
    FI_DURATION: 48,

    // Int32 indices inside struct WebCodecsSharedState (offsets / 4).
    SS_SIGNAL: 0,
    SS_QUEUED_FRAMES: 1,
    SS_INFLIGHT: 2,
    SS_BUSY: 3,
    SS_FAILED: 4,
    SS_NEXT_PAYLOAD_SIZE: 5,
    SS_NEXT_PIXFMT: 6,
    SS_COPY_RESULT: 7,

    // Registry is lazily created on first decoder creation; this library file
    // is merged into both the main thread and every pthread module, but only
    // the main thread ever touches the maps (__proxy:'sync' on every entry).
    registry: null,
    nextId: 1,

    ensureRegistry: function() {
      if (!this.registry)
        this.registry = new Map();
      return this.registry;
    },

    // Pull the canonical enum numeric values out of the Embind bindings
    // registered by EMSCRIPTEN_BINDINGS(kodi_webcodecs_bridge) in
    // DVDVideoCodecWebCodecs.cpp. Embind exposes each C++ enum value as an
    // object with a .value property holding the underlying integer.
    syncEnumsFromEmbind: function() {
      if (this._enumsReady)
        return;
      const pf = Module['WebCodecsPixelFormat'];
      const ps = Module['WebCodecsPushStatus'];
      if (!pf || !ps)
        throw new Error('WebCodecs bridge enums missing from Module (Embind not linked?)');
      this.PIXFMT_UNKNOWN = pf.UNKNOWN.value;
      this.PIXFMT_YUV420P = pf.YUV420P.value;
      this.PIXFMT_NV12 = pf.NV12.value;
      this.PUSH_QUEUED = ps.QUEUED.value;
      this.PUSH_EMPTY = ps.EMPTY.value;
      this.PUSH_HANDLE_NOT_FOUND = ps.HANDLE_NOT_FOUND.value;
      this.PUSH_DECODER_FAILED = ps.DECODER_FAILED.value;
      this.PUSH_NOT_CONFIGURED = ps.NOT_CONFIGURED.value;
      this.PUSH_DECODE_THREW = ps.DECODE_THREW.value;
      this.PUSH_BUSY = ps.BUSY.value;
      this._enumsReady = true;
    },

    getState: function(handle) {
      return this.registry ? this.registry.get(handle) : null;
    },

    inflight: function(state) {
      const decoder = state.decoder;
      const queueSize = decoder && decoder.state === 'configured' ? decoder.decodeQueueSize : 0;
      return queueSize + (state.copying ? 1 : 0) + (state.flushing ? 1 : 0);
    },

    publishState: function(state) {
      if (!state.sharedPtr)
        return;
      const base = state.sharedPtr >> 2;
      const inflight = this.inflight(state);
      const next = state.frames.length > 0 ? state.frames[0] : null;
      Atomics.store(HEAP32, base + this.SS_QUEUED_FRAMES, state.frames.length);
      Atomics.store(HEAP32, base + this.SS_INFLIGHT, inflight);
      Atomics.store(HEAP32, base + this.SS_BUSY,
                    inflight + state.frames.length >= this.MAX_INFLIGHT ? 1 : 0);
      Atomics.store(HEAP32, base + this.SS_FAILED, state.failed ? 1 : 0);
      Atomics.store(HEAP32, base + this.SS_NEXT_PAYLOAD_SIZE, next ? next.payloadSize : 0);
      Atomics.store(HEAP32, base + this.SS_NEXT_PIXFMT, next ? next.pixelFormat : this.PIXFMT_UNKNOWN);
      Atomics.store(HEAP32, base + this.SS_COPY_RESULT, state.copyResult);
      Atomics.add(HEAP32, base + this.SS_SIGNAL, 1);
      Atomics.notify(HEAP32, base + this.SS_SIGNAL);
    },

    // Plane layout for the formats we accept, with tightly packed strides.
    describeFrame: function(frame, width, height) {
      const format = frame.format || 'I420';
      const yStride = width;
      const uvHeight = (height + 1) >> 1;
      const ySize = yStride * height;

      if (format === 'NV12') {
        const uvSize = yStride * uvHeight;
        return {
          pixelFormat: this.PIXFMT_NV12,
          payloadSize: ySize + uvSize,
          yStride, uStride: yStride, vStride: 0,
          uOffset: ySize, vOffset: 0,
          layout: [{ offset: 0, stride: yStride }, { offset: ySize, stride: yStride }],
        };
      }

      if (format === 'I420') {
        const uvStride = (width + 1) >> 1;
        const uvSize = uvStride * uvHeight;
        return {
          pixelFormat: this.PIXFMT_YUV420P,
          payloadSize: ySize + uvSize * 2,
          yStride, uStride: uvStride, vStride: uvStride,
          uOffset: ySize, vOffset: ySize + uvSize,
          layout: [
            { offset: 0, stride: yStride },
            { offset: ySize, stride: uvStride },
            { offset: ySize + uvSize, stride: uvStride },
          ],
        };
      }

      return null;
    },

    // Copies straight into wasm memory: under pthreads the heap is a
    // SharedArrayBuffer and existing views stay valid across growth. Browsers
    // whose copyTo() still rejects shared views fall back to a scratch buffer
    // plus one memcpy.
    copyFrame: async function(state, entry, dstPtr) {
      const options = { layout: entry.layout };
      if (entry.visibleRect)
        options.rect = entry.visibleRect;

      if (state.directCopy) {
        try {
          await entry.frame.copyTo(HEAPU8.subarray(dstPtr, dstPtr + entry.payloadSize), options);
          return;
        } catch (e) {
          if (!(e instanceof TypeError))
            throw e;
          state.directCopy = false;
        }
      }

      if (!state.scratch || state.scratch.byteLength < entry.payloadSize)
        state.scratch = new Uint8Array(entry.payloadSize);
      const scratch = state.scratch.subarray(0, entry.payloadSize);
      await entry.frame.copyTo(scratch, options);
      HEAPU8.set(scratch, dstPtr);
    },

    // Build the configure() dictionary from the stored codec parameters.
    buildConfig: function(state, width, height) {
      const config = {
        codec: state.codec,
        optimizeForLatency: true,
        hardwareAcceleration: 'prefer-hardware',
      };
      if (width > 0) config.codedWidth = width;
      if (height > 0) config.codedHeight = height;
      if (state.codec.startsWith('avc1'))
        config.avc = { format: state.annexB ? 'annexb' : 'avc' };
      if (state.description)
        config.description = state.description;
      return config;
    },

    writeFrameInfo: function(infoPtr, frame) {
      HEAP32[(infoPtr + this.FI_PIXFMT) >> 2] = frame.pixelFormat | 0;
      HEAP32[(infoPtr + this.FI_WIDTH) >> 2] = frame.width | 0;
      HEAP32[(infoPtr + this.FI_HEIGHT) >> 2] = frame.height | 0;
      HEAP32[(infoPtr + this.FI_Y_STRIDE) >> 2] = frame.yStride | 0;
      HEAP32[(infoPtr + this.FI_U_STRIDE) >> 2] = frame.uStride | 0;
      HEAP32[(infoPtr + this.FI_V_STRIDE) >> 2] = frame.vStride | 0;
      HEAP32[(infoPtr + this.FI_U_OFFSET) >> 2] = frame.uOffset | 0;
      HEAP32[(infoPtr + this.FI_V_OFFSET) >> 2] = frame.vOffset | 0;
      HEAP32[(infoPtr + this.FI_KEYFRAME) >> 2] = frame.keyFrame ? 1 : 0;
      HEAP32[(infoPtr + this.FI_PAYLOAD_SIZE) >> 2] = frame.payloadSize | 0;
      HEAPF64[(infoPtr + this.FI_PTS) >> 3] = frame.ptsSeconds;
      HEAPF64[(infoPtr + this.FI_DURATION) >> 3] = frame.durationSeconds;
    },
  },

  // ---------------------------------------------------------------------------
  // webcodecs_create_decoder: build + configure a VideoDecoder.
  // sig: i (ret) | string*, i, i, u8*, i, i, WebCodecsSharedState*
  // ---------------------------------------------------------------------------
  webcodecs_create_decoder__deps: ['$WebCodecsBridge'],
  webcodecs_create_decoder__proxy: 'sync',
  webcodecs_create_decoder__sig: 'iiiiiiii',
  webcodecs_create_decoder: function(codecPtr, width, height, extraPtr, extraSize, annexB, sharedPtr) {
    if (typeof VideoDecoder === 'undefined' || typeof EncodedVideoChunk === 'undefined') {
      console.warn('WASM WebCodecs: VideoDecoder / EncodedVideoChunk not available');
      return 0;
    }

    try {
      WebCodecsBridge.syncEnumsFromEmbind();
    } catch (e) {
      console.warn('WASM WebCodecs:', e);
      return 0;
    }

    const registry = WebCodecsBridge.ensureRegistry();
    const id = WebCodecsBridge.nextId++;
    const codec = UTF8ToString(codecPtr);

    const state = {
      id,
      codec,
      sharedPtr,
      annexB: !!annexB,
      failed: false,
      errorMessage: '',
      lastTimestamp: 0,
      frames: [],
      copying: false,
      copyResult: 0,
      scratch: null,
      directCopy: true,
      flushing: false,
      generation: 0,
      droppedFrames: 0,
      highWaterMark: 0,
      decoder: null,
      description: null,
      configured: false,
    };

    const errorCallback = (error) => {
      state.failed = true;
      state.errorMessage = 'VideoDecoder error: ' + (error && error.message ? error.message : error);
      console.warn('WASM WebCodecs:', state.errorMessage);
      WebCodecsBridge.publishState(state);
    };

    const outputCallback = (frame) => {
      const visibleRect = frame.visibleRect || null;
      const width = visibleRect ? visibleRect.width : frame.codedWidth;
      const height = visibleRect ? visibleRect.height : frame.codedHeight;
      const timestampMicros = Number.isFinite(frame.timestamp) ? Number(frame.timestamp)
                                                                : state.lastTimestamp;
      const durationMicros = Number.isFinite(frame.duration) ? Number(frame.duration) : 0;
      state.lastTimestamp = timestampMicros;

      const described = WebCodecsBridge.describeFrame(frame, width, height);
      if (!described) {
        state.failed = true;
        state.errorMessage = 'unsupported frame format: ' + frame.format;
        frame.close();
        WebCodecsBridge.publishState(state);
        return;
      }

      // Safety valve: push_packet reports BUSY long before this is reached.
      if (state.frames.length >= WebCodecsBridge.FRAME_QUEUE_HIGH_WATER) {
        state.droppedFrames += 1;
        frame.close();
        WebCodecsBridge.publishState(state);
        return;
      }

      state.frames.push(Object.assign({
        frame,
        visibleRect,
        width,
        height,
        ptsSeconds: timestampMicros / WebCodecsBridge.MICROSECONDS_PER_SECOND,
        durationSeconds: durationMicros / WebCodecsBridge.MICROSECONDS_PER_SECOND,
        keyFrame: frame.type === 'key',
      }, described));
      if (state.frames.length > state.highWaterMark)
        state.highWaterMark = state.frames.length;
      WebCodecsBridge.publishState(state);
    };

    try {
      if (extraSize > 0)
        state.description = HEAPU8.slice(extraPtr, extraPtr + extraSize);

      state.decoder = new VideoDecoder({ output: outputCallback, error: errorCallback });
      state.decoder.addEventListener('dequeue', () => WebCodecsBridge.publishState(state));

      const config = WebCodecsBridge.buildConfig(state, width, height);

      // isConfigSupported is advisory: we log but don't block on it because
      // it's async and we need a synchronous return here. A failed config
      // will surface via the decoder's error callback.
      try {
        VideoDecoder.isConfigSupported(config).then((support) => {
          if (!support || !support.supported) {
            state.failed = true;
            state.errorMessage = 'isConfigSupported rejected config for ' + codec;
            console.warn('WASM WebCodecs: isConfigSupported rejected', codec, support);
            WebCodecsBridge.publishState(state);
          }
        }).catch((error) => {
          state.failed = true;
          state.errorMessage = 'isConfigSupported threw: ' + String(error);
          WebCodecsBridge.publishState(state);
        });
      } catch (probeError) {
        console.warn('WASM WebCodecs: isConfigSupported threw synchronously', probeError);
      }

      state.decoder.configure(config);
      state.configured = true;
      registry.set(id, state);
      WebCodecsBridge.publishState(state);

      console.info('WASM WebCodecs: configured VideoDecoder', {
        codec, annexB: !!annexB, descriptionBytes: extraSize, width, height,
      });
      return id;
    } catch (e) {
      console.warn('WASM WebCodecs: create/configure decoder failed', e);
      // The state never reached the registry, so webcodecs_destroy_decoder cannot
      // reach the decoder to close it.
      try {
        if (state.decoder) state.decoder.close();
      } catch (closeError) {
        console.warn('WASM WebCodecs: decoder close failed', closeError);
      }
      return 0;
    }
  },

  // ---------------------------------------------------------------------------
  webcodecs_destroy_decoder__deps: ['$WebCodecsBridge'],
  webcodecs_destroy_decoder__proxy: 'sync',
  webcodecs_destroy_decoder__sig: 'vi',
  webcodecs_destroy_decoder: function(handle) {
    const state = WebCodecsBridge.getState(handle);
    if (!state) return;
    state.generation += 1;
    for (const entry of state.frames) entry.frame.close();
    state.frames.length = 0;
    try {
      if (state.decoder) state.decoder.close();
    } catch (e) {
      console.warn('WASM WebCodecs: decoder close failed', e);
    }
    // A copy still in flight finishes on its own; it must not publish into
    // memory the codec may have freed by then.
    state.sharedPtr = 0;
    WebCodecsBridge.registry.delete(handle);
  },

  // ---------------------------------------------------------------------------
  webcodecs_reset_decoder__deps: ['$WebCodecsBridge'],
  webcodecs_reset_decoder__proxy: 'sync',
  webcodecs_reset_decoder__sig: 'ii',
  webcodecs_reset_decoder: function(handle) {
    const state = WebCodecsBridge.getState(handle);
    if (!state || !state.decoder) return 0;
    try {
      state.decoder.reset();
      state.generation += 1;
      for (const entry of state.frames) entry.frame.close();
      state.frames.length = 0;
      state.flushing = false;
      state.droppedFrames = 0;
      state.highWaterMark = 0;
      state.failed = false;
      state.errorMessage = '';
      // reset() returns the decoder to 'unconfigured'; we must configure again.
      state.decoder.configure(WebCodecsBridge.buildConfig(state, 0, 0));
      WebCodecsBridge.publishState(state);
      return 1;
    } catch (e) {
      state.failed = true;
      state.errorMessage = 'reset failed: ' + String(e);
      WebCodecsBridge.publishState(state);
      return 0;
    }
  },

  // ---------------------------------------------------------------------------
  webcodecs_flush_decoder__deps: ['$WebCodecsBridge'],
  webcodecs_flush_decoder__proxy: 'sync',
  webcodecs_flush_decoder__sig: 'ii',
  webcodecs_flush_decoder: function(handle) {
    const B = WebCodecsBridge;
    const state = B.getState(handle);
    if (!state || state.failed || !state.decoder || state.decoder.state !== 'configured')
      return 0;
    if (state.flushing)
      return 1;

    const generation = state.generation;
    state.flushing = true;
    state.decoder.flush().then(() => {
      if (state.generation !== generation) return;
      state.flushing = false;
      B.publishState(state);
    }).catch((e) => {
      // reset() rejects a pending flush with AbortError; that is not a failure.
      if (state.generation !== generation) return;
      state.flushing = false;
      if (!e || e.name !== 'AbortError') {
        state.failed = true;
        state.errorMessage = 'flush failed: ' + String(e);
      }
      B.publishState(state);
    });
    B.publishState(state);
    return 1;
  },

  // ---------------------------------------------------------------------------
  webcodecs_push_packet__deps: ['$WebCodecsBridge'],
  webcodecs_push_packet__proxy: 'sync',
  webcodecs_push_packet__sig: 'iiiiidd',
  webcodecs_push_packet: function(handle, dataPtr, dataSize, keyFrame, ptsSeconds, durationSeconds) {
    const B = WebCodecsBridge;
    const state = B.getState(handle);
    if (!state) return B.PUSH_HANDLE_NOT_FOUND;
    if (state.failed) return B.PUSH_DECODER_FAILED;
    if (!state.decoder || state.decoder.state !== 'configured') {
      if (!state.errorMessage)
        state.errorMessage = 'decoder not configured (state=' +
          (state.decoder ? state.decoder.state : 'null') + ')';
      return B.PUSH_NOT_CONFIGURED;
    }

    if (dataSize <= 0)
      return B.PUSH_EMPTY;

    if (B.inflight(state) >= B.MAX_INFLIGHT)
      return B.PUSH_BUSY;

    const tsMicros = Math.round(ptsSeconds * B.MICROSECONDS_PER_SECOND);
    const durMicros = Math.max(0, Math.round(durationSeconds * B.MICROSECONDS_PER_SECOND));
    const payload = HEAPU8.slice(dataPtr, dataPtr + dataSize);

    try {
      state.decoder.decode(new EncodedVideoChunk({
        type: keyFrame ? 'key' : 'delta',
        timestamp: tsMicros,
        duration: durMicros > 0 ? durMicros : undefined,
        data: payload,
      }));
      B.publishState(state);
      return B.PUSH_QUEUED;
    } catch (e) {
      state.failed = true;
      state.errorMessage = 'decode threw: ' + String(e);
      B.publishState(state);
      return B.PUSH_DECODE_THREW;
    }
  },

  // ---------------------------------------------------------------------------
  webcodecs_copy_next_frame__deps: ['$WebCodecsBridge'],
  webcodecs_copy_next_frame__proxy: 'sync',
  webcodecs_copy_next_frame__sig: 'iiiii',
  webcodecs_copy_next_frame: function(handle, dstPtr, dstSize, infoPtr) {
    const B = WebCodecsBridge;
    const state = B.getState(handle);
    if (!state) return 0;
    if (state.failed) return -1;
    if (state.copying || state.frames.length === 0) return 0;

    const entry = state.frames[0];
    B.writeFrameInfo(infoPtr, entry);
    if (entry.payloadSize > dstSize)
      return -2;

    state.frames.shift();
    state.copying = true;
    state.copyResult = 0;
    const generation = state.generation;
    B.publishState(state);

    B.copyFrame(state, entry, dstPtr).then(() => {
      state.copyResult = 1;
    }, (e) => {
      state.copyResult = -1;
      if (state.generation === generation) {
        state.failed = true;
        state.errorMessage = 'frame copy failed: ' + String(e);
      }
    }).finally(() => {
      entry.frame.close();
      state.copying = false;
      B.publishState(state);
    });
    return 1;
  },

  // ---------------------------------------------------------------------------
  webcodecs_read_stats__deps: ['$WebCodecsBridge'],
  webcodecs_read_stats__proxy: 'sync',
  webcodecs_read_stats__sig: 'iiii',
  webcodecs_read_stats: function(handle, droppedPtr, highWaterPtr) {
    const state = WebCodecsBridge.getState(handle);
    if (!state) return 0;
    HEAP32[droppedPtr >> 2] = state.droppedFrames | 0;
    HEAP32[highWaterPtr >> 2] = state.highWaterMark | 0;
    return 1;
  },

  // ---------------------------------------------------------------------------
  webcodecs_take_error__deps: ['$WebCodecsBridge'],
  webcodecs_take_error__proxy: 'sync',
  webcodecs_take_error__sig: 'iiii',
  webcodecs_take_error: function(handle, dstPtr, dstSize) {
    if (dstSize <= 1) return 0;
    const state = WebCodecsBridge.getState(handle);
    if (!state) return 0;
    const decoderState = state.decoder ? state.decoder.state : 'none';
    const message = state.errorMessage || '';
    if (!message && decoderState === 'configured')
      return 0;

    state.errorMessage = '';
    const text = decoderState + '|' + message;
    const encoded = new TextEncoder().encode(text);
    const maxCopy = Math.min(encoded.length, dstSize - 1);
    HEAPU8.set(encoded.subarray(0, maxCopy), dstPtr);
    HEAPU8[dstPtr + maxCopy] = 0;
    return maxCopy;
  },

});
