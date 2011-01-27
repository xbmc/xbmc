/*
 * hdhomerun_video.h
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
#ifdef __cplusplus
extern "C" {
#endif

struct hdhomerun_video_sock_t;

struct hdhomerun_video_stats_t {
	uint32_t packet_count;
	uint32_t network_error_count;
	uint32_t transport_error_count;
	uint32_t sequence_error_count;
	uint32_t overflow_error_count;
};

#define TS_PACKET_SIZE 188
#define VIDEO_DATA_PACKET_SIZE (188 * 7)
#define VIDEO_DATA_BUFFER_SIZE_1S (20000000 / 8)

#define VIDEO_RTP_DATA_PACKET_SIZE ((188 * 7) + 12)

/*
 * Create a video/data socket.
 *
 * uint16_t listen_port: Port number to listen on. Set to 0 to auto-select.
 * size_t buffer_size: Size of receive buffer. For 1 second of buffer use VIDEO_DATA_BUFFER_SIZE_1S. 
 * struct hdhomerun_debug_t *dbg: Pointer to debug logging object. May be NULL.
 *
 * Returns a pointer to the newly created control socket.
 *
 * When no longer needed, the socket should be destroyed by calling hdhomerun_control_destroy.
 */
extern LIBTYPE struct hdhomerun_video_sock_t *hdhomerun_video_create(uint16_t listen_port, size_t buffer_size, struct hdhomerun_debug_t *dbg);
extern LIBTYPE void hdhomerun_video_destroy(struct hdhomerun_video_sock_t *vs);

/*
 * Get the port the socket is listening on.
 *
 * Returns 16-bit port with native endianness, or 0 on error.
 */
extern LIBTYPE uint16_t hdhomerun_video_get_local_port(struct hdhomerun_video_sock_t *vs);

/*
 * Read data from buffer.
 *
 * size_t max_size: The maximum amount of data to be returned.
 * size_t *pactual_size: The caller-supplied pactual_size value will be updated to contain the amount
 *		of data available to the caller.
 *
 * Returns a pointer to the data, or NULL if no data is available.
 * The data will remain valid until another call to hdhomerun_video_recv.
 *
 * The amount of data returned will always be a multiple of VIDEO_DATA_PACKET_SIZE (1316).
 * Attempting to read a single TS frame (188 bytes) will not return data as it is less than
 * the minimum size.
 *
 * The buffer is implemented as a ring buffer. It is possible for this function to return a small
 * amount of data when more is available due to the wrap-around case.
 */
extern LIBTYPE uint8_t *hdhomerun_video_recv(struct hdhomerun_video_sock_t *vs, size_t max_size, size_t *pactual_size);

/*
 * Flush the buffer.
 */
extern LIBTYPE void hdhomerun_video_flush(struct hdhomerun_video_sock_t *vs);

/*
 * Debug print internal stats.
 */
extern LIBTYPE void hdhomerun_video_debug_print_stats(struct hdhomerun_video_sock_t *vs);
extern LIBTYPE void hdhomerun_video_get_stats(struct hdhomerun_video_sock_t *vs, struct hdhomerun_video_stats_t *stats);

#ifdef __cplusplus
}
#endif
