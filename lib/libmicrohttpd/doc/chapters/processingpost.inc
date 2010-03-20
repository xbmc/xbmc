The previous chapters already have demonstrated a variety of possibilities to send information 
to the HTTP server, but it is not recommended that the @emph{GET} method is used to alter the way
the server operates. To induce changes on the server, the @emph{POST} method is preferred over
and is much more powerful than @emph{GET} and will be introduced in this chapter.

We are going to write an application that asks for the visitor's name and, after the user has posted it,
composes an individual response text. Even though it was not mandatory to use the @emph{post} method here,
as there is no permanent change caused by the post, it is an illustrative example on how to share data
between different functions for the same connection. Furthermore, the reader should be able to extend
it easily.

@heading GET request
When the first @emph{GET} request arrives, the server shall respond with a HTML page containing an
edit field for the name.

@verbatim
const char* askpage = "<html><body>\
                       What's your name, Sir?<br>\
                       <form action=\"/namepost\" method=\"post\">\
                       <input name=\"name\" type=\"text\"\
                       <input type=\"submit\" value=\" Send \"></form>\
                       </body></html>";
@end verbatim
@noindent

The @code{action} entry is the @emph{URI} to be called by the browser when posting, and the
@code{name} will be used later to be sure it is the editbox's content that has been posted.

We also prepare the answer page, where the name is to be filled in later, and an error page 
as the response for anything but proper @emph{GET} and @emph{POST} requests:

@verbatim
const char* greatingpage="<html><body><h1>Welcome, %s!</center></h1></body></html>";

const char* errorpage="<html><body>This doesn't seem to be right.</body></html>";
@end verbatim
@noindent

Whenever we need to send a page, we use an extra function
@code{int SendPage(struct MHD_Connection *connection, const char* page)}
for this, which does not contain anything new and whose implementation is therefore left out here. 


@heading POST request
Posted data can be of arbitrary and considerable size; for example, if a user uploads a big
image to the server. Similar to the case of the header fields, there may also be different streams
of posted data, such as one containing the text of an editbox and another the state of a button.
Likewise, we will have to register an iterator function that is going to be called maybe several times 
not only if there are different POSTs but also if one POST has only been received partly yet and
needs processing before another chunk can be received.

Such an iterator function is called by a @emph{postprocessor}, which must be created upon arriving
of the post request.  We want the iterator function to read the first post data which is tagged
@code{name} and to create an individual greeting string based on the template and the name. 
But in order to pass this string to other functions and still be able to differentiate different
connections, we must first define a structure to share the information, holding the most import entries.

@verbatim
struct connection_info_struct
{
  int connectiontype;
  char *answerstring;
  struct MHD_PostProcessor *postprocessor; 
};
@end verbatim
@noindent

With these information available to the iterator function, it is able to fulfill its task. 
Once it has composed the greeting string, it returns @code{MHD_NO} to inform the post processor
that it does not need to be called again. Note that this function does not handle processing
of data for the same @code{key}. If we were to expect that the name will be posted in several
chunks, we had to expand the namestring dynamically as additional parts of it with the same @code{key}
came in. But in this example, the name is assumed to fit entirely inside one single packet.

@verbatim
int iterate_post (void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
                  const char *filename, const char *content_type,
                  const char *transfer_encoding, const char *data, uint64_t off, size_t size)
{
  struct connection_info_struct *con_info = (struct connection_info_struct*) coninfo_cls;
  

  if (0 == strcmp (key, "name"))
    {
      if ((size > 0) && (size <= MAXNAMESIZE))
        {
          char *answerstring;
          answerstring = malloc (MAXANSWERSIZE);
          if (!answerstring) return MHD_NO;
      
          snprintf (answerstring, MAXANSWERSIZE, greatingpage, data);
          con_info->answerstring = answerstring;      
        } 
      else con_info->answerstring = NULL;

      return MHD_NO;
    }

  return MHD_YES;
}
@end verbatim
@noindent

Once a connection has been established, it can be terminated for many reasons. As these
reasons include unexpected events, we have to register another function that cleans up any resources
that might have been allocated for that connection by us, namely the post processor and the greetings
string. This cleanup function must take into account that it will also be called for finished 
requests other than @emph{POST} requests.

@verbatim
void request_completed (void *cls, struct MHD_Connection *connection, void **con_cls,
                        enum MHD_RequestTerminationCode toe)
{
  struct connection_info_struct *con_info = (struct connection_info_struct*) *con_cls;


  if (NULL == con_info) return;

  if (con_info->connectiontype == POST)
    {
      MHD_destroy_post_processor (con_info->postprocessor);        
      if (con_info->answerstring) free (con_info->answerstring);
    }
  
  free (con_info);
  *con_cls = NULL;   
}
@end verbatim
@noindent

@emph{GNU libmicrohttpd} is informed that it shall call the above function when the daemon is started
in the main function.

@verbatim
...
daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                           &answer_to_connection, NULL, MHD_OPTION_NOTIFY_COMPLETED,
                           request_completed, NULL, MHD_OPTION_END);
...
@end verbatim
@noindent

@heading Request handling
With all other functions prepared, we can now discuss the actual request handling.

On the first iteration for a new request, we start by allocating a new instance of a 
@code{ConnectionInfoStruct} structure, which will store all necessary information for later
iterations and other functions.

@verbatim
int answer_to_connection (void *cls, struct MHD_Connection *connection, const char *url, 
                          const char *method, const char *version, const char *upload_data, 
                          size_t *upload_data_size, void **con_cls)
{
  if(NULL == *con_cls) 
    {
      struct connection_info_struct *con_info;

      con_info = malloc (sizeof (struct connection_info_struct));
      if (NULL == con_info) return MHD_NO;
      con_info->answerstring = NULL;
@end verbatim
@noindent

If the new request is a @emph{POST}, the postprocessor must be created now. In addition, the type
of the request is stored for convenience.
@verbatim
      if (0 == strcmp (method, "POST")) 
        {      
          con_info->postprocessor = MHD_create_post_processor (connection, POSTBUFFERSIZE, 
                                                               iterate_post, (void*) con_info);   

          if (NULL == con_info->postprocessor) 
            {
              free (con_info); 
              return MHD_NO;
            }

          con_info->connectiontype = POST;
        } 
      else con_info->connectiontype = GET;
@end verbatim
@noindent

The address of our structure will both serve as the indicator for successive iterations and to remember
the particular details about the connection.
@verbatim
      *con_cls = (void*) con_info; 
      return MHD_YES;
    }
@end verbatim
@noindent

The rest of the function will not be executed on the first iteration. A @emph{GET} request is easily
satisfied by sending the question form.
@verbatim
  if (0 == strcmp (method, "GET")) 
    {
      return send_page (connection, askpage);     
    } 
@end verbatim
@noindent

In case of @emph{POST}, we invoke the post processor for as long as data keeps incoming, setting
@code{*upload_data_size} to zero in order to indicate that we have processed---or at least have
considered---all of it.
@verbatim
  if (0 == strcmp (method, "POST")) 
    {
      struct connection_info_struct *con_info = *con_cls;

      if (*upload_data_size != 0) 
        {
          MHD_post_process(con_info->postprocessor, upload_data, *upload_data_size);
          *upload_data_size = 0;
          
          return MHD_YES;
        } 
      else if (NULL != con_info->answerstring) return send_page (connection, con_info->answerstring);
    } 
@end verbatim
@noindent

If they are neither @emph{GET} nor @emph{POST} requests, the error page is returned finally.
@verbatim
  return send_page(connection, errorpage); 
}
@end verbatim
@noindent

These were the important parts of the program @code{simplepost.c}.
