<html>
<!- Copyright (c) Go Ahead Software Inc., 1999-2000. All Rights Reserved. ->
<head>
<title>ASP Test Page</title>
<link rel="stylesheet" href="style/normal_ws.css" type="text/css">
<% language=javascript %>
</head>

<body>

<h1>ASP / JavaScript&#153; Test</h1>
<h2>Expanded ASP data: <% aspTest("Peter Smith", "112 Merry Way"); %></h2>

<P>
<% var z; \
   for (z=0; z<5; z=z+1) \
     { \
     if (z<=2) \
		write(z+" is less than 3<br>"); \
     else if (z==3) \
		write(z+" is equal to 3<br>"); \
     else \
		write(z+" is greater than 3<br>"); \
     } \
%>
</P>


</body>
</html>
