/*
 * hdhomerun_video.c
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

#include "hdhomerun.h"

struct hdhomerun_video_sock_t {
	pthread_mutex_t lock;
	uint8_t *buffer;
	size_t buffer_size;
	volatile size_t head;
	volatile size_t tail;
	size_t advance;
	volatile bool_t terminate;
	pthread_t thread;
	int sock;
	uint32_t rtp_sequence;
	struct hdhomerun_debug_t *dbg;
	volatile uint32_t packet_count;
	volatile uint32_t transport_error_count;
	volatile uint32_t network_error_count;
	volatile uint32_t sequence_error_count;
	volatile uint32_t overflow_error_count;
	volatile uint8_t sequence[0x2000];
};

static THREAD_FUNC_PREFIX hdhomerun_video_thread_execute(void *arg);

struct hdhomerun_video_sock_t *hdhomerun_video_create(uint16_t listen_port, size_t buffer_size, struct hdhomerun_debug_t *dbg)
{
	/* Create object. */
	struct hdhomerun_video_sock_t *vs = (struct hdhomerun_video_sock_t *)calloc(1, sizeof(struct hdhomerun_video_sock_t));
	if (!vs) {
		hdhomerun_debug_printf(dbg, "hdhomerun_video_create: failed to allocate video object\n");
		return NULL;
	}

	vs->dbg = dbg;
	vs->sock = -1;
	pthread_mutex_init(&vs->lock, NULL);

	/* Reset sequence tracking. */
	hdhomerun_video_flush(vs);

	/* Buffer size. */
	vs->buffer_size = (buffer_size / VIDEO_DATA_PACKET_SIZE) * VIDEO_DATA_PACKET_SIZE;
	if (vs->buffer_size == 0) {
		hdhomerun_debug_printf(dbg, "hdhomerun_video_create: invalid buffer size (%lu bytes)\n", (unsigned long)buffer_size);
		goto error;
	}
	vs->buffer_size += VIDEO_DATA_PACKET_SIZE;

	/* Create buffer. */
	vs->buffer = (uint8_t *)malloc(vs->buffer_size);
	if (!vs->buffer) {
		hdhomerun_debug_printf(dbg, "hdhomerun_video_create: failed to allocate buffer (%lu bytes)\n", (unsigned long)vs->buffer_size);
		goto error;
	}
	
	/* Create socket. */
	vs->sock = (int)socket(AF_INET, SOCK_DGRAM, 0);
	if (vs->sock == -1) {
		hdhomerun_debug_printf(dbg, "hdhomerun_video_create: failed to allocate socket\n");
		goto error;
	}

	/* Expand socket buffer size. */
	int rx_size = 1024 * 1024;
	setsockopt(vs->sock, SOL_SOCKET, SO_RCVBUF, (char *)&rx_size, sizeof(rx_size));

	/* Set timeouts. */
	setsocktimeout(vs->sock, SOL_SOCKET, SO_SNDTIMEO, 1000);
	setsocktimeout(vs->sock, SOL_SOCKET, SO_RCVTIMEO, 1000);

	/* Bind socket. */
	struct sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_addr.sin_port = htons(listen_port);
	if (bind(vs->sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) != 0) {
		hdhomerun_debug_printf(dbg, "hdhomerun_video_create: failed to bind socket (port %u)\n", listen_port);
		goto error;
	}

	/* Start thread. */
	if (pthread_create(&vs->thread, NULL, &hdhomerun_video_thread_execute, vs) != 0) {
		hdhomerun_debug_printf(dbg, "hdhomerun_video_create: failed to start thread\n");
		goto error;
	}

	/* Success. */
	return vs;

error:
	if (vs->sock != -1) {
		close(vs->sock);
	}
	if (vs->buffer) {
		free(vs->buffer);
	}
	free(vs);
	return NULL;
}

void hdhomerun_video_destroy(struct hdhomerun_video_sock_t *vs)
{
	vs->terminate = TRUE;
	pthread_join(vs->thread, NULL);

	close(vs->sock);
	free(vs->buffer);

	free(vs);
}

uint16_t hdhomerun_video_get_local_port(struct hdhomerun_video_sock_t *vs)
{
	struct sockaddr_in sock_addr;
	socklen_t sockaddr_size = sizeof(sock_addr);
	if (getsockname(vs->sock, (struct sockaddr*)&sock_addr, &sockaddr_size) != 0) {
		hdhomerun_debug_printf(vs->dbg, "hdhomerun_video_get_local_port: getsockname failed (%d)\n", sock_getlasterror);
		return 0;
	}

	return ntohs(sock_addr.sin_port);
}

static void hdhomerun_video_stats_ts_pkt(struct hdhomerun_video_sock_t *vs, uint8_t *ptr)
{
	uint16_t packet_identifier = ((uint16_t)(ptr[1] & 0x1F) << 8) | (uint16_t)ptr[2];
	if (packet_identifier == 0x1FFF) {
		return;
	}

	bool_t transport_error = ptr[1] >> 7;
	if (transport_error) {
		vs->transport_error_count++;
		vs->sequence[packet_identifier] = 0xFF;
		return;
	}

	uint8_t continuity_counter = ptr[3] & 0x0F;
	uint8_t previous_sequence = vs->sequence[packet_identifier];

	if (continuity_counter == ((previous_sequence + 1) & 0x0F)) {
		vs->sequence[packet_identifier] = continuity_counter;
		return;
	}
	if (previous_sequence == 0xFF) {
		vs->sequence[packet_identifier] = continuity_counter;
		return;
	}
	if (continuity_counter == previous_sequence) {
		return;
	}

	vs->sequence_error_count++;
	vs->sequence[packet_identifier] = continuity_counter;
}

static void hdhomerun_video_parse_rtp(struct hdhomerun_video_sock_t *vs, struct hdhomerun_pkt_t *pkt)
{
	pkt->pos += 2;
	uint32_t rtp_sequence = hdhomerun_pkt_read_u16(pkt);
	pkt->pos += 8;

	if (rtp_sequence != ((vs->rtp_sequence + 1) & 0xFFFF)) {
		if (vs->rtp_sequence != 0xFFFFFFFF) {
			vs->network_error_count++;

			/* restart pid sequence check */
			memset((void *)vs->sequence, 0xFF, sizeof(vs->sequence));
		}
	}

	vs->rtp_sequence = rtp_sequence;
}

static THREAD_FUNC_PREFIX hdhomerun_video_thread_execute(void *arg)
{
	struct hdhomerun_video_sock_t *vs = (struct hdhomerun_video_sock_t *)arg;
	struct hdhomerun_pkt_t pkt_inst;

	while (!vs->terminate) {
		struct hdhomerun_pkt_t *pkt = &pkt_inst;
		hdhomerun_pkt_reset(pkt);

		/* Receive. */
		int length = recv(vs->sock, (char *)pkt->end, VIDEO_RTP_DATA_PACKET_SIZE, 0);
		pkt->end += length;

		if (length == VIDEO_RTP_DATA_PACKET_SIZE) {
			hdhomerun_video_parse_rtp(vs, pkt);
			length = (int)(pkt->end - pkt->pos);
		}

		if (length != VIDEO_DATA_PACKET_SIZE) {
			if (length > 0) {
				/* Data received but not valid - ignore. */
				continue;
			}
			if (sock_getlasterror_socktimeout) {
				/* Wait for more data. */
				continue;
			}
			vs->terminate = TRUE;
			return NULL;
		}

		pthread_mutex_lock(&vs->lock);

		/* Store in ring buffer. */
		size_t head = vs->head;
		uint8_t *ptr = vs->buffer + head;
		memcpy(ptr, pkt->pos, length);

		/* Stats. */
		vs->packet_count++;
		hdhomerun_video_stats_ts_pkt(vs, ptr + TS_PACKET_SIZE * 0);
		hdhomerun_video_stats_ts_pkt(vs, ptr + TS_PACKET_SIZE * 1);
		hdhomerun_video_stats_ts_pkt(vs, ptr + TS_PACKET_SIZE * 2);
		hdhomerun_video_stats_ts_pkt(vs, ptr + TS_PACKET_SIZE * 3);
		hdhomerun_video_stats_ts_pkt(vs, ptr + TS_PACKET_SIZE * 4);
		hdhomerun_video_stats_ts_pkt(vs, ptr + TS_PACKET_SIZE * 5);
		hdhomerun_video_stats_ts_pkt(vs, ptr + TS_PACKET_SIZE * 6);

		/* Calculate new head. */
		head += length;
		if (head >= vs->buffer_size) {
			head -= vs->buffer_size;
		}

		/* Check for buffer overflow. */
		if (head == vs->tail) {
			vs->overflow_error_count++;
			pthread_mutex_unlock(&vs->lock);
			continue;
		}

		/* Atomic update. */
		vs->head = head;

		pthread_mutex_unlock(&vs->lock);
	}

	return NULL;
}

uint8_t *hdhomerun_video_recv(struct hdhomerun_video_sock_t *vs, size_t max_size, size_t *pactual_size)
{
	pthread_mutex_lock(&vs->lock);

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
		pthread_mutex_unlock(&vs->lock);
		return NULL;
	}

	size_t size = (max_size / VIDEO_DATA_PACKET_SIZE) * VIDEO_DATA_PACKET_SIZE;
	if (size == 0) {
		vs->advance = 0;
		*pactual_size = 0;
		pthread_mutex_unlock(&vs->lock);
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
	uint8_t *result = vs->buffer + tail;

	pthread_mutex_unlock(&vs->lock);
	return result;
}

void hdhomerun_video_flush(struct hdhomerun_video_sock_t *vs)
{
	pthread_mutex_lock(&vs->lock);

	vs->tail = vs->head;
	vs->advance = 0;

	memset((void *)vs->sequence, 0xFF, sizeof(vs->sequence));

	vs->rtp_sequence = 0xFFFFFFFF;

	vs->packet_count = 0;
	vs->transport_error_count = 0;
	vs->network_error_count = 0;
	vs->sequence_error_count = 0;
	vs->overflow_error_count = 0;

	pthread_mutex_unlock(&vs->lock);
}

void hdhomerun_video_debug_print_stats(struct hdhomerun_video_sock_t *vs)
{
	struct hdhomerun_video_stats_t stats;
	hdhomerun_video_get_stats(vs, &stats);

	hdhomerun_debug_printf(vs->dbg, "video sock: pkt=%ld net=%ld te=%ld miss=%ld drop=%ld\n",
		stats.packet_count, stats.network_error_count,
		stats.transport_error_count, stats.sequence_error_count,
		stats.overflow_error_count
	);
}

void hdhomerun_video_get_stats(struct hdhomerun_video_sock_t *vs, struct hdhomerun_video_stats_t *stats)
{
	memset(stats, 0, sizeof(struct hdhomerun_video_stats_t));

	pthread_mutex_lock(&vs->lock);

	stats->packet_count = vs->packet_count;
	stats->network_error_count = vs->network_error_count;
	stats->transport_error_count = vs->transport_error_count;
	stats->sequence_error_count = vs->sequence_error_count;
	stats->overflow_error_count = vs->overflow_error_count;

	pthread_mutex_unlock(&vs->lock);
}
