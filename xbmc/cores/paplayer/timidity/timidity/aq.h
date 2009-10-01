/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    aq.h - Audio queue.
	   Written by Masanao Izumo <mo@goice.co.jp>
*/

#ifndef ___AQ_H_
#define ___AQ_H_

/* interfaces */

extern int aq_calc_fragsize(void);
/* aq_calc_fragsize() calculates the best fragment size for audio device.
 */

extern void aq_setup(void);
/* aq_setup() allocates the buffer for software queue, and estimate
 * maxmum queue size of audio device.
 */

extern void aq_set_soft_queue(double soft_buff_time, double fill_start_time);
/* aq_set_soft_queue() makes software audio queue.
 * If fill_start_time is positive, TiMidity doesn't start playing immidiately
 * until the autio buffer is filled.
 */

extern int aq_add(int32 *samples, int32 count);
/* aq_add() adds new samples to software queue.  If samples is NULL,
 * aq_add() only updates internal software queue buffer.
 */

extern int32 aq_samples(void);
/* aq_samples() returns number of samples which is really played out.
 */

extern int32 aq_filled(void);
extern int32 aq_soft_filled(void);
/* aq_filled() returns filled queue length of audio device.
 * aq_soft_filled() returns filled queue length of software buffer.
 */

extern int32 aq_fillable(void);
/* aq_fillable() returns fillable queue length of qudio device. */

extern int aq_flush(int discard);
/* If discard is true, aq_flush() discards all audio queue and returns
 * immediately, otherwise aq_flush() waits until play all out.
 * aq_flush() returns RC_* message.
 */

extern int aq_soft_flush(void);
/* aq_soft_flush() transfers all buffer to device */

extern int aq_fill_nonblocking(void);
/* aq_fill_nonblocking() transfers software audio buffer to device.
 * This function doesn't block if (play_mode->flag&PF_CAN_TRACE) is true.
 */

extern double aq_filled_ratio(void);
/* aq_filled_ratio() returns filled ratio for audio device queue. */

extern int aq_get_dev_queuesize(void);
/* aq_get_dev_queuesize() returns device audio queue length
 */

extern int aq_fill_buffer_flag;
/* non-zero if aq_add() is in filling mode */

#endif /* ___AQ_H_ */
