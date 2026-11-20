/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

// The AudioWorklet module that audio_worklet.js loads from next to kodi.js.
// Views are created once over fixed byte ranges: the SharedArrayBuffer keeps
// the length it had when it was posted, and the ring and counters were
// allocated before that and do not move while the node exists. The counters
// are 32-bit and wrap.
class KodiAudioProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    const o = options.processorOptions;
    const b = o.buffer;
    this.ready = new Uint8Array(b, o.ready, 1);
    this.prebufferComplete = new Uint8Array(b, o.prebufferComplete, 1);
    this.active = new Int32Array(b, o.activeCallbacks, 1);
    this.currentGeneration = new Uint32Array(b, o.currentGeneration, 1);
    this.generation = o.generation;
    this.readFrame = new Uint32Array(b, o.readFrame, 1);
    this.writeFrame = new Uint32Array(b, o.writeFrame, 1);
    this.underrunFrames = new Uint32Array(b, o.underrunFrames, 1);
    this.readIndex = 0;
    this.capacity = o.capacity;
    this.prebuffer = o.prebuffer;
    this.planes = [];
    for (let ch = 0; ch < o.channels; ++ch)
      this.planes.push(new Float32Array(b, o.ring + ch * o.capacity * 4, o.capacity));
    this.port.postMessage(0);
  }

  process(inputs, outputs) {
    const out = outputs[0];
    let rendered = false;
    Atomics.add(this.active, 0, 1);
    try {
      rendered = this.render(out);
    } finally {
      Atomics.sub(this.active, 0, 1);
    }
    if (!rendered)
      for (let ch = 0; ch < out.length; ++ch)
        out[ch].fill(0);
    return true;
  }

  render(out) {
    // A processor of a context that is still closing must not touch the ring
    // of the node that replaced it.
    if (out.length === 0 || !Atomics.load(this.ready, 0) ||
        Atomics.load(this.currentGeneration, 0) !== this.generation)
      return false;

    const frames = out[0].length;
    const read = Atomics.load(this.readFrame, 0);
    const available = (Atomics.load(this.writeFrame, 0) - read) >>> 0;
    if (!Atomics.load(this.prebufferComplete, 0)) {
      if (available < this.prebuffer)
        return false;
      Atomics.store(this.prebufferComplete, 0, 1);
    }

    const toRead = Math.min(available, frames);
    if (toRead < frames)
      Atomics.add(this.underrunFrames, 0, frames - toRead);
    if (toRead === 0)
      return false;

    const start = this.readIndex;
    const first = Math.min(toRead, this.capacity - start);
    for (let ch = 0; ch < out.length; ++ch) {
      const dst = out[ch];
      if (ch >= this.planes.length) {
        dst.fill(0);
        continue;
      }
      const src = this.planes[ch];
      for (let i = 0; i < first; ++i)
        dst[i] = src[start + i];
      for (let i = first; i < toRead; ++i)
        dst[i] = src[i - first];
      dst.fill(0, toRead);
    }

    this.readIndex = (start + toRead) % this.capacity;
    Atomics.store(this.readFrame, 0, read + toRead);
    return true;
  }
}
registerProcessor('kodi-audio', KodiAudioProcessor);
