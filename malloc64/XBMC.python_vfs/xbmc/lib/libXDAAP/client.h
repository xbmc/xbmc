/* client class
 *
 * Copyright (c) 2004 David Hammerton
 * crazney@crazney.net
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

/* PUBLIC */

#ifndef _CLIENT_H
#define _CLIENT_H

#include <stdio.h>
#include "portability.h"

#ifdef __cplusplus
extern "C" {
#endif

/* type defintions */
typedef struct DAAP_SClientTAG DAAP_SClient;
typedef struct DAAP_SClientHostTAG DAAP_SClientHost;

/* the callback will call with a status code.
 * The updating and downloading status codes also call back
 * with an integer value, between 1 and 1000, representing
 * how much of the operation is complete.
 * Further information on the downloading status can be
 * discovered from the download streaming APIs
 * When you recieve the dying status, you have released the last
 * reference to the SClient object and should think about
 * releasing all of your SClientHost objects (although its not
 * mandatory, they won't be much good anymore).
 */

/* constants */
typedef enum
{
    DAAP_STATUS_error        = -2,
    DAAP_STATUS_dying        = -1,
    DAAP_STATUS_idle         =  0,
    DAAP_STATUS_connecting,
    DAAP_STATUS_negotiating,
    DAAP_STATUS_updating,
    DAAP_STATUS_downloading,
    DAAP_STATUS_hostschanged
} DAAP_Status;

/* function pointer definitions */
typedef void (*DAAP_fnClientStatus)(DAAP_SClient *, DAAP_Status, int, void*);

/* Hosts are discoverd via mDNS (rendezvous) automatically by the Client
 * interface (FIXME: only in multi-threaded environments for now).
 * Known hosts can be enumerated with the  DAAP_Client_EnumerateHosts function.
 * libopendaap holds a lock on the host list during the entire enumeration
 * process, so it is recommended that your DAAP_fnClientEnumerateHosts callback
 * is rather fast. Perhaps store the information you want to a list and process
 * it afterwards.
 * return 0 if you wish to interupt the enumeration (it will release the lock
 * and return immediatly) or any other number to continue.
 * DO NOT call any other DAAP_Client API functions during the enumeration.
 * You MAY call DAAP_ClientHost API functions during the enumeration. However,
 * be sure to call DAAP_Client_AddRef before doing so
 */
typedef int (*DAAP_fnClientEnumerateHosts)(DAAP_SClient *, DAAP_SClientHost *host,
                                          void *);

/* Async write callback, flag=1 for header, flag=2 for data, flag<=0 for finished */
typedef int (*DAAP_fnHttpWrite)(const char *buffer, int size, int flag, int contentlength, void* context);


/* Interface - Client */
DAAP_SClient *DAAP_Client_Create(DAAP_fnClientStatus pfnCallback,
                                 void *pvCallbackContext);
int DAAP_Client_SetDebug(DAAP_SClient *pCThis, const char *const debug);
unsigned int DAAP_Client_AddRef(DAAP_SClient *pCThis);
unsigned int DAAP_Client_Release(DAAP_SClient *pCThis);
unsigned int DAAP_Client_EnumerateHosts(DAAP_SClient *pCThis,
                                        DAAP_fnClientEnumerateHosts pfnCallback,
                                        void *context);
/* hack  -  don't use externally unless you have to. */
DAAP_SClientHost *DAAP_Client_AddHost(DAAP_SClient *pCThis, char *host,
                                      char *sharename, char *sharename_friendly);

unsigned int DAAP_Client_GetDatabases(DAAP_SClientHost *pCHThis);

int GetStreamThreadStatus(void);

/* Interface - ClientHost */

/* databases structure, get from DAAP_ClientHost_GetDatabases */
typedef struct
{
    int id;
    char *name;
} DAAP_ClientHost_Database;

/* item structure, get from DAAP_ClientHost_GetDatabaseItems */
typedef struct
{
    int id;
    char *itemname;
    char *songalbum;
    char *songartist;
    short songbeatsperminute;
    short songbitrate;
    short songdisccount;
    short songdiscnumber;
    char *songgenre;
    int songsamplerate;
    int songsize;
    int songtime;
    short songtrackcount;
    short songtracknumber;
    char songuserrating;
    short songyear;
    char *songformat;
} DAAP_ClientHost_DatabaseItem;

typedef struct
{
    int songid;
} DAAP_ClientHost_DatabasePlaylistItem;

typedef struct
{
    int id;
    int count;
    DAAP_ClientHost_DatabasePlaylistItem *items;
    char *itemname;
} DAAP_ClientHost_DatabasePlaylist;

typedef struct
{
    int size;
	int streamlen;
    void *data;
} DAAP_ClientHost_Song;

typedef struct albumTAG albumPTR;
struct albumTAG
{
	char *album;
	albumPTR *next;
};

typedef struct artistTAG artistPTR;
struct artistTAG
{
	char *artist;
	albumPTR *albumhead;
	artistPTR *next;
};



/* ClientHost classes must be created through the Client APIs,
 * these functions are intended to retrieve static information
 * from a SClientHost. Appart from AddRef and Release, no
 * state will be changed with these functions.  It is recommended
 * you keep a reference to them after the enumeration, however. */
unsigned int DAAP_ClientHost_AddRef(DAAP_SClientHost *pCHThis);
unsigned int DAAP_ClientHost_Release(DAAP_SClientHost *pCHThis);

/* call this to get the name of the host (user friendly name in utf8)
 * returns 0 on success, otherwise the size of the buffer required.
 */
unsigned int DAAP_ClientHost_GetSharename(DAAP_SClientHost *pCHThis,
                                          char *buf,
                                          int bufsize);

/* if the client is password protected, Connect will fail
 * with -401. Then call this with the password and
 * the next call to connect will use this password */

void DAAP_ClientHost_SetPassword(DAAP_SClientHost *pCHThis,
                                 char *password);
/* you must connect before using any of the things below.
 * disconnect when you no longer want to use it.
 */
unsigned int DAAP_ClientHost_Connect(DAAP_SClientHost *pCHThis);
unsigned int DAAP_ClientHost_Disconnect(DAAP_SClientHost *pCHThis);

/* returns 0 on successful copy, otherwise returns length
 * of required buffer (buffer is an array of size 'n' -
 *   string allocated at end of buffer)
 */
unsigned int DAAP_ClientHost_GetDatabases(DAAP_SClientHost *pCHThis,
                                          DAAP_ClientHost_Database *buffer,
                                          int *n, int bufsize);

/* returns 0 on successful copy, -1 on error, otherwise
 * length of required buffer
 */
int DAAP_ClientHost_GetDatabaseItems(DAAP_SClientHost *pCHThis,
                                     int databaseid,
                                     DAAP_ClientHost_DatabaseItem *buffer,
                                     int *n, int bufsize);

/* returns 0 on successful copy, otherwise returns length
 * of required buffer
 *   FIXME: for now databaseid is ignored, presumes single database
 */
unsigned int DAAP_ClientHost_GetPlaylists(DAAP_SClientHost *pCHThis,
                                          int databaseid,
                                          DAAP_ClientHost_DatabasePlaylist *buffer,
                                          int *n, int bufsize);

/* returns 0 on successful copy, -1 on error, otherwise
 * length of required buffer
 */
unsigned int DAAP_ClientHost_GetPlaylistItems(DAAP_SClientHost *pCHThis,
                                              int databaseid, int playlistid,
                                              DAAP_ClientHost_DatabasePlaylistItem *buffer,
                                              int *n, int bufsize);

/* returns 0 on success, -1 on error. be sure to free the file
 * with DAAP_ClientHost_FreeAudioFile */
int DAAP_ClientHost_GetAudioFile(DAAP_SClientHost *pCHThis,
                                 int databaseid, int songid,
                                 const char *songformat,
                                 DAAP_ClientHost_Song *song);
int DAAP_ClientHost_FreeAudioFile(DAAP_SClientHost *pCHThis,
                                  DAAP_ClientHost_Song *song);
/* async get of an audio file. writes it as it gets it to
 * the fd specified. also sends callback if possible.
 * returns immediatly.
 * I suggest you use a pipe as the fd.
 */
int DAAP_ClientHost_AsyncGetAudioFile(DAAP_SClientHost *pCHThis,
                                      int databaseid, int songid,
                                      const char *songformat,
#if defined(SYSTEM_POSIX)
								      int fd);
#elif defined(SYSTEM_WIN32)
								      HANDLE fd);
#else
	                                  FILE *fd);
#endif

int DAAP_ClientHost_AsyncGetAudioFileCallback(DAAP_SClientHost *pCHThis,
                                      int databaseid, int songid,
                                      const char *songformat,
                                      int startbyte,
                                      DAAP_fnHttpWrite callback,
                                      void* context);

/* call this to make the async get abort asap */
int DAAP_ClientHost_AsyncStop(DAAP_SClientHost *pCHThis);

/* [incomplete]
 * database update stuff. call this to make libopendaap open a request
 * to iTunes asking for database changes.
 * It will most likely run in a different thread and return status through
 * your status callback.
 * It will keep pushing new update requests until AsyncStopUpdate is called.
 */
int DAAP_ClientHost_AsyncWaitUpdate(DAAP_SClientHost *pCHThis);
/* [unimplemented]
 * stop the async wait update
 */
int DAAP_ClientHost_AsyncStopUpdate(DAAP_SClientHost *pCHThis);

// XBMC Async ops
int DAAP_ClientHost_GetAudioFileAsync(DAAP_SClientHost *pCHThis,
                                 int databaseid, int songid, char *songformat,
                                 DAAP_ClientHost_Song *song);

void DAAP_ClientHost_StopAudioFileAsync(DAAP_SClientHost *pCHThis);

//unsigned int Priv_DAAP_ClientHost_GetDatabasePlaylistItems(DAAP_SClientHost *pCHThis,
                                                         //int databaseid,
                                                         //int playlistid,
														 //DAAP_ClientHost_DatabasePlaylistItem *items);


#ifdef __cplusplus
}
#endif

#endif /* _CLIENT_H */
