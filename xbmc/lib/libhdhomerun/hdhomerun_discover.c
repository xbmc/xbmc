/*
 * hdhomerun_discover.c
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
#include "hdhomerun_discover.h"

#if defined(__CYGWIN__) || defined(__WINDOWS__)
#include <windows.h>
#include <iptypes.h>
#include <iphlpapi.h>
#endif

#if defined(__APPLE__)
#include <ifaddrs.h>
#endif

struct hdhomerun_discover_sock_t {
	int sock;
};

static struct hdhomerun_discover_sock_t *hdhomerun_discover_create(void)
{
	struct hdhomerun_discover_sock_t *ds = (struct hdhomerun_discover_sock_t *)malloc(sizeof(struct hdhomerun_discover_sock_t));
	if (!ds) {
		return NULL;
	}
	
	/* Create socket. */
	ds->sock = (int)socket(AF_INET, SOCK_DGRAM, 0);
	if (ds->sock == -1) {
		free(ds);
		return NULL;
	}

	/* Set timeouts. */
	setsocktimeout(ds->sock, SOL_SOCKET, SO_SNDTIMEO, 1000);
	setsocktimeout(ds->sock, SOL_SOCKET, SO_RCVTIMEO, 1000);

	/* Allow broadcast. */
	int sock_opt = 1;
	setsockopt(ds->sock, SOL_SOCKET, SO_BROADCAST, (char *)&sock_opt, sizeof(sock_opt));

	/* Bind socket. */
	struct sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_addr.sin_port = htons(0);
	if (bind(ds->sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) != 0) {
		close(ds->sock);
		free(ds);
		return NULL;
	}

	/* Success. */
	return ds;
}

static void hdhomerun_discover_destroy(struct hdhomerun_discover_sock_t *ds)
{
	close(ds->sock);
	free(ds);
}

static int hdhomerun_discover_send_packet(struct hdhomerun_discover_sock_t *ds, uint32_t ip_addr, uint32_t device_type, uint32_t device_id)
{
	uint8_t buffer[1024];
	uint8_t *ptr = buffer;
	hdhomerun_write_discover_request(&ptr, device_type, device_id);

	struct sockaddr_in sock_addr;
	memset(&sock_addr, 0, sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = htonl(ip_addr);
	sock_addr.sin_port = htons(HDHOMERUN_DISCOVER_UDP_PORT);

	int length = (int)(ptr - buffer);
	if (sendto(ds->sock, (char *)buffer, (int)length, 0, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) != length) {
		return -1;
	}

	return 0;
}
#if defined(__XBOX__)

static int hdhomerun_discover_send_internal(struct hdhomerun_discover_sock_t *ds, uint32_t device_type, uint32_t device_id)
{
	uint32_t broadcast = 0xFFFFFFFF;
	if (hdhomerun_discover_send_packet(ds, broadcast, device_type, device_id) < 0) {
    return -1;
	}
	return 0;
}

#elif defined(__CYGWIN__) || defined(__WINDOWS__)

static int hdhomerun_discover_send_internal(struct hdhomerun_discover_sock_t *ds, uint32_t device_type, uint32_t device_id)
{
	PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));
	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

	DWORD Ret = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
	if (Ret != NO_ERROR) {
		free(pAdapterInfo);
		if (Ret != ERROR_BUFFER_OVERFLOW) {
			return -1;
		}
		pAdapterInfo = (IP_ADAPTER_INFO *)malloc(ulOutBufLen); 
		Ret = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
		if (Ret != NO_ERROR) {
			free(pAdapterInfo);
			return -1;
		}
	}

	unsigned int send_count = 0;
	PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
	while (pAdapter) {
		IP_ADDR_STRING *pIPAddr = &pAdapter->IpAddressList;
		while (pIPAddr) {
			uint32_t addr = ntohl(inet_addr(pIPAddr->IpAddress.String));
			uint32_t mask = ntohl(inet_addr(pIPAddr->IpMask.String));
			
			uint32_t broadcast = addr | ~mask;
			if ((broadcast == 0x00000000) || (broadcast == 0xFFFFFFFF)) {
				pIPAddr = pIPAddr->Next;
				continue;
			}

			if (hdhomerun_discover_send_packet(ds, broadcast, device_type, device_id) < 0) {
				pIPAddr = pIPAddr->Next;
				continue;
			}

			send_count++;

			pIPAddr = pIPAddr->Next;
		}

		pAdapter = pAdapter->Next;
	}

	free(pAdapterInfo);

	if (send_count == 0) {
		return -1;
	}

	return 0;
}

#elif defined(__linux__)

static int hdhomerun_discover_send_internal(struct hdhomerun_discover_sock_t *ds, uint32_t device_type, uint32_t device_id)
{
	FILE *fp = fopen("/proc/net/route", "r");
	if (!fp) {
		return -1;
	}

	unsigned int send_count = 0;
	while (1) {
		char line[256];
		if (!fgets(line, sizeof(line), fp)) {
			break;
		}
		line[255] = 0;

		uint32_t dest;
		uint32_t mask;
		if (sscanf(line, "%*s %x %*x %*x %*d %*d %*d %x", &dest, &mask) != 2) {
			continue;
		}
		dest = ntohl(dest);
		mask = ntohl(mask);
		
		uint32_t broadcast = dest | ~mask;

		if ((broadcast == 0x00000000) || (broadcast == 0xFFFFFFFF)) {
			continue;
		}

		if (hdhomerun_discover_send_packet(ds, broadcast, device_type, device_id) < 0) {
			continue;
		}

		send_count++;
	}

	fclose(fp);

	if (send_count == 0) {
		return -1;
	}

	return 0;
}

#elif defined(__APPLE__)

static int hdhomerun_discover_send_internal(struct hdhomerun_discover_sock_t *ds, uint32_t device_type, uint32_t device_id)
{
	struct ifaddrs *ifap;
	if (getifaddrs(&ifap) < 0) {
		return -1;
	}

	struct ifaddrs *p = ifap;
	unsigned int send_count = 0;
	while (p) {
		struct sockaddr_in *addr_in = (struct sockaddr_in *)p->ifa_addr;
		struct sockaddr_in *mask_in = (struct sockaddr_in *)p->ifa_netmask;
		if (!addr_in || !mask_in) {
			p = p->ifa_next;
			continue;
		}

		uint32_t addr = ntohl(addr_in->sin_addr.s_addr);
		uint32_t mask = ntohl(mask_in->sin_addr.s_addr);

		uint32_t broadcast = addr | ~mask;

		if ((broadcast == 0x00000000) || (broadcast == 0xFFFFFFFF)) {
			p = p->ifa_next;
			continue;
		}

		if (hdhomerun_discover_send_packet(ds, broadcast, device_type, device_id) < 0) {
			p = p->ifa_next;
			continue;
		}

		send_count++;
		p = p->ifa_next;
	}

	freeifaddrs(ifap);

	if (send_count == 0) {
		return -1;
	}

	return 0;
}

#else

static int hdhomerun_discover_send_internal(struct hdhomerun_discover_sock_t *ds, uint32_t device_type, uint32_t device_id)
{
	return -1;
}

#endif

static int hdhomerun_discover_send(struct hdhomerun_discover_sock_t *ds, uint32_t target_ip, uint32_t device_type, uint32_t device_id)
{
	if (target_ip != 0) {
		return hdhomerun_discover_send_packet(ds, target_ip, device_type, device_id);
	}

	if (hdhomerun_discover_send_internal(ds, device_type, device_id) < 0) {
		return hdhomerun_discover_send_packet(ds, 0xFFFFFFFF, device_type, device_id);
	}

	return 0;
}

static int hdhomerun_discover_recv(struct hdhomerun_discover_sock_t *ds, struct hdhomerun_discover_device_t *result)
{
	struct timeval t;
	t.tv_sec = 0;
	t.tv_usec = 250000;

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(ds->sock, &readfds);

	if (select(ds->sock+1, &readfds, NULL, NULL, &t) < 0) {
		return -1;
	}
	if (!FD_ISSET(ds->sock, &readfds)) {
		return 0;
	}

	uint8_t buffer[1024];
	struct sockaddr_in sock_addr;
	socklen_t sockaddr_size = sizeof(sock_addr);
	int rx_length = recvfrom(ds->sock, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&sock_addr, &sockaddr_size);
	if (rx_length <= 0) {
		/* Don't return error - windows machine on VPN can sometimes cause a sock error here but otherwise works. */
		return 0;
	}
	if (rx_length < HDHOMERUN_MIN_PEEK_LENGTH) {
		return 0;
	}

	size_t length = hdhomerun_peek_packet_length(buffer);
	if (length > (size_t)rx_length) {
		return 0;
	}

	uint8_t *ptr = buffer;
	uint8_t *end = buffer + length;
	int type = hdhomerun_process_packet(&ptr, &end);
	if (type != HDHOMERUN_TYPE_DISCOVER_RPY) {
		return 0;
	}

	result->ip_addr = ntohl(sock_addr.sin_addr.s_addr);
	result->device_type = 0;
	result->device_id = 0;
	while (1) {
		uint8_t tag;
		size_t len;
		uint8_t *value;
		if (hdhomerun_read_tlv(&ptr, end, &tag, &len, &value) < 0) {
			break;
		}

		switch (tag) {
		case HDHOMERUN_TAG_DEVICE_TYPE:
			if (len != 4) {
				break;
			}
			result->device_type = hdhomerun_read_u32(&value);
			break;
		case HDHOMERUN_TAG_DEVICE_ID:
			if (len != 4) {
				break;
			}
			result->device_id = hdhomerun_read_u32(&value);
			break;
		default:
			break;
		}
	}

	return 1;
}

static struct hdhomerun_discover_device_t *hdhomerun_discover_find_in_list(struct hdhomerun_discover_device_t result_list[], int count, uint32_t device_id)
{
	int index;
	for (index = 0; index < count; index++) {
		struct hdhomerun_discover_device_t *result = &result_list[index];
		if (result->device_id == device_id) {
			return result;
		}
	}

	return NULL;
}

static int hdhomerun_discover_find_devices_internal(struct hdhomerun_discover_sock_t *ds, uint32_t target_ip, uint32_t device_type, uint32_t device_id, struct hdhomerun_discover_device_t result_list[], int max_count)
{
	int count = 0;

	int attempt;
	for (attempt = 0; attempt < 4; attempt++) {
		if (hdhomerun_discover_send(ds, target_ip, device_type, device_id) < 0) {
			return -1;
		}

		uint64_t timeout = getcurrenttime() + 250;
		while (getcurrenttime() < timeout) {
			struct hdhomerun_discover_device_t *result = &result_list[count];

			int ret = hdhomerun_discover_recv(ds, result);
			if (ret < 0) {
				return -1;
			}
			if (ret == 0) {
				break;
			}

			/* Filter. */
			if (device_type != HDHOMERUN_DEVICE_TYPE_WILDCARD) {
				if (device_type != result->device_type) {
					continue;
				}
			}
			if (device_id != HDHOMERUN_DEVICE_ID_WILDCARD) {
				if (device_id != result->device_id) {
					continue;
				}
			}

			/* Ensure not already in list. */
			if (hdhomerun_discover_find_in_list(result_list, count, result->device_id)) {
				continue;
			}

			/* Add to list. */
			count++;
			if (count >= max_count) {
				return count;
			}
		}
	}

	return count;
}

int hdhomerun_discover_find_device(uint32_t device_id, struct hdhomerun_discover_device_t *result)
{
	struct hdhomerun_discover_sock_t *ds = hdhomerun_discover_create();
	if (!ds) {
		return -1;
	}

	int ret = hdhomerun_discover_find_devices_internal(ds, 0, HDHOMERUN_DEVICE_TYPE_WILDCARD, device_id, result, 1);

	hdhomerun_discover_destroy(ds);
	return ret;
}

int hdhomerun_discover_find_devices(uint32_t device_type, struct hdhomerun_discover_device_t result_list[], int max_count)
{
	struct hdhomerun_discover_sock_t *ds = hdhomerun_discover_create();
	if (!ds) {
		return -1;
	}

	int ret = hdhomerun_discover_find_devices_internal(ds, 0, device_type, HDHOMERUN_DEVICE_ID_WILDCARD, result_list, max_count);

	hdhomerun_discover_destroy(ds);
	return ret;
}

int hdhomerun_discover_find_devices_custom(uint32_t target_ip, uint32_t device_type, uint32_t device_id, struct hdhomerun_discover_device_t result_list[], int max_count)
{
	struct hdhomerun_discover_sock_t *ds = hdhomerun_discover_create();
	if (!ds) {
		return -1;
	}

	int ret = hdhomerun_discover_find_devices_internal(ds, target_ip, device_type, device_id, result_list, max_count);

	hdhomerun_discover_destroy(ds);
	return ret;
}

bool_t hdhomerun_discover_validate_device_id(uint32_t device_id)
{
	static uint32_t lookup_table[16] = {0xA, 0x5, 0xF, 0x6, 0x7, 0xC, 0x1, 0xB, 0x9, 0x2, 0x8, 0xD, 0x4, 0x3, 0xE, 0x0};

	uint32_t checksum = 0;

	checksum ^= lookup_table[(device_id >> 28) & 0x0F];
	checksum ^= (device_id >> 24) & 0x0F;
	checksum ^= lookup_table[(device_id >> 20) & 0x0F];
	checksum ^= (device_id >> 16) & 0x0F;
	checksum ^= lookup_table[(device_id >> 12) & 0x0F];
	checksum ^= (device_id >> 8) & 0x0F;
	checksum ^= lookup_table[(device_id >> 4) & 0x0F];
	checksum ^= (device_id >> 0) & 0x0F;

	return (checksum == 0);
}

