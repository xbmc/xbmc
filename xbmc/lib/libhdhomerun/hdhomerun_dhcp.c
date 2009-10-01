/*
 * hdhomerun_dhcp.c
 *
 * Copyright © 2006-2007 Silicondust Engineering Ltd. <www.silicondust.com>.
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
#include "hdhomerun_dhcp.h"

struct dhcp_hdr_t {
	uint8_t bootp_message_type;
	uint8_t hardware_type;
	uint8_t hardware_address_length;
	uint8_t hops;
	uint32_t transaction_id;
	uint16_t seconds_elapsed;
	uint16_t bootp_flags;
	uint32_t client_ip;
	uint32_t your_ip;
	uint32_t next_server_ip;
	uint32_t relay_agent_ip;
	uint8_t client_mac[16];
	uint8_t server_host_name[64];
	uint8_t boot_file_name[128];
	uint32_t magic_cookie;
};

struct hdhomerun_dhcp_t {
	int sock;
	uint32_t local_address;
	pthread_t thread;
	volatile bool_t terminate;
};

static THREAD_FUNC_PREFIX hdhomerun_dhcp_thread_execute(void *arg);

struct hdhomerun_dhcp_t *hdhomerun_dhcp_create(uint32_t bind_address)
{
	if (bind_address != 0) {
		if ((bind_address & 0xFFFF0000) != 0xA9FE0000) {
			return NULL;
		}
	}

	/* Create socket. */
	int sock = (int)socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		return NULL;
	}

	/* Set timeout. */
	setsocktimeout(sock, SOL_SOCKET, SO_RCVTIMEO, 1000);

	/* Allow broadcast. */
	int sock_opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&sock_opt, sizeof(sock_opt));

	/* Allow reuse. */
	sock_opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt, sizeof(sock_opt));

	/* Bind socket. */
	struct sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = htonl(bind_address);
	sock_addr.sin_port = htons(67);
	if (bind(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) != 0) {
		close(sock);
		return NULL;
	}

	/* Allocate object. */
	struct hdhomerun_dhcp_t *dhcp = (struct hdhomerun_dhcp_t *)calloc(1, sizeof(struct hdhomerun_dhcp_t));
	if (!dhcp) {
		close(sock);
		return NULL;
	}

	dhcp->sock = sock;

	if (bind_address != 0) {
		dhcp->local_address = bind_address;
	} else {
		dhcp->local_address = 0xA9FEFFFF;
	}

	/* Spawn thread. */
	if (pthread_create(&dhcp->thread, NULL, &hdhomerun_dhcp_thread_execute, dhcp) != 0) {
		close(sock);
		free(dhcp);
		return NULL;
	}

	/* Success. */
	return dhcp;
}

void hdhomerun_dhcp_destroy(struct hdhomerun_dhcp_t *dhcp)
{
	dhcp->terminate = TRUE;
	pthread_join(dhcp->thread, NULL);

	close(dhcp->sock);
	free(dhcp);
}

static void hdhomerun_dhcp_send(struct hdhomerun_dhcp_t *dhcp, uint8_t message_type, struct hdhomerun_pkt_t *pkt)
{
	pkt->pos = pkt->start;
	struct dhcp_hdr_t *hdr = (struct dhcp_hdr_t *)pkt->pos;
	pkt->pos += sizeof(struct dhcp_hdr_t);
	pkt->end = pkt->pos;

	uint32_t remote_addr = 0xA9FE0000;
	remote_addr |= (uint32_t)hdr->client_mac[4] << 8;
	remote_addr |= (uint32_t)hdr->client_mac[5] << 0;
	if ((remote_addr == 0xA9FE0000) || (remote_addr == 0xA9FEFFFF)) {
		remote_addr = 0xA9FE8080;
	}

	hdr->bootp_message_type = 0x02;
	hdr->your_ip = htonl(remote_addr);
	hdr->next_server_ip = htonl(0x00000000);

	hdhomerun_pkt_write_u8(pkt, 53);
	hdhomerun_pkt_write_u8(pkt, 1);
	hdhomerun_pkt_write_u8(pkt, message_type);

	hdhomerun_pkt_write_u8(pkt, 54);
	hdhomerun_pkt_write_u8(pkt, 4);
	hdhomerun_pkt_write_u32(pkt, dhcp->local_address);

	hdhomerun_pkt_write_u8(pkt, 51);
	hdhomerun_pkt_write_u8(pkt, 4);
	hdhomerun_pkt_write_u32(pkt, 7*24*60*60);

	hdhomerun_pkt_write_u8(pkt, 1);
	hdhomerun_pkt_write_u8(pkt, 4);
	hdhomerun_pkt_write_u32(pkt, 0xFFFF0000);

	hdhomerun_pkt_write_u8(pkt, 0xFF);

	while (pkt->pos < pkt->start + 300) {
		hdhomerun_pkt_write_u8(pkt, 0x00);
	}

	struct sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = htonl(0xFFFFFFFF);
	sock_addr.sin_port = htons(68);

	sendto(dhcp->sock, (char *)pkt->start, (int)(pkt->end - pkt->start), 0, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
}

static void hdhomerun_dhcp_recv(struct hdhomerun_dhcp_t *dhcp, struct hdhomerun_pkt_t *pkt)
{
	pkt->pos = pkt->start;
	struct dhcp_hdr_t *hdr = (struct dhcp_hdr_t *)pkt->pos;
	pkt->pos += sizeof(struct dhcp_hdr_t);
	if (pkt->pos > pkt->end) {
		return;
	}

	if (ntohl(hdr->magic_cookie) != 0x63825363) {
		return;
	}

	static uint8_t vendor[3] = {0x00, 0x18, 0xDD};
	if (memcmp(hdr->client_mac, vendor, 3) != 0) {
		return;
	}

	if (pkt->pos + 3 > pkt->end) {
		return;
	}
	if (hdhomerun_pkt_read_u8(pkt) != 53) {
		return;
	}
	if (hdhomerun_pkt_read_u8(pkt) != 1) {
		return;
	}
	uint8_t message_type_val = hdhomerun_pkt_read_u8(pkt);

	switch (message_type_val) {
	case 0x01:
		hdhomerun_dhcp_send(dhcp, 0x02, pkt);
		break;
	case 0x03:
		hdhomerun_dhcp_send(dhcp, 0x05, pkt);
		break;
	default:
		return;
	}
}

static THREAD_FUNC_PREFIX hdhomerun_dhcp_thread_execute(void *arg)
{
	struct hdhomerun_dhcp_t *dhcp = (struct hdhomerun_dhcp_t *)arg;
	struct hdhomerun_pkt_t pkt_inst;

	while (1) {
		if (dhcp->terminate) {
			return NULL;
		}

		struct hdhomerun_pkt_t *pkt = &pkt_inst;
		hdhomerun_pkt_reset(pkt);

		int rx_length = recv(dhcp->sock, (char *)pkt->end, (int)(pkt->limit - pkt->end), 0);
		if (rx_length <= 0) {
			if (!sock_getlasterror_socktimeout) {
				sleep(1);
			}
			continue;
		}
		pkt->end += rx_length;

		hdhomerun_dhcp_recv(dhcp, pkt);
	}
}
