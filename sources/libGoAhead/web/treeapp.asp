<html>
<!- Copyright (c) Go Ahead Software Inc., 1994-2000. All Rights Reserved. ->

<head>
<TITLE>GoAhead Embedded Management Framework Technical Reference</TITLE>
<BASE TARGET="view">
<% language=javascript %> 
<STYLE TYPE="TEXT/CSS">
<!--
body {  background-color: #FFFFFF; margin-top: 0px; margin-right: 0px; margin-bottom: 0px; margin-left: 0px}
-->
</STYLE></head>

<body background="graphics/sidebar.gif">

<APPLET align=left code=treeApp.class codebase="/classes"
	height=100% id=TreeObject width=180 valign=top archive=treeapp.jar VSPACE="0" HSPACE="0">
  <PARAM NAME="authorization" VALUE="<% write(HTTP_AUTHORIZATION); %>">
  <PARAM NAME="fontcolor" VALUE="FFFFFF">
  <PARAM NAME="linecolor" VALUE="FFFFFF">
  <PARAM NAME="backgroundcolor" VALUE="000000">
  <PARAM NAME="datascript" VALUE="contents.asp">
  <PARAM NAME="port" VALUE="<% write(SERVER_PORT); %>">
  <PARAM NAME="hilightfontcolor" VALUE="FFF000">
  <PARAM NAME="system" VALUE="<% write(SERVER_ADDR); %>">
  <PARAM NAME="font" VALUE="Ariel,Helvetica bold 10">
</APPLET> 
</BODY>
</html>
