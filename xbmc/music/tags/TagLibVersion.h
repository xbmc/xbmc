/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <taglib/taglib.h>

/*!
 * The single place where the linked TagLib version becomes a feature define.
 *
 * TagLib only exposes TAGLIB_{MAJOR,MINOR,PATCH}_VERSION, so every site wanting "at least x.y.z"
 * has to spell out the same three-way comparison. Spelling it out once here keeps the copies from
 * drifting when a floor moves.
 */
#define TAGLIB_VERSION_INT \
  ((TAGLIB_MAJOR_VERSION) * 10000 + (TAGLIB_MINOR_VERSION) * 100 + (TAGLIB_PATCH_VERSION))

/*!
 * TagLib's Matroska API arrived in 2.2 and edition/chapter uid() in 2.2.1, but the floor sits at
 * 2.3.1: 2.2.x lacks the EBML MasterElement unbounded-recursion fix, and 2.3 crashes on a Matroska
 * file with an invalid or missing seek head, mishandles unknown-size elements and drops chapters
 * carrying no UID. Those are malformed-media cases a library scan is expected to survive rather
 * than reject, and all of them are reachable from untrusted media files.
 *
 * When this is not defined, Matroska music tags are read through FFmpeg instead - see
 * CAudioBookFileDirectory and CMusicInfoTagLoaderFFmpeg. Both paths share the tag name mapping in
 * MatroskaTagMapping, so the feature degrades in fidelity rather than disappearing.
 *
 * 2.3.1 needs 002-matroska-fast-scan-segment-lookup.patch: it bounds the Segment lookup by the
 * AudioProperties::Fast scan limit and so reads no tags from any Matroska over 512 KiB. Internal
 * builds carry the patch, a distro 2.3.1 does not. Move this floor to the first release carrying
 * the fix upstream.
 */
/*!
 * Defining KODI_NO_TAGLIB_MATROSKA builds the FFmpeg reader instead, whatever TagLib is linked.
 * It exists so that the configuration distributions below the floor actually get can be built and
 * tested without holding an old TagLib, and is not a user facing option.
 */
#if TAGLIB_VERSION_INT >= 20301 && !defined(KODI_NO_TAGLIB_MATROSKA)
#define HAS_TAGLIB_MATROSKA 1
#endif
