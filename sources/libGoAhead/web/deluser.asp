<html>
<!- Copyright (c) Go Ahead Software Inc., 2000-2000. All Rights Reserved. ->
<head>
<title>Delete a User</title>
<meta http-equiv="Pragma" content="no-cache">
<link rel="stylesheet" href="style/normal_ws.css" type="text/css">
<% language=javascript %>
</head>

<body>
<h1>Delete a User</h1>
<form action=/goform/DeleteUser method=POST>

<table>
<tr>
<% MakeUserList(); %>
</tr>
<tr>
    <td></td>
      <td ALIGN="CENTER"> 
        <input type=submit name=ok value="OK" title="Delete the User"> <input type=submit name=ok value="Cancel"></td>
</tr>
</table>

</form>

</body>
</html>
