// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Team Kodi
//
// Main-thread setup for Kodi's WASM build: profile persistence and canvas
// focus. Rendering goes through a WebGL context Emscripten creates on
// <canvas id="canvas"> and proxies to the Kodi pthread.

// Persist Kodi's user profile ($HOME/.kodi) to IndexedDB via IDBFS so that
// sources.xml, guisettings.xml, the databases, etc. survive a page refresh.
// Mount and populate run inside Module.preRun, and addRunDependency blocks
// main() until IDBFS has finished loading from IndexedDB.
(function installIdbfsPersistence() {
  var Module = globalThis.Module = globalThis.Module || {};
  var PROFILE_PATH = '/home/web_user/.kodi';

  Module.preRun = Module.preRun || [];
  Module.preRun.push(function mountKodiProfile() {
    try {
      FS.mkdirTree(PROFILE_PATH);
    } catch (e) {
      console.warn('[kodi] mkdirTree ' + PROFILE_PATH + ':', e);
    }

    try {
      FS.mount(IDBFS, {}, PROFILE_PATH);
    } catch (e) {
      console.warn('[kodi] IDBFS mount failed, profile will not persist:', e);
      return;
    }

    // Debounced background flush to IndexedDB.
    var syncing = false;
    var pending = false;
    function flushToIdb() {
      if (syncing) { pending = true; return; }
      syncing = true;
      FS.syncfs(false, function (err) {
        syncing = false;
        if (err) { console.warn('[kodi] IDBFS persist:', err); }
        if (pending) { pending = false; flushToIdb(); }
      });
    }

    // Block main() until the profile has been loaded from IndexedDB. A flush
    // before then would delete whatever has not been loaded yet.
    addRunDependency('kodi-idbfs-populate');
    FS.syncfs(true, function (err) {
      removeRunDependency('kodi-idbfs-populate');
      if (err) {
        console.warn('[kodi] IDBFS populate failed, profile will not persist:', err);
        return;
      }

      setInterval(flushToIdb, 5000);

      if (typeof window !== 'undefined') {
        window.addEventListener('pagehide', flushToIdb, { capture: true });
        window.addEventListener('beforeunload', flushToIdb, { capture: true });
        // A TV can stop a hidden app without either of the above firing.
        document.addEventListener('visibilitychange', function () {
          if (document.visibilityState === 'hidden') { flushToIdb(); }
        });
      }
    });
  });
})();

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
