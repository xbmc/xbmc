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

/* PRIVATE */

/* all requests are handled syncrhonously, they _will_ block */

#ifndef _HTTP_CLIENT_H
#define _HTTP_CLIENT_H

typedef struct HTTP_ConnectionTAG HTTP_Connection;

struct HTTP_GetResultTAG
{
    int httpStatusCode;
    int contentlen;
    void *data;
};
typedef struct HTTP_GetResultTAG HTTP_GetResult;

/* callback function for http, flag=1 for header, flag=2 for data*/
typedef int (*HTTP_fnHttpWrite)(const char *buffer, int size, int flag, int contentlength, void* context);

HTTP_Connection *HTTP_Client_Open(const char *host, const char *password);
void HTTP_Client_Close(HTTP_Connection *connection);
HTTP_GetResult *HTTP_Client_Get(HTTP_Connection *connection,
                                const char *path,
                                const char *hash,
                                const char *extra_header,
                                int reset_send_close);

int HTTP_Client_Get_Callback(HTTP_Connection *connection,
                                const char *path,
                                const char *hash,
                                const char *extra_header,
                                int reset_send_close,
                                HTTP_fnHttpWrite callback, void *context);


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
                           int reset_send_close);

void HTTP_Client_FreeResult(HTTP_GetResult *res);

typedef struct HTTP_ConnectionWatchTAG HTTP_ConnectionWatch;

HTTP_ConnectionWatch *HTTP_Client_WatchQueue_New();
void HTTP_Client_WatchQueue_RunLoop(HTTP_ConnectionWatch *watch);

/* sends the GET with the given path / hash (generally update)
 * and adds the socket to the recv watch.
 */
void HTTP_Client_WatchQueue_AddUpdateWatch(
        HTTP_ConnectionWatch *watch,
        const HTTP_Connection *connection,
        const char *path,
        const char *hash,
        void (*response_callback)(void *ctx), /* FIXME result? */
        void *callback_ctx);

void HTTP_Client_WatchQueue_RemoveUpdateWatch(
        HTTP_ConnectionWatch *watch,
        const HTTP_Connection *connection);

void HTTP_Client_WatchQueue_Destroy(HTTP_ConnectionWatch *watch);


#endif /* _HTTP_CLIENT_H */

