/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

// Web Audio output for CWasmAudioWorkletManager. The AudioContext lives on the
// browser main thread. The processor in audio_worklet_processor.js reads the
// manager's ring and counters straight from the wasm memory SharedArrayBuffer,
// so the AudioWorklet scope never instantiates the wasm module.
mergeInto(LibraryManager.library, {
  $WasmAudio: {
    ctx: null,
    node: null,
    resumeHandler: null,
    maxChannels: 0,

    // WASM_AUDIO_NODE_* in WasmAudioWorklet.h
    NODE_STARTED: 1,
    NODE_FAILED: 2,

    setResumeHooks: function(handler) {
      const method = handler ? 'addEventListener' : 'removeEventListener';
      const target = handler || this.resumeHandler;
      if (!target)
        return;
      window[method]('pointerdown', target, true);
      window[method]('keydown', target, true);
      window[method]('touchstart', target, true);
      document[method]('visibilitychange', target, true);
      this.resumeHandler = handler;
    },

    // Autoplay policy keeps a context suspended until a user gesture.
    armResume: function(ctx) {
      if (ctx.state === 'running' || this.resumeHandler)
        return;
      const tryResume = () => {
        if (ctx.state !== 'running')
          ctx.resume().catch(() => {});
        if (ctx.state === 'running')
          this.setResumeHooks(null);
      };
      this.setResumeHooks(tryResume);
      // A context suspended for another reason needs no gesture.
      tryResume();
    },
  },

  wasm_audio_open__deps: ['$WasmAudio'],
  wasm_audio_open__proxy: 'sync',
  wasm_audio_open__sig: 'iiiii',
  wasm_audio_open: function(sampleRate, runningFlag, sampleRateOut, quantumSizeOut) {
    let ctx;
    try {
      // The largest output buffer the browser offers; the added latency is part
      // of outputLatency and so of the delay the sink reports.
      ctx = new AudioContext({latencyHint: 'playback', sampleRate: sampleRate || undefined});
    } catch (e) {
      console.error('WASM AudioWorklet: cannot create AudioContext', e);
      return 0;
    }
    WasmAudio.ctx = ctx;
    HEAP32[sampleRateOut >> 2] = ctx.sampleRate;
    HEAP32[quantumSizeOut >> 2] = ctx.renderQuantumSize || 128;

    ctx.onstatechange = () => {
      Atomics.store(HEAPU8, runningFlag, ctx.state === 'running' ? 1 : 0);
      if (ctx.state === 'running')
        WasmAudio.setResumeHooks(null);
      else if (ctx.state !== 'closed')
        WasmAudio.armResume(ctx);
    };
    ctx.onstatechange();
    return 1;
  },

  wasm_audio_start__deps: ['$WasmAudio'],
  wasm_audio_start__proxy: 'sync',
  wasm_audio_start__sig: 'viiiiiiiiiiii',
  wasm_audio_start: function(channels, ring, capacity, prebuffer, ready, prebufferComplete,
                             activeCallbacks, generation, readFrame, writeFrame, underrunFrames,
                             state) {
    const ctx = WasmAudio.ctx;
    // Events of a context that was closed in the meantime are dropped, and a
    // failure is final: the processor's first message may arrive after it.
    const setState = (value) => {
      if (WasmAudio.ctx !== ctx ||
          Atomics.load(HEAP32, state >> 2) === WasmAudio.NODE_FAILED)
        return;
      Atomics.store(HEAP32, state >> 2, value);
      Atomics.notify(HEAP32, state >> 2);
    };
    if (!ctx) {
      Atomics.store(HEAP32, state >> 2, WasmAudio.NODE_FAILED);
      return;
    }

    ctx.audioWorklet.addModule(locateFile('audio_worklet_processor.js'))
        .then(() => {
          if (WasmAudio.ctx !== ctx)
            return;
          const node = new AudioWorkletNode(ctx, 'kodi-audio', {
            numberOfInputs: 0,
            numberOfOutputs: 1,
            outputChannelCount: [channels],
            channelCount: channels,
            channelCountMode: 'explicit',
            channelInterpretation: 'speakers',
            processorOptions: {
              buffer: wasmMemory.buffer,
              channels: channels,
              ring: ring,
              capacity: capacity,
              prebuffer: prebuffer,
              ready: ready,
              prebufferComplete: prebufferComplete,
              activeCallbacks: activeCallbacks,
              currentGeneration: generation,
              generation: Atomics.load(HEAPU32, generation >> 2),
              readFrame: readFrame,
              writeFrame: writeFrame,
              underrunFrames: underrunFrames,
            },
          });
          // AudioDestinationNode.channelCount defaults to 2 regardless of the
          // hardware, which would fold a 5.1 node to stereo.
          try {
            ctx.destination.channelCount = Math.min(channels, ctx.destination.maxChannelCount);
          } catch (e) {
            console.warn('WASM AudioWorklet: cannot set destination channelCount', e);
          }
          // The processor is constructed later, on the audio thread, and posts
          // a message once its views exist.
          node.port.onmessage = () => setState(WasmAudio.NODE_STARTED);
          node.onprocessorerror = (e) => {
            console.error('WASM AudioWorklet: the processor failed', e);
            setState(WasmAudio.NODE_FAILED);
          };
          node.connect(ctx.destination);
          WasmAudio.node = node;
        })
        .catch((e) => {
          console.error('WASM AudioWorklet: cannot start the processor', e);
          setState(WasmAudio.NODE_FAILED);
        });
  },

  wasm_audio_close__deps: ['$WasmAudio'],
  wasm_audio_close__proxy: 'sync',
  wasm_audio_close__sig: 'v',
  wasm_audio_close: function() {
    const ctx = WasmAudio.ctx;
    if (!ctx)
      return;
    WasmAudio.ctx = null;
    WasmAudio.setResumeHooks(null);
    ctx.onstatechange = null;
    if (WasmAudio.node) {
      // An open port with a listener keeps the node and its context alive.
      WasmAudio.node.port.close();
      WasmAudio.node.disconnect();
      WasmAudio.node = null;
    }
    ctx.close().catch(() => {});
  },

  wasm_audio_latency_us__deps: ['$WasmAudio'],
  wasm_audio_latency_us__proxy: 'sync',
  wasm_audio_latency_us__sig: 'i',
  wasm_audio_latency_us: function() {
    const ctx = WasmAudio.ctx;
    if (!ctx)
      return 0;
    let total = 0;
    if (typeof ctx.baseLatency === 'number' && isFinite(ctx.baseLatency))
      total += ctx.baseLatency;
    if (typeof ctx.outputLatency === 'number' && isFinite(ctx.outputLatency))
      total += ctx.outputLatency;
    return total > 0 ? Math.min(Math.floor(total * 1e6), 0x7fffffff) : 0;
  },

  wasm_audio_max_output_channels__deps: ['$WasmAudio'],
  wasm_audio_max_output_channels__proxy: 'sync',
  wasm_audio_max_output_channels__sig: 'i',
  wasm_audio_max_output_channels: function() {
    let ctx = WasmAudio.ctx;
    // With no sink open, only the first query needs a context of its own.
    if (!ctx && WasmAudio.maxChannels)
      return WasmAudio.maxChannels;
    const temporary = !ctx;
    try {
      if (!ctx)
        ctx = new AudioContext();
      WasmAudio.maxChannels = ctx.destination.maxChannelCount | 0;
      return WasmAudio.maxChannels;
    } catch (e) {
      return 0;
    } finally {
      if (temporary && ctx)
        ctx.close().catch(() => {});
    }
  },
});
