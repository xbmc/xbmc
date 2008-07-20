<html>
<!- Copyright (c) Go Ahead Software Inc., 2000-2000. All Rights Reserved. ->
<head>
<title>Add a User</title>
<meta http-equiv="Pragma" content="no-cache">
<link rel="stylesheet" href="style/normal_ws.css" type="text/css">
<% language=javascript %>
</head>

<body>
<h1>Add a User</h1>
<form action=/goform/AddUser method=POST>
<table>
<tr>
	<td>User ID:</td>
<td>
	<input type=text name=user title="User ID" size=40 value="">
</td>
</tr>
<tr>
	<td>Group:</td><td><% MakeGroupList(); %></td>
</tr>
<tr>
	<td>Enabled:</td>
<td>
	<INPUT TYPE=checkbox CHECKED name=enabled title="Enabled">
</td>
</tr>
<tr>
	<td>Password:</td>
<td>
	<input type=password name=password size=40 title="Enter Password" value="">
</td>
</tr>
<tr>
	<td>Confirm:</td>
<td>
	<input type=password name=passconf size=40 title="Confirm Password" value="">
</td>
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
