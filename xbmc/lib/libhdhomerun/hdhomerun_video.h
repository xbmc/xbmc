/*
 * hdhomerun_video.h
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
#ifdef __cplusplus
extern "C" {
#endif

struct hdhomerun_video_sock_t;

#define TS_PACKET_SIZE 188
#define VIDEO_DATA_PACKET_SIZE (188 * 7)
#define VIDEO_DATA_BUFFER_SIZE_1S (20000000 / 8)

/*
 * Create a video/data socket.
 *
 * uint16_t listen_port: Port number to listen on. Set to 0 to auto-select.
 * size_t buffer_size: Size of receive buffer. For 1 second of buffer use VIDEO_DATA_BUFFER_SIZE_1S. 
 *
 * Returns a pointer to the newly created control socket.
 *
 * When no longer needed, the socket should be destroyed by calling hdhomerun_control_destroy.
 */
extern struct hdhomerun_video_sock_t *hdhomerun_video_create(uint16_t listen_port, size_t buffer_size);
extern void hdhomerun_video_destroy(struct hdhomerun_video_sock_t *vs);

/*
 * Get the port the socket is listening on.
 *
 * Returns 16-bit port with native endianness, or 0 on error.
 */
extern uint16_t hdhomerun_video_get_local_port(struct hdhomerun_video_sock_t *vs);

/*
 * Get the low-level socket handle.
 */
extern int hdhomerun_video_get_sock(struct hdhomerun_video_sock_t *vs);

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
extern uint8_t *hdhomerun_video_recv(struct hdhomerun_video_sock_t *vs, size_t max_size, size_t *pactual_size);

/*
 * Flush the buffer.
 */
extern void hdhomerun_video_flush(struct hdhomerun_video_sock_t *vs);

#ifdef __cplusplus
}
#endif
