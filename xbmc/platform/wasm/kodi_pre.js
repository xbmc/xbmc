// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Team Kodi
//
// Main-thread setup for Kodi's WASM build: canvas focus. Rendering goes
// through a WebGL context Emscripten creates on <canvas id="canvas"> and
// proxies to the Kodi pthread.

(function () {
  if (typeof document === 'undefined') {
    return; // pthread worker
  }

  var Module = globalThis.Module = globalThis.Module || {};
  var kodi = (Module.kodi = Module.kodi || {});

  var canvas = document.getElementById('canvas');
  if (!canvas) {
    console.warn('[kodi] No <canvas id="canvas"> found; rendering disabled.');
    return;
  }
  // tabindex=0 allows the canvas to receive focus so keyboard input reaches Kodi.
  if (canvas.getAttribute('tabindex') === '-1' || canvas.getAttribute('tabindex') === null) {
    canvas.setAttribute('tabindex', '0');
  }

  // The WebGL context is created on this canvas by Emscripten (proxied from the
  // Kodi pthread); nothing else may take a rendering context on it.
  kodi.canvas = canvas;

  var prevOnRuntime = Module.onRuntimeInitialized;
  Module.onRuntimeInitialized = function () {
    try { canvas.focus(); } catch (_) {}
    if (typeof prevOnRuntime === 'function') {
      try { prevOnRuntime(); } catch (e) { console.error(e); }
    }
  };
})();
