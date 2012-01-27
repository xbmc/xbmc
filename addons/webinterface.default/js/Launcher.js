/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/* This launcher is based on the scriptaculous.js launch script */

// Copyright (c) 2005-2008 Thomas Fuchs (http://script.aculo.us, http://mir.aculo.us)
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// For details, see the script.aculo.us web site: http://script.aculo.us/

var DEBUG_MODE = true; /* Set to false to enable cached javascript */

var Launcher = {
  VERSION: '1.0.0',
  REQUIRED_JQUERY: '1.5.2',
  load: function(libraryName) {
    document.write('<script type="text/javascript" src="' + libraryName + '?' + (DEBUG_MODE ? this.randomValue() : this.VERSION) + '"><\/script>');
  },
  init: function() {
    function convertVersionString(versionString) {
      var v = versionString.replace(/_.*|\./g, '');
      v = parseInt(v + (v.length == 4 ? '' : '0'));
      return versionString.indexOf('_') > -1 ? v-1 : v;
    }

    if(!jQuery || (convertVersionString(jQuery.fn.jquery) < convertVersionString(Launcher.REQUIRED_JQUERY)))
      throw("XBMC Web Interface requires the jQuery JavaScript framework >= " + Launcher.REQUIRED_JQUERY);

    var js = /Launcher\.js(\?.*)?$/;
    $('html').find('script[src]').each(
      function(i, s) {
        if (s.src.match(js)) {
          var path = s.src.replace(js, ''),
          includes = s.src.match(/\?.*load=([a-z,]*)/);
          $.each((includes ? includes[1] : 'jquery.lazyload,iscroll-min,Core,MediaLibrary,NowPlayingManager').split(','), function(i, include) {
            Launcher.load(path + include + '.js')
          });
        }
      });
  },
  randomValue: function() {
    return Math.random();
  }
}

Launcher.init();