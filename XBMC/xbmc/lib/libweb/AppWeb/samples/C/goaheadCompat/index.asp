<HTML>
<HEAD>
<TITLE>ASP Test Page</TITLE>
</HEAD>

<BODY>
<h1>ASP Test Page</h1>

<h3>Testing ASP procedure aspTest</h3>
<blockquote>
<p>
<% aspTest("Alfred Crimes", "222 Willow Road"); %>
</p>
</blockquote>

<h3>Testing GoForm procedure formTest</h3>
<form action="/goform/formTest" method="POST" name="formTest">
	Name <input type="text" name="name" size="20" maxlength="50" value="">
	Address <input type="text" name="address" size="20" maxlength="50" value="">
	<input type="submit" name="OK" size="20" maxlength="50" value="OK">
</form>

<h3>Test Error Return</h3>
<form action="/goform/formWithError" method="POST" name="formTest">
	<input type="submit" name="OK" size="20" maxlength="50" 
		value="Intentional Error">
</form>

<h3>CGI Environment Variables</h3>
<% write("REMOTE_ADDR is " + REMOTE_ADDR);%> <br/>
<% write("QUERY_STRING is " + QUERY_STRING);%> <br/>
<% write("after\n");%> <br/>
<% write("AUTH_TYPE is " + AUTH_TYPE);%> <br/>
<% write("CONTENT_LENGTH is " + CONTENT_LENGTH);%> <br/>
<% write("CONTENT_TYPE is " + CONTENT_TYPE);%> <br/>
<% write("GATEWAY_INTERFACE is " + GATEWAY_INTERFACE);%> <br/>
<% write("PATH_INFO is " + PATH_INFO);%> <br/>
<% write("PATH_TRANSLATED is " + PATH_TRANSLATED);%> <br/>
<% write("QUERY_STRING is " + QUERY_STRING);%> <br/>
<% write("QUERY_STRING is " + QUERY_STRING);%> <br/>
<% write("QUERY_STRING is " + QUERY_STRING);%> <br/>
<% write("QUERY_STRING is " + QUERY_STRING);%> <br/>
<% write("QUERY_STRING is " + QUERY_STRING);%> <br/>
<% write("REMOTE_ADDR is " + REMOTE_ADDR);%> <br/>
<% write("REMOTE_USER is " + REMOTE_USER);%> <br/>
<% write("REQUEST_METHOD is " + REQUEST_METHOD);%> <br/>
<% write("SERVER_NAME is " + SERVER_NAME);%> <br/>
<% write("SERVER_PORT is " + SERVER_PORT);%> <br/>
<% write("SERVER_PROTOCOL is " + SERVER_PROTOCOL);%> <br/>
<% write("SERVER_SOFTWARE is " + SERVER_SOFTWARE);%> <br/>

<% write("HTTP_HOST is " + HTTP_HOST);%> <br/>
<% write("HTTP_USER_AGENT is " + HTTP_USER_AGENT);%> <br/>
<% write("HTTP_CONNECTION is " + HTTP_CONNECTION);%> <br/>

</BODY>
</HTML>
