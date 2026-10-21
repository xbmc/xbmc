/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

//
// Emscripten JS library implementing the WebCodecs bridge declared in
// DVDVideoCodecWebCodecsBridge.h. Linked via --js-library (see
// cmake/scripts/wasm/ArchSetup.cmake).
//
// Every exported function carries a __proxy attribute so calls from any pthread
// are marshalled to the main browser thread, where the VideoDecoder lives. The
// per-frame calls are 'async' and report through WebCodecsSharedState, which
// the C++ side polls with Atomics instead of a round trip (see publishState).
//
// Output frames get a per-decoder sequence number. The VideoFrame stays in a
// Map under that number and its metadata goes into the ring inside the shared
// state, written before framesProduced is published, so the codec takes frames
// without a call into JS and names them by number for upload, copy or release.
//
// Function signatures must match the C prototypes; mismatched __sig will
// silently corrupt arguments on threaded builds.
//

mergeInto(LibraryManager.library, {

  // ---------------------------------------------------------------------------
  // Internal module: shared state + helpers, attached only if any bridge
  // function is used (Emscripten dead-strips unreferenced $-symbols).
  // ---------------------------------------------------------------------------
  $WebCodecsBridge : {
    MICROSECONDS_PER_SECOND : 1000000.0,
    FRAME_RING : 32,

    // Enum values below are pulled from the C++-owned Embind registrations on
    // first use by syncEnumsFromEmbind(). The C++ header
    // (DVDVideoCodecWebCodecsBridge.h) and FFmpeg's pixfmt.h are the source of
    // truth; do not hardcode these here.
    pixelFormats : null, // VideoFrame.format string -> WebCodecsPixelFormat
    colorMatrix : null, // VideoColorSpace.matrix -> AVColorSpace
    colorPrimaries : null,
    colorTransfer : null,
    colorMatrixUnspecified : 0,
    colorPrimariesUnspecified : 0,
    colorTransferUnspecified : 0,
    COPY_OK : 0,
    COPY_FAILED : 0,
    COPY_DST_TOO_SMALL : 0,
    COPY_NO_FRAME : 0,
    _enumsReady : false,

    // Byte offsets inside struct WebCodecsFrameInfo (kept in sync with the
    // static_asserts in DVDVideoCodecWebCodecsBridge.h).
    FI_SIZE : 80,
    FI_WIDTH : 0,
    FI_HEIGHT : 4,
    FI_DISPLAY_WIDTH : 8,
    FI_DISPLAY_HEIGHT : 12,
    FI_PIXFMT : 16,
    FI_KEYFRAME : 20,
    FI_COLOR_MATRIX : 24,
    FI_COLOR_PRIMARIES : 28,
    FI_COLOR_TRANSFER : 32,
    FI_FULL_RANGE : 36,
    FI_PAYLOAD_SIZE : 40,
    FI_Y_STRIDE : 44,
    FI_U_STRIDE : 48,
    FI_V_STRIDE : 52,
    FI_U_OFFSET : 56,
    FI_V_OFFSET : 60,
    FI_PTS : 64,
    FI_DURATION : 72,

    // Int32 indices inside struct WebCodecsSharedState (offsets / 4), and the
    // byte offset of the ring.
    SS_SIGNAL : 0,
    SS_FRAMES_PRODUCED : 1,
    SS_FRAMES_TAKEN : 2,
    SS_INFLIGHT : 3,
    SS_FAILED : 4,
    SS_PUSHES_PROCESSED : 5,
    SS_COPY_DONE : 6,
    SS_COPY_RESULT : 7,
    SS_OPEN_FRAMES : 8,
    SS_DECODING : 9,
    SS_CONFIGURED : 10,
    SS_HARDWARE : 11,
    SS_RING_OFFSET : 48,

    // Result of webcodecs_probe_texture_upload, null until it has run.
    textureUpload : null,

    // Registry is lazily created on first decoder creation; this library file
    // is merged into both the main thread and every pthread module, but only
    // the main thread ever touches the maps (every entry is proxied).
    registry : null,
    nextId : 1,

    ensureRegistry : function() {
      if (!this.registry)
        this.registry = new Map();
      return this.registry;
    },

    // Pull the canonical enum numeric values out of the Embind bindings
    // registered by EMSCRIPTEN_BINDINGS(kodi_webcodecs_bridge) in
    // DVDVideoCodecWebCodecs.cpp. Embind exposes each C++ enum value as an
    // object with a .value property holding the underlying integer.
    syncEnumsFromEmbind : function() {
      if (this._enumsReady)
        return;
      const pf = Module['WebCodecsPixelFormat'];
      const cr = Module['WebCodecsCopyResult'];
      const spc = Module['AVColorSpace'];
      const pri = Module['AVColorPrimaries'];
      const trc = Module['AVColorTransferCharacteristic'];
      if (!pf || !cr || !spc || !pri || !trc)
        throw new Error('WebCodecs bridge enums missing from Module (Embind not linked?)');

      this.pixelFormats = {};
      for (const name of Object.keys(pf))
      {
        if (pf[name] && typeof pf[name].value === 'number')
          this.pixelFormats[name] = pf[name].value;
      }
      this.COPY_OK = cr.OK.value;
      this.COPY_FAILED = cr.FAILED.value;
      this.COPY_DST_TOO_SMALL = cr.DST_TOO_SMALL.value;
      this.COPY_NO_FRAME = cr.NO_FRAME.value;

      // VideoColorSpace names on the left, FFmpeg's on the right.
      this.colorMatrix = {
        rgb : spc.RGB.value,
        bt709 : spc.BT709.value,
        bt470bg : spc.BT470BG.value,
        smpte170m : spc.SMPTE170M.value,
        'bt2020-ncl' : spc.BT2020_NCL.value,
      };
      this.colorMatrixUnspecified = spc.UNSPECIFIED.value;
      this.colorPrimaries = {
        bt709 : pri.BT709.value,
        bt470bg : pri.BT470BG.value,
        smpte170m : pri.SMPTE170M.value,
        bt2020 : pri.BT2020.value,
        smpte432 : pri.SMPTE432.value,
      };
      this.colorPrimariesUnspecified = pri.UNSPECIFIED.value;
      this.colorTransfer = {
        bt709 : trc.BT709.value,
        smpte170m : trc.SMPTE170M.value,
        'iec61966-2-1' : trc.IEC61966_2_1.value,
        linear : trc.LINEAR.value,
        pq : trc.SMPTE2084.value,
        hlg : trc.ARIB_STD_B67.value,
      };
      this.colorTransferUnspecified = trc.UNSPECIFIED.value;
      this._enumsReady = true;
    },

    getState : function(handle) { return this.registry ? this.registry.get(handle) : null; },

    framesTaken : function(
        state) { return Atomics.load(HEAP32, (state.sharedPtr >> 2) + this.SS_FRAMES_TAKEN); },

    inflight : function(state) {
      const decoder = state.decoder;
      const queueSize = decoder && decoder.state === 'configured' ? decoder.decodeQueueSize : 0;
      return queueSize + (state.copying ? 1 : 0);
    },

    publishState : function(state) {
      if (!state.sharedPtr)
        return;
      const base = state.sharedPtr >> 2;
      Atomics.store(HEAP32, base + this.SS_FRAMES_PRODUCED, state.framesProduced);
      Atomics.store(HEAP32, base + this.SS_INFLIGHT, this.inflight(state));
      Atomics.store(HEAP32, base + this.SS_FAILED, state.failed ? 1 : 0);
      Atomics.store(HEAP32, base + this.SS_PUSHES_PROCESSED, state.pushesProcessed);
      // copyDone tells the codec that copyResult belongs to its copy.
      Atomics.store(HEAP32, base + this.SS_COPY_RESULT, state.copyResult);
      Atomics.store(HEAP32, base + this.SS_COPY_DONE, state.copyDone);
      Atomics.store(HEAP32, base + this.SS_OPEN_FRAMES, state.frames.size);
      Atomics.store(HEAP32, base + this.SS_DECODING,
                    state.pendingChunks.length + (state.copying ? 1 : 0));
      Atomics.store(HEAP32, base + this.SS_CONFIGURED, state.configured ? 1 : 0);
      Atomics.store(HEAP32, base + this.SS_HARDWARE,
                    state.configured && state.hardwareAcceleration === 'prefer-hardware' ? 1 : 0);
      Atomics.add(HEAP32, base + this.SS_SIGNAL, 1);
      Atomics.notify(HEAP32, base + this.SS_SIGNAL);
    },

    // Tightly packed plane layout for the formats the sysmem copy path accepts;
    // null for every other format.
    copyLayout : function(format, width, height) {
      if (format === 'RGBA' || format === 'RGBX' || format === 'BGRA' || format === 'BGRX')
      {
        const stride = width * 4;
        return {
          payloadSize : stride * height,
          yStride : stride,
          uStride : 0,
          vStride : 0,
          uOffset : 0,
          vOffset : 0,
          planes : [ {offset : 0, stride} ],
        };
      }

      const yStride = width;
      const uvHeight = (height + 1) >> 1;
      const ySize = yStride * height;

      if (format === 'NV12')
      {
        const uvStride = ((width + 1) >> 1) * 2;
        const uvSize = uvStride * uvHeight;
        return {
          payloadSize : ySize + uvSize,
          yStride,
          uStride : uvStride,
          vStride : 0,
          uOffset : ySize,
          vOffset : 0,
          planes : [ {offset : 0, stride : yStride}, {offset : ySize, stride : uvStride} ],
        };
      }

      if (format === 'I420')
      {
        const uvStride = (width + 1) >> 1;
        const uvSize = uvStride * uvHeight;
        return {
          payloadSize : ySize + uvSize * 2,
          yStride,
          uStride : uvStride,
          vStride : uvStride,
          uOffset : ySize,
          vOffset : ySize + uvSize,
          planes : [
            {offset : 0, stride : yStride},
            {offset : ySize, stride : uvStride},
            {offset : ySize + uvSize, stride : uvStride},
          ],
        };
      }

      return null;
    },

    // Removes the chunk an output frame came from, which kept its timestamp,
    // and returns it, or null. Frames are output in presentation order, so
    // chunks with an earlier timestamp will never be output (e.g. VP8 alt-ref
    // frames) and are removed too.
    retireChunks : function(state, timestampMicros) {
      let retired = null;
      state.pendingChunks = state.pendingChunks.filter((chunk) => {
        if (!retired && chunk.timestamp === timestampMicros)
        {
          retired = chunk;
          return false;
        }
        return chunk.timestamp >= timestampMicros;
      });
      return retired;
    },

    // Everything the codec needs to know about an output frame, in the form
    // writeFrameInfo() stores into a ring slot.
    describeFrame : function(state, frame, chunk) {
      const visibleRect = frame.visibleRect || null;
      const width = visibleRect ? visibleRect.width : frame.codedWidth;
      const height = visibleRect ? visibleRect.height : frame.codedHeight;
      const timestampMicros =
          Number.isFinite(frame.timestamp) ? Number(frame.timestamp) : state.lastTimestamp;
      const durationMicros = Number.isFinite(frame.duration) ? Number(frame.duration) : 0;
      state.lastTimestamp = timestampMicros;

      const format = frame.format || '';
      const colorSpace = frame.colorSpace || {};
      const lookup = (table, key, fallback) =>
          key != null && Object.prototype.hasOwnProperty.call(table, key) ? table[key] : fallback;
      const pixelFormat =
          frame.format ? lookup(this.pixelFormats, format, 0) : this.pixelFormats.opaque;

      return {
        frame,
        visibleRect,
        width,
        height,
        displayWidth : frame.displayWidth || width,
        displayHeight : frame.displayHeight || height,
        pixelFormat,
        keyFrame : !!(chunk && chunk.keyFrame),
        colorMatrix : lookup(this.colorMatrix, colorSpace.matrix, this.colorMatrixUnspecified),
        colorPrimaries :
            lookup(this.colorPrimaries, colorSpace.primaries, this.colorPrimariesUnspecified),
        colorTransfer :
            lookup(this.colorTransfer, colorSpace.transfer, this.colorTransferUnspecified),
        fullRange : colorSpace.fullRange === true    ? 1
                    : colorSpace.fullRange === false ? 0
                                                     : -1,
        // Only the copy path reads the layout; the texture path is spared the allocation.
        layout : this.textureUpload === 1 ? null : this.copyLayout(format, width, height),
        // A frame reordered past later chunks finds its own retired already.
        ptsSeconds : (chunk ? chunk.synthetic : state.syntheticTimestamps)
                         ? NaN
                         : timestampMicros / this.MICROSECONDS_PER_SECOND,
        durationSeconds : durationMicros / this.MICROSECONDS_PER_SECOND,
      };
    },

    writeFrameInfo : function(slotPtr, entry) {
      const layout = entry.layout;
      HEAP32[(slotPtr + this.FI_WIDTH) >> 2] = entry.width | 0;
      HEAP32[(slotPtr + this.FI_HEIGHT) >> 2] = entry.height | 0;
      HEAP32[(slotPtr + this.FI_DISPLAY_WIDTH) >> 2] = entry.displayWidth | 0;
      HEAP32[(slotPtr + this.FI_DISPLAY_HEIGHT) >> 2] = entry.displayHeight | 0;
      HEAP32[(slotPtr + this.FI_PIXFMT) >> 2] = entry.pixelFormat | 0;
      HEAP32[(slotPtr + this.FI_KEYFRAME) >> 2] = entry.keyFrame ? 1 : 0;
      HEAP32[(slotPtr + this.FI_COLOR_MATRIX) >> 2] = entry.colorMatrix | 0;
      HEAP32[(slotPtr + this.FI_COLOR_PRIMARIES) >> 2] = entry.colorPrimaries | 0;
      HEAP32[(slotPtr + this.FI_COLOR_TRANSFER) >> 2] = entry.colorTransfer | 0;
      HEAP32[(slotPtr + this.FI_FULL_RANGE) >> 2] = entry.fullRange | 0;
      HEAP32[(slotPtr + this.FI_PAYLOAD_SIZE) >> 2] = layout ? layout.payloadSize | 0 : 0;
      HEAP32[(slotPtr + this.FI_Y_STRIDE) >> 2] = layout ? layout.yStride | 0 : 0;
      HEAP32[(slotPtr + this.FI_U_STRIDE) >> 2] = layout ? layout.uStride | 0 : 0;
      HEAP32[(slotPtr + this.FI_V_STRIDE) >> 2] = layout ? layout.vStride | 0 : 0;
      HEAP32[(slotPtr + this.FI_U_OFFSET) >> 2] = layout ? layout.uOffset | 0 : 0;
      HEAP32[(slotPtr + this.FI_V_OFFSET) >> 2] = layout ? layout.vOffset | 0 : 0;
      HEAPF64[(slotPtr + this.FI_PTS) >> 3] = entry.ptsSeconds;
      HEAPF64[(slotPtr + this.FI_DURATION) >> 3] = entry.durationSeconds;
    },

    closeFrames : function(state, fromSequence) {
      for (const [sequence, entry] of state.frames)
      {
        if (sequence >= fromSequence)
        {
          entry.frame.close();
          state.frames.delete(sequence);
        }
      }
    },

    // Copies straight into wasm memory: under pthreads the heap is a
    // SharedArrayBuffer and existing views stay valid across growth. Browsers
    // whose copyTo() still rejects shared views fall back to a scratch buffer
    // plus one memcpy.
    copyFrame : async function(state, entry, dstPtr) {
      const size = entry.layout.payloadSize;
      const options = {layout : entry.layout.planes};
      if (entry.visibleRect)
        options.rect = entry.visibleRect;

      if (state.directCopy)
      {
        try
        {
          await entry.frame.copyTo(HEAPU8.subarray(dstPtr, dstPtr + size), options);
          return;
        }
        catch (e)
        {
          if (!(e instanceof TypeError))
            throw e;
          state.directCopy = false;
        }
      }

      if (!state.scratch || state.scratch.byteLength < size)
        state.scratch = new Uint8Array(size);
      const scratch = state.scratch.subarray(0, size);
      await entry.frame.copyTo(scratch, options);
      HEAPU8.set(scratch, dstPtr);
    },

    newDecoder : function(state) {
      state.decoder = new VideoDecoder({output : state.onOutput, error : state.onError});
      state.decoder.addEventListener('dequeue', () => WebCodecsBridge.publishState(state));
    },

    // Build the configure() dictionary from the stored codec parameters.
    buildConfig : function(state, width, height) {
      const config = {
        codec : state.codec,
        optimizeForLatency : true,
        hardwareAcceleration : state.hardwareAcceleration,
      };
      if (width > 0)
        config.codedWidth = width;
      if (height > 0)
        config.codedHeight = height;
      if (state.codec.startsWith('avc1'))
        config.avc = {format : state.annexB ? 'annexb' : 'avc'};
      else if (state.codec.startsWith('hvc1') || state.codec.startsWith('hev1'))
        config.hevc = {format : state.annexB ? 'annexb' : 'hevc'};
      if (state.description)
        config.description = state.description;
      return config;
    },

    // Chrome reads 'prefer-hardware' as hardware only, so a codec its platform
    // decoder lacks (VP8 on macOS, say) is refused outright; 'no-preference'
    // still picks the hardware decoder when there is one. Resolves to the
    // first preference isConfigSupported() accepts, or null.
    selectHardwareAcceleration : async function(state, width, height) {
      for (const preference of ['prefer-hardware', 'no-preference'])
      {
        state.hardwareAcceleration = preference;
        const support =
            await VideoDecoder.isConfigSupported(this.buildConfig(state, width, height));
        if (support && support.supported)
          return preference;
      }
      return null;
    },
  },

  // ---------------------------------------------------------------------------
  // webcodecs_probe_texture_upload: does texImage2D take a VideoFrame here?
  // Meant to run before the GL command stream has anything in it, so the
  // texture binding it leaves behind (none) is the initial state.
  // ---------------------------------------------------------------------------
  webcodecs_probe_texture_upload__deps : [ '$WebCodecsBridge', '$GL' ],
  webcodecs_probe_texture_upload__proxy : 'sync',
  webcodecs_probe_texture_upload__sig : 'i',
  webcodecs_probe_texture_upload : function() {
    const B = WebCodecsBridge;
    if (B.textureUpload !== null)
      return B.textureUpload;
    B.textureUpload = 0;

    const gl = GL.currentContext ? GL.currentContext.GLctx : null;
    if (!gl || typeof VideoFrame === 'undefined')
    {
      console.info('WASM WebCodecs: no WebGL context or no VideoFrame, frames will be copied');
      return 0;
    }

    let frame = null;
    let texture = null;
    try
    {
      frame = new VideoFrame(new Uint8Array(2 * 2 * 4),
                             {format : 'RGBA', codedWidth : 2, codedHeight : 2, timestamp : 0});
      texture = gl.createTexture();
      gl.bindTexture(gl.TEXTURE_2D, texture);
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, frame);
      B.textureUpload = gl.getError() === gl.NO_ERROR ? 1 : 0;
    }
    catch (e)
    {
      console.warn('WASM WebCodecs: texImage2D(VideoFrame) threw', e);
    }
    finally
    {
      if (frame)
        frame.close();
      gl.bindTexture(gl.TEXTURE_2D, null);
      if (texture)
        gl.deleteTexture(texture);
    }
    console.info('WASM WebCodecs: texImage2D(VideoFrame)',
                 B.textureUpload ? 'supported' : 'unsupported');
    return B.textureUpload;
  },

  // ---------------------------------------------------------------------------
  // webcodecs_create_decoder: build + configure a VideoDecoder.
  // sig: i (ret) | string*, i, i, u8*, i, i, WebCodecsSharedState*
  // ---------------------------------------------------------------------------
  webcodecs_create_decoder__deps : [ '$WebCodecsBridge' ],
  webcodecs_create_decoder__proxy : 'sync',
  webcodecs_create_decoder__sig : 'iiiiiiii',
  webcodecs_create_decoder : function(codecPtr, width, height, extraPtr, extraSize, annexB,
                                      sharedPtr) {
    if (typeof VideoDecoder === 'undefined' || typeof EncodedVideoChunk === 'undefined')
    {
      console.warn('WASM WebCodecs: VideoDecoder / EncodedVideoChunk not available');
      return 0;
    }

    try
    {
      WebCodecsBridge.syncEnumsFromEmbind();
    }
    catch (e)
    {
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
      annexB : !!annexB,
      failed : false,
      errorMessage : '',
      lastTimestamp : 0,
      lastChunkTimestamp : 0,
      syntheticTimestamps : false,
      pendingChunks : [], // { timestamp, keyFrame, synthetic } of chunks still being decoded
      frames : new Map(), // sequence -> describeFrame() entry
      framesProduced : 0,
      copying : false,
      copyDone : 0,
      copyResult : 0,
      pushesProcessed : 0,
      scratch : null,
      directCopy : true,
      generation : 0,
      droppedFrames : 0,
      highWaterMark : 0,
      uploadFailed : false,
      decoder : null,
      description : null,
      hardwareAcceleration : 'prefer-hardware',
      configured : false,
    };

    const errorCallback = (error) => {
      state.failed = true;
      state.errorMessage =
          'VideoDecoder error: ' + (error && error.message ? error.message : error);
      console.warn('WASM WebCodecs:', state.errorMessage);
      WebCodecsBridge.publishState(state);
    };

    const outputCallback = (frame) => {
      const B = WebCodecsBridge;
      if (!state.sharedPtr)
      {
        frame.close();
        return;
      }
      const chunk = B.retireChunks(state, frame.timestamp);

      // The codec's in-flight cap keeps the queue far below the ring size.
      const taken = B.framesTaken(state);
      if (state.framesProduced - taken >= B.FRAME_RING)
      {
        state.droppedFrames += 1;
        frame.close();
        B.publishState(state);
        return;
      }

      const sequence = state.framesProduced;
      const entry = B.describeFrame(state, frame, chunk);
      B.writeFrameInfo(state.sharedPtr + B.SS_RING_OFFSET + (sequence % B.FRAME_RING) * B.FI_SIZE,
                       entry);
      state.frames.set(sequence, entry);
      state.framesProduced = sequence + 1;
      if (state.framesProduced - taken > state.highWaterMark)
        state.highWaterMark = state.framesProduced - taken;
      B.publishState(state);
    };

    try
    {
      if (extraSize > 0)
        state.description = HEAPU8.slice(extraPtr, extraPtr + extraSize);

      state.onOutput = outputCallback;
      state.onError = errorCallback;
      WebCodecsBridge.newDecoder(state);
      registry.set(id, state);
      WebCodecsBridge.publishState(state);

      WebCodecsBridge.selectHardwareAcceleration(state, width, height)
          .then((preference) => {
            // Destroyed or failed while the probe ran.
            if (!state.sharedPtr || state.failed)
              return;
            if (!preference)
            {
              state.failed = true;
              state.errorMessage = 'isConfigSupported rejected every config for ' + codec;
              console.warn('WASM WebCodecs:', state.errorMessage);
              WebCodecsBridge.publishState(state);
              return;
            }
            state.decoder.configure(WebCodecsBridge.buildConfig(state, width, height));
            state.configured = true;
            WebCodecsBridge.publishState(state);
            console.info('WASM WebCodecs: configured VideoDecoder', {
              codec,
              hardwareAcceleration : preference,
              annexB : !!annexB,
              descriptionBytes : extraSize,
              width,
              height,
            });
          })
          .catch((error) => {
            if (!state.sharedPtr)
              return;
            state.failed = true;
            state.errorMessage = 'configure failed: ' + String(error);
            console.warn('WASM WebCodecs:', state.errorMessage);
            WebCodecsBridge.publishState(state);
          });
      return id;
    }
    catch (e)
    {
      console.warn('WASM WebCodecs: VideoDecoder construction failed', e);
      return 0;
    }
  },

  // ---------------------------------------------------------------------------
  webcodecs_destroy_decoder__deps : [ '$WebCodecsBridge' ],
  webcodecs_destroy_decoder__proxy : 'sync',
  webcodecs_destroy_decoder__sig : 'vi',
  webcodecs_destroy_decoder : function(handle) {
    const state = WebCodecsBridge.getState(handle);
    if (!state)
      return;
    state.generation += 1;
    state.pendingChunks = [];
    WebCodecsBridge.closeFrames(state, 0);
    try
    {
      if (state.decoder)
        state.decoder.close();
    }
    catch (e)
    {
      console.warn('WASM WebCodecs: decoder close failed', e);
    }
    // A copy still in flight finishes on its own; it must not publish into
    // memory the codec may have freed by then.
    state.sharedPtr = 0;
    WebCodecsBridge.registry.delete(handle);
  },

  // ---------------------------------------------------------------------------
  // Frames the codec has taken belong to Kodi buffers and are closed by their
  // release; only the untaken ones are closed here. The codec sets
  // framesTaken = framesProduced when this returns.
  webcodecs_reset_decoder__deps : [ '$WebCodecsBridge' ],
  webcodecs_reset_decoder__proxy : 'sync',
  webcodecs_reset_decoder__sig : 'ii',
  webcodecs_reset_decoder : function(handle) {
    const B = WebCodecsBridge;
    const state = B.getState(handle);
    if (!state || !state.decoder)
      return 0;
    try
    {
      // An error closes the VideoDecoder for good; a new one takes its place.
      if (state.decoder.state === 'closed')
        B.newDecoder(state);
      else
        state.decoder.reset();
      state.generation += 1;
      state.pendingChunks = [];
      B.closeFrames(state, B.framesTaken(state));
      state.droppedFrames = 0;
      state.highWaterMark = 0;
      state.failed = false;
      state.errorMessage = '';
      // reset() returns the decoder to 'unconfigured'; we must configure again.
      state.decoder.configure(B.buildConfig(state, 0, 0));
      B.publishState(state);
      return 1;
    }
    catch (e)
    {
      state.failed = true;
      state.errorMessage = 'reset failed: ' + String(e);
      B.publishState(state);
      return 0;
    }
  },

  // ---------------------------------------------------------------------------
  // Asynchronous: the video thread never waits for the main thread here. The
  // caller malloc'd the packet and this side frees it.
  webcodecs_push_packet__deps : [ '$WebCodecsBridge', 'free' ],
  webcodecs_push_packet__proxy : 'async',
  webcodecs_push_packet__sig : 'viiiidd',
  webcodecs_push_packet : function(handle, dataPtr, dataSize, keyFrame, ptsSeconds,
                                   durationSeconds) {
    const B = WebCodecsBridge;
    const payload = HEAPU8.slice(dataPtr, dataPtr + dataSize);
    _free(dataPtr);
    const state = B.getState(handle);
    if (!state)
      return;

    state.pushesProcessed += 1;
    if (!state.failed)
    {
      if (!state.decoder || state.decoder.state !== 'configured')
      {
        state.failed = true;
        state.errorMessage =
            'decoder not configured (state=' + (state.decoder ? state.decoder.state : 'null') + ')';
      }
      else
      {
        // A packet without timestamp follows the previous chunk by 1 us, so
        // retireChunks() can still tell the chunks apart.
        const synthetic = !Number.isFinite(ptsSeconds);
        const tsMicros = synthetic ? state.lastChunkTimestamp + 1
                                   : Math.round(ptsSeconds * B.MICROSECONDS_PER_SECOND);
        const durMicros = Math.max(0, Math.round(durationSeconds * B.MICROSECONDS_PER_SECOND));
        try
        {
          state.decoder.decode(new EncodedVideoChunk({
            type : keyFrame ? 'key' : 'delta',
            timestamp : tsMicros,
            duration : durMicros > 0 ? durMicros : undefined,
            data : payload,
          }));
          state.pendingChunks.push({timestamp : tsMicros, keyFrame : !!keyFrame, synthetic});
          state.lastChunkTimestamp = tsMicros;
          state.syntheticTimestamps = synthetic;
        }
        catch (e)
        {
          state.failed = true;
          state.errorMessage = 'decode threw: ' + String(e);
        }
      }
    }
    B.publishState(state);
  },

  // ---------------------------------------------------------------------------
  // Asynchronous. Runs in the render thread's GL command stream: the caller
  // enqueues it before the glBindTexture and draw that use the texture, and
  // Emscripten's proxying queue keeps that order. Binds on whichever texture
  // unit is active, as the caller's own glBindTexture would.
  webcodecs_upload_frame__deps : [ '$WebCodecsBridge', '$GL' ],
  webcodecs_upload_frame__proxy : 'async',
  webcodecs_upload_frame__sig : 'viii',
  webcodecs_upload_frame : function(handle, sequence, glTexture) {
    const B = WebCodecsBridge;
    const state = B.getState(handle);
    if (!state)
      return;
    const entry = state.frames.get(sequence);
    if (!entry)
      return;

    try
    {
      const gl = GL.currentContext ? GL.currentContext.GLctx : null;
      const texture = GL.textures[glTexture];
      if (gl && texture)
      {
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, entry.frame);
      }
    }
    catch (e)
    {
      if (!state.uploadFailed)
      {
        state.uploadFailed = true;
        console.warn('WASM WebCodecs: texImage2D(VideoFrame) failed', e);
      }
      state.failed = true;
      state.errorMessage = 'texture upload failed: ' + String(e);
      B.publishState(state);
    }
  },

  // ---------------------------------------------------------------------------
  webcodecs_release_frame__deps : [ '$WebCodecsBridge' ],
  webcodecs_release_frame__proxy : 'async',
  webcodecs_release_frame__sig : 'vii',
  webcodecs_release_frame : function(handle, sequence) {
    const state = WebCodecsBridge.getState(handle);
    if (!state)
      return;
    const entry = state.frames.get(sequence);
    if (!entry)
      return;
    state.frames.delete(sequence);
    entry.frame.close();
    WebCodecsBridge.publishState(state);
  },

  // ---------------------------------------------------------------------------
  // Sysmem fallback. Asynchronous; the outcome lands in copyDone/copyResult.
  webcodecs_copy_frame__deps : [ '$WebCodecsBridge' ],
  webcodecs_copy_frame__proxy : 'async',
  webcodecs_copy_frame__sig : 'viiiii',
  webcodecs_copy_frame : function(handle, sequence, copyId, dstPtr, dstSize) {
    const B = WebCodecsBridge;
    const state = B.getState(handle);
    if (!state)
      return;

    const finish = (result) => {
      state.copyDone = copyId;
      state.copyResult = result;
      B.publishState(state);
    };
    const entry = state.frames.get(sequence);
    if (!entry)
      return finish(B.COPY_NO_FRAME);
    // The codec has taken the frame, so nothing else would close it.
    const reject = (result) => {
      state.frames.delete(sequence);
      entry.frame.close();
      return finish(result);
    };
    if (state.failed || state.copying)
      return reject(B.COPY_FAILED);
    if (!entry.layout || entry.layout.payloadSize > dstSize)
      return reject(B.COPY_DST_TOO_SMALL);

    state.frames.delete(sequence);
    state.copying = true;
    const generation = state.generation;
    let result = B.COPY_FAILED;
    B.copyFrame(state, entry, dstPtr)
        .then(() => { result = B.COPY_OK; },
              (e) => {
                if (state.generation === generation)
                {
                  state.failed = true;
                  state.errorMessage = 'frame copy failed: ' + String(e);
                }
              })
        .finally(() => {
          entry.frame.close();
          state.copying = false;
          finish(result);
        });
  },

  // ---------------------------------------------------------------------------
  webcodecs_read_stats__deps : [ '$WebCodecsBridge' ],
  webcodecs_read_stats__proxy : 'sync',
  webcodecs_read_stats__sig : 'iiii',
  webcodecs_read_stats : function(handle, droppedPtr, highWaterPtr) {
    const state = WebCodecsBridge.getState(handle);
    if (!state)
      return 0;
    HEAP32[droppedPtr >> 2] = state.droppedFrames | 0;
    HEAP32[highWaterPtr >> 2] = state.highWaterMark | 0;
    return 1;
  },

  // ---------------------------------------------------------------------------
  webcodecs_take_error__deps : [ '$WebCodecsBridge' ],
  webcodecs_take_error__proxy : 'sync',
  webcodecs_take_error__sig : 'iiii',
  webcodecs_take_error : function(handle, dstPtr, dstSize) {
    if (dstSize <= 1)
      return 0;
    const state = WebCodecsBridge.getState(handle);
    if (!state)
      return 0;
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
