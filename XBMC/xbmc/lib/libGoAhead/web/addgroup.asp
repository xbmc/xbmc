<html>
<!- Copyright (c) Go Ahead Software Inc., 2000-2000. All Rights Reserved. ->
<head>
<title>Add a User Group</title>
<meta http-equiv="Pragma" content="no-cache">
<link rel="stylesheet" href="style/normal_ws.css" type="text/css">
<% language=javascript %>
</head>

<body>
<h1>Add a User Group</h1>
<form action=/goform/AddGroup method=POST>

<table>
<tr>
	<td>Group Name:</td>
<td>
	<input type=text name=group title="Group Name" size=40 value="">
</td>
</tr>
<tr>
	<td>Privilege:</td><td><% MakePrivilegeList(); %></td>
</tr>
<tr>
	<td>Access Method:</td><td><% MakeAccessMethodList(); %></td>
</tr>
<tr>
	<td>Enabled:</td>
<td>
	<INPUT TYPE=checkbox CHECKED name=enabled title="Enabled">
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
