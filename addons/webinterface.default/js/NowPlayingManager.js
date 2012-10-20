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

var NowPlayingManager = function() {
  this.init();
  return true;
}

NowPlayingManager.prototype = {
  updateCounter: 0,
  activePlayer: "",
  activePlayerId: -1,
  currentItem: -1,
  playing: false,
  paused: false,
  playlistid: -1,

  init: function() {
    $('#pbPause').hide(); /* Assume we are not playing something */
    this.bindPlaybackControls();
    this.updateState();
    $('#nextTrack').bind('click', jQuery.proxy(this.showPlaylist, this));
    $('#nowPlayingPlaylist').bind('click', function() {return false;});
    $(window).bind('click', jQuery.proxy(this.hidePlaylist, this));
  },
  updateState: function() {
    xbmc.rpc.request({
      'context': this,
      'method': 'Player.GetActivePlayers',
      'timeout': 3000,
      'success': function(data) {
        if (data && data.result && data.result.length > 0) {
          if (data.result[0].playerid != this.activePlayerId) {
            this.activePlayerId = data.result[0].playerid;
            this.activePlayer = data.result[0].type;
            if (this.activePlayer == "audio")
            {
              this.stopVideoPlaylistUpdate();
              this.displayAudioNowPlaying();
            }
            else if (this.activePlayer == "video")
            {
              this.stopAudioPlaylistUpdate();
              this.displayVideoNowPlaying();
            }
            else
            {
              this.stopVideoPlaylistUpdate();
              this.stopAudioPlaylistUpdate();
              this.activePlayer = "";
              this.activePlayerId = -1;
            }

            this.stopRefreshTime();
            this.updatePlayer();
          }
        }
        else if (!data || !data.result || data.result.length <= 0)
        {
          this.stopVideoPlaylistUpdate();
          this.stopAudioPlaylistUpdate();
          this.activePlayer = "";
          this.activePlayerId = -1;
        }

        if (this.activePlayerId >= 0) {
          this.showFooter();
        } else {
          this.stopRefreshTime();
          this.hideFooter();
        }

        setTimeout(jQuery.proxy(this.updateState, this), 1000);
      },
      'error': function(data, error) {
        xbmc.core.displayCommunicationError();
        setTimeout(jQuery.proxy(this.updateState, this), 2000);
      }
    });
  },
  updatePlayer: function() {
    xbmc.rpc.request({
      'context': this,
      'method': 'Player.GetProperties',
      'params': {
        'playerid': this.activePlayerId,
        'properties': [
          'playlistid',
          'speed',
          'position',
          'totaltime',
          'time'
        ]
      },
      'success': function(data) {
        if (data && data.result)
        {
          this.playlistid = data.result.playlistid;
          this.playing = data.result.speed != 0;
          this.paused = data.result.speed == 0;
          this.currentItem = data.result.position;
          this.trackBaseTime = xbmc.core.timeToDuration(data.result.time);
          this.trackDurationTime = xbmc.core.timeToDuration(data.result.totaltime);
          if (!this.autoRefreshAudioData && !this.autoRefreshVideoData && this.playing) {
            if (this.activePlayer == 'audio') {
              this.autoRefreshAudioData = true;
              this.refreshAudioData();
            } else if (this.activePlayer == 'video') {
              this.autoRefreshVideoData = true;
              this.refreshVideoData();
            }
          }
        }
        if ((this.autoRefreshAudioData || this.autoRefreshVideoData) && !this.activeItemTimer) {
          this.activeItemTimer = 1;
          setTimeout(jQuery.proxy(this.updateActiveItemDurationLoop, this), 1000);
        }
      }
    });
  },
  bindPlaybackControls: function() {
    $('#pbNext').bind('click', jQuery.proxy(this.nextTrack, this));
    $('#pbPrev').bind('click', jQuery.proxy(this.prevTrack, this));
    $('#pbStop').bind('click', jQuery.proxy(this.stopTrack, this));
    $('#pbPlay').bind('click', jQuery.proxy(this.playPauseTrack, this));
    $('#pbPause').bind('click', jQuery.proxy(this.playPauseTrack, this));
  },
  showPlaylist: function() {
    $('#nextText').html('Playlist: ');
    $('#nowPlayingPlaylist').show();
    return false;
  },
  hidePlaylist: function() {
    $('#nextText').html('Next: ');
    $('#nowPlayingPlaylist').hide();
    return false;
  },
  hideFooter: function() {
    $('#footerPopover').hide();
    $('#overlay').css('bottom','0px');
  },
  showFooter: function() {
    $('#footerPopover').show();
    $('#overlay').css('bottom','150px');
  },
  nextTrack: function() {
    if (this.activePlayer) {
      xbmc.rpc.request({
        'method': 'Player.GoTo',
        'params': {
          'playerid': this.activePlayerId,
          'to': 'next'
        },
        'success': function() {}
      });
    }
  },
  prevTrack: function() {
    if (this.activePlayer) {
      xbmc.rpc.request({
        'method': 'Player.GoTo',
        'params': {
          'playerid': this.activePlayerId,
          'to': 'previous'
        },
        'success': function() {}
      });
    }
  },
  stopTrack: function() {
    if (this.activePlayer) {
      xbmc.rpc.request({
        'context': this,
        'method': 'Player.Stop',
        'params': {
          'playerid': this.activePlayerId
        },
        'success': function(data) {
          if (data && data.result == 'OK') {
            this.playing = false;
            this.paused = false;
            this.trackBaseTime = 0;
            this.trackDurationTime = 0;
            this.showPlayButton();
          }
        }
      });
    }
  },
  playPauseTrack: function() {
    if (this.activePlayer) {
      var method = ((this.playing || this.paused) ? 'Player.PlayPause' : 'Playlist.Play');
      xbmc.rpc.request({
        'context': this,
        'method': method, 
        'params': {
          'playerid': this.activePlayerId
        },
        'success': function(data) {
          if (data && data.result) {
            this.playing = data.result.speed != 0;
            this.paused = data.result.speed == 0;
            if (this.playing) {
              this.showPauseButton();
            } else {
              this.showPlayButton();
            }
          }
        }
      });
    }
  },
  showPauseButton: function() {
    $('#pbPause').show();
    $('#pbPlay').hide();
  },
  showPlayButton: function() {
    $('#pbPause').hide();
    $('#pbPlay').show();
  },
  displayAudioNowPlaying: function() {
    if (!this.autoRefreshAudioPlaylist) {
      this.autoRefreshAudioPlaylist = true;
      this.updateAudioPlaylist();
    }
  },
  displayVideoNowPlaying: function() {
    if (!this.autoRefreshVideoPlaylist) {
      this.autoRefreshVideoPlaylist = true;
      this.updateVideoPlaylist();
    }
  },
  playPlaylistItem: function(sender) {
    var sequenceId = $(sender.currentTarget).attr('seq');
    if (!this.activePlaylistItem || (this.activePlaylistItem !== undefined && sequenceId != this.activePlaylistItem.seq)) {
      xbmc.rpc.request({
        'method': 'Player.GoTo',
        'params': {
          'playerid': this.activePlayerId,
          'to': sequenceId
        },
        'success': function() {}
      });
    }
    this.hidePlaylist();
  },
  playlistChanged: function(newPlaylist) {
    if (this.activePlaylist && !newPlaylist || !this.activePlaylist && newPlaylist) {
      return true;
    }
    if (!this.activePlaylist && !newPlaylist) {
      return false;
    }
    if (this.activePlaylist.length != newPlaylist.length) {
      return true;
    }
    for (var i = 0; i < newPlaylist.length; i++) {
      if (!this.comparePlaylistItems(this.activePlaylist[i], newPlaylist[i])) {
        return true;
      }
    }
    return false;
  },
  updateAudioPlaylist: function() {
    xbmc.rpc.request({
      'context': this,
      'method': 'Playlist.GetItems',
      'params': {
        'playlistid': this.playlistid,
        'properties':[
          'title',
          'album',
          'artist',
          'duration',
          'thumbnail'
        ]
      },
      'success': function(data) {
        if (data && data.result && data.result.items && data.result.items.length > 0 && data.result.limits.total > 0) {
          if (!this.activePlaylistItem || this.playlistChanged(data.result.items) || (this.activePlaylistItem && (this.activePlaylistItem.seq != this.currentItem))) {
            var ul = $('<ul>');
            var activeItem;
            $.each($(data.result.items), jQuery.proxy(function(i, item) {
              var li = $('<li>');
              var code = '<span class="duration">' + xbmc.core.durationToString(item.duration) + '</span><div class="trackInfo" title="' + item.title + ' - ' + item.artist + '"><span class="trackTitle">' + item.title + '</span> - <span class="trackArtist">' + item.artist + '</span></div>';
              if (i == this.currentItem) {
                activeItem = item;
                activeItem.seq = i;
                li.addClass('activeItem');
              }
              if (i == (this.currentItem + 1)) {
                $('#nextTrack').html(code).show();
              }
              li.bind('click', jQuery.proxy(this.playPlaylistItem, this));
              ul.append(li.attr('seq', i).html(code));
            }, this));
            if (data.result.limits.total > 1) {
              if (activeItem && data.result.limits.total-1 == activeItem.seq) {
                $('#nextTrack').html('<div class="trackInfo">Last track in playlist</div>').show();
              }
              $('#nextText').show();
              $('#nowPlayingPlaylist').html('').append(ul);
            } else {
              $('#nextText').hide();
              $('#nowPlayingPlaylist').hide();
              $('#nextTrack').hide();
            }
            if (!this.comparePlaylistItems(activeItem, this.activePlaylistItem)) {
              this.activePlaylistItem = activeItem;
              if (!this.updateActiveItemDurationRunOnce) {
                this.updateActiveItemDurationRunOnce = true;
                this.updatePlayer();
              }
            } else if (!activeItem) {
              this.stopRefreshTime();
            }
            this.activePlaylist = data.result.items;
            $('#videoDescription').hide();
            $('#audioDescription').show();
            $('#nowPlayingPanel').show();
          }
        } else {
          this.activePlaylist = null;
          $('#audioDescription').hide();
          $('#nowPlayingPanel').hide();
        }
        if (this.autoRefreshAudioPlaylist) {
          setTimeout(jQuery.proxy(this.updateAudioPlaylist, this), 1000);
        }
      },
      'error': function(data) {
        xbmc.core.displayCommunicationError();
        if (this.autoRefreshAudioPlaylist) {
          setTimeout(jQuery.proxy(this.updateAudioPlaylist, this), 2000); /* Slow down request period */
        }
      }
    });
  },
  stopAudioPlaylistUpdate: function() {
    this.autoRefreshAudioPlaylist = false;
    this.updateActiveItemDurationRunOnce = false;
  },
  stopVideoPlaylistUpdate: function() {
    this.autoRefreshVideoPlaylist = false;
    this.updateActiveItemDurationRunOnce = false;
  },
  updateActiveItemDurationLoop: function() {
    this.activeItemTimer = 0;
    this.updatePlayer();
  },
  refreshAudioDataLoop: function() {
    this.audioRefreshTimer = 0;
    this.refreshAudioData();
  },
  refreshAudioData: function() {
    if (this.autoRefreshAudioData && !this.audioRefreshTimer) {
      this.audioRefreshTimer = 1;
      setTimeout(jQuery.proxy(this.refreshAudioDataLoop, this), 1000);
    }
    if (this.playing && !this.paused) {
      this.trackBaseTime++;
    }
    if (this.paused) {
      this.showPlayButton();
    } else if (this.playing) {
      this.showPauseButton();
    }
    if (this.activePlaylistItem) {
      if (this.activePlaylistItem != this.lastPlaylistItem) {
        this.lastPlaylistItem = this.activePlaylistItem;
        var imgPath = xbmc.core.DEFAULT_ALBUM_COVER;
        if (this.activePlaylistItem.thumbnail) {
          imgPath = 'image/' + encodeURI(this.activePlaylistItem.thumbnail);
        }
        $('#audioCoverArt').html('<img src="' + imgPath + '" alt="' + this.activePlaylistItem.album + ' cover art">');
        $('#audioTrackTitle').html('<span title="' + this.activePlaylistItem.title + '">' + this.activePlaylistItem.title + '</span>');
        if (this.activePlaylistItem.album) {
          $('#audioAlbumTitle').html('<span title="' + this.activePlaylistItem.album + '">' + this.activePlaylistItem.album + '</span>')
                     .show();
        } else {
          $('#audioAlbumTitle').hide();
        }
        $('#audioArtistTitle').html(this.activePlaylistItem.artist);
        $('#progressBar').attr('style', '');
      }
      $('#audioDuration').html(xbmc.core.durationToString(this.trackBaseTime) + ' / ' + xbmc.core.durationToString(this.trackDurationTime));
      var buttonWidth = $('#progressBar .progressIndicator').width();
      var progressBarWidth = (this.trackBaseTime / this.trackDurationTime) * 100;
      var progressSliderPosition = Math.ceil(($('#progressBar').width() / 100) * progressBarWidth) - buttonWidth;
      if (progressSliderPosition < 0) {
        progressSliderPosition = 0;
      }
      if (progressBarWidth <= 100) {
        $('#progressBar .elapsedTime').width(progressBarWidth + '%');
        $('#progressBar .progressIndicator').css('left', progressSliderPosition);
      }
    }
  },
  refreshVideoDataLoop: function() {
    this.videoRefreshTimer = 0;
    this.refreshVideoData();
  },
  refreshVideoData: function() {
    if (this.autoRefreshVideoData && !this.videoRefreshTimer) {
      this.videoRefreshTimer = 1;
      setTimeout(jQuery.proxy(this.refreshVideoDataLoop, this), 1500);
    }
    if (this.playing && !this.paused) {
      this.trackBaseTime++;
    }
    if (this.paused) {
      this.showPlayButton();
    } else if (this.playing) {
      this.showPauseButton();
    }
    if (this.activePlaylistItem) {
      if (this.activePlaylistItem != this.lastPlaylistItem) {
        this.lastPlaylistItem = this.activePlaylistItem;
        var imgPath = xbmc.core.DEFAULT_VIDEO_COVER;
        if (this.activePlaylistItem.thumbnail) {
          imgPath = 'image/' + encodeURI(this.activePlaylistItem.thumbnail);
        }
        $('#videoCoverArt').html('<img src="' + imgPath + '" alt="' + this.activePlaylistItem.title + ' cover art">');
        $('#videoShowTitle').html(this.activePlaylistItem.showtitle||'&nbsp;');
        var extra = '';
        if (this.activePlaylistItem.season >= 0 && this.activePlaylistItem.episode >= 0) {
          extra = this.activePlaylistItem.season + 'x' + this.activePlaylistItem.episode + ' ';
        }
        $('#videoTitle').html(extra + this.activePlaylistItem.title);
      }
      $('#videoDuration').html(xbmc.core.durationToString(this.trackBaseTime) + ' / ' + xbmc.core.durationToString(this.trackDurationTime));
      var buttonWidth = $('#progressBar .progressIndicator').width();
      var progressBarWidth = (this.trackBaseTime / this.trackDurationTime) * 100;
      var progressSliderPosition = Math.ceil(($('#progressBar').width() / 100) * progressBarWidth) - buttonWidth;
      if (progressSliderPosition < 0) {
        progressSliderPosition = 0;
      }
      if (progressBarWidth <= 100) {
        $('#progressBar .elapsedTime').width(progressBarWidth + '%');
        $('#progressBar .progressIndicator').css('left', progressSliderPosition);
      }
    }
  },
  stopRefreshTime: function() {
    this.autoRefreshAudioData = false;
    this.autoRefreshVideoData = false;
  },
  comparePlaylistItems: function(item1, item2) {
    if (!item1 || !item2) {
      if (!item1 && !item2) {
        return true;
      }
      return false;
    }
    if (item1.title != item2.title) {
      return false;
    }
    if (item1.album != item2.album) {
      return false;
    }
    if (item1.artist != item2.artist) {
      return false;
    }
    if (item1.duration != item2.duration) {
      return false;
    }
    if (item1.label != item2.label) {
      return false;
    }
    if (item1.season != item2.season) {
      return false;
    }
    if (item1.episode != item2.episode) {
      return false;
    }
    return true;
  },
  updateVideoPlaylist: function() {
    xbmc.rpc.request({
      'context': this,
      'method': 'Playlist.GetItems',
      'params': {
        'playlistid': this.playlistid,
        'properties':[
          'title',
          'season',
          'episode',
          'plot',
          'runtime',
          'showtitle',
          'thumbnail'
        ]
      },
      'success': function(data) {
        if (data && data.result && data.result.items && data.result.items.length > 0 && data.result.limits.total > 0) {
          if (this.playlistChanged(data.result.items)) {
            var ul = $('<ul>');
            var activeItem;
            $.each($(data.result.items), jQuery.proxy(function(i, item) {
              var li = $('<li>');
              var extra = '';
              if (item.season >= 0 && item.episode >= 0) {
                extra = item.season + 'x' + item.episode + ' ';
              }
              var code = '<span class="duration">' + xbmc.core.durationToString(item.runtime) + '</span><div class="trackInfo" title="' + extra + item.title + '"><span class="trackTitle">' + extra + item.title + '</span></div>';
              if (i == this.currentItem) {
                activeItem = item;
                activeItem.seq = i;
                li.addClass('activeItem');
              }
              if (i == (this.currentItem + 1)) {
                $('#nextTrack').html(code).show();
              }
              li.bind('click', jQuery.proxy(this.playPlaylistItem, this));
              ul.append(li.attr('seq', i).html(code));
            }, this));
            if (data.result.limits.total > 1) {
              $('#nextText').show();
              if (activeItem && data.result.limits.total == activeItem.seq) {
                $('#nextTrack').html('<div class="trackInfo">Last track in playlist</div>').show();
              }
              $('#nowPlayingPlaylist').html('').append(ul);
            } else {
              $('#nextText').hide();
              $('#nowPlayingPlaylist').hide();
              $('#nextTrack').hide();
            }
            if (!this.comparePlaylistItems(activeItem, this.activePlaylistItem)) {
              this.activePlaylistItem = activeItem;
              if (!this.updateActiveItemDurationRunOnce) {
                this.updateActiveItemDurationRunOnce = true;
                this.updatePlayer();
              }
            } else if (!activeItem) {
              this.stopRefreshTime();
            }
            this.activePlaylist = data.result.items;
            $('#videoDescription').show();
            $('#audioDescription').hide();
            $('#nowPlayingPanel').show();
          }
        } else {
          xbmc.rpc.request({
            'context': this,
            'method': 'Player.GetItem',
            'params': {
              'playerid': this.activePlayerId,
              'properties': [
                'title',
                'season',
                'episode',
                'plot',
                'runtime',
                'showtitle',
                'thumbnail'
              ]
            },
            'success': function(data) {
              if (data && data.result && data.result.item) {
                this.activePlaylistItem = data.result.item;
                if (!this.updateActiveItemDurationRunOnce) {
                  this.updateActiveItemDurationRunOnce = true;
                  this.updatePlayer();
                }
                
                $('#nextText').hide();
                $('#nowPlayingPlaylist').hide();
                $('#nextTrack').hide();

                $('#videoDescription').show();
                $('#audioDescription').hide();
                $('#nowPlayingPanel').show();
              }
              else {
                this.activePlaylist = null;
                $('#videoDescription').hide();
                $('#nowPlayingPanel').hide();
              }
            },
            'error': function(data) {
              xbmc.core.displayCommunicationError();
              if (this.autoRefreshVideoPlaylist) {
                setTimeout(jQuery.proxy(this.updateVideoPlaylist, this), 2000); /* Slow down request period */
              }
            }
          });
        }
        if (this.autoRefreshVideoPlaylist) {
          setTimeout(jQuery.proxy(this.updateVideoPlaylist, this), 1000);
        }
      },
      'error': function(data) {
        xbmc.core.displayCommunicationError();
        if (this.autoRefreshVideoPlaylist) {
          setTimeout(jQuery.proxy(this.updateVideoPlaylist, this), 2000); /* Slow down request period */
        }
      }
    });
  }
}
