/* RTCP code taken directly from the most recent RTP specification:
 *     RFC 3550
 * Implementation
 */

#include "rtcp_from_spec.h"

/*****

A.7 Computing the RTCP Transmission Interval

   The following functions implement the RTCP transmission and reception
   rules described in Section 6.2. These rules are coded in several
   functions:

       o  rtcp_interval() computes the deterministic calculated
          interval, measured in seconds.  The parameters are defined in
          Section 6.3.

       o  OnExpire() is called when the RTCP transmission timer expires.

       o  OnReceive() is called whenever an RTCP packet is received.

   Both OnExpire() and OnReceive() have event e as an argument. This is
   the next scheduled event for that participant, either an RTCP report
   or a BYE packet.  It is assumed that the following functions are
   available:

       o  Schedule(time t, event e) schedules an event e to occur at
          time t. When time t arrives, the function OnExpire is called
          with e as an argument.

       o  Reschedule(time t, event e) reschedules a previously scheduled
          event e for time t.

       o  SendRTCPReport(event e) sends an RTCP report.

       o  SendBYEPacket(event e) sends a BYE packet.

       o  TypeOfEvent(event e) returns EVENT_BYE if the event being
          processed is for a BYE packet to be sent, else it returns
          EVENT_REPORT.

       o  PacketType(p) returns PACKET_RTCP_REPORT if packet p is an
          RTCP report (not BYE), PACKET_BYE if its a BYE RTCP packet,
          and PACKET_RTP if its a regular RTP data packet.

       o  ReceivedPacketSize() and SentPacketSize() return the size of
          the referenced packet in octets.

       o  NewMember(p) returns a 1 if the participant who sent packet p
          is not currently in the member list, 0 otherwise. Note this
          function is not sufficient for a complete implementation
          because each CSRC identifier in an RTP packet and each SSRC in
          a BYE packet should be processed.

       o  NewSender(p) returns a 1 if the participant who sent packet p
          is not currently in the sender sublist of the member list, 0
          otherwise.

       o  AddMember() and RemoveMember() to add and remove participants
          from the member list.

       o  AddSender() and RemoveSender() to add and remove participants
          from the sender sublist of the member list.
*****/


   double rtcp_interval(int members,
                        int senders,
                        double rtcp_bw,
                        int we_sent,
                        double avg_rtcp_size,
                        int initial)
   {
       /*
        * Minimum average time between RTCP packets from this site (in
        * seconds).  This time prevents the reports from `clumping' when
        * sessions are small and the law of large numbers isn't helping
        * to smooth out the traffic.  It also keeps the report interval
        * from becoming ridiculously small during transient outages like
        * a network partition.
        */
       double const RTCP_MIN_TIME = 5.;
       /*
        * Fraction of the RTCP bandwidth to be shared among active
        * senders.  (This fraction was chosen so that in a typical
        * session with one or two active senders, the computed report
        * time would be roughly equal to the minimum report time so that
        * we don't unnecessarily slow down receiver reports.) The
        * receiver fraction must be 1 - the sender fraction.
        */
       double const RTCP_SENDER_BW_FRACTION = 0.25;
       double const RTCP_RCVR_BW_FRACTION = (1-RTCP_SENDER_BW_FRACTION);
       /*
        * To compensate for "unconditional reconsideration" converging to a
        * value below the intended average.
        */
       double const COMPENSATION = 2.71828 - 1.5;

       double t;                   /* interval */
       double rtcp_min_time = RTCP_MIN_TIME;
       int n;                      /* no. of members for computation */

       /*
        * Very first call at application start-up uses half the min
        * delay for quicker notification while still allowing some time
        * before reporting for randomization and to learn about other
        * sources so the report interval will converge to the correct
        * interval more quickly.
        */
       if (initial) {
           rtcp_min_time /= 2;
       }

       /*
        * If there were active senders, give them at least a minimum
        * share of the RTCP bandwidth.  Otherwise all participants share
        * the RTCP bandwidth equally.
        */
       n = members;
       if (senders > 0 && senders < members * RTCP_SENDER_BW_FRACTION) {
           if (we_sent) {
               rtcp_bw *= RTCP_SENDER_BW_FRACTION;
               n = senders;
           } else {
               rtcp_bw *= RTCP_RCVR_BW_FRACTION;
               n -= senders;
           }
       }

       /*
        * The effective number of sites times the average packet size is
        * the total number of octets sent when each site sends a report.
        * Dividing this by the effective bandwidth gives the time
        * interval over which those packets must be sent in order to
        * meet the bandwidth target, with a minimum enforced.  In that
        * time interval we send one report so this time is also our
        * average time between reports.
        */
       t = avg_rtcp_size * n / rtcp_bw;
       if (t < rtcp_min_time) t = rtcp_min_time;

       /*
        * To avoid traffic bursts from unintended synchronization with
        * other sites, we then pick our actual next report interval as a
        * random number uniformly distributed between 0.5*t and 1.5*t.
        */
       t = t * (drand48() + 0.5);
       t = t / COMPENSATION;
       return t;
   }

   void OnExpire(event e,
                 int    members,
                 int    senders,
                 double rtcp_bw,
                 int    we_sent,
                 double *avg_rtcp_size,
                 int    *initial,
                 time_tp   tc,
                 time_tp   *tp,
                 int    *pmembers)
   {
       /* This function is responsible for deciding whether to send
        * an RTCP report or BYE packet now, or to reschedule transmission.
        * It is also responsible for updating the pmembers, initial, tp,
        * and avg_rtcp_size state variables. This function should be called
        * upon expiration of the event timer used by Schedule(). */

       double t;     /* Interval */
       double tn;    /* Next transmit time */

       /* In the case of a BYE, we use "unconditional reconsideration" to
        * reschedule the transmission of the BYE if necessary */

       if (TypeOfEvent(e) == EVENT_BYE) {
           t = rtcp_interval(members,
                             senders,
                             rtcp_bw,
                             we_sent,
                             *avg_rtcp_size,
                             *initial);
           tn = *tp + t;
           if (tn <= tc) {
               SendBYEPacket(e);
               exit(1);
           } else {
               Schedule(tn, e);
           }

       } else if (TypeOfEvent(e) == EVENT_REPORT) {
           t = rtcp_interval(members,
                             senders,
                             rtcp_bw,
                             we_sent,
                             *avg_rtcp_size,
                             *initial);
           tn = *tp + t;

           if (tn <= tc) {
               SendRTCPReport(e);
               *avg_rtcp_size = (1./16.)*SentPacketSize(e) +
                   (15./16.)*(*avg_rtcp_size);
               *tp = tc;

               /* We must redraw the interval. Don't reuse the
                  one computed above, since its not actually
                  distributed the same, as we are conditioned
                  on it being small enough to cause a packet to
                  be sent */

               t = rtcp_interval(members,
                                 senders,
                                 rtcp_bw,
                                 we_sent,
                                 *avg_rtcp_size,
                                 *initial);

               Schedule(t+tc,e);
               *initial = 0;
           } else {
               Schedule(tn, e);
           }
           *pmembers = members;
       }
   }


   void OnReceive(packet p,
                  event e,
                  int *members,
                  int *pmembers,
                  int *senders,
                  double *avg_rtcp_size,
                  double *tp,
                  double tc,
                  double tn)
   {
       /* What we do depends on whether we have left the group, and
        * are waiting to send a BYE (TypeOfEvent(e) == EVENT_BYE) or
        * an RTCP report. p represents the packet that was just received. */

       if (PacketType(p) == PACKET_RTCP_REPORT) {
           if (NewMember(p) && (TypeOfEvent(e) == EVENT_REPORT)) {
               AddMember(p);
               *members += 1;
           }
           *avg_rtcp_size = (1./16.)*ReceivedPacketSize(p) +
               (15./16.)*(*avg_rtcp_size);
       } else if (PacketType(p) == PACKET_RTP) {
           if (NewMember(p) && (TypeOfEvent(e) == EVENT_REPORT)) {
               AddMember(p);
               *members += 1;
           }
           if (NewSender(p) && (TypeOfEvent(e) == EVENT_REPORT)) {
               AddSender(p);
               *senders += 1;
           }
       } else if (PacketType(p) == PACKET_BYE) {
           *avg_rtcp_size = (1./16.)*ReceivedPacketSize(p) +
               (15./16.)*(*avg_rtcp_size);

           if (TypeOfEvent(e) == EVENT_REPORT) {
               if (NewSender(p) == FALSE) {
                   RemoveSender(p);
                   *senders -= 1;
               }

               if (NewMember(p) == FALSE) {
                   RemoveMember(p);
                   *members -= 1;
               }

               if(*members < *pmembers) {
                   tn = tc + (((double) *members)/(*pmembers))*(tn - tc);
                   *tp = tc - (((double) *members)/(*pmembers))*(tc - *tp);

                   /* Reschedule the next report for time tn */

                   Reschedule(tn, e);
                   *pmembers = *members;
               }

           } else if (TypeOfEvent(e) == EVENT_BYE) {
               *members += 1;
           }
       }
   }
