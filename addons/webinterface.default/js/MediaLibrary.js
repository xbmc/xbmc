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
			$('#pictureLibrary').click(jQuery.proxy(this.pictureLibraryOpen, this));
			$('#overlay').click(jQuery.proxy(this.hideOverlay, this));
			$(window).resize(jQuery.proxy(this.updatePlayButtonLocation, this));
		},
		resetPage: function() {
			$('#musicLibrary').removeClass('selected');
			$('#movieLibrary').removeClass('selected');
			$('#tvshowLibrary').removeClass('selected');
			$('#pictureLibrary').removeClass('selected');
			this.hideOverlay();
		},
		musicLibraryOpen: function(event) {
			this.resetPage();
			$('#musicLibrary').addClass('selected');
			$('.contentContainer').hide();
			var libraryContainer = $('#libraryContainer');
			if (!libraryContainer || libraryContainer.length == 0) {
				$('#spinner').show();
				libraryContainer = $('<div>');
				libraryContainer.attr('id', 'libraryContainer')
								.addClass('contentContainer');
				$('#content').append(libraryContainer);
				jQuery.post(JSON_RPC + '?GetAlbums', '{"jsonrpc": "2.0", "method": "AudioLibrary.GetAlbums", "params": { "start": 0, "fields": ["album_description", "album_theme", "album_mood", "album_style", "album_type", "album_label", "album_artist", "album_genre", "album_rating", "album_title"] }, "id": 1}', jQuery.proxy(function(data) {
					if (data && data.result && data.result.albums) {
						this.albumList = data.result.albums;
						this.albumList.sort(jQuery.proxy(this.albumArtistSorter, this));
						$.each($(this.albumList), jQuery.proxy(function(i, item) {
							var floatableAlbum = this.generateThumb('album', item.thumbnail, item.album_title, item.album_artist);
							floatableAlbum.bind('click', { album: item }, jQuery.proxy(this.displayAlbumDetails, this));
							libraryContainer.append(floatableAlbum);
						}, this));
						libraryContainer.append($('<div>').addClass('footerPadding'));
						$('#spinner').hide();
						//$('#libraryContainer img').lazyload();
						libraryContainer.bind('scroll', { activeLibrary: libraryContainer }, jQuery.proxy(this.updateScrollEffects, this));
						libraryContainer.trigger('scroll');
						myScroll = new iScroll('libraryContainer');
					} else {
						libraryContainer.html('');
					}
				}, this), 'json');
			} else {
				libraryContainer.show();
				libraryContainer.trigger('scroll');
			}
		},
		getThumbnailPath: function(thumbnail) {
			return thumbnail ? ('/vfs/' + thumbnail) : DEFAULT_ALBUM_COVER;
		},
		generateThumb: function(type, thumbnail, album_title, album_artist) {
			var floatableAlbum = $('<div>');
			var path = this.getThumbnailPath(thumbnail);
			var title = album_title||'';
			var artist = album_artist||'';
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
				case 'image':
				case 'directory':
					className = 'floatableAlbum';
					code = '<p class="album" title="' + album_title + '">' + title + '</p>';
					break;
			}
			return floatableAlbum.addClass(className).html('<div class="imgWrapper"><div class="inner"><img src="' + path + '" alt="" /></div></div>' + code);
		},
		showAlbumSelectorBlock: function(album) {
			if (album) {
				//Find album in stored array
				var prevAlbum = null,
					nextAlbum = null;
				$.each($(this.albumList), jQuery.proxy(function(i, item) {
					if (item.albumid == album.albumid) {
						if (this.albumList.length > 1) {
							prevAlbum = this.albumList[i <= 0 ? this.albumList.length-1 : i-1];
							nextAlbum = this.albumList[i >= this.albumList.length ? 0 : i+1];
						}
						return false; /* .each break */
					}
				}, this));
				var albumSelectorBlock = $('#albumSelector');
				if (!albumSelectorBlock || albumSelectorBlock.length == 0) {
					albumSelectorBlock = $('<div>');
					albumSelectorBlock.attr('id', 'albumSelector')
									  .html('<table><tr><td class="allAlbums">All Albums</td><td class="activeAlbumTitle"></td><td class="prevAlbum">&nbsp;</td><td class="nextAlbum">&nbsp;</td></tr></table>');
					$('#content').prepend(albumSelectorBlock);
					$('#albumSelector .allAlbums').bind('click', jQuery.proxy(this.hideAlbumDetails, this));
				}
				$('#albumSelector .prevAlbum').unbind();
				$('#albumSelector .nextAlbum').unbind();
				if (prevAlbum) {
					$('#albumSelector .prevAlbum').bind('click', {album: prevAlbum}, jQuery.proxy(this.displayAlbumDetails, this));
				}
				if (nextAlbum) {
					$('#albumSelector .nextAlbum').bind('click', {album: nextAlbum}, jQuery.proxy(this.displayAlbumDetails, this));
				}
				$('#albumSelector .activeAlbumTitle').html(album.album_title||'Unknown Album');
				albumSelectorBlock.show();
			}
		},
		hideAlbumDetails: function() {
			$('.contentContainer').hide();
			this.musicLibraryOpen();
		},
		displayAlbumDetails: function(event) {
			this.showAlbumSelectorBlock(event.data.album);
			var albumDetailsContainer = $('#albumDetails' + event.data.album.albumid);
			$('#topScrollFade').hide();
			if (!albumDetailsContainer || albumDetailsContainer.length == 0) {
				$('#spinner').show();
				jQuery.post(JSON_RPC + '?GetSongs', '{"jsonrpc": "2.0", "method": "AudioLibrary.GetSongs", "params": { "fields": ["title", "artist", "genre", "tracknumber", "discnumber", "duration", "year"], "albumid" : ' + event.data.album.albumid + ' }, "id": 1}', jQuery.proxy(function(data) {
					albumDetailsContainer = $('<div>');
					albumDetailsContainer.attr('id', 'albumDetails' + event.data.album.albumid)
										 .addClass('contentContainer')
										 .addClass('albumContainer')
										 .html('<table class="albumView"><thead><tr class="headerRow"><th>Artwork</th><th>&nbsp;</th><th>Name</th><th class="time">Time</th><th>Artist</th><th>Genre</th></tr></thead><tbody class="resultSet"></tbody></table>');
					$('.contentContainer').hide();
					$('#content').append(albumDetailsContainer);
					var albumThumbnail = event.data.album.thumbnail;
					var albumTitle = event.data.album.album_title||'Unknown Album';
					var albumArtist = event.data.album.album_artist||'Unknown Artist';
					var trackCount = data.result.total;
					$.each($(data.result.songs), jQuery.proxy(function(i, item) {
						if (i == 0) {
							var trackRow = $('<tr>').addClass('trackRow').addClass('tr' + i % 2);
							trackRow.append($('<td>').attr('rowspan', ++trackCount + 1).addClass('albumThumb'));
							for (var a = 0; a < 5; a++) {
								trackRow.append($('<td>').html('&nbsp').attr('style', 'display: none'));
							}
							$('#albumDetails' + event.data.album.albumid + ' .resultSet').append(trackRow);
						}
						var trackRow = $('<tr>').addClass('trackRow').addClass('tr' + i % 2);
						var trackNumberTD = $('<td>')
							.html(item.tracknumber)
							.bind('click', { song: item, album: event.data.album }, jQuery.proxy(this.playTrack, this));
						trackRow.append(trackNumberTD);
						var trackTitleTD = $('<td>')
							.html(item.title)
							.bind('click', { song: item, album: event.data.album }, jQuery.proxy(this.playTrack, this));
						trackRow.append(trackTitleTD);
						var trackDurationTD = $('<td>')
							.addClass('time')
							.html(durationToString(item.duration))
							.bind('click', { song: item, album: event.data.album }, jQuery.proxy(this.playTrack, this));
						trackRow.append(trackDurationTD);
						var trackArtistTD = $('<td>')
							.html(item.artist)
							.bind('click', { song: item, album: event.data.album }, jQuery.proxy(this.playTrack, this));
						trackRow.append(trackArtistTD);
						var trackGenreTD = $('<td>')
							.html(item.genre)
							.bind('click', { song: item, album: event.data.album }, jQuery.proxy(this.playTrack, this));
						trackRow.append(trackGenreTD);
						$('#albumDetails' + event.data.album.albumid + ' .resultSet').append(trackRow);
					}, this));
					if (trackCount > 0) {
						var trackRow = $('<tr>').addClass('fillerTrackRow');
						for (var i = 0; i < 5; i++) {
							trackRow.append($('<td>').html('&nbsp'));
						}
						$('#albumDetails' + event.data.album.albumid + ' .resultSet').append(trackRow);

						var trackRow2 = $('<tr>').addClass('fillerTrackRow2');
						trackRow2.append($('<td>').addClass('albumBG').html('&nbsp'));
						for (var i = 0; i < 5; i++) {
							trackRow2.append($('<td>').html('&nbsp'));
						}
						$('#albumDetails' + event.data.album.albumid + ' .resultSet').append(trackRow2);
					}
					$('#albumDetails' + event.data.album.albumid + ' .albumThumb')
						.append(this.generateThumb('album', albumThumbnail, albumTitle, albumArtist))
						.append($('<div>').addClass('footerPadding'));
					$('#spinner').hide();
					myScroll = new iScroll('albumDetails' + event.data.album.albumid);
				}, this), 'json');
			} else {
				$('.contentContainer').hide();
				$('#albumDetails' + event.data.album.albumid).show();
			}
		},
		displayTVShowDetails: function(event) {
			var tvshowDetailsContainer = $('#tvShowDetails' + event.data.tvshow.tvshowid);
			$('#topScrollFade').hide();
			if (!tvshowDetailsContainer || tvshowDetailsContainer.length == 0) {
				$('#spinner').show();
				jQuery.post(JSON_RPC + '?GetTVShowSeasons', '{"jsonrpc": "2.0", "method": "VideoLibrary.GetSeasons", "params": { "fields": ["genre", "director", "trailer", "tagline", "plot", "plotoutline", "title", "originaltitle", "lastplayed", "showtitle", "firstaired", "duration", "season", "episode", "runtime", "year", "playcount", "rating", "writer", "studio", "mpaa", "premiered"], "tvshowid" : ' + event.data.tvshow.tvshowid + ' }, "id": 1}', jQuery.proxy(function(data) {
					tvshowDetailsContainer = $('<div>');
					tvshowDetailsContainer.attr('id', 'tvShowDetails' + event.data.tvshow.tvshowid)
										  .css('display', 'none')
										  .addClass('contentContainer')
										  .addClass('tvshowContainer');
					tvshowDetailsContainer.append(this.generateThumb('tvshow', event.data.tvshow.thumbnail, event.data.tvshow.title));
					if (data && data.result && data.result.seasons && data.result.seasons.length > 0) {
						var absWrapper = $('<div>').addClass('showDetailsWrapper');
						var showDetails = $('<div>').addClass('showDetails');
						showDetails.append($('<p>').html(data.result.seasons[0].showtitle).addClass('showTitle'));
						showDetails.append($('<p>').html('<span class="heading">Genre:</span> ' + data.result.seasons[0].genre));
						showDetails.append($('<p>').html('<span class="heading">Studio:</span> ' + data.result.seasons[0].studio));
						absWrapper.append(showDetails);
						var seasonSelectionContainer = $('<div>').addClass('seasonPicker');
						var seasonSelectionList = $('<ul>');
						var episodeCount = 0;
						var firstSeason;
						$.each($(data.result.seasons), jQuery.proxy(function(i, item) {
							episodeCount += item.episode;
							var season = $('<li>').html(item.title);
							if (i == 0) {
								season.addClass('activeSeason');
								firstSeason = season;
								this.tvActiveShowContainer = tvshowDetailsContainer;
							}
							season.bind('click', {tvshow: event.data.tvshow.tvshowid, season: item, element: season}, jQuery.proxy(this.displaySeasonListings, this));
							seasonSelectionList.append(season);
						}, this));
						showDetails.append($('<p>').html('<span class="heading">Episodes:</span> ' + episodeCount));
						seasonSelectionContainer.append(seasonSelectionList);
						absWrapper.append(seasonSelectionContainer);
						tvshowDetailsContainer.append(absWrapper);
						if (firstSeason) {
							firstSeason.trigger('click');
						}
						$('#content').append(tvshowDetailsContainer);
						tvshowDetailsContainer.fadeIn();
					}
					$('#spinner').hide();
				}, this), 'json');
			} else {
				$('.contentContainer').hide();
				$('#tvShowDetails' + event.data.show.showid);
			}
		},
		displaySeasonListings: function(event) {
			if (event.data.element != this.tvActiveSeason) {
				//Remove style from old season.
				if (this.tvActiveSeason) {
					$(this.tvActiveSeason).removeClass('activeSeason');
				}
				//Hide old listings
				var oldListings = $('.episodeListingsContainer', this.tvActiveShowContainer).fadeOut();
				//Update ActiveSeason
				this.tvActiveSeason = event.data.element;
				$(this.tvActiveSeason).addClass('activeSeason');			
				//Populate new listings
				jQuery.post(JSON_RPC + '?GetTVSeasonEpisodes', '{"jsonrpc": "2.0", "method": "VideoLibrary.GetEpisodes", "params": { "fields": ["genre", "director", "trailer", "tagline", "plot", "plotoutline", "title", "originaltitle", "lastplayed", "showtitle", "firstaired", "duration", "season", "episode", "runtime", "year", "playcount", "rating", "writer", "studio", "mpaa", "premiered"], "season" : ' + event.data.season.season + ', "tvshowid" : ' + event.data.tvshow + ' }, "id": 1}', jQuery.proxy(function(data) {
					var episodeListingsContainer = $('<div>').addClass('episodeListingsContainer');
					var list = $('<ul>');
					$.each($(data.result.episodes), jQuery.proxy(function(i, item) {
						var episodePicture = $('<img>');
						episodePicture.attr('src', this.getThumbnailPath(item.thumbnail));
						var episodeTitle = $('<p>').html(item.title);
						var episode = $('<li>').append(episodePicture).append(episodeTitle);
						list.append(episode);
					}, this));
					episodeListingsContainer.append(list);
					$(this.tvActiveShowContainer).prepend(episodeListingsContainer);
				}, this), 'json');
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
			var movieDetails = $('<div>').attr('id', 'movie-' + event.data.movie.movieid).addClass('moviePopoverContainer');
			movieDetails.append($('<img>').attr('src', '/images/close-button.png').addClass('closeButton').bind('click', jQuery.proxy(this.hideOverlay, this)));
			movieDetails.append($('<img>').attr('src', this.getThumbnailPath(event.data.movie.thumbnail)).addClass('movieCover'));
			movieDetails.append($('<div>').addClass('playIcon').bind('click', {movie: event.data.movie}, jQuery.proxy(this.playMovie, this)));
			var movieTitle = $('<p>').addClass('movieTitle');
			var yearText = event.data.movie.year ? ' <span class="year">(' + event.data.movie.year + ')</span>' : '';
			movieTitle.html(event.data.movie.title + yearText);
			movieDetails.append(movieTitle);
			if (event.data.movie.runtime) {
				movieDetails.append($('<p>').addClass('runtime').html('<strong>Runtime:</strong> ' + event.data.movie.runtime + ' minutes'));
			}
			if (event.data.movie.plot) {
				movieDetails.append($('<p>').addClass('plot').html(event.data.movie.plot));
			}
			if (event.data.movie.genre) {
				movieDetails.append($('<p>').addClass('genre').html('<strong>Genre:</strong> ' + event.data.movie.genre));
			}
			if (event.data.movie.rating) {
				//Todo
			}
			if (event.data.movie.director) {
				movieDetails.append($('<p>').addClass('director').html('<strong>Directed By:</strong> ' + event.data.movie.director));
			}
			this.activeCover = movieDetails;
			$('body').append(movieDetails);
			$('#overlay').show();
			this.updatePlayButtonLocation();
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
			$('.contentContainer').hide();
			var libraryContainer = $('#movieLibraryContainer');
			if (!libraryContainer || libraryContainer.length == 0) {
				$('#spinner').show();
				jQuery.post(JSON_RPC + '?GetMovies', '{"jsonrpc": "2.0", "method": "VideoLibrary.GetMovies", "params": { "start": 0, "fields": ["genre", "director", "trailer", "tagline", "plot", "plotoutline", "title", "originaltitle", "lastplayed", "showtitle", "firstaired", "duration", "season", "episode", "runtime", "year", "playcount", "rating"] }, "id": 1}', jQuery.proxy(function(data) {
					if (data && data.result && data.result.movies) {
							libraryContainer = $('<div>');
							libraryContainer.attr('id', 'movieLibraryContainer')
											.addClass('contentContainer');
							$('#content').append(libraryContainer);
					} else {
						libraryContainer.html('');
					}
					data.result.movies.sort(jQuery.proxy(this.movieTitleSorter, this));
					$.each($(data.result.movies), jQuery.proxy(function(i, item) {
						var floatableMovieCover = this.generateThumb('movie', item.thumbnail, item.title);
						floatableMovieCover.bind('click', { movie: item }, jQuery.proxy(this.displayMovieDetails, this));
						libraryContainer.append(floatableMovieCover);
					}, this));
					libraryContainer.append($('<div>').addClass('footerPadding'));
					$('#spinner').hide();
					libraryContainer.bind('scroll', { activeLibrary: libraryContainer }, jQuery.proxy(this.updateScrollEffects, this));
					libraryContainer.trigger('scroll');
					//$('#libraryContainer img').lazyload();
					myScroll = new iScroll('movieLibraryContainer');
				}, this), 'json');
			} else {
				libraryContainer.show();
				libraryContainer.trigger('scroll');
			}
		},
		tvshowLibraryOpen: function() {
			this.resetPage();
			$('#tvshowLibrary').addClass('selected');
			$('.contentContainer').hide();
			var libraryContainer = $('#tvshowLibraryContainer');
			if (!libraryContainer || libraryContainer.length == 0) {
				$('#spinner').show();
				jQuery.post(JSON_RPC + '?GetTVShows', '{"jsonrpc": "2.0", "method": "VideoLibrary.GetTVShows", "params": { "start": 0, "fields": ["genre", "director", "trailer", "tagline", "plot", "plotoutline", "title", "originaltitle", "lastplayed", "showtitle", "firstaired", "duration", "season", "episode", "runtime", "year", "playcount", "rating"] }, "id": 1}', jQuery.proxy(function(data) {
					if (data && data.result && data.result.tvshows) {
							libraryContainer = $('<div>');
							libraryContainer.attr('id', 'tvshowLibraryContainer')
											.addClass('contentContainer');
							$('#content').append(libraryContainer);
					} else {
						libraryContainer.html('');
					}
					$.each($(data.result.tvshows), jQuery.proxy(function(i, item) {
						var floatableTVShowCover = this.generateThumb('tvshow', item.thumbnail, item.title);
						floatableTVShowCover.bind('click', { tvshow: item }, jQuery.proxy(this.displayTVShowDetails, this));
						libraryContainer.append(floatableTVShowCover);
					}, this));
					libraryContainer.append($('<div>').addClass('footerPadding'));
					//$('#libraryContainer img').lazyload();
					$('#spinner').hide();
					libraryContainer.bind('scroll', { activeLibrary: libraryContainer }, jQuery.proxy(this.updateScrollEffects, this));
					libraryContainer.trigger('scroll');
					myScroll = new iScroll('tvshowLibraryContainer');
				}, this), 'json');
			} else {
				libraryContainer.show();
				libraryContainer.trigger('scroll');
			}
		},
		updateScrollEffects: function(event) {
			if (event.data.activeLibrary && $(event.data.activeLibrary).scrollTop() > 0) {
				$('#topScrollFade').fadeIn();
			} else {
				$('#topScrollFade').fadeOut();
			}
		},
		albumArtistSorter: function(a, b) {
			var result = this.sortAlpha(a.album_artist, b.album_artist);
			if (result == 0) {
				return this.sortAlpha(a.album_title, b.album_title);
			}
			return result;
		},
		sortAlpha: function(aStr, bStr) {
			aStr = aStr.toLowerCase();
			bStr = bStr.toLowerCase();
			if (aStr < bStr) {
				return -1;
			}
			if (aStr > bStr) {
				return 1;
			}
			return 0;
		},
		movieTitleSorter: function(a, b) {
			return this.sortAlpha(a.title, b.title);
		},
		startSlideshow: function(event) {
			var directory = event.data.directory.file;
			jQuery.post(JSON_RPC + '?StartSlideshow', '{"jsonrpc": "2.0", "method": "XBMC.StartSlideshow", "params": { "recursive" : "true", "random":"true", "directory" : "' + directory + '" }, "id": 1}', null, 'json');
		},
		showDirectory: function(event) {
			var directory = event.data.directory.file;
			this.resetPage();
			$('#pictureLibrary').addClass('selected');
			$('.contentContainer').hide();
			var libraryContainer = $('#pictureLibraryDirContainer' + directory);
			if (!libraryContainer || libraryContainer.length == 0) {
				$('#spinner').show();                           
				jQuery.post(JSON_RPC + '?GetDirectory', '{"jsonrpc": "2.0", "method": "Files.GetDirectory", "params": { "media" : "pictures", "directory":"' + directory + '" }, "id": 1}', jQuery.proxy(function(data) {
					if (data && data.result && ( data.result.directories || data.result.files )) {
						libraryContainer = $('<div>');
						libraryContainer.attr('id', 'pictureLibraryDirContainer' + directory)
										.addClass('contentContainer');
						$('#content').append(libraryContainer);
						var breadcrumb = $('<div>');
						var seperator = '/';
						var item = '';
						var directoryArray = directory.split(seperator);
						jQuery.each(directoryArray, function(i,v) {
							if(v != '') {
								item += v + seperator;
								//tmp.bind('click', { directory: item }, jQuery.proxy(this.showDirectory, this));               
								breadcrumb.append($('<div>').text(' > ' + v).css('float','left').addClass('breadcrumb'));                                                 
							}
						});
						libraryContainer.append(breadcrumb);
						libraryContainer.append($('<div>').css('clear','both'));
						if (data.result.directories) {
							$.each($(data.result.directories), jQuery.proxy(function(i, item) {
								var floatableShare = this.generateThumb('directory', item.thumbnail, item.label);
								floatableShare.bind('click', { directory: item }, jQuery.proxy(this.showDirectory, this));              
								//var slideshow = $('<div">');
								//slideshow.html('<div>Slideshow</div>');
								//slideshow.bind('click', { directory: item }, jQuery.proxy(this.startSlideshow, this));        
								//floatableShare.append(slideshow);
								libraryContainer.append(floatableShare);
							}, this));
						}
						if (data.result.files) {
							$.each($(data.result.files), jQuery.proxy(function(i, item) {
								var floatableImage = this.generateThumb('image', item.file, item.label);
								libraryContainer.append(floatableImage);
							}, this));
						}
						libraryContainer.append($('<div>').addClass('footerPadding'));
					} else {
						libraryContainer.html('');
					}
					$('#spinner').hide();
					libraryContainer.bind('scroll', { activeLibrary: libraryContainer }, jQuery.proxy(this.updateScrollEffects, this));
					libraryContainer.trigger('scroll');
					myScroll = new iScroll('#pictureLibraryDirContainer' + directory);
				}, this), 'json');
			} else {
				libraryContainer.show();
				libraryContainer.trigger('scroll');
			}
		},
		pictureLibraryOpen: function() {
			this.resetPage();
			$('#pictureLibrary').addClass('selected');
			$('.contentContainer').hide();
			var libraryContainer = $('#pictureLibraryContainer');
			if (!libraryContainer || libraryContainer.length == 0) {
				$('#spinner').show();
				jQuery.post(JSON_RPC + '?GetSources', '{"jsonrpc": "2.0", "method": "Files.GetSources", "params": { "media" : "pictures" }, "id": 1}',                                  jQuery.proxy(function(data) {
					if (data && data.result && data.result.shares) {
						libraryContainer = $('<div>');
						libraryContainer.attr('id', 'pictureLibraryContainer')
										.addClass('contentContainer');
						$('#content').append(libraryContainer);
					} else {
						libraryContainer.html('');
					}
					$.each($(data.result.shares), jQuery.proxy(function(i, item) {
						var floatableShare = this.generateThumb('directory', item.thumbnail, item.label);
						floatableShare.bind('click', { directory: item }, jQuery.proxy(this.showDirectory, this));                                      
						libraryContainer.append(floatableShare);
					}, this));
					libraryContainer.append($('<div>').addClass('footerPadding'));
					$('#spinner').hide();
					libraryContainer.bind('scroll', { activeLibrary: libraryContainer }, jQuery.proxy(this.updateScrollEffects, this));
					libraryContainer.trigger('scroll');
					myScroll = new iScroll('#pictureLibraryContainer');
				}, this), 'json');
			} else {
				libraryContainer.show();
				libraryContainer.trigger('scroll');
			}
		}
	}