// SPDX-License-Identifier: GPL-2.0-or-later
// Run with: node --test tools/wasm/tests/test-audio-worklet.mjs
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
import {test} from 'node:test';

const source = readFileSync(new URL('../../../xbmc/platform/wasm/WasmAudioWorkletManager.cpp', import.meta.url), 'utf8');

// Execute the embedded production JavaScript against controlled Web Audio events.
function script(signature) {
  const position = source.indexOf(signature);
  assert.ok(position >= 0, signature);
  const tail = source.slice(position);
  const macro = tail.search(/(?:EM_ASM|MAIN_THREAD_(?:ASYNC_)?EM_ASM)\(/);
  assert.ok(macro >= 0, signature);
  const start = tail.indexOf('{', macro);
  let depth = 1, end = start + 1;
  while (depth) {
    if (tail[end] === '{') depth++;
    if (tail[end] === '}') depth--;
    end++;
  }
  return `(() => {${tail.slice(start + 1, end - 1)}})()`;
}

const settle = () => new Promise(resolve => setImmediate(resolve));

class Events extends EventTarget {
  listeners = new Map();
  addEventListener(type, fn, options) {
    super.addEventListener(type, fn, options);
    if (!this.listeners.has(type)) this.listeners.set(type, new Set());
    this.listeners.get(type).add(fn);
  }
  removeEventListener(type, fn, options) {
    super.removeEventListener(type, fn, options);
    this.listeners.get(type)?.delete(fn);
  }
  get count() { return [...this.listeners.values()].reduce((n, set) => n + set.size, 0); }
}

class Context extends Events {
  state = 'running';
  allowed = true;
  delayedSuspend = false;
  delayedResume = false;
  resumes = 0;
  pendingSuspends = [];
  pendingResumes = [];
  stateTo(state) { this.state = state; this.dispatchEvent(new Event('statechange')); }
  suspend() {
    return new Promise(resolve => {
      const finish = () => { this.stateTo('suspended'); resolve(); };
      if (this.delayedSuspend) this.pendingSuspends.push(finish);
      else finish();
    });
  }
  resume() {
    this.resumes++;
    if (!this.allowed) return Promise.reject(new Error('User gesture required'));
    return new Promise(resolve => {
      const finish = () => { this.stateTo('running'); resolve(); };
      if (this.delayedResume) this.pendingResumes.push(finish);
      else finish();
    });
  }
}

function environment(ctx = new Context()) {
  const window = new Events(), document = new Events();
  const env = vm.createContext({window, document, $0: 1, $1: 0, $2: 3,
    HEAPU8: new Uint8Array(new SharedArrayBuffer(1)), Atomics,
    emscriptenGetAudioObject: () => ctx,
    locateFile: name => `https://example.test/${name}`});
  const run = name => vm.runInContext(script(name), env);
  run('void TrackContextStateOnMain(');
  return {ctx, window, document, env, run};
}

for (const delayedSuspend of [false, true]) {
  for (const delayedResume of [false, true]) {
    test(`restart resumes without input: suspend delayed=${delayedSuspend}, resume delayed=${delayedResume}`, async () => {
      const e = environment();
      Object.assign(e.ctx, {delayedSuspend, delayedResume});
      e.run('void CWasmAudioWorkletManager::Suspend(');
      e.run('void CWasmAudioWorkletManager::InstallResumeHooks(');
      e.ctx.pendingSuspends.splice(0).forEach(f => f());
      e.ctx.pendingResumes.splice(0).forEach(f => f());
      await settle();
      assert.equal(e.ctx.state, 'running');
      assert.equal(e.window.count + e.document.count, 0);
      assert.equal(e.env.HEAPU8[0], 1);
    });
  }
}

test('autoplay rejection retains one set of gesture hooks, then removes them on resume', async () => {
  const e = environment();
  e.ctx.allowed = false;
  e.run('void CWasmAudioWorkletManager::Suspend(');
  e.run('void CWasmAudioWorkletManager::InstallResumeHooks(');
  e.ctx.dispatchEvent(new Event('statechange'));
  await settle();
  assert.equal(e.ctx.state, 'suspended');
  assert.equal(e.window.count + e.document.count, 4);
  e.ctx.allowed = true;
  e.ctx.delayedResume = true;
  e.window.dispatchEvent(new Event('keydown'));
  e.ctx.pendingResumes.splice(0).forEach(f => f());
  await settle();
  assert.equal(e.ctx.state, 'running');
  assert.equal(e.window.count + e.document.count, 0);
});

test('cleared sink hooks cannot resume after a late suspension or queued input', async () => {
  const e = environment();
  e.ctx.allowed = false;
  e.run('void CWasmAudioWorkletManager::Suspend(');
  e.run('void CWasmAudioWorkletManager::InstallResumeHooks(');
  const stale = e.ctx.__kodiResumeHandler;
  e.run('void ClearResumeHooksOnMain(');
  e.ctx.allowed = true;
  const before = e.ctx.resumes;
  e.ctx.stateTo('suspended');
  stale();
  await settle();
  assert.equal(e.ctx.resumes, before);
  assert.equal(e.window.count + e.document.count, 0);
});

test('repeated restart does not accumulate gesture listeners', async () => {
  const e = environment();
  for (let i = 0; i < 10; i++) {
    e.run('void ClearResumeHooksOnMain(');
    e.ctx.allowed = false;
    e.run('void CWasmAudioWorkletManager::Suspend(');
    e.run('void CWasmAudioWorkletManager::InstallResumeHooks(');
    await settle();
    assert.equal(e.window.count + e.document.count, 4);
    e.ctx.allowed = true;
    e.window.dispatchEvent(new Event('pointerdown'));
    assert.equal(e.ctx.state, 'running');
    assert.equal(e.window.count + e.document.count, 0);
  }
});

function publication() {
  let resolve, reject;
  const calls = [], urls = [];
  const ctx = {audioWorklet: {addModule(url) {
    urls.push(url);
    return new Promise((yes, no) => {resolve = yes; reject = no;});
  }}};
  const e = environment();
  e.env.emscriptenGetAudioObject = () => ctx;
  e.env.getWasmTableEntry = () => (...args) => calls.push(args);
  const publish = () => e.run('void PublishProcessorOnMain(');
  return {e, calls, urls, publish, resolve: () => resolve(), reject: () => reject(new Error('Publication failure'))};
}

test('publication uses the same-origin module, not a CSP-blocked data URL', async () => {
  const p = publication();
  p.publish();
  if (p.urls[0].startsWith('data:')) p.reject(); else p.resolve();
  await settle();
  assert.deepEqual(p.calls, [[1, 1, 3]]);
});

for (const outcome of ['success', 'reject', 'destroyed']) {
  test(`delayed publication: ${outcome}`, async () => {
    const p = publication();
    p.publish();
    await settle();
    assert.deepEqual(p.calls, []);
    if (outcome === 'destroyed') p.e.env.emscriptenGetAudioObject = () => undefined;
    if (outcome === 'reject') p.reject(); else p.resolve();
    await settle();
    assert.deepEqual(p.calls, outcome === 'destroyed' ? [] : [[1, outcome === 'success' ? 1 : 0, 3]]);
  });
}
