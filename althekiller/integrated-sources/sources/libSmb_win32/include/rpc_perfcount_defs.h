#ifndef _RPC_PERFCOUNT_DEFS_H
#define _RPC_PERFCOUNT_DEFS_H

/*
 * The following #defines match what is in winperf.h. 
 * See that include file for more details, or look up
 * "Performance Data Format" on MSDN
 * 
 * Rather than including them in rpc_perfcount.h, they
 * were broken out into a separate .h file so that they
 * can be included by other programs that need this info
 * without pulling in everything else samba-related.
 */

#define PERF_NO_INSTANCES             -1
#define PERF_NO_UNIQUE_ID	      -1

/* These determine the data size */
#define PERF_SIZE_DWORD               0x00000000
#define PERF_SIZE_LARGE               0x00000100
#define PERF_SIZE_ZERO                0x00000200
#define PERF_SIZE_VARIABLE_LEN        0x00000300

/* These determine the usage of the counter */
#define PERF_TYPE_NUMBER              0x00000000
#define PERF_TYPE_COUNTER             0x00000400
#define PERF_TYPE_TEXT                0x00000800
#define PERF_TYPE_ZERO                0x00000C00

/* If PERF_TYPE_NUMBER was selected, these provide display information */
#define PERF_NUMBER_HEX               0x00000000
#define PERF_NUMBER_DECIMAL           0x00010000
#define PERF_NUMBER_DEC_1000          0x00020000

/* If PERF_TYPE_COUNTER was selected, these provide display information */
#define PERF_COUNTER_VALUE            0x00000000
#define PERF_COUNTER_RATE             0x00010000
#define PERF_COUNTER_FRACTION         0x00020000
#define PERF_COUNTER_BASE             0x00030000
#define PERF_COUNTER_ELAPSED          0x00040000
#define PERF_COUNTER_QUEUELEN         0x00050000
#define PERF_COUNTER_HISTOGRAM        0x00060000
#define PERF_COUNTER_PRECISION        0x00070000

/* If PERF_TYPE_TEXT was selected, these provide display information */
#define PERF_TEXT_UNICODE             0x00000000
#define PERF_TEXT_ASCII               0x00010000

/* These provide information for which tick count to use when computing elapsed interval */
#define PERF_TIMER_TICK               0x00000000
#define PERF_TIMER_100NS              0x00100000
#define PERF_OBJECT_TIMER             0x00200000

/* These affect how the data is manipulated prior to being displayed */
#define PERF_DELTA_COUNTER            0x00400000
#define PERF_DELTA_BASE               0x00800000
#define PERF_INVERSE_COUNTER          0x01000000
#define PERF_MULTI_COUNTER            0x02000000

/* These determine if any text gets added when the value is displayed */
#define PERF_DISPLAY_NO_SUFFIX        0x00000000
#define PERF_DISPLAY_PER_SEC          0x10000000
#define PERF_DISPLAY_PERCENT          0x20000000
#define PERF_DISPLAY_SECONDS          0x30000000
#define PERF_DISPLAY_NOSHOW           0x40000000

/* These determine the DetailLevel of the counter */
#define PERF_DETAIL_NOVICE            100
#define PERF_DETAIL_ADVANCED          200
#define PERF_DETAIL_EXPERT            300
#define PERF_DETAIL_WIZARD            400

#endif /* _RPC_PERFCOUNT_DEFS_H */
