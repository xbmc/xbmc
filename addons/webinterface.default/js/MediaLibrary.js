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

var MediaLibrary = function() {
  this.init();
  return true;
}

MediaLibrary.prototype = {
  playlists: { },

  init: function() {
    this.bindControls();
    this.getPlaylists();
  },

  bindControls: function() {
    $('#musicLibrary').click(jQuery.proxy(this.musicLibraryOpen, this));
    $('#movieLibrary').click(jQuery.proxy(this.movieLibraryOpen, this));
    $('#tvshowLibrary').click(jQuery.proxy(this.tvshowLibraryOpen, this));
    $('#pictureLibrary').click(jQuery.proxy(this.pictureLibraryOpen, this));
    $('#remoteControl').click(jQuery.proxy(this.remoteControlOpen, this));
    $('#overlay').click(jQuery.proxy(this.hideOverlay, this));
    $(window).resize(jQuery.proxy(this.updatePlayButtonLocation, this));
  },
  resetPage: function() {
    $('#musicLibrary').removeClass('selected');
    $('#movieLibrary').removeClass('selected');
    $('#tvshowLibrary').removeClass('selected');
    $('#remoteControl').removeClass('selected');
    $('#pictureLibrary').removeClass('selected');
    this.hideOverlay();
  },
  replaceAll: function(haystack, find, replace) {
    var parts = haystack.split(find);
    var result = "";
    var first = true;
    for (index in parts)
    {
      if (!first)
        result += replace;
      else
        first = false;

      result += parts[index];
    }

    return result;
  },

  getPlaylists: function() {
    jQuery.ajax({
      type: 'POST',
      url: JSON_RPC + '?GetPlaylists',
      data: '{"jsonrpc": "2.0", "method": "Playlist.GetPlaylists", "id": 1}',
      timeout: 3000,
      success: jQuery.proxy(function(data) {
        if (data && data.result && data.result.length > 0) {
          $.each($(data.result), jQuery.proxy(function(i, item) {
            this.playlists[item.type] = item.playlistid;
          }, this));
        }
      }, this),
      error: jQuery.proxy(function(data, error) {
        displayCommunicationError();
        setTimeout(jQuery.proxy(this.updateState, this), 2000);
      }, this),
      dataType: 'json'});
  },
  remoteControlOpen: function(event) {
    this.resetPage();
    $('#remoteControl').addClass('selected');
    $('.contentContainer').hide();
    var libraryContainer = $('#remoteContainer');
    if (!libraryContainer || libraryContainer.length == 0) {
      $('#spinner').show();
      libraryContainer = $('<div>');
      libraryContainer.attr('id', 'remoteContainer')
              .addClass('contentContainer');
      $('#content').append(libraryContainer);
      var keys=[
          {name:'up',width:'40px',height:'30px',top:'28px',left:'58px'}
          ,{name:'down',width:'40px',height:'30px',top:'122px',left:'58px'}
          ,{name:'left',width:'40px',height:'30px',top:'74px',left:'15px'}
          ,{name:'right',width:'40px',height:'30px',top:'74px',left:'104px'}
          ,{name:'ok',width:'40px',height:'30px',top:'74px',left:'58px'}
          ,{name:'back',width:'40px',height:'30px',top:'13px',left:'161px'}
          ,{name:'home',width:'40px',height:'30px',top:'154px',left:'8px'}
          ,{name:'mute',width:'40px',height:'30px',top:'107px',left:'391px'}
          ,{name:'power',width:'30px',height:'30px',top:'-3px',left:'13px'}
          ,{name:'volumeup',width:'30px',height:'30px',top:'49px',left:'422px'}
          ,{name:'volumedown',width:'30px',height:'30px',top:'49px',left:'367px'}
          ,{name:'playpause',width:'32px',height:'23px',top:'62px',left:'260px'}
          ,{name:'stop',width:'32px',height:'23px',top:'62px',left:'211px'}
          ,{name:'next',width:'38px',height:'25px',top:'102px',left:'304px'}
          ,{name:'previous',width:'38px',height:'25px',top:'101px',left:'160px'}
          ,{name:'forward',width:'32px',height:'23px',top:'102px',left:'259px'}
          ,{name:'rewind',width:'32px',height:'23px',top:'101px',left:'211px'}
          ,{name:'cleanlib_a',width:'46px',height:'26px',top:'47px',left:'553px'}
          ,{name:'updatelib_a',width:'46px',height:'26px',top:'47px',left:'492px'}
          ,{name:'cleanlib_v',width:'46px',height:'26px',top:'111px',left:'553px'}
          ,{name:'updatelib_v',width:'46px',height:'26px',top:'111px',left:'492px'}
         ];
      for (var akey in keys) {
        var aremotekey=$('<p>').attr('id',keys[akey]['name']);
        aremotekey.addClass('remote_key')
          .css('height',keys[akey]['height'])
          .css('width',keys[akey]['width'])
          .css('top',keys[akey]['top'])
          .css('left',keys[akey]['left'])
          //.css('border','1px solid black')
          .bind('click',{key: keys[akey]['name']},jQuery.proxy(this.pressRemoteKey,this));
          libraryContainer.append(aremotekey);
      }


    } else {
      libraryContainer.show();
      libraryContainer.trigger('scroll');
    }

    $('#spinner').hide();
  },

  pressRemoteKey: function(event) {
    var keyPressed=event.data.key;
    $('#spinner').show();
    var player = -1;
    // TODO: Get active player
    if($('#videoDescription').is(':visible'))
      player = this.playlists["video"];
    else if($('#audioDescription').is(':visible'))
      player = this.playlists["audio"];

    //common part
    switch(keyPressed) {
      case 'cleanlib_a':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "AudioLibrary.Clean", "id": 1}', function(data){$('#spinner').hide();}, 'json');
        return;
      case 'updatelib_a':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "AudioLibrary.Scan", "id": 1}', function(data){$('#spinner').hide();}, 'json');
        return;
      case 'cleanlib_v':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "VideoLibrary.Clean", "id": 1}', function(data){$('#spinner').hide();}, 'json');
        return;
      case 'updatelib_v':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "VideoLibrary.Scan", "id": 1}', function(data){$('#spinner').hide();}, 'json');
        return;
      case 'back':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Input.Back", "id": 1}', function(data){$('#spinner').hide();}, 'json');
        return;
      case 'home':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Input.Home", "id": 1}', function(data){$('#spinner').hide();}, 'json');
        return;
      case 'mute':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Application.SetMute", "params": { "mute": "toggle" }, "id": 1}', function(data){$('#spinner').hide();}, 'json');
        return;
      case 'power':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "System.Shutdown", "id": 1}', function(data){$('#spinner').hide();}, 'json');
        return;
      case 'volumeup':
      jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Application.GetProperties", "params": { "properties": [ "volume" ] }, "id": 1}', function(data){
        var volume = data.result.volume + 1;
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Application.SetVolume", "params": { "volume": '+volume+' }, "id": 1}', function(data){
          $('#spinner').hide();
        }, 'json');
       }, 'json');
        return;
      case 'volumedown':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Application.GetProperties", "params": { "properties": [ "volume" ] }, "id": 1}', function(data){
          var volume = data.result.volume - 1;
          jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Application.SetVolume", "params": { "volume": '+volume+' }, "id": 1}', function(data){
            $('#spinner').hide();
          }, 'json');
       }, 'json');
        return;
    }

    switch(keyPressed) {
      case 'up':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Input.Up", "id": 1}', function(data){$('#spinner').hide();}, 'json');
        return;
      case 'down':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Input.Down", "id": 1}', function(data){$('#spinner').hide();}, 'json');
        return;
      case 'left':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Input.Left", "id": 1}', function(data){$('#spinner').hide();}, 'json');
        return;
      case 'right':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Input.Right", "id": 1}', function(data){$('#spinner').hide();}, 'json');
        return;
      case 'ok':
        jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Input.Select", "id": 1}', function(data){$('#spinner').hide();}, 'json');
        return;
    }

    if (player >= 0)
    {
      switch(keyPressed) {
        case 'playpause':
          jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Player.PlayPause", "params": { "playerid": ' + player + ' }, "id": 1}', function(data){$('#spinner').hide();}, 'json');
          return;
        case 'stop':
          jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Player.Stop", "params": { "playerid": ' + player + ' }, "id": 1}', function(data){$('#spinner').hide();}, 'json');
          return;
        case 'next':
          jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Player.GoNext", "params": { "playerid": ' + player + ' }, "id": 1}', function(data){$('#spinner').hide();}, 'json');
          return;
        case 'previous':
          jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Player.GoPrevious", "params": { "playerid": ' + player + ' }, "id": 1}', function(data){$('#spinner').hide();}, 'json');
          return;
        case 'forward':
          jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Player.SetSpeed", "params": { "playerid": ' + player + ', "speed": "increment" }, "id": 1}', function(data){$('#spinner').hide();}, 'json');
          return;
        case 'rewind':
          jQuery.post(JSON_RPC + '?SendRemoteKey', '{"jsonrpc": "2.0", "method": "Player.SetSpeed", "params": { "playerid": ' + player + ', "speed": "decrement" }, "id": 1}', function(data){$('#spinner').hide();}, 'json');
          return;
      }
    }
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
      jQuery.post(JSON_RPC + '?GetAlbums', '{"jsonrpc": "2.0", "method": "AudioLibrary.GetAlbums", "params": { "limits": { "start": 0 }, "properties": ["description", "theme", "mood", "style", "type", "albumlabel", "artist", "genre", "rating", "title", "year", "thumbnail"], "sort": { "method": "artist" } }, "id": 1}', jQuery.proxy(function(data) {
        if (data && data.result && data.result.albums) {
          this.albumList = data.result.albums;
          $.each($(this.albumList), jQuery.proxy(function(i, item) {
            var floatableAlbum = this.generateThumb('album', item.thumbnail, item.title, item.artist);
            floatableAlbum.bind('click', { album: item }, jQuery.proxy(this.displayAlbumDetails, this));
            libraryContainer.append(floatableAlbum);
          }, this));
          libraryContainer.append($('<div>').addClass('footerPadding'));
          $('#spinner').hide();
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
  generateThumb: function(type, thumbnail, title, artist) {
    var floatableAlbum = $('<div>');
    var path = this.getThumbnailPath(thumbnail);
    title = title || '';
    artist = artist ||'';
    if (title.length > 18 && !(title.length <= 21)) {
      title = title.substring(0, 18) + '...';
    }
    if (artist.length > 20 && !(artist.length <= 22)) {
      artist = artist.substring(0, 20) + '...';
    }
    var className = '';
    var code = '';
    switch(type) {
      case 'album':
        className = 'floatableAlbum';
        code = '<p class="album" title="' + title + '">' + title + '</p><p class="artist" title="' + artist + '">' + artist + '</p>';
        break;
      case 'movie':
        className = 'floatableMovieCover';
        code = '<p class="album" title="' + title + '">' + title + '</p>';
        break;
      case 'tvshow':
        className = 'floatableTVShowCover';
        break;
      case 'tvshowseason':
        className = 'floatableTVShowCoverSeason';
        break;
      case 'image':
      case 'directory':
        className = 'floatableAlbum';
        code = '<p class="album" title="' + title + '">' + title + '</p>';
        break;
    }
    return floatableAlbum.addClass(className).html('<div class="imgWrapper"><div class="inner"><img src="' + path + '" alt="' + title + '" /></div></div>' + code);
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
      $('#albumSelector .activeAlbumTitle').html(album.title||'Unknown Album');
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
      jQuery.post(JSON_RPC + '?GetSongs', '{"jsonrpc": "2.0", "method": "AudioLibrary.GetSongs", "params": { "properties": ["title", "artist", "genre", "track", "duration", "year", "rating", "playcount"], "albumid" : ' + event.data.album.albumid + ' }, "id": 1}', jQuery.proxy(function(data) {
        albumDetailsContainer = $('<div>');
        albumDetailsContainer.attr('id', 'albumDetails' + event.data.album.albumid)
                   .addClass('contentContainer')
                   .addClass('albumContainer')
                   .html('<table class="albumView"><thead><tr class="headerRow"><th>Artwork</th><th>&nbsp;</th><th>Name</th><th class="time">Time</th><th>Artist</th><th>Genre</th></tr></thead><tbody class="resultSet"></tbody></table>');
        $('.contentContainer').hide();
        $('#content').append(albumDetailsContainer);
        var albumThumbnail = event.data.album.thumbnail;
        var albumTitle = event.data.album.title||'Unknown Album';
        var albumArtist = event.data.album.artist||'Unknown Artist';
        var trackCount = data.result.limits.total;
        $.each($(data.result.songs), jQuery.proxy(function(i, item) {
          if (i == 0) {
            var trackRow = $('<tr>').addClass('trackRow').addClass('tr' + i % 2);
            trackRow.append($('<td>').attr('rowspan', ++trackCount + 1).addClass('albumThumb'));
            for (var a = 0; a < 5; a++) {
              trackRow.append($('<td>').html('&nbsp').attr('style', 'display: none'));
            }
            $('#albumDetails' + event.data.album.albumid + ' .resultSet').append(trackRow);
          }
          var trackRow = $('<tr>').addClass('trackRow').addClass('tr' + i % 2).bind('click', { album: event.data.album, itmnbr: i }, jQuery.proxy(this.playTrack,this));
          var trackNumberTD = $('<td>')
            .html(item.track)

          trackRow.append(trackNumberTD);
          var trackTitleTD = $('<td>')
            .html(item.title);

          trackRow.append(trackTitleTD);
          var trackDurationTD = $('<td>')
            .addClass('time')
            .html(durationToString(item.duration));

          trackRow.append(trackDurationTD);
          var trackArtistTD = $('<td>')
            .html(item.artist);

          trackRow.append(trackArtistTD);
          var trackGenreTD = $('<td>')
            .html(item.genre);

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

  togglePosterView: function(event){
    var view=event.data.mode;
    var wthumblist,hthumblist,hthumbdetails;
    $("#toggleBanner").removeClass('activeMode');
    $("#togglePoster").removeClass('activeMode');
    $("#toggleLandscape").removeClass('activeMode');
    switch(view) {
      case 'poster':
        setCookie('TVView','poster');
        wthumblist='135px';
        hthumblist='199px';
        hthumbdetails='559px';
        $("#togglePoster").addClass('activeMode');
        break;
      case 'landscape':
        setCookie('TVView','landscape');
        wthumblist='210px';
        hthumblist='118px';
        hthumbdetails='213px';
        $("#toggleLandscape").addClass('activeMode');
        break;
      default: //set banner view as default
        setCookie('TVView','banner');
        wthumblist='379px';
        hthumblist='70px';
        hthumbdetails='70px';
        $("#toggleBanner").addClass('activeMode');
        break;
    }
    $(".floatableTVShowCover, .floatableTVShowCover div.imgWrapper, .floatableTVShowCover img, .floatableTVShowCover div.imgWrapper div.inner").css('width',wthumblist).css('height',hthumblist);
    $(".floatableTVShowCoverSeason div.imgWrapper, .floatableTVShowCoverSeason div.imgWrapper div.inner,.floatableTVShowCoverSeason img, .floatableTVShowCoverSeason").css('height',hthumbdetails);
  },

  displayTVShowDetails: function(event) {
    var tvshowDetailsContainer = $('#tvShowDetails' + event.data.tvshow.tvshowid);
    $('#topScrollFade').hide();
    toggle=this.toggle.detach();
    if (!tvshowDetailsContainer || tvshowDetailsContainer.length == 0) {
      $('#spinner').show();
      jQuery.post(JSON_RPC + '?GetTVShowSeasons', '{"jsonrpc": "2.0", "method": "VideoLibrary.GetSeasons", "params": { "properties": [ "season", "showtitle", "playcount", "episode", "thumbnail","fanart" ], "tvshowid" : ' + event.data.tvshow.tvshowid + ' }, "id": 1}', jQuery.proxy(function(data) {
        tvshowDetailsContainer = $('<div>');
        tvshowDetailsContainer.attr('id', 'tvShowDetails' + event.data.tvshow.tvshowid)
                              .css('display', 'none')
                              .addClass('contentContainer')
                              .addClass('tvshowContainer');
        var showThumb = this.generateThumb('tvshowseason', event.data.tvshow.thumbnail, event.data.tvshow.title);
        if (data && data.result && data.result.seasons && data.result.seasons.length > 0) {
          var showDetails = $('<div>').addClass('showDetails');
          showDetails.append(toggle);
          showDetails.append($('<p>').html(data.result.seasons[0].showtitle).addClass('showTitle'));
          var seasonSelectionSelect = $('<select>').addClass('seasonPicker');
          //var episodeCount = 0;
          this.tvActiveShowContainer = tvshowDetailsContainer;
          //var fanart;
          $.each($(data.result.seasons), jQuery.proxy(function(i, item) {
//              if(fanart==null && item.fanart!=null){
//                fanart=item.fanart;
//              }
//              //episodeCount += item.episode;
            var season = $('<option>').attr('value',i);
            season.text(item.label);
            seasonSelectionSelect.append(season);

          }, this));
//            if(fanart!=null)
//            {
//              $('.contentContainer').css('background','url("'+this.getThumbnailPath(fanart)+'")').css('background-size','cover');
//            }
          seasonSelectionSelect.bind('change', {tvshow: event.data.tvshow.tvshowid, seasons: data.result.seasons, element: seasonSelectionSelect}, jQuery.proxy(this.displaySeasonListings, this));
          //showDetails.append($('<p>').html('<span class="heading">Episodes:</span> ' + episodeCount));
          showDetails.append(seasonSelectionSelect);
          tvshowDetailsContainer.append(showDetails);
          tvshowDetailsContainer.append(showThumb);
          seasonSelectionSelect.trigger('change');
          $('#content').append(tvshowDetailsContainer);
          if(getCookie('TVView')!=null && getCookie('TVView')!='banner'){
          var view=getCookie('TVView');
          switch(view) {
            case 'poster':
              togglePoster.trigger('click');
              break;
            case 'landscape':
              toggleLandscape.trigger('click')
              break;
          }
        }
          tvshowDetailsContainer.fadeIn();
        }
        $('#spinner').hide();
      }, this), 'json');
    } else {
      $('.contentContainer').hide();
      $('#tvShowDetails' + event.data.tvshow.tvshowid).show();
      $('#tvShowDetails' + event.data.tvshow.tvshowid +' select').trigger('change');
    }
  },
  displaySeasonListings: function(event) {
    //retrieve selected season
    var selectedVal=event.data.element.val();
    var seasons=event.data.seasons;
    $('#topScrollFade').hide();
    //Hide old listings
    var oldListings = $('.episodeListingsContainer', this.tvActiveShowContainer).fadeOut();
    //Update ActiveSeason
    this.tvActiveSeason = selectedVal;
    //Populate new listings
    jQuery.post(JSON_RPC + '?GetTVSeasonEpisodes', '{"jsonrpc": "2.0", "method": "VideoLibrary.GetEpisodes", "params": { "properties": [ "title", "thumbnail","episode","plot","season"], "season" : ' + seasons[selectedVal].season + ', "tvshowid" : ' + event.data.tvshow + ' }, "id": 1}', jQuery.proxy(function(data) {
      var episodeListingsContainer = $('<div>').addClass('episodeListingsContainer');
      var episodeTable= $('<table>').addClass('seasonView').html('<thead><tr class="headerRow"><th class="thumbHeader">N&deg;</th><th>Title</th><th class="thumbHeader">Thumb</th><th class="thumbHeader">Details</th></tr></thead><tbody class="resultSet"></tbody>');
      $.each($(data.result.episodes), jQuery.proxy(function(i, item) {
        var episodeRow = $('<tr>').addClass('episodeRow').addClass('tr' + i % 2);
        var episodePictureImg = $('<img>').bind('click', { episode: item }, jQuery.proxy(this.playTVShow, this)).css('cursor','pointer');
        episodePictureImg.attr('src', this.getThumbnailPath(item.thumbnail));
        var episodePicture=$('<td>').addClass('episodeThumb').append(episodePictureImg).bind('click', { episode: item }, jQuery.proxy(this.playTVShow, this));
        var episodeNumber = $('<td>').addClass('episodeNumber').html(item.episode).bind('click', { episode: item }, jQuery.proxy(this.playTVShow, this));
        var episodeTitle = $('<td>').html(item.title).bind('click', { episode: item }, jQuery.proxy(this.playTVShow, this));
        var episodeDetails = $('<td class="info">').html('').bind('click',{episode:item}, jQuery.proxy(this.displayEpisodeDetails, this)).css('cursor','pointer');
        episodeRow.append(episodeNumber).append(episodeTitle).append(episodePicture).append(episodeDetails);
        episodeTable.append(episodeRow);
      }, this));
      episodeListingsContainer.append(episodeTable);
      $(this.tvActiveShowContainer).append(episodeListingsContainer);
    }, this), 'json');

  },

  displayEpisodeDetails: function(event) {
    var episodeDetails = $('<div>').attr('id', 'episode-' + event.data.episode.episodeid).addClass('episodePopoverContainer');
    episodeDetails.append($('<img>').attr('src', '/images/close-button.png').addClass('closeButton').bind('click', jQuery.proxy(this.hideOverlay, this)));
    episodeDetails.append($('<img>').attr('src', this.getThumbnailPath(event.data.episode.thumbnail)).addClass('episodeCover'));
    episodeDetails.append($('<div>').addClass('playIcon').bind('click', {episode: event.data.episode}, jQuery.proxy(this.playTVShow, this)));
    var episodeTitle = $('<p>').addClass('episodeTitle');
    var yearText = event.data.episode.year ? ' <span class="year">(' + event.data.episode.year + ')</span>' : '';
    episodeTitle.html(event.data.episode.title + yearText);
    episodeDetails.append(episodeTitle);
    if (event.data.episode.runtime) {
      episodeDetails.append($('<p>').addClass('runtime').html('<strong>Runtime:</strong> ' + event.data.epispde.runtime + ' minutes'));
    }
    if (event.data.episode.season) {
      episodeDetails.append($('<p>').addClass('season').html('<strong>Season:</strong> ' + event.data.episode.season));
    }
    if (event.data.episode.episode) {
      episodeDetails.append($('<p>').addClass('episode').html('<strong>Episode:</strong> ' + event.data.episode.episode));
    }
    if (event.data.episode.plot) {
      episodeDetails.append($('<p>').addClass('plot').html('<strong>Plot:</strong> <br/><br/>' +event.data.episode.plot));
    }
    if (event.data.episode.genre) {
      episodeDetails.append($('<p>').addClass('genre').html('<strong>Genre:</strong> ' + event.data.episode.genre));
    }
    if (event.data.episode.rating) {
      //Todo
    }
    if (event.data.episode.director) {
      episodeDetails.append($('<p>').addClass('director').html('<strong>Directed By:</strong> ' + event.data.episode.director));
    }
    this.activeCover = episodeDetails;
    $('body').append(episodeDetails);
    $('#overlay').show();
    this.updatePlayButtonLocation();
  },

  playTVShow: function(event) {
    jQuery.post(JSON_RPC + '?AddTvShowToPlaylist', '{"jsonrpc": "2.0", "method": "Player.Open", "params": { "item": { "episodeid": ' + event.data.episode.episodeid + ' } }, "id": 1}', jQuery.proxy(function(data) {
      this.hideOverlay();
    }, this), 'json');
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
        var heightpi=$(movieContainer[0]).height();
        playIcon.width(Math.floor(0.65*heightpi));
        playIcon.height(heightpi);
      }
    }
    var episodeContainer = $('.episodeCover');
    if (episodeContainer.length > 0) {
      var playIcon = $('.playIcon');
      if (playIcon.length > 0) {
        var widthpi=$(episodeContainer[0]).width();
        playIcon.width(widthpi);
        //assume 16/9 thumb
        playIcon.height(Math.floor(widthpi*9/16));
      }
    }
  },
  playMovie: function(event) {
    jQuery.post(JSON_RPC + '?PlayMovie', '{"jsonrpc": "2.0", "method": "Player.Open", "params": { "item": { "movieid": ' + event.data.movie.movieid + ' } }, "id": 1}', jQuery.proxy(function(data) {
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
    jQuery.post(JSON_RPC + '?ClearPlaylist', '{"jsonrpc": "2.0", "method": "Playlist.Clear", "params": { "playlistid": ' + this.playlists["audio"] + ' }, "id": 1}', jQuery.proxy(function(data) {
      //check that clear worked.
      jQuery.post(JSON_RPC + '?AddAlbumToPlaylist', '{"jsonrpc": "2.0", "method": "Playlist.Add", "params": { "playlistid": ' + this.playlists["audio"] + ', "item": { "albumid": ' + event.data.album.albumid + ' } }, "id": 1}', jQuery.proxy(function(data) {
        //play specific song in playlist
        jQuery.post(JSON_RPC + '?PlaylistItemPlay', '{"jsonrpc": "2.0", "method": "Player.Open", "params": { "item": { "playlistid": ' + this.playlists["audio"] + ', "position": '+ event.data.itmnbr + ' } }, "id": 1}', function() {}, 'json');
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
      jQuery.post(JSON_RPC + '?GetMovies', '{"jsonrpc": "2.0", "method": "VideoLibrary.GetMovies", "params": { "limits": { "start": 0 }, "properties": [ "genre", "director", "trailer", "tagline", "plot", "plotoutline", "title", "originaltitle", "lastplayed", "runtime", "year", "playcount", "rating", "thumbnail", "file" ], "sort": { "method": "sorttitle", "ignorearticle": true } }, "id": 1}', jQuery.proxy(function(data) {
        if (data && data.result && data.result.movies) {
            libraryContainer = $('<div>');
            libraryContainer.attr('id', 'movieLibraryContainer')
                    .addClass('contentContainer');
            $('#content').append(libraryContainer);
        } else {
          libraryContainer.html('');
        }
        //data.result.movies.sort(jQuery.proxy(this.movieTitleSorter, this));
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
      toggle=$('<p>').addClass('toggle');
      togglePoster= $('<span>Poster</span>');
      togglePoster.attr('id', 'togglePoster')
            .css('cursor','pointer')
            .bind('click',{mode: 'poster'},jQuery.proxy(this.togglePosterView,this));
      toggleBanner= $('<span>Banner</span>');
      toggleBanner.attr('id', 'toggleBanner')
            .css('cursor','pointer')
            .addClass('activeMode')
            .bind('click',{mode: 'banner'},jQuery.proxy(this.togglePosterView,this));
      toggleLandscape= $('<span>Landscape</span>');
      toggleLandscape.attr('id', 'toggleLandscape')
            .css('cursor','pointer')
            .bind('click',{mode: 'landscape'},jQuery.proxy(this.togglePosterView,this));
      toggle.append(toggleBanner).append(' | ').append(togglePoster).append(' | ').append(toggleLandscape);
      this.toggle=toggle;
      jQuery.post(JSON_RPC + '?GetTVShows', '{"jsonrpc": "2.0", "method": "VideoLibrary.GetTVShows", "params": { "properties": ["genre", "plot", "title", "lastplayed", "episode", "year", "playcount", "rating", "thumbnail", "studio", "mpaa", "premiered"] }, "id": 1}', jQuery.proxy(function(data) {
        if (data && data.result && data.result.tvshows) {
            libraryContainer = $('<div>');
            libraryContainer.append(toggle);
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
        $('#spinner').hide();
        libraryContainer.bind('scroll', { activeLibrary: libraryContainer }, jQuery.proxy(this.updateScrollEffects, this));
        libraryContainer.trigger('scroll');
        myScroll = new iScroll('tvshowLibraryContainer');
        if(getCookie('TVView')!=null && getCookie('TVView')!='banner'){
          var view=getCookie('TVView');
          switch(view) {
            case 'poster':
              togglePoster.trigger('click');
              break;
            case 'landscape':
              toggleLandscape.trigger('click')
              break;
          }
        }
      }, this), 'json');
    } else {
      libraryContainer.prepend($(".toggle").detach()).show();
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
  startSlideshow: function(event) {
    jQuery.post(JSON_RPC + '?StartSlideshow', '{"jsonrpc": "2.0", "method": "Player.Open", "params": { "item": { "recursive" : "true", "random":"true", "path" : "' + this.replaceAll(event.data.directory.file, "\\", "\\\\") + '" } }, "id": 1}', null, 'json');
  },
  showDirectory: function(event) {
    var directory = event.data.directory.file;
    var jsonDirectory = this.replaceAll(directory, "\\", "\\\\");
    this.resetPage();
    $('#pictureLibrary').addClass('selected');
    $('.contentContainer').hide();
    var libraryContainer = $('#pictureLibraryDirContainer' + directory);
    if (!libraryContainer || libraryContainer.length == 0) {
      $('#spinner').show();
      jQuery.post(JSON_RPC + '?GetDirectory', '{"jsonrpc": "2.0", "method": "Files.GetDirectory", "params": { "media" : "pictures", "directory": "' + jsonDirectory + '" }, "id": 1}', jQuery.proxy(function(data) {
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
          if (data.result.files) {
            $.each($(data.result.files), jQuery.proxy(function(i, item) {
              if (item.filetype == "file")
              {
                var floatableImage = this.generateThumb('image', item.file, item.label);
                libraryContainer.append(floatableImage);
              }
              else if (item.filetype == "directory")
              {
                var floatableShare = this.generateThumb('directory', item.thumbnail, item.label);
                floatableShare.bind('click', { directory: item }, jQuery.proxy(this.showDirectory, this));
                //var slideshow = $('<div">');
                //slideshow.html('<div>Slideshow</div>');
                //slideshow.bind('click', { directory: item }, jQuery.proxy(this.startSlideshow, this));
                //floatableShare.append(slideshow);
                libraryContainer.append(floatableShare);
              }
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
      jQuery.post(JSON_RPC + '?GetSources', '{"jsonrpc": "2.0", "method": "Files.GetSources", "params": { "media" : "pictures" }, "id": 1}', jQuery.proxy(function(data) {
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