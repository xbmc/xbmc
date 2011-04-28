/*
 *      Copyright (C) 2005-2010 Team XBMC
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

var NowPlayingManager = function() {
		this.init();
		return true;
	}

NowPlayingManager.prototype = {
		updateCounter: 0,
		init: function() {
			$('#pbPause').hide(); /* Assume we are not playing something */
			this.bindPlaybackControls();
			this.updateState();
			$('#nextTrack').bind('click', jQuery.proxy(this.showPlaylist, this));
			$('#nowPlayingPlaylist').bind('click', function() {return false;});
			$(window).bind('click', jQuery.proxy(this.hidePlaylist, this));
		},
		updateState: function() {
			jQuery.ajax({
				type: 'POST', 
				url: JSON_RPC + '?UpdateState', 
				data: '{"jsonrpc": "2.0", "method": "Player.GetActivePlayers", "id": 1}', 
				timeout: 2000,
				success: jQuery.proxy(function(data) {
					if (data && data.result) {
						if (data.result.audio && this.activePlayer != 'Audio') {
							this.activePlayer = 'Audio';
							this.stopVideoPlaylistUpdate();
							this.displayAudioNowPlaying();
							this.stopRefreshTime();
						} else if (data.result.video && this.activePlayer != 'Video') {
							this.activePlayer = 'Video';
							this.stopAudioPlaylistUpdate();
							this.displayVideoNowPlaying();
							this.stopRefreshTime();
						} else if (!data.result.audio && !data.result.video) {
							this.stopRefreshTime();
						}
					}
					setTimeout(jQuery.proxy(this.updateState, this), 1000);
				}, this),
				error: jQuery.proxy(function(data, error) {
					displayCommunicationError();
					setTimeout(jQuery.proxy(this.updateState, this), 2000);
				}, this), 
				dataType: 'json'});
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
		nextTrack: function() {
			if (this.activePlayer) {
				jQuery.post(JSON_RPC + '?SkipNext', '{"jsonrpc": "2.0", "method": "' + this.activePlayer + 'Player.SkipNext", "id": 1}', jQuery.proxy(function(data) {
					if (data && data.result == 'OK') {
						//this.updateAudioPlaylist(true);
					}
				}, this), 'json');
			}
		},
		prevTrack: function() {
			if (this.activePlayer) {
				jQuery.post(JSON_RPC + '?SkipPrevious', '{"jsonrpc": "2.0", "method": "' + this.activePlayer + 'Player.SkipPrevious", "id": 1}', jQuery.proxy(function(data) {
					if (data && data.result == 'OK') {
						//this.updateAudioPlaylist(true);
					}
				}, this), 'json');
			}
		},
		stopTrack: function() {
			if (this.activePlayer) {
				jQuery.post(JSON_RPC + '?Stop', '{"jsonrpc": "2.0", "method": "' + this.activePlayer + 'Player.Stop", "id": 1}', jQuery.proxy(function(data) {
					if (data && data.result == 'OK') {
						this.playing = false;
						this.paused = false;
						this.trackBaseTime = 0;
						this.trackDurationTime = 0;
						this.showPlayButton();
					}
				}, this), 'json');
			}
		},
		playPauseTrack: function() {
			if (this.activePlayer) {
				var method = this.activePlayer + ((this.playing || this.paused) ? 'Player.PlayPause' : 'Playlist.Play');
				jQuery.post(JSON_RPC + '?PlayPause', '{"jsonrpc": "2.0", "method": "' + method + '", "id": 1}', jQuery.proxy(function(data) {
					if (data && data.result) {
						this.playing = data.result.playing;
						this.paused = data.result.paused;
						if (this.playing) {
							this.showPauseButton();
						} else {
							this.showPlayButton();
						}
					}
				}, this), 'json');
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
				jQuery.post(JSON_RPC + '?PlaylistItemPlay', '{"jsonrpc": "2.0", "method": "' + this.activePlayer + 'Playlist.Play", "params": { "item": ' + sequenceId + '}, "id": 1}', function() {}, 'json');
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
			jQuery.ajax({
				type: 'POST', 
				url: JSON_RPC + '?updateAudioPlaylist', 
				data: '{"jsonrpc": "2.0", "method": "AudioPlaylist.GetItems", "params": { "fields": ["title", "album", "artist", "duration"] }, "id": 1}', 
				success: jQuery.proxy(function(data) {
					if (data && data.result && data.result.items && data.result.limits.total > 0) {
						//Compare new playlist to active playlist, only redraw if a change is noticed
						if (!this.activePlaylistItem || this.playlistChanged(data.result.items) || (this.activePlaylistItem && (this.activePlaylistItem.seq != data.result.state.current))) {
							var ul = $('<ul>');
							var activeItem;
							$.each($(data.result.items), jQuery.proxy(function(i, item) {
								var li = $('<li>');
								var code = '<span class="duration">' + durationToString(item.duration) + '</span><div class="trackInfo" title="' + item.title + ' - ' + item.artist + '"><span class="trackTitle">' + item.title + '</span> - <span class="trackArtist">' + item.artist + '</span></div>';
								if (i == data.result.state.current) {
									activeItem = item;
									activeItem.seq = i;
									li.addClass('activeItem');
								}
								if (i == (data.result.state.current + 1)) {
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
									this.updateActiveItemDuration();
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
				}, this),
				error: jQuery.proxy(function(data) {
					displayCommunicationError();
					if (this.autoRefreshAudioPlaylist) {
						setTimeout(jQuery.proxy(this.updateAudioPlaylist, this), 2000); /* Slow down request period */
					}
				}, this),
				dataType: 'json'
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
			this.updateActiveItemDuration();
		},
		updateActiveItemDuration: function() {
			jQuery.post(JSON_RPC + '?updateDuration', '{"jsonrpc": "2.0", "method": "' + this.activePlayer + 'Player.GetTime", "id": 1}', jQuery.proxy(function(data) {
				if (data && data.result) {
					this.trackBaseTime = timeToDuration(data.result.time);
					this.trackDurationTime = timeToDuration(data.result.total);
					this.playing = data.result.playing;
					this.paused = data.result.paused;
					if (!this.autoRefreshAudioData && !this.autoRefreshVideoData) {
						if (data.result.playing) {				
							if (this.activePlayer == 'Audio') {
								this.autoRefreshAudioData = true;
								this.refreshAudioData();
							} else if (this.activePlayer == 'Video') {
								this.autoRefreshVideoData = true;
								this.refreshVideoData();
							}
						}
					}
				}
				if ((this.autoRefreshAudioData || this.autoRefreshVideoData) && !this.activeItemTimer) {
					this.activeItemTimer = 1;
					setTimeout(jQuery.proxy(this.updateActiveItemDurationLoop, this), 1000);
				}
			}, this), 'json');
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
					var imgPath = DEFAULT_ALBUM_COVER;
					if (this.activePlaylistItem.thumbnail) {
						imgPath = (this.activePlaylistItem.thumbnail.startsWith('special://') ? '/vfs/' : 'images/') + this.activePlaylistItem.thumbnail;
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
				$('#audioDuration').html(durationToString(this.trackBaseTime) + ' / ' + durationToString(this.trackDurationTime));
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
				setTimeout(jQuery.proxy(this.refreshVideoDataLoop, this), 1000);
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
					var imgPath = DEFAULT_VIDEO_COVER;
					if (this.activePlaylistItem.thumbnail) {
						imgPath = (this.activePlaylistItem.thumbnail.startsWith('special://') ? '/vfs/' : 'images/') + this.activePlaylistItem.thumbnail;
					}
					$('#videoCoverArt').html('<img src="' + imgPath + '" alt="' + this.activePlaylistItem.title + ' cover art">');
					var imgWidth = $('#videoCoverArt img').width();
					$('#progressBar').width(365 - (imgWidth - 100));
					$('#videoTrackWrap').width(365 - (imgWidth - 100));
					$('#videoTitle').width(365 - (imgWidth - 100));
					$('#videoShowTitle').html(this.activePlaylistItem.showtitle||'&nbsp;');
					var extra = '';
					if (this.activePlaylistItem.season >= 0 && this.activePlaylistItem.episode >= 0) {
						extra = this.activePlaylistItem.season + 'x' + this.activePlaylistItem.episode + ' ';
					}
					$('#videoTitle').html(extra + this.activePlaylistItem.title);
				}
				$('#videoDuration').html(durationToString(this.trackBaseTime) + ' / ' + durationToString(this.trackDurationTime));
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
			jQuery.ajax({
				type: 'POST', 
				url: JSON_RPC + '?updateVideoPlaylist', 
				data: '{"jsonrpc": "2.0", "method": "VideoPlaylist.GetItems", "params": { "fields": ["title", "season", "episode", "plot", "runtime", "showtitle"] }, "id": 1}', 
				success: jQuery.proxy(function(data) {
					if (data && data.result && data.result.items && data.result.limits.total > 0) {
						//Compare new playlist to active playlist, only redraw if a change is noticed.
						if (this.playlistChanged(data.result.items)) {
							var ul = $('<ul>');
							var activeItem;
							$.each($(data.result.items), jQuery.proxy(function(i, item) {
								var li = $('<li>');
								var extra = '';
								if (item.season >= 0 && item.episode >= 0) {
									extra = item.season + 'x' + item.episode + ' ';
								}
								var code = '<span class="duration">' + durationToString(item.runtime) + '</span><div class="trackInfo" title="' + extra + item.title + '"><span class="trackTitle">' + extra + item.title + '</span></div>';
								if (i == data.result.state.current) {
									activeItem = item;
									activeItem.seq = i;
									li.addClass('activeItem');
								}
								if (i == (data.result.state.current + 1)) {
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
									this.updateActiveItemDuration();
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
						this.activePlaylist = null;
						$('#videoDescription').hide();
						$('#nowPlayingPanel').hide();
					}					
					if (this.autoRefreshVideoPlaylist) {
						setTimeout(jQuery.proxy(this.updateVideoPlaylist, this), 1000);
					}
				}, this),
				error: jQuery.proxy(function(data) {
					displayCommunicationError();
					if (this.autoRefreshVideoPlaylist) {
						setTimeout(jQuery.proxy(this.updateVideoPlaylist, this), 2000); /* Slow down request period */
					}
				}, this),
				dataType: 'json'
			});
		}
	}
