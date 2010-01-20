/*
 * hdhomerun_control.c
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

#define HDHOMERUN_CONTROL_SEND_TIMEOUT 5000
#define HDHOMERUN_CONTROL_RECV_TIMEOUT 5000
#define HDHOMERUN_CONTROL_UPGRADE_TIMEOUT 20000

struct hdhomerun_control_sock_t {
	uint32_t desired_device_id;
	uint32_t desired_device_ip;
	uint32_t actual_device_id;
	uint32_t actual_device_ip;
	int sock;
	struct hdhomerun_debug_t *dbg;
	struct hdhomerun_pkt_t tx_pkt;
	struct hdhomerun_pkt_t rx_pkt;
};

static void hdhomerun_control_close_sock(struct hdhomerun_control_sock_t *cs)
{
	if (cs->sock == -1) {
		return;
	}

	close(cs->sock);
	cs->sock = -1;
}

void hdhomerun_control_set_device(struct hdhomerun_control_sock_t *cs, uint32_t device_id, uint32_t device_ip)
{
	hdhomerun_control_close_sock(cs);

	cs->desired_device_id = device_id;
	cs->desired_device_ip = device_ip;
	cs->actual_device_id = 0;
	cs->actual_device_ip = 0;
}

struct hdhomerun_control_sock_t *hdhomerun_control_create(uint32_t device_id, uint32_t device_ip, struct hdhomerun_debug_t *dbg)
{
	struct hdhomerun_control_sock_t *cs = (struct hdhomerun_control_sock_t *)calloc(1, sizeof(struct hdhomerun_control_sock_t));
	if (!cs) {
		hdhomerun_debug_printf(dbg, "hdhomerun_control_create: failed to allocate control object\n");
		return NULL;
	}

	cs->dbg = dbg;
	cs->sock = -1;
	hdhomerun_control_set_device(cs, device_id, device_ip);

	return cs;
}

void hdhomerun_control_destroy(struct hdhomerun_control_sock_t *cs)
{
	hdhomerun_control_close_sock(cs);
	free(cs);
}

static bool_t hdhomerun_control_connect_sock(struct hdhomerun_control_sock_t *cs)
{
	if (cs->sock != -1) {
		return TRUE;
	}

	if ((cs->desired_device_id == 0) && (cs->desired_device_ip == 0)) {
		hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_connect_sock: no device specified\n");
		return FALSE;
	}

	/* Find device. */
	struct hdhomerun_discover_device_t result;
	if (hdhomerun_discover_find_devices_custom(cs->desired_device_ip, HDHOMERUN_DEVICE_TYPE_WILDCARD, cs->desired_device_id, &result, 1) <= 0) {
		hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_connect_sock: device not found\n");
		return FALSE;
	}
	cs->actual_device_ip = result.ip_addr;
	cs->actual_device_id = result.device_id;

	/* Create socket. */
	cs->sock = (int)socket(AF_INET, SOCK_STREAM, 0);
	if (cs->sock == -1) {
		hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_connect_sock: failed to create socket (%d)\n", sock_getlasterror);
		return FALSE;
	}

	/* Set timeouts. */
	setsocktimeout(cs->sock, SOL_SOCKET, SO_SNDTIMEO, HDHOMERUN_CONTROL_SEND_TIMEOUT);
	setsocktimeout(cs->sock, SOL_SOCKET, SO_RCVTIMEO, HDHOMERUN_CONTROL_RECV_TIMEOUT);

	/* Initiate connection. */
	struct sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = htonl(cs->actual_device_ip);
	sock_addr.sin_port = htons(HDHOMERUN_CONTROL_TCP_PORT);
	if (connect(cs->sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) != 0) {
		hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_connect_sock: failed to connect (%d)\n", sock_getlasterror);
		hdhomerun_control_close_sock(cs);
		return FALSE;
	}

	/* Success. */
	return TRUE;
}

uint32_t hdhomerun_control_get_device_id(struct hdhomerun_control_sock_t *cs)
{
	if (!hdhomerun_control_connect_sock(cs)) {
		hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_get_device_id: connect failed\n");
		return 0;
	}

	return cs->actual_device_id;
}

uint32_t hdhomerun_control_get_device_ip(struct hdhomerun_control_sock_t *cs)
{
	if (!hdhomerun_control_connect_sock(cs)) {
		hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_get_device_ip: connect failed\n");
		return 0;
	}

	return cs->actual_device_ip;
}

uint32_t hdhomerun_control_get_device_id_requested(struct hdhomerun_control_sock_t *cs)
{
	return cs->desired_device_id;
}

uint32_t hdhomerun_control_get_device_ip_requested(struct hdhomerun_control_sock_t *cs)
{
	return cs->desired_device_ip;
}

uint32_t hdhomerun_control_get_local_addr(struct hdhomerun_control_sock_t *cs)
{
	if (!hdhomerun_control_connect_sock(cs)) {
		hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_get_local_addr: connect failed\n");
		return 0;
	}

	struct sockaddr_in sock_addr;
	socklen_t sockaddr_size = sizeof(sock_addr);
	if (getsockname(cs->sock, (struct sockaddr*)&sock_addr, &sockaddr_size) != 0) {
		hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_get_local_addr: getsockname failed (%d)\n", sock_getlasterror);
		return 0;
	}

	return ntohl(sock_addr.sin_addr.s_addr);
}

static int hdhomerun_control_send_sock(struct hdhomerun_control_sock_t *cs, struct hdhomerun_pkt_t *tx_pkt)
{
	int length = (int)(tx_pkt->end - tx_pkt->start);
	if (send(cs->sock, (char *)tx_pkt->start, (int)length, 0) != length) {
		hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_send_sock: send failed (%d)\n", sock_getlasterror);
		hdhomerun_control_close_sock(cs);
		return -1;
	}

	return 1;
}

static int hdhomerun_control_recv_sock(struct hdhomerun_control_sock_t *cs, struct hdhomerun_pkt_t *rx_pkt, uint16_t *ptype, uint64_t recv_timeout)
{
	uint64_t stop_time = getcurrenttime() + recv_timeout;
	hdhomerun_pkt_reset(rx_pkt);

	while (getcurrenttime() < stop_time) {
		struct timeval t;
		t.tv_sec = 0;
		t.tv_usec = 250000;
	
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(cs->sock, &readfds);
	
		if (select(cs->sock+1, &readfds, NULL, NULL, &t) < 0) {
			hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_recv_sock: select failed (%d)\n", sock_getlasterror);
			hdhomerun_control_close_sock(cs);
			return -1;
		}
	
		if (!FD_ISSET(cs->sock, &readfds)) {
			continue;
		}
	
		int rx_length = recv(cs->sock, (char *)rx_pkt->end, (int)(rx_pkt->limit - rx_pkt->end), 0);
		if (rx_length <= 0) {
			hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_recv_sock: recv failed (%d)\n", sock_getlasterror);
			hdhomerun_control_close_sock(cs);
			return -1;
		}
		rx_pkt->end += rx_length;

		int ret = hdhomerun_pkt_open_frame(rx_pkt, ptype);
		if (ret < 0) {
			hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_recv_sock: frame error\n");
			hdhomerun_control_close_sock(cs);
			return -1;
		}
		if (ret == 0) {
			continue;
		}

		return 1;
	}

	hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_recv_sock: timeout\n");
	hdhomerun_control_close_sock(cs);
	return -1;
}

static int hdhomerun_control_send_recv_internal(struct hdhomerun_control_sock_t *cs, struct hdhomerun_pkt_t *tx_pkt, struct hdhomerun_pkt_t *rx_pkt, uint16_t type, uint64_t recv_timeout)
{
	hdhomerun_pkt_seal_frame(tx_pkt, type);

	int i;
	for (i = 0; i < 2; i++) {
		if (cs->sock == -1) {
			if (!hdhomerun_control_connect_sock(cs)) {
				hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_send_recv: connect failed\n");
				return -1;
			}
		}

		if (hdhomerun_control_send_sock(cs, tx_pkt) < 0) {
			continue;
		}
		if (!rx_pkt) {
			return 1;
		}

		uint16_t rsp_type;
		if (hdhomerun_control_recv_sock(cs, rx_pkt, &rsp_type, recv_timeout) < 0) {
			continue;
		}
		if (rsp_type != type + 1) {
			hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_send_recv: unexpected frame type\n");
			hdhomerun_control_close_sock(cs);
			continue;
		}

		return 1;
	}

	hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_send_recv: failed\n");
	return -1;
}

int hdhomerun_control_send_recv(struct hdhomerun_control_sock_t *cs, struct hdhomerun_pkt_t *tx_pkt, struct hdhomerun_pkt_t *rx_pkt, uint16_t type)
{
	return hdhomerun_control_send_recv_internal(cs, tx_pkt, rx_pkt, type, HDHOMERUN_CONTROL_RECV_TIMEOUT);
}

static int hdhomerun_control_get_set(struct hdhomerun_control_sock_t *cs, const char *name, const char *value, uint32_t lockkey, char **pvalue, char **perror)
{
	struct hdhomerun_pkt_t *tx_pkt = &cs->tx_pkt;
	struct hdhomerun_pkt_t *rx_pkt = &cs->rx_pkt;

	/* Request. */
	hdhomerun_pkt_reset(tx_pkt);

	int name_len = (int)strlen(name) + 1;
	if (tx_pkt->end + 3 + name_len > tx_pkt->limit) {
		hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_get_set: request too long\n");
		return -1;
	}
	hdhomerun_pkt_write_u8(tx_pkt, HDHOMERUN_TAG_GETSET_NAME);
	hdhomerun_pkt_write_var_length(tx_pkt, name_len);
	hdhomerun_pkt_write_mem(tx_pkt, (void *)name, name_len);

	if (value) {
		int value_len = (int)strlen(value) + 1;
		if (tx_pkt->end + 3 + value_len > tx_pkt->limit) {
			hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_get_set: request too long\n");
			return -1;
		}
		hdhomerun_pkt_write_u8(tx_pkt, HDHOMERUN_TAG_GETSET_VALUE);
		hdhomerun_pkt_write_var_length(tx_pkt, value_len);
		hdhomerun_pkt_write_mem(tx_pkt, (void *)value, value_len);
	}

	if (lockkey != 0) {
		if (tx_pkt->end + 6 > tx_pkt->limit) {
			hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_get_set: request too long\n");
			return -1;
		}
		hdhomerun_pkt_write_u8(tx_pkt, HDHOMERUN_TAG_GETSET_LOCKKEY);
		hdhomerun_pkt_write_var_length(tx_pkt, 4);
		hdhomerun_pkt_write_u32(tx_pkt, lockkey);
	}

	/* Send/Recv. */
	if (hdhomerun_control_send_recv_internal(cs, tx_pkt, rx_pkt, HDHOMERUN_TYPE_GETSET_REQ, HDHOMERUN_CONTROL_RECV_TIMEOUT) < 0) {
		hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_get_set: send/recv error\n");
		return -1;
	}

	/* Response. */
	while (1) {
		uint8_t tag;
		size_t len;
		uint8_t *next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
		if (!next) {
			break;
		}

		switch (tag) {
		case HDHOMERUN_TAG_GETSET_VALUE:
			if (pvalue) {
				*pvalue = (char *)rx_pkt->pos;
				rx_pkt->pos[len] = 0;
			}
			if (perror) {
				*perror = NULL;
			}
			return 1;

		case HDHOMERUN_TAG_ERROR_MESSAGE:
			rx_pkt->pos[len] = 0;
			hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_get_set: %s\n", rx_pkt->pos);

			if (pvalue) {
				*pvalue = NULL;
			}
			if (perror) {
				*perror = (char *)rx_pkt->pos;
			}

			return 0;
		}

		rx_pkt->pos = next;
	}

	hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_get_set: missing response tags\n");
	return -1;
}

int hdhomerun_control_get(struct hdhomerun_control_sock_t *cs, const char *name, char **pvalue, char **perror)
{
	return hdhomerun_control_get_set(cs, name, NULL, 0, pvalue, perror);
}

int hdhomerun_control_set(struct hdhomerun_control_sock_t *cs, const char *name, const char *value, char **pvalue, char **perror)
{
	return hdhomerun_control_get_set(cs, name, value, 0, pvalue, perror);
}

int hdhomerun_control_set_with_lockkey(struct hdhomerun_control_sock_t *cs, const char *name, const char *value, uint32_t lockkey, char **pvalue, char **perror)
{
	return hdhomerun_control_get_set(cs, name, value, lockkey, pvalue, perror);
}

int hdhomerun_control_upgrade(struct hdhomerun_control_sock_t *cs, FILE *upgrade_file)
{
	struct hdhomerun_pkt_t *tx_pkt = &cs->tx_pkt;
	struct hdhomerun_pkt_t *rx_pkt = &cs->rx_pkt;
	uint32_t sequence = 0;

	/* Upload. */
	while (1) {
		uint8_t data[256];
		size_t length = fread(data, 1, 256, upgrade_file);
		if (length == 0) {
			break;
		}

		hdhomerun_pkt_reset(tx_pkt);
		hdhomerun_pkt_write_u32(tx_pkt, sequence);
		hdhomerun_pkt_write_mem(tx_pkt, data, length);

		if (hdhomerun_control_send_recv_internal(cs, tx_pkt, NULL, HDHOMERUN_TYPE_UPGRADE_REQ, 0) < 0) {
			hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_upgrade: send/recv failed\n");
			return -1;
		}

		sequence += (uint32_t)length;
	}

	if (sequence == 0) {
		/* No data in file. Error, but no need to close connection. */
		hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_upgrade: zero length file\n");
		return 0;
	}

	/* Execute upgrade. */
	hdhomerun_pkt_reset(tx_pkt);
	hdhomerun_pkt_write_u32(tx_pkt, 0xFFFFFFFF);

	if (hdhomerun_control_send_recv_internal(cs, tx_pkt, rx_pkt, HDHOMERUN_TYPE_UPGRADE_REQ, HDHOMERUN_CONTROL_UPGRADE_TIMEOUT) < 0) {
		hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_upgrade: send/recv failed\n");
		return -1;
	}

	/* Check response. */
	while (1) {
		uint8_t tag;
		size_t len;
		uint8_t *next = hdhomerun_pkt_read_tlv(rx_pkt, &tag, &len);
		if (!next) {
			break;
		}

		switch (tag) {
		case HDHOMERUN_TAG_ERROR_MESSAGE:
			rx_pkt->pos[len] = 0;
			hdhomerun_debug_printf(cs->dbg, "hdhomerun_control_upgrade: %s\n", (char *)rx_pkt->pos);
			return 0;

		default:
			break;
		}

		rx_pkt->pos = next;
	}

	return 1;
}
