/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// Browser side of MseBridge.h: a MediaSource playing in a <video> element of
// WasmMediaLayer. Kodi's threads hand over fragmented MP4 for each track; the bridge
// queues it and appends it to the track's SourceBuffer one piece at a time, on the
// browser main thread where the MediaSource lives.

mergeInto(LibraryManager.library, {
  $Mse : {
    sources : new Map(),
    nextHandle : 1,
    // Data queued per track before appends are refused, in bytes.
    MAX_QUEUED : 32 * 1024 * 1024,

    // Field offsets of MseState, in bytes.
    SIGNAL : 0,
    OPEN : 4,
    CAN_PLAY : 8,
    ENDED : 12,
    ERROR : 16,
    VIDEO_SESSION : 20,
    AUDIO_SESSION : 24,
    POSITION : 32,

    store : function(source, offset,
                     value) { Atomics.store(HEAP32, (source.statePtr + offset) >> 2, value | 0); },

    signal : function(source) {
      var index = (source.statePtr + Mse.SIGNAL) >> 2;
      Atomics.add(HEAP32, index, 1);
      Atomics.notify(HEAP32, index);
    },

    setPosition : function(source, seconds) {
      HEAPF64[(source.statePtr + Mse.POSITION) >> 3] = seconds;
      Mse.signal(source);
    },

    fail : function(source, code, message) {
      if (message)
        console.error('[kodi][mse] ' + message);
      Mse.store(source, Mse.ERROR, code);
      Mse.signal(source);
    },

    listen : function(source, target, type, handler) {
      target.addEventListener(type, handler);
      source.listeners.push(function() { target.removeEventListener(type, handler); });
    },

    track : function(handle, video) {
      var source = Mse.sources.get(handle);
      return source ? source.tracks[video ? 'video' : 'audio'] : undefined;
    },

    publishSession : function(source, track) {
      Mse.store(source, track.video ? Mse.VIDEO_SESSION : Mse.AUDIO_SESSION, track.session);
    },

    // One update at a time. SourceBuffer.updating turns false before its updateend is
    // delivered, so the bridge keeps track of the update itself.
    pump : function(source, track) {
      if (track.appending || track.removing || track.buffer.updating ||
          source.media.readyState === 'closed')
        return;
      if (track.queue.length === 0)
      {
        Mse.maybeEnd(source);
        return;
      }
      try
      {
        track.buffer.appendBuffer(track.queue[0]);
        track.appending = {session : track.session};
      }
      catch (e)
      {
        if (e.name === 'QuotaExceededError')
        {
          track.full = true;
          Mse.evict(source, track);
        }
        else
          Mse.fail(source, 1, 'append: ' + e.message);
      }
    },

    // Removes what has played to make room, or tries again a little later.
    evict : function(source, track) {
      var buffered = track.buffer.buffered;
      var played = source.video.currentTime - 2;
      if (buffered.length > 0 && played > buffered.start(0))
      {
        try
        {
          track.buffer.remove(buffered.start(0), played);
          track.removing = true;
          return;
        }
        catch (e)
        {
        }
      }
      setTimeout(function() {
        if (Mse.sources.get(source.handle) === source)
          Mse.pump(source, track);
      }, 250);
    },

    updated : function(source, track) {
      if (track.appending)
      {
        // An append from before a seek, whose queue is gone, ends without taking anything.
        if (track.appending.session === track.session)
        {
          track.queued -= track.queue.shift().byteLength;
          track.full = false;
        }
        track.appending = null;
      }
      else
        track.removing = false;
      Mse.pump(source, track);
    },

    // Ends the source once every track has all its data in.
    maybeEnd : function(source) {
      if (source.media.readyState !== 'open')
        return;
      var tracks = [ source.tracks.video, source.tracks.audio ].filter(Boolean);
      for (var i = 0; i < tracks.length; i++)
      {
        if (!tracks[i].ended || tracks[i].queue.length > 0 || tracks[i].appending ||
            tracks[i].buffer.updating)
          return;
      }
      if (tracks.length === 0)
        return;
      try
      {
        source.media.endOfStream();
      }
      catch (e)
      {
      }
    },

    watchFrames : function(source) {
      var video = source.video;
      if (typeof video.requestVideoFrameCallback !== 'function')
        return;
      var frame = function(now, metadata) {
        Mse.setPosition(source, metadata.mediaTime);
        source.frameRequest = video.requestVideoFrameCallback(frame);
      };
      source.frameRequest = video.requestVideoFrameCallback(frame);
    },
  },

  mse_available__proxy : 'sync',
  mse_available : function() { return typeof MediaSource !== 'undefined' ? 1 : 0; },

  mse_is_type_supported__proxy : 'sync',
  mse_is_type_supported : function(typePtr) {
    return typeof MediaSource !== 'undefined' && MediaSource.isTypeSupported(UTF8ToString(typePtr))
               ? 1
               : 0;
  },

  mse_create__deps : [ '$Mse', '$WasmMediaLayer' ],
  mse_create__proxy : 'sync',
  mse_create : function(statePtr) {
    if (typeof MediaSource === 'undefined')
      return -1;
    var media = new MediaSource();
    var video = document.createElement('video');
    WasmMediaLayer.attach(video);

    var handle = Mse.nextHandle++;
    var source = {
      handle : handle,
      media : media,
      video : video,
      url : URL.createObjectURL(media),
      statePtr : statePtr,
      tracks : {},
      listeners : [],
      frameRequest : null,
    };
    Mse.sources.set(handle, source);

    Mse.listen(source, media, 'sourceopen', function() {
      Mse.store(source, Mse.OPEN, 1);
      Mse.signal(source);
    });
    Mse.listen(source, media, 'sourceclose', function() {
      Mse.store(source, Mse.OPEN, 0);
      Mse.signal(source);
    });
    ['timeupdate', 'seeked', 'pause'].forEach(function(type) {
      Mse.listen(source, video, type, function() { Mse.setPosition(source, video.currentTime); });
    });
    Mse.listen(source, video, 'canplay', function() {
      Mse.store(source, Mse.CAN_PLAY, 1);
      Mse.signal(source);
    });
    Mse.listen(source, video, 'ended', function() {
      Mse.store(source, Mse.ENDED, 1);
      Mse.signal(source);
    });
    Mse.listen(source, video, 'error', function() {
      var error = video.error;
      Mse.fail(source, 2, 'media error ' + (error ? error.code + ' ' + error.message : ''));
    });
    Mse.watchFrames(source);

    video.src = source.url;
    return handle;
  },

  mse_destroy__deps : [ '$Mse', '$WasmMediaLayer' ],
  mse_destroy__proxy : 'sync',
  mse_destroy : function(handle) {
    var source = Mse.sources.get(handle);
    if (!source)
      return;
    Mse.sources.delete(handle);
    source.listeners.forEach(function(remove) { remove(); });
    if (source.frameRequest !== null &&
        typeof source.video.cancelVideoFrameCallback === 'function')
      source.video.cancelVideoFrameCallback(source.frameRequest);
    source.video.pause();
    source.video.removeAttribute('src');
    source.video.load();
    WasmMediaLayer.detach(source.video);
    URL.revokeObjectURL(source.url);
  },

  mse_add_track__deps : [ '$Mse' ],
  mse_add_track__proxy : 'sync',
  mse_add_track : function(handle, video, typePtr, timestampOffset) {
    var source = Mse.sources.get(handle);
    if (!source || source.media.readyState !== 'open')
      return -1;
    var type = UTF8ToString(typePtr);
    try
    {
      var buffer = source.media.addSourceBuffer(type);
      buffer.mode = 'segments';
      buffer.timestampOffset = timestampOffset;
      var track = {
        video : !!video,
        buffer : buffer,
        queue : [],
        queued : 0,
        session : 1,
        full : false,
        ended : false,
        appending : null,
        removing : false,
      };
      source.tracks[video ? 'video' : 'audio'] = track;
      Mse.listen(source, buffer, 'updateend', function() { Mse.updated(source, track); });
      Mse.listen(source, buffer, 'error',
                 function() { Mse.fail(source, 1, 'cannot append ' + type); });
      Mse.publishSession(source, track);
      Mse.signal(source);
      return 0;
    }
    catch (e)
    {
      console.error('[kodi][mse] add track ' + type + ': ' + e.message);
      return -1;
    }
  },

  mse_set_duration__deps : [ '$Mse' ],
  mse_set_duration__proxy : 'sync',
  mse_set_duration : function(handle, seconds) {
    var source = Mse.sources.get(handle);
    if (!source)
      return -1;
    try
    {
      source.media.duration = isFinite(seconds) ? seconds : Infinity;
      return 0;
    }
    catch (e)
    {
      console.error('[kodi][mse] duration ' + seconds + ': ' + e.message);
      return -1;
    }
  },

  mse_append__deps : [ '$Mse' ],
  mse_append__proxy : 'sync',
  mse_append : function(handle, video, dataPtr, size, session) {
    var source = Mse.sources.get(handle);
    var track = Mse.track(handle, video);
    if (!track)
      return 3;
    if (session !== track.session)
      return 2;
    if (track.full || track.queued > Mse.MAX_QUEUED)
      return 1;
    // A copy: SourceBuffers do not take views of the shared wasm memory.
    track.queue.push(HEAPU8.slice(dataPtr, dataPtr + size));
    track.queued += size;
    Mse.pump(source, track);
    return 0;
  },

  mse_end_track__deps : [ '$Mse' ],
  mse_end_track__proxy : 'sync',
  mse_end_track : function(handle, video, session) {
    var track = Mse.track(handle, video);
    if (!track || session !== track.session)
      return;
    track.ended = true;
    Mse.maybeEnd(Mse.sources.get(handle));
  },

  mse_play__deps : [ '$Mse' ],
  mse_play__proxy : 'sync',
  mse_play : function(handle) {
    var source = Mse.sources.get(handle);
    if (!source)
      return;
    var playing = source.video.play();
    if (playing && playing.catch)
      playing.catch(function(e) { console.warn('[kodi][mse] play: ' + e.message); });
  },

  mse_pause__deps : [ '$Mse' ],
  mse_pause__proxy : 'sync',
  mse_pause : function(handle) {
    var source = Mse.sources.get(handle);
    if (source)
      source.video.pause();
  },

  mse_set_playback_rate__deps : [ '$Mse' ],
  mse_set_playback_rate__proxy : 'sync',
  mse_set_playback_rate : function(handle, rate) {
    var source = Mse.sources.get(handle);
    if (source)
      source.video.playbackRate = rate;
  },

  mse_set_volume__deps : [ '$Mse' ],
  mse_set_volume__proxy : 'sync',
  mse_set_volume : function(handle, volume) {
    var source = Mse.sources.get(handle);
    if (source)
      source.video.volume = Math.min(1, Math.max(0, volume));
  },

  mse_seek__deps : [ '$Mse' ],
  mse_seek__proxy : 'sync',
  mse_seek : function(handle, seconds) {
    var source = Mse.sources.get(handle);
    if (!source)
      return;
    [ source.tracks.video, source.tracks.audio ].filter(Boolean).forEach(function(track) {
      track.session++;
      track.queue = [];
      track.queued = 0;
      track.full = false;
      track.ended = false;
      // The next data starts with an initialisation segment; a removal has to finish. The
      // aborted append still ends with an updateend.
      if (track.appending && track.buffer.updating)
      {
        try
        {
          track.buffer.abort();
        }
        catch (e)
        {
        }
      }
      Mse.publishSession(source, track);
    });
    Mse.store(source, Mse.ENDED, 0);
    source.video.currentTime = seconds;
    Mse.setPosition(source, seconds);
  },
});
