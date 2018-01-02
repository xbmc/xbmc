/*****************************************************************
|
|   Wasabi - Blocker Socket internal header
|
|   $Id: NptBsdBlockerSocket.h 268 2015-05-28 19:13:46Z ehodzic $
|   Original author: Edin Hodzic (dino@concisoft.com)
|
|   This software is provided to you pursuant to your agreement 
|   with Intertrust Technologies Corporation ("Intertrust").
|   This software may be used only in accordance with the terms 
|   of the agreement.
|
|   Copyright (c) 2015 by Intertrust. All rights reserved. 
|
****************************************************************/

#ifndef _NPT_BSD_BLOCKER_SOCKET_H_
#define _NPT_BSD_BLOCKER_SOCKET_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptHash.h"
#include "NptThreads.h"

/*----------------------------------------------------------------------
|   forward declaration
+---------------------------------------------------------------------*/
class NPT_BsdSocketFd;
typedef NPT_Reference<NPT_BsdSocketFd> NPT_BsdSocketFdReference;

/*----------------------------------------------------------------------
|   NPT_Hash<NPT_Thread::ThreadId>
+---------------------------------------------------------------------*/
template <>
struct NPT_Hash<NPT_Thread::ThreadId>
{
    NPT_UInt32 operator()(NPT_Thread::ThreadId i) const { return NPT_Fnv1aHash32(reinterpret_cast<const NPT_UInt8*>(&i), sizeof(i)); }
};

/*----------------------------------------------------------------------
|   NPT_BlockerSocket
+---------------------------------------------------------------------*/
class NPT_BsdBlockerSocket {
public:
    NPT_BsdBlockerSocket(const NPT_BsdSocketFdReference& fd) {
        Set(NPT_Thread::GetCurrentThreadId(), fd.AsPointer());
    }
    ~NPT_BsdBlockerSocket() {
        Set(NPT_Thread::GetCurrentThreadId(), NULL);
    }

    static NPT_Result Cancel(NPT_Thread::ThreadId id);

private:
    static NPT_Result Set(NPT_Thread::ThreadId id, NPT_BsdSocketFd* fd);

    static NPT_Mutex MapLock;
    static NPT_HashMap<NPT_Thread::ThreadId, NPT_BsdSocketFd*> Map;
};

#endif // _NPT_BSD_BLOCKER_SOCKET_H_
