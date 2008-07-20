<html>
<!- Copyright (c) Go Ahead Software Inc., 2000-2000. All Rights Reserved. ->
<head>
<title>Add an Access Limit</title>
<meta http-equiv="Pragma" content="no-cache">
<link rel="stylesheet" href="style/normal_ws.css" type="text/css">
<% language=javascript %>
</head>

<body>
<h1>Add an Access Limit</h1>
<form action=/goform/AddAccessLimit method=POST>

<table>
<tr>
	<td>URL:</td><td><input type=text name=url title="URL" size=40 value=""></td>
</tr>
<tr>
	<td>Group:</td><td><% MakeGroupList(); %></td>
</tr>
<tr>
	<td>Access Method:</td><td><% MakeAccessMethodList(); %></td>
</tr>
<tr>
	<td>Secure:</td><td><INPUT TYPE=checkbox name=secure" title="Secure"></td>
</tr>
<tr>
    <td></td>
      <td ALIGN="CENTER"> 
        <input type=submit name=ok value="OK"> <input type=submit name=ok value="Cancel"></td>
</tr>
</table>

</form>

</body>
</html>
