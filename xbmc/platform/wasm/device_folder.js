/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// Browser side of filesystem/wasm/DeviceFolder.cpp: folders of this device that the user
// chose with the folder picker of the File System Access API. The handles live on the
// browser thread, whose API is asynchronous; the calling Kodi thread waits until
// kodi_device_folder_finish() is called. File contents are read on the calling thread,
// from a blob: URL of the file.

mergeInto(LibraryManager.library, {
  $DeviceFolder__deps : [ 'kodi_device_folder_finish', '$stringToNewUTF8', '$UTF8ToString' ],
  $DeviceFolder : {
    // Keep in sync with DeviceFolder.cpp.
    OP_AVAILABLE : 0,
    OP_LIST : 1,
    OP_STAT : 2,
    OP_OPEN : 3,
    OP_PICK : 4,
    OP_CLOSE : 5,

    DB : 'kodi-device-folders',
    STORE : 'folders',
    folders : null, // id -> FileSystemDirectoryHandle, once loaded from IndexedDB

    openDb : function() {
      return new Promise(function(resolve, reject) {
        var req = indexedDB.open(DeviceFolder.DB, 1);
        req.onupgradeneeded = function() { req.result.createObjectStore(DeviceFolder.STORE); };
        req.onsuccess = function() { resolve(req.result); };
        req.onerror = function() { reject(req.error); };
      });
    },

    load : async function() {
      if (DeviceFolder.folders)
        return;
      var folders = new Map();
      try
      {
        var db = await DeviceFolder.openDb();
        await new Promise(function(resolve, reject) {
          var tx = db.transaction(DeviceFolder.STORE, 'readonly');
          var cursor = tx.objectStore(DeviceFolder.STORE).openCursor();
          cursor.onsuccess = function() {
            var c = cursor.result;
            if (c)
            {
              folders.set(c.key, c.value);
              c.continue();
            }
          };
          tx.oncomplete = resolve;
          tx.onerror = function() { reject(tx.error); };
        });
        db.close();
      }
      catch (e)
      {
        console.warn('[device folder] stored folders not loaded:', e);
      }
      DeviceFolder.folders = folders;
    },

    store : async function(id, handle) {
      var db = await DeviceFolder.openDb();
      await new Promise(function(resolve, reject) {
        var tx = db.transaction(DeviceFolder.STORE, 'readwrite');
        tx.objectStore(DeviceFolder.STORE).put(handle, id);
        tx.oncomplete = resolve;
        tx.onerror = function() { reject(tx.error); };
      });
      db.close();
    },

    // The folder's name in a form that is safe as the host of a URL, made unique.
    newId : function(name) {
      var base = name.normalize('NFC').replace(/[^\p{L}\p{N}._-]+/gu, '-').replace(/^-+|-+$/g, '') ||
                 'folder';
      var id = base;
      for (var n = 2; DeviceFolder.folders.has(id); ++n)
        id = base + '-' + n;
      return id;
    },

    folder : async function(id) {
      await DeviceFolder.load();
      var handle = DeviceFolder.folders.get(id);
      if (!handle)
        throw new DOMException('no folder ' + id, 'NotFoundError');
      // The browser may ask again in a later session.
      if ((await handle.queryPermission({mode : 'read'})) !== 'granted' &&
          (await handle.requestPermission({mode : 'read'})) !== 'granted')
        throw new DOMException('permission denied', 'NotAllowedError');
      return handle;
    },

    // kind: 'd', 'f', or null to accept either
    walk : async function(path, kind) {
      var parts = path.split('/').filter(function(p) { return p.length > 0; });
      var handle = await DeviceFolder.folder(parts.shift());
      for (var i = 0; i < parts.length; ++i)
      {
        var last = i === parts.length - 1;
        if (!last || kind === 'd')
          handle = await handle.getDirectoryHandle(parts[i]);
        else if (kind === 'f')
          handle = await handle.getFileHandle(parts[i]);
        else
        {
          try
          {
            handle = await handle.getDirectoryHandle(parts[i]);
          }
          catch (e)
          {
            if (e.name !== 'TypeMismatchError')
              throw e;
            handle = await handle.getFileHandle(parts[i]);
          }
        }
      }
      return handle;
    },

    line : function(kind, size, mtime, segment, label) {
      return kind + '\t' + size + '\t' + mtime + '\t' + segment + '\t' + label + '\n';
    },

    folderLine : function(id, handle) { return DeviceFolder.line('d', 0, 0, id, handle.name); },

    list : async function(path) {
      if (path === '')
      {
        await DeviceFolder.load();
        var text = '';
        DeviceFolder.folders.forEach(function(handle, id) {
          text += DeviceFolder.folderLine(id, handle);
        });
        return {text : text};
      }
      var dir = await DeviceFolder.walk(path, 'd');
      var pending = [];
      for await (var entry of dir.values())
      {
        if (entry.kind === 'directory')
          pending.push(DeviceFolder.line('d', 0, 0, entry.name, entry.name));
        else
          pending.push(entry.getFile().then(
              function(file) {
                return DeviceFolder.line('f', file.size, Math.floor(file.lastModified / 1000),
                                         file.name, file.name);
              },
              function() { return ''; }));
      }
      return {text : (await Promise.all(pending)).join('')};
    },

    pick : async function() {
      var handle = await window.showDirectoryPicker({mode : 'read'});
      await DeviceFolder.load();
      for (var [knownId, known] of DeviceFolder.folders)
      {
        if (await known.isSameEntry(handle))
          return {text : DeviceFolder.folderLine(knownId, known)};
      }
      var id = DeviceFolder.newId(handle.name);
      DeviceFolder.folders.set(id, handle);
      await DeviceFolder.store(id, handle);
      return {text : DeviceFolder.folderLine(id, handle)};
    },

    stat : async function(path) {
      var handle = await DeviceFolder.walk(path, null);
      if (handle.kind === 'directory')
        return {isDir : true};
      var file = await handle.getFile();
      return {size : file.size, mtime : Math.floor(file.lastModified / 1000)};
    },

    open : async function(path) {
      var file = await (await DeviceFolder.walk(path, 'f')).getFile();
      return {text : URL.createObjectURL(file), size : file.size};
    },

    run : async function(op, path) {
      switch (op)
      {
        case DeviceFolder.OP_AVAILABLE:
          // Samsung's TV browser has the function, but it fails at once without a picker.
          if (typeof window.showDirectoryPicker !== 'function' || typeof tizen !== 'undefined')
            throw new DOMException('no folder picker', 'NotSupportedError');
          return {};
        case DeviceFolder.OP_LIST:
          return DeviceFolder.list(path);
        case DeviceFolder.OP_STAT:
          return DeviceFolder.stat(path);
        case DeviceFolder.OP_OPEN:
          return DeviceFolder.open(path);
        case DeviceFolder.OP_PICK:
          return DeviceFolder.pick();
        case DeviceFolder.OP_CLOSE:
          URL.revokeObjectURL(path);
          return {};
      }
      throw new DOMException('bad op ' + op, 'NotSupportedError');
    },

    errno : function(e) {
      switch (e && e.name)
      {
        case 'NotFoundError':
          return 2;
        case 'NotAllowedError':
        case 'SecurityError':
          return 13;
        case 'TypeMismatchError':
          return 20;
        case 'AbortError':
          return 125;
        default:
          return 5;
      }
    },
  },

  // Runs on the browser thread; finishes later, from a promise.
  kodi_device_folder_start__deps : [ '$DeviceFolder' ],
  kodi_device_folder_start__sig : 'vppip',
  kodi_device_folder_start : function(ctx, call, op, pathPtr) {
    DeviceFolder.run(op, UTF8ToString(pathPtr))
        .then(
            function(r) {
              _kodi_device_folder_finish(ctx, call, 0, r.text ? stringToNewUTF8(r.text) : 0,
                                         r.size || 0, r.mtime || 0, r.isDir ? 1 : 0);
            },
            function(e) { _kodi_device_folder_finish(ctx, call, DeviceFolder.errno(e), 0, 0, 0, 0); });
  },

  // Runs on the calling thread: one synchronous range request on the file's blob: URL.
  kodi_device_folder_read__sig : 'ipdpi',
  kodi_device_folder_read : function(urlPtr, offset, bufPtr, length) {
    try
    {
      var xhr = new XMLHttpRequest();
      xhr.open('GET', UTF8ToString(urlPtr), false);
      xhr.responseType = 'arraybuffer';
      xhr.setRequestHeader('Range', 'bytes=' + offset + '-' + (offset + length - 1));
      xhr.send(null);
      if (xhr.status !== 206)
        return -1;
      var data = new Uint8Array(xhr.response);
      if (data.length > length)
        data = data.subarray(0, length);
      HEAPU8.set(data, bufPtr);
      return data.length;
    }
    catch (e)
    {
      return -1;
    }
  },
});
