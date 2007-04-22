/* objreq.c
 * Object Requester
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

void objreq_end(struct status *, struct object_request *);
void object_timer(struct object_request *);

/* prev_url is a pointer to previous url or NULL */
/* prev_url will NOT be deallocated */
void request_object(struct terminal *term, unsigned char *url, unsigned char *prev_url, int pri, int cache, void (*upcall)(struct object_request *, void *), void *data, struct object_request **rqp)
{
	struct object_request *rq;
	if (!(rq = mem_calloc(sizeof(struct object_request)))) return;
	rq->state = O_WAITING;
	rq->refcount = 1;
	rq->term = term ? term->count : 0;
	rq->stat.end = (void (*)(struct status *, void *))objreq_end;
	rq->stat.data = rq;
	if (!(rq->orig_url = stracpy(url))) {
		mem_free(rq);
		return;
	}
	if (!(rq->url = stracpy(url))) {
		mem_free(rq->orig_url);
		mem_free(rq);
		return;
	}
	rq->pri = pri;
	rq->cache = cache;
	rq->upcall = upcall;
	rq->data = data;
	rq->timer = -1;
	rq->z = get_time() - STAT_UPDATE_MAX;
	rq->last_update = rq->z;
	rq->last_bytes = 0;
	if (rq->prev_url)mem_free(rq->prev_url);
	rq->prev_url=stracpy(prev_url);
	if (rqp) *rqp = rq;
	load_url(url, prev_url, &rq->stat, pri, cache);
}

void objreq_end(struct status *stat, struct object_request *rq)
{
	if (stat->state < 0) {
		if (stat->ce && stat->ce->redirect && rq->state == O_WAITING && rq->redirect_cnt++ < MAX_REDIRECTS) {
			unsigned char *u, *p;
			change_connection(stat, NULL, PRI_CANCEL);
			u = join_urls(rq->url, stat->ce->redirect);
                        if (!options_get_bool("http_bugs_302_redirect") && !stat->ce->redirect_get && (p = strchr(u, POST_CHAR))) add_to_strn(&u, p);
			mem_free(rq->url);
			rq->url = u;
			load_url(u, rq->prev_url, &rq->stat, rq->pri, rq->cache);
			return;
		}
	}
	if (stat->ce && !stat->ce->redirect) {
		rq->state = O_LOADING;
		if (!rq->ce) (rq->ce = stat->ce)->refcount++;
	}
	if (rq->timer != -1) kill_timer(rq->timer);
	rq->timer = install_timer(0, (void (*)(void *))object_timer, rq);
}

void object_timer(struct object_request *rq)
{
	int last = rq->last_bytes;
	if (rq->ce) rq->last_bytes = rq->ce->length;
	rq->timer = -1;
	if (rq->stat.state < 0 && (!rq->stat.ce || !rq->stat.ce->redirect)) {
		if (rq->stat.ce) {
			rq->state = rq->stat.state != S_OKAY ? O_INCOMPLETE : O_OK;
			/*(rq->ce = rq->stat.ce)->refcount++;*/
		} else rq->state = O_FAILED;
	}
	if (rq->stat.state != S_TRANS) {
		rq->last_update = rq->z;
		if (rq->upcall) rq->upcall(rq, rq->data);
	} else {
		ttime ct = get_time();
		ttime t = ct - rq->last_update;
		rq->timer = install_timer(STAT_UPDATE_MIN, (void (*)(void *))object_timer, rq);
		if (t >= STAT_UPDATE_MAX || (t >= STAT_UPDATE_MIN && rq->ce && rq->last_bytes > last)) {
			rq->last_update = ct;
			if (rq->upcall) rq->upcall(rq, rq->data);
		}
	}
}

void release_object_get_stat(struct object_request **rqq, struct status *news, int pri)
{
	struct object_request *rq = *rqq;
	if (!rq) return;
	*rqq = NULL;
	if (--rq->refcount) return;
	change_connection(&rq->stat, news, pri);
	if (rq->timer != -1) kill_timer(rq->timer);
	if (rq->ce) rq->ce->refcount--;
	mem_free(rq->orig_url);
	mem_free(rq->url);
	if (rq->prev_url)mem_free(rq->prev_url);
	mem_free(rq);
}

void release_object(struct object_request **rqq)
{
	release_object_get_stat(rqq, NULL, PRI_CANCEL);
}

void detach_object_connection(struct object_request *rq, int pos)
{
	if (rq->state == O_WAITING || rq->state == O_FAILED) {
		internal("detach_object_connection: no data received");
		return;
	}
	if (rq->refcount == 1) detach_connection(&rq->stat, pos);
}

void clone_object(struct object_request *rq, struct object_request **rqq)
{
	(*rqq = rq)->refcount++;
}

void stop_object_connection(struct object_request *rq)
{
        struct connection *c;
        struct status *stat=&(rq->stat);
        if (stat->state < 0) return;
        if (rq) {
                c=stat->c;
                setcstate(c, S_INTERRUPTED);
		abort_connection(c);
        }
}
