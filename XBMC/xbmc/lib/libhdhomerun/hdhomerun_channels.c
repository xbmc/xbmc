/*
 * hdhomerun_channels.c
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

#define FREQUENCY_RESOLUTION 62500

struct hdhomerun_channel_entry_t {
	struct hdhomerun_channel_entry_t *next;
	struct hdhomerun_channel_entry_t *prev;
	uint32_t frequency;
	uint8_t channel_number;
	char name[16];
};

struct hdhomerun_channel_list_t {
	struct hdhomerun_channel_entry_t *head;
	struct hdhomerun_channel_entry_t *tail;
};

struct hdhomerun_channelmap_range_t {
	uint8_t channel_range_start;
	uint8_t channel_range_end;
	uint32_t frequency;
	uint32_t spacing;
};

struct hdhomerun_channelmap_record_t {
	const char *channelmap_prefix;
	const char *channelmap;
	const struct hdhomerun_channelmap_range_t *range_list;
	const char *channelmap_scan_group;
	const char *countrycodes;
};

/* AU antenna channels. Channels {0, 1, 2, 6, 7, 8, 9, 9A} are numbered {2, 3, 4, 5, 6, 7, 8, 9} by the HDHomeRun. */
static const struct hdhomerun_channelmap_range_t hdhomerun_channelmap_range_au_bcast[] = {
	{  2,   2,  48500000, 7000000},
	{  3,   4,  59500000, 7000000},
	{  5,  12, 177500000, 7000000},
	{ 28,  69, 529500000, 7000000},
	{  0,   0,         0,       0}
};

/* AU cable channels. TBD. */
static const struct hdhomerun_channelmap_range_t hdhomerun_channelmap_range_au_cable[] = {
	{  0,   0,         0,       0}
};

/* EU antenna channels. */
static const struct hdhomerun_channelmap_range_t hdhomerun_channelmap_range_eu_bcast[] = {
	{  2,   4,  50500000, 7000000},
	{  5,  12, 177500000, 7000000},
	{ 21,  69, 474000000, 8000000},
	{  0,   0,         0,       0}
};

/* EU cable channels. Channels do not have simple numbers - the HDHomeRun uses its own numbering scheme (subject to change). */
static const struct hdhomerun_channelmap_range_t hdhomerun_channelmap_range_eu_cable[] = {
	{  6,   7, 113000000, 8000000},
	{  9, 100, 138000000, 8000000},
	{  0,   0,         0,       0}
};

/* US antenna channels. */
static const struct hdhomerun_channelmap_range_t hdhomerun_channelmap_range_us_bcast[] = {
	{  2,   4,  57000000, 6000000},
	{  5,   6,  79000000, 6000000},
	{  7,  13, 177000000, 6000000},
	{ 14,  69, 473000000, 6000000},
	{  0,   0,         0,       0}
};

/* US cable channels. */
static const struct hdhomerun_channelmap_range_t hdhomerun_channelmap_range_us_cable[] = {
	{  2,   4,  57000000, 6000000},
	{  5,   6,  79000000, 6000000},
	{  7,  13, 177000000, 6000000},
	{ 14,  22, 123000000, 6000000},
	{ 23,  94, 219000000, 6000000},
	{ 95,  99,  93000000, 6000000},
	{100, 135, 651000000, 6000000},
	{  0,   0,         0,       0}
};

/* US cable channels (HRC). */
static const struct hdhomerun_channelmap_range_t hdhomerun_channelmap_range_us_hrc[] = {
	{  2,   4,  55752700, 6000300},
	{  5,   6,  79753900, 6000300},
	{  7,  13, 175758700, 6000300},
	{ 14,  22, 121756000, 6000300},
	{ 23,  94, 217760800, 6000300},
	{ 95,  99,  91754500, 6000300},
	{100, 135, 649782400, 6000300},
	{  0,   0,         0,       0}
};

/* US cable channels (IRC). */
static const struct hdhomerun_channelmap_range_t hdhomerun_channelmap_range_us_irc[] = {
	{  2,   4,  57012500, 6000000},
	{  5,   6,  81012500, 6000000},
	{  7,  13, 177012500, 6000000},
	{ 14,  22, 123012500, 6000000},
	{ 23,  41, 219012500, 6000000},
	{ 42,  42, 333025000, 6000000},
	{ 43,  94, 339012500, 6000000},
	{ 95,  97,  93012500, 6000000},
	{ 98,  99, 111025000, 6000000},
	{100, 135, 651012500, 6000000},
	{  0,   0,         0,       0}
};

static const struct hdhomerun_channelmap_record_t hdhomerun_channelmap_table[] = {
	{"au", "au-bcast", hdhomerun_channelmap_range_au_bcast, "au-bcast",               "AU"},
	{"au", "au-cable", hdhomerun_channelmap_range_au_cable, "au-cable",               "AU"},
	{"eu", "eu-bcast", hdhomerun_channelmap_range_eu_bcast, "eu-bcast",               "EU"},
	{"eu", "eu-cable", hdhomerun_channelmap_range_eu_cable, "eu-cable",               "EU"},
	{"tw", "tw-bcast", hdhomerun_channelmap_range_us_bcast, "tw-bcast",               "TW"},
	{"tw", "tw-cable", hdhomerun_channelmap_range_us_cable, "tw-cable",               "TW"},
	{"us", "us-bcast", hdhomerun_channelmap_range_us_bcast, "us-bcast",               "CA US"},
	{"us", "us-cable", hdhomerun_channelmap_range_us_cable, "us-cable us-hrc us-irc", "CA US"},
	{"us", "us-hrc",   hdhomerun_channelmap_range_us_hrc  , "us-cable us-hrc us-irc", "CA US"},
	{"us", "us-irc",   hdhomerun_channelmap_range_us_irc,   "us-cable us-hrc us-irc", "CA US"},
	{NULL, NULL,       NULL,                                NULL,                     NULL}
};

const char *hdhomerun_channelmap_convert_countrycode_to_channelmap_prefix(const char *countrycode)
{
	const struct hdhomerun_channelmap_record_t *record = hdhomerun_channelmap_table;
	while (record->channelmap) {
		if (strstr(record->countrycodes, countrycode)) {
			return record->channelmap_prefix;
		}
		record++;
	}

	return "eu";
}

const char *hdhomerun_channelmap_get_channelmap_scan_group(const char *channelmap)
{
	const struct hdhomerun_channelmap_record_t *record = hdhomerun_channelmap_table;
	while (record->channelmap) {
		if (strstr(channelmap, record->channelmap)) {
			return record->channelmap_scan_group;
		}
		record++;
	}

	return NULL;
}

uint8_t hdhomerun_channel_entry_channel_number(struct hdhomerun_channel_entry_t *entry)
{
	return entry->channel_number;
}

uint32_t hdhomerun_channel_entry_frequency(struct hdhomerun_channel_entry_t *entry)
{
	return entry->frequency;
}

const char *hdhomerun_channel_entry_name(struct hdhomerun_channel_entry_t *entry)
{
	return entry->name;
}

struct hdhomerun_channel_entry_t *hdhomerun_channel_list_first(struct hdhomerun_channel_list_t *channel_list)
{
	return channel_list->head;
}

struct hdhomerun_channel_entry_t *hdhomerun_channel_list_last(struct hdhomerun_channel_list_t *channel_list)
{
	return channel_list->tail;
}

struct hdhomerun_channel_entry_t *hdhomerun_channel_list_next(struct hdhomerun_channel_list_t *channel_list, struct hdhomerun_channel_entry_t *entry)
{
	return entry->next;
}

struct hdhomerun_channel_entry_t *hdhomerun_channel_list_prev(struct hdhomerun_channel_list_t *channel_list, struct hdhomerun_channel_entry_t *entry)
{
	return entry->prev;
}

uint32_t hdhomerun_channel_list_total_count(struct hdhomerun_channel_list_t *channel_list)
{
	uint32_t count = 0;

	struct hdhomerun_channel_entry_t *entry = hdhomerun_channel_list_first(channel_list);
	while (entry) {
		count++;
		entry = hdhomerun_channel_list_next(channel_list, entry);
	}

	return count;
}

uint32_t hdhomerun_channel_list_frequency_count(struct hdhomerun_channel_list_t *channel_list)
{
	uint32_t count = 0;
	uint32_t last_frequency = 0;

	struct hdhomerun_channel_entry_t *entry = hdhomerun_channel_list_first(channel_list);
	while (entry) {
		if (entry->frequency != last_frequency) {
			last_frequency = entry->frequency;
			count++;
		}

		entry = hdhomerun_channel_list_next(channel_list, entry);
	}

	return count;
}

uint32_t hdhomerun_channel_frequency_truncate(uint32_t frequency)
{
	return (frequency / FREQUENCY_RESOLUTION) * FREQUENCY_RESOLUTION;
}

uint32_t hdhomerun_channel_number_to_frequency(struct hdhomerun_channel_list_t *channel_list, uint8_t channel_number)
{
	struct hdhomerun_channel_entry_t *entry = hdhomerun_channel_list_first(channel_list);
	while (entry) {
		if (entry->channel_number == channel_number) {
			return entry->frequency;
		}

		entry = hdhomerun_channel_list_next(channel_list, entry);
	}

	return 0;
}

uint8_t hdhomerun_channel_frequency_to_number(struct hdhomerun_channel_list_t *channel_list, uint32_t frequency)
{
	frequency = hdhomerun_channel_frequency_truncate(frequency);

	struct hdhomerun_channel_entry_t *entry = hdhomerun_channel_list_first(channel_list);
	while (entry) {
		if (entry->frequency == frequency) {
			return entry->channel_number;
		}
		if (entry->frequency > frequency) {
			return 0;
		}

		entry = hdhomerun_channel_list_next(channel_list, entry);
	}

	return 0;
}

static void hdhomerun_channel_list_build_insert(struct hdhomerun_channel_list_t *channel_list, struct hdhomerun_channel_entry_t *entry)
{
	struct hdhomerun_channel_entry_t *prev = NULL;
	struct hdhomerun_channel_entry_t *next = channel_list->head;

	while (next) {
		if (next->frequency > entry->frequency) {
			break;
		}

		prev = next;
		next = next->next;
	}

	entry->prev = prev;
	entry->next = next;

	if (prev) {
		prev->next = entry;
	} else {
		channel_list->head = entry;
	}

	if (next) {
		next->prev = entry;
	} else {
		channel_list->tail = entry;
	}
}

static void hdhomerun_channel_list_build_range(struct hdhomerun_channel_list_t *channel_list, const char *channelmap, const struct hdhomerun_channelmap_range_t *range)
{
	uint8_t channel_number;
	for (channel_number = range->channel_range_start; channel_number <= range->channel_range_end; channel_number++) {
		struct hdhomerun_channel_entry_t *entry = (struct hdhomerun_channel_entry_t *)calloc(1, sizeof(struct hdhomerun_channel_entry_t));
		if (!entry) {
			return;
		}

		entry->channel_number = channel_number;
		entry->frequency = range->frequency + ((uint32_t)(channel_number - range->channel_range_start) * range->spacing);
		entry->frequency = hdhomerun_channel_frequency_truncate(entry->frequency);
		sprintf(entry->name, "%s:%u", channelmap, entry->channel_number);

		hdhomerun_channel_list_build_insert(channel_list, entry);
	}
}

static void hdhomerun_channel_list_build_ranges(struct hdhomerun_channel_list_t *channel_list, const struct hdhomerun_channelmap_record_t *record)
{
	const struct hdhomerun_channelmap_range_t *range = record->range_list;
	while (range->frequency) {
		hdhomerun_channel_list_build_range(channel_list, record->channelmap, range);
		range++;
	}
}

void hdhomerun_channel_list_destroy(struct hdhomerun_channel_list_t *channel_list)
{
	while (channel_list->head) {
		struct hdhomerun_channel_entry_t *entry = channel_list->head;
		channel_list->head = entry->next;
		free(entry);
	}

	free(channel_list);
}

struct hdhomerun_channel_list_t *hdhomerun_channel_list_create(const char *channelmap)
{
	struct hdhomerun_channel_list_t *channel_list = (struct hdhomerun_channel_list_t *)calloc(1, sizeof(struct hdhomerun_channel_list_t));
	if (!channel_list) {
		return NULL;
	}

	const struct hdhomerun_channelmap_record_t *record = hdhomerun_channelmap_table;
	while (record->channelmap) {
		if (!strstr(channelmap, record->channelmap)) {
			record++;
			continue;
		}

		hdhomerun_channel_list_build_ranges(channel_list, record);
		record++;
	}

	if (!channel_list->head) {
		free(channel_list);
		return NULL;
	}

	return channel_list;
}
