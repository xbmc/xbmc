/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// Browser side of TizenEmss.h: Samsung's Tizen WASM Player
// (tizentvwasm.ElementaryMediaStreamSource) playing in a <video> element of
// WasmMediaLayer. The TV decodes, keeps audio and video in sync and shows the video on its
// own plane, wherever the page above it is transparent.
//
// The source, its tracks and the media element belong to the browser main thread, so
// every entry point that touches them is proxied there. Appends go to the TV straight
// from the calling pthread through tizentvwasm.SideThreadElementaryMediaTrack.

mergeInto(LibraryManager.library, {
  $TizenEmss : {
    sources : new Map(),
    nextHandle : 1,

    // Field offsets of TizenEmssState, in bytes.
    SIGNAL : 0,
    READY_STATE : 4,
    VIDEO_OPEN : 8,
    VIDEO_SESSION : 12,
    AUDIO_OPEN : 16,
    AUDIO_SESSION : 20,
    CAN_PLAY : 24,
    ENDED : 28,
    ERROR : 32,
    DECODING_MODE : 36,
    POSITION : 40,

    READY_STATES : {'detached' : 0, 'closed' : 1, 'open-pending' : 2, 'open' : 3, 'ended' : 4},
    ERRORS : {'config-error' : 1, 'drm-error' : 2, 'pipeline-error' : 3, 'out-of-memory' : 4},
    DECODING_MODES : {'DecodingModeHardware' : 1, 'DecodingModeSoftware' : 2},

    api : function() {
      return typeof tizentvwasm !== 'undefined' && !!tizentvwasm.ElementaryMediaStreamSource;
    },

    store : function(source, offset,
                     value) { Atomics.store(HEAP32, (source.statePtr + offset) >> 2, value | 0); },

    signal : function(source) {
      var index = (source.statePtr + TizenEmss.SIGNAL) >> 2;
      Atomics.add(HEAP32, index, 1);
      Atomics.notify(HEAP32, index);
    },

    publishTrack : function(source, track, kind, event) {
      var open = kind === 'video' ? TizenEmss.VIDEO_OPEN : TizenEmss.AUDIO_OPEN;
      var session = kind === 'video' ? TizenEmss.VIDEO_SESSION : TizenEmss.AUDIO_SESSION;
      TizenEmss.store(source, open, track.isOpen ? 1 : 0);
      TizenEmss.store(source, session,
                      event && event.sessionId !== undefined ? event.sessionId : track.sessionId);
      // activeDecodingMode is known once the track opens.
      if (kind === 'video')
      {
        var mode = 0;
        try
        {
          mode = TizenEmss.DECODING_MODES[track.activeDecodingMode] || 0;
        }
        catch (e)
        {
        }
        TizenEmss.store(source, TizenEmss.DECODING_MODE, mode);
      }
      TizenEmss.signal(source);
    },

    publishReadyState : function(source) {
      var state = TizenEmss.READY_STATES[source.emss.readyState];
      TizenEmss.store(source, TizenEmss.READY_STATE, state === undefined ? 0 : state);
      TizenEmss.signal(source);
    },

    fail : function(source, code, message) {
      if (message)
        console.error('[kodi][emss] ' + message);
      TizenEmss.store(source, TizenEmss.ERROR, code);
      TizenEmss.signal(source);
    },

    listen : function(source, target, type, handler) {
      target.addEventListener(type, handler);
      source.listeners.push(function() { target.removeEventListener(type, handler); });
    },

    trackConfig : function(mimePtr, extraPtr, extraSize) {
      return {
        mimeType : UTF8ToString(mimePtr),
        extradata : extraSize > 0 ? HEAPU8.slice(extraPtr, extraPtr + extraSize)
                                  : new Uint8Array(0),
      };
    },

    addTrack : function(handle, kind, config) {
      var source = TizenEmss.sources.get(handle);
      if (!source)
        return -1;
      try
      {
        var track = kind === 'video' ? source.emss.addVideoTrack(config)
                                     : source.emss.addAudioTrack(config);
        var publish = function(event) { TizenEmss.publishTrack(source, track, kind, event); };
        TizenEmss.listen(source, track, 'trackopen', publish);
        TizenEmss.listen(source, track, 'trackclosed', publish);
        TizenEmss.listen(source, track, 'sessionidchanged', publish);
        source.tracks.push(track);
        publish();
        return track.trackId;
      }
      catch (e)
      {
        console.error('[kodi][emss] add ' + kind + ' track ' + config.mimeType + ': ' + e.message);
        return -1;
      }
    },

    validate : function(check) {
      try
      {
        return check() === 'ConfigSupported' ? 1 : 0;
      }
      catch (e)
      {
        return -1;
      }
    },

    appendResult : function(e) {
      var message = (e && e.message) || '';
      // The byte limit reports a full buffer, the packet count limit an overflow.
      if (message.indexOf('buffer full') >= 0 || message.indexOf('buffer overflow') >= 0)
        return 1;
      // A packet from a session the track has moved past; the wording differs by firmware.
      if (message.indexOf('Invalid session') >= 0 || message.indexOf('wrong session_id') >= 0)
        return 2;
      if (message.indexOf('keyframe') >= 0)
        return 3;
      console.error('[kodi][emss] append: ' + message);
      return 4;
    },
  },

  tizen_emss_available__deps : [ '$TizenEmss' ],
  tizen_emss_available__proxy : 'sync',
  tizen_emss_available : function() {
    return TizenEmss.api() && typeof tizentvwasm.SideThreadElementaryMediaTrack !== 'undefined' ? 1
                                                                                                : 0;
  },

  tizen_emss_validate_video__deps : [ '$TizenEmss' ],
  tizen_emss_validate_video__proxy : 'sync',
  tizen_emss_validate_video : function(mimePtr, width, height, framerateNum, framerateDen) {
    if (!TizenEmss.api() || !tizentvwasm.ElementaryMediaTrack ||
        typeof tizentvwasm.ElementaryMediaTrack.ValidateVideoConfig !== 'function')
      return -1;
    var config = {
      mimeType : UTF8ToString(mimePtr),
      width : width,
      height : height,
      framerateNum : framerateNum,
      framerateDen : framerateDen,
    };
    return TizenEmss.validate(function() {
      return tizentvwasm.ElementaryMediaTrack.ValidateVideoConfig(config, 'latency-mode-normal',
                                                                  'rendering-mode-media-element');
    });
  },

  tizen_emss_validate_audio__deps : [ '$TizenEmss' ],
  tizen_emss_validate_audio__proxy : 'sync',
  tizen_emss_validate_audio : function(mimePtr, formatPtr, layoutPtr, samplesPerSecond) {
    if (!TizenEmss.api() || !tizentvwasm.ElementaryMediaTrack ||
        typeof tizentvwasm.ElementaryMediaTrack.ValidateAudioConfig !== 'function')
      return -1;
    var config = {
      mimeType : UTF8ToString(mimePtr),
      sampleFormat : UTF8ToString(formatPtr),
      channelLayout : UTF8ToString(layoutPtr),
      samplesPerSecond : samplesPerSecond,
    };
    return TizenEmss.validate(
        function() { return tizentvwasm.ElementaryMediaTrack.ValidateAudioConfig(config); });
  },

  tizen_emss_create__deps : [ '$TizenEmss', '$WasmMediaLayer' ],
  tizen_emss_create__proxy : 'sync',
  tizen_emss_create : function(statePtr) {
    if (!TizenEmss.api())
      return -1;
    // Tizen 9 has no tizentvwasm.getAvailableApis() to tell which constructor it takes.
    var emss;
    try
    {
      emss = new tizentvwasm.ElementaryMediaStreamSource('latency-mode-normal',
                                                         'rendering-mode-media-element');
    }
    catch (e)
    {
      try
      {
        emss = new tizentvwasm.ElementaryMediaStreamSource('normal');
      }
      catch (e)
      {
        console.error('[kodi][emss] create: ' + e.message);
        return -1;
      }
    }

    // The TV shows the video on a plane under the page, through the element's box.
    var video = document.createElement('video');
    WasmMediaLayer.attach(video);

    var handle = TizenEmss.nextHandle++;
    var source = {
      emss : emss,
      video : video,
      url : URL.createObjectURL(emss),
      statePtr : statePtr,
      tracks : [],
      listeners : [],
    };
    TizenEmss.sources.set(handle, source);

    var readyState = function() { TizenEmss.publishReadyState(source); };
    ['sourceopen', 'sourceopenpending', 'sourceclosed', 'sourceended', 'sourcedetached'].forEach(
        function(type) { TizenEmss.listen(source, emss, type, readyState); });
    TizenEmss.listen(source, emss, 'playbackpositionchanged', function(event) {
      HEAPF64[(source.statePtr + TizenEmss.POSITION) >> 3] = event.playbackPosition;
      TizenEmss.signal(source);
    });
    TizenEmss.listen(source, emss, 'error', function(event) {
      TizenEmss.fail(source, TizenEmss.ERRORS[event.error] || 6,
                     'source error: ' + (event.message || event.error));
    });
    TizenEmss.listen(source, video, 'canplay', function() {
      TizenEmss.store(source, TizenEmss.CAN_PLAY, 1);
      TizenEmss.signal(source);
    });
    TizenEmss.listen(source, video, 'ended', function() {
      TizenEmss.store(source, TizenEmss.ENDED, 1);
      TizenEmss.signal(source);
    });
    TizenEmss.listen(source, video, 'error', function() {
      var error = video.error;
      TizenEmss.fail(source, 5, 'media error ' + (error ? error.code + ' ' + error.message : ''));
    });

    video.src = source.url;
    readyState();
    return handle;
  },

  tizen_emss_destroy__deps : [ '$TizenEmss', '$WasmMediaLayer' ],
  tizen_emss_destroy__proxy : 'sync',
  tizen_emss_destroy : function(handle) {
    var source = TizenEmss.sources.get(handle);
    if (!source)
      return;
    TizenEmss.sources.delete(handle);
    source.listeners.forEach(function(remove) { remove(); });
    try
    {
      source.video.pause();
    }
    catch (e)
    {
    }
    source.video.removeAttribute('src');
    source.video.load();
    WasmMediaLayer.detach(source.video);
    URL.revokeObjectURL(source.url);
    if (typeof source.emss.destroy === 'function')
      source.emss.destroy();
  },

  tizen_emss_add_video_track__deps : [ '$TizenEmss' ],
  tizen_emss_add_video_track__proxy : 'sync',
  tizen_emss_add_video_track : function(handle, mimePtr, extraPtr, extraSize, width, height,
                                        framerateNum, framerateDen) {
    var config = TizenEmss.trackConfig(mimePtr, extraPtr, extraSize);
    config.width = width;
    config.height = height;
    config.framerateNum = framerateNum;
    config.framerateDen = framerateDen;
    return TizenEmss.addTrack(handle, 'video', config);
  },

  tizen_emss_add_audio_track__deps : [ '$TizenEmss' ],
  tizen_emss_add_audio_track__proxy : 'sync',
  tizen_emss_add_audio_track : function(handle, mimePtr, extraPtr, extraSize, formatPtr, layoutPtr,
                                        samplesPerSecond) {
    var config = TizenEmss.trackConfig(mimePtr, extraPtr, extraSize);
    config.sampleFormat = UTF8ToString(formatPtr);
    config.channelLayout = UTF8ToString(layoutPtr);
    config.samplesPerSecond = samplesPerSecond;
    return TizenEmss.addTrack(handle, 'audio', config);
  },

  tizen_emss_set_duration__deps : [ '$TizenEmss' ],
  tizen_emss_set_duration__proxy : 'sync',
  tizen_emss_set_duration : function(handle, seconds) {
    var source = TizenEmss.sources.get(handle);
    if (!source)
      return -1;
    try
    {
      source.emss.duration = isFinite(seconds) ? seconds : Infinity;
      return 0;
    }
    catch (e)
    {
      console.error('[kodi][emss] duration ' + seconds + ': ' + e.message);
      return -1;
    }
  },

  tizen_emss_open__deps : [ '$TizenEmss' ],
  tizen_emss_open__proxy : 'sync',
  tizen_emss_open : function(handle) {
    var source = TizenEmss.sources.get(handle);
    if (!source)
      return -1;
    try
    {
      // The source can be destroyed, and its state reused, before open() settles.
      var alive = function() { return TizenEmss.sources.get(handle) === source; };
      Promise.resolve(source.emss.open())
          .then(function() {
            if (alive())
              TizenEmss.publishReadyState(source);
          })
          .catch(function(e) {
            if (alive())
              TizenEmss.fail(source, 3, 'open: ' + e.message);
          });
      return 0;
    }
    catch (e)
    {
      TizenEmss.fail(source, 3, 'open: ' + e.message);
      return -1;
    }
  },

  tizen_emss_play__deps : [ '$TizenEmss' ],
  tizen_emss_play__proxy : 'sync',
  tizen_emss_play : function(handle) {
    var source = TizenEmss.sources.get(handle);
    if (!source)
      return;
    var playing = source.video.play();
    if (playing && playing.catch)
      playing.catch(function(e) { console.warn('[kodi][emss] play: ' + e.message); });
  },

  tizen_emss_pause__deps : [ '$TizenEmss' ],
  tizen_emss_pause__proxy : 'sync',
  tizen_emss_pause : function(handle) {
    var source = TizenEmss.sources.get(handle);
    if (source)
      source.video.pause();
  },

  tizen_emss_set_playback_rate__deps : [ '$TizenEmss' ],
  tizen_emss_set_playback_rate__proxy : 'sync',
  tizen_emss_set_playback_rate : function(handle, rate) {
    var source = TizenEmss.sources.get(handle);
    if (!source)
      return;
    try
    {
      source.video.playbackRate = rate;
    }
    catch (e)
    {
      console.warn('[kodi][emss] playback rate ' + rate + ': ' + e.message);
    }
  },

  tizen_emss_seek__deps : [ '$TizenEmss' ],
  tizen_emss_seek__proxy : 'sync',
  tizen_emss_seek : function(handle, seconds) {
    var source = TizenEmss.sources.get(handle);
    if (!source)
      return;
    TizenEmss.store(source, TizenEmss.ENDED, 0);
    source.video.currentTime = seconds;
  },

  tizen_emss_append__deps : [ '$TizenEmss' ],
  tizen_emss_append : function(trackId, dataPtr, size, pts, dts, duration, keyFrame, sessionId,
                               width, height) {
    var packet = {
      pts : pts,
      dts : dts,
      duration : duration,
      isKeyFrame : !!keyFrame,
      isEncrypted : false,
      sessionId : sessionId,
    };
    if (width > 0 && height > 0)
    {
      packet.width = width;
      packet.height = height;
    }
    try
    {
      tizentvwasm.SideThreadElementaryMediaTrack.appendPacketSync(
          trackId, packet, new Uint8Array(HEAPU8.buffer, dataPtr, size));
      return 0;
    }
    catch (e)
    {
      return TizenEmss.appendResult(e);
    }
  },

  tizen_emss_append_end__deps : [ '$TizenEmss' ],
  tizen_emss_append_end : function(trackId, sessionId) {
    try
    {
      tizentvwasm.SideThreadElementaryMediaTrack.appendEndOfTrackSync(trackId, sessionId);
      return 0;
    }
    catch (e)
    {
      return TizenEmss.appendResult(e);
    }
  },
});
