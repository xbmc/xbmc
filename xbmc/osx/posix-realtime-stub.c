/*
 * Copyright (c), MM Weiss
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 *     1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *     
 *     2. Redistributions in binary form must reproduce the above copyright notice, 
 *     this list of conditions and the following disclaimer in the documentation 
 *     and/or other materials provided with the distribution.
 *     
 *     3. Neither the name of the MM Weiss nor the names of its contributors 
 *     may be used to endorse or promote products derived from this software without 
 *     specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR 
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  posix-realtime-stub.c a draft
 *  gcc -Wall -c posix-realtime-stub.c
 * 
 */

#ifdef __APPLE__

#pragma weak clock_getres
#pragma weak clock_gettime
#pragma weak clock_nanosleep
#pragma weak clock_getcpuclockid

#include <sys/time.h>
#include <sys/resource.h>
#include <mach/mach.h>
#include <mach/clock.h>
#include <mach/mach_time.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include "posix-realtime-stub.h"

static mach_timebase_info_data_t rlclk_timebase_info;

struct rlclock { // one clock per thread creating a queue ?
	host_t rlclk_host;
	task_t rlclk_task;
	thread_act_t rlclk_thread;
	clockid_t *rlclk_id;
	semaphore_t rlclk_sem; // next_step clock_alarm and timers
	void *rlclks;
};

/** to solve 

	
	from mach_task_self() get_current_thread
	thread_act_array_t task_threads(task_t task);
	
	case CLOCK_PROCESS_CPUTIME_ID:
	case CLOCK_THREAD_CPUTIME_ID:
	task_info(mach_task, TASK_BASIC_INFO, (task_info_t)&tinfo, &tflag) return the total not thread per thread
	
	clock_getcpuclockid
	loop:
		if rlclks[i]->rlclk_thread == current_thread
			return *(clockid_t *)rlclks[i]->rlclk_id
*/


extern void clock_get_uptime(uint64_t *result);
//extern void microtime(struct timeval *tv);

/**
extern void clock_get_calendar_microtime(uint32_t *secs, uint32_t *microsecs);
extern void clock_get_calendar_nanotime(uint32_t *secs, uint32_t *nanosecs);
extern void	clock_get_system_microtime(uint32_t	*secs, uint32_t *microsecs);
extern void	clock_get_system_nanotime(uint32_t *secs, uint32_t *nanosecs);
extern void	clock_timebase_info(mach_timebase_info_t info);
*/

int clock_getres(clockid_t clk_id, struct timespec *res) {
	uint64_t nano;
	mach_msg_type_number_t tflag;
	clock_serv_t clock_port;
	clock_id_t clk_serv_id;
	
	int retval = -1;	
	switch (clk_id) {
		case CLOCK_REALTIME:
		case CLOCK_MONOTONIC:
		case CLOCK_PROCESS_CPUTIME_ID:
		case CLOCK_THREAD_CPUTIME_ID:
			retval = 0;
			if (res) {
				clk_serv_id = clk_id == CLOCK_REALTIME ? CALENDAR_CLOCK : SYSTEM_CLOCK;
				if (KERN_SUCCESS == host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &clock_port)) {
					if (KERN_SUCCESS == clock_get_attributes(clock_port, CLOCK_GET_TIME_RES, (clock_attr_t)&nano, &tflag)) {
						res->tv_sec = 0;  
						res->tv_nsec = nano;
					} else {
						errno = EINVAL;
						retval = -1;
					}
				} else {
					errno = EINVAL;
					retval = -1;
				}
			}
		break;
		default:
			errno = EINVAL;
	}
	return retval;
}

static int __clock_gettime_stub(host_t mach_host, task_t mach_task, clockid_t clk_id, struct timespec *tp) {
	kern_return_t   ret;
	clock_serv_t    clk;
	clock_id_t clk_serv_id;
	mach_timespec_t tm;
	
	uint64_t start, end, delta, nano;
	
	task_basic_info_data_t tinfo;
	task_thread_times_info_data_t ttinfo;
	mach_msg_type_number_t tflag;
	
	int retval = -1;
	switch (clk_id) {
		case CLOCK_REALTIME:
		case CLOCK_MONOTONIC:
			clk_serv_id = clk_id == CLOCK_REALTIME ? CALENDAR_CLOCK : SYSTEM_CLOCK;
			if (KERN_SUCCESS == (ret = host_get_clock_service(mach_host, clk_serv_id, &clk))) {
				if (KERN_SUCCESS == (ret = clock_get_time(clk, &tm))) {
					if (tp) {
						tp->tv_sec = tm.tv_sec;
						tp->tv_nsec = tm.tv_nsec;
					}
					retval = 0;
				}
			}
			if (KERN_SUCCESS != ret) {
				errno = EINVAL;
				retval = -1;
			}
		break;
		case CLOCK_PROCESS_CPUTIME_ID:
		case CLOCK_THREAD_CPUTIME_ID:
			if (clk_id == CLOCK_PROCESS_CPUTIME_ID) {
				tflag = TASK_BASIC_INFO_COUNT;
				if (KERN_SUCCESS == (ret = task_info(mach_task, TASK_BASIC_INFO, (task_info_t)&tinfo, &tflag))) {
					if (tp) {
						tp->tv_sec = tinfo.user_time.seconds; // + tinfo.system_time.seconds;
						tp->tv_nsec = tinfo.user_time.microseconds; // + tinfo.system_time.microseconds;
					}
					retval = 0;
				}				
				struct rusage r_usage;
				if (0 == getrusage(RUSAGE_SELF, &r_usage)) {
					if (tp) {
						tp->tv_sec = r_usage.ru_utime.tv_sec; // + r_usage.ru_stime.tv_sec;
						tp->tv_nsec = r_usage.ru_utime.tv_usec; // + r_usage.ru_stime.tv_usec;
					}
					retval = 0;
				}
			}
			if (clk_id == CLOCK_THREAD_CPUTIME_ID) {
				tflag = TASK_THREAD_TIMES_INFO_COUNT;
				if (KERN_SUCCESS == (ret = task_info(mach_task, TASK_THREAD_TIMES_INFO, (task_info_t)&ttinfo, &tflag))) {
					if (tp) { // uptime ? or should substract to now
						tp->tv_sec = ttinfo.user_time.seconds; // + ttinfo.system_time.seconds;
						tp->tv_nsec = ttinfo.user_time.microseconds; // + ttinfo.system_time.microseconds;
					}
					retval = 0;
				}
			}
			
			if (0 != retval) {
				start = mach_absolute_time();
				if (clk_id == CLOCK_PROCESS_CPUTIME_ID) {
					getpid();
				} else {
					sched_yield();
				}
				end = mach_absolute_time();
				delta = end - start;	
				if (0 == rlclk_timebase_info.denom) {
					mach_timebase_info(&rlclk_timebase_info);
				}
				nano = delta * rlclk_timebase_info.numer / rlclk_timebase_info.denom;
				if (tp) {
					tp->tv_sec = nano * 1e-9;  
					tp->tv_nsec = nano - (tp->tv_sec * 1e9);
				}
				retval = 0;
			}
		break;
		default:
			errno = EINVAL;
			retval = -1;
	}
	return retval;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
	return __clock_gettime_stub(mach_host_self(), mach_task_self(), clk_id, tp);
}

int clock_nanosleep(__unused clockid_t clock_id, int flags, const struct timespec *rqtp, struct timespec *rmtp) {
	/*clock_serv_t system_clock;
	host_get_clock_service(mach_host_self(), REALTIME_CLOCK, &system_clock);
	
	
	tvalspec_t.tv_sec =  rqtp->tv_sec + rmtp->tv_sec; ? nanosleep
	tvalspec_t.tv_nsec =  rqtp->tv_nsec + rmtp->tv_nsec;
	
	clock_sleep(system_clock, TIME_ABSOLUTE, wakeup_time, NULL);
	
	clock_sleep(mach_port_t clock_name,
                 sleep_type_t sleep_type,
                 tvalspec_t sleep_time,
                 clock_name wake_time // NULL
                );

	*/
	return 0;
}

int clock_getcpuclockid(__unused pid_t pid, clockid_t *clock_id) {
	/*
	kern_return_t  ret;
	host_t ctask = mach_task_self(), taskforpid;
	task_for_pid
  	
  	if(KERN_SUCCESS == (ret = task_for_pid(ctask, pid, &taskforpid))) {
  		taskforpid->state->ref_clock_id ?
  		
  		thread_act_array_t task_threads(task_t task); ? thread_act->state->ref_clock_id which_one ? 
  		
  		CALENDAR_CLOCK -> CLOCK_REALTIME
  		SYSTEM_CLOCK -> CLOCK_MONOTONIC
  		mach_port_deallocate(ctask, taskforpid);
  	}
  	
  	*/
	*clock_id = CLOCK_REALTIME;
	return 0;
}

#endif // __APPLE__

/* EOF */