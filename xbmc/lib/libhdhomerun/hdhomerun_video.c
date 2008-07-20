/*
 * hdhomerun_video.c
 *
 * Copyright © 2006 Silicondust Engineering Ltd. <www.silicondust.com>.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "hdhomerun_os.h"
#include "hdhomerun_pkt.h"
#include "hdhomerun_video.h"

#ifndef __WINDOWS__
#include <pthread.h>
#endif

struct hdhomerun_video_sock_t {
	uint8_t *buffer;
	size_t buffer_size;
	volatile size_t head;
	volatile size_t tail;
	size_t advance;
	volatile bool_t running;
	volatile bool_t terminate;
#ifdef __WINDOWS__
  HANDLE thread;
#else
  pthread_t thread;
#endif
	int sock;
};

#ifdef __WINDOWS__
static DWORD WINAPI hdhomerun_video_thread(void *arg);
#else
static void *hdhomerun_video_thread(void *arg);
#endif

static bool_t hdhomerun_video_bind_sock_internal(struct hdhomerun_video_sock_t *vs, uint16_t listen_port)
{
	struct sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_addr.sin_port = htons(listen_port);
	if (bind(vs->sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) != 0) {
		return FALSE;
	}
	return TRUE;
}

static bool_t hdhomerun_video_bind_sock(struct hdhomerun_video_sock_t *vs, uint16_t listen_port)
{
	if (listen_port != 0) {
		return hdhomerun_video_bind_sock_internal(vs, listen_port);
	}

#if defined(__CYGWIN__) || defined(__WINDOWS__)
	/* Windows firewall silently blocks a listening port if the port number is not explicitly given. */
	/* Workaround - pick a random port number. The port may already be in use to try multiple port numbers. */
	srand((int)getcurrenttime());
	int retry;
	for (retry = 8; retry > 0; retry--) {
		uint16_t listen_port = (uint16_t)((rand() % 32768) + 32768);
		if (hdhomerun_video_bind_sock_internal(vs, listen_port)) {
			return TRUE;
		}
	}
	return FALSE;
#else
	return hdhomerun_video_bind_sock_internal(vs, listen_port);
#endif
}

struct hdhomerun_video_sock_t *hdhomerun_video_create(uint16_t listen_port, size_t buffer_size)
{
	/* Create object. */
	struct hdhomerun_video_sock_t *vs = (struct hdhomerun_video_sock_t *)calloc(1, sizeof(struct hdhomerun_video_sock_t));
	if (!vs) {
		return NULL;
	}

	/* Buffer size. */
	vs->buffer_size = (buffer_size / VIDEO_DATA_PACKET_SIZE) * VIDEO_DATA_PACKET_SIZE;
	if (vs->buffer_size == 0) {
		free(vs);
		return NULL;
	}
	vs->buffer_size += VIDEO_DATA_PACKET_SIZE;

	/* Create buffer. */
	vs->buffer = (uint8_t *)malloc(vs->buffer_size);
	if (!vs->buffer) {
		free(vs);
		return NULL;
	}
	
	/* Create socket. */
	vs->sock = (int)socket(AF_INET, SOCK_DGRAM, 0);
	if (vs->sock == -1) {
		free(vs->buffer);
		free(vs);
		return NULL;
	}

	/* Expand socket buffer size. */
	int rx_size = 1024 * 1024;
	setsockopt(vs->sock, SOL_SOCKET, SO_RCVBUF, (char *)&rx_size, sizeof(rx_size));

	/* Set timeouts. */
	setsocktimeout(vs->sock, SOL_SOCKET, SO_SNDTIMEO, 1000);
	setsocktimeout(vs->sock, SOL_SOCKET, SO_RCVTIMEO, 1000);

	/* Bind socket. */
	if (!hdhomerun_video_bind_sock(vs, listen_port)) {
		hdhomerun_video_destroy(vs);
		return NULL;
	}

	/* Start thread. */
#ifdef __WINDOWS__
  if ((vs->thread = CreateThread(NULL, 0, &hdhomerun_video_thread, vs, 0, NULL)) == NULL) {
#else
  if (pthread_create(&vs->thread, NULL, &hdhomerun_video_thread, vs) != 0) {
#endif
		hdhomerun_video_destroy(vs);
		return NULL;
	}
	vs->running = 1;

	/* Success. */
	return vs;
}

void hdhomerun_video_destroy(struct hdhomerun_video_sock_t *vs)
{
	if (vs->running) {
		vs->terminate = 1;
#ifdef __WINDOWS__
    WaitForSingleObject( vs->thread, INFINITE );
    CloseHandle( vs->thread );
#else
		pthread_join(vs->thread, NULL);
#endif
	}
	close(vs->sock);
	free(vs->buffer);
	free(vs);
}

uint16_t hdhomerun_video_get_local_port(struct hdhomerun_video_sock_t *vs)
{
	struct sockaddr_in sock_addr;
	socklen_t sockaddr_size = sizeof(sock_addr);
	if (getsockname(vs->sock, (struct sockaddr*)&sock_addr, &sockaddr_size) != 0) {
		return 0;
	}
	return ntohs(sock_addr.sin_port);
}

int hdhomerun_video_get_sock(struct hdhomerun_video_sock_t *vs)
{
	return vs->sock;
}
#ifdef __WINDOWS__
static DWORD WINAPI hdhomerun_video_thread(void *arg)
#else
static void *hdhomerun_video_thread(void *arg)
#endif
{
	struct hdhomerun_video_sock_t *vs = (struct hdhomerun_video_sock_t *)arg;

	while (!vs->terminate) {
		size_t head = vs->head;

		/* Receive. */
		int length = recv(vs->sock, (char *)vs->buffer + head, VIDEO_DATA_PACKET_SIZE, 0);
		if (length != VIDEO_DATA_PACKET_SIZE) {
			if (length > 0) {
				/* Data received but not valid - ignore. */
				continue;
			}
			if (sock_getlasterror_socktimeout) {
				/* Wait for more data. */
				continue;
			}
			vs->terminate = 1;
			return NULL;
		}

		/* Calculate new head. */
		head += length;
		if (head >= vs->buffer_size) {
			head -= vs->buffer_size;
		}

		/* Check for buffer overflow. */
		if (head == vs->tail) {
			continue;
		}

		/* Atomic update. */
		vs->head = head;
	}

	return NULL;
}

uint8_t *hdhomerun_video_recv(struct hdhomerun_video_sock_t *vs, size_t max_size, size_t *pactual_size)
{
	size_t head = vs->head;
	size_t tail = vs->tail;

	if (vs->advance > 0) {
		tail += vs->advance;
		if (tail >= vs->buffer_size) {
			tail -= vs->buffer_size;
		}
	
		/* Atomic update. */
		vs->tail = tail;
	}

	if (head == tail) {
		vs->advance = 0;
		*pactual_size = 0;
		return NULL;
	}

	size_t size = (max_size / VIDEO_DATA_PACKET_SIZE) * VIDEO_DATA_PACKET_SIZE;
	if (size == 0) {
		vs->advance = 0;
		*pactual_size = 0;
		return NULL;
	}

	size_t avail;
	if (head > tail) {
		avail = head - tail;
	} else {
		avail = vs->buffer_size - tail;
	}
	if (size > avail) {
		size = avail;
	}
	vs->advance = size;
	*pactual_size = size;
	return vs->buffer + tail;
}

void hdhomerun_video_flush(struct hdhomerun_video_sock_t *vs)
{
	/* Atomic update of tail. */
	vs->tail = vs->head;
	vs->advance = 0;
}

