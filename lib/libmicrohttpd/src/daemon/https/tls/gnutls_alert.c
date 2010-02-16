/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA
 *
 */

#include <gnutls_int.h>
#include <gnutls_errors.h>
#include <gnutls_record.h>
#include <debug.h>

typedef struct
{
  MHD_gnutls_alert_description_t alert;
  const char *desc;
} MHD_gnutls_alert_entry;

static const MHD_gnutls_alert_entry MHD_gtls_sup_alerts[] = {
  {GNUTLS_A_CLOSE_NOTIFY, "Close notify"},
  {GNUTLS_A_UNEXPECTED_MESSAGE, "Unexpected message"},
  {GNUTLS_A_BAD_RECORD_MAC, "Bad record MAC"},
  {GNUTLS_A_DECRYPTION_FAILED, "Decryption failed"},
  {GNUTLS_A_RECORD_OVERFLOW, "Record overflow"},
  {GNUTLS_A_DECOMPRESSION_FAILURE, "Decompression failed"},
  {GNUTLS_A_HANDSHAKE_FAILURE, "Handshake failed"},
  {GNUTLS_A_BAD_CERTIFICATE, "Certificate is bad"},
  {GNUTLS_A_UNSUPPORTED_CERTIFICATE, "Certificate is not supported"},
  {GNUTLS_A_CERTIFICATE_REVOKED, "Certificate was revoked"},
  {GNUTLS_A_CERTIFICATE_EXPIRED, "Certificate is expired"},
  {GNUTLS_A_CERTIFICATE_UNKNOWN, "Unknown certificate"},
  {GNUTLS_A_ILLEGAL_PARAMETER, "Illegal parameter"},
  {GNUTLS_A_UNKNOWN_CA, "CA is unknown"},
  {GNUTLS_A_ACCESS_DENIED, "Access was denied"},
  {GNUTLS_A_DECODE_ERROR, "Decode error"},
  {GNUTLS_A_DECRYPT_ERROR, "Decrypt error"},
  {GNUTLS_A_EXPORT_RESTRICTION, "Export restriction"},
  {GNUTLS_A_PROTOCOL_VERSION, "Error in protocol version"},
  {GNUTLS_A_INSUFFICIENT_SECURITY, "Insufficient security"},
  {GNUTLS_A_USER_CANCELED, "User canceled"},
  {GNUTLS_A_INTERNAL_ERROR, "Internal error"},
  {GNUTLS_A_NO_RENEGOTIATION, "No renegotiation is allowed"},
  {GNUTLS_A_CERTIFICATE_UNOBTAINABLE,
   "Could not retrieve the specified certificate"},
  {GNUTLS_A_UNSUPPORTED_EXTENSION, "An unsupported extension was sent"},
  {GNUTLS_A_UNRECOGNIZED_NAME,
   "The server name sent was not recognized"},
  {GNUTLS_A_UNKNOWN_PSK_IDENTITY,
   "The SRP/PSK username is missing or not known"},
};

#define GNUTLS_ALERT_LOOP(b) \
        const MHD_gnutls_alert_entry *p; \
                for(p = MHD_gtls_sup_alerts; p->desc != NULL; p++) { b ; }

#define GNUTLS_ALERT_ID_LOOP(a) \
                        GNUTLS_ALERT_LOOP( if(p->alert == alert) { a; break; })


/**
  * MHD__gnutls_alert_get_name - Returns a string describing the alert number given
  * @alert: is an alert number #MHD_gtls_session_t structure.
  *
  * This function will return a string that describes the given alert
  * number or NULL.  See MHD_gnutls_alert_get().
  *
  **/
const char *
MHD__gnutls_alert_get_name (MHD_gnutls_alert_description_t alert)
{
  const char *ret = NULL;

  GNUTLS_ALERT_ID_LOOP (ret = p->desc);

  return ret;
}

/**
  * MHD__gnutls_alert_send - This function sends an alert message to the peer
  * @session: is a #MHD_gtls_session_t structure.
  * @level: is the level of the alert
  * @desc: is the alert description
  *
  * This function will send an alert to the peer in order to inform
  * him of something important (eg. his Certificate could not be verified).
  * If the alert level is Fatal then the peer is expected to close the
  * connection, otherwise he may ignore the alert and continue.
  *
  * The error code of the underlying record send function will be returned,
  * so you may also receive GNUTLS_E_INTERRUPTED or GNUTLS_E_AGAIN as well.
  *
  * Returns 0 on success.
  *
  **/
int
MHD__gnutls_alert_send (MHD_gtls_session_t session,
                        MHD_gnutls_alert_level_t level,
                        MHD_gnutls_alert_description_t desc)
{
  uint8_t data[2];
  int ret;
  const char *name;

  data[0] = (uint8_t) level;
  data[1] = (uint8_t) desc;

  name = MHD__gnutls_alert_get_name ((int) data[1]);
  if (name == NULL)
    name = "(unknown)";
  MHD__gnutls_record_log ("REC: Sending Alert[%d|%d] - %s\n", data[0],
                          data[1], name);

  if ((ret = MHD_gtls_send_int (session, GNUTLS_ALERT, -1, data, 2)) >= 0)
    return 0;
  else
    return ret;
}

/**
  * MHD_gtls_error_to_alert - This function returns an alert code based on the given error code
  * @err: is a negative integer
  * @level: the alert level will be stored there
  *
  * Returns an alert depending on the error code returned by a gnutls
  * function. All alerts sent by this function should be considered fatal.
  * The only exception is when err == GNUTLS_E_REHANDSHAKE, where a warning
  * alert should be sent to the peer indicating that no renegotiation will
  * be performed.
  *
  * If there is no mapping to a valid alert the alert to indicate internal error
  * is returned.
  *
  **/
int
MHD_gtls_error_to_alert (int err, int *level)
{
  int ret, _level = -1;

  switch (err)
    {                           /* send appropriate alert */
    case GNUTLS_E_DECRYPTION_FAILED:
      /* GNUTLS_A_DECRYPTION_FAILED is not sent, because
       * it is not defined in SSL3. Note that we must
       * not distinguish Decryption failures from mac
       * check failures, due to the possibility of some
       * attacks.
       */
      ret = GNUTLS_A_BAD_RECORD_MAC;
      _level = GNUTLS_AL_FATAL;
      break;
    case GNUTLS_E_DECOMPRESSION_FAILED:
      ret = GNUTLS_A_DECOMPRESSION_FAILURE;
      _level = GNUTLS_AL_FATAL;
      break;
    case GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER:
    case GNUTLS_E_ILLEGAL_SRP_USERNAME:
      ret = GNUTLS_A_ILLEGAL_PARAMETER;
      _level = GNUTLS_AL_FATAL;
      break;
    case GNUTLS_E_ASN1_ELEMENT_NOT_FOUND:
    case GNUTLS_E_ASN1_IDENTIFIER_NOT_FOUND:
    case GNUTLS_E_ASN1_DER_ERROR:
    case GNUTLS_E_ASN1_VALUE_NOT_FOUND:
    case GNUTLS_E_ASN1_GENERIC_ERROR:
    case GNUTLS_E_ASN1_VALUE_NOT_VALID:
    case GNUTLS_E_ASN1_TAG_ERROR:
    case GNUTLS_E_ASN1_TAG_IMPLICIT:
    case GNUTLS_E_ASN1_TYPE_ANY_ERROR:
    case GNUTLS_E_ASN1_SYNTAX_ERROR:
    case GNUTLS_E_ASN1_DER_OVERFLOW:
      ret = GNUTLS_A_BAD_CERTIFICATE;
      _level = GNUTLS_AL_FATAL;
      break;
    case GNUTLS_E_UNKNOWN_CIPHER_SUITE:
    case GNUTLS_E_UNKNOWN_COMPRESSION_ALGORITHM:
    case GNUTLS_E_INSUFFICIENT_CREDENTIALS:
    case GNUTLS_E_NO_CIPHER_SUITES:
    case GNUTLS_E_NO_COMPRESSION_ALGORITHMS:
      ret = GNUTLS_A_HANDSHAKE_FAILURE;
      _level = GNUTLS_AL_FATAL;
      break;
    case GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION:
      ret = GNUTLS_A_UNSUPPORTED_EXTENSION;
      _level = GNUTLS_AL_FATAL;
      break;
    case GNUTLS_E_UNEXPECTED_PACKET:
    case GNUTLS_E_UNEXPECTED_HANDSHAKE_PACKET:
      ret = GNUTLS_A_UNEXPECTED_MESSAGE;
      _level = GNUTLS_AL_FATAL;
      break;
    case GNUTLS_E_REHANDSHAKE:
      ret = GNUTLS_A_NO_RENEGOTIATION;
      _level = GNUTLS_AL_WARNING;
      break;
    case GNUTLS_E_UNSUPPORTED_VERSION_PACKET:
      ret = GNUTLS_A_PROTOCOL_VERSION;
      _level = GNUTLS_AL_FATAL;
      break;
    case GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE:
      ret = GNUTLS_A_UNSUPPORTED_CERTIFICATE;
      _level = GNUTLS_AL_FATAL;
      break;
    case GNUTLS_E_UNEXPECTED_PACKET_LENGTH:
      ret = GNUTLS_A_RECORD_OVERFLOW;
      _level = GNUTLS_AL_FATAL;
      break;
    case GNUTLS_E_INTERNAL_ERROR:
    case GNUTLS_E_NO_TEMPORARY_DH_PARAMS:
    case GNUTLS_E_NO_TEMPORARY_RSA_PARAMS:
      ret = GNUTLS_A_INTERNAL_ERROR;
      _level = GNUTLS_AL_FATAL;
      break;
    case GNUTLS_E_DH_PRIME_UNACCEPTABLE:
    case GNUTLS_E_NO_CERTIFICATE_FOUND:
      ret = GNUTLS_A_INSUFFICIENT_SECURITY;
      _level = GNUTLS_AL_FATAL;
      break;
    default:
      ret = GNUTLS_A_INTERNAL_ERROR;
      _level = GNUTLS_AL_FATAL;
      break;
    }

  if (level != NULL)
    *level = _level;

  return ret;
}


/**
 * MHD__gnutls_alert_send_appropriate - This function sends an alert to the peer depending on the error code
 * @session: is a #MHD_gtls_session_t structure.
 * @err: is an integer
 *
 * Sends an alert to the peer depending on the error code returned by a gnutls
 * function. This function will call MHD_gtls_error_to_alert() to determine
 * the appropriate alert to send.
 *
 * This function may also return GNUTLS_E_AGAIN, or GNUTLS_E_INTERRUPTED.
 *
 * If the return value is GNUTLS_E_INVALID_REQUEST, then no alert has
 * been sent to the peer.
 *
 * Returns zero on success.
 */
int
MHD__gnutls_alert_send_appropriate (MHD_gtls_session_t session, int err)
{
  int alert;
  int level;

  alert = MHD_gtls_error_to_alert (err, &level);
  if (alert < 0)
    {
      return alert;
    }

  return MHD__gnutls_alert_send (session, level, alert);
}

/**
  * MHD_gnutls_alert_get - Returns the last alert number received.
  * @session: is a #MHD_gtls_session_t structure.
  *
  * This function will return the last alert number received. This
  * function should be called if GNUTLS_E_WARNING_ALERT_RECEIVED or
  * GNUTLS_E_FATAL_ALERT_RECEIVED has been returned by a gnutls
  * function.  The peer may send alerts if he thinks some things were
  * not right. Check gnutls.h for the available alert descriptions.
  *
  * If no alert has been received the returned value is undefined.
  *
  **/
MHD_gnutls_alert_description_t
MHD_gnutls_alert_get (MHD_gtls_session_t session)
{
  return session->internals.last_alert;
}
