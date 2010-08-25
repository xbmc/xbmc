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
			$('#videoLibrary').click(jQuery.proxy(this.videoLibraryOpen, this));
		},
		musicLibraryOpen: function(event) {
			$('#musicLibrary').addClass('selected');
			$('#videoLibrary').removeClass('selected');
			jQuery.post(JSON_RPC + '?GetAlbums', '{"jsonrpc": "2.0", "method": "AudioLibrary.GetAlbums", "params": { "fields": ["album_description", "album_theme", "album_mood", "album_style", "album_type", "album_label", "album_artist", "album_genre", "album_rating", "album_title"] }, "id": 1}', jQuery.proxy(function(data) {
				if (data && data.result && data.result.albums) {
					$('#content').html('');
					$.each($(data.result.albums), jQuery.proxy(function(i, item) {
						var floatableAlbum = $('<div>');
						var path = item.thumbnail ? ('/vfs/' + item.thumbnail) : DEFAULT_ALBUM_COVER;
						var title = item.album_title;
						if (title.length > 18 && !(title.length <= 21)) {
							title = item.album_title.substring(0, 18) + '...';
						}
						floatableAlbum.addClass('floatableAlbum')
									  .html('<div class="imgWrapper"><img src="' + path + '" alt="" /></div><p class="album" title="' + item.album_title + '">' + title + '</p><p class="artist">' + item.album_artist + '</p>')
									  .bind('click', {albumId: item.albumid}, jQuery.proxy(this.displayAlbumDetails, this));
						$('#content').append(floatableAlbum);
					}, this));
				}
			}, this), 'json');
		},
		displayAlbumDetails: function(event) {
			jQuery.post(JSON_RPC + '?GetSongs', '{"jsonrpc": "2.0", "method": "AudioLibrary.GetSongs", "params": { "albumid" : ' + event.data.albumId + ' }, "id": 1}', jQuery.proxy(function(data) {
				$('#content').html('<table><tr><th>Artwork</th><th>&nbsp;</th><th>Name</th><th>Time</th><th>Artist</th><th>Genre</th></tr><tbody id="resultSet"></tbody></table>');
				console.log(data);
			}, this));
		},
		videoLibraryOpen: function() {
			$('#musicLibrary').removeClass('selected');
			$('#videoLibrary').addClass('selected');
			$('#content').html('');
		}
	}
