/*
 * hdhomerun_channelscan.h
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

#define HDHOMERUN_CHANNELSCAN_PROGRAM_NORMAL 0
#define HDHOMERUN_CHANNELSCAN_PROGRAM_NODATA 1
#define HDHOMERUN_CHANNELSCAN_PROGRAM_CONTROL 2
#define HDHOMERUN_CHANNELSCAN_PROGRAM_ENCRYPTED 3

struct hdhomerun_channelscan_t;

extern LIBTYPE struct hdhomerun_channelscan_t *channelscan_create(struct hdhomerun_device_t *hd, const char *channelmap);
extern LIBTYPE void channelscan_destroy(struct hdhomerun_channelscan_t *scan);

extern LIBTYPE int channelscan_advance(struct hdhomerun_channelscan_t *scan, struct hdhomerun_channelscan_result_t *result);
extern LIBTYPE int channelscan_detect(struct hdhomerun_channelscan_t *scan, struct hdhomerun_channelscan_result_t *result);
extern LIBTYPE uint8_t channelscan_get_progress(struct hdhomerun_channelscan_t *scan);

#ifdef __cplusplus
}
#endif
