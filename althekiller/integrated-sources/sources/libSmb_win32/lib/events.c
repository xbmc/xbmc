/*
   Unix SMB/CIFS implementation.
   Timed event library.
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) Volker Lendecke 2005

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"

static struct timed_event *timed_events;

static int timed_event_destructor(void *p)
{
	struct timed_event *te = talloc_get_type_abort(p, struct timed_event);
	DEBUG(10, ("Destroying timed event %lx \"%s\"\n", (unsigned long)te,
		te->event_name));
	DLIST_REMOVE(timed_events, te);
	return 0;
}

/****************************************************************************
 Schedule a function for future calling, cancel with TALLOC_FREE().
 It's the responsibility of the handler to call TALLOC_FREE() on the event
 handed to it.
****************************************************************************/

struct timed_event *add_timed_event(TALLOC_CTX *mem_ctx,
				struct timeval when,
				const char *event_name,
				void (*handler)(struct timed_event *te,
						const struct timeval *now,
						void *private_data),
				void *private_data)
{
	struct timed_event *te, *last_te, *cur_te;

	te = TALLOC_P(mem_ctx, struct timed_event);
	if (te == NULL) {
		DEBUG(0, ("talloc failed\n"));
		return NULL;
	}

	te->when = when;
	te->event_name = event_name;
	te->handler = handler;
	te->private_data = private_data;

	/* keep the list ordered */
	last_te = NULL;
	for (cur_te = timed_events; cur_te; cur_te = cur_te->next) {
		/* if the new event comes before the current one break */
		if (!timeval_is_zero(&cur_te->when) &&
				timeval_compare(&te->when, &cur_te->when) < 0) {
			break;
		}
		last_te = cur_te;
	}

	DLIST_ADD_AFTER(timed_events, te, last_te);
	talloc_set_destructor(te, timed_event_destructor);

	DEBUG(10, ("Added timed event \"%s\": %lx\n", event_name,
			(unsigned long)te));
	return te;
}

void run_events(void)
{
	struct timeval now;

	if (timed_events == NULL) {
		/* No syscall if there are no events */
		DEBUG(11, ("run_events: No events\n"));
		return;
	}

	GetTimeOfDay(&now);

	if (timeval_compare(&now, &timed_events->when) < 0) {
		/* Nothing to do yet */
		DEBUG(11, ("run_events: Nothing to do\n"));
		return;
	}

	DEBUG(10, ("Running event \"%s\" %lx\n", timed_events->event_name,
		(unsigned long)timed_events));

	timed_events->handler(timed_events, &now, timed_events->private_data);
	return;
}

struct timeval *get_timed_events_timeout(struct timeval *to_ret)
{
	struct timeval now;

	if (timed_events == NULL) {
		return NULL;
	}

	now = timeval_current();
	*to_ret = timeval_until(&now, &timed_events->when);

	DEBUG(10, ("timed_events_timeout: %d/%d\n", (int)to_ret->tv_sec,
		(int)to_ret->tv_usec));

	return to_ret;
}
