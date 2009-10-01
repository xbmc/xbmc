/*
 * hdhomerun_channels.h
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

#ifdef __cplusplus
extern "C" {
#endif

struct hdhomerun_channel_entry_t;
struct hdhomerun_channel_list_t;

extern LIBTYPE const char *hdhomerun_channelmap_convert_countrycode_to_channelmap_prefix(const char *countrycode);
extern LIBTYPE const char *hdhomerun_channelmap_get_channelmap_scan_group(const char *channelmap);

extern LIBTYPE uint8_t hdhomerun_channel_entry_channel_number(struct hdhomerun_channel_entry_t *entry);
extern LIBTYPE uint32_t hdhomerun_channel_entry_frequency(struct hdhomerun_channel_entry_t *entry);
extern LIBTYPE const char *hdhomerun_channel_entry_name(struct hdhomerun_channel_entry_t *entry);

extern LIBTYPE struct hdhomerun_channel_list_t *hdhomerun_channel_list_create(const char *channelmap);
extern LIBTYPE void hdhomerun_channel_list_destroy(struct hdhomerun_channel_list_t *channel_list);

extern LIBTYPE struct hdhomerun_channel_entry_t *hdhomerun_channel_list_first(struct hdhomerun_channel_list_t *channel_list);
extern LIBTYPE struct hdhomerun_channel_entry_t *hdhomerun_channel_list_last(struct hdhomerun_channel_list_t *channel_list);
extern LIBTYPE struct hdhomerun_channel_entry_t *hdhomerun_channel_list_next(struct hdhomerun_channel_list_t *channel_list, struct hdhomerun_channel_entry_t *entry);
extern LIBTYPE struct hdhomerun_channel_entry_t *hdhomerun_channel_list_prev(struct hdhomerun_channel_list_t *channel_list, struct hdhomerun_channel_entry_t *entry);
extern LIBTYPE uint32_t hdhomerun_channel_list_total_count(struct hdhomerun_channel_list_t *channel_list);
extern LIBTYPE uint32_t hdhomerun_channel_list_frequency_count(struct hdhomerun_channel_list_t *channel_list);

extern LIBTYPE uint32_t hdhomerun_channel_frequency_truncate(uint32_t frequency);
extern LIBTYPE uint32_t hdhomerun_channel_number_to_frequency(struct hdhomerun_channel_list_t *channel_list, uint8_t channel_number);
extern LIBTYPE uint8_t hdhomerun_channel_frequency_to_number(struct hdhomerun_channel_list_t *channel_list, uint32_t frequency);

#ifdef __cplusplus
}
#endif
