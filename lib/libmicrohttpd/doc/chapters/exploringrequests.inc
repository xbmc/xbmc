This chapter will deal with the information which the client sends to the
server at every request. We are going to examine the most useful fields of such an request
and print them out in a readable manner. This could be useful for logging facilities.

The starting point is the @emph{hellobrowser} program with the former response removed.

This time, we just want to collect information in the callback function, thus we will
just return MHD_NO after we have probed the request. This way, the connection is closed
without much ado by the server.

@verbatim
int answer_to_connection (void *cls, struct MHD_Connection *connection, const char *url, 
                         const char *method, const char *version, const char *upload_data, 
                         size_t *upload_data_size, void **con_cls)
{
  ...  
  return MHD_NO;
}
@end verbatim
@noindent
The ellipsis marks the position where the following instructions shall be inserted.


We begin with the most obvious information available to the server, the request line. You should
already have noted that a request consists of a command (or "method") and a URI (e.g. a filename).
It also contains a string for the version of the protocol which can be found in @code{version}.
To call it a "new request" is justified because we return only @code{MHD_NO}, thus ensuring the 
function will not be called again for this connection. 
@verbatim
printf ("New request %s for %s using version %s\n", method, url, version);
@end verbatim
@noindent

The rest of the information is a bit more hidden. Nevertheless, there is lot of it sent from common
Internet browsers. It is stored in "key-name" pairs and we want to list what we find in the header. 
As there is no mandatory set of keys a client has to send, each key--name pair is printed out one by
one until there are no more left. We do this by writing a separate function which will be called for
each pair just like the above function is called for each HTTP request. 
It can then print out the content of this pair.
@verbatim
int print_out_key (void *cls, enum MHD_ValueKind kind, const char *key, const char *value)
{
  printf ("%s = %s\n", key, value);
  return MHD_YES;
}
@end verbatim
@noindent

To start the iteration process that calls our new function for every key, the line
@verbatim
MHD_get_connection_values (connection, MHD_HEADER_KIND, print_out_key, NULL);
@end verbatim
@noindent
needs to be inserted in the connection callback function too. The second parameter tells the function 
that we are only interested in keys from the general HTTP header of the request. Our iterating
function @code{PrintOutKey} does not rely on any additional information to fulfill its duties
so the last parameter can be NULL.

All in all, this constitutes the complete @code{logger.c} program for this chapter which can be 
found in the @code{examples} section.

Connecting with any modern Internet browser should yield a handful of keys. You should try to 
interpret them with the aid of @emph{RFC 2616}.
Especially worth mentioning is the host key which is often used to serve several different websites
hosted under one single IP address but reachable by different domain names.

@heading Conclusion
The introduced capabilities to itemize the content of a simple GET request---especially the
URI---should already allow the server to satisfy clients' requests for small specific resources
(e.g. files) or even induce alteration of how the server operates. However, the latter is not
recommended as the GET method (including its header data) is by convention considered a "SAFE"
operation, which should not change the server's state in a significant way, but temporally actions
like searching for a passed string is fine.

Of course, no transmission can occur while the return value is still set to @code{MHD_NO} in the
callback function.

@heading Exercises
@itemize @bullet
@item
By parsing the @code{url} string and delivering responses accordingly, implement a small server for
"virtual" files. When asked for @code{/index.htm@{l@}}, let the response consist of a HTML page
containing a link to @code{/another.html} page which is also to be created "on the fly" in case of
being requested. If neither of these two pages are requested, @code{MHD_HTTP_NOT_FOUND} shall be
returned accompanied by an informative message.

@item
A very interesting information has still been ignored by our logger---the client's IP address. 
Implement a callback function 
@verbatim
int on_client_connect (void *cls,
                       const struct sockaddr *addr,socklen_t addrlen)
@end verbatim
@noindent
that prints out the IP address in an appropriate format. You might want to use the posix function
@code{inet_ntoa} but bear in mind that @code{addr} is actually just a structure containing other
substructures and is @emph{not} the variable this function expects. 
Make sure to return @code{MHD_YES} so that the library knows the client is allowed to connect
(and to request). If one wanted to limit access basing on IP addresses, this would be the place
to do it. The address of your function will then be passed as the third parameter of the
@code{MHD_start_daemon} call.

@end itemize
