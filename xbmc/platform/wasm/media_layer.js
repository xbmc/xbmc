/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// Browser side of WasmMediaLayer.h: the <video> elements of the media backends, placed
// under the GUI canvas over the video area. The video shows wherever the canvas above it
// is transparent.

mergeInto(LibraryManager.library, {
  $WasmMediaLayer : {
    videos : new Set(),
    rect : null, // video area in canvas pixels, null for the whole canvas
    canvasStyle : null, // the canvas style to restore once the last video is gone

    attach : function(video) {
      video.style.cssText = 'position:fixed;z-index:0;object-fit:fill;background:transparent';
      var canvas = Module['canvas'];
      canvas.parentNode.insertBefore(video, canvas);
      if (WasmMediaLayer.videos.size === 0)
      {
        WasmMediaLayer.canvasStyle = canvas.style.cssText;
        canvas.style.position = 'relative';
        canvas.style.zIndex = '1';
        canvas.style.background = 'transparent';
      }
      WasmMediaLayer.videos.add(video);
      WasmMediaLayer.place(video);
    },

    detach : function(video) {
      WasmMediaLayer.videos.delete(video);
      video.remove();
      if (WasmMediaLayer.videos.size === 0 && WasmMediaLayer.canvasStyle !== null)
      {
        Module['canvas'].style.cssText = WasmMediaLayer.canvasStyle;
        WasmMediaLayer.canvasStyle = null;
      }
    },

    place : function(video) {
      var canvas = Module['canvas'];
      var box = canvas.getBoundingClientRect();
      var rect = WasmMediaLayer.rect;
      var sx = canvas.width > 0 ? box.width / canvas.width : 1;
      var sy = canvas.height > 0 ? box.height / canvas.height : 1;
      video.style.left = (box.left + (rect ? rect.x * sx : 0)) + 'px';
      video.style.top = (box.top + (rect ? rect.y * sy : 0)) + 'px';
      video.style.width = (rect ? rect.width * sx : box.width) + 'px';
      video.style.height = (rect ? rect.height * sy : box.height) + 'px';
    },
  },

  wasm_media_set_display_rect__deps : [ '$WasmMediaLayer' ],
  wasm_media_set_display_rect__proxy : 'sync',
  wasm_media_set_display_rect : function(x, y, width, height) {
    WasmMediaLayer.rect =
        width > 0 && height > 0 ? {x : x, y : y, width : width, height : height} : null;
    WasmMediaLayer.videos.forEach(function(video) { WasmMediaLayer.place(video); });
  },
});
