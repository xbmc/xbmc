/* SPDX-License-Identifier: MIT */
/*
 * kodi-web-video: the decoded frames of a web page's video, shown by Kodi on
 * a display plane underneath the page.
 *
 * The web engine's video sink connects to the AF_UNIX SOCK_SEQPACKET socket
 * Kodi names in KODI_WEB_VIDEO_SOCKET, one connection per sink. Every message
 * starts with a uint32_t type; all fields are in host byte order, both ends
 * run on the same host.
 *
 *  sink -> Kodi  HELLO          version
 *  Kodi -> sink  HELLO_REPLY    version, the DRM formats and modifiers Kodi
 *                               can show
 *  sink -> Kodi  FRAME          id, size, DRM format and modifier, plane
 *                               layout, colorimetry; carries one descriptor
 *                               (SCM_RIGHTS) per DMA-BUF object
 *  Kodi -> sink  RELEASE        id of a frame Kodi no longer shows
 *  sink -> Kodi  CLEAR          the sink shows nothing until its next FRAME
 *  sink -> Kodi  RECT           where on the view the video is shown, in the
 *                               view's pixels; an empty one leaves the
 *                               placement to Kodi
 *
 * The sink sends a frame when it is due and keeps it until Kodi releases it,
 * as Kodi scans the frame out from the memory the sink holds. Kodi releases
 * every frame once the connection closes.
 */

#ifndef KODI_WEB_VIDEO_H
#define KODI_WEB_VIDEO_H

#include <stdint.h>

#define KODI_WEB_VIDEO_VERSION 1
#define KODI_WEB_VIDEO_ENV "KODI_WEB_VIDEO_SOCKET"
#define KODI_WEB_VIDEO_MAX_FORMATS 64
#define KODI_WEB_VIDEO_MAX_PLANES 4

enum kodi_web_video_type
{
  KODI_WEB_VIDEO_HELLO = 1,
  KODI_WEB_VIDEO_HELLO_REPLY,
  KODI_WEB_VIDEO_FRAME,
  KODI_WEB_VIDEO_RELEASE,
  KODI_WEB_VIDEO_CLEAR,
  KODI_WEB_VIDEO_RECT,
};

/* the matrix coefficients, as in ITU-T H.273 */
enum kodi_web_video_matrix
{
  KODI_WEB_VIDEO_MATRIX_UNSPECIFIED = 0,
  KODI_WEB_VIDEO_MATRIX_BT601,
  KODI_WEB_VIDEO_MATRIX_BT709,
  KODI_WEB_VIDEO_MATRIX_BT2020,
};

enum kodi_web_video_range
{
  KODI_WEB_VIDEO_RANGE_UNSPECIFIED = 0,
  KODI_WEB_VIDEO_RANGE_LIMITED,
  KODI_WEB_VIDEO_RANGE_FULL,
};

enum kodi_web_video_transfer
{
  KODI_WEB_VIDEO_TRANSFER_UNSPECIFIED = 0,
  KODI_WEB_VIDEO_TRANSFER_SDR,
  KODI_WEB_VIDEO_TRANSFER_PQ,
  KODI_WEB_VIDEO_TRANSFER_HLG,
};

struct kodi_web_video_hello
{
  uint32_t type;
  uint32_t version;
};

struct kodi_web_video_format
{
  uint32_t fourcc;
  uint32_t pad;
  uint64_t modifier;
};

struct kodi_web_video_hello_reply
{
  uint32_t type;
  uint32_t version;
  uint32_t format_count;
  uint32_t pad;
  struct kodi_web_video_format formats[KODI_WEB_VIDEO_MAX_FORMATS];
};

struct kodi_web_video_frame
{
  uint32_t type;
  uint32_t id;
  uint32_t width;
  uint32_t height;
  uint32_t fourcc;
  uint32_t object_count;
  uint64_t modifier;
  uint32_t plane_count;
  uint32_t object[KODI_WEB_VIDEO_MAX_PLANES];
  uint32_t offset[KODI_WEB_VIDEO_MAX_PLANES];
  uint32_t stride[KODI_WEB_VIDEO_MAX_PLANES];
  /* pixel aspect ratio */
  uint32_t par_n;
  uint32_t par_d;
  uint32_t matrix;
  uint32_t range;
  uint32_t transfer;
};

struct kodi_web_video_release
{
  uint32_t type;
  uint32_t id;
};

struct kodi_web_video_clear
{
  uint32_t type;
};

struct kodi_web_video_rect
{
  uint32_t type;
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
};

#endif
