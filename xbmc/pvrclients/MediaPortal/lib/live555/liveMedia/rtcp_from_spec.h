/* RTCP code taken directly from the most recent RTP specification:
 *     draft-ietf-avt-rtp-new-11.txt
 * C header
 */

#ifndef _RTCP_FROM_SPEC_H
#define _RTCP_FROM_SPEC_H

#include <stdlib.h>

/* Definitions of _ANSI_ARGS and EXTERN that will work in either
   C or C++ code:
 */
#undef _ANSI_ARGS_
#if ((defined(__STDC__) || defined(SABER)) && !defined(NO_PROTOTYPE)) || defined(__cplusplus) || defined(USE_PROTOTYPE)
#   define _ANSI_ARGS_(x)	x
#else
#   define _ANSI_ARGS_(x)	()
#endif
#ifdef __cplusplus
#   define EXTERN extern "C"
#else
#   define EXTERN extern
#endif

/* The code from the spec assumes a type "event"; make this a void*: */
typedef void* event;

#define EVENT_UNKNOWN 0
#define EVENT_REPORT 1
#define EVENT_BYE 2

/* The code from the spec assumes a type "time_tp"; make this a double: */
typedef double time_tp;

/* The code from the spec assumes a type "packet"; make this a void*: */
typedef void* packet;

#define PACKET_UNKNOWN_TYPE 0
#define PACKET_RTP 1
#define PACKET_RTCP_REPORT 2
#define PACKET_BYE 3

/* The code from the spec calls drand48(), but we have drand30() instead */
#define drand48 drand30

/* The code calls "exit()", but we don't want to exit, so make it a noop: */
#define exit(n) do {} while (0)

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

/* EXPORTS: */

EXTERN void OnExpire _ANSI_ARGS_((event, int, int, double, int, double*, int*, time_tp, time_tp*, int*));

EXTERN void OnReceive _ANSI_ARGS_((packet, event, int*, int*, int*, double*, double*, double, double));

/* IMPORTS: */

EXTERN void Schedule _ANSI_ARGS_((double,event));
EXTERN void Reschedule _ANSI_ARGS_((double,event));
EXTERN void SendRTCPReport _ANSI_ARGS_((event));
EXTERN void SendBYEPacket _ANSI_ARGS_((event));
EXTERN int TypeOfEvent _ANSI_ARGS_((event));
EXTERN int SentPacketSize _ANSI_ARGS_((event));
EXTERN int PacketType _ANSI_ARGS_((packet));
EXTERN int ReceivedPacketSize _ANSI_ARGS_((packet));
EXTERN int NewMember _ANSI_ARGS_((packet));
EXTERN int NewSender _ANSI_ARGS_((packet));
EXTERN void AddMember _ANSI_ARGS_((packet));
EXTERN void AddSender _ANSI_ARGS_((packet));
EXTERN void RemoveMember _ANSI_ARGS_((packet));
EXTERN void RemoveSender _ANSI_ARGS_((packet));
EXTERN double drand30 _ANSI_ARGS_((void));

#endif
