/* private header
 *
 * Copyright (c) 2003 David Hammerton
 * crazney@crazney.net
 *
 * private structures, function prototypes, etc
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

//#include "threadpool.h"
//#include "discover.h"
#include "client.h"
#include "httpClient.h"
#include "thread.h"

//#include "mdnsd/mdnsd.h"

/* function prototypes */
//unsigned int CP_GetTickCount();
char *safe_sprintf(const char *format, ...);

/* Client */
typedef struct DAAP_ClientHost_FakeTAG DAAP_ClientHost_Fake;

struct CP_SThreadPool;

struct DAAP_SClientTAG
{
    unsigned int uiRef;

    ts_mutex mtObjectLock;

    DAAP_fnClientStatus pfnCallbackStatus;
    void *pvCallbackStatusContext;

    DAAP_SClientHost *hosts;
    DAAP_ClientHost_Fake *fakehosts;

#if defined(SYSTEM_POSIX)
    struct CP_SThreadPool *tp;
#endif

    HTTP_ConnectionWatch *update_watch;
};

typedef struct
{
    int id;
    int nItems;
    int items_size;
    DAAP_ClientHost_DatabaseItem *items;
} DatabaseItemContainer;

typedef struct
{
    int id;
    int nPlaylists;
    int playlists_size;
    DAAP_ClientHost_DatabasePlaylist *playlists;
} DatabasePlaylistContainer;

struct DAAP_SClientHostTAG
{
    unsigned int uiRef;

    DAAP_SClient *parent;

    char *host; /* FIXME: use an address container (IPv4 vs IPv6) */
    HTTP_Connection *connection;

    char sharename_friendly[1005];
    char sharename[1005]; /* from mDNS */

    /* dmap/daap fields */
    int sessionid;
    int revision_number;

    int request_id;

    short version_major;
    short version_minor;

    int nDatabases;
    int databases_size;
    DAAP_ClientHost_Database *databases;

    DatabaseItemContainer *dbitems;
    DatabasePlaylistContainer *dbplaylists;

    int interrupt;

    char *password;

    DAAP_SClientHost *prev;
    DAAP_SClientHost *next;

    int marked; /* used for discover cb */
};
#if defined(SYSTEM_POSIX) /* otherwise use the structure elsewhere */
/* Discover */
#define DISC_RR_CACHE_SIZE 500
struct SDiscoverTAG
{
    unsigned int uiRef;

    ts_mutex mtObjectLock; /* this requires an object wide lock
                                     since the service thread holds a reference
                                     and tests it for death */
    ts_mutex mtWorkerLock;

#ifdef _WIN32
    fnDiscUpdated pfnUpdateCallback;
#endif
    void *pvCallbackArg;

    struct CP_SThreadPool *tp;

#ifdef _WIN32
    mdnsd mdnsd_info;
#endif
    int socket;

    int newquery_pipe[2];
    // answers
    /* answers */
    int pending_hosts;
#ifdef _WIN32
    SDiscover_HostList *prenamed;
    SDiscover_HostList *pending;
    SDiscover_HostList *have;
#endif
};

typedef struct CP_STPJobQueueTAG CP_STPJobQueue;
/* ThreadPool */
struct CP_STPJobQueueTAG
{
    CP_STPJobQueue *prev;
    CP_STPJobQueue *next;

    void (*fnJobCallback)(void *, void *);
    void *arg1, *arg2;
};

typedef struct CP_STPTimerQueueTAG CP_STPTimerQueue;
struct CP_STPTimerQueueTAG
{
    CP_STPTimerQueue *prev;
    CP_STPTimerQueue *next;

    unsigned int uiTimeSet;
    unsigned int uiTimeWait;

    void (*fnTimerCallback)(void *, void *);
    void *arg1, *arg2;
};

struct CP_SThreadPoolTAG
{
    unsigned int uiRef;

    unsigned int uiMaxThreads;
    ts_thread *prgptThreads; /* variable sized array */
    unsigned int uiThreadCount;

    ts_mutex mtJobQueueMutex;
    unsigned int uiJobCount;
    CP_STPJobQueue *pTPJQHead;
    CP_STPJobQueue *pTPJQTail;
    ts_condition cndJobPosted;

    ts_mutex mtTimerQueueMutex;
    CP_STPTimerQueue *pTPTQTail;
    ts_condition cndTimerPosted;

    unsigned int uiDying;
};
#endif

