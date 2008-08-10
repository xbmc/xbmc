// Place the code and data below here into the LIBXDAAP section.
#ifndef __GNUC__
#pragma code_seg( "LIBXDAAP_TEXT" )
#pragma data_seg( "LIBXDAAP_DATA" )
#pragma bss_seg( "LIBXDAAP_BSS" )
#pragma const_seg( "LIBXDAAP_RD" )
#endif

/* ioloop stuff
 * Can add IO watches, event watches
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

#define WIN32_LEAN_AND_MEAN

#include "portability.h"

#ifdef SYSTEM_POSIX
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <fcntl.h>
#else
#error implement me
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "thread.h"
#include "ioloop.h"
#include "debug.h"

#define DEFAULT_DEBUG_CHANNEL "ioloop"


typedef struct select_itemTAG select_item;

struct ioloopTAG
{
    select_item *select_head;
    /* these two events could probably be combined into one */
    fd_event *destroy_event;
    fd_event *modify_select_event;
    int destroying, loop_running;
    ts_mutex mutex;

    /* debug */
    int entry_level;
};

/******* abstraction layer for events ********/
struct fd_eventTAG
{
    int pipe[2];
    int signalled;
    int manual_reset;
    fd_event_callback callback;
    void *cb_context;
};

fd_event *fd_event_create(int manual_reset,
                          fd_event_callback callback,
                          void *cb_context)
{
    fd_event *new_event = malloc(sizeof(fd_event));

    new_event->signalled = 0;
    new_event->manual_reset = manual_reset;
    new_event->callback = callback;
    new_event->cb_context = cb_context;

    if (pipe(new_event->pipe))
    {
        free(new_event);
        return NULL;
    }

    return new_event;
}

void fd_event_destroy(fd_event *event)
{
    close(event->pipe[0]);
    close(event->pipe[1]);
}

void fd_event_signal(fd_event *event)
{
    const char buf[1] = {0};
    event->signalled = 1;
    write(event->pipe[1], buf, 1);
}

void fd_event_reset(fd_event *event)
{
    event->signalled = 0;
    char buf[1] = {0};
    int flags;

    /* set non-blocking on read pipe */
    flags = fcntl(event->pipe[0], F_GETFL, 0);
    if (flags == -1) flags = 0;
    fcntl(event->pipe[0], F_SETFL, flags | O_NONBLOCK);

    /* read until empty - pipe is non-blocking as above */
    while (read(event->pipe[0], buf, 1) == 1)
    {
    }

    /* set blocking */
    fcntl(event->pipe[0], F_SETFL, flags);
}

static int fd_event_getfd(fd_event *event)
{
    return event->pipe[0];
}

static void fd_event_handle(int fd, void *ctx)
{
    fd_event *event = (fd_event*)ctx;
    if (!event->manual_reset)
        fd_event_reset(event);

    if (event->callback)
        event->callback(event, event->cb_context);
}

/************* main event loop for connection watch *********/
/******* select abstraction *******/
struct select_itemTAG
{
    int fd;
    select_fd_datacb callback;
    void *cb_context;
    select_item *next;
};

void ioloop_add_select_item(
        ioloop *ioloop,
        int fd,
        select_fd_datacb callback,
        void *cb_context)
{
    select_item *new_select = malloc(sizeof(select_item));

    if (ioloop->entry_level) FIXME("reentering ioloop, could be a problem\n");

    ts_mutex_lock(ioloop->mutex);

    new_select->fd = fd;
    new_select->callback = callback;
    new_select->cb_context = cb_context;
    new_select->next = ioloop->select_head;
    ioloop->select_head = new_select;

    fd_event_signal(ioloop->modify_select_event);
    ts_mutex_unlock(ioloop->mutex);
}

void ioloop_delete_select_item(
        ioloop *ioloop,
        int fd)
{
    select_item *cur;
    select_item *prev = NULL;

    if (ioloop->entry_level) FIXME("reentering ioloop, could be a problem\n");

    ts_mutex_lock(ioloop->mutex);
    cur = ioloop->select_head;
    while (cur)
    {
        if (cur->fd == fd)
        {
            if (prev) prev->next = cur->next;
            else ioloop->select_head = cur->next;
            free(cur);
            ts_mutex_unlock(ioloop->mutex);
            return;
        }

        prev = cur;
        cur = cur->next;
    }
    fd_event_signal(ioloop->modify_select_event);
    ts_mutex_unlock(ioloop->mutex);
}

void ioloop_add_select_event(
        ioloop *ioloop,
        fd_event *event)
{
    ioloop_add_select_item(ioloop, fd_event_getfd(event),
                           fd_event_handle, (void*)event);
}

void ioloop_delete_select_event(
        ioloop *ioloop,
        fd_event *event)
{
    ioloop_delete_select_item(ioloop, fd_event_getfd(event));
}

static void ioloop_realdestroy(ioloop *ioloop)
{
    select_item *cur = ioloop->select_head;
    TRACE("(%p)\n", ioloop);
    while (cur)
    {
        select_item *next = cur->next;
        free(cur);
        cur = next;
    }
    fd_event_destroy(ioloop->destroy_event);
    fd_event_destroy(ioloop->modify_select_event);
    ts_mutex_destroy(ioloop->mutex);
}

void ioloop_runloop(ioloop *ioloop)
{
    ioloop->loop_running = 1;
    do
    {
        fd_set fdset;
        int retval;
        int n = 0, i = 0;
        select_item *cur = NULL;

        /* setup select */
        FD_ZERO(&fdset);

        ts_mutex_lock(ioloop->mutex);
        cur = ioloop->select_head;
        while (cur)
        {
            FD_SET(cur->fd, &fdset);
            if (cur->fd > n) n = cur->fd;

            cur = cur->next;
            i++;
        }
        ts_mutex_unlock(ioloop->mutex);

        retval = select(n, &fdset, NULL, NULL, NULL);

        if (retval <= 0)
        {
            ERR("select failed\n");
            continue;
        }

        ts_mutex_lock(ioloop->mutex);
        n = 0;
        cur = ioloop->select_head;
        while (cur && retval > n)
        {
            if (FD_ISSET(cur->fd, &fdset))
            {
                ioloop->entry_level++;
                cur->callback(cur->fd, cur->cb_context);
                ioloop->entry_level--;
                n++;
            }

            cur = cur->next;
        }
        ts_mutex_unlock(ioloop->mutex);

    } while (ioloop->destroying == 0);
    ioloop_realdestroy(ioloop);
}

ioloop* ioloop_create()
{
    ioloop *newIOLoop = malloc(sizeof(ioloop));
    newIOLoop->select_head = NULL;
    ts_mutex_create_recursive(newIOLoop->mutex);

    newIOLoop->entry_level = 0;
    newIOLoop->loop_running = 0;
    newIOLoop->destroying = 0;

 /* can probably combine this into one */
    newIOLoop->modify_select_event = fd_event_create(0, NULL, NULL);
    newIOLoop->destroy_event = fd_event_create(0, NULL, NULL);
    ioloop_add_select_event(newIOLoop, newIOLoop->modify_select_event);
    ioloop_add_select_event(newIOLoop, newIOLoop->destroy_event);

    return newIOLoop;
}

void ioloop_destroy(ioloop *ioloop)
{
    if (ioloop->entry_level) TRACE("reentering ioloop, could be a problem\n");

    ts_mutex_lock(ioloop->mutex);
    if (ioloop->loop_running)
    {
        ioloop->destroying = 1;
        fd_event_signal(ioloop->destroy_event);
    }
    else
    {
        ioloop_realdestroy(ioloop);
    }
    /* mutex may be destroyed by now */
    /*ts_mutex_unlock(ioloop->mutex);*/
}

