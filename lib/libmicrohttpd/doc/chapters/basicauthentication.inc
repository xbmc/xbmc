With the small exception of IP address based access control, 
requests from all connecting clients where served equally until now.
This chapter discusses a first method of client's authentication and
its limits. 

A very simple approach feasible with the means already discussed would
be to expect the password in the @emph{URI} string before granting access to
the secured areas. The password could be separated from the actual resource identifier
by a certain character, thus the request line might look like
@verbatim
GET /picture.png?mypassword
@end verbatim
@noindent

In a situation where the client is customized enough and the connection occurs
through secured lines (e.g., a embedded device directly attached to another via wire),
this can be a reasonable choice.

But when it is assumed that the user connecting does so with an ordinary Internet browser,
this implementation brings some problems about. For example, the URI including the password
stays in the address field or at least in the history of the browser for anybody near enough to see. 
It will also be inconvenient to add the password manually to any new URI when the browser does
not know how to compose this automatically.

At least the convenience issue can be addressed by employing the simplest built-in password
facilities of HTTP compliant browsers, hence we want to start there. It will however turn out
to have still severe weaknesses in terms of security which need consideration.

Before we will start implementing @emph{Basic Authentication} as described in @emph{RFC 2617},
we should finally abandon the bad practice of responding every request the first time our callback
is called for a given connection. This is becoming more important now because the client and 
the server will have to talk in a more bi-directional way than before to 

But how can we tell whether the callback has been called before for the particular connection?
Initially, the pointer this parameter references is set by @emph{MHD} in the callback. But it will 
also be "remembered" on the next call (for the same connection).
Thus, we will generate no response until the parameter is non-null---implying the callback was
called before at least once. We do not need to share information between different calls of the callback,
so we can set the parameter to any adress that is assured to be not null. The pointer to the 
@code{connection} structure will be pointing to a legal adress, so we take this.

Not even the headers will be looked at on the first iteration.

@verbatim
int answer_to_connection (void *cls, struct MHD_Connection *connection,
                          const char *url, const char *method, const char *version, 
                          const char *upload_data, size_t *upload_data_size,
                          void **con_cls)
{
  if (0 != strcmp(method, "GET")) return MHD_NO;
  if (NULL == *con_cls) {*con_cls = connection; return MHD_YES;}

  ... 
  /* else respond accordingly */
  ...
}
@end verbatim
@noindent

Note how we lop off the connection on the first condition, but return asking for more on 
the other one with @code{MHD_YES}.
With the framework improved, we can proceed to implement the actual authentication process.

@heading Request for authentication 

Let us assume we had only files not intended to be handed out without the correct username/password,
so every "GET" request will be challenged.
@emph{RFC 2617} describes how the server shall ask for authentication by adding a
@emph{WWW-Authenticate} response header with the name of the @emph{realm} protected.

We let an extra function function do this.
@verbatim
int ask_for_authentication (struct MHD_Connection *connection, const char *realm)
{
  int ret;
  struct MHD_Response *response;
  char *headervalue;
  const char *strbase = "Basic realm=";
  
  response = MHD_create_response_from_data (0, NULL, MHD_NO, MHD_NO);
  if (!response) return MHD_NO;
  
  headervalue = malloc (strlen (strbase) + strlen (realm) + 1);
  if (!headervalue) return MHD_NO;  

  strcpy (headervalue, strbase);
  strcat (headervalue, realm);
  
  ret = MHD_add_response_header (response, "WWW-Authenticate", headervalue);
  free (headervalue);  
  if (!ret) {MHD_destroy_response (response); return MHD_NO;}

  ret = MHD_queue_response (connection, MHD_HTTP_UNAUTHORIZED, response);
  
  MHD_destroy_response (response);

  return ret;
}
@end verbatim
@noindent

@code{#define} the realm name according to your own taste, e.g. "Maintenance" or "Area51" but
it will need to have extra quotes.

But the client may send the authentication right away, so it would be wrong to ask for
it without checking the request's header--where the authentication is expected to be found.

@heading Authentication in detail
Checking @emph{RFC 2617} again, we find that the client will pack the username and password, by
whatever means he might have obtained them, in a line separated by a colon---and then encodes
them to @emph{Base64}. The actual implementation of this encoding are not within the scope of
this tutorial although a working function is included in the complete source file of the example.

An unencoded word describing the authentication method (here "Basic") will precede the code
and the resulting line is the value of a request header of the type "Authorization".  

This header line thus is of interest to the function checking a connection for a given username/password:
@verbatim
int is_authenticated (struct MHD_Connection *connection,
                      const char *username, const char *password)
{
  const char *headervalue;
  ...

  headervalue = MHD_lookup_connection_value (connection, MHD_HEADER_KIND,
                                             "Authorization");
  if (NULL == headervalue) return 0;
@end verbatim
@noindent

where, firstly, the authentication method will be checked.
@verbatim
const char *strbase = "Basic ";
...
if (0 != strncmp (headervalue, strbase, strlen (strbase))) return 0;
@end verbatim
@noindent

Of course, we could decode the passed credentials in the next step and compare them directly to
the given strings. But as this would involve string parsing, which is more complicated then string
composing, it is done the other way around---the clear text credentials will be encoded to @emph{Base64}
and then compared against the headerline. The authentication method string will be left out here as
it has been checked already at this point.
@verbatim
  char *expected_b64, *expected;
  int authenticated;

  ...
  strcpy (expected, username);
  strcat (expected, ":");
  strcat (expected, password);  

  expected_b64 = string_to_base64 (expected);
  if (NULL == expected_b64) return 0;
 
  strcpy (expected, strbase);
  authenticated = (strcmp (headervalue + strlen (strbase), expected_b64) == 0);

  free (expected_b64);

  return authenticated;
}
@end verbatim
@noindent

These two functions---together with a response function in case of positive authentication doing little 
new---allow the rest of the callback function to be rather short. 
@verbatim
  if (!is_authenticated (connection, USER, PASSWORD)) 
    return ask_for_authentication (connection, REALM); 
  
  return secret_page (connection);
}
@end verbatim
@noindent

See the @code{examples} directory for the complete example file.

@heading Remarks
For a proper server, the conditional statements leading to a return of @code{MHD_NO} should yield a 
response with a more precise status code instead of silently closing the connection. For example,
failures of memory allocation are best reported as @emph{internal server error} and unexpected 
authentication methods as @emph{400 bad request}.

@heading Exercises
@itemize @bullet
@item
Make the server respond to wrong credentials (but else correct requests) with the recommended
@emph{401 unauthorized} status code. If the client still does not authenticate correctly within the
same connection, close it and store the client's IP address for a certain time. (It is OK to check for
expiration not until the main thread wakes up again on the next connection.) If the client fails
authenticating three times during this period, add it to another list whose entries the 
@code{AcceptPolicyCallback} function denies connection (temporally).

@item
With the network utility @emph{netcat} connect and log the response of a "GET" request as you
did in the exercise of the first example, this time to a file. Now stop the server and let @emph{netcat}
listen on the same port the server used to listen on and have it fake being the proper server by giving
the file's content as the response (e.g. @code{cat log | nc -l -p 8888}). Pretending to think your were
connecting to the actual server, browse to the eavesdropper and give the correct credentials.

Copy and paste the encoded string you see in netcat's output to some of the Base64 decode tools available online
and see how both the user's name and password could be completely restored.

@end itemize


