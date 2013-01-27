/*
 * Copyright (C) 2003 by the libdvdnav project
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

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dvdnav/dvdnav.h>

/* shall we use libdvdnav's read ahead cache? */
#define DVD_READ_CACHE 1

/* which is the default language for menus/audio/subpictures? */
#define DVD_LANGUAGE "en"

#ifdef WIN32
#define S_IRWXG 0
#endif

int main(int argc, char **argv) {
  dvdnav_t *dvdnav;
  uint8_t mem[DVD_VIDEO_LB_LEN];
  int finished = 0;
  int output_fd = 0;
  int dump = 0, tt_dump = 0;

  if (argc < 2) {
    printf("Specify DVD device.\n");
    return 1;
  }

  /* open dvdnav handle */
  printf("Opening DVD...\n");
  if (dvdnav_open(&dvdnav, argv[1]) != DVDNAV_STATUS_OK) {
    printf("Error on dvdnav_open\n");
    return 1;
  }

  /* set read ahead cache usage */
  if (dvdnav_set_readahead_flag(dvdnav, DVD_READ_CACHE) != DVDNAV_STATUS_OK) {
    printf("Error on dvdnav_set_readahead_flag: %s\n", dvdnav_err_to_string(dvdnav));
    return 2;
  }

  /* set the language */
  if (dvdnav_menu_language_select(dvdnav, DVD_LANGUAGE) != DVDNAV_STATUS_OK ||
      dvdnav_audio_language_select(dvdnav, DVD_LANGUAGE) != DVDNAV_STATUS_OK ||
      dvdnav_spu_language_select(dvdnav, DVD_LANGUAGE) != DVDNAV_STATUS_OK) {
    printf("Error on setting languages: %s\n", dvdnav_err_to_string(dvdnav));
    return 2;
  }

  /* set the PGC positioning flag to have position information relatively to the
   * whole feature instead of just relatively to the current chapter */
  if (dvdnav_set_PGC_positioning_flag(dvdnav, 1) != DVDNAV_STATUS_OK) {
    printf("Error on dvdnav_set_PGC_positioning_flag: %s\n", dvdnav_err_to_string(dvdnav));
    return 2;
  }


  /* the read loop which regularly calls dvdnav_get_next_block
   * and handles the returned events */
  printf("Reading...\n");
  while (!finished) {
    int result, event, len;
    uint8_t *buf = mem;

    /* the main reading function */
#if DVD_READ_CACHE
    result = dvdnav_get_next_cache_block(dvdnav, &buf, &event, &len);
#else
    result = dvdnav_get_next_block(dvdnav, buf, &event, &len);
#endif

    if (result == DVDNAV_STATUS_ERR) {
      printf("Error getting next block: %s\n", dvdnav_err_to_string(dvdnav));
      return 3;
    }

    switch (event) {
    case DVDNAV_BLOCK_OK:
      /* We have received a regular block of the currently playing MPEG stream.
       * A real player application would now pass this block through demuxing
       * and decoding. We simply write it to disc here. */

      if (!output_fd) {
	printf("Opening output...\n");
	output_fd = open("libdvdnav.mpg", O_CREAT | O_WRONLY, S_IRWXU | S_IRWXG);
	if (output_fd == -1) {
	  printf("Error opening output\n");
	  return 4;
	}
      }

      if (dump || tt_dump)
	write(output_fd, buf, len);

      break;
    case DVDNAV_NOP:
      /* Nothing to do here. */
      break;
    case DVDNAV_STILL_FRAME:
      /* We have reached a still frame. A real player application would wait
       * the amount of time specified by the still's length while still handling
       * user input to make menus and other interactive stills work.
       * A length of 0xff means an indefinite still which has to be skipped
       * indirectly by some user interaction. */
      {
	dvdnav_still_event_t *still_event = (dvdnav_still_event_t *)buf;
	if (still_event->length < 0xff)
	  printf("Skipping %d seconds of still frame\n", still_event->length);
	else
	  printf("Skipping indefinite length still frame\n");
	dvdnav_still_skip(dvdnav);
      }
      break;
    case DVDNAV_WAIT:
      /* We have reached a point in DVD playback, where timing is critical.
       * Player application with internal fifos can introduce state
       * inconsistencies, because libdvdnav is always the fifo's length
       * ahead in the stream compared to what the application sees.
       * Such applications should wait until their fifos are empty
       * when they receive this type of event. */
      printf("Skipping wait condition\n");
      dvdnav_wait_skip(dvdnav);
      break;
    case DVDNAV_SPU_CLUT_CHANGE:
      /* Player applications should pass the new colour lookup table to their
       * SPU decoder */
      break;
    case DVDNAV_SPU_STREAM_CHANGE:
      /* Player applications should inform their SPU decoder to switch channels */
      break;
    case DVDNAV_AUDIO_STREAM_CHANGE:
      /* Player applications should inform their audio decoder to switch channels */
      break;
    case DVDNAV_HIGHLIGHT:
      /* Player applications should inform their overlay engine to highlight the
       * given button */
      {
	dvdnav_highlight_event_t *highlight_event = (dvdnav_highlight_event_t *)buf;
	printf("Selected button %d\n", highlight_event->buttonN);
      }
      break;
    case DVDNAV_VTS_CHANGE:
      /* Some status information like video aspect and video scale permissions do
       * not change inside a VTS. Therefore this event can be used to query such
       * information only when necessary and update the decoding/displaying
       * accordingly. */
      break;
    case DVDNAV_CELL_CHANGE:
      /* Some status information like the current Title and Part numbers do not
       * change inside a cell. Therefore this event can be used to query such
       * information only when necessary and update the decoding/displaying
       * accordingly. */
      {
	int32_t tt = 0, ptt = 0;
	uint32_t pos, len;
	char input = '\0';

	dvdnav_current_title_info(dvdnav, &tt, &ptt);
	dvdnav_get_position(dvdnav, &pos, &len);
	printf("Cell change: Title %d, Chapter %d\n", tt, ptt);
	printf("At position %.0f%% inside the feature\n", 100 * (double)pos / (double)len);

	dump = 0;
	if (tt_dump && tt != tt_dump)
	  tt_dump = 0;

	if (!dump && !tt_dump) {
	  fflush(stdin);
	  while ((input != 'a') && (input != 's') && (input != 'q') && (input != 't')) {
	    printf("(a)ppend cell to output\n(s)kip cell\nappend until end of (t)itle\n(q)uit\n");
	    scanf("%c", &input);
	  }

	  switch (input) {
	  case 'a':
	    dump = 1;
	    break;
	  case 't':
	    tt_dump = tt;
	    break;
	  case 'q':
	    finished = 1;
	  }
	}
      }
      break;
    case DVDNAV_NAV_PACKET:
      /* A NAV packet provides PTS discontinuity information, angle linking information and
       * button definitions for DVD menus. Angles are handled completely inside libdvdnav.
       * For the menus to work, the NAV packet information has to be passed to the overlay
       * engine of the player so that it knows the dimensions of the button areas. */
      {
	pci_t *pci;

	/* Applications with fifos should not use these functions to retrieve NAV packets,
	 * they should implement their own NAV handling, because the packet you get from these
	 * functions will already be ahead in the stream which can cause state inconsistencies.
	 * Applications with fifos should therefore pass the NAV packet through the fifo
	 * and decoding pipeline just like any other data. */
	pci = dvdnav_get_current_nav_pci(dvdnav);
	dvdnav_get_current_nav_dsi(dvdnav);

	if(pci->hli.hl_gi.btn_ns > 0) {
	  int button;

	  printf("Found %i DVD menu buttons...\n", pci->hli.hl_gi.btn_ns);

	  for (button = 0; button < pci->hli.hl_gi.btn_ns; button++) {
	    btni_t *btni = &(pci->hli.btnit[button]);
	    printf("Button %i top-left @ (%i,%i), bottom-right @ (%i,%i)\n",
		    button + 1, btni->x_start, btni->y_start,
		    btni->x_end, btni->y_end);
	  }

	  button = 0;
	  while ((button <= 0) || (button > pci->hli.hl_gi.btn_ns)) {
	    printf("Which button (1 to %i): ", pci->hli.hl_gi.btn_ns);
	    scanf("%i", &button);
	  }

	  printf("Selecting button %i...\n", button);
	  /* This is the point where applications with fifos have to hand in a NAV packet
	   * which has traveled through the fifos. See the notes above. */
	  dvdnav_button_select_and_activate(dvdnav, pci, button);
	}
      }
      break;
    case DVDNAV_HOP_CHANNEL:
      /* This event is issued whenever a non-seamless operation has been executed.
       * Applications with fifos should drop the fifos content to speed up responsiveness. */
      break;
    case DVDNAV_STOP:
      /* Playback should end here. */
      {
	finished = 1;
      }
      break;
    default:
      printf("Unknown event (%i)\n", event);
      finished = 1;
      break;
    }
#if DVD_READ_CACHE
    dvdnav_free_cache_block(dvdnav, buf);
#endif
  }

  /* destroy dvdnav handle */
  if (dvdnav_close(dvdnav) != DVDNAV_STATUS_OK) {
    printf("Error on dvdnav_close: %s\n", dvdnav_err_to_string(dvdnav));
    return 5;
  }
  close(output_fd);

  return 0;
}
