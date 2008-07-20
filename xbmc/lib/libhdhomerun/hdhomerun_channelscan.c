/*
 * hdhomerun_channelscan.c
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
#include "hdhomerun_control.h"
#include "hdhomerun_device.h"
#include "hdhomerun_channelscan.h"

#define CHANNEL_MAP_US_BCAST 1
#define CHANNEL_MAP_US_CABLE 2
#define CHANNEL_MAP_US_HRC 3
#define CHANNEL_MAP_US_IRC 4

struct channelscan_entry_t {
	struct channelscan_entry_t *next;
	uint8_t channel_map;
	uint8_t channel;
	uint32_t frequency;
};

struct channelscan_map_range_t {
	uint8_t channel_map;
	uint8_t channel_range_start;
	uint8_t channel_range_end;
	uint32_t frequency;
	uint32_t spacing;
};

static const struct channelscan_map_range_t channelscan_map_ranges[] = {
	{CHANNEL_MAP_US_BCAST,   2,   4,  57000000, 6000000},
	{CHANNEL_MAP_US_BCAST,   5,   6,  79000000, 6000000},
	{CHANNEL_MAP_US_BCAST,   7,  13, 177000000, 6000000},
	{CHANNEL_MAP_US_BCAST,  14,  69, 473000000, 6000000},
	{CHANNEL_MAP_US_BCAST,  70,  83, 809000000, 6000000},

	{CHANNEL_MAP_US_CABLE,   1,   1,  75000000, 6000000},
	{CHANNEL_MAP_US_CABLE,   2,   4,  57000000, 6000000},
	{CHANNEL_MAP_US_CABLE,   5,   6,  79000000, 6000000},
	{CHANNEL_MAP_US_CABLE,   7,  13, 177000000, 6000000},
	{CHANNEL_MAP_US_CABLE,  14,  22, 123000000, 6000000},
	{CHANNEL_MAP_US_CABLE,  23,  94, 219000000, 6000000},
	{CHANNEL_MAP_US_CABLE,  95,  99,  93000000, 6000000},
	{CHANNEL_MAP_US_CABLE, 100, 159, 651000000, 6000000},

	{CHANNEL_MAP_US_HRC,     1,   1,  73753600, 6000300},
	{CHANNEL_MAP_US_HRC,     2,   4,  55752700, 6000300},
	{CHANNEL_MAP_US_HRC,     5,   6,  79753900, 6000300},
	{CHANNEL_MAP_US_HRC,     7,  13, 175758700, 6000300},
	{CHANNEL_MAP_US_HRC,    14,  22, 121756000, 6000300},
	{CHANNEL_MAP_US_HRC,    23,  94, 217760800, 6000300},
	{CHANNEL_MAP_US_HRC,    95,  99,  91754500, 6000300},
	{CHANNEL_MAP_US_HRC,   100, 159, 649782400, 6000300},

	{CHANNEL_MAP_US_IRC,     1,   1,  75012500, 6000000},
	{CHANNEL_MAP_US_IRC,     2,   4,  57012500, 6000000},
	{CHANNEL_MAP_US_IRC,     5,   6,  81012500, 6000000},
	{CHANNEL_MAP_US_IRC,     7,  13, 177012500, 6000000},
	{CHANNEL_MAP_US_IRC,    14,  22, 123012500, 6000000},
	{CHANNEL_MAP_US_IRC,    23,  41, 219012500, 6000000},
	{CHANNEL_MAP_US_IRC,    42,  42, 333025000, 6000000},
	{CHANNEL_MAP_US_IRC,    43,  94, 339012500, 6000000},
	{CHANNEL_MAP_US_IRC,    95,  97,  93012500, 6000000},
	{CHANNEL_MAP_US_IRC,    98,  99, 111025000, 6000000},
	{CHANNEL_MAP_US_IRC,   100, 159, 651012500, 6000000},

	{0,                      0,   0,         0,       0}
};

static const char *channelscan_map_name(uint8_t channel_map)
{
	switch (channel_map) {
	case CHANNEL_MAP_US_BCAST:
		return "us-bcast";
	case CHANNEL_MAP_US_CABLE:
		return "us-cable";
	case CHANNEL_MAP_US_HRC:
		return "us-hrc";
	case CHANNEL_MAP_US_IRC:
		return "us-irc";
	default:
		return "unknown";
	}
}

static void channelscan_list_build_insert(struct channelscan_entry_t **list, struct channelscan_entry_t *entry)
{
	struct channelscan_entry_t **pprev = list;
	struct channelscan_entry_t *p = *list;

	while (p) {
		if (p->frequency > entry->frequency) {
			break;
		}

		pprev = &p->next;
		p = p->next;
	}

	entry->next = p;
	*pprev = entry;
}

static void channelscan_list_build_range(struct channelscan_entry_t **list, const struct channelscan_map_range_t *range)
{
	uint8_t channel;
	for (channel = range->channel_range_start; channel <= range->channel_range_end; channel++) {
		struct channelscan_entry_t *entry = (struct channelscan_entry_t *)calloc(1, sizeof(struct channelscan_entry_t));
		if (!entry) {
			return;
		}

		entry->channel_map = range->channel_map;
		entry->channel = channel;
		entry->frequency = range->frequency + ((uint32_t)(channel - range->channel_range_start) * range->spacing);
		entry->frequency = (entry->frequency / 62500) * 62500;

		channelscan_list_build_insert(list, entry);
	}
}

static void channelscan_list_build(struct channelscan_entry_t **list)
{
	*list = NULL;

	const struct channelscan_map_range_t *range = channelscan_map_ranges;
	while (range->channel_map) {
		channelscan_list_build_range(list, range);
		range++;
	}
}

static void channelscan_list_free(struct channelscan_entry_t **list)
{
	while (1) {
		struct channelscan_entry_t *entry = *list;
		if (!entry) {
			break;
		}

		*list = entry->next;
		free(entry);
	}
}

static int channelscan_execute_find_lock_internal(struct hdhomerun_device_t *hd, uint32_t frequency, struct hdhomerun_tuner_status_t *status)
{
	char channel_str[64];

	/* Set 8vsb channel. */
	sprintf(channel_str, "8vsb:%ld", (unsigned long)frequency);
	int ret = hdhomerun_device_set_tuner_channel(hd, channel_str);
	if (ret <= 0) {
		return ret;
	}

	/* Wait for lock. */
	ret = hdhomerun_device_wait_for_lock(hd, status);
	if (ret <= 0) {
		return ret;
	}
	if (status->lock_supported || status->lock_unsupported) {
		return 1;
	}

	/* Set qam channel. */
	sprintf(channel_str, "qam:%ld", (unsigned long)frequency);
	ret = hdhomerun_device_set_tuner_channel(hd, channel_str);
	if (ret <= 0) {
		return ret;
	}

	/* Wait for lock. */
	ret = hdhomerun_device_wait_for_lock(hd, status);
	if (ret <= 0) {
		return ret;
	}
	if (status->lock_supported || status->lock_unsupported) {
		return 1;
	}

	return 1;
}

static int channelscan_execute_find_lock(struct hdhomerun_device_t *hd, uint32_t frequency, struct hdhomerun_tuner_status_t *status)
{
	int ret = channelscan_execute_find_lock_internal(hd, frequency, status);
	if (ret <= 0) {
		return ret;
	}

	if (!status->lock_supported) {
		return 1;
	}

	int i;
	for (i = 0; i < 5 * 4; i++) {
		usleep(250000);

		ret = hdhomerun_device_get_tuner_status(hd, status);
		if (ret <= 0) {
			return ret;
		}

		if (status->symbol_error_quality == 100) {
			break;
		}
	}

	return 1;
}

static int channelscan_execute_find_programs(struct hdhomerun_device_t *hd, char **pstreaminfo)
{
	*pstreaminfo = NULL;

	char *streaminfo;
	int ret = hdhomerun_device_get_tuner_streaminfo(hd, &streaminfo);
	if (ret <= 0) {
		return ret;
	}

	char *last_streaminfo = strdup(streaminfo);
	if (!last_streaminfo) {
		return -1;
	}

	int same = 0;
	int i;
	for (i = 0; i < 5 * 4; i++) {
		usleep(250000);

		ret = hdhomerun_device_get_tuner_streaminfo(hd, &streaminfo);
		if (ret <= 0) {
			free(last_streaminfo);
			return ret;
		}

		if (strcmp(streaminfo, last_streaminfo) != 0) {
			free(last_streaminfo);
			last_streaminfo = strdup(streaminfo);
			if (!last_streaminfo) {
				return -1;
			}
			same = 0;
			continue;
		}

		same++;
		if (same >= 4 - 1) {
			break;
		}
	}

	*pstreaminfo = last_streaminfo;
	return 1;
}

static int channelscan_execute_callback(channelscan_callback_t callback, va_list callback_ap, const char *type, const char *str)
{
	if (!callback) {
		return 1;
	}
	
	va_list ap;
	va_copy(ap, callback_ap);
	int ret = callback(ap, type, str);
	va_end(ap);

	return ret;
}

static int channelscan_execute_internal(struct hdhomerun_device_t *hd, struct channelscan_entry_t **pentry, channelscan_callback_t callback, va_list callback_ap)
{
	struct channelscan_entry_t *entry = *pentry;
	uint32_t frequency = entry->frequency;
	char buffer[256];
	int ret;

	/* Combine channels with same frequency. */
	char *ptr = buffer;
	sprintf(ptr, "%ld (", (unsigned long)frequency);
	ptr = strchr(ptr, 0);
	while (1) {
		sprintf(ptr, "%s:%d", channelscan_map_name(entry->channel_map), entry->channel);
		ptr = strchr(ptr, 0);

		entry = entry->next;
		if (!entry) {
			break;
		}
		if (entry->frequency != frequency) {
			break;
		}

		sprintf(ptr, ", ");
		ptr = strchr(ptr, 0);
	}
	sprintf(ptr, ")");
	*pentry = entry;

	ret = channelscan_execute_callback(callback, callback_ap, "SCANNING", buffer);
	if (ret <= 0) {
		return ret;
	}

	/* Find lock. */
	struct hdhomerun_tuner_status_t status;
	ret = channelscan_execute_find_lock(hd, frequency, &status);

	ptr = buffer;
	sprintf(ptr, "%s (ss=%u snq=%u seq=%u)", status.lock_str, status.signal_strength, status.signal_to_noise_quality, status.symbol_error_quality);

	ret = channelscan_execute_callback(callback, callback_ap, "LOCK", buffer);
	if (ret <= 0) {
		return ret;
	}

	if (!status.lock_supported) {
		return 1;
	}

	/* Detect programs. */
	char *streaminfo = NULL;
	ret = channelscan_execute_find_programs(hd, &streaminfo);
	if (ret <= 0) {
		return ret;
	}

	ptr = streaminfo;
	while (1) {
		char *end = strchr(ptr, '\n');
		if (!end) {
			break;
		}

		*end++ = 0;

		ret = channelscan_execute_callback(callback, callback_ap, "PROGRAM", ptr);
		if (ret <= 0) {
			free(streaminfo);
			return ret;
		}

		ptr = end;
	}

	free(streaminfo);

	/* Complete. */
	return 1;
}

int channelscan_execute(struct hdhomerun_device_t *hd, channelscan_callback_t callback, ...)
{
	struct channelscan_entry_t *list;
	channelscan_list_build(&list);

	va_list callback_ap;
	va_start(callback_ap, callback);

	struct channelscan_entry_t *entry = list;
	int result = 0;
	while (entry) {
		result = channelscan_execute_internal(hd, &entry, callback, callback_ap);
		if (result <= 0) {
			break;
		}
	}

	va_end(callback_ap);
	channelscan_list_free(&list);
	return result;
}
