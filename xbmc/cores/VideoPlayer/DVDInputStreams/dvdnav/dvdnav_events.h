/*
 * Copyright (C) 2001 Rich Wareham <richwareham@users.sourceforge.net>
 *
 * This file is part of libdvdnav, a DVD navigation library.
 *
 * libdvdnav is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdnav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdnav; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * This header defines events and event types
 */

#pragma once

/*
 * DVDNAV_BLOCK_OK
 *
 * A regular data block from the DVD has been returned.
 * This one should be demuxed and decoded for playback.
 */
#define DVDNAV_BLOCK_OK 0

/*
 * DVDNAV_NOP
 *
 * Just ignore this.
 */
#define DVDNAV_NOP 1

/*
 * DVDNAV_STILL_FRAME
 *
 * We have reached a still frame. The player application should wait
 * the amount of time specified by the still's length while still handling
 * user input to make menus and other interactive stills work.
 * The last delivered frame should be kept showing.
 * Once the still has timed out, call dvdnav_skip_still().
 * A length of 0xff means an infinite still which has to be skipped
 * indirectly by some user interaction.
 */
#define DVDNAV_STILL_FRAME 2

typedef struct {
  /* The length (in seconds) the still frame should be displayed for,
   * or 0xff if infinite. */
  int length;
} dvdnav_still_event_t;


/*
 * DVDNAV_SPU_STREAM_CHANGE
 *
 * Inform the SPU decoding/overlaying engine to switch SPU channels.
 */
#define DVDNAV_SPU_STREAM_CHANGE 3

typedef struct {
  /* The physical (MPEG) stream number for widescreen SPU display.
   * Use this, if you blend the SPU on an anamorphic image before
   * unsqueezing it. */
  int physical_wide;

  /* The physical (MPEG) stream number for letterboxed display.
   * Use this, if you blend the SPU on an anamorphic image after
   * unsqueezing it. */
  int physical_letterbox;

  /* The physical (MPEG) stream number for pan&scan display.
   * Use this, if you blend the SPU on an anamorphic image after
   * unsqueezing it the pan&scan way. */
  int physical_pan_scan;

  /* The logical (DVD) stream number. */
  int logical;
} dvdnav_spu_stream_change_event_t;


/*
 * DVDNAV_AUDIO_STREAM_CHANGE
 *
 * Inform the audio decoder to switch channels.
 */
#define DVDNAV_AUDIO_STREAM_CHANGE 4

typedef struct {
  /* The physical (MPEG) stream number. */
  int physical;

  /* The logical (DVD) stream number. */
  int logical;
} dvdnav_audio_stream_change_event_t;


/*
 * DVDNAV_VTS_CHANGE
 *
 * Some status information like video aspect and video scale permissions do
 * not change inside a VTS. Therefore this event can be used to query such
 * information only when necessary and update the decoding/displaying
 * accordingly.
 */
#define DVDNAV_VTS_CHANGE 5

typedef struct {
  int old_vtsN;                 /* the old VTS number */
  DVDDomain_t old_domain; /* the old domain */
  int new_vtsN;                 /* the new VTS number */
  DVDDomain_t new_domain; /* the new domain */
} dvdnav_vts_change_event_t;


/*
 * DVDNAV_CELL_CHANGE
 *
 * Some status information like the current Title and Part numbers do not
 * change inside a cell. Therefore this event can be used to query such
 * information only when necessary and update the decoding/displaying
 * accordingly.
 * Some useful information for accurate time display is also reported
 * together with this event.
 */
#define DVDNAV_CELL_CHANGE 6

typedef struct {
  int     cellN;       /* the new cell number */
  int     pgN;         /* the current program number */
  int64_t cell_length; /* the length of the current cell in sectors */
  int64_t pg_length; /* the length of the current program in sectors */
  int64_t pgc_length;  /* the length of the current program chain in PTS ticks */
  int64_t cell_start; /* the start offset of the current cell relatively to the PGC in sectors */
  int64_t pg_start; /* the start offset of the current PG relatively to the PGC in sectors */
} dvdnav_cell_change_event_t;


/*
 * DVDNAV_NAV_PACKET
 *
 * NAV packets are useful for various purposes. They define the button
 * highlight areas and VM commands of DVD menus, so they should in any
 * case be sent to the SPU decoder/overlaying engine for the menus to work.
 * NAV packets also provide a way to detect PTS discontinuities, because
 * they carry the start and end PTS values for the current VOBU.
 * (pci.vobu_s_ptm and pci.vobu_e_ptm) Whenever the start PTS of the
 * current NAV does not match the end PTS of the previous NAV, a PTS
 * discontinuity has occurred.
 * NAV packets can also be used for time display, because they are
 * timestamped relatively to the current Cell.
 */
#define DVDNAV_NAV_PACKET 7

/*
 * DVDNAV_STOP
 *
 * Applications should end playback here. A subsequent dvdnav_get_next_block()
 * call will restart the VM from the beginning of the DVD.
 */
#define DVDNAV_STOP 8

/*
 * DVDNAV_HIGHLIGHT
 *
 * The current button highlight changed. Inform the overlaying engine to
 * highlight a different button. Please note, that at the moment only mode 1
 * highlights are reported this way. That means, when the button highlight
 * has been moved around by some function call, you will receive an event
 * telling you the new button. But when a button gets activated, you have
 * to handle the mode 2 highlighting (that is some different colour the
 * button turns to on activation) in your application.
 */
#define DVDNAV_HIGHLIGHT 9

typedef struct {
  /* highlight mode: 0 - hide, 1 - show, 2 - activate, currently always 1 */
  int display;

  /* FIXME: these fields are currently not set */
  uint32_t palette; /* The CLUT entries for the highlight palette
                           (4-bits per entry -> 4 entries) */
  uint16_t sx,sy,ex,ey; /* The start/end x,y positions */
  uint32_t pts;         /* Highlight PTS to match with SPU */

  /* button number for the SPU decoder/overlaying engine */
  uint32_t buttonN;
} dvdnav_highlight_event_t;


/*
 * DVDNAV_SPU_CLUT_CHANGE
 *
 * Inform the SPU decoder/overlaying engine to update its colour lookup table.
 * The CLUT is given as 16 uint32_t's in the buffer.
 */
#define DVDNAV_SPU_CLUT_CHANGE 10

/*
 * DVDNAV_HOP_CHANNEL
 *
 * A non-seamless operation has been performed. Applications can drop all
 * their internal fifo's content, which will speed up the response.
 */
#define DVDNAV_HOP_CHANNEL 12

/*
 * DVDNAV_WAIT
 *
 * We have reached a point in DVD playback, where timing is critical.
 * Player application with internal fifos can introduce state
 * inconsistencies, because libdvdnav is always the fifo's length
 * ahead in the stream compared to what the application sees.
 * Such applications should wait until their fifos are empty
 * when they receive this type of event.
 * Once this is achieved, call dvdnav_skip_wait().
 */
#define DVDNAV_WAIT 13
