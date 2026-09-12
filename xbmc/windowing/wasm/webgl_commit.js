/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

// Presents Emscripten's offscreen framebuffer without the synchronous GL state
// queries emscripten_webgl_commit_frame() makes. Scissor test is re-enabled
// unconditionally: CRenderSystemGLES keeps it enabled between frames.
mergeInto(LibraryManager.library, {
  wasm_webgl_commit_frame__deps: ['$GL'],
  wasm_webgl_commit_frame__proxy: 'sync',
  wasm_webgl_commit_frame__sig: 'i',
  wasm_webgl_commit_frame: function() {
    const context = GL.currentContext;
    if (!context || !context.GLctx || !context.defaultFbo) return 0;
    const gl = context.GLctx;
    gl.disable(gl.SCISSOR_TEST);
    gl.bindFramebuffer(gl.READ_FRAMEBUFFER, context.defaultFbo);
    gl.bindFramebuffer(gl.DRAW_FRAMEBUFFER, null);
    gl.blitFramebuffer(0, 0, gl.canvas.width, gl.canvas.height,
                       0, 0, gl.canvas.width, gl.canvas.height,
                       gl.COLOR_BUFFER_BIT, gl.NEAREST);
    gl.bindFramebuffer(gl.FRAMEBUFFER, context.defaultFbo);
    gl.enable(gl.SCISSOR_TEST);
    return 1;
  },
});
