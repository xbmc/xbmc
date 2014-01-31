/*
 * Copyright (c) 2010-2012, Freescale Semiconductor, Inc. All rights reserved.
 *
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Module Name:    TimeStamp.h
 *
 * Description:    include TimeStamp stratege for VPU / SW video decoder plugin
 *
 * Portability:    This code is written for Linux OS and Gstreamer
 */

/*
 * Changelog:
  11/2/2010        draft version       Lyon Wang
 *
 */

#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_


/**
 * GST_CLOCK_TIME_NONE:
 *
 * Constant to define an undefined clock time.
 */

typedef long long TSM_TIMESTAMP;

typedef enum
{
  MODE_AI,
  MODE_FIFO,
} TSMGR_MODE;

#define TSM_TIMESTAMP_NONE ((long long)(-1))
#define TSM_KEY_NONE ((void *)0)

/**
 * GST_CLOCK_TIME_IS_VALID:
 * @time: clock time to validate
 *
 * Tests if a given #GstClockTime represents a valid defined time.
 */

#ifdef __cplusplus
#define EXTERN
#else
#define EXTERN extern
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/*!
 * This function receive timestamp into timestamp manager.
 *
 * @param	handle		handle of timestamp manager.
 *
 * @param	timestamp	timestamp received
 *
 * @return	
 */
  EXTERN void TSManagerReceive (void *handle, TSM_TIMESTAMP timestamp);

  EXTERN void TSManagerReceive2 (void *handle, TSM_TIMESTAMP timestamp,
      int size);

  EXTERN void TSManagerFlush2 (void *handle, int size);

  EXTERN void TSManagerValid2 (void *handle, int size, void *key);

/*!
 * This function send the timestamp for next output frame.
 *
 * @param	handle		handle of timestamp manager.
 *
 * @return	timestamp for next output frame.
 */
  EXTERN TSM_TIMESTAMP TSManagerSend (void *handle);

  EXTERN TSM_TIMESTAMP TSManagerSend2 (void *handle, void *key);

  EXTERN TSM_TIMESTAMP TSManagerQuery2 (void *handle, void *key);

  EXTERN TSM_TIMESTAMP TSManagerQuery (void *handle);
/*!
 * This function resync timestamp handler when reset and seek
 *
 * @param	handle		handle of timestamp manager.
 *
 * @param	synctime    the postion time needed to set, if value invalid, position keeps original
 * 
 * @param	mode		playing mode (AI or FIFO)
 *
 * @return	
 */
  EXTERN void resyncTSManager (void *handle, TSM_TIMESTAMP synctime,
      TSMGR_MODE mode);
/*!
 * This function create and reset timestamp handler
 *
 * @param	ts_buf_size	 time stamp queue buffer size 
 * 
 * @return	
 */
  EXTERN void *createTSManager (int ts_buf_size);
/*!
 * This function destory timestamp handler
 *
 * @param	handle		handle of timestamp manager.
 * 
 * @return	
 */
  EXTERN void destroyTSManager (void *handle);
/*!
 * This function set  history buffer frame interval by fps_n and fps_d 
 *
 * @param	handle		handle of timestamp manager.
 * 
 * @param	framerate       the framerate to be set
 * 
 * @return	
 */
  EXTERN void setTSManagerFrameRate (void *handle, int fps_n, int fps_d);
//EXTERN void setTSManagerFrameRate(void * handle, float framerate);
/*!
 * This function set the current calculated Frame Interval
 *
 * @param	handle		handle of timestamp manager.
 * 
 * @return	
 */
  EXTERN TSM_TIMESTAMP getTSManagerFrameInterval (void *handle);
/*!
 * This function get  the current time stamp postion
 *
 * @param	handle		handle of timestamp manager.
 * 
 * @return	
 */
  EXTERN TSM_TIMESTAMP getTSManagerPosition (void *handle);
  EXTERN int getTSManagerPreBufferCnt (void *handle);

#ifdef __cplusplus
}
#endif

#endif /*_TIMESTAMP_H_ */
