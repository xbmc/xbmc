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

#ifndef _IOLOOP_H
#define _IOLOOP_H

typedef struct fd_eventTAG fd_event;

/* ioloop */
typedef void (*select_fd_datacb)(int fd, void *ctx);
typedef struct ioloopTAG ioloop;

void ioloop_add_select_item(
        ioloop *ioloop,
        int fd,
        select_fd_datacb callback,
        void *cb_context);

void ioloop_delete_select_item(
        ioloop *ioloop,
        int fd);

void ioloop_add_select_event(
        ioloop *ioloop,
        fd_event *event);

void ioloop_delete_select_event(
        ioloop *ioloop,
        fd_event *event);

ioloop* ioloop_create();

void ioloop_destroy(ioloop *ioloop);

/* doesn't return until it's destroyed.
 * event and select item callbacks will get
 * dispatched from within this context
 */
void ioloop_runloop(ioloop *ioloop);


/* events */
typedef void (*fd_event_callback)(fd_event *event, void *context);

fd_event *fd_event_create(int manual_reset,
                          fd_event_callback callback,
                          void *cb_context);

void fd_event_destroy(fd_event *event);

void fd_event_signal(fd_event *event);

void fd_event_reset(fd_event *event);


#endif /* _IOLOOP_H */

