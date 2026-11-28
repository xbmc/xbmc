/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: Zlib
 *  See LICENSES/README.md for more information.
 */

/* Replaces zlib's private zutil.h, which is not installed, with the subset
 * infback9.c and inftree9.c need so they build against the public zlib.h. */

#pragma once

#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#define zmemcpy memcpy
#define Tracev(x)
#define Tracevv(x)

#define ZALLOC(strm, items, size) (*((strm)->zalloc))((strm)->opaque, (items), (size))
#define ZFREE(strm, addr) (*((strm)->zfree))((strm)->opaque, (voidpf)(addr))

static voidpf zcalloc(voidpf opaque, unsigned items, unsigned size)
{
  (void)opaque;
  return calloc(items, size);
}

static void zcfree(voidpf opaque, voidpf ptr)
{
  (void)opaque;
  free(ptr);
}
