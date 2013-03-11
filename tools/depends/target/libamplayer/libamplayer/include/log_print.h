
#ifndef PLAYER_LOG_H
#define PLAYER_LOG_H

#define MAX_LOG_SIZE	(20*1024)

__attribute__ ((format (printf, 2, 3)))
void log_lprint(const int level, const char *fmt, ...);


#define AM_LOG_PANIC 	0
#define AM_LOG_FATAL 	8
#define AM_LOG_ERROR 	16
#define AM_LOG_WARNING 	24
#define AM_LOG_INFO 	32
#define AM_LOG_VERBOSE 	40
#define AM_LOG_DEBUG 	60
#define AM_LOG_DEBUG1 	70
#define AM_LOG_DEBUG2 	80
#define AM_LOG_TRACE 	90


#define log_print(fmt...) 	log_lprint(0,##fmt)
#define log_error(fmt...) 	log_lprint(AM_LOG_ERROR,##fmt)
#define log_warning(fmt...) log_lprint(AM_LOG_WARNING,##fmt)
#define log_info(fmt...) 	log_lprint(AM_LOG_INFO,##fmt)
/*default global_level=5,
if the level<global_level print out
*/
#define log_debug(fmt...) 	log_lprint(AM_LOG_DEBUG,##fmt)
#define log_debug1(fmt...) 	log_lprint(AM_LOG_DEBUG1,##fmt)
#define log_debug2(fmt...) 	log_lprint(AM_LOG_DEBUG2,##fmt)
#define log_trace(fmt...) 	log_lprint(AM_LOG_TRACE,##fmt)

#define  DEBUG_PN() log_print("[%s:%d]\n", __FUNCTION__, __LINE__)

void log_close(void);
int log_open(const char *name);
int update_loglevel_setting(void);
#endif
