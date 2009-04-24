/*
 * hdhomerun_channelscan.c
 *
 * Copyright © 2007-2008 Silicondust Engineering Ltd. <www.silicondust.com>.
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

struct hdhomerun_channelscan_t {
	struct hdhomerun_device_t *hd;
	uint32_t scanned_channels;
	struct hdhomerun_channel_list_t *channel_list;	
	struct hdhomerun_channel_entry_t *next_channel;
};

struct hdhomerun_channelscan_t *channelscan_create(struct hdhomerun_device_t *hd, const char *channelmap)
{
	struct hdhomerun_channelscan_t *scan = (struct hdhomerun_channelscan_t *)calloc(1, sizeof(struct hdhomerun_channelscan_t));
	if (!scan) {
		return NULL;
	}

	scan->hd = hd;

	scan->channel_list = hdhomerun_channel_list_create(channelmap);
	if (!scan->channel_list) {
		free(scan);
		return NULL;
	}

	scan->next_channel = hdhomerun_channel_list_last(scan->channel_list);
	return scan;
}

void channelscan_destroy(struct hdhomerun_channelscan_t *scan)
{
	free(scan);
}

static int channelscan_execute_find_lock(struct hdhomerun_channelscan_t *scan, uint32_t frequency, struct hdhomerun_channelscan_result_t *result)
{
	/* Set channel. */
	char channel_str[64];
	sprintf(channel_str, "auto:%ld", (unsigned long)frequency);

	int ret = hdhomerun_device_set_tuner_channel(scan->hd, channel_str);
	if (ret <= 0) {
		return ret;
	}

	/* Wait for lock. */
	ret = hdhomerun_device_wait_for_lock(scan->hd, &result->status);
	if (ret <= 0) {
		return ret;
	}
	if (!result->status.lock_supported) {
		return 1;
	}

	/* Wait for symbol quality = 100%. */
	int i;
	for (i = 0; i < 5 * 4; i++) {
		usleep(250000);

		ret = hdhomerun_device_get_tuner_status(scan->hd, NULL, &result->status);
		if (ret <= 0) {
			return ret;
		}

		if (result->status.symbol_error_quality == 100) {
			return 1;
		}
	}

	/* Timeout. */
	return 1;
}

static void channelscan_extract_name(struct hdhomerun_channelscan_program_t *program, const char *line)
{
	/* Find start of name. */
	const char *start = strchr(line, ' ');
	if (!start) {
		return;
	}
	start++;

	start = strchr(start, ' ');
	if (!start) {
		return;
	}
	start++;

	/* Find end of name. */
	const char *end = strstr(start, " (");
	if (!end) {
		end = strchr(line, 0);
	}

	if (end <= start) {
		return;
	}

	/* Extract name. */
	size_t length = (size_t)(end - start);
	if (length > sizeof(program->name) - 1) {
		length = sizeof(program->name) - 1;
	}

	strncpy(program->name, start, length);
	program->name[length] = 0;
}

static int channelscan_execute_detect_programs(struct hdhomerun_channelscan_t *scan, struct hdhomerun_channelscan_result_t *result, int *pchanged)
{
	*pchanged = FALSE;

	char *streaminfo;
	int ret = hdhomerun_device_get_tuner_streaminfo(scan->hd, &streaminfo);
	if (ret <= 0) {
		return ret;
	}

	char *line = streaminfo;
	int program_count = 0;

	while (1) {
		char *end = strchr(line, '\n');
		if (!end) {
			break;
		}

		*end = 0;

		struct hdhomerun_channelscan_program_t program;
		memset(&program, 0, sizeof(program));

		strncpy(program.program_str, line, sizeof(program.program_str));
		program.program_str[sizeof(program.program_str) - 1] = 0;

		unsigned int program_number;
		unsigned int virtual_major, virtual_minor;
		if (sscanf(line, "%u: %u.%u", &program_number, &virtual_major, &virtual_minor) != 3) {
			continue;
		}

		program.program_number = program_number;
		program.virtual_major = virtual_major;
		program.virtual_minor = virtual_minor;

		channelscan_extract_name(&program, line);

		program.type = HDHOMERUN_CHANNELSCAN_PROGRAM_NORMAL;
		if (strstr(line, "(no data)")) {
			program.type = HDHOMERUN_CHANNELSCAN_PROGRAM_NODATA;
		}
		if (strstr(line, "(control)")) {
			program.type = HDHOMERUN_CHANNELSCAN_PROGRAM_CONTROL;
		}
		if (strstr(line, "(encrypted)")) {
			program.type = HDHOMERUN_CHANNELSCAN_PROGRAM_ENCRYPTED;
		}

		if (memcmp(&result->programs[program_count], &program, sizeof(program)) != 0) {
			memcpy(&result->programs[program_count], &program, sizeof(program));
			*pchanged = TRUE;
		}

		program_count++;
		if (program_count >= HDHOMERUN_CHANNELSCAN_MAX_PROGRAM_COUNT) {
			break;
		}

		line = end + 1;
	}

	if (result->program_count != program_count) {
		result->program_count = program_count;
		*pchanged = TRUE;
	}

	return 1;
}

int channelscan_advance(struct hdhomerun_channelscan_t *scan, struct hdhomerun_channelscan_result_t *result)
{
	memset(result, 0, sizeof(struct hdhomerun_channelscan_result_t));

	struct hdhomerun_channel_entry_t *entry = scan->next_channel;
	if (!entry) {
		return 0;
	}

	/* Combine channels with same frequency. */
	result->frequency = hdhomerun_channel_entry_frequency(entry);
	strncpy(result->channel_str, hdhomerun_channel_entry_name(entry), sizeof(result->channel_str) - 1);
	result->channel_str[sizeof(result->channel_str) - 1] = 0;

	while (1) {
		entry = hdhomerun_channel_list_prev(scan->channel_list, entry);
		if (!entry) {
			scan->next_channel = NULL;
			break;
		}

		if (hdhomerun_channel_entry_frequency(entry) != result->frequency) {
			scan->next_channel = entry;
			break;
		}

		char *ptr = strchr(result->channel_str, 0);
		sprintf(ptr, ", %s", hdhomerun_channel_entry_name(entry));
	}

	return 1;
}

int channelscan_detect(struct hdhomerun_channelscan_t *scan, struct hdhomerun_channelscan_result_t *result)
{
	scan->scanned_channels++;

	/* Find lock. */
	int ret = channelscan_execute_find_lock(scan, result->frequency, result);
	if (ret <= 0) {
		return ret;
	}
	if (!result->status.lock_supported) {
		return 1;
	}

	/* Detect programs. */
	result->program_count = 0;

	int changed;
	ret = channelscan_execute_detect_programs(scan, result, &changed);
	if (ret <= 0) {
		return ret;
	}

	int same_count = 0;
	int i;
	for (i = 0; i < 5 * 4; i++) {
		usleep(250000);

		ret = channelscan_execute_detect_programs(scan, result, &changed);
		if (ret <= 0) {
			return ret;
		}

		if (changed) {
			same_count = 0;
			continue;
		}

		same_count++;
		if (same_count >= 8) {
			break;
		}
	}

	/* Complete. */
	return 1;
}

uint8_t channelscan_get_progress(struct hdhomerun_channelscan_t *scan)
{
	struct hdhomerun_channel_entry_t *entry = scan->next_channel;
	if (!entry) {
		return 100;
	}

	uint32_t channels_remaining = 1;
	uint32_t frequency = hdhomerun_channel_entry_frequency(entry);

	while (1) {
		entry = hdhomerun_channel_list_prev(scan->channel_list, entry);
		if (!entry) {
			break;
		}

		if (hdhomerun_channel_entry_frequency(entry) != frequency) {
			channels_remaining++;
			frequency = hdhomerun_channel_entry_frequency(entry);
		}
	}

	return scan->scanned_channels * 100 / (scan->scanned_channels + channels_remaining);
}
