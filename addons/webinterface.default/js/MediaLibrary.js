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

var MediaLibrary = function() {
		this.init();
		return true;
	}

MediaLibrary.prototype = {
		init: function() {
			this.bindControls();
		},
		bindControls: function() {
			$('#musicLibrary').click(jQuery.proxy(this.musicLibraryOpen, this));
			$('#movieLibrary').click(jQuery.proxy(this.movieLibraryOpen, this));
			$('#tvshowLibrary').click(jQuery.proxy(this.tvshowLibraryOpen, this));
			$('#overlay').click(jQuery.proxy(this.hideOverlay, this));
			$(window).resize(jQuery.proxy(this.updatePlayButtonLocation, this));
		},
		resetPage: function() {
			$('#musicLibrary').removeClass('selected');
			$('#movieLibrary').removeClass('selected');
			$('#tvshowLibrary').removeClass('selected');
			this.hideOverlay();
		},
		musicLibraryOpen: function(event) {
			this.resetPage();
			$('#musicLibrary').addClass('selected');
			$('.contentContainer').css('z-index', 1);
			var libraryContainer = $('#libraryContainer');
			if (!libraryContainer || libraryContainer.length == 0) {
				$('#spinner').show();
				jQuery.post(JSON_RPC + '?GetAlbums', '{"jsonrpc": "2.0", "method": "AudioLibrary.GetAlbums", "params": { "start": 0, "fields": ["album_description", "album_theme", "album_mood", "album_style", "album_type", "album_label", "album_artist", "album_genre", "album_rating", "album_title"] }, "id": 1}', jQuery.proxy(function(data) {
					if (data && data.result && data.result.albums) {
							libraryContainer = $('<div>');
							libraryContainer.css('z-index', 100)
											.attr('id', 'libraryContainer')
											.addClass('contentContainer');
							$('#content').append(libraryContainer);
					} else {
						libraryContainer.html('');
					}
					$.each($(data.result.albums), jQuery.proxy(function(i, item) {
						var floatableAlbum = this.generateThumb('album', item.thumbnail, item.album_title, item.album_artist);
						floatableAlbum.bind('click', { album: item }, jQuery.proxy(this.displayAlbumDetails, this));
						libraryContainer.append(floatableAlbum);
					}, this));
					$('#spinner').hide();
					//$('#libraryContainer img').lazyload();
					libraryContainer.bind('scroll', { activeLibrary: libraryContainer }, jQuery.proxy(this.updateScrollEffects, this));
					libraryContainer.trigger('scroll');
				}, this), 'json');
			} else {
				libraryContainer.css('z-index', 100);
				libraryContainer.trigger('scroll');
			}
		},
		getThumbnailPath: function(thumbnail) {
			return thumbnail ? ('/vfs/' + thumbnail) : DEFAULT_ALBUM_COVER;
		},
		generateThumb: function(type, thumbnail, album_title, album_artist) {
			var floatableAlbum = $('<div>');
			var path = this.getThumbnailPath(thumbnail);
			var title = album_title;
			var artist = album_artist;
			if (title.length > 18 && !(title.length <= 21)) {
				title = album_title.substring(0, 18) + '...';
			}
			if (artist.length > 20 && !(artist.length <= 22)) {
				artist = album_artist.substring(0, 20) + '...';
			}
			var className = '';
			var code = '';
			switch(type) {
				case 'album':
					className = 'floatableAlbum';
					code = '<p class="album" title="' + album_title + '">' + title + '</p><p class="artist" title="' + album_artist + '">' + artist + '</p>';
					break;
				case 'movie':
					className = 'floatableMovieCover';
					code = '<p class="album" title="' + album_title + '">' + title + '</p>';
					break;
				case 'tvshow':
					className = 'floatableTVShowCover';
					break;
			}
			floatableAlbum.addClass(className).html('<div class="imgWrapper"><div class="inner"><img src="' + path + '" alt="" /></div></div>' + code);
			return floatableAlbum;
		},
		displayAlbumDetails: function(event) {
			var albumDetailsContainer = $('#albumDetails' + event.data.album.albumid);
			$('#topScrollFade').hide();
			if (!albumDetailsContainer || albumDetailsContainer.length == 0) {
				$('#spinner').show();
				jQuery.post(JSON_RPC + '?GetSongs', '{"jsonrpc": "2.0", "method": "AudioLibrary.GetSongs", "params": { "fields": ["title", "artist", "genre", "tracknumber", "discnumber", "duration", "year"], "albumid" : ' + event.data.album.albumid + ' }, "id": 1}', jQuery.proxy(function(data) {
					albumDetailsContainer = $('<div>');
					albumDetailsContainer.attr('id', 'albumDetails' + event.data.album.albumid)
										 .addClass('contentContainer')
										 .css('z-index', 100)
										 .html('<table class="albumView"><tr><th>Artwork</th><th>&nbsp;</th><th>Name</th><th>Time</th><th>Artist</th><th>Genre</th></tr><tbody class="resultSet"></tbody></table>');
					$('#content').append(albumDetailsContainer);
					var albumThumbnail = event.data.album.thumbnail;
					var albumTitle = event.data.album.album_title||'Unknown';
					var albumArtist = event.data.album.album_artist||'Unknown';
					var trackCount = data.result.total;
					$.each($(data.result.songs), jQuery.proxy(function(i, item) {
						var trackRow = $('<tr>');
						if (i == 0) {
							var albumTD = $('<td>');
							albumTD.attr('rowspan', ++trackCount).addClass('albumThumb');
							trackRow.append(albumTD);
						}
						var trackNumberTD = $('<td>');
						trackNumberTD.html(item.tracknumber).addClass('track').bind('click', { song: item, album: event.data.album }, jQuery.proxy(this.playTrack, this));
						trackRow.append(trackNumberTD);
						var trackTitleTD = $('<td>');
						trackTitleTD.html(item.title).addClass('track').bind('click', { song: item, album: event.data.album }, jQuery.proxy(this.playTrack, this));
						trackRow.append(trackTitleTD);
						var trackDurationTD = $('<td>');
						trackDurationTD.html(durationToString(item.duration)).addClass('track').bind('click', { song: item, album: event.data.album }, jQuery.proxy(this.playTrack, this));
						trackRow.append(trackDurationTD);
						var trackArtistTD = $('<td>');
						trackArtistTD.html(item.artist).addClass('track').bind('click', { song: item, album: event.data.album }, jQuery.proxy(this.playTrack, this));
						trackRow.append(trackArtistTD);
						var trackGenreTD = $('<td>');
						trackGenreTD.html(item.genre).addClass('track').bind('click', { song: item, album: event.data.album }, jQuery.proxy(this.playTrack, this));
						trackRow.append(trackGenreTD);
						$('#albumDetails' + event.data.album.albumid + ' .resultSet').append(trackRow);
					}, this));
					if (trackCount > 0) {
						var trackRow = $('<tr>');
						var trackNumberTD = $('<td>');
						trackNumberTD.addClass('fillerTrack').html('&nbsp');
						trackRow.append(trackNumberTD);
						var trackTitleTD = $('<td>');
						trackTitleTD.addClass('fillerTrack').html('&nbsp');
						trackRow.append(trackTitleTD);
						var trackDurationTD = $('<td>');
						trackDurationTD.addClass('fillerTrack').html('&nbsp');
						trackRow.append(trackDurationTD);
						var trackArtistTD = $('<td>');
						trackArtistTD.addClass('fillerTrack').html('&nbsp');
						trackRow.append(trackArtistTD);
						var trackGenreTD = $('<td>');
						trackGenreTD.addClass('fillerTrack').html('&nbsp');
						trackRow.append(trackGenreTD);
						$('#albumDetails' + event.data.album.albumid + ' .resultSet').append(trackRow);
					}
					$('#albumDetails' + event.data.album.albumid + ' .albumThumb').append(this.generateThumb('album', albumThumbnail, albumTitle, albumArtist));
					$('.contentContainer').css('z-index', 1);
					$('#spinner').hide();
				}, this), 'json');
			} else {
				$('.contentContainer').css('z-index', 1);
				$('#albumDetails' + event.data.album.albumid).css('z-index', 100);
			}
		},
		hideOverlay: function(event) {
			if (this.activeCover) {
				$(this.activeCover).remove();
				this.activeCover = null;
			}
			$('#overlay').hide();
		},
		updatePlayButtonLocation: function(event) {
			var movieContainer = $('.movieCover');
			if (movieContainer.length > 0) {
				var playIcon = $('.playIcon');
				if (playIcon.length > 0) {
					playIcon.width($(movieContainer[0]).width());
					playIcon.height($(movieContainer[0]).height());
				}
			}
		},
		playMovie: function(event) {
			jQuery.post(JSON_RPC + '?PlayMovie', '{"jsonrpc": "2.0", "method": "XBMC.Play", "params": { "movieid": ' + event.data.movie.movieid + ' }, "id": 1}', jQuery.proxy(function(data) {
				
				this.hideOverlay();
			}, this), 'json');
		},
		displayMovieDetails: function(event) {
			var movieDetails = $('<div>');
			movieDetails.attr('id', 'movie-' + event.data.movie.movieid);
			movieDetails.addClass('moviePopoverContainer');
			var closeButton = $('<img>');
			closeButton.attr('src', '/images/close-button.png');
			closeButton.addClass('closeButton').bind('click', jQuery.proxy(this.hideOverlay, this));
			movieDetails.append(closeButton);
			var movieCover = $('<img>');
			movieCover.attr('src', this.getThumbnailPath(event.data.movie.thumbnail)).addClass('movieCover');
			movieDetails.append(movieCover);
			var playIcon = $('<div>');
			playIcon.addClass('playIcon');
			playIcon.bind('click', {movie: event.data.movie}, jQuery.proxy(this.playMovie, this));
			movieDetails.append(playIcon);
			var movieTitle = $('<p>');
			movieTitle.addClass('movieTitle');
			var yearText = event.data.movie.year ? ' <span class="year">(' + event.data.movie.year + ')</span>' : '';
			movieTitle.html(event.data.movie.title + yearText);
			movieDetails.append(movieTitle);
			if (event.data.movie.runtime) {
				var runtime = $('<p>');
				runtime.addClass('runtime').html('<strong>Runtime:</strong> ' + event.data.movie.runtime + ' minutes');
				movieDetails.append(runtime);
			}
			if (event.data.movie.plot) {
				var plot = $('<p>');
				plot.addClass('plot').html(event.data.movie.plot);
				movieDetails.append(plot);
			}
			if (event.data.movie.genre) {
				var genre = $('<p>');
				genre.addClass('genre').html('<strong>Genre:</strong> ' + event.data.movie.genre);
				movieDetails.append(genre);
			}
			if (event.data.movie.rating) {
				//Todo
			}
			if (event.data.movie.director) {
				var director = $('<p>');
				director.addClass('director').html('<strong>Directed By:</strong> ' + event.data.movie.director);
				movieDetails.append(director);
			}
			this.activeCover = movieDetails;
			$('body').append(movieDetails);
			$('#overlay').show();
			this.updatePlayButtonLocation();
		},
		displayTVShowDetails: function(event) {

		},
		playTrack: function(event) {
			jQuery.post(JSON_RPC + '?ClearPlaylist', '{"jsonrpc": "2.0", "method": "AudioPlaylist.Clear", "id": 1}', jQuery.proxy(function(data) {
				//check that clear worked.
				jQuery.post(JSON_RPC + '?AddAlbumToPlaylist', '{"jsonrpc": "2.0", "method": "AudioPlaylist.Add", "params": { "albumid": ' + event.data.album.albumid + ' }, "id": 1}', jQuery.proxy(function(data) {
					//play specific song in playlist
					jQuery.post(JSON_RPC + '?PlaylistItemPlay', '{"jsonrpc": "2.0", "method": "AudioPlaylist.Play", "params": { "songid": ' + event.data.song.songid + ' }, "id": 1}', function() {}, 'json');
				}, this), 'json');
			}, this), 'json');
		},
		movieLibraryOpen: function() {
			this.resetPage();
			$('#movieLibrary').addClass('selected');
			$('.contentContainer').css('z-index', 1);
			var libraryContainer = $('#movieLibraryContainer');
			if (!libraryContainer || libraryContainer.length == 0) {
				$('#spinner').show();
				jQuery.post(JSON_RPC + '?GetMovies', '{"jsonrpc": "2.0", "method": "VideoLibrary.GetMovies", "params": { "start": 0, "fields": ["genre", "director", "trailer", "tagline", "plot", "plotoutline", "title", "originaltitle", "lastplayed", "showtitle", "firstaired", "duration", "season", "episode", "runtime", "year", "playcount", "rating"] }, "id": 1}', jQuery.proxy(function(data) {
					if (data && data.result && data.result.movies) {
							libraryContainer = $('<div>');
							libraryContainer.css('z-index', 100)
											.attr('id', 'movieLibraryContainer')
											.addClass('contentContainer');
							$('#content').append(libraryContainer);
					} else {
						libraryContainer.html('');
					}
					$.each($(data.result.movies), jQuery.proxy(function(i, item) {
						var floatableMovieCover = this.generateThumb('movie', item.thumbnail, item.title, "");
						floatableMovieCover.bind('click', { movie: item }, jQuery.proxy(this.displayMovieDetails, this));
						libraryContainer.append(floatableMovieCover);
					}, this));
					$('#spinner').hide();
					libraryContainer.bind('scroll', { activeLibrary: libraryContainer }, jQuery.proxy(this.updateScrollEffects, this));
					libraryContainer.trigger('scroll');
					//$('#libraryContainer img').lazyload();
				}, this), 'json');
			} else {
				libraryContainer.css('z-index', 100);
				libraryContainer.trigger('scroll');
			}
		},
		tvshowLibraryOpen: function() {
			this.resetPage();
			$('#tvshowLibrary').addClass('selected');
			$('.contentContainer').css('z-index', 1);
			var libraryContainer = $('#tvshowLibraryContainer');
			if (!libraryContainer || libraryContainer.length == 0) {
				$('#spinner').show();
				jQuery.post(JSON_RPC + '?GetTVShows', '{"jsonrpc": "2.0", "method": "VideoLibrary.GetTVShows", "params": { "start": 0, "fields": ["genre", "director", "trailer", "tagline", "plot", "plotoutline", "title", "originaltitle", "lastplayed", "showtitle", "firstaired", "duration", "season", "episode", "runtime", "year", "playcount", "rating"] }, "id": 1}', jQuery.proxy(function(data) {
					if (data && data.result && data.result.tvshows) {
							libraryContainer = $('<div>');
							libraryContainer.css('z-index', 100)
											.attr('id', 'tvshowLibraryContainer')
											.addClass('contentContainer');
							$('#content').append(libraryContainer);
					} else {
						libraryContainer.html('');
					}
					$.each($(data.result.tvshows), jQuery.proxy(function(i, item) {
						var floatableTVShowCover = this.generateThumb('tvshow', item.thumbnail, item.title, "");
						floatableTVShowCover.bind('click', { tvshow: item }, jQuery.proxy(this.displayTVShowDetails, this));
						libraryContainer.append(floatableTVShowCover);
					}, this));
					//$('#libraryContainer img').lazyload();
					$('#spinner').hide();
					libraryContainer.bind('scroll', { activeLibrary: libraryContainer }, jQuery.proxy(this.updateScrollEffects, this));
					libraryContainer.trigger('scroll');
				}, this), 'json');
			} else {
				libraryContainer.css('z-index', 100);
				libraryContainer.trigger('scroll');
			}
		},
		updateScrollEffects: function(event) {
			if (event.data.activeLibrary && $(event.data.activeLibrary).scrollTop() > 0) {
				$('#topScrollFade').fadeIn();
			} else {
				$('#topScrollFade').fadeOut();
			}
		}
	}