The most basic task for a HTTP server is to deliver a static text message to any client connecting to it.
Given that this is also easy to implement, it is an excellent problem to start with.

For now, the particular filename the client asks for shall have no effect on the message that will 
be returned. In addition, the server shall end the connection after the message has been sent so that
the client will know there is nothing more to expect.

The C program @code{hellobrowser.c}, which is to be found in the examples section, does just that.
If you are very eager, you can compile and start it right away but it is advisable to type the
lines in by yourself as they will be discussed and explained in detail. 

After the necessary includes and the definition of the port which our server should listen on
@verbatim
#include <platform.h>
#include <microhttpd.h>

#define PORT 8888
@end verbatim
@noindent
the desired behaviour of our server when HTTP request arrive has to be implemented. We already have
agreed that it should not care about the particular details of the request, such as who is requesting
what. The server will respond merely with the same small HTML page to every request. 

The function we are going to write now will be called by @emph{GNU libmicrohttpd} every time an
appropriate request comes in. While the name of this callback function is arbitrary, its parameter
list has to follow a certain layout. So please, ignore the lot of parameters for now, they will be
explained at the point they are needed. We have to use only one of them,
@code{struct MHD_Connection *connection}, for the minimalistic functionality we want to archive at the moment.

This parameter is set by the @emph{libmicrohttpd} daemon and holds the necessary information to
relate the call with a certain connection. Keep in mind that a server might have to satisfy hundreds
of concurrent connections and we have to make sure that the correct data is sent to the destined
client. Therefore, this variable is a means to refer to a particular connection if we ask the
daemon to sent the reply.

Talking about the reply, it is defined as a string right after the function header
@verbatim
int answer_to_connection (void *cls, struct MHD_Connection *connection, const char *url, 
                          const char *method, const char *version, const char *upload_data, 
                          size_t *upload_data_size, void **con_cls)
{
  const char *page  = "<html><body>Hello, browser!</body></html>";
@end verbatim
@noindent
HTTP is a rather strict protocol and the client would certainly consider it "inappropriate" if we
just sent the answer string "as is". Instead, it has to be wrapped in certain layers, called headers,
of additional information. Luckily, most of the work in this area is done by the library for us---we
just have to ask. Our reply string packed in the necessary layers will be called a "response".
To obtain such a response we hand our data (the reply--string) and its size over to the 
@code{MHD_create_response_from_data} function. The last two parameters basically tell @emph{MHD}
that we do not want it to dispose the message data for us when it has been sent and there also needs
no internal copy to be done because the @emph{constant} string won't change anyway.

@verbatim
  struct MHD_Response *response;
  int ret;

  response = MHD_create_response_from_data (strlen (page),
                                            (void*) page, MHD_NO, MHD_NO);
@end verbatim
@noindent
Now that the the response has been laced up, it is ready for delivery and can be queued for sending. 
This is done by passing it to another @emph{GNU libmicrohttpd} function. As all our work was done in
the scope of one function, the recipient is without doubt the one associated with the
local variable @code{connection} and consequently this variable is given to the queue function. 
Every HTTP response is accompanied by a status code, here "OK", so that the client knows 
this response is the intended result of his request and not due to some error or malfunction.

Finally, the packet is destroyed and the return value from the queue returned,
already being set at this point to either MHD_YES or MHD_NO in case of success or failure.

@verbatim
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);

  return ret;
}
@end verbatim
@noindent
With the primary task of our server implemented, we can start the actual server daemon which will listen 
on @code{PORT} for connections. This is done in the main function.
@verbatim
int main ()
{
  struct MHD_Daemon *daemon;

  daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, 
                             &answer_to_connection, NULL, MHD_OPTION_END);
  if (NULL == daemon) return 1;
@end verbatim
@noindent
The first parameter is one of three possible modes of operation. Here we want the daemon to run in
a separate thread and to manage all incoming connections in the same thread. This means that while
producing the response for one connection, the other connections will be put on hold. In this
chapter, where the reply is already known and therefore the request is served quickly, this poses no problem.

We will allow all clients to connect regardless of their name or location, therefore we do not check
them on connection and set the forth and fifth parameter to NULL.

Parameter six is the address of the function we want to be called whenever a new connection has been
established. Our @code{AnswerToConnection} knows best what the client wants and needs no additional
information (which could be passed via the next parameter) so the next parameter is NULL. Likewise,
we do not need to pass extra options to the daemon so we just write the MHD_OPTION_END as the last parameter.

As the server daemon runs in the background in its own thread, the execution flow in our main
function will contine right after the call. Because of this, we must delay the execution flow in the
main thread or else the program will terminate prematurely. We let it pause in a processing-time
friendly manner by waiting for the enter key to be pressed. In the end, we stop the daemon so it can
do its cleanup tasks.
@verbatim
  getchar (); 

  MHD_stop_daemon (daemon);
  return 0;
}

@end verbatim
@noindent
The first example is now complete.

Compile it with 
@verbatim
cc hellobrowser.c -o hellobrowser -I$PATH_TO_LIBMHD_INCLUDES 
  -L$PATH_TO_LIBMHD_INCLUDES -static -lmicrohttpd -pthread
@end verbatim
with the two paths set accordingly and run it.

Now open your favorite Internet browser and go to the address @code{localhost:8888}, provided that
is the port you chose. If everything works as expected, the browser will present the message of the
static HTML page it got from our minimal server.

@heading Remarks
To keep this first example as small as possible, some drastic shortcuts were taken and are to be
discussed now. 

Firstly, there is no distinction made between the kinds of requests a client could send. We implied
that the client sends a GET request, that means, that he actually asked for some data. Even when
it is not intended to accept POST requests, a good server should at least recognize that this
request does not constitute a legal request and answer with an error code. This can be easily
implemented by checking if the parameter @code{method} equals the string "GET" and returning a
@code{MHD_NO} if not so.

Secondly, the above practice of queuing a response upon the first call of the callback function
brings with it some limitations. This is because the content of the message body will not be
received if a response is queued in the first iteration. Furthermore, the connection will be closed
right after the response has been transferred then.

Both of these issues you will find addressed in the official @code{minimal_example.c} residing in
the @code{src/examples} directory of the @emph{GNU libmicrohttpd} package. The source code of this
program should look very familiar to you by now and easy to understand.

For our example, the @code{must_copy} and @code{must_free} parameter at the response construction
function could be set to @code{MHD_NO}. In the usual case, responses cannot be sent immediately
after being queued. For example, there might be other data on the system that needs to be sent with
a higher priority. Nevertheless, the queue function will return successfully---raising the problem
that the data we have pointed to may be invalid by the time it is about being sent. This is not an
issue here because we can expect the @code{page} string, which is a constant @emph{string literal}
here, to be static. That means it will be present and unchanged for as long as the program runs. 
For dynamic data, one could choose to either have @emph{MHD} free the memory @code{page} points 
to itself when it is not longer needed or, alternatively, have the library to make and manage 
its own copy of it.

@heading Exercises
@itemize @bullet
@item
While the server is running, use a program like telnet or netcat to connect to it. Try to form a
valid HTTP1.1 request yourself like
@verbatim
GET /dontcare HTTP1.1
Host: itsme
<enter>
@end verbatim
@noindent
and see what the server returns to you.
     

@item
Also, try other requests, like POST, and see how our server does not mind and why.
How far in malforming a request can you go before the builtin functionality of @emph{MHD} intervenes
and an altered response is sent? Make sure you read about the status codes in the @emph{RFC}.


@item
Add the option @code{MHD_USE_PEDANTIC_CHECKS} to the start function of the daemon in @code{main}.
Mind the special format of the parameter list here which is described in the manual. How indulgent
is the server now to your input?


@item
Let the main function take a string as the first command line argument and pass @code{argv[1]} to
the @code{MHD_start_daemon} function as the sixth parameter. The address of this string will be
passed to the callback function via the @code{cls} variable. Decorate the text given at the command
line when the server is started with proper HTML tags and send it as the response instead of the
former static string.


@item
@emph{Demanding:} Write a separate function returning a string containing some useful information, 
for example, the time. Pass the function's address as the sixth parameter and evaluate this function
on every request anew in @code{AnswerToConnection}. Remember to free the memory of the string
every time after satisfying the request.

@end itemize
