/*
     This file is part of libmicrohttpd
     (C) 2007, 2008 Daniel Pittman and Christian Grothoff

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

/**
 * @file connection.c
 * @brief  Methods for managing SSL/TLS connections. This file is only
 *         compiled if ENABLE_HTTPS is set.
 * @author Sagie Amir
 * @author Christian Grothoff
 */

#include "internal.h"
#include "connection.h"
#include "memorypool.h"
#include "response.h"
#include "reason_phrase.h"

/* get opaque type */
#include "gnutls_int.h"
#include "gnutls_record.h"

/* TODO #include rm "gnutls_errors.h" */
#include "gnutls_errors.h"

/**
 * This function is called once a secure connection has been marked
 * for closure.
 *
 * NOTE: Some code duplication with connection_close_error
 * in connection.c
 *
 * @param connection: the connection to close
 * @param termination_code: the termination code with which the notify completed callback function is called.
 */
static void
MHD_tls_connection_close (struct MHD_Connection *connection,
                          enum MHD_RequestTerminationCode termination_code)
{
  MHD__gnutls_bye (connection->tls_session, GNUTLS_SHUT_WR);
  connection->tls_session->internals.read_eof = 1;
  MHD_connection_close (connection, termination_code);
}

/**
 * This function was created to handle per-connection processing that
 * has to happen even if the socket cannot be read or written to.  All
 * implementations (multithreaded, external select, internal select)
 * call this function.
 *
 * @param connection being handled
 * @return MHD_YES if we should continue to process the
 *         connection (not dead yet), MHD_NO if it died
 */
static int
MHD_tls_connection_handle_idle (struct MHD_Connection *connection)
{
  unsigned int timeout;

#if DEBUG_STATES
  MHD_DLOG (connection->daemon, "%s: state: %s\n",
            __FUNCTION__, MHD_state_to_string (connection->state));
#endif
  timeout = connection->daemon->connection_timeout;
  if ((connection->socket_fd != -1) && (timeout != 0)
      && (time (NULL) - timeout > connection->last_activity))
    {
      MHD_tls_connection_close (connection,
                                MHD_REQUEST_TERMINATED_TIMEOUT_REACHED);
      return MHD_NO;
    }
  switch (connection->state)
    {
      /* on newly created connections we might reach here before any reply has been received */
    case MHD_TLS_CONNECTION_INIT:
      return MHD_YES;
      /* close connection if necessary */
    case MHD_CONNECTION_CLOSED:
      if (connection->socket_fd != -1)
        MHD_tls_connection_close (connection,
                                  MHD_REQUEST_TERMINATED_COMPLETED_OK);
      return MHD_NO;
    case MHD_TLS_HANDSHAKE_FAILED:
      MHD_tls_connection_close (connection,
                                MHD_REQUEST_TERMINATED_WITH_ERROR);
      return MHD_NO;
      /* some HTTP state */
    default:
      return MHD_connection_handle_idle (connection);
    }
  return MHD_YES;
}

/**
 * This function handles a particular SSL/TLS connection when
 * it has been determined that there is data to be read off a
 * socket. Message processing is done by message type which is
 * determined by peeking into the first message type byte of the
 * stream.
 *
 * Error message handling: all fatal level messages cause the
 * connection to be terminated.
 *
 * Application data is forwarded to the underlying daemon for
 * processing.
 *
 * @param connection : the source connection
 * @return MHD_YES if we should continue to process the
 *         connection (not dead yet), MHD_NO if it died
 */
static int
MHD_tls_connection_handle_read (struct MHD_Connection *connection)
{
  int ret;
  unsigned char msg_type;

  connection->last_activity = time (NULL);
  if (connection->state == MHD_CONNECTION_CLOSED ||
      connection->state == MHD_TLS_HANDSHAKE_FAILED)
    return MHD_NO;

#if DEBUG_STATES
  MHD_DLOG (connection->daemon, "%s: state: %s\n",
            __FUNCTION__, MHD_state_to_string (connection->state));
#endif

  /* discover content type */
  if (RECV (connection->socket_fd, &msg_type, 1, MSG_PEEK) == -1)
    {
#if HAVE_MESSAGES
      MHD_DLOG (connection->daemon, "Failed to peek into TLS content type\n");
#endif
      return MHD_NO;
    }

  switch (msg_type)
    {
      /* check for handshake messages first */
    case GNUTLS_HANDSHAKE:
      /* negotiate handshake only while in INIT & HELLO_REQUEST states */
      if (connection->state == MHD_TLS_CONNECTION_INIT ||
          connection->state == MHD_TLS_HELLO_REQUEST)
        {
          ret = MHD__gnutls_handshake (connection->tls_session);
          if (ret == 0)
            {
              /* set connection state to enable HTTP processing */
              connection->state = MHD_CONNECTION_INIT;
              break;
            }
          /* set connection as closed */
          else
            {
#if HAVE_MESSAGES
              MHD_DLOG (connection->daemon,
                        "Error: Handshake has failed (%d)\n", ret);
#endif
              connection->state = MHD_TLS_HANDSHAKE_FAILED;
              return MHD_NO;
            }
        }
      /* a handshake message has been received out of bound */
      else
        {
#if HAVE_MESSAGES
          MHD_DLOG (connection->daemon,
                    "Error: received handshake message out of context\n");
#endif
          MHD_tls_connection_close (connection,
                                    MHD_REQUEST_TERMINATED_WITH_ERROR);
          return MHD_NO;
        }

      /* ignore any out of bound change chiper spec messages */
    case GNUTLS_CHANGE_CIPHER_SPEC:
      MHD_tls_connection_close (connection,
                                MHD_REQUEST_TERMINATED_WITH_ERROR);
      return MHD_NO;

    case GNUTLS_ALERT:
      /*
       * this call of MHD_gtls_recv_int expects 0 bytes read.
       * done to decrypt alert message
       */
      MHD_gtls_recv_int (connection->tls_session, GNUTLS_ALERT,
                         GNUTLS_HANDSHAKE_FINISHED, 0, 0);

      /* CLOSE_NOTIFY */
      if (connection->tls_session->internals.last_alert ==
          GNUTLS_A_CLOSE_NOTIFY)
        {
          connection->state = MHD_CONNECTION_CLOSED;
          return MHD_YES;
        }
      /* non FATAL or WARNING */
      else if (connection->tls_session->internals.last_alert_level !=
               GNUTLS_AL_FATAL)
        {
#if HAVE_MESSAGES
          MHD_DLOG (connection->daemon,
                    "Received TLS alert: %s\n",
                    MHD__gnutls_alert_get_name ((int)
                                                connection->tls_session->
                                                internals.last_alert));
#endif
          return MHD_YES;
        }
      /* FATAL */
      else if (connection->tls_session->internals.last_alert_level ==
               GNUTLS_AL_FATAL)
        {
          MHD_tls_connection_close (connection,
                                    MHD_REQUEST_TERMINATED_WITH_ERROR);
          return MHD_NO;
        }
      /* this should never execute */
      else
        {
#if HAVE_MESSAGES
          MHD_DLOG (connection->daemon,
                    "Received unrecognized alert: %d\n",
                    connection->tls_session->internals.last_alert);
#endif
          return MHD_NO;
        }


      /* forward application level content to MHD */
    case GNUTLS_APPLICATION_DATA:
      return MHD_connection_handle_read (connection);

    case GNUTLS_INNER_APPLICATION:
      break;
    default:
#if HAVE_MESSAGES
      MHD_DLOG (connection->daemon,
                "Error: unrecognized TLS message type: %d, connection state: %s. l: %d, f: %s\n",
                msg_type, MHD_state_to_string (connection->state), __LINE__,
                __FUNCTION__);
#endif
      /* close connection upon reception of unrecognized message type */
      MHD_tls_connection_close (connection,
                                MHD_REQUEST_TERMINATED_WITH_ERROR);
      return MHD_NO;
    }

  return MHD_YES;
}

/**
 * This function was created to handle writes to sockets when it has
 * been determined that the socket can be written to. This function
 * will forward all write requests to the underlying daemon unless
 * the connection has been marked for closing.
 *
 * @return MHD_connection_handle_write() if we should continue to
 *         process the connection (not dead yet), MHD_NO if it died
 */
static int
MHD_tls_connection_handle_write (struct MHD_Connection *connection)
{
  connection->last_activity = time (NULL);

#if DEBUG_STATES
  MHD_DLOG (connection->daemon, "%s: state: %s\n",
            __FUNCTION__, MHD_state_to_string (connection->state));
#endif

  switch (connection->state)
    {
    case MHD_CONNECTION_CLOSED:
    case MHD_TLS_HANDSHAKE_FAILED:
      return MHD_NO;
      /* some HTTP connection state */
    default:
      return MHD_connection_handle_write (connection);
    }
  return MHD_NO;
}

/**
 * Set connection callback function to be used through out
 * the processing of this secure connection.
 */
void
MHD_set_https_calbacks (struct MHD_Connection *connection)
{
  connection->read_handler = &MHD_tls_connection_handle_read;
  connection->write_handler = &MHD_tls_connection_handle_write;
  connection->idle_handler = &MHD_tls_connection_handle_idle;
}

/* end of connection_https.c */
