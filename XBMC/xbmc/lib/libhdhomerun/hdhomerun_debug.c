/*
 * hdhomerun_debug.c
 *
 * Copyright © 2006 Silicondust Engineering Ltd. <www.silicondust.com>.
 *
 * This library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * As a special exception to the GNU Lesser General Public License,
 * you may link, statically or dynamically, an application with a
 * publicly distributed version of the Library to produce an
 * executable file containing portions of the Library, and
 * distribute that executable file under terms of your choice,
 * without any of the additional requirements listed in clause 4 of
 * the GNU Lesser General Public License.
 * 
 * By "a publicly distributed version of the Library", we mean
 * either the unmodified Library as distributed by Silicondust, or a
 * modified version of the Library that is distributed under the
 * conditions defined in the GNU Lesser General Public License.
 */

/*
 * The debug logging includes optional support for connecting to the
 * Silicondust support server. This option should not be used without
 * being explicitly enabled by the user. Debug information should be
 * limited to information useful to diagnosing a problem.
 *  - Silicondust.
 */

#include "hdhomerun.h"

#if !defined(HDHOMERUN_DEBUG_HOST)
#define HDHOMERUN_DEBUG_HOST "debug.silicondust.com"
#endif
#if !defined(HDHOMERUN_DEBUG_PORT)
#define HDHOMERUN_DEBUG_PORT "8002"
#endif

struct hdhomerun_debug_message_t
{
	struct hdhomerun_debug_message_t *next;
	struct hdhomerun_debug_message_t *prev;
	char buffer[2048];
};

struct hdhomerun_debug_t
{
	pthread_t thread;
	volatile bool_t enabled;
	volatile bool_t terminate;
	char *prefix;

	pthread_mutex_t print_lock;
	pthread_mutex_t queue_lock;
	pthread_mutex_t send_lock;

	struct hdhomerun_debug_message_t *queue_head;
	struct hdhomerun_debug_message_t *queue_tail;
	uint32_t queue_depth;

	uint64_t connect_delay;

	char *file_name;
	FILE *file_fp;
	int sock;
};

static THREAD_FUNC_PREFIX hdhomerun_debug_thread_execute(void *arg);

struct hdhomerun_debug_t *hdhomerun_debug_create(void)
{
	struct hdhomerun_debug_t *dbg = (struct hdhomerun_debug_t *)calloc(1, sizeof(struct hdhomerun_debug_t));
	if (!dbg) {
		return NULL;
	}

	dbg->sock = -1;

	pthread_mutex_init(&dbg->print_lock, NULL);
	pthread_mutex_init(&dbg->queue_lock, NULL);
	pthread_mutex_init(&dbg->send_lock, NULL);

	if (pthread_create(&dbg->thread, NULL, &hdhomerun_debug_thread_execute, dbg) != 0) {
		free(dbg);
		return NULL;
	}

	return dbg;
}

/* Send lock held by caller */
static void hdhomerun_debug_close_file(struct hdhomerun_debug_t *dbg)
{
	if (!dbg->file_fp) {
		return;
	}

	fclose(dbg->file_fp);
	dbg->file_fp = NULL;
}

/* Send lock held by caller */
static void hdhomerun_debug_close_sock(struct hdhomerun_debug_t *dbg)
{
	if (dbg->sock == -1) {
		return;
	}

	close(dbg->sock);
	dbg->sock = -1;
}

void hdhomerun_debug_destroy(struct hdhomerun_debug_t *dbg)
{
	dbg->terminate = TRUE;
	pthread_join(dbg->thread, NULL);

	hdhomerun_debug_close_file(dbg);
	hdhomerun_debug_close_sock(dbg);

	if (dbg->prefix) {
		free(dbg->prefix);
	}

	free(dbg);
}

void hdhomerun_debug_set_prefix(struct hdhomerun_debug_t *dbg, const char *prefix)
{
	pthread_mutex_lock(&dbg->print_lock);

	if (dbg->prefix) {
		free(dbg->prefix);
		dbg->prefix = NULL;
	}

	if (prefix) {
		dbg->prefix = strdup(prefix);
	}

	pthread_mutex_unlock(&dbg->print_lock);
}

void hdhomerun_debug_set_filename(struct hdhomerun_debug_t *dbg, const char *filename)
{
	pthread_mutex_lock(&dbg->send_lock);

	if (!filename && !dbg->file_name) {
		pthread_mutex_unlock(&dbg->send_lock);
		return;
	}
	if (filename && dbg->file_name) {
		if (strcmp(filename, dbg->file_name) == 0) {
			pthread_mutex_unlock(&dbg->send_lock);
			return;
		}
	}

	hdhomerun_debug_close_file(dbg);
	hdhomerun_debug_close_sock(dbg);

	if (dbg->file_name) {
		free(dbg->file_name);
		dbg->file_name = NULL;
	}
	if (filename) {
		dbg->file_name = strdup(filename);
	}

	pthread_mutex_unlock(&dbg->send_lock);
}

void hdhomerun_debug_enable(struct hdhomerun_debug_t *dbg)
{
	pthread_mutex_lock(&dbg->send_lock);

	dbg->enabled = TRUE;

	pthread_mutex_unlock(&dbg->send_lock);
}

void hdhomerun_debug_disable(struct hdhomerun_debug_t *dbg)
{
	pthread_mutex_lock(&dbg->send_lock);

	dbg->enabled = FALSE;
	hdhomerun_debug_close_file(dbg);
	hdhomerun_debug_close_sock(dbg);

	pthread_mutex_unlock(&dbg->send_lock);
}

bool_t hdhomerun_debug_enabled(struct hdhomerun_debug_t *dbg)
{
	if (!dbg) {
		return FALSE;
	}

	return dbg->enabled;
}

void hdhomerun_debug_flush(struct hdhomerun_debug_t *dbg, uint64_t timeout)
{
	timeout = getcurrenttime() + timeout;

	while (getcurrenttime() < timeout) {
		pthread_mutex_lock(&dbg->queue_lock);
		struct hdhomerun_debug_message_t *message = dbg->queue_tail;
		pthread_mutex_unlock(&dbg->queue_lock);

		if (!message) {
			return;
		}

		msleep(10);
	}
}

void hdhomerun_debug_printf(struct hdhomerun_debug_t *dbg, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	hdhomerun_debug_vprintf(dbg, fmt, args);
	va_end(args);
}

void hdhomerun_debug_vprintf(struct hdhomerun_debug_t *dbg, const char *fmt, va_list args)
{
	if (!dbg) {
		return;
	}
	if (!dbg->enabled) {
		return;
	}

	struct hdhomerun_debug_message_t *message = (struct hdhomerun_debug_message_t *)malloc(sizeof(struct hdhomerun_debug_message_t));
	if (!message) {
		return;
	}

	char *ptr = message->buffer;
	char *end = message->buffer + sizeof(message->buffer) - 2;
	*end = 0;

	/*
	 * Timestamp.
	 */
	time_t current_time = time(NULL);
	ptr += strftime(ptr, end - ptr, "%Y%m%d-%H:%M:%S ", localtime(&current_time));
	if (ptr > end) {
		ptr = end;
	}

	/*
	 * Debug prefix.
	 */
	pthread_mutex_lock(&dbg->print_lock);

	if (dbg->prefix) {
		int len = snprintf(ptr, end - ptr, "%s ", dbg->prefix);
		len = (len <= 0) ? 0 : len;
		ptr += len;
		if (ptr > end) {
			ptr = end;
		}
	}

	pthread_mutex_unlock(&dbg->print_lock);

	/*
	 * Message text.
	 */
	int len = vsnprintf(ptr, end - ptr, fmt, args);
	len = (len < 0) ? 0 : len; /* len does not include null */
	ptr += len;
	if (ptr > end) {
		ptr = end;
	}

	/*
	 * Force newline.
	 */
	if ((ptr[-1] != '\n') && (ptr + 1 <= end)) {
		*ptr++ = '\n';
	}

	/*
	 * Force NULL.
	 */
	if (ptr + 1 > end) {
		ptr = end - 1;
	}
	*ptr++ = 0;

	/*
	 * Enqueue.
	 */
	pthread_mutex_lock(&dbg->queue_lock);

	message->prev = NULL;
	message->next = dbg->queue_head;
	dbg->queue_head = message;
	if (message->next) {
		message->next->prev = message;
	} else {
		dbg->queue_tail = message;
	}
	dbg->queue_depth++;

	pthread_mutex_unlock(&dbg->queue_lock);
}

/* Send lock held by caller */
static bool_t hdhomerun_debug_output_message_file(struct hdhomerun_debug_t *dbg, struct hdhomerun_debug_message_t *message)
{
	if (!dbg->file_fp) {
		uint64_t current_time = getcurrenttime();
		if (current_time < dbg->connect_delay) {
			return FALSE;
		}
		dbg->connect_delay = current_time + 30*1000;

		dbg->file_fp = fopen(dbg->file_name, "a");
		if (!dbg->file_fp) {
			return FALSE;
		}
	}

	fprintf(dbg->file_fp, "%s", message->buffer);
	fflush(dbg->file_fp);

	return TRUE;
}

/* Send lock held by caller */
#if defined(__CYGWIN__)
static bool_t hdhomerun_debug_output_message_sock(struct hdhomerun_debug_t *dbg, struct hdhomerun_debug_message_t *message)
{
	return TRUE;
}
#else
static bool_t hdhomerun_debug_output_message_sock(struct hdhomerun_debug_t *dbg, struct hdhomerun_debug_message_t *message)
{
	if (dbg->sock == -1) {
		uint64_t current_time = getcurrenttime();
		if (current_time < dbg->connect_delay) {
			return FALSE;
		}
		dbg->connect_delay = current_time + 30*1000;

		dbg->sock = (int)socket(AF_INET, SOCK_STREAM, 0);
		if (dbg->sock == -1) {
			return FALSE;
		}

		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		struct addrinfo *sock_info;
		if (getaddrinfo(HDHOMERUN_DEBUG_HOST, HDHOMERUN_DEBUG_PORT, &hints, &sock_info) != 0) {
			hdhomerun_debug_close_sock(dbg);
			return FALSE;
		}
		if (connect(dbg->sock, sock_info->ai_addr, (int)sock_info->ai_addrlen) != 0) {
			freeaddrinfo(sock_info);
			hdhomerun_debug_close_sock(dbg);
			return FALSE;
		}
		freeaddrinfo(sock_info);
	}

	size_t length = strlen(message->buffer);
	if (send(dbg->sock, (char *)message->buffer, (int)length, 0) != length) {
		hdhomerun_debug_close_sock(dbg);
		return FALSE;
	}

	return TRUE;
}
#endif

static bool_t hdhomerun_debug_output_message(struct hdhomerun_debug_t *dbg, struct hdhomerun_debug_message_t *message)
{
	pthread_mutex_lock(&dbg->send_lock);

	if (!dbg->enabled) {
		pthread_mutex_unlock(&dbg->send_lock);
		return TRUE;
	}

	bool_t ret;
	if (dbg->file_name) {
		ret = hdhomerun_debug_output_message_file(dbg, message);
	} else {
		ret = hdhomerun_debug_output_message_sock(dbg, message);
	}

	pthread_mutex_unlock(&dbg->send_lock);
	return ret;
}

static void hdhomerun_debug_pop_and_free_message(struct hdhomerun_debug_t *dbg)
{
	pthread_mutex_lock(&dbg->queue_lock);

	struct hdhomerun_debug_message_t *message = dbg->queue_tail;
	dbg->queue_tail = message->prev;
	if (message->prev) {
		message->prev->next = NULL;
	} else {
		dbg->queue_head = NULL;
	}
	dbg->queue_depth--;

	pthread_mutex_unlock(&dbg->queue_lock);

	free(message);
}

static THREAD_FUNC_PREFIX hdhomerun_debug_thread_execute(void *arg)
{
	struct hdhomerun_debug_t *dbg = (struct hdhomerun_debug_t *)arg;

	while (!dbg->terminate) {

		pthread_mutex_lock(&dbg->queue_lock);
		struct hdhomerun_debug_message_t *message = dbg->queue_tail;
		uint32_t queue_depth = dbg->queue_depth;
		pthread_mutex_unlock(&dbg->queue_lock);

		if (!message) {
			msleep(250);
			continue;
		}

		if (queue_depth > 256) {
			hdhomerun_debug_pop_and_free_message(dbg);
			continue;
		}

		if (!hdhomerun_debug_output_message(dbg, message)) {
			msleep(250);
			continue;
		}

		hdhomerun_debug_pop_and_free_message(dbg);
	}

	return 0;
}
