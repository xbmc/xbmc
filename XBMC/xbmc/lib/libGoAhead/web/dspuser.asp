<html>
<!- Copyright (c) Go Ahead Software Inc., 2000-2000. All Rights Reserved. ->
<head>
<title>Display a User</title>
<meta http-equiv="Pragma" content="no-cache">
<link rel="stylesheet" href="style/normal_ws.css" type="text/css">
<% language=javascript %>
</head>

<body>
<h1>Display a User</h1>
<form action=/goform/DisplayUser method=POST>

<table>
<tr>
<% MakeUserList(); %>
</tr>
<tr>
    <td></td>
      <td ALIGN="CENTER"> 
        <input type=submit name=ok value="OK" title="Display the User"> <input type=submit name=ok value="Cancel"></td>
</tr>
</table>

</form>

</body>
</html>
