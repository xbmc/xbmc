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
 * This is the main header file applications should include if they want
 * to access dvdnav functionality.
 */

#ifndef LIBDVDNAV_DVDNAV_H
#define LIBDVDNAV_DVDNAV_H

#define MP_DVDNAV 1

#ifdef __cplusplus
extern "C" {
#endif

#include <dvdnav/dvd_types.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/nav_types.h>
#include <dvdread/ifo_types.h> /* For vm_cmd_t */
#include <dvdnav/dvdnav_events.h>



/*********************************************************************
 * dvdnav data types                                                 *
 *********************************************************************/

/*
 * Opaque data-type can be viewed as a 'DVD handle'. You should get
 * a pointer to a dvdnav_t from the dvdnav_open() function.
 * Never call free() on the pointer, you have to give it back with
 * dvdnav_close().
 */
typedef struct dvdnav_s dvdnav_t;

/* Status as reported by most of libdvdnav's functions */
typedef int32_t dvdnav_status_t;

/*
 * Unless otherwise stated, all functions return DVDNAV_STATUS_OK if
 * they succeeded, otherwise DVDNAV_STATUS_ERR is returned and the error may
 * be obtained by calling dvdnav_err_to_string().
 */
#define DVDNAV_STATUS_ERR 0
#define DVDNAV_STATUS_OK  1

/*********************************************************************
 * initialisation & housekeeping functions                           *
 *********************************************************************/

/*
 * These functions allow you to open a DVD device and associate it
 * with a dvdnav_t.
 */

/*
 * Attempts to open the DVD drive at the specified path and pre-cache
 * the CSS-keys. libdvdread is used to access the DVD, so any source
 * supported by libdvdread can be given with "path". Currently,
 * libdvdread can access: DVD drives, DVD image files, DVD file-by-file
 * copies.
 *
 * The resulting dvdnav_t handle will be written to *dest.
 */
dvdnav_status_t dvdnav_open(dvdnav_t **dest, const char *path);

/*
 * Closes a dvdnav_t previously opened with dvdnav_open(), freeing any
 * memory associated with it.
 */
dvdnav_status_t dvdnav_close(dvdnav_t *self);

/*
 * Resets the DVD virtual machine and cache buffers.
 */
dvdnav_status_t dvdnav_reset(dvdnav_t *self);

/*
 * Fills a pointer with a value pointing to a string describing
 * the path associated with an open dvdnav_t. It assigns *path to NULL
 * on error.
 */
dvdnav_status_t dvdnav_path(dvdnav_t *self, const char **path);

/*
 * Returns a human-readable string describing the last error.
 */
const char* dvdnav_err_to_string(dvdnav_t *self);


/*********************************************************************
 * changing and reading DVD player characteristics                   *
 *********************************************************************/

/*
 * These functions allow you to manipulate the various global characteristics
 * of the DVD playback engine.
 */

/*
 * Sets the region mask (bit 0 set implies region 1, bit 1 set implies
 * region 2, etc) of the virtual machine. Generally you will only need to set
 * this if you are playing RCE discs which query the virtual machine as to its
 * region setting.
 *
 * This has _nothing_ to do with the region setting of the DVD drive.
 */
dvdnav_status_t dvdnav_set_region_mask(dvdnav_t *self, int32_t region_mask);

/*
 * Returns the region mask (bit 0 set implies region 1, bit 1 set implies
 * region 2, etc) of the virtual machine.
 *
 * This has _nothing_ to do with the region setting of the DVD drive.
 */
dvdnav_status_t dvdnav_get_region_mask(dvdnav_t *self, int32_t *region_mask);

/*
 * Specify whether read-ahead caching should be used. You may not want this if your
 * decoding engine does its own buffering.
 *
 * The default read-ahead cache does not use an additional thread for the reading
 * (see read_cache.c for a threaded cache, but note that this code is currently
 * unmaintained). It prebuffers on VOBU level by reading ahead several buffers
 * on every read request. The speed of this prebuffering has been optimized to
 * also work on slow DVD drives.
 *
 * If in addition you want to prevent memcpy's to improve performance, have a look
 * at dvdnav_get_next_cache_block().
 */
dvdnav_status_t dvdnav_set_readahead_flag(dvdnav_t *self, int32_t read_ahead_flag);

/*
 * Query whether read-ahead caching/buffering will be used.
 */
dvdnav_status_t dvdnav_get_readahead_flag(dvdnav_t *self, int32_t *read_ahead_flag);

/*
 * Specify whether the positioning works PGC or PG based.
 * Programs (PGs) on DVDs are similar to Chapters and a program chain (PGC)
 * usually covers a whole feature. This affects the behaviour of the
 * functions dvdnav_get_position() and dvdnav_sector_search(). See there.
 * Default is PG based positioning.
 */
dvdnav_status_t dvdnav_set_PGC_positioning_flag(dvdnav_t *self, int32_t pgc_based_flag);

/*
 * Query whether positioning is PG or PGC based.
 */
dvdnav_status_t dvdnav_get_PGC_positioning_flag(dvdnav_t *self, int32_t *pgc_based_flag);


/*********************************************************************
 * reading data                                                      *
 *********************************************************************/

/*
 * These functions are used to poll the playback engine and actually get data
 * off the DVD.
 */

/*
 * Attempts to get the next block off the DVD and copies it into the buffer 'buf'.
 * If there is any special actions that may need to be performed, the value
 * pointed to by 'event' gets set accordingly.
 *
 * If 'event' is DVDNAV_BLOCK_OK then 'buf' is filled with the next block
 * (note that means it has to be at /least/ 2048 bytes big). 'len' is
 * then set to 2048.
 *
 * Otherwise, buf is filled with an appropriate event structure and
 * len is set to the length of that structure.
 *
 * See the dvdnav_events.h header for information on the various events.
 */
dvdnav_status_t dvdnav_get_next_block(dvdnav_t *self, uint8_t *buf,
				      int32_t *event, int32_t *len);

/*
 * This basically does the same as dvdnav_get_next_block. The only difference is
 * that it avoids a memcopy, when the requested block was found in the cache.
 * In such a case (cache hit) this function will return a different pointer than
 * the one handed in, pointing directly into the relevant block in the cache.
 * Those pointers must _never_ be freed but instead returned to the library via
 * dvdnav_free_cache_block().
 */
dvdnav_status_t dvdnav_get_next_cache_block(dvdnav_t *self, uint8_t **buf,
					    int32_t *event, int32_t *len);

/*
 * All buffers which came from the internal cache (when dvdnav_get_next_cache_block()
 * returned a buffer different from the one handed in) have to be freed with this
 * function. Although handing in other buffers not from the cache doesn't cause any harm.
 */
dvdnav_status_t dvdnav_free_cache_block(dvdnav_t *self, unsigned char *buf);

/*
 * If we are currently in a still-frame this function skips it.
 *
 * See also the DVDNAV_STILL_FRAME event.
 */
dvdnav_status_t dvdnav_still_skip(dvdnav_t *self);

/*
 * If we are currently in WAIT state, that is: the application is required to
 * wait for its fifos to become empty, calling this signals libdvdnav that this
 * is achieved and that it can continue.
 *
 * See also the DVDNAV_WAIT event.
 */
dvdnav_status_t dvdnav_wait_skip(dvdnav_t *self);

/*
 * Returns the still time from the currently playing cell.
 * The still time is given in seconds with 0xff meaning an indefinite still.
 *
 * This function can be used to detect still frames before they are reached.
 * Some players might need this to prepare for a frame to be shown for a
 * longer time than usual.
 */
uint32_t dvdnav_get_next_still_flag(dvdnav_t *self);

/*
 * Stops playback. The next event obtained with one of the get_next_block
 * functions will be a DVDNAV_STOP event.
 *
 * It is not required to call this before dvdnav_close().
 */
dvdnav_status_t dvdnav_stop(dvdnav_t *self);


/*********************************************************************
 * title/part navigation                                             *
 *********************************************************************/

/*
 * Returns the number of titles on the disk.
 */
dvdnav_status_t dvdnav_get_number_of_titles(dvdnav_t *self, int32_t *titles);

/*
 * Returns the number of parts within the given title.
 */
dvdnav_status_t dvdnav_get_number_of_parts(dvdnav_t *self, int32_t title, int32_t *parts);

/*
 * Plays the specified title of the DVD from its beginning (that is: part 1).
 */
dvdnav_status_t dvdnav_title_play(dvdnav_t *self, int32_t title);

/*
 * Plays the specified title, starting from the specified part.
 */
dvdnav_status_t dvdnav_part_play(dvdnav_t *self, int32_t title, int32_t part);

/*
 * Plays the specified title, starting from the specified program
 */
dvdnav_status_t dvdnav_program_play(dvdnav_t *self, int32_t title, int32_t pgcn, int32_t pgn);

/*
 * Stores in *times an array (that the application *must* free) of
 * dvdtimes corresponding to the chapter times for the chosen title.
 * *duration will have the duration of the title
 * The number of entries in *times is the result of the function.
 * On error *times is NULL and the output is 0
 */
uint32_t dvdnav_describe_title_chapters(dvdnav_t *self, int32_t title, uint64_t **times, uint64_t *duration);

/*
 * Play the specified amount of parts of the specified title of
 * the DVD then STOP.
 *
 * Currently unimplemented!
 */
dvdnav_status_t dvdnav_part_play_auto_stop(dvdnav_t *self, int32_t title,
					   int32_t part, int32_t parts_to_play);

/*
 * Play the specified title starting from the specified time.
 *
 * Currently unimplemented!
 */
dvdnav_status_t dvdnav_time_play(dvdnav_t *self, int32_t title,
				 uint64_t time);

/*
 * Stop playing the current position and jump to the specified menu.
 *
 * See also DVDMenuID_t from libdvdread
 */
dvdnav_status_t dvdnav_menu_call(dvdnav_t *self, DVDMenuID_t menu);

/*
 * Return the title number and part currently being played.
 * A title of 0 indicates we are in a menu. In this case, part
 * is set to the current menu's ID.
 */
dvdnav_status_t dvdnav_current_title_info(dvdnav_t *self, int32_t *title,
					  int32_t *part);

/*
 * Return the title number, pgcn and pgn currently being played.
 * A title of 0 indicates, we are in a menu.
 */
dvdnav_status_t dvdnav_current_title_program(dvdnav_t *self, int32_t *title,
					  int32_t *pgcn, int32_t *pgn);

/*
 * Return the current position (in blocks) within the current
 * title and the length (in blocks) of said title.
 *
 * Current implementation is wrong and likely to behave unpredictably!
 * Use is discouraged!
 */
dvdnav_status_t dvdnav_get_position_in_title(dvdnav_t *self,
					     uint32_t *pos,
					     uint32_t *len);

/*
 * This function is only available for compatibility reasons.
 *
 * Stop playing the current position and start playback of the current title
 * from the specified part.
 */
dvdnav_status_t dvdnav_part_search(dvdnav_t *self, int32_t part);


/*********************************************************************
 * program chain/program navigation                                  *
 *********************************************************************/

/*
 * Stop playing the current position and start playback from the last
 * VOBU boundary before the given sector. The sector number is not
 * meant to be an absolute physical DVD sector, but a relative sector
 * in the current program. This function cannot leave the current
 * program and will fail if asked to do so.
 *
 * If program chain based positioning is enabled
 * (see dvdnav_set_PGC_positioning_flag()), this will seek to the relative
 * sector inside the current program chain.
 *
 * 'origin' can be one of SEEK_SET, SEEK_CUR, SEEK_END as defined in
 * fcntl.h.
 */
dvdnav_status_t dvdnav_sector_search(dvdnav_t *self,
				     uint64_t offset, int32_t origin);

/*
 returns the current stream time in PTS ticks as reported by the IFO structures
 divide it by 90000 to get the current play time in seconds
 */
int64_t dvdnav_get_current_time(dvdnav_t *self);

/*
 * Find the nearest vobu and jump to it
 *
 * Alternative to dvdnav_time_search
 */
dvdnav_status_t dvdnav_jump_to_sector_by_time(dvdnav_t *this,
            uint64_t time_in_pts_ticks, int32_t mode);

/*
 * Stop playing the current position and start playback of the title
 * from the specified timecode.
 *
 * Currently implemented using interpolation, which is slightly inaccurate.
 */
dvdnav_status_t dvdnav_time_search(dvdnav_t *self,
				   uint64_t time);

/*
 * Stop playing current position and play the "GoUp"-program chain.
 * (which generally leads to the title menu or a higher-level menu).
 */
dvdnav_status_t dvdnav_go_up(dvdnav_t *self);

/*
 * Stop playing the current position and start playback at the
 * previous program (if it exists).
 */
dvdnav_status_t dvdnav_prev_pg_search(dvdnav_t *self);

/*
 * Stop playing the current position and start playback at the
 * first program.
 */
dvdnav_status_t dvdnav_top_pg_search(dvdnav_t *self);

/*
 * Stop playing the current position and start playback at the
 * next program (if it exists).
 */
dvdnav_status_t dvdnav_next_pg_search(dvdnav_t *self);

/*
 * Return the current position (in blocks) within the current
 * program and the length (in blocks) of current program.
 *
 * If program chain based positioning is enabled
 * (see dvdnav_set_PGC_positioning_flag()), this will return the
 * relative position in and the length of the current program chain.
 */
dvdnav_status_t dvdnav_get_position(dvdnav_t *self, uint32_t *pos,
				    uint32_t *len);


/*********************************************************************
 * menu highlights                                                   *
 *********************************************************************/

/*
 * Most functions related to highlights take a NAV PCI packet as a parameter.
 * While you can get such a packet from libdvdnav, this will result in
 * errors for players with internal FIFOs because due to the FIFO length,
 * libdvdnav will be ahead in the stream compared to what the user is
 * seeing on screen.  Therefore, player applications who have a NAV
 * packet available, which is better in sync with the actual playback,
 * should always pass this one to these functions.
 */

/*
 * Get the currently highlighted button
 * number (1..36) or 0 if no button is highlighted.
 */
dvdnav_status_t dvdnav_get_current_highlight(dvdnav_t *self, int32_t *button);

/*
 * Returns the Presentation Control Information (PCI) structure associated
 * with the current position.
 *
 * Read the general notes above.
 * See also libdvdreads nav_types.h for definition of pci_t.
 */
pci_t* dvdnav_get_current_nav_pci(dvdnav_t *self);

/*
 * Returns the DSI (data search information) structure associated
 * with the current position.
 *
 * Read the general notes above.
 * See also libdvdreads nav_types.h for definition of dsi_t.
 */
dsi_t* dvdnav_get_current_nav_dsi(dvdnav_t *self);

/*
 * Get the area associated with a certain button.
 */
dvdnav_status_t dvdnav_get_highlight_area(pci_t *nav_pci , int32_t button, int32_t mode,
					  dvdnav_highlight_area_t *highlight);

/*
 * Move button highlight around as suggested by function name (e.g. with arrow keys).
 */
dvdnav_status_t dvdnav_upper_button_select(dvdnav_t *self, pci_t *pci);
dvdnav_status_t dvdnav_lower_button_select(dvdnav_t *self, pci_t *pci);
dvdnav_status_t dvdnav_right_button_select(dvdnav_t *self, pci_t *pci);
dvdnav_status_t dvdnav_left_button_select(dvdnav_t *self, pci_t *pci);

/*
 * Activate ("press") the currently highlighted button.
 */
dvdnav_status_t dvdnav_button_activate(dvdnav_t *self, pci_t *pci);

/*
 * Highlight a specific button.
 */
dvdnav_status_t dvdnav_button_select(dvdnav_t *self, pci_t *pci, int32_t button);

/*
 * Activate ("press") specified button.
 */
dvdnav_status_t dvdnav_button_select_and_activate(dvdnav_t *self, pci_t *pci, int32_t button);

/*
 * Activate ("press") a button and execute specified command.
 */
dvdnav_status_t dvdnav_button_activate_cmd(dvdnav_t *self, int32_t button, vm_cmd_t *cmd);

/*
 * Select button at specified video frame coordinates.
 */
dvdnav_status_t dvdnav_mouse_select(dvdnav_t *self, pci_t *pci, int32_t x, int32_t y);

/*
 * Activate ("press") button at specified video frame coordinates.
 */
dvdnav_status_t dvdnav_mouse_activate(dvdnav_t *self, pci_t *pci, int32_t x, int32_t y);


/*********************************************************************
 * languages                                                         *
 *********************************************************************/

/*
 * The language codes expected by these functions are two character
 * codes as defined in ISO639.
 */

/*
 * Set which menu language we should use per default.
 */
dvdnav_status_t dvdnav_menu_language_select(dvdnav_t *self,
					   char *code);

/*
 * Set which audio language we should use per default.
 */
dvdnav_status_t dvdnav_audio_language_select(dvdnav_t *self,
					    char *code);

/*
 * Set which spu language we should use per default.
 */
dvdnav_status_t dvdnav_spu_language_select(dvdnav_t *self,
					  char *code);


/*********************************************************************
 * obtaining stream attributes                                       *
 *********************************************************************/

/*
 * Return a string describing the title of the DVD.
 * This is an ID string encoded on the disc by the author. In many cases
 * this is a descriptive string such as `THE_MATRIX' but sometimes is singularly
 * uninformative such as `PDVD-011421'. Some DVD authors even forget to set this,
 * so you may also read the default of the authoring software they used, like
 * `DVDVolume'.
 */
dvdnav_status_t dvdnav_get_title_string(dvdnav_t *self, const char **title_str);

/*
 * Returns a string containing the serial number of the DVD.
 * This has a max of 15 characters and should be more unique than the
 * title string.
 */
dvdnav_status_t dvdnav_get_serial_string(dvdnav_t *self, const char **serial_str);

/*
 * Get video aspect code.
 * The aspect code does only change on VTS boundaries.
 * See the DVDNAV_VTS_CHANGE event.
 *
 * 0 -- 4:3, 2 -- 16:9
 */
uint8_t dvdnav_get_video_aspect(dvdnav_t *self);

/*
 * Get video resolution.
 */
int dvdnav_get_video_resolution(dvdnav_t *self, uint32_t *width, uint32_t *height);

/*
 * Get video scaling permissions.
 * The scaling permission does only change on VTS boundaries.
 * See the DVDNAV_VTS_CHANGE event.
 *
 * bit0 set = deny letterboxing, bit1 set = deny pan&scan
 */
uint8_t dvdnav_get_video_scale_permission(dvdnav_t *self);

/*
 * Converts a *logical* audio stream id into language code
 * (returns 0xffff if no such stream).
 */
uint16_t dvdnav_audio_stream_to_lang(dvdnav_t *self, uint8_t stream);

/*
 * Returns the format of *logical* audio stream 'stream'
 * (returns 0xffff if no such stream).
 */
uint16_t dvdnav_audio_stream_format(dvdnav_t *self, uint8_t stream);

/*
 * Returns number of channels in *logical* audio stream 'stream'
 * (returns 0xffff if no such stream).
 */
uint16_t dvdnav_audio_stream_channels(dvdnav_t *self, uint8_t stream);

/*
 * Converts a *logical* subpicture stream id into country code
 * (returns 0xffff if no such stream).
 */
uint16_t dvdnav_spu_stream_to_lang(dvdnav_t *self, uint8_t stream);

/*
 * Converts a *physical* (MPEG) audio stream id into a logical stream number.
 */
int8_t dvdnav_get_audio_logical_stream(dvdnav_t *self, uint8_t audio_num);

#define HAVE_GET_AUDIO_ATTR
/*
 * Get audio attr
 */
dvdnav_status_t dvdnav_get_audio_attr(dvdnav_t *self, uint8_t audio_mum, audio_attr_t *audio_attr);

/*
 * Converts a *physical* (MPEG) subpicture stream id into a logical stream number.
 */
int8_t dvdnav_get_spu_logical_stream(dvdnav_t *self, uint8_t subp_num);

#define HAVE_GET_SPU_ATTR
/*
 * Get spu attr
 */
dvdnav_status_t dvdnav_get_spu_attr(dvdnav_t *self, uint8_t audio_mum, subp_attr_t *subp_attr);

/*
 * Get active audio stream.
 */
int8_t dvdnav_get_active_audio_stream(dvdnav_t *self);

/*
 * Get active spu stream.
 */
int8_t dvdnav_get_active_spu_stream(dvdnav_t *self);

/*
 * Get the set of user operations that are currently prohibited.
 * There are potentially new restrictions right after
 * DVDNAV_CHANNEL_HOP and DVDNAV_NAV_PACKET.
 */
user_ops_t dvdnav_get_restrictions(dvdnav_t *self);


/*********************************************************************
 * multiple angles                                                   *
 *********************************************************************/

/*
 * The libdvdnav library abstracts away the difference between seamless and
 * non-seamless angles. From the point of view of the programmer you just set the
 * angle number and all is well in the world. You will always see only the
 * selected angle coming from the get_next_block functions.
 *
 * Note:
 * It is quite possible that some tremendously strange DVD feature might change the
 * angle number from under you. Generally you should always view the results from
 * dvdnav_get_angle_info() as definitive only up to the next time you call
 * dvdnav_get_next_block().
 */

/*
 * Sets the current angle. If you try to follow a non existent angle
 * the call fails.
 */
dvdnav_status_t dvdnav_angle_change(dvdnav_t *self, int32_t angle);

/*
 * Returns the current angle and number of angles present.
 */
dvdnav_status_t dvdnav_get_angle_info(dvdnav_t *self, int32_t *current_angle,
				      int32_t *number_of_angles);

/*********************************************************************
 * domain queries                                                    *
 *********************************************************************/

/*
 * Are we in the First Play domain?
 */
int8_t dvdnav_is_domain_fp(dvdnav_t *self);

/*
 * Are we in the Video management Menu domain?
 */
int8_t dvdnav_is_domain_vmgm(dvdnav_t *self);

/*
 * Are we in the Video Title Menu domain?
 */
int8_t dvdnav_is_domain_vtsm(dvdnav_t *self);

/*
 * Are we in the Video Title Set domain?
 */
int8_t dvdnav_is_domain_vts(dvdnav_t *self);

void dvdnav_free(void* pdata);

#ifdef __cplusplus
}
#endif

#endif /* LIBDVDNAV_DVDNAV_H */
