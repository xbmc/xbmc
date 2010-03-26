/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2007 Johannes Lehtinen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *-----------------------------------------------------------------------*/

#ifndef CALLBACKCOUNTER_H_
#define CALLBACKCOUNTER_H_

#ifdef __cplusplus
extern "C" {
#endif

/** A type for cbc_counters_t structure */
typedef struct cbc_counters_t cbc_counters_t;

/** A container for callback counters */
struct cbc_counters_t {
	
	/** Call counter for the create function */
	int create;
	
	/** Call counter for the start function */
	int start;
	
	/** Call counter for the logger function */
	int logger;
	
	/** Call counter for the plug-in listener function */
	int listener;
	
	/** Call counter for the run function */
	int run;
	
	/** Call counter for the stop function */
	int stop;
	
	/** Call counter for the destroy function */
	int destroy;
	
	/** Copy of context arg 0 from the call to start, or NULL */
	char *context_arg_0;
};

#ifdef __cplusplus
}
#endif

#endif /*CALLBACKCOUNTER_H_*/
