// Place the code and data below here into the LIBXDAAP section.
#ifndef __GNUC__
#pragma code_seg( "LIBXDAAP_TEXT" )
#pragma data_seg( "LIBXDAAP_DATA" )
#pragma bss_seg( "LIBXDAAP_BSS" )
#pragma const_seg( "LIBXDAAP_RD" )
#endif

/* HTTP client requests
 * (mostly GET)
 *
 * Copyright (c) 2004 David Hammerton
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Port to XBMC by Forza of the XBMC Media Center development team
 *
 * 3rd July 2004
 *
 */

#include "libXDAAP.h"


#include "portability.h"
#include "thread.h"

#ifndef WIN32
#include "ioloop.h"
#endif

#include "httpClient.h"
#include "debug.h"

#define DEFAULT_DEBUG_CHANNEL "http_client"

#ifdef _LINUX
typedef int SOCKET;
#include <netdb.h>
#include <netinet/in.h>
#endif

#if defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/socket.h>
#endif

struct HTTP_ConnectionTAG
{
    char *host;
    char *password;
	SOCKET sockfd;
};

typedef struct HTTP_HeaderTAG HTTP_Header;
struct HTTP_HeaderTAG
{
    char *field_name;
    char *field_value;
    HTTP_Header *next;
};

/* when we get a song with iTunes, we have to create a new
 * HTTP connection. It seems to expect the HTTP connections
 * source ports to be seperated by 2. I'm guessing this is
 * because if the source port is X then it expects some
 * audiocast thing to be at X-1. (which we don't support).
 */
static void bind_socket(int sockfd)
{
    static int port = 1050; /* start here */

    struct sockaddr_in saddrBindingSocket;

    int bound = 0;
    int times = 0;

    saddrBindingSocket.sin_family = AF_INET;
    saddrBindingSocket.sin_addr.s_addr = INADDR_ANY;

    do
    {
        int res;

        saddrBindingSocket.sin_port = htons(port);
        res = bind(sockfd, (struct sockaddr *)&saddrBindingSocket,
                   sizeof(saddrBindingSocket));
        if (res == 0) bound = 1;
        port++;
    } while (!bound && (times++ < 20));
}


static SOCKET HTTP_Connect(const char *host, const char *port)
{
#ifdef WIN32
	SOCKET sockfd;
	struct sockaddr_in sa;

	memset(&sa, 0, sizeof (sa));
	sa.sin_port = htons(atoi(port));
	sa.sin_family = AF_INET;
	if (inet_addr(host) == INADDR_NONE)
    {
		return -1;
	}
	else
    {
		sa.sin_addr.s_addr = inet_addr(host);
    }

	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd < 0)	return -1;

	if (connect(sockfd, (struct sockaddr *)&sa, sizeof (sa)) != 0)
	{
		DAAP_SOCKET_CLOSE(sockfd);
		return -1;
	}

	return sockfd;
#else
    int sockfd, ret;
    struct addrinfo hints, *res, *ressave;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    ret = getaddrinfo(host, port, &hints, &res);

    if (ret)
    {
        ERR("getaddrinfo error: [%s] (%s)\n",
            gai_strerror(ret), host);
        return -1;
    }

    ressave = res;
    sockfd = -1;

    while (res)
    {
        sockfd = socket(res->ai_family,
                        res->ai_socktype,
                        res->ai_protocol);
        if (sockfd >= 0)
        {
            bind_socket(sockfd);

            if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
                break;

#ifdef SYSTEM_WIN32
            closesocket(sockfd);
#else
            close(sockfd);
#endif
            sockfd = -1;
        }
        res = res->ai_next;
    }

    freeaddrinfo(ressave);

    return sockfd;
#endif	
}

static void HTTP_Close(SOCKET sockfd)
{
	DAAP_SOCKET_CLOSE(sockfd);
    //close(sockfd);
}

HTTP_Connection *HTTP_Client_Open(const char *host, const char *password)
{
    HTTP_Connection *new = NULL;
    int sockfd;
    char *port_str;
    char *host_cpy = NULL;

    port_str = strchr(host, ':');
    if (port_str)
    {
        char *c;
        host_cpy = strdup(host);
        c = strchr(host_cpy, ':');
        c[0] = 0;

        port_str++;
    }
    if (port_str && !port_str[0])
        port_str = NULL;
    if (!port_str)
        port_str = "3689";

    sockfd = HTTP_Connect(host_cpy ? host_cpy : host, port_str);

    if (sockfd == -1) goto end;

    new = malloc(sizeof(HTTP_Connection));
    new->sockfd = sockfd;
    new->host = malloc(strlen(host)+1);
    strcpy(new->host, host_cpy ? host_cpy : host);

    if (password)
        new->password = strdup(password);
    else
        new->password = NULL;

end:
    if (host_cpy) free(host_cpy);
    return new;
}

static void HTTP_Client_Reset(HTTP_Connection *connection)
{
    HTTP_Close(connection->sockfd);
    connection->sockfd = HTTP_Connect(connection->host, "3689");
    if (connection->sockfd == -1) ERR("unhandled error\n");
}

void HTTP_Client_Close(HTTP_Connection *connection)
{
    HTTP_Close(connection->sockfd);
    if (connection->password) free(connection->password);
    free(connection->host);
    free(connection);
}

#define IS_CRLF(p) ((*p) == '\n' || (*p) == '\r')

static int HTTP_ProcessHeaders(char *curin, int curlen,
                               char **curout,
                               int *requiredExtraLen,
                               char **contentout, int *contentoutlen,
                               HTTP_Header **headers)
{
    char *curHeaderStart = curin;
    int curHeaderLen = 0;
    int n = 0;

    *requiredExtraLen = 0;
    *contentout = NULL;

    while (n < curlen)
    {
        HTTP_Header *header;
        int i;
        /* FIXME: there is a bug here. if the previous header ends on a single
         * \r, then the next header this will trigger with the \n and we'll
         * quit early :(
         */
        if (IS_CRLF(curHeaderStart))
        { /* end of header */
            char *content = curHeaderStart + 1;

            if (n < curlen-1)
            {
                if (IS_CRLF(content))
                {
                    content++;
                    n++;
                }
                *contentout = content;
                *contentoutlen = curlen - n - 1;
            }
            return 0;
        }
        while (!IS_CRLF(&curHeaderStart[curHeaderLen]))
        {
            curHeaderLen++;
            n++;
            if (n >= curlen)
            {
                if (curHeaderStart == curin)
                    *requiredExtraLen = 1;
                *curout = curHeaderStart;
                return 1;
            }
        }
        n++;
        curHeaderLen++;
        if (IS_CRLF(&curin[n]))
            n++;

        /* sizeof(HTTP_Header) + curHeaderLen + \0 + \0 - ':' - ' '
         * + (possibly a \0 if its not a real header) = 1 */
        header = malloc(sizeof(HTTP_Header) + curHeaderLen + 1);

        i = 0;
        header->field_name = (char*)header + sizeof(HTTP_Header);
        while (curHeaderStart[i] != ':' && i < curHeaderLen)
        {
            header->field_name[i] = curHeaderStart[i];
            i++;
        }

        if (curHeaderStart[i] == ':')
        {
            header->field_name[i] = 0;
            header->field_value = (char*)header + sizeof(HTTP_Header) +
                                  strlen(header->field_name) + 1;
            strncpy(header->field_value, &curHeaderStart[i+2],
                    curHeaderLen - i - 2);
            header->field_value[curHeaderLen - i - 3] = 0;
        }
        else
        {
            header->field_name[curHeaderLen-1] = 0;
            header->field_value = NULL;
        }
        header->next = NULL;

        /* we want to store it at the tail, but we are given the head */
        if (!(*headers))
        {
            *headers = header;
        }
        else
        {
            HTTP_Header *curIter = *headers;
            while (curIter->next)
            {
                curIter = curIter->next;
            }
            curIter->next = header;
        }

        curHeaderStart = &curin[n];
        curHeaderLen = 0;
    }
    *curout = NULL;
    return 1;
}

int HTTP_PassStandardHeaders(HTTP_Header *headersList,
                             int *httpContentLength)
{
    int httpStatusCode;
    HTTP_Header *cur;

    *httpContentLength = 0;

    /* first get the first header, this will be status code */
    if (headersList->field_value == NULL &&
            strncmp(headersList->field_name, "HTTP/1.1 ", 8) == 0)
    {
        TRACE("http status line: '%s'\n", headersList->field_name);
        httpStatusCode = atoi(headersList->field_name + 8);
    }
    else
    {
        ERR("HTTP status code wan't first\n");
        return 0;
    }

    /* now find content length (then return) */
    cur = headersList;
    while (cur)
    {
        if (strcmp(cur->field_name, "Content-Length") == 0)
        {
            *httpContentLength = atoi(cur->field_value);
            return httpStatusCode;
        }
        cur = cur->next;
    }

    return httpStatusCode;
}

#define METHOD_GET "GET "
#define HTTP_VERSION " HTTP/1.1\r\n"
#define HEADER_HOST "Host: "
#define DAAP_VERSION "\r\nClient-DAAP-Version: 3.0\r\n"
#define HEADER_USERAGENT "User-Agent: iTunes/4.6 (Windows; N)\r\n"
#define HEADER_ACCEPT "Accept-Language: en-us, en;q=5.0\r\n"
#define DAAP_VALIDATE "Client-DAAP-Validation: "
#define DAAP_ACCESS_INDEX "Client-DAAP-Access-Index: 2\r\n"
#define ACCEPT_ENCODING "Accept-Encoding: gzip\r\n"
#define CONNECTION_CLOSE "Connection: close\r\n"
#define HEADER_PASSWD "Authorization: Basic "
#define END_REQUEST "\r\n"

static int HTTP_Client_RequestGet(const HTTP_Connection *connection,
                                  const char *path, const char *hash,
                                  const char *extra_header,
                                  int send_close)
{
    int buffer_size = 0;
    char *buffer = NULL;
    char *buffersend = NULL;
    int tosend;
    int ret, res = 1;

    buffer_size += strlen(METHOD_GET);
    buffer_size += strlen(path);
    buffer_size += strlen(HTTP_VERSION);
    buffer_size += strlen(HEADER_HOST);
    buffer_size += strlen(connection->host);
    buffer_size += strlen(DAAP_VERSION);
    buffer_size += strlen(HEADER_USERAGENT);
    buffer_size += strlen(HEADER_ACCEPT);
    if (send_close)
        buffer_size += strlen(CONNECTION_CLOSE);
    buffer_size += strlen(DAAP_ACCESS_INDEX);
    if (hash)
    {
        buffer_size += strlen(DAAP_VALIDATE);
        buffer_size += strlen(hash);
        buffer_size += strlen(END_REQUEST);
    }
    if (connection->password)
    {
        buffer_size += strlen(HEADER_PASSWD);
        buffer_size += strlen(connection->password);
        buffer_size += strlen(END_REQUEST);
    }
//    buffer_size += strlen(ACCEPT_ENCODING);
    buffer_size += strlen(END_REQUEST);
    if (extra_header) buffer_size += strlen(extra_header);
    buffer_size += 1;

    buffer = malloc(buffer_size);
    buffer[0] = 0;
    strcat(buffer, METHOD_GET);
    strcat(buffer, path);
    strcat(buffer, HTTP_VERSION);
    strcat(buffer, HEADER_HOST);
    strcat(buffer, connection->host);
    strcat(buffer, DAAP_VERSION);
    strcat(buffer, HEADER_USERAGENT);
    strcat(buffer, HEADER_ACCEPT);
    strcat(buffer, DAAP_ACCESS_INDEX);
    if (hash)
    {
        strcat(buffer, DAAP_VALIDATE);
        strcat(buffer, hash);
        strcat(buffer, END_REQUEST);
    }
//    strcat(buffer, ACCEPT_ENCODING);
    if (extra_header) strcat(buffer, extra_header);
    if (send_close)
        strcat(buffer, CONNECTION_CLOSE);
    if (connection->password)
    {
        strcat(buffer, HEADER_PASSWD);
        strcat(buffer, connection->password);
        strcat(buffer, END_REQUEST);
    }
    /* just for printing. we shouldn't send this; */
    strcat(buffer, END_REQUEST);
    buffer[buffer_size-1] = 0;

    TRACE("about to send something of size %i\n", buffer_size);
    if (TRACE_ON) DPRINTF("%s\n\n", buffer);
#if 0
    debug_hexdump(buffer, buffer_size);
#endif

    buffersend = buffer;
    tosend = buffer_size - 1;
    while (tosend)
    {
        ret = DAAP_SOCKET_WRITE(connection->sockfd, buffersend, tosend);
        if (ret == -1)
        {
/*            ERR("send error: [%s]\n", strerror(errno));*/
            res = 0;
            goto end;
        }
        tosend -= ret;
        buffersend -= ret;
    }

end:
    free(buffer);
    return res;
}

static char *HTTP_Client_ReadHeaders(const HTTP_Connection *connection,
                                     HTTP_Header **headersList,
                                     char **contentFromHeaders,
                                     int *contentLenFromHeaders)
{
    char *buffer = NULL;
    int buffer_size;
    int ret;
    char *headers;
    int headers_needmore = 0;

    buffer_size = 1000;
    buffer = malloc(buffer_size);
    headers = NULL;
    do
    {
        int read_max = buffer_size;
        if (headers)
        {
            char *new;
            int required_copy;

            required_copy = buffer_size - (headers - buffer);
            if (headers_needmore)
                buffer_size *= 2;
            new = malloc(buffer_size);
            memcpy(new, headers, required_copy);

            free(buffer);
            buffer = new;

            headers = buffer + required_copy;
            read_max = buffer_size - required_copy;
        }
        else headers = buffer;
        ret = DAAP_SOCKET_READ(connection->sockfd, headers, read_max);
        if (ret == -1)
        {
            ERR("an error occured on recv!\n");
            free(buffer);
            goto end;
        }
        //debug_hexdump(buffer, ret);
    }
    while (HTTP_ProcessHeaders(buffer, ret,
                               &headers,
                               &headers_needmore,
                               contentFromHeaders, contentLenFromHeaders,
                               headersList));

    if (!*headersList)
        goto end;

    return buffer;
end:
    free(buffer);
    return NULL;

}

HTTP_GetResult *HTTP_Client_Get(HTTP_Connection *connection,
                                const char *path,
                                const char *hash,
                                const char *extra_header,
                                int reset_send_close) 
{
    char *databuffer = NULL;
    int torecv;
    int ret;
    char *headerbuffer = NULL;

    char *contentFromHeaders;
    int contentLenFromHeaders;

    HTTP_Header *headersList = NULL;
    HTTP_GetResult *result = NULL;

    int httpStatusCode;
    int httpContentLength;

    if (!HTTP_Client_RequestGet(connection, path, hash,
                                extra_header, reset_send_close))
        goto end;


    if (!(headerbuffer = HTTP_Client_ReadHeaders(connection, &headersList,
                                 &contentFromHeaders, &contentLenFromHeaders)))
    {
        ERR("failed to recieve any headers\n");
        goto end;
    }

    httpStatusCode = HTTP_PassStandardHeaders(headersList,
                                              &httpContentLength);

    result = malloc(sizeof(HTTP_GetResult) + httpContentLength);
    result->httpStatusCode = httpStatusCode;
    result->data = NULL;
    result->contentlen = 0;

    if (httpStatusCode == 401) /* unauthorized */
    {
        goto end;
    }
    if (httpStatusCode != 200 && httpStatusCode != 206)
    {
        ERR("invalid status code [%i]\n", httpStatusCode);
        goto end;
    }
    if (httpContentLength == 0)
    {
        ERR("no content length\n");
        goto end;
    }

    result->data = (char*)result + sizeof(HTTP_GetResult);
    result->contentlen = httpContentLength;

    torecv = httpContentLength;
    databuffer = (char *) result->data;
    if (contentFromHeaders)
    {
        memcpy(result->data, contentFromHeaders, contentLenFromHeaders);
        torecv -= contentLenFromHeaders;
        databuffer += contentLenFromHeaders;
    }
    free(headerbuffer);
    while (torecv)
    {
        ret = DAAP_SOCKET_READ(connection->sockfd, databuffer, torecv);
        if (ret == -1)
        {
            ERR("an error occured on recv\n");
            goto end;
        }
        torecv -= ret;
        databuffer += ret;
    }

	if (headersList)
	{
		HTTP_Header *header = NULL;

		while(headersList)
		{
			header = headersList->next;
			free(headersList);
			headersList = header;
		}
	}

    if (reset_send_close)
        HTTP_Client_Reset(connection);

    return result;
end:
	if (headerbuffer) free(headerbuffer);

    if (reset_send_close)
        HTTP_Client_Reset(connection);
    ERR("returning with error\n");

    return NULL;
}

int HTTP_Client_Get_Callback(HTTP_Connection *connection,
                                const char *path,
                                const char *hash,
                                const char *extra_header,
                                int reset_send_close, 
                                HTTP_fnHttpWrite callback, 
                                void* context)
{
    int chunksize = 1024;  
    char *databuffer = malloc(chunksize);
    char *headerbuffer = NULL;

    int torecv;
    int ret;
    int result = -1;
       
    char *contentFromHeaders;
    int contentLenFromHeaders;
    HTTP_Header *headersList = NULL;

    int httpStatusCode;
    int httpContentLength;

    if (!HTTP_Client_RequestGet(connection, path, hash,
                                extra_header, reset_send_close))
        goto end;


    if (!(headerbuffer = HTTP_Client_ReadHeaders(connection, &headersList,
                                 &contentFromHeaders, &contentLenFromHeaders)))
    {
        ERR("failed to recieve any headers\n");
        goto end;
    }

    httpStatusCode = HTTP_PassStandardHeaders(headersList,
                                              &httpContentLength);

    if (httpStatusCode == 401) /* unauthorized */
    {
        ERR("unauthorized");
        goto end;
    }
    if (httpStatusCode != 200 && httpStatusCode != 206)
    {
        ERR("invalid status code [%i]\n", httpStatusCode);
        goto end;
    }
    if (httpContentLength == 0)
    {
        ERR("no content length\n");
        goto end;
    }

    torecv = httpContentLength;
    
    if (contentFromHeaders)
    {
        ret = callback( contentFromHeaders, contentLenFromHeaders, 1, httpContentLength, context );
        if( ret < 0 )
        {
          TRACE("callback aborted");
          goto end;
        }

        torecv -= contentLenFromHeaders;
    }
    
    while (torecv)
    {
        ret = DAAP_SOCKET_READ(connection->sockfd, databuffer, chunksize);
        if (ret == -1)
        {
            ERR("an error occured on recv\n");
            goto end;
        }
        torecv -= ret;

        ret = callback( databuffer, ret, 2, httpContentLength, context );
        if( ret < 0 )
        {
          TRACE("callback aborted");
          goto end;
        }
    }


    result = 0;
end:
	  if (headersList)
	  {
		  HTTP_Header *header = NULL;

		  while(headersList)
		  {
			  header = headersList->next;
			  free(headersList);
			  headersList = header;
		  }
	  }

    if (headerbuffer) free(headerbuffer);
    if (databuffer) free(databuffer);

    if (reset_send_close)
        HTTP_Client_Reset(connection);


    return result;
}


/* used for async io */
int HTTP_Client_Get_ToFile(HTTP_Connection *connection,
                           const char *path,
                           const char *hash,
                           const char *extra_header,
#ifdef SYSTEM_POSIX
                           int filed,
#elif defined(SYSTEM_WIN32)
						   HANDLE filed,
#else
						   FILE *filed,
#endif
                           int (*pfnStatus)(void*,int),
                           void *userdata,
                           int reset_send_close)
{
    char *headerbuffer;
    char *contentFromHeaders;
    int contentLenFromHeaders;
    HTTP_Header *headersList = NULL;

    int httpStatusCode;
    int httpContentLength;

    char readbuffer[1024];
    int torecv;

    int last_micropercent = -1;
    int micropercent;


    if (!HTTP_Client_RequestGet(connection, path, hash,
                                extra_header, reset_send_close))
        goto end;

    if (!(headerbuffer = HTTP_Client_ReadHeaders(connection, &headersList,
                                 &contentFromHeaders, &contentLenFromHeaders)))
    {
        ERR("failed to recieve any headers\n");
        goto end;
    }

    httpStatusCode = HTTP_PassStandardHeaders(headersList,
                                              &httpContentLength);

    if (httpStatusCode != 200)
    {
        ERR("invalid status code [%i]\n", httpStatusCode);
        goto end;
    }
    if (httpContentLength == 0)
    {
        ERR("no content length\n");
        goto end;
    }

    torecv = httpContentLength;
    /* FIXME: write in portions */
    if (contentFromHeaders)
    {
#if defined(SYSTEM_POSIX)
        write(filed, contentFromHeaders, contentLenFromHeaders);
#elif defined(SYSTEM_WIN32)
		DWORD total_written = 0;
		do
		{
			DWORD written = 0;
		    WriteFile(filed, contentFromHeaders + total_written,
				      contentLenFromHeaders - total_written,
			          &written, NULL);
			total_written += written;
		} while (total_written < (unsigned)contentLenFromHeaders);
#else
		int total_written = 0;
		do
		{
			int written = 0;
			written = fwrite(contentFromHeaders + total_written, 1,
				   contentLenFromHeaders - total_written,
				   filed);
			total_written += written;
		} while (total_written < contentLenFromHeaders);
#endif
        torecv -= contentLenFromHeaders;
    }
    free(headerbuffer);

    micropercent = (int)(((float)(httpContentLength - torecv) /
                         (float)httpContentLength) * 1000.0);
    if (micropercent > last_micropercent)
    {
        if (pfnStatus(userdata, micropercent))
        {
            HTTP_Client_Reset(connection);
            goto end;
        }
        last_micropercent = micropercent;
    }


    while (torecv)
    {
        int ret;
        int readsize = (int)sizeof(readbuffer) < torecv ?
            (int)sizeof(readbuffer) : torecv;
        ret = DAAP_SOCKET_READ(connection->sockfd, readbuffer, readsize);
        if (ret == -1)
        {
            ERR("an error occured on recv\n");
            goto end;
        }
        torecv -= ret;
#if defined(SYSTEM_POSIX)
        write(filed, readbuffer, ret);
#elif defined(SYSTEM_WIN32)
		{
		  DWORD total_written = 0;
		  do
		  {
			  DWORD written = 0;
		      WriteFile(filed, readbuffer + total_written,
				        ret - total_written,
			            &written, NULL);
			  total_written += written;
		  } while (total_written < (unsigned)ret);
		}
#else
		{
		  int total_written = 0;
		  do
		  {
			  int written = 0;
			  written = fwrite(readbuffer + total_written, 1,
				     ret - total_written,
				     filed);
			  total_written += written;
		  } while (total_written < ret);
		}
#endif
        micropercent = (int)(((float)(httpContentLength - torecv) /
                             (float)httpContentLength) * 1000.0);
        if (micropercent > last_micropercent)
        {
            if (pfnStatus(userdata, micropercent))
            {
                HTTP_Client_Reset(connection);
                goto end;
            }
            last_micropercent = micropercent;
        }
    }

    if (reset_send_close)
        HTTP_Client_Reset(connection);
    return 1;
end:
    if (reset_send_close)
        HTTP_Client_Reset(connection);
    return 0;
}

void HTTP_Client_FreeResult(HTTP_GetResult *res)
{
    free(res);
}
#ifdef WIN32
HTTP_ConnectionWatch *HTTP_Client_WatchQueue_New()
{
  return NULL;
}
void HTTP_Client_WatchQueue_RunLoop(HTTP_ConnectionWatch *watch)
{

}
void HTTP_Client_WatchQueue_Destroy(HTTP_ConnectionWatch *watch)
{

}

void HTTP_Client_WatchQueue_AddUpdateWatch(
        HTTP_ConnectionWatch *watch,
        const HTTP_Connection *connection,
        const char *path,
        const char *hash,
        void (*response_callback)(void *ctx), /* FIXME result? */
        void *callback_ctx)
{

}

void HTTP_Client_WatchQueue_RemoveUpdateWatch(
        HTTP_ConnectionWatch *watch,
        const HTTP_Connection *connection)
{
}
#else

/*************** For the ConnectionWatch stuff ************/
typedef struct watch_listTAG watch_list;
struct watch_listTAG
{
    const HTTP_Connection *connection;
    void (*callback)(void *ctx);
    void *cb_context;
    watch_list *next;
    HTTP_ConnectionWatch *parent;
};


struct HTTP_ConnectionWatchTAG
{
    ioloop *main_loop;
    watch_list *watch_list_head;
    ts_mutex mutex;
};


HTTP_ConnectionWatch *HTTP_Client_WatchQueue_New()
{
    HTTP_ConnectionWatch *new_watch = malloc(sizeof(HTTP_ConnectionWatch));
    if (!new_watch) goto err;

    new_watch->main_loop = NULL;
    new_watch->watch_list_head = NULL;

    new_watch->main_loop = ioloop_create();
    if (!new_watch->main_loop)
        goto err;

    ts_mutex_create(new_watch->mutex);

    return new_watch;

err:
    if (new_watch)
    {
        if (new_watch->main_loop) ioloop_destroy(new_watch->main_loop);
        free(new_watch);
    }
    return NULL;
}

void HTTP_Client_WatchQueue_RunLoop(HTTP_ConnectionWatch *watch)
{
    ioloop_runloop(watch->main_loop);
}

void HTTP_Client_WatchQueue_Destroy(HTTP_ConnectionWatch *watch)
{
    watch_list *cur;
    ts_mutex_lock(watch->mutex);
    ioloop_destroy(watch->main_loop);

    cur = watch->watch_list_head;
    while (cur)
    {
        free(cur);
        cur = cur->next;
    }

    ts_mutex_destroy(watch->mutex);
}

void httpwatch_callback(int fd, void *ctx)
{
    watch_list *watch_item = (watch_list*)ctx;
    void (*cb)(void *ctx);
    void *cb_ctx;

    ts_mutex_lock(watch_item->parent->mutex);
    cb = watch_item->callback;
    cb_ctx = watch_item->cb_context;
    ts_mutex_unlock(watch_item->parent->mutex);

    if (cb) cb(cb_ctx);
}

void HTTP_Client_WatchQueue_AddUpdateWatch(
        HTTP_ConnectionWatch *watch,
        const HTTP_Connection *connection,
        const char *path,
        const char *hash,
        void (*response_callback)(void *ctx), /* FIXME result? */
        void *callback_ctx)
{
    watch_list *new_item = NULL;
    if (!HTTP_Client_RequestGet(connection, path, hash, NULL, 0))
    {
        TRACE("get failed\n");
        return; /* FIXME send failure */
    }

    ts_mutex_lock(watch->mutex);

    new_item = malloc(sizeof(watch_list));

    new_item->connection = connection;
    new_item->callback = response_callback;
    new_item->cb_context = callback_ctx;
    new_item->next = watch->watch_list_head;
    watch->watch_list_head = new_item;
    new_item->parent = watch;

    ioloop_add_select_item(watch->main_loop, connection->sockfd,
                           httpwatch_callback, new_item);

    ts_mutex_unlock(watch->mutex);
}

void HTTP_Client_WatchQueue_RemoveUpdateWatch(
        HTTP_ConnectionWatch *watch,
        const HTTP_Connection *connection)
{
    watch_list *cur;
    watch_list *prev = NULL;
    ts_mutex_lock(watch->mutex);

    cur = watch->watch_list_head;
    while (cur)
    {
        if (cur->connection == connection)
        {
            ioloop_delete_select_item(watch->main_loop, connection->sockfd);
            if (prev) prev->next = cur->next;
            else watch->watch_list_head = cur->next;
            cur->callback = NULL;
            free(cur);
            ts_mutex_unlock(watch->mutex);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
    ERR("connection not being watched?\n");

    ts_mutex_unlock(watch->mutex);
}

#endif
