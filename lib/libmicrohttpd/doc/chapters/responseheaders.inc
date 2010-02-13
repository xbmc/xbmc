Now that we are able to inspect the incoming request in great detail,
this chapter discusses the means to enrich the outgoing responses likewise.

As you have learned in the @emph{Hello, Browser} chapter, some obligatory 
header fields are added and set automatically for simple responses by the library
itself but if more advanced features are desired, additional fields have to be created.
One of the possible fields is the content type field and an example will be developed around it.
This will lead to an application capable of correctly serving different types of files.


When we responded with HTML page packed in the static string previously, the client had no choice
but guessing about how to handle the response, because the server hadn't told him. 
What if we had sent a picture or a sound file?  Would the message have been understood
or merely been displayed as an endless stream of random characters in the browser?
This is what the mime content types are for. The header of the response is extended
by certain information about how the data is to be interpreted. 

To introduce the concept, a picture of the format @emph{PNG} will be sent to the client 
and labeled accordingly with @code{image/png}.
Once again, we can base the new example on the @code{hellobrowser} program.

@verbatim
#define FILENAME "picture.png"
#define MIMETYPE "image/png"

int answer_to_connection (void *cls, struct MHD_Connection *connection, const char *url, 
                          const char *method, const char *version, const char *upload_data, 
                          size_t *upload_data_size, void **con_cls)
{
  unsigned char *buffer = NULL;
  struct MHD_Response *response;
@end verbatim
@noindent
 
We want the program to load the graphics file into memory:
@verbatim
  long size;
  FILE *fp;
  int ret = 0;

  if (0 != strcmp(method, "GET")) return MHD_NO;

  size = get_file_size (FILENAME);
  if (size != 0)
    {
      fp = fopen (FILENAME, "rb");
      if (fp) 
        {
          buffer = malloc (size);
      
            if (buffer)
              if (size == fread (buffer, 1, size, fp)) ret = 1;
      
          fclose(fp);
        }     
    }
@end verbatim
@noindent

The @code{GetFileSize} function, which returns a size of zero if the file could not be opened or
found, is left out on this page for tidiness. 

When dealing with files and allocating memory, there is a lot that could go wrong on the
server side and if so, the client should be informed with @code{MHD_HTTP_INTERNAL_SERVER_ERROR}.

@verbatim
  if (!ret) 
    {
      const char *errorstr = "<html><body>An internal server error has occured!\
                              </body></html>";

      if (buffer) free(buffer);
    
      response = MHD_create_response_from_data(strlen(errorstr), (void*)errorstr,
                                               MHD_NO, MHD_NO);

      if (response)
        {     
          ret = MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
          MHD_destroy_response (response);

          return MHD_YES;    
        } 
      else return MHD_NO;
    }
@end verbatim
@noindent

Note that we nevertheless have to create a response object even for sending a simple error code.
Otherwise, the connection would just be closed without comment, leaving the client curious about
what has happened.

But in the case of success a response will be constructed that contains the buffer filled with the
file's content. 

@verbatim
response = MHD_create_response_from_data (size, (void*)buffer, MHD_YES, MHD_NO);
@end verbatim
@noindent

Contrary to the above case where a static string will be sent, this time we have to 
keep track of the dynamically allocated buffer. As discussed in the @ref{Hello browser example}, 
the buffer cannot be safely freed as soon as the function call returns. Instead, we ask the function
to keep charge of freeing the buffer itself when it is not longer needed. Thus, no further @code{free}
command is invoked by us.

Up to this point, there was little new. The actual novelty is that we enhance the header with the
meta data about the content. Aware of the field's name we want to add, it is as easy as that:
@verbatim
MHD_add_response_header(response, "Content-Type", MIMETYPE);
@end verbatim
@noindent
We do not have to append a colon expected by the protocol hehind the first 
field---@emph{GNU libhttpdmicro} will take care of this. 

The function finishes with the well-known lines
@verbatim
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}
@end verbatim
@noindent

The complete program @code{responseheaders.c} is in the @code{examples} section as usual.
Find a @emph{PNG} file you like and save it to the directory the example is run from under the name
@code{picture.png}. You should find the image displayed on your browser if everything worked well.

@heading Remarks
The include file of the @emph{MHD} library comes with the header types mentioned in @emph{RFC 2616}
already defined as macros. Thus, we could have written @code{MHD_HTTP_HEADER_CONTENT_TYPE} instead
of @code{"Content-Type"} as well. However, one is not limited to these standard headers and could
add custom response headers without violating the protocol. Whether, and how, the client would react
to these custom header is up to the receiver. Likewise, the client is allowed to send custom request
headers to the server as well, opening up yet more possibilities how client and server could 
communicate with each other.

The method of creating the response from one big chunk of data is only feasible for smaller files.
A public file server satisfying many request at the same time would be choking under these high
demands on memory very soon. Serving responses in smaller parts would be more adequate here and
will be a topic of a future chapter.

@heading Exercises
@itemize @bullet

@item
Remember that the original program was written under a few assumptions---a small, static response
being one of them. In order to simulate a very large or hard to reach file that cannot be provided
instantly, postpone the queuing in the callback with the @code{sleep} function for 30 seconds 
@emph{if} the file @code{/big.png} is requested (but deliver the same as above). A request for
@code{/picture.png} should provide just the same but without any artificial delays.

Now start two instances of your browser (or even use two machines) and see how the second client
is put on hold while the first waits for his request on the slow file to be fulfilled.

Finally, change the sourcecode to use @code{MHD_USE_THREAD_PER_CONNECTION} when the daemon is 
started and try again.


@item
Did you succeed in implementing the clock exercise yet? This time, let the server save the 
program's start time @code{t} and implement a response simulating a countdown that reaches 0 at
@code{t+60}. Returning a message saying on which point the countdown is, the response should
ultimately be to reply "Done" if the program has been running long enough,

A non official, but widely understood, response header line is @code{Refresh: DELAY; url=URL} with
the uppercase words substituted to tell the client it should request the given resource after 
the given delay again. Improve your program in that the browser (any modern browser should work)
automatically reconnects and asks for the status again every 5 seconds or so. The URL would have
to be composed so that it begins with "http://", followed by the @emph{URI} the server is reachable
from the client's point of view.

Maybe you want also to visualize the countdown as a status bar by creating a 
@code{<table>} consisting of one row and @code{n} columns whose fields contain small images of either
a red or a green light.

@end itemize
