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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

var xbmc = xbmc || {};
xbmc.core = {};

/* Global Paths */
xbmc.core.JSON_RPC = 'jsonrpc';
xbmc.core.DEFAULT_ALBUM_COVER = 'images/DefaultAlbumCover.png';
xbmc.core.DEFAULT_VIDEO_COVER = 'images/DefaultVideo.png';

/* Prototypes */

String.prototype.startsWith = function(prefix) {
  return this.indexOf(prefix) === 0;
}

String.prototype.endsWith = function(suffix) {
  return this.match(suffix + "$") == suffix;
}

xbmc.core.durationToString = function (duration) {
  if (!duration) {
    return '00:00';
  }
  minutes = Math.floor(duration / 60);
  hours = Math.floor(minutes / 60);
  minutes = minutes % 60;
  seconds = duration % 60;
  var result = '';
  if (hours) {
    result += (hours < 10 ? '0' + hours : hours) + ':';
  }
  result += (minutes < 10 ? '0' + minutes : minutes) + ':' + (seconds < 10 ? '0' + seconds : seconds);
  return result;
}

xbmc.core.timeToDuration = function (time) {
  return time.hours * 3600 + time.minutes * 60 + time.seconds;
}

xbmc.core.applyDeviceFixes = function () {
  document.addEventListener('touchmove', function(e){ e.preventDefault(); });
}

xbmc.core.displayCommunicationError = function (m) {
  clearTimeout(xbmc.core.commsErrorTimeout);
  var message = m || 'Connection to server lost';
  $('#commsErrorPanel').html(message).show();
  xbmc.core.commsErrorTimeout = setTimeout('xbmc.core.hideCommunicationError()', 5000);
}

xbmc.core.hideCommunicationError = function () {
  $('#commsErrorPanel').hide();
}

xbmc.core.setCookie = function (name,value,days) {
  if (days) {
    var date = new Date();
    date.setTime(date.getTime()+(days*24*60*60*1000));
    var expires = "; expires="+date.toGMTString();
  }
  else var expires = "";
  document.cookie = name+"="+value+expires+"; path=/";
}

xbmc.core.getCookie = function (name) {
  var nameEQ = name + "=";
  var ca = document.cookie.split(';');
  for(var i=0;i < ca.length;i++) {
    var c = ca[i];
    while (c.charAt(0)==' ') c = c.substring(1,c.length);
    if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
  }
  return null;
}

