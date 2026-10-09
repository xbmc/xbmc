// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 Team Kodi
//
// Development-only pre-js, linked when ENABLE_WASM_DEV_PROXY=ON. Rewrites every
// cross-origin http(s) XHR/fetch issued by the module to '/proxy?u=<encoded>',
// the same-origin proxy tools/wasm/serve.py exposes, so sources without CORS
// headers are reachable from a page served over http(s). Runs in the main
// thread and in every pthread worker, hence the check on the environment rather
// than on Module settings, which workers do not see.

(function installHttpProxy(scope) {
  var servedOverHttp = scope.location && /^https?:$/.test(scope.location.protocol);
  var onTizen = /Tizen/i.test((scope.navigator && scope.navigator.userAgent) || '');
  if (!servedOverHttp || onTizen) {
    return;
  }
  var base = '/proxy';

  function rewrite(url) {
    if (typeof url !== 'string') {
      return url;
    }
    try {
      var u = new URL(url, scope.location.href);
      if (u.protocol !== 'http:' && u.protocol !== 'https:') {
        return url;
      }
      if (u.origin === scope.location.origin) {
        return url;
      }
      return base + '?u=' + encodeURIComponent(u.href);
    } catch (_) {
      return url;
    }
  }

  if (scope.XMLHttpRequest && scope.XMLHttpRequest.prototype &&
      !scope.XMLHttpRequest.prototype.__kodiProxyPatched) {
    var origOpen = scope.XMLHttpRequest.prototype.open;
    scope.XMLHttpRequest.prototype.open = function (method, url) {
      arguments[1] = rewrite(url);
      return origOpen.apply(this, arguments);
    };
    scope.XMLHttpRequest.prototype.__kodiProxyPatched = true;
  }

  if (typeof scope.fetch === 'function' && !scope.fetch.__kodiProxyPatched) {
    var origFetch = scope.fetch.bind(scope);
    var patched = function (input, init) {
      if (typeof input === 'string') {
        input = rewrite(input);
      } else if (input && typeof input.url === 'string') {
        var rewritten = rewrite(input.url);
        if (rewritten !== input.url) {
          input = new Request(rewritten, input);
        }
      }
      return origFetch(input, init);
    };
    patched.__kodiProxyPatched = true;
    scope.fetch = patched;
  }
})(globalThis);
