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
 * 2.3.2: 2.2.x lacks the EBML MasterElement unbounded-recursion fix, and 2.3.0 crashes on a Matroska
 * file with an invalid or missing seek head, mishandles unknown-size elements and drops chapters
 * carrying no UID. Those are malformed-media cases a library scan is expected to survive rather
 * than reject, and all of them are reachable from untrusted media files.
 *
 * When this is not defined, Matroska music tags are read through FFmpeg instead - see
 * CAudioBookFileDirectory and CMusicInfoTagLoaderFFmpeg. Both paths share the tag name mapping in
 * MatroskaTagMapping, so the feature degrades in fidelity rather than disappearing.
 *
 * TagLib 2.3.2 also fixes the AudioProperties::Fast Segment lookup, which otherwise reads no tags
 * from Matroska files over 512 KiB.
 *
 * Defining KODI_NO_TAGLIB_MATROSKA builds the FFmpeg reader whatever TagLib is linked, so that the
 * configuration a distribution below the floor gets can be built and tested without an old TagLib
 * to hand. It is not a user facing option:
 *
 *     cmake -DCMAKE_CXX_FLAGS=-DKODI_NO_TAGLIB_MATROSKA ...
 */
#if TAGLIB_VERSION_INT >= 20302 && !defined(KODI_NO_TAGLIB_MATROSKA)
#define HAS_TAGLIB_MATROSKA 1
#endif
