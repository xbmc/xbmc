/*
 * Copyright (c) 2007, Thomas Robinson
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of the tlrobinson.net nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

function executeXbmcCommand(pageId)
{
	var pageName = decodeURIComponent(pageId.substr(6));
	
	var args = pageName.split(";");
	switch(args[0]) {
		case "list": 			xbmcListing(args, pageName); break;
		case "playlist": 		xbmcPlaylist(args); break;
		case "play": 			xbmcPlay(args); break;
		case "playlistplay":	xbmcPlaylistPlay(args); break;
		case "add": 			xbmcAddToPlaylist(args); break;
		case "remove": 			xbmcRemoveFromPlaylist(args); break;break;
		case "clear": 			xbmcClearPlaylist(args); break;
		case "nowplaying": 		xbmcNowPlaying(); break;
		case "scripts": 		xbmcScripts(); break;
		case "runscript": 		xbmcRunScript(args); break;
		default: alert("***DEBUG***: unknown command");
	}
}

function xbmcScripts() {
	xbmcHttpSimple("GetMediaLocation(files;q:\\Scripts)", function(req) {
		var newPage = document.createElement("ul");
		newPage.setAttribute("id", "xbmc-nowplaying");
		newPage.setAttribute("title", "Scripts");
		
		var lines = req.responseText.split("\n");
		for (var i = 0; i < lines.length; i++) {
			var fields = lines[i].split(";");
			if (lines[i] != "" && fields.length == 3) {
				var listItem = document.createElement("li");
				var pageLink = document.createElement("a");
				var scriptPath = (fields[2] == "1") ? fields[1]+"default.py" : fields[1];
				pageLink.setAttribute("href", "#xbmc-runscript;" + scriptPath);
				pageLink.appendChild(document.createTextNode(fields[0]));
				listItem.appendChild(pageLink);
				newPage.appendChild(listItem);
			}
		}
		iui.insertPage(newPage);
	});
}

function xbmcRunScript(args) { xbmcHttpSimple("ExecBuiltIn(RunScript("+args[1]+"))", null); }

function xbmcClearPlaylist(args) {
	switch(args[1]) {
		case "Music": 		command = "ClearPlayList(0)"; break;
		case "Video":		command = "ClearPlayList(1)"; break;
		case "Slideshow":  	command = "ClearSlideshow"; break;
		default: alert("***DEBUG***: unknown playlist: " + args[1]); return;
	}
	xbmcHttpSimple(command, function(req) {
		if (req.responseText.substring(0, 2) == "OK")
			xbmcPlaylist(args);
		else
			alert("***DEBUG***: " + req.responseText);
	});
}

function xbmcPlay(args) {
	xbmcHttpSimple("PlayFile("+args[1]+")", function(req) {
		if (req.responseText.substring(0, 2) == "OK")
			xbmcNowPlaying();
		else
			alert("***DEBUG***: " + req.responseText);
	});
}

function xbmcPlaylistPlay(args) {
	var command1, command2;
	switch (args[1]) {
		case "Music":
			command1 = "SetCurrentPlaylist(0)";
			command2 = "SetPlaylistSong("+((args.length>2)?args[2]:0)+")";
			break;
		case "Video":
			command1 = "SetCurrentPlaylist(1)";
			command2 = "SetPlaylistSong("+((args.length>2)?args[2]:0)+")";
			break;
		case "Slideshow":
			command1 = "SlideshowSelect("+args[2]+")";
			command2 = "PlaySlideshow";
			break;
		default:
			alert("***DEBUG***: unknown playlist: " + args[1]);
	}
	
	xbmcHttpSimple(command1, function(req1) {
		if (req1.responseText.substring(0, 2) != "OK")
			alert("***DEBUG***: " + req1.responseText);
		else {
			if (command2 != "") {
				xbmcHttpSimple(command2, function(req2) {
					if (req2.responseText.substring(0, 2) != "OK")
						alert("***DEBUG***: " + req2.responseText);
					else
						xbmcNowPlaying();
				});
			} else
				xbmcNowPlaying();
		}
	});
}

function xbmcAddToPlaylist(args) {
	var command;
	switch (args[1]) {
		case "Music":    command = "AddToPlayList(" +args[2]+";0)"; break;
		case "Video":    command = "AddToPlayList(" +args[2]+";1)"; break;
		case "Pictures": command = "AddToSlideshow("+args[2]+")"; break;
		default:         command = "AddToPlayList(" +args[2]+")"; break;
	}
	xbmcHttpSimple(command, function(req) {
		if (req.responseText.substring(0, 2) != "OK")
			alert("***DEBUG***: " + req.responseText); //debug
		else {
		}
	});
}

function xbmcRemoveFromPlaylist(args) {
	var command;
	switch (args[2]) {
		case "Music":    command = "RemoveFromPlaylist("+args[1]+";0)"; break;
		case "Video":    command = "RemoveFromPlaylist("+args[1]+";1)"; break;
		default: alert("***DEBUG*** [can't do that]"); return;
	}
	xbmcHttpSimple(command, function(req) {
		if (req.responseText.substring(0, 2) != "OK")
			alert("***DEBUG***: " + req.responseText);
		else {
			var liId = args[1]+";"+args[2];
			var elem = $(liId);
			if (elem) {
				elem.parentNode.removeChild(elem);
			}
		}
	});
}

function xbmcListing(args, pageName) {
	var command = (args.length==3)?"GetMediaLocation("+args[1]+";"+args[2]+";)":"GetShares("+args[1]+";appendone)"
	
	xbmcHttpSimple(command, function(req) {
		var newPage = document.createElement("ul");
		newPage.setAttribute("id", pageName);
		newPage.setAttribute("title", pageName.split(";").pop().match(/[^\\/]+(?=[\\/]?$)/)[0]); // get just the last part of the path
		
		var lines = req.responseText.split("\n");
		for (var i = 0; i < lines.length; i++) {
			var fields = lines[i].split(";");
			if (lines[i] != "" && fields.length == 3) {
				
				var listItem = document.createElement("li");
				var pageLink = document.createElement("a");
				
				var classes = "";
				
				//if (fields[1].search(/multipath:\/\//i) < 0 && fields[1].search(/shout:\/\//i) < 0) {
					classes += "addSet ";
					
					var addLink = document.createElement("a");
					addLink.setAttribute("href", "#xbmc-add;" + args[1] + ";" + fields[1]); // AddToPlayList(media;[playlist];[mask])
					addLink.appendChild(document.createTextNode(""));
					listItem.appendChild(addLink);
				//}
				
				if (fields[2] == "1") {
					pageLink.setAttribute("href", "#xbmc-list;" + args[1] + ";" + fields[1]);
				} else {
					pageLink.setAttribute("href", "#xbmc-play;" + fields[1]);
					classes += " playButton";
				}	
				listItem.setAttribute("class", classes);

				pageLink.appendChild(document.createTextNode(fields[0]));
				
				listItem.appendChild(pageLink);
				
				newPage.appendChild(listItem);
			}
		}
		iui.insertPage(newPage);
	});
}

function xbmcPlaylist(args) {
	var command;
	switch (args[1]) {
		case "Music":     command = "GetPlaylistContents(0)"; break;
		case "Video":     command = "GetPlaylistContents(1)"; break;
		case "Slideshow": command = "GetSlideshowContents"; break;
		default: alert("***DEBUG***: unknown playlist: " + args[1]); return;
	}
	xbmcHttpSimple(command, function(req) {
		var is_new = false;
		var id = args[1] + "_Playlist";
		var playlist = $(id);
		if (!playlist) {
			is_new = true;
			playlist = document.createElement("ul");
			playlist.setAttribute("id", id);
			playlist.setAttribute("title", args[1] + " Playlist");
		} else {
			while (playlist.hasChildNodes())
				playlist.removeChild(playlist.firstChild);
		}
		
		var listItem, link;
		
		listItem = document.createElement("li");
		listItem.setAttribute("class", "group");
		listItem.appendChild(document.createTextNode("Options"));
		playlist.appendChild(listItem);
		
		listItem = document.createElement("li");
		link = document.createElement("a");
		link.setAttribute("href", "#xbmc-playlistplay;"+args[1]);
		link.appendChild(document.createTextNode("Play Playlist"));
		listItem.appendChild(link);
		playlist.appendChild(listItem);
		
		listItem = document.createElement("li");
		link = document.createElement("a");
		link.setAttribute("href", "#xbmc-clear;"+args[1]);
		link.appendChild(document.createTextNode("Clear All"));
		listItem.appendChild(link);
		playlist.appendChild(listItem);
		
		listItem = document.createElement("li");
		listItem.setAttribute("class", "group");
		listItem.appendChild(document.createTextNode("Playlist Items"))
		playlist.appendChild(listItem);
		
		var lines = req.responseText.split("\n");
		if (lines[0] != "[Empty]") {
			for (var i = 1; i < lines.length; i++) {
				listItem = document.createElement("li");
				listItem.setAttribute("class", "removeSet playButton");
				listItem.setAttribute("id", lines[i]+";"+args[1]);
				
				var addLink = document.createElement("a");
				addLink.setAttribute("href", "#xbmc-remove;"+lines[i]+";"+args[1]); // RemoveFromPlaylist(filename;[playlist])
				addLink.appendChild(document.createTextNode(""));
				
				var pageLink = document.createElement("a");
				var playId = (args[1]=="Slideshow")?lines[i]:(i-1);
				pageLink.setAttribute("href", "#xbmc-playlistplay;"+args[1]+";"+playId);
				pageLink.appendChild(document.createTextNode(lines[i]));
				
				listItem.appendChild(addLink);
				listItem.appendChild(pageLink);
				
				playlist.appendChild(listItem);
			}
		} else {
			//alert("Empty playlist! " + args[1])
		}
		
		if (is_new)
			iui.insertPage(playlist);
		else 
			iui.showPage(playlist);
	});
}

function xbmcNowPlaying() {
	xbmcHttpSimple("GetCurrentlyPlaying", function(req) {
		var is_new = false;
		
		var nowplaying = $("xbmc-nowplaying");
		if (!nowplaying) {
			is_new = true;
			nowplaying  = document.createElement("div");
			nowplaying.setAttribute("id", "xbmc-nowplaying");
			nowplaying.setAttribute("title", "Now Playing");
			nowplaying.setAttribute("class", "panel");
		} else {
			while (nowplaying.hasChildNodes())
				nowplaying.removeChild(nowplaying.firstChild);
		}
		
		var np_title, np_artist, np_artwork;
		var np_fieldset = document.createElement("fieldset");
		
		var lines = req.responseText.split("\n");
		for (var i = 0; i < lines.length; i++) {
			var fields = new Array(2);//lines[i].split(":", 2);
			fields[0] = lines[i].substring(0, lines[i].indexOf(":"));
			fields[1] = lines[i].substring(lines[i].indexOf(":")+1);
			if (lines[i] != "") {
				if (fields[0] != "Thumb" && fields[0] != "SongNo" && fields[0] != "Percentage") {
					var tmpDiv = document.createElement("div");
					tmpDiv.setAttribute("class", "row");
					np_fieldset.appendChild(tmpDiv);
					
					var label = fields[0];
					var content = fields[1];
					
					switch (fields[0]) {
					case "Bitrate":
						label = "Bit rate"
						content = fields[1] + " Kbps";
						break;
					case "Samplerate":
						label = "Sample rate"
						content = fields[1] + " KHz";
						break;
					case "PlayStatus":
						label = "Status";
						break;
					case "File size":
						content = formatSize(fields[1]);
						break;
					}

					var tmpLabel = document.createElement("label");
					tmpLabel.appendChild(document.createTextNode(label));
					tmpDiv.appendChild(tmpLabel);
					
					var tmpInput = document.createElement("input");
					tmpInput.setAttribute("type", "text");
					tmpInput.setAttribute("name", fields[0]);
					tmpInput.setAttribute("value", content);
					tmpInput.setAttribute("readonly", "readonly");
					tmpDiv.appendChild(tmpInput);
				}
				
				if (fields[0] == "Title") {
					np_title = document.createElement("h2");
					np_title.appendChild(document.createTextNode(fields[1]))
				}
				else if (fields[0] == "Artist") {
					np_artist = document.createElement("h3");
					np_artist.appendChild(document.createTextNode(fields[1]))
				}
				else if (fields[0] == "Thumb") {
					np_artwork = document.createElement("img");
					if (fields[1] == "defaultAlbumCover.png") {
						np_artwork.setAttribute("src", "images/noartplaceholder.png");
					} else if (fields[1] == "defaultVideoCover.png") {
						np_artwork.setAttribute("src", "images/noartplaceholder-videos.png");
					} else {
						var src = fields[1];
						var filename = src.split("\\").pop();
						var dst = "Q:\\web\\iphone\\thumbs\\"+filename;
						xbmcHttpSimple("FileCopy("+src+";"+dst+")", function(req) {
							np_artwork.setAttribute("src", "thumbs/"+filename);
						});
					}
				}
			}
		}
		
		if (np_title)
			nowplaying.appendChild(np_title);
		if (np_artist)
			nowplaying.appendChild(np_artist);
		if (np_artwork)
			nowplaying.appendChild(np_artwork);
		nowplaying.appendChild(np_fieldset);
		
		if (is_new)
			iui.insertPage(nowplaying);
		else 
			iui.showPage(nowplaying);
	});
}