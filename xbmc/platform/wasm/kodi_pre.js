// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Team Kodi
//
// Main-thread setup for Kodi's WASM build: profile persistence, canvas focus
// and clipboard paste. Rendering goes through a WebGL context Emscripten
// creates on <canvas id="canvas"> and proxies to the Kodi pthread.

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

    // Block main() until the profile has been loaded from IndexedDB.
    addRunDependency('kodi-idbfs-populate');
    FS.syncfs(true, function (err) {
      if (err) {
        console.warn('[kodi] IDBFS populate:', err);
      }
      removeRunDependency('kodi-idbfs-populate');
    });

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

    setInterval(flushToIdb, 5000);

    if (typeof window !== 'undefined') {
      window.addEventListener('pagehide', flushToIdb, { capture: true });
      window.addEventListener('beforeunload', flushToIdb, { capture: true });
    }
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
  // tabindex=0 allows the canvas to receive focus so paste and keyboard reach Kodi.
  if (canvas.getAttribute('tabindex') === '-1' || canvas.getAttribute('tabindex') === null) {
    canvas.setAttribute('tabindex', '0');
  }

  // The WebGL context is created on this canvas by Emscripten (proxied from the
  // Kodi pthread); nothing else may take a rendering context on it.
  kodi.canvas = canvas;

  var prevOnRuntime = Module.onRuntimeInitialized;
  Module.onRuntimeInitialized = function () {
    // Must run on the browser main thread (document is undefined on pthread workers).
    document.addEventListener(
        'paste',
        function (e) {
          // A paste into a DOM field (the native keyboard's input) is the browser's.
          var t = e.target;
          if (t && (t.tagName === 'INPUT' || t.tagName === 'TEXTAREA' || t.isContentEditable)) {
            return;
          }
          try {
            var text = (e.clipboardData && e.clipboardData.getData)
                ? e.clipboardData.getData('text/plain')
                : String();
            if (text === undefined || text === null) {
              text = String();
            }
            e.preventDefault();
            if (typeof Module.ccall === 'function') {
              Module.ccall('kodi_wasm_dispatch_paste', null, ['string'], [text]);
            }
          } catch (err) {
            console.error('[kodi] paste handler:', err);
          }
        },
        true);
    try { canvas.focus(); } catch (_) {}
    if (typeof prevOnRuntime === 'function') {
      try { prevOnRuntime(); } catch (e) { console.error(e); }
    }
  };
})();
