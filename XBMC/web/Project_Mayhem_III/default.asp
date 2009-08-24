<%
    if (isset("Action")) {
        if (Action == "UnqueMusicplaylist") {
            var ItemCount;
            var z;
            ItemCount = xbmcCommand("catalog","items");
            for (z = 0;  z < ItemCount;  z = z + 1) {
                // dirty workaround, cause unque, + z won't work
                xbmcCommand("navigate", Action);
                xbmcCommand("catalog","unque");
            }
            Action = "musicplaylist";          
        }
        xbmcCommand("navigate", Action);
    }
    
    if (isset("command")) {
        // execute a few commands before listing the contents
        if (command == "select") {
            xbmcCommand("catalog", "select," + item);
        } else if (command == "unque") {
            xbmcCommand("catalog", "unque," + item);
        }
    }

    var navigatorstate;
    var BannerClass;
    var InfoClass;
    var BackgroundClass;
    var ItemFileClass;
    navigatorstate = xbmcCommand("navigatorstate");

    if (isset("DisplayConfiguration")) {
        BannerClass = "SiteBannerConfiguration";
        InfoClass = "SiteInfoConfiguration";
        BackgroundClass = "SiteBackgroundConfiguration";
	} else if (isset("DisplayInfo")) {
        BannerClass = "SiteBannerConfiguration";
        InfoClass = "SiteInfoConfiguration";
        BackgroundClass = "SiteBackgroundConfiguration";
    } else if (navigatorstate == "pictures") {
        BannerClass = "SiteBannerMyPictures";
        InfoClass = "SiteInfoMyPictures";
        BackgroundClass = "SiteBackgroundMyPictures";
        ItemFileClass = "ItemFileMyPictures";
    } else if (navigatorstate == "music") {
        BannerClass = "SiteBannerMyMusic";
        InfoClass = "SiteInfoMyMusic";
        BackgroundClass = "SiteBackgroundMyMusic";
        ItemFileClass = "ItemFileMyMusic";
    } else if (navigatorstate == "videos") {
        BannerClass = "SiteBannerMyVideos";
        InfoClass = "SiteInfoMyVideos";
        BackgroundClass = "SiteBackgroundMyVideos";
        ItemFileClass = "ItemFileMyVideos";
    } else if (navigatorstate == "musicplaylist") {
        BannerClass = "SiteBannerMusicPlaylist";
        InfoClass = "SiteInfoMusicPlaylist";
        BackgroundClass = "SiteBackgroundMusicPlaylist";
        ItemFileClass = "ItemFileMusicPlaylist";
    } else if (navigatorstate == "videoplaylist") {
        BannerClass = "SiteBannerVideoPlaylist";
        InfoClass = "SiteInfoVideoPlaylist";
        BackgroundClass = "SiteBackgroundVideoPlaylist";
        ItemFileClass = "ItemFileVideoPlaylist";
    } else {
        BannerClass = "SiteBannerNone";
        InfoClass = "SiteInfoLogo";
        BackgroundClass = "SiteBackgroundHome";
    }

%><?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
     "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
    <head>
        <title> XBMC - Remote Control </title>
        <link href="styles/Project%20Mayhem/screen.css" rel="stylesheet" type="text/css" />
        <link rel="shortcut icon" href="/styles/Project%20Mayhem/images/ShortcutIcon.ico" type="image/x-icon" />        

        <meta http-equiv="content-type" content="text/html; charset=utf-8" />
    </head>
  
    <body>
    
        <div id="SiteContainer" class="<% write(BackgroundClass); %>">

            <div id="SiteLeftContainer">
                <div id="SiteInfoContainer">
                    <div id="<% write(InfoClass); %>"></div>
                </div>
    
                <div id="SiteMenuContainer">
                    <ul>
                        <li class="MyPictures"><a href="default.asp?Action=pictures"><span>My Pictures</span></a></li>
                        <li class="MyMusic"><a href="default.asp?Action=music"><span>My Music</span></a></li>
                        <li class="MyVideos"><a href="default.asp?Action=videos"><span>My Videos</span></a></li>
                        <li class="MusicPlaylist"><a href="default.asp?Action=musicplaylist"><span>Music Playlist</span></a></li>
                        <li class="VideoPlaylist"><a href="default.asp?Action=videoplaylist"><span>Video Playlist</span></a></li>
                    </ul>
                </div>
      
                <div id="SiteControlContainer">
                    <ul>
                        <li class="Stop"><a href="/xbmcCmds/xbmcForm?command=stop" target="CommandFrame"><span>Stop</span></a></li>
                        <li class="Play"><a href="/xbmcCmds/xbmcForm?command=play" target="CommandFrame"><span>Play</span></a></li>
                        <li class="Pause"><a href="/xbmcCmds/xbmcForm?command=pause" target="CommandFrame"><span>Pause</span></a></li>
                        <li class="Previous"><a href="/xbmcCmds/xbmcForm?command=previous" target="CommandFrame"><span>Previous</span></a></li>
                        <li class="Next"><a href="/xbmcCmds/xbmcForm?command=next" target="CommandFrame"><span>Next</span></a></li>
                        <li class="SkipForward"><a href="/xbmcCmds/xbmcHttp?command=SeekPercentageRelative(5)" target="CommandFrame"><span>Skip Forward</span></a></li>
                        <li class="SkipBackward"><a href="/xbmcCmds/xbmcHttp?command=SeekPercentageRelative(-5)" target="CommandFrame"><span>Skip Backward</span></a></li>
                    </ul>
                </div>
            
                <div id="SiteSubMenuContainer">
                    <ul>
                        <li class="Subpanel"><a href=""><span>Subpanel</span></a>
                            <ul>
                                <li><a href="SubRemote"><span>Remote</span></a></li>
                                <li><a href="SubConfiguration"><span>Configuration</span></a></li>
                                <li><a href="/xbmcCmds/xbmcForm?command=exit" target="CommandFrame"><span>Dashboard</span></a></li>
                                <li><a href="/xbmcCmds/xbmcForm?command=reboot" target="CommandFrame"><span>Reboot</span></a></li>
                            </ul>
                        </li>
<!--
						<li class="Info"><a href="default.asp?DisplayInfo=true"><span>Info</span></a></li>                  
-->
						<li class="Configuration"><a href="default.asp?DisplayConfiguration=true&amp;page=bookmarks"><span>Sources</span></a></li>
						<li class="Reboot"><a href="/xbmcCmds/xbmcForm?command=restart" target="CommandFrame"><span>Reboot</span></a></li>
						<li class="Shutdown"><a href="/xbmcCmds/xbmcForm?command=shutdown" target="CommandFrame"><span>Shutdown</span></a></li>
					</ul>
				</div>

  	<div id="SiteVolumeContainer">
		<ul>
		        <li class="vol0"><a href="/xbmcCmds/xbmcHttp?command=setvolume&parameter=0" target="CommandFrame"></a></li>
		        <li class="vol10"><a href="/xbmcCmds/xbmcHttp?command=setvolume&parameter=10" target="CommandFrame"></a></li>
		        <li class="vol20"><a href="/xbmcCmds/xbmcHttp?command=setvolume&parameter=20" target="CommandFrame"></a></li>
  	        <li class="vol30"><a href="/xbmcCmds/xbmcHttp?command=setvolume&parameter=30" target="CommandFrame"></a></li>
		        <li class="vol40"><a href="/xbmcCmds/xbmcHttp?command=setvolume&parameter=40" target="CommandFrame"></a></li>
  	        <li class="vol50"><a href="/xbmcCmds/xbmcHttp?command=setvolume&parameter=50" target="CommandFrame"></a></li>
		        <li class="vol60"><a href="/xbmcCmds/xbmcHttp?command=setvolume&parameter=60" target="CommandFrame"></a></li>
  	        <li class="vol70"><a href="/xbmcCmds/xbmcHttp?command=setvolume&parameter=70" target="CommandFrame"></a></li>
		        <li class="vol80"><a href="/xbmcCmds/xbmcHttp?command=setvolume&parameter=80" target="CommandFrame"></a></li>
	 	        <li class="vol90"><a href="/xbmcCmds/xbmcHttp?command=setvolume&parameter=90" target="CommandFrame"></a></li>
		        <li class="vol100"><a href="/xbmcCmds/xbmcHttp?command=setvolume&parameter=100" target="CommandFrame"></a></li>
		</ul>
	</div>    
            </div>
            
    
            <div id="SiteRightContainer">
            
                <div id="SiteBannerContainer">
                    <div id="<% write(BannerClass); %>"></div>
                </div>
                
                <div id="SiteContentContainer">
<%

  if (isset("DisplayInfo") ) 
  {
	write("<a href='/scripts/SystemInfo.spy' target='_blank'>SystemInfo</a> \n");
	write("<a href='/scripts/MediaInfo.spy' target='_blank'>MediaInfo</a> \n");
  }
  else if (isset("DisplayConfiguration") ) {
    /*
     * xbmc configuration options
    
      xbmcCfgBookmarkSize(type)
      xbmcCfgGetBookmark(type, parameter, id)
      xbmcCfgAddBookmark(type, name, path [, position])
      xbmcCfgSaveBookmark(type, name, path, position)
      xbmcCfgRemoveBookmark(type, position)
      xbmcCfgSaveConfiguration(filename)
      xbmcCfgGetOption(name)
      xbmcCfgSetOption(name, value)
    
     */
    
    write("<a href='default.asp?DisplayConfiguration=true&amp;page=bookmarks'>Sources</a> \n");

    write("<br />\n");
    write("<br />\n");
    
    
    /* if action isset we want to save / edit or remove something */
    if (isset("action"))
    {
      if (action == "savebookmark")
      {
        if (isset("name") == "1" && isset("path") == "1" && isset("type") == "1" && isset("position") == "1")
        {
          xbmcCfgSaveBookmark(type, name, path, position);
        }
        else
          write("Error");
      }
      if (action == "addbookmark")
      {
        if (isset("name") == "1" && isset("path") == "1" && isset("type") == "1")
        {
          xbmcCfgAddBookmark(type, name, path);
        }
        else
          write("Error");
      }
      if (action == "remove")
      {
        if (isset("page"))
        {
          // return to bookmark page and not to editbookmarks
          page = "bookmarks";
          xbmcCfgRemoveBookmark(type, position);
        }
      }
      if (action == "saveoptions")
      {
        xbmcCfgSetOption("home", home);
        xbmcCfgSetOption("CDDBIpAdres", CDDBIpAdres);
    
        var usef = "no";
        if (isset("useFDrive")) usef = "yes";
        xbmcCfgSetOption("useFDrive", usef);
    
        var useg = "no";
        if (isset("useGDrive")) useg = "yes";
        xbmcCfgSetOption("useGDrive", useg);
    
        xbmcCfgSetOption("dashboard", dashboard);
        xbmcCfgSetOption("dvdplayer", dvdplayer);
        xbmcCfgSetOption("subtitles", subtitles);
        xbmcCfgSetOption("startwindow", startwindow);
        xbmcCfgSetOption("pictureextensions", pictureextensions);
        xbmcCfgSetOption("musicextensions", musicextensions);
        xbmcCfgSetOption("videoextensions", videoextensions);
        xbmcCfgSetOption("thumbnails", thumbnails);
        xbmcCfgSetOption("shortcuts", shortcuts);
        xbmcCfgSetOption("albums", albums);
        xbmcCfgSetOption("recordings", recordings);
        xbmcCfgSetOption("screenshots", screenshots);
    
        var remcodes = "no";
        if (isset("displayremotecodes")) remcodes = "yes";
        xbmcCfgSetOption("displayremotecodes", remcodes);
      }
      if (action == "save")
      {
        xbmcCfgSaveConfiguration("XBoxMediaCenter.xml");
      }
    }
    
    if (isset("page"))
    {
      var i;
      var name;
      var value;
      var options;
    
      if (page == "bookmarks")
      {
        var musicbookmarks = xbmcCfgBookmarkSize("music");
        var picturebookmarks = xbmcCfgBookmarkSize("pictures");
        var videobookmarks = xbmcCfgBookmarkSize("video");
        var filebookmarks = xbmcCfgBookmarkSize("files");
        var programbookmarks = xbmcCfgBookmarkSize("myprograms");
        
    
        /* Add new Bookmark button */
        write("<form name='new_bookmark' method='post' action='default.asp?DisplayConfiguration=true&amp;page=addbookmark'>\n");
        write("  <input type='submit' name='addnewbookmark' value='Add new source'><br>\n");
        write("</form>\n");
    
        /* Display Music Bookmarks */
        write("<form name='music_bookmarks' method='post' action='default.asp?DisplayConfiguration=true&amp;page=editbookmark&amp;type=music'>\n");
        write("<br/>Music Sources:<br>\n");
        write("  <input type='submit' name='action' value='edit'>\n");
        write("  <input type='submit' name='action' value='remove'>\n");
        write("  <select name='position'>\n");
        i = 0;
        for (i=1; i<=musicbookmarks; i=i+1)
        {
          write("    <option value=" + i + ">" + xbmcCfgGetBookmark("music", "name", i) + "</option>\n");
        }
        write("  </select>\n");
        write("</form>\n");
    
        /* Display Picture Bookmarks */
        write("<form name='picture_bookmarks' method='post' action='default.asp?DisplayConfiguration=true&amp;page=editbookmark&amp;type=pictures'>\n");
        write("<br/>Picture Sources:<br>\n");
        write("  <input type='submit' name='action' value='edit'>\n");
        write("  <input type='submit' name='action' value='remove'>\n");
        write("  <select name='position'>\n");
        i = 0;
        for (i=1; i<=picturebookmarks; i=i+1)
        {
          write("    <option value=" + i + ">" + xbmcCfgGetBookmark("pictures", "name", i) + "</option>\n");
        }
        write("  </select>\n");
        write("</form>\n");
    
        /* Display Video Bookmarks */
        write("<form name='video_bookmarks' method='post' action='default.asp?DisplayConfiguration=true&amp;page=editbookmark&amp;type=video'>\n");
        write("<br/>Video Sources:<br>\n");
        write("  <input type='submit' name='action' value='edit'>\n");
        write("  <input type='submit' name='action' value='remove'>\n");
        write("  <select name='position'>\n");
        i = 0;
        for (i=1; i<=videobookmarks; i=i+1)
        {
          write("    <option value=" + i + ">" + xbmcCfgGetBookmark("video", "name", i) + "</option>\n");
        }
        write("  </select>\n");
        write("</form>\n");    
      }
      else if (page == "options")
      {
        write("	<form action='default.asp?DisplayConfiguration=true' method='post' name='cfgform' id='cfgform'>" + \
              "		<input name='action' type='hidden' value='saveoptions'>" + \
              "		<input name='page' type='hidden' value='options'>" + \
              "		<table width='500'>");
        write("			<tr>" + \
              "				<td width='200'><label>Home</label></td>" + \
              "				<td><input name='home' type='text' value='"); write(xbmcCfgGetOption("home")); write("' size='25'><br></td>" + \
              "			</tr>");
        write("			<tr>" + \
              "				<td><label>CDDB IP Address</label></td><td>" + \
              "				<input name='CDDBIpAdres' type='text' value='"); write(xbmcCfgGetOption("CDDBIpAdres")); write("' size='25'><br></td>" + \
              "			</tr>");
    
        // Use F Drive
        write("			<tr>" + \
              "					<td><label>Use F Drive</label></td><td>" + \
              "           <input name='useFDrive' type='checkbox' value='true' ");
              if (xbmcCfgGetOption("useFDrive") == "yes")	write("checked>");
              else write(">");
        write("         <br></td>" + \
              "			</tr>");
    
        // Use G Drive
        write("			<tr>" + \
              "					<td><label>Use G Drive</label></td><td>" + \
              "           <input name='useGDrive' type='checkbox' value='true' ");
              if (xbmcCfgGetOption("useGDrive") == "yes")	write("checked>");
              else write(">");
        write("         <br></td>" + \
              "			</tr>");
        write("			<tr>" + \
              "				<td><label>Dashboard</label></td>" + \
              "				<td><input name='dashboard' type='text' value='"); write(xbmcCfgGetOption("dashboard")); write("' size='25'><br></td>" + \
              "			</tr>");
        write("			<tr>" + \
              "				<td><label>Dvdplayer</label></td>" + \
              "				<td><input name='dvdplayer' type='text' value='"); write(xbmcCfgGetOption("dvdplayer")); write("' size='25'><br></td>" + \
              "			</tr>");
        write("			<tr>" + \
              "				<td><label>Subtitles</label></td>" + \
              "				<td><input name='subtitles' type='text' value='"); write(xbmcCfgGetOption("subtitles")); write("' size='25'><br></td>" + \
              "			</tr>");
        write("			<tr>" + \
              "				<td><label>Startwindow</label></td>" + \
              "				<td><input name='startwindow' type='text' value='"); write(xbmcCfgGetOption("startwindow")); write("' size='25'><br></td>" + \
              "			</tr>");
        write("			<tr>" + \
              "				<td><label>Picture Extensions</label></td>" + \
              "				<td><input name='pictureextensions' type='text' value='"); write(xbmcCfgGetOption("pictureextensions")); write("' size='25'><br></td>" + \
              "			</tr>");
        write("			<tr>" + \
              "				<td><label>Music Extensions</label></td>" + \
              "				<td><input name='musicextensions' type='text' value='"); write(xbmcCfgGetOption("musicextensions")); write("' size='25'><br></td>" + \
              "			</tr>");
        write("			<tr>" + \
              "				<td><label>Video Extensions</label></td>" + \
              "				<td><input name='videoextensions' type='text' value='"); write(xbmcCfgGetOption("videoextensions")); write("' size='25'><br></td>" + \
              "			</tr>");
        write("			<tr>" + \
              "				<td><label>Thumbnails</label></td>" + \
              "				<td><input name='thumbnails' type='text' value='"); write(xbmcCfgGetOption("thumbnails")); write("' size='25'><br></td>" + \
              "			</tr>");
        write("			<tr>" + \
              "				<td><label>Shortcuts</label></td>" + \
              "				<td><input name='shortcuts' type='text' value='"); write(xbmcCfgGetOption("shortcuts")); write("' size='25'><br></td>" + \
              "			</tr>" );
        write("			<tr>" + \
              "				<td><label>Albums</label></td>" + \
              "				<td><input name='albums' type='text' value='"); write(xbmcCfgGetOption("albums")); write("' size='25'><br></td>" + \
              "			</tr>");
        write("			<tr>" + \
              "				<td><label>Recordings</label></td>" + \
              "				<td><input name='recordings' type='text' value='"); write(xbmcCfgGetOption("recordings")); write("' size='25'><br></td>" + \
              "			</tr>");
        write("			<tr>" + \
              "				<td><label>Screenshots</label></td>" + \
              "				<td><input name='screenshots' type='text' value='"); write(xbmcCfgGetOption("screenshots")); write("' size='25'><br></td>" + \
              "			</tr>");
    
        // Display remote codes
        write("			<tr>" + \
              "					<td><label>Display remote codes</label></td><td>" + \
              "           <input name='displayremotecodes' type='checkbox' value='true' ");
              if (xbmcCfgGetOption("displayremotecodes") == "yes")	write("checked>");
              else write(">");
        write("         <br></td>" + \
              "			</tr>");
    
        write("		</table><br>" + \
              "		<input type='submit' name='save' value='save'>" + \
              "	</form><br>");
      }
      else if (page == "editbookmark")
      {
        if (isset("type") == "1" && isset("position") == "1")
        {
          write("<form name='savebookmark' method='post' action='default.asp?DisplayConfiguration=true&amp;page=bookmarks&amp;action=savebookmark'>\n");
          write("<input name='position' type='hidden' value='" + position + "'>\n");
          write("<input name='type' type='hidden' value='" + type + "'>\n");
          write("  <table width='500' border='0'>\n");
          write("    <tr> \n");
          write("      <td><label>name</label></td>\n");
          write("      <td><input type='text' name='name' value='" + xbmcCfgGetBookmark(type, "name", position) + "'></td>\n");
          write("    </tr>\n");
          write("    <tr> \n");
          write("      <td><label>path</label></td>\n");
          write("      <td><input type='text' name='path' value='" + xbmcCfgGetBookmark(type, "path", position) + "'></td>\n");
          write("    </tr>\n");
          write("  </table><br>\n");
          write("  <input type='submit' name='save' value='save'>\n");
          write("</form>\n");
        }
      }
      else if (page == "addbookmark")
      {
        var data = "<form name='addbookmark' method='post' action='default.asp?DisplayConfiguration=true&amp;page=bookmarks&amp;action=addbookmark'>\n";
        data = data + "  <table width='500' border='0'>\n";
        data = data + "    <tr> \n";
        data = data + "      <td><label>Type</label></td>\n";
        data = data + "      <td>";
        data = data + "        <select name='type'>\n";
        data = data + "          <option value=files>File</option>\n";
        data = data + "          <option value=music>Music</option>\n";
        data = data + "          <option value=myprograms>Program</option>\n";
        data = data + "          <option value=pictures>Picture</option>\n";
        data = data + "          <option value=video>Video</option>\n";
        data = data + "        </select>\n";
        data = data + "      </td>\n";
        data = data + "    </tr>\n";
        data = data + "    <tr> \n";
        data = data + "      <td><label>Name</label></td>\n";
        data = data + "      <td><input type='text' name='name' value=''></td>\n";
        data = data + "    </tr>\n";
        data = data + "    <tr> \n";
        data = data + "      <td><label>Path</label></td>\n";
        data = data + "      <td><input type='text' name='path' value=''></td>\n";
        data = data + "    </tr>\n";
        data = data + "  </table><br>\n";
        data = data + "  <input type='submit' name='save' value='save'>\n";
        data = data + "</form>\n";
    
        write(data);
      }
      if (page == "load_save")
      {
        write("<br><br><a href='default.asp?DisplayConfiguration=true&amp;action=save'>Save</a>\n");
      }
    }


  } else {
 
  
    var z;
    var n;
    var i;
    var data;
  
  
    n = xbmcCommand("catalog","items"); // number of items to list
    file = xbmcCommand("catalog","first");
    
    if ( n > 0) {
        write("                    <ul id=\"ItemList\">\n");
    }
    
    for (z=0; z<n; z=z+1)
    {
      var type = xbmcCommand("catalog","type," + z);
  
  
      data = "                        <li>";
  
      if (type != "directory") {
        // we have a file to play, send command to server when clicked and do not refresh the current page
        data = data + "<div class=\"" + ItemFileClass + "\"></div>";
      } else { 
        data = data + "<div class=\"ItemFolder\"></div>"; 
      }
  
  
      if (file != ".." && navigatorstate != "musicplaylist" && navigatorstate != "videoplaylist" && navigatorstate != "pictures") {
        data = data + "<div class=\"AddQueue\"><a href=\"/xbmcCmds/xbmcForm?command=catalog&amp;parameter=que," + z + "\" target=\"CommandFrame\" title=\"add to queue\"><span>Q</span></a></div>";
      } else if (navigatorstate == "musicplaylist" || navigatorstate == "videoplaylist")	{
        data = data + "<div class=\"RemoveQueue\"><a href=\"default.asp?command=unque&amp;item=" + z + "\" title=\"remove from queue\"><span>R</span></a></div>";
      }
  
  
      if (type != "directory") {
        // we have a file to play, send command to server when clicked and do not refresh the current page
        data = data + "<div class=\"FileName\"><a href=\"/xbmcCmds/xbmcForm?command=catalog&amp;parameter=select," + z + "\" target=\"CommandFrame\">";
      } else { 
        data = data + "<div class=\"FileName\"><a href=\"default.asp?command=select&amp;item=" + z + "\">"; 
      }
      
      data = data + file + "</a></div>";
      
      data = data + "</li>\n";
      write(data);
      
        
      file = xbmcCommand("catalog","next");
    }

    if ( n > 0) {
        write("                    </ul>\n");
    }
    
    if (navigatorstate == "musicplaylist") {
        write ("<a href=\"default.asp?Action=UnqueMusicplaylist\" id=\"UnqueMusciplaylist\"><span>Clear Playlist</span></a>\n");
    }


}
%>
                </div>
    
            </div>
            
        </div>
        
        <div id="SiteCommandContainer">
            <iframe src="about:blank" width="1" height="1" name="CommandFrame"></iframe>
        </div>
  
    </body>
</html>
