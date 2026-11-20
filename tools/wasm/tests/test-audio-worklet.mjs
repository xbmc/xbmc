// SPDX-License-Identifier: GPL-2.0-or-later
// Run with: node --test tools/wasm/tests/test-audio-worklet.mjs
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
import {test} from 'node:test';

const read = name =>
    readFileSync(new URL(`../../../xbmc/platform/wasm/${name}`, import.meta.url), 'utf8');
const source = read('audio_worklet.js');
const processorSource = read('audio_worklet_processor.js');

const settle = () => new Promise(resolve => setImmediate(resolve));

// Counts the registered listeners; a removal only counts when it matches one.
class Events extends EventTarget {
  listeners = new Map();
  registered(type, capture) {
    const key = `${type}/${capture === true}`;
    if (!this.listeners.has(key))
      this.listeners.set(key, new Set());
    return this.listeners.get(key);
  }
  addEventListener(type, fn, capture) {
    super.addEventListener(type, fn, capture);
    this.registered(type, capture).add(fn);
  }
  removeEventListener(type, fn, capture) {
    // Node's EventTarget ignores a boolean capture argument here.
    super.removeEventListener(type, fn, {capture: capture === true});
    this.registered(type, capture).delete(fn);
  }
  get count() {
    return [...this.listeners.values()].reduce((n, set) => n + set.size, 0);
  }
}

class Context {
  destination = {maxChannelCount: 2, channelCount: 2};
  modules = [];
  audioWorklet = {
    addModule: url => {
      this.modules.push(url);
      return Promise.resolve();
    },
  };
  onstatechange = null;
  resumes = 0;
  constructor(state, allowed, options) {
    this.state = state;
    this.allowed = allowed;
    this.options = options;
    this.sampleRate = options?.sampleRate ?? 48000;
  }
  stateTo(state) {
    this.state = state;
    this.onstatechange?.();
  }
  resume() {
    this.resumes++;
    return this.allowed ? Promise.resolve().then(() => this.stateTo('running'))
                        : Promise.reject(new Error('User gesture required'));
  }
  close() {
    this.state = 'closed';
    return Promise.resolve();
  }
}

// Loads the library the way Emscripten does, against controlled browser objects.
function environment(initialState = 'suspended', allowed = true) {
  const env = {window: new Events(), document: new Events(), contexts: [], nodes: []};
  const memory = new SharedArrayBuffer(4096);
  const library = {};
  const sandbox = vm.createContext({
    LibraryManager: {library},
    mergeInto: Object.assign,
    console: {error() {}, warn() {}},
    window: env.window,
    document: env.document,
    wasmMemory: {buffer: memory},
    HEAP32: new Int32Array(memory),
    HEAPU8: new Uint8Array(memory),
    HEAPU32: new Uint32Array(memory),
    locateFile: name => `https://example.test/${name}`,
    AudioContext: function(options) {
      if (env.failContext)
        throw new Error('NotSupportedError');
      const ctx = new Context(initialState, allowed, options);
      env.contexts.push(ctx);
      return ctx;
    },
    AudioWorkletNode: class {
      port = {onmessage: null, closed: false, close() { this.closed = true; }};
      onprocessorerror = null;
      constructor(ctx, name, options) {
        this.options = options;
        env.nodes.push(this);
      }
      connect() {}
      disconnect() {}
    },
  });
  vm.runInContext(source, sandbox);
  sandbox.WasmAudio = library.$WasmAudio;
  env.listeners = () => env.window.count + env.document.count;
  env.heap32 = sandbox.HEAP32;
  env.heapU8 = sandbox.HEAPU8;
  env.open = (sampleRate = 48000) => library.wasm_audio_open(sampleRate, RUNNING, 4, 8);
  env.close = () => library.wasm_audio_close();
  env.maxChannels = () => library.wasm_audio_max_output_channels();
  env.latency = () => library.wasm_audio_latency_us();
  env.start = (channels = 2) =>
      library.wasm_audio_start(channels, 256, 8, 0, 16, 17, 20, GENERATION, 28, 32, 36, STATE);
  return env;
}

const RUNNING = 0;
const GENERATION = 24;
const STATE = 40;

// The processor over its own shared memory. The writer follows CWasmAudioRing::Write(),
// which TestWasmAudioRing.cpp checks against a reader that follows the processor.
function processor({capacity = 8, prebuffer = 0, start = 0} = {}) {
  const memory = new SharedArrayBuffer(1024);
  const posted = [];
  let Processor;
  vm.runInNewContext(processorSource, {
    AudioWorkletProcessor: class {
      port = {postMessage: message => posted.push(message)};
    },
    registerProcessor: (name, cls) => { Processor = cls; },
  });
  const mem = {
    ready: new Uint8Array(memory, 0, 1),
    prebufferComplete: new Uint8Array(memory, 1, 1),
    active: new Int32Array(memory, 4, 1),
    generation: new Uint32Array(memory, 8, 1),
    read: new Uint32Array(memory, 12, 1),
    write: new Uint32Array(memory, 16, 1),
    underrun: new Uint32Array(memory, 20, 1),
    ring: [new Float32Array(memory, 64, capacity), new Float32Array(memory, 64 + capacity * 4, capacity)],
  };
  mem.ready[0] = 1;
  mem.generation[0] = 1;
  mem.read[0] = mem.write[0] = start;
  const p = new Processor({
    processorOptions: {
      buffer: memory, channels: 2, capacity, prebuffer, ready: 0, prebufferComplete: 1,
      activeCallbacks: 4, currentGeneration: 8, generation: 1, readFrame: 12, writeFrame: 16,
      underrunFrames: 20, ring: 64,
    },
  });
  let writeIndex = 0;
  let next = 1;
  const write = frames => {
    for (let i = 0; i < frames; ++i, ++next) {
      mem.ring[0][writeIndex] = next;
      mem.ring[1][writeIndex] = -next;
      writeIndex = (writeIndex + 1) % capacity;
    }
    mem.write[0] += frames;
  };
  const render = () => {
    const out = [new Float32Array(4).fill(99), new Float32Array(4).fill(99)];
    assert.equal(p.process([], [out]), true);
    assert.deepEqual([...out[1]], [...out[0]].map(v => -v || 0));
    return [...out[0]];
  };
  return {p, mem, posted, write, render};
}

test('the processor reads across the ring end', () => {
  const {mem, posted, write, render} = processor();
  assert.deepEqual(posted, [0]);
  write(6);
  assert.deepEqual(render(), [1, 2, 3, 4]);
  write(6);
  assert.deepEqual(render(), [5, 6, 7, 8]);
  assert.deepEqual(render(), [9, 10, 11, 12]);
  assert.equal(mem.read[0], 12);
  assert.equal(mem.underrun[0], 0);
  assert.equal(mem.active[0], 0);
});

test('the frame counters wrap at 2^32', () => {
  const {mem, write, render} = processor({start: 0xfffffffc});
  write(8);
  assert.equal(mem.write[0], 4);
  assert.deepEqual(render(), [1, 2, 3, 4]);
  assert.equal(mem.read[0], 0);
  assert.deepEqual(render(), [5, 6, 7, 8]);
  assert.equal(mem.read[0], 4);
  assert.equal(mem.underrun[0], 0);
});

test('an underrun plays what is there and counts the missing frames', () => {
  const {mem, write, render} = processor();
  write(3);
  assert.deepEqual(render(), [1, 2, 3, 0]);
  assert.equal(mem.underrun[0], 1);
  assert.deepEqual(render(), [0, 0, 0, 0]);
  assert.equal(mem.underrun[0], 5);
  assert.equal(mem.read[0], 3);
});

test('nothing plays before the prebuffer is reached', () => {
  const {mem, write, render} = processor({prebuffer: 4});
  write(2);
  assert.deepEqual(render(), [0, 0, 0, 0]);
  assert.equal(mem.prebufferComplete[0], 0);
  assert.equal(mem.underrun[0], 0);
  write(2);
  assert.deepEqual(render(), [1, 2, 3, 4]);
  assert.equal(mem.prebufferComplete[0], 1);
});

test('a drain plays what is queued below the prebuffer', () => {
  const {mem, write, render} = processor({prebuffer: 4});
  write(2);
  assert.deepEqual(render(), [0, 0, 0, 0]);
  mem.prebufferComplete[0] = 1;
  assert.deepEqual(render(), [1, 2, 0, 0]);
  assert.equal(mem.read[0], 2);
});

test('a processor of another generation leaves the ring alone', () => {
  const {mem, write, render} = processor();
  write(4);
  mem.generation[0] = 2;
  assert.deepEqual(render(), [0, 0, 0, 0]);
  assert.equal(mem.read[0], 0);
  mem.generation[0] = 1;
  mem.ready[0] = 0;
  assert.deepEqual(render(), [0, 0, 0, 0]);
  assert.equal(mem.read[0], 0);
});

test('a throwing render does not leave the processor marked active', () => {
  const {p, mem} = processor();
  p.render = () => { throw new Error('render'); };
  assert.throws(() => p.process([], [[new Float32Array(4)]]));
  assert.equal(mem.active[0], 0);
});

test('output channels without a ring plane are silent', () => {
  const {p, write} = processor();
  write(4);
  const out = [0, 1, 2].map(() => new Float32Array(4).fill(99));
  p.process([], [out]);
  assert.deepEqual(out.map(channel => [...channel]),
                   [[1, 2, 3, 4], [-1, -2, -3, -4], [0, 0, 0, 0]]);
});

test('the context is created at the requested sample rate', () => {
  const env = environment('running');
  assert.equal(env.open(44100), 1);
  assert.equal(env.contexts[0].options.sampleRate, 44100);
  assert.equal(env.contexts[0].options.latencyHint, 'playback');
  assert.equal(env.heap32[1], 44100);
  assert.equal(env.heap32[2], 128);
  assert.equal(env.heapU8[RUNNING], 1);
  env.close();

  env.open(0);
  assert.equal(env.contexts[1].options.sampleRate, undefined);
  assert.equal(env.heap32[1], 48000);
});

test('a context that cannot be created fails the open', () => {
  const env = environment();
  env.failContext = true;
  env.heap32[1] = env.heap32[2] = -1;
  env.heapU8[RUNNING] = 7;
  assert.equal(env.open(44100), 0);
  assert.equal(env.listeners(), 0);
  assert.deepEqual([env.heap32[1], env.heap32[2], env.heapU8[RUNNING]], [-1, -1, 7]);
  assert.equal(env.latency(), 0);
});

test('a suspended context arms one set of resume listeners', async () => {
  const env = environment('suspended', false);
  assert.equal(env.open(), 1);
  assert.equal(env.heapU8[RUNNING], 0);
  assert.equal(env.listeners(), 4);
  env.contexts[0].onstatechange();
  await settle();
  assert.equal(env.listeners(), 4);
  assert.equal(env.contexts[0].resumes, 1);
});

test('a suspended context that may start resumes without a gesture', async () => {
  const env = environment();
  env.open();
  await settle();
  assert.equal(env.contexts[0].resumes, 1);
  assert.equal(env.heapU8[RUNNING], 1);
  assert.equal(env.listeners(), 0);

  env.contexts[0].stateTo('suspended');
  assert.equal(env.heapU8[RUNNING], 0);
  await settle();
  assert.equal(env.contexts[0].resumes, 2);
  assert.equal(env.heapU8[RUNNING], 1);
  assert.equal(env.listeners(), 0);
});

test('a rejected resume keeps the listeners and a successful one removes them', async () => {
  const env = environment('suspended', false);
  env.open();
  const ctx = env.contexts[0];
  env.window.dispatchEvent(new Event('keydown'));
  await settle();
  assert.equal(ctx.resumes, 2);
  assert.equal(env.listeners(), 4);

  ctx.allowed = true;
  env.window.dispatchEvent(new Event('pointerdown'));
  await settle();
  assert.equal(ctx.resumes, 3);
  assert.equal(env.heapU8[RUNNING], 1);
  assert.equal(env.listeners(), 0);
});

test('closing removes the listeners and stops resuming', async () => {
  const env = environment('suspended', false);
  env.open();
  env.close();
  assert.equal(env.listeners(), 0);
  env.window.dispatchEvent(new Event('keydown'));
  await settle();
  assert.equal(env.contexts[0].resumes, 1);
  assert.equal(env.contexts[0].state, 'closed');
});

test('open and close cycles do not accumulate listeners', () => {
  const env = environment('suspended', false);
  for (let i = 0; i < 5; ++i) {
    env.open();
    assert.equal(env.listeners(), 4);
    env.close();
    assert.equal(env.listeners(), 0);
  }
});

test('the node is started once its processor answers and failed when it errors', async () => {
  const env = environment('running');
  env.open();
  new Uint32Array(env.heap32.buffer)[GENERATION >> 2] = 7;
  env.start();
  await settle();
  assert.deepEqual(env.contexts[0].modules, ['https://example.test/audio_worklet_processor.js']);
  const node = env.nodes[0];
  assert.equal(node.options.processorOptions.generation, 7);
  assert.equal(env.heap32[STATE >> 2], 0);
  node.port.onmessage({data: 0});
  assert.equal(env.heap32[STATE >> 2], 1);
  node.onprocessorerror(new Event('processorerror'));
  assert.equal(env.heap32[STATE >> 2], 2);
  node.port.onmessage({data: 0});
  assert.equal(env.heap32[STATE >> 2], 2);
});

test('the destination takes the channel count, up to what the output offers', async () => {
  const env = environment('running');
  env.open();
  env.contexts[0].destination.maxChannelCount = 6;
  env.start(6);
  await settle();
  assert.equal(env.contexts[0].destination.channelCount, 6);
  assert.equal(env.nodes[0].options.outputChannelCount[0], 6);
  assert.equal(env.nodes[0].options.processorOptions.channels, 6);
  env.close();

  env.open();
  env.start(6);
  await settle();
  assert.equal(env.contexts[1].destination.channelCount, 2);
  assert.equal(env.nodes[1].options.outputChannelCount[0], 6);
});

test('a start without a context fails at once', () => {
  const env = environment('running');
  env.start();
  assert.equal(env.heap32[STATE >> 2], 2);
});

test('a processor that cannot be constructed fails the start', async () => {
  const env = environment('running');
  env.open();
  env.start();
  await settle();
  env.nodes[0].onprocessorerror(new Event('processorerror'));
  assert.equal(env.heap32[STATE >> 2], 2);
});

test('a module that cannot be loaded fails the start', async () => {
  const env = environment('running');
  env.open();
  env.contexts[0].audioWorklet.addModule = () => Promise.reject(new Error('blocked'));
  env.start();
  await settle();
  assert.equal(env.nodes.length, 0);
  assert.equal(env.heap32[STATE >> 2], 2);
});

test('events of a closed context are ignored', async () => {
  const env = environment('running');
  env.open();
  env.start();
  await settle();
  env.close();
  assert.equal(env.nodes[0].port.closed, true);
  env.nodes[0].port.onmessage({data: 0});
  env.nodes[0].onprocessorerror(new Event('processorerror'));
  assert.equal(env.heap32[STATE >> 2], 0);
});

test('the channel count comes from the open context', () => {
  const env = environment();
  env.open();
  env.contexts[0].destination.maxChannelCount = 6;
  assert.equal(env.maxChannels(), 6);
  assert.equal(env.contexts.length, 1);
  assert.equal(env.contexts[0].state, 'suspended');
});

test('without an open context the channel count needs one temporary context', () => {
  const env = environment();
  assert.equal(env.maxChannels(), 2);
  assert.equal(env.maxChannels(), 2);
  assert.equal(env.contexts.length, 1);
  assert.equal(env.contexts[0].state, 'closed');
  assert.equal(env.listeners(), 0);

  env.open();
  env.contexts[1].destination.maxChannelCount = 8;
  assert.equal(env.maxChannels(), 8);
  env.close();
  assert.equal(env.maxChannels(), 8);
  assert.equal(env.contexts.length, 2);
});

test('the latency adds what the context reports, in microseconds', () => {
  const env = environment();
  assert.equal(env.latency(), 0);
  env.open();
  const ctx = env.contexts[0];
  assert.equal(env.latency(), 0);
  ctx.baseLatency = 0.01;
  assert.equal(env.latency(), 10000);
  ctx.outputLatency = 0.02;
  assert.equal(env.latency(), 30000);
  ctx.outputLatency = NaN;
  assert.equal(env.latency(), 10000);
  ctx.outputLatency = Infinity;
  assert.equal(env.latency(), 10000);
  ctx.outputLatency = 1e6;
  assert.equal(env.latency(), 0x7fffffff);
  ctx.baseLatency = -1;
  ctx.outputLatency = 0;
  assert.equal(env.latency(), 0);
});
