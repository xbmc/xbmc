/*
 *	wiiuse
 *
 *	Written By:
 *		Michael Laforest	< para >
 *		Email: < thepara (--AT--) g m a i l [--DOT--] com >
 *
 *	Copyright 2006-2007
 *
 *	This file is part of wiiuse.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	$Header$
 *
 */

/**
 *	@file
 *	@brief Handles wiimote events.
 *
 *	The file includes functions that handle the events
 *	that are sent from the wiimote to us.
 */

#include <stdio.h>

#ifndef WIN32
	#include <sys/time.h>
	#include <unistd.h>
	#include <errno.h>
#else
	#include <winsock2.h>
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <math.h>

#include "definitions.h"
#include "io.h"
#include "wiiuse_internal.h"
#include "dynamics.h"
#include "ir.h"
#include "nunchuk.h"
#include "classic.h"
#include "guitar_hero_3.h"
#include "events.h"

static void idle_cycle(struct wiimote_t* wm);
static void clear_dirty_reads(struct wiimote_t* wm);
static void propagate_event(struct wiimote_t* wm, byte event, byte* msg);
static void event_data_read(struct wiimote_t* wm, byte* msg);
static void event_status(struct wiimote_t* wm, byte* msg);
static void handle_expansion(struct wiimote_t* wm, byte* msg);

static void save_state(struct wiimote_t* wm);
static int state_changed(struct wiimote_t* wm);

/**
 *	@brief Poll the wiimotes for any events.
 *
 *	@param wm		An array of pointers to wiimote_t structures.
 *	@param wiimotes	The number of wiimote_t structures in the \a wm array.
 *
 *	@return Returns number of wiimotes that an event has occurred on.
 *
 *	It is necessary to poll the wiimote devices for events
 *	that occur.  If an event occurs on a particular wiimote,
 *	the event variable will be set.
 */
int wiiuse_poll(struct wiimote_t** wm, int wiimotes) {
	int evnt = 0;

	#ifndef WIN32
		/*
		 *	*nix
		 */
		struct timeval tv;
		fd_set fds;
		int r;
		int i;
		int highest_fd = -1;

		if (!wm) return 0;

		/* block select() for 1/2000th of a second */
		tv.tv_sec = 0;
		tv.tv_usec = 500;

		FD_ZERO(&fds);

		for (i = 0; i < wiimotes; ++i) {
			/* only poll it if it is connected */
			if (WIIMOTE_IS_SET(wm[i], WIIMOTE_STATE_CONNECTED)) {
				FD_SET(wm[i]->in_sock, &fds);

				/* find the highest fd of the connected wiimotes */
				if (wm[i]->in_sock > highest_fd)
					highest_fd = wm[i]->in_sock;
			}

			wm[i]->event = WIIUSE_NONE;
		}

		if (highest_fd == -1)
			/* nothing to poll */
			return 0;

		if (select(highest_fd + 1, &fds, NULL, NULL, &tv) == -1) {
			WIIUSE_ERROR("Unable to select() the wiimote interrupt socket(s).");
			perror("Error Details");
			return 0;
		}

		/* check each socket for an event */
		for (i = 0; i < wiimotes; ++i) {
			/* if this wiimote is not connected, skip it */
			if (!WIIMOTE_IS_CONNECTED(wm[i]))
				continue;

			if (FD_ISSET(wm[i]->in_sock, &fds)) {
				/* clear out the event buffer */
				memset(wm[i]->event_buf, 0, sizeof(wm[i]->event_buf));

				/* clear out any old read requests */
				clear_dirty_reads(wm[i]);

				/* read the pending message into the buffer */
				r = read(wm[i]->in_sock, wm[i]->event_buf, sizeof(wm[i]->event_buf));
				if (r == -1) {
					/* error reading data */
					WIIUSE_ERROR("Receiving wiimote data (id %i).", wm[i]->unid);
					perror("Error Details");

					if (errno == ENOTCONN) {
						/* this can happen if the bluetooth dongle is disconnected */
						WIIUSE_ERROR("Bluetooth appears to be disconnected.  Wiimote unid %i will be disconnected.", wm[i]->unid);
						wiiuse_disconnect(wm[i]);
						wm[i]->event = WIIUSE_UNEXPECTED_DISCONNECT;
					}

					continue;
				}
				if (!r) {
					/* remote disconnect */
					wiiuse_disconnected(wm[i]);
					evnt = 1;
					continue;
				}

				/* propagate the event */
				propagate_event(wm[i], wm[i]->event_buf[1], wm[i]->event_buf+2);
				evnt += (wm[i]->event != WIIUSE_NONE);
			} else {
				idle_cycle(wm[i]);
			}
		}
	#else
		/*
		 *	Windows
		 */
		int i;

		if (!wm) return 0;

		for (i = 0; i < wiimotes; ++i) {
			wm[i]->event = WIIUSE_NONE;

			if (wiiuse_io_read(wm[i])) {
				/* propagate the event */
				propagate_event(wm[i], wm[i]->event_buf[0], wm[i]->event_buf+1);
				evnt += (wm[i]->event != WIIUSE_NONE);

				/* clear out the event buffer */
				memset(wm[i]->event_buf, 0, sizeof(wm[i]->event_buf));
			} else {
				idle_cycle(wm[i]);
			}
		}
	#endif

	return evnt;
}


/**
 *	@brief Called on a cycle where no significant change occurs.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 */
static void idle_cycle(struct wiimote_t* wm) {
	/*
	 *	Smooth the angles.
	 *
	 *	This is done to make sure that on every cycle the orientation
	 *	angles are smoothed.  Normally when an event occurs the angles
	 *	are updated and smoothed, but if no packet comes in then the
	 *	angles remain the same.  This means the angle wiiuse reports
	 *	is still an old value.  Smoothing needs to be applied in this
	 *	case in order for the angle it reports to converge to the true
	 *	angle of the device.
	 */
	if (WIIUSE_USING_ACC(wm) && WIIMOTE_IS_FLAG_SET(wm, WIIUSE_SMOOTHING)) {
		apply_smoothing(&wm->accel_calib, &wm->orient, SMOOTH_ROLL);
		apply_smoothing(&wm->accel_calib, &wm->orient, SMOOTH_PITCH);
	}

	/* clear out any old read requests */
	clear_dirty_reads(wm);
}


/**
 *	@brief Clear out all old 'dirty' read requests.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 */
static void clear_dirty_reads(struct wiimote_t* wm) {
	struct read_req_t* req = wm->read_req;

	while (req && req->dirty) {
		WIIUSE_DEBUG("Cleared old read request for address: %x", req->addr);

		wm->read_req = req->next;
		free(req);
		req = wm->read_req;
	}
}


/**
 *	@brief Analyze the event that occurred on a wiimote.
 *
 *	@param wm		An array of pointers to wiimote_t structures.
 *	@param event	The event that occurred.
 *	@param msg		The message specified in the event packet.
 *
 *	Pass the event to the registered event callback.
 */
static void propagate_event(struct wiimote_t* wm, byte event, byte* msg) {
	save_state(wm);

	switch (event) {
		case WM_RPT_BTN:
		{
			/* button */
			wiiuse_pressed_buttons(wm, msg);
			break;
		}
		case WM_RPT_BTN_ACC:
		{
			/* button - motion */
			wiiuse_pressed_buttons(wm, msg);

			wm->accel.x = msg[2];
			wm->accel.y = msg[3];
			wm->accel.z = msg[4];

			/* calculate the remote orientation */
			calculate_orientation(&wm->accel_calib, &wm->accel, &wm->orient, WIIMOTE_IS_FLAG_SET(wm, WIIUSE_SMOOTHING));

			/* calculate the gforces on each axis */
			calculate_gforce(&wm->accel_calib, &wm->accel, &wm->gforce);

			break;
		}
		case WM_RPT_READ:
		{
			/* data read */
			event_data_read(wm, msg);

			/* yeah buttons may be pressed, but this wasn't an "event" */
			return;
		}
		case WM_RPT_CTRL_STATUS:
		{
			/* controller status */
			event_status(wm, msg);

			/* don't execute the event callback */
			return;
		}
		case WM_RPT_BTN_EXP:
		{
			/* button - expansion */
			wiiuse_pressed_buttons(wm, msg);
			handle_expansion(wm, msg+2);

			break;
		}
		case WM_RPT_BTN_ACC_EXP:
		{
			/* button - motion - expansion */
			wiiuse_pressed_buttons(wm, msg);

			wm->accel.x = msg[2];
			wm->accel.y = msg[3];
			wm->accel.z = msg[4];

			calculate_orientation(&wm->accel_calib, &wm->accel, &wm->orient, WIIMOTE_IS_FLAG_SET(wm, WIIUSE_SMOOTHING));
			calculate_gforce(&wm->accel_calib, &wm->accel, &wm->gforce);

			handle_expansion(wm, msg+5);

			break;
		}
		case WM_RPT_BTN_ACC_IR:
		{
			/* button - motion - ir */
			wiiuse_pressed_buttons(wm, msg);

			wm->accel.x = msg[2];
			wm->accel.y = msg[3];
			wm->accel.z = msg[4];

			calculate_orientation(&wm->accel_calib, &wm->accel, &wm->orient, WIIMOTE_IS_FLAG_SET(wm, WIIUSE_SMOOTHING));
			calculate_gforce(&wm->accel_calib, &wm->accel, &wm->gforce);

			/* ir */
			calculate_extended_ir(wm, msg+5);

			break;
		}
		case WM_RPT_BTN_IR_EXP:
		{
			/* button - ir - expansion */
			wiiuse_pressed_buttons(wm, msg);
			handle_expansion(wm, msg+12);

			/* ir */
			calculate_basic_ir(wm, msg+2);

			break;
		}
		case WM_RPT_BTN_ACC_IR_EXP:
		{
			/* button - motion - ir - expansion */
			wiiuse_pressed_buttons(wm, msg);

			wm->accel.x = msg[2];
			wm->accel.y = msg[3];
			wm->accel.z = msg[4];

			calculate_orientation(&wm->accel_calib, &wm->accel, &wm->orient, WIIMOTE_IS_FLAG_SET(wm, WIIUSE_SMOOTHING));
			calculate_gforce(&wm->accel_calib, &wm->accel, &wm->gforce);

			handle_expansion(wm, msg+15);

			/* ir */
			calculate_basic_ir(wm, msg+5);

			break;
		}
		case WM_RPT_WRITE:
		{
			/* write feedback - safe to skip */
			break;
		}
		default:
		{
			WIIUSE_WARNING("Unknown event, can not handle it [Code 0x%x].", event);
			return;
		}
	}

	/* was there an event? */
	if (state_changed(wm))
		wm->event = WIIUSE_EVENT;
}


/**
 *	@brief Find what buttons are pressed.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param msg		The message specified in the event packet.
 */
void wiiuse_pressed_buttons(struct wiimote_t* wm, byte* msg) {
	short now;

	/* convert to big endian */
	now = BIG_ENDIAN_SHORT(*(short*)msg) & WIIMOTE_BUTTON_ALL;

	/* pressed now & were pressed, then held */
	wm->btns_held = (now & wm->btns);

	/* were pressed or were held & not pressed now, then released */
	wm->btns_released = ((wm->btns | wm->btns_held) & ~now);

	/* buttons pressed now */
	wm->btns = now;
}


/**
 *	@brief Received a data packet from a read request.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param msg		The message specified in the event packet.
 *
 *	Data from the wiimote comes in packets.  If the requested
 *	data segment size is bigger than one packet can hold then
 *	several packets will be received.  These packets are first
 *	reassembled into one, then the registered callback function
 *	that handles data reads is invoked.
 */
static void event_data_read(struct wiimote_t* wm, byte* msg) {
	/* we must always assume the packet received is from the most recent request */
	byte err;
	byte len;
	unsigned short offset;
	struct read_req_t* req = wm->read_req;

	wiiuse_pressed_buttons(wm, msg);

	/* find the next non-dirty request */
	while (req && req->dirty)
		req = req->next;

	/* if we don't have a request out then we didn't ask for this packet */
	if (!req) {
		WIIUSE_WARNING("Received data packet when no request was made.");
		return;
	}

	err = msg[2] & 0x0F;

	if (err == 0x08)
		WIIUSE_WARNING("Unable to read data - address does not exist.");
	else if (err == 0x07)
		WIIUSE_WARNING("Unable to read data - address is for write-only registers.");
	else if (err)
		WIIUSE_WARNING("Unable to read data - unknown error code %x.", err);

	if (err) {
		/* this request errored out, so skip it and go to the next one */

		/* delete this request */
		wm->read_req = req->next;
		free(req);

		/* if another request exists send it to the wiimote */
		if (wm->read_req)
			wiiuse_send_next_pending_read_request(wm);

		return;
	}

	len = ((msg[2] & 0xF0) >> 4) + 1;
	offset = BIG_ENDIAN_SHORT(*(unsigned short*)(msg + 3));
	req->addr = (req->addr & 0xFFFF);

	req->wait -= len;
	if (req->wait >= req->size)
		/* this should never happen */
		req->wait = 0;

	WIIUSE_DEBUG("Received read packet:");
	WIIUSE_DEBUG("    Packet read offset:   %i bytes", offset);
	WIIUSE_DEBUG("    Request read offset:  %i bytes", req->addr);
	WIIUSE_DEBUG("    Read offset into buf: %i bytes", offset - req->addr);
	WIIUSE_DEBUG("    Read data size:       %i bytes", len);
	WIIUSE_DEBUG("    Still need:           %i bytes", req->wait);

	/* reconstruct this part of the data */
	memcpy((req->buf + offset - req->addr), (msg + 5), len);

	#ifdef WITH_WIIUSE_DEBUG
	{
		int i = 0;
		printf("Read: ");
		for (; i < req->size - req->wait; ++i)
			printf("%x ", req->buf[i]);
		printf("\n");
	}
	#endif

	/* if all data has been received, execute the read event callback or generate event */
	if (!req->wait) {
		if (req->cb) {
			/* this was a callback, so invoke it now */
			req->cb(wm, req->buf, req->size);

			/* delete this request */
			wm->read_req = req->next;
			free(req);
		} else {
			/*
			 *	This should generate an event.
			 *	We need to leave the event in the array so the client
			 *	can access it still.  We'll flag is as being 'dirty'
			 *	and give the client one cycle to use it.  Next event
			 *	we will remove it from the list.
			 */
			wm->event = WIIUSE_READ_DATA;
			req->dirty = 1;
		}

		/* if another request exists send it to the wiimote */
		if (wm->read_req)
			wiiuse_send_next_pending_read_request(wm);
	}
}


/**
 *	@brief Read the controller status.
 *
 *	@param wm		Pointer to a wiimote_t structure.
 *	@param msg		The message specified in the event packet.
 *
 *	Read the controller status and execute the registered status callback.
 */
static void event_status(struct wiimote_t* wm, byte* msg) {
	int led[4] = {0};
	int attachment = 0;
	int ir = 0;
	int exp_changed = 0;

	/*
	 *	An event occurred.
	 *	This event can be overwritten by a more specific
	 *	event type during a handshake or expansion removal.
	 */
	wm->event = WIIUSE_STATUS;

	wiiuse_pressed_buttons(wm, msg);

	/* find what LEDs are lit */
	if (msg[2] & WM_CTRL_STATUS_BYTE1_LED_1)	led[0] = 1;
	if (msg[2] & WM_CTRL_STATUS_BYTE1_LED_2)	led[1] = 1;
	if (msg[2] & WM_CTRL_STATUS_BYTE1_LED_3)	led[2] = 1;
	if (msg[2] & WM_CTRL_STATUS_BYTE1_LED_4)	led[3] = 1;

	/* is an attachment connected to the expansion port? */
	if ((msg[2] & WM_CTRL_STATUS_BYTE1_ATTACHMENT) == WM_CTRL_STATUS_BYTE1_ATTACHMENT)
		attachment = 1;

	/* is the speaker enabled? */
	if ((msg[2] & WM_CTRL_STATUS_BYTE1_SPEAKER_ENABLED) == WM_CTRL_STATUS_BYTE1_SPEAKER_ENABLED)
		WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_SPEAKER);

	/* is IR sensing enabled? */
	if ((msg[2] & WM_CTRL_STATUS_BYTE1_IR_ENABLED) == WM_CTRL_STATUS_BYTE1_IR_ENABLED)
		ir = 1;

	/* find the battery level and normalize between 0 and 1 */
	wm->battery_level = (msg[5] / (float)WM_MAX_BATTERY_CODE);

	/* expansion port */
	if (attachment && !WIIMOTE_IS_SET(wm, WIIMOTE_STATE_EXP)) {
		/* send the initialization code for the attachment */
		handshake_expansion(wm, NULL, 0);
		exp_changed = 1;
	} else if (!attachment && WIIMOTE_IS_SET(wm, WIIMOTE_STATE_EXP)) {
		/* attachment removed */
		disable_expansion(wm);
		exp_changed = 1;
	}

	#ifdef WIN32
	if (!attachment) {
		WIIUSE_DEBUG("Setting timeout to normal %i ms.", wm->normal_timeout);
		wm->timeout = wm->normal_timeout;
	}
	#endif

	/*
	 *	From now on the remote will only send status packets.
	 *	We need to send a WIIMOTE_CMD_REPORT_TYPE packet to
	 *	reenable other incoming reports.
	 */
	if (exp_changed && WIIMOTE_IS_SET(wm, WIIMOTE_STATE_IR)) {
		/*
		 *	Since the expansion status changed IR needs to
		 *	be reset for the new IR report mode.
		 */
		WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_IR);
		wiiuse_set_ir(wm, 1);
	} else
		wiiuse_set_report_type(wm);
}


/**
 *	@brief Handle data from the expansion.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param msg		The message specified in the event packet for the expansion.
 */
static void handle_expansion(struct wiimote_t* wm, byte* msg) {
	switch (wm->exp.type) {
		case EXP_NUNCHUK:
			nunchuk_event(&wm->exp.nunchuk, msg);
			break;
		case EXP_CLASSIC:
			classic_ctrl_event(&wm->exp.classic, msg);
			break;
		case EXP_GUITAR_HERO_3:
			guitar_hero_3_event(&wm->exp.gh3, msg);
			break;
		default:
			break;
	}
}


/**
 *	@brief Handle the handshake data from the expansion device.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param data		The data read in from the device.
 *	@param len		The length of the data block, in bytes.
 *
 *	Tries to determine what kind of expansion was attached
 *	and invoke the correct handshake function.
 *
 *	If the data is NULL then this function will try to start
 *	a handshake with the expansion.
 */
void handshake_expansion(struct wiimote_t* wm, byte* data, unsigned short len) {
	int id;

	if (!data) {
		byte* handshake_buf;
		byte buf = 0x00;

		if (WIIMOTE_IS_SET(wm, WIIMOTE_STATE_EXP))
			disable_expansion(wm);

		/* increase the timeout until the handshake completes */
		#ifdef WIN32
		WIIUSE_DEBUG("Setting timeout to expansion %i ms.", wm->exp_timeout);
		wm->timeout = wm->exp_timeout;
		#endif

		wiiuse_write_data(wm, WM_EXP_MEM_ENABLE, &buf, 1);

		/* get the calibration data */
		handshake_buf = malloc(EXP_HANDSHAKE_LEN * sizeof(byte));
		wiiuse_read_data_cb(wm, handshake_expansion, handshake_buf, WM_EXP_MEM_CALIBR, EXP_HANDSHAKE_LEN);

		/* tell the wiimote to send expansion data */
		WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_EXP);

		return;
	}

	id = BIG_ENDIAN_LONG(*(int*)(data + 220));

	/* call the corresponding handshake function for this expansion */
	switch (id) {
		case EXP_ID_CODE_NUNCHUK:
		{
			if (nunchuk_handshake(wm, &wm->exp.nunchuk, data, len))
				wm->event = WIIUSE_NUNCHUK_INSERTED;
			break;
		}
		case EXP_ID_CODE_CLASSIC_CONTROLLER:
		{
			if (classic_ctrl_handshake(wm, &wm->exp.classic, data, len))
				wm->event = WIIUSE_CLASSIC_CTRL_INSERTED;
			break;
		}
		case EXP_ID_CODE_GUITAR:
		{
			if (guitar_hero_3_handshake(wm, &wm->exp.gh3, data, len))
				wm->event = WIIUSE_GUITAR_HERO_3_CTRL_INSERTED;
			break;
		}
		default:
		{
			WIIUSE_WARNING("Unknown expansion type. Code: 0x%x", id);
			break;
		}
	}

	free(data);
}



/**
 *	@brief Disable the expansion device if it was enabled.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param data		The data read in from the device.
 *	@param len		The length of the data block, in bytes.
 *
 *	If the data is NULL then this function will try to start
 *	a handshake with the expansion.
 */
void disable_expansion(struct wiimote_t* wm) {
	if (!WIIMOTE_IS_SET(wm, WIIMOTE_STATE_EXP))
		return;

	/* tell the assoicated module the expansion was removed */
	switch (wm->exp.type) {
		case EXP_NUNCHUK:
			nunchuk_disconnected(&wm->exp.nunchuk);
			wm->event = WIIUSE_NUNCHUK_REMOVED;
			break;
		case EXP_CLASSIC:
			classic_ctrl_disconnected(&wm->exp.classic);
			wm->event = WIIUSE_CLASSIC_CTRL_REMOVED;
			break;
		case EXP_GUITAR_HERO_3:
			guitar_hero_3_disconnected(&wm->exp.gh3);
			wm->event = WIIUSE_GUITAR_HERO_3_CTRL_REMOVED;
			break;
		default:
			break;
	}

	WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_EXP);
	wm->exp.type = EXP_NONE;
}


/**
 *	@brief Save important state data.
 *	@param wm	A pointer to a wiimote_t structure.
 */
static void save_state(struct wiimote_t* wm) {
	/* wiimote */
	wm->lstate.btns = wm->btns;
	wm->lstate.accel = wm->accel;

	/* ir */
	if (WIIUSE_USING_IR(wm)) {
		wm->lstate.ir_ax = wm->ir.ax;
		wm->lstate.ir_ay = wm->ir.ay;
		wm->lstate.ir_distance = wm->ir.distance;
	}

	/* expansion */
	switch (wm->exp.type) {
		case EXP_NUNCHUK:
			wm->lstate.exp_ljs_ang = wm->exp.nunchuk.js.ang;
			wm->lstate.exp_ljs_mag = wm->exp.nunchuk.js.mag;
			wm->lstate.exp_btns = wm->exp.nunchuk.btns;
			wm->lstate.exp_accel = wm->exp.nunchuk.accel;
			break;

		case EXP_CLASSIC:
			wm->lstate.exp_ljs_ang = wm->exp.classic.ljs.ang;
			wm->lstate.exp_ljs_mag = wm->exp.classic.ljs.mag;
			wm->lstate.exp_rjs_ang = wm->exp.classic.rjs.ang;
			wm->lstate.exp_rjs_mag = wm->exp.classic.rjs.mag;
			wm->lstate.exp_r_shoulder = wm->exp.classic.r_shoulder;
			wm->lstate.exp_l_shoulder = wm->exp.classic.l_shoulder;
			wm->lstate.exp_btns = wm->exp.classic.btns;
			break;

		case EXP_GUITAR_HERO_3:
			wm->lstate.exp_ljs_ang = wm->exp.gh3.js.ang;
			wm->lstate.exp_ljs_mag = wm->exp.gh3.js.mag;
			wm->lstate.exp_r_shoulder = wm->exp.gh3.whammy_bar;
			wm->lstate.exp_btns = wm->exp.gh3.btns;
			break;

		case EXP_NONE:
			break;
	}
}


/**
 *	@brief Determine if the current state differs significantly from the previous.
 *	@param wm	A pointer to a wiimote_t structure.
 *	@return	1 if a significant change occurred, 0 if not.
 */
static int state_changed(struct wiimote_t* wm) {
	#define STATE_CHANGED(a, b)		if (a != b)				return 1

	#define CROSS_THRESH(last, now, thresh)										\
				do {															\
					if (WIIMOTE_IS_FLAG_SET(wm, WIIUSE_ORIENT_THRESH)) {		\
						if ((diff_f(last.roll, now.roll) >= thresh) ||			\
							(diff_f(last.pitch, now.pitch) >= thresh) ||		\
							(diff_f(last.yaw, now.yaw) >= thresh))				\
						{														\
							last = now;											\
							return 1;											\
						}														\
					} else {													\
						if (last.roll != now.roll)		return 1;				\
						if (last.pitch != now.pitch)	return 1;				\
						if (last.yaw != now.yaw)		return 1;				\
					}															\
				} while (0)

	#define CROSS_THRESH_XYZ(last, now, thresh)									\
				do {															\
					if (WIIMOTE_IS_FLAG_SET(wm, WIIUSE_ORIENT_THRESH)) {		\
						if ((diff_f(last.x, now.x) >= thresh) ||				\
							(diff_f(last.y, now.y) >= thresh) ||				\
							(diff_f(last.z, now.z) >= thresh))					\
						{														\
							last = now;											\
							return 1;											\
						}														\
					} else {													\
						if (last.x != now.x)		return 1;					\
						if (last.y != now.y)		return 1;					\
						if (last.z != now.z)		return 1;					\
					}															\
				} while (0)

	/* ir */
	if (WIIUSE_USING_IR(wm)) {
		STATE_CHANGED(wm->lstate.ir_ax, wm->ir.ax);
		STATE_CHANGED(wm->lstate.ir_ay, wm->ir.ay);
		STATE_CHANGED(wm->lstate.ir_distance, wm->ir.distance);
	}

	/* accelerometer */
	if (WIIUSE_USING_ACC(wm)) {
		/* raw accelerometer */
		CROSS_THRESH_XYZ(wm->lstate.accel, wm->accel, wm->accel_threshold);

		/* orientation */
 		CROSS_THRESH(wm->lstate.orient, wm->orient, wm->orient_threshold);
	}

	/* expansion */
	switch (wm->exp.type) {
		case EXP_NUNCHUK:
		{
			STATE_CHANGED(wm->lstate.exp_ljs_ang, wm->exp.nunchuk.js.ang);
			STATE_CHANGED(wm->lstate.exp_ljs_mag, wm->exp.nunchuk.js.mag);
			STATE_CHANGED(wm->lstate.exp_btns, wm->exp.nunchuk.btns);

			CROSS_THRESH(wm->lstate.exp_orient, wm->exp.nunchuk.orient, wm->exp.nunchuk.orient_threshold);
			CROSS_THRESH_XYZ(wm->lstate.exp_accel, wm->exp.nunchuk.accel, wm->exp.nunchuk.accel_threshold);
			break;
		}
		case EXP_CLASSIC:
		{
			STATE_CHANGED(wm->lstate.exp_ljs_ang, wm->exp.classic.ljs.ang);
			STATE_CHANGED(wm->lstate.exp_ljs_mag, wm->exp.classic.ljs.mag);
			STATE_CHANGED(wm->lstate.exp_rjs_ang, wm->exp.classic.rjs.ang);
			STATE_CHANGED(wm->lstate.exp_rjs_mag, wm->exp.classic.rjs.mag);
			STATE_CHANGED(wm->lstate.exp_r_shoulder, wm->exp.classic.r_shoulder);
			STATE_CHANGED(wm->lstate.exp_l_shoulder, wm->exp.classic.l_shoulder);
			STATE_CHANGED(wm->lstate.exp_btns, wm->exp.classic.btns);
			break;
		}
		case EXP_GUITAR_HERO_3:
		{
			STATE_CHANGED(wm->lstate.exp_ljs_ang, wm->exp.gh3.js.ang);
			STATE_CHANGED(wm->lstate.exp_ljs_mag, wm->exp.gh3.js.mag);
			STATE_CHANGED(wm->lstate.exp_r_shoulder, wm->exp.gh3.whammy_bar);
			STATE_CHANGED(wm->lstate.exp_btns, wm->exp.gh3.btns);
			break;
		}
		case EXP_NONE:
		{
			break;
		}
	}

	STATE_CHANGED(wm->lstate.btns, wm->btns);

	return 0;
}
