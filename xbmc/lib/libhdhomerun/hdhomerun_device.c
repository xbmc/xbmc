/*
 * hdhomerun_record.c
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
#include "hdhomerun_control.h"
#include "hdhomerun_video.h"
#include "hdhomerun_device.h"

struct hdhomerun_device_t {
	struct hdhomerun_control_sock_t *cs;
	struct hdhomerun_video_sock_t *vs;
	unsigned int tuner;
	char result_buffer[1024];
};

struct hdhomerun_device_t *hdhomerun_device_create(uint32_t device_id, uint32_t device_ip, unsigned int tuner)
{
	if (!hdhomerun_discover_validate_device_id(device_id)) {
		return NULL;
	}

	struct hdhomerun_device_t *hd = (struct hdhomerun_device_t *)calloc(1, sizeof(struct hdhomerun_device_t));
	if (!hd) {
		return NULL;
	}

	hd->tuner = tuner;

	hd->cs = hdhomerun_control_create(device_id, device_ip);
	if (!hd->cs) {
		free(hd);
		return NULL;
	}

	return hd;
}

struct hdhomerun_device_t *hdhomerun_device_create_from_str(const char *device_str)
{
	unsigned long a[4];
	if (sscanf(device_str, "%lu.%lu.%lu.%lu", &a[0], &a[1], &a[2], &a[3]) == 4) {
		unsigned long device_ip = (a[0] << 24) | (a[1] << 16) | (a[2] << 8) | (a[3] << 0);
		return hdhomerun_device_create(HDHOMERUN_DEVICE_ID_WILDCARD, (uint32_t)device_ip, 0);
	}

	unsigned long device_id;
	unsigned int tuner;
	if (sscanf(device_str, "%lx:%u", &device_id, &tuner) == 2) {
		return hdhomerun_device_create((uint32_t)device_id, 0, tuner);
	}
	if (sscanf(device_str, "%lx-%u", &device_id, &tuner) == 2) {
		return hdhomerun_device_create((uint32_t)device_id, 0, tuner);
	}
	if (sscanf(device_str, "%lx", &device_id) == 1) {
		return hdhomerun_device_create((uint32_t)device_id, 0, 0);
	}

	return NULL;
}

void hdhomerun_device_destroy(struct hdhomerun_device_t *hd)
{
	if (hd->vs) {
		hdhomerun_video_destroy(hd->vs);
	}

	hdhomerun_control_destroy(hd->cs);

	free(hd);
}

void hdhomerun_device_set_tuner(struct hdhomerun_device_t *hd, unsigned int tuner)
{
	hd->tuner = tuner;
}

int hdhomerun_device_set_tuner_from_str(struct hdhomerun_device_t *hd, const char *tuner_str)
{
	unsigned int tuner;
	if (sscanf(tuner_str, "%u", &tuner) == 1) {
		hd->tuner = tuner;
		return 1;
	}
	if (sscanf(tuner_str, "/tuner%u", &tuner) == 1) {
		hd->tuner = tuner;
		return 1;
	}

	return -1;
}

uint32_t hdhomerun_device_get_device_id(struct hdhomerun_device_t *hd)
{
	return hdhomerun_control_get_device_id(hd->cs);
}

uint32_t hdhomerun_device_get_device_ip(struct hdhomerun_device_t *hd)
{
	return hdhomerun_control_get_device_ip(hd->cs);
}

unsigned int hdhomerun_device_get_tuner(struct hdhomerun_device_t *hd)
{
	return hd->tuner;
}

struct hdhomerun_control_sock_t *hdhomerun_device_get_control_sock(struct hdhomerun_device_t *hd)
{
	return hd->cs;
}

struct hdhomerun_video_sock_t *hdhomerun_device_get_video_sock(struct hdhomerun_device_t *hd)
{
	if (!hd->vs) {
		hd->vs = hdhomerun_video_create(0, VIDEO_DATA_BUFFER_SIZE_1S);
	}
	return hd->vs;
}

uint32_t hdhomerun_device_get_local_machine_addr(struct hdhomerun_device_t *hd)
{
	return hdhomerun_control_get_local_addr(hd->cs);
}

static uint32_t hdhomerun_device_get_status_parse(const char *status_str, const char *tag)
{
	const char *ptr = strstr(status_str, tag);
	if (!ptr) {
		return 0;
	}

	unsigned long value = 0;
	sscanf(ptr + strlen(tag), "%lu", &value);

	return (uint32_t)value;
}

int hdhomerun_device_get_tuner_status(struct hdhomerun_device_t *hd, struct hdhomerun_tuner_status_t *status)
{
	memset(status, 0, sizeof(struct hdhomerun_tuner_status_t));

	char name[32];
	sprintf(name, "/tuner%u/status", hd->tuner);

	char *status_str;
	int ret = hdhomerun_control_get(hd->cs, name, &status_str, NULL);
	if (ret <= 0) {
		return ret;
	}

	char *channel = strstr(status_str, "ch=");
	if (channel) {
		sscanf(channel + 3, "%31s", status->channel);
	}

	char *lock = strstr(status_str, "lock=");
	if (lock) {
		sscanf(lock + 5, "%31s", status->lock_str);
	}

	status->signal_strength = (unsigned int)hdhomerun_device_get_status_parse(status_str, "ss=");
	status->signal_to_noise_quality = (unsigned int)hdhomerun_device_get_status_parse(status_str, "snq=");
	status->symbol_error_quality = (unsigned int)hdhomerun_device_get_status_parse(status_str, "seq=");
	status->raw_bits_per_second = hdhomerun_device_get_status_parse(status_str, "bps=");
	status->packets_per_second = hdhomerun_device_get_status_parse(status_str, "pps=");

	status->signal_present = status->signal_strength >= 25;

	if (strcmp(status->lock_str, "none") != 0) {
		if (status->lock_str[0] == '(') {
			status->lock_unsupported = TRUE;
		} else {
			status->lock_supported = TRUE;
		}
	}

	return 1;
}

int hdhomerun_device_get_tuner_streaminfo(struct hdhomerun_device_t *hd, char **pstreaminfo)
{
	char name[32];
	sprintf(name, "/tuner%u/streaminfo", hd->tuner);
	return hdhomerun_control_get(hd->cs, name, pstreaminfo, NULL);
}

int hdhomerun_device_get_tuner_channel(struct hdhomerun_device_t *hd, char **pchannel)
{
	char name[32];
	sprintf(name, "/tuner%u/channel", hd->tuner);
	return hdhomerun_control_get(hd->cs, name, pchannel, NULL);
}

int hdhomerun_device_get_tuner_channelmap(struct hdhomerun_device_t *hd, char **pchannelmap)
{
	char name[32];
	sprintf(name, "/tuner%u/channelmap", hd->tuner);
	return hdhomerun_control_get(hd->cs, name, pchannelmap, NULL);
}

int hdhomerun_device_get_tuner_filter(struct hdhomerun_device_t *hd, char **pfilter)
{
	char name[32];
	sprintf(name, "/tuner%u/filter", hd->tuner);
	return hdhomerun_control_get(hd->cs, name, pfilter, NULL);
}

int hdhomerun_device_get_tuner_program(struct hdhomerun_device_t *hd, char **pprogram)
{
	char name[32];
	sprintf(name, "/tuner%u/program", hd->tuner);
	return hdhomerun_control_get(hd->cs, name, pprogram, NULL);
}

int hdhomerun_device_get_tuner_target(struct hdhomerun_device_t *hd, char **ptarget)
{
	char name[32];
	sprintf(name, "/tuner%u/target", hd->tuner);
	return hdhomerun_control_get(hd->cs, name, ptarget, NULL);
}

int hdhomerun_device_get_ir_target(struct hdhomerun_device_t *hd, char **ptarget)
{
	return hdhomerun_control_get(hd->cs, "/ir/target", ptarget, NULL);
}

int hdhomerun_device_get_version(struct hdhomerun_device_t *hd, char **pversion_str, uint32_t *pversion_num)
{
	char *version_str;
	int ret = hdhomerun_control_get(hd->cs, "/sys/version", &version_str, NULL);
	if (ret <= 0) {
		return ret;
	}

	if (pversion_str) {
		*pversion_str = version_str;
	}

	if (pversion_num) {
		unsigned long version_num;
		if (sscanf(version_str, "%lu", &version_num) != 1) {
			*pversion_num = 0;
		} else {
			*pversion_num = (uint32_t)version_num;
		}
	}

	return 1;
}

int hdhomerun_device_set_tuner_channel(struct hdhomerun_device_t *hd, const char *channel)
{
	char name[32];
	sprintf(name, "/tuner%u/channel", hd->tuner);
	return hdhomerun_control_set(hd->cs, name, channel, NULL, NULL);
}

int hdhomerun_device_set_tuner_channelmap(struct hdhomerun_device_t *hd, const char *channelmap)
{
	char name[32];
	sprintf(name, "/tuner%u/channelmap", hd->tuner);
	return hdhomerun_control_set(hd->cs, name, channelmap, NULL, NULL);
}

int hdhomerun_device_set_tuner_filter(struct hdhomerun_device_t *hd, const char *filter)
{
	char name[32];
	sprintf(name, "/tuner%u/filter", hd->tuner);
	return hdhomerun_control_set(hd->cs, name, filter, NULL, NULL);
}

int hdhomerun_device_set_tuner_program(struct hdhomerun_device_t *hd, const char *program)
{
	char name[32];
	sprintf(name, "/tuner%u/program", hd->tuner);
	return hdhomerun_control_set(hd->cs, name, program, NULL, NULL);
}

int hdhomerun_device_set_tuner_target(struct hdhomerun_device_t *hd, char *target)
{
	char name[32];
	sprintf(name, "/tuner%u/target", hd->tuner);
	return hdhomerun_control_set(hd->cs, name, target, NULL, NULL);
}

int hdhomerun_device_set_tuner_target_to_local(struct hdhomerun_device_t *hd)
{
	/* Create video socket. */
	hdhomerun_device_get_video_sock(hd);
	if (!hd->vs) {
		return -1;
	}

	/* Set target. */
	char target[64];
	uint32_t local_ip = hdhomerun_control_get_local_addr(hd->cs);
	uint16_t local_port = hdhomerun_video_get_local_port(hd->vs);
	sprintf(target, "%u.%u.%u.%u:%u",
		(unsigned int)(local_ip >> 24) & 0xFF, (unsigned int)(local_ip >> 16) & 0xFF,
		(unsigned int)(local_ip >> 8) & 0xFF, (unsigned int)(local_ip >> 0) & 0xFF,
		(unsigned int)local_port
	);

	return hdhomerun_device_set_tuner_target(hd, target);
}

int hdhomerun_device_set_ir_target(struct hdhomerun_device_t *hd, const char *target)
{
	return hdhomerun_control_set(hd->cs, "/ir/target", target, NULL, NULL);
}

int hdhomerun_device_get_var(struct hdhomerun_device_t *hd, const char *name, char **pvalue, char **perror)
{
	return hdhomerun_control_get(hd->cs, name, pvalue, perror);
}

int hdhomerun_device_set_var(struct hdhomerun_device_t *hd, const char *name, const char *value, char **pvalue, char **perror)
{
	return hdhomerun_control_set(hd->cs, name, value, pvalue, perror);
}

int hdhomerun_device_wait_for_lock(struct hdhomerun_device_t *hd, struct hdhomerun_tuner_status_t *status)
{
	/* Wait for up to 1.5 seconds for lock. */
	int i;
	for (i = 0; i < 6; i++) {
		usleep(250000);

		/* Get status to check for lock. Quality numbers will not be valid yet. */
		int ret = hdhomerun_device_get_tuner_status(hd, status);
		if (ret <= 0) {
			return ret;
		}

		if (!status->signal_present) {
			return 1;
		}
		if (status->lock_supported || status->lock_unsupported) {
			return 1;
		}
	}

	return 1;
}

int hdhomerun_device_stream_start(struct hdhomerun_device_t *hd)
{
	/* Set target. */
	int ret = hdhomerun_device_set_tuner_target_to_local(hd);
	if (ret <= 0) {
		return ret;
	}

	/* Flush video buffer. */
	usleep(64000);
	hdhomerun_video_flush(hd->vs);

	/* Success. */
	return 1;
}

uint8_t *hdhomerun_device_stream_recv(struct hdhomerun_device_t *hd, size_t max_size, size_t *pactual_size)
{
	if (!hd->vs) {
		return NULL;
	}
	return hdhomerun_video_recv(hd->vs, max_size, pactual_size);
}

void hdhomerun_device_stream_flush(struct hdhomerun_device_t *hd)
{
	hdhomerun_video_flush(hd->vs);
}

void hdhomerun_device_stream_stop(struct hdhomerun_device_t *hd)
{
	hdhomerun_device_set_tuner_target(hd, "none");
}

int hdhomerun_device_firmware_version_check(struct hdhomerun_device_t *hd, uint32_t features)
{
	uint32_t version;
	if (hdhomerun_device_get_version(hd, NULL, &version) <= 0) {
		return -1;
	}

	if (version < 20070131) {
		return 0;
	}

	return 1;
}

int hdhomerun_device_upgrade(struct hdhomerun_device_t *hd, FILE *upgrade_file)
{
	hdhomerun_control_set(hd->cs, "/tuner0/channel", "none", NULL, NULL);
	hdhomerun_control_set(hd->cs, "/tuner1/channel", "none", NULL, NULL);
	return hdhomerun_control_upgrade(hd->cs, upgrade_file);
}
