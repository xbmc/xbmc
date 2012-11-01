/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

(function (document) {
    "use strict";

    var i,
        script,
        debug = false, /* Set to true to disable cached javascript */
        version = (debug ? Math.random() : '2.1.0'),
        scripts = [
            "js/jquery-1.8.2.min.js",
            "js/json2.js",
            "js/iscroll-min.js",
            "js/xbmc.core.js",
            "js/xbmc.rpc.js",
            "js/MediaLibrary.js",
            "js/NowPlayingManager.js",
            "js/xbmc.init.js"
        ];

    for (i = 0; i < scripts.length; i += 1) {
        script = '<script type="text/javascript" src="';
        script += scripts[i] + '?' + version;
        script += '"><\/script>';
        document.write(script);
    }
}(window.document));

