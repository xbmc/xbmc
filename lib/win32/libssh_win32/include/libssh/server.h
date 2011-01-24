/* Public include file for server support */
/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2003-2008 by Aris Adamantiadis
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

/**
 * @defgroup ssh_server SSH Server
 * @addtogroup ssh_server
 * @{
 */

#ifndef SERVER_H
#define SERVER_H

#include "libssh/libssh.h"
#define SERVERBANNER CLIENTBANNER

#ifdef __cplusplus
extern "C" {
#endif

enum ssh_bind_options_e {
  SSH_BIND_OPTIONS_BINDADDR,
  SSH_BIND_OPTIONS_BINDPORT,
  SSH_BIND_OPTIONS_BINDPORT_STR,
  SSH_BIND_OPTIONS_HOSTKEY,
  SSH_BIND_OPTIONS_DSAKEY,
  SSH_BIND_OPTIONS_RSAKEY,
  SSH_BIND_OPTIONS_BANNER,
  SSH_BIND_OPTIONS_LOG_VERBOSITY,
  SSH_BIND_OPTIONS_LOG_VERBOSITY_STR
};

//typedef struct ssh_bind_struct SSH_BIND;
typedef struct ssh_bind_struct* ssh_bind;

/**
 * @brief Creates a new SSH server bind.
 *
 * @return A newly allocated ssh_bind session pointer.
 */
LIBSSH_API ssh_bind ssh_bind_new(void);

/**
 * @brief Set the opitons for the current SSH server bind.
 *
 * @param  ssh_bind     The ssh server bind to use.
 *
 * @param  options      The option structure to set.
 */
LIBSSH_API int ssh_bind_options_set(ssh_bind sshbind,
    enum ssh_bind_options_e type, const void *value);

/**
 * @brief Start listening to the socket.
 *
 * @param  ssh_bind_o     The ssh server bind to use.
 *
 * @return 0 on success, < 0 on error.
 */
LIBSSH_API int ssh_bind_listen(ssh_bind ssh_bind_o);

/**
 * @brief  Set the session to blocking/nonblocking mode.
 *
 * @param  ssh_bind_o     The ssh server bind to use.
 *
 * @param  blocking     Zero for nonblocking mode.
 */
LIBSSH_API void ssh_bind_set_blocking(ssh_bind ssh_bind_o, int blocking);

/**
 * @brief Recover the file descriptor from the session.
 *
 * @param  ssh_bind_o     The ssh server bind to get the fd from.
 *
 * @return The file descriptor.
 */
LIBSSH_API socket_t ssh_bind_get_fd(ssh_bind ssh_bind_o);

/**
 * @brief Set the file descriptor for a session.
 *
 * @param  ssh_bind_o     The ssh server bind to set the fd.
 *
 * @param  fd           The file descriptssh_bind B
 */
LIBSSH_API void ssh_bind_set_fd(ssh_bind ssh_bind_o, socket_t fd);

/**
 * @brief Allow the file descriptor to accept new sessions.
 *
 * @param  ssh_bind_o     The ssh server bind to use.
 */
LIBSSH_API void ssh_bind_fd_toaccept(ssh_bind ssh_bind_o);

/**
 * @brief Accept an incoming ssh connection and initialize the session.
 *
 * @param  ssh_bind_o     The ssh server bind to accept a connection.
 * @param  session			A preallocated ssh session
 * @see ssh_new
 * @return A newly allocated ssh session, NULL on error.
 */
LIBSSH_API int ssh_bind_accept(ssh_bind ssh_bind_o, ssh_session session);

/**
 * @brief Free a ssh servers bind.
 *
 * @param  ssh_bind_o     The ssh server bind to free.
 */
LIBSSH_API void ssh_bind_free(ssh_bind ssh_bind_o);

/**
 * @brief Exchange the banner and cryptographic keys.
 *
 * @param  session      The ssh session to accept a connection.
 *
 * @return 0 on success, < 0 on error.
 */
LIBSSH_API int ssh_accept(ssh_session session);

LIBSSH_API int channel_write_stderr(ssh_channel channel, const void *data, uint32_t len);

/* messages.c */
LIBSSH_API int ssh_message_reply_default(ssh_message msg);

LIBSSH_API char *ssh_message_auth_user(ssh_message msg);
LIBSSH_API char *ssh_message_auth_password(ssh_message msg);
LIBSSH_API ssh_public_key ssh_message_auth_publickey(ssh_message msg);
LIBSSH_API int ssh_message_auth_reply_success(ssh_message msg,int partial);
LIBSSH_API int ssh_message_auth_reply_pk_ok(ssh_message msg, ssh_string algo, ssh_string pubkey);
LIBSSH_API int ssh_message_auth_set_methods(ssh_message msg, int methods);

LIBSSH_API int ssh_message_service_reply_success(ssh_message msg);
LIBSSH_API char *ssh_message_service_service(ssh_message msg);

LIBSSH_API void ssh_set_message_callback(ssh_session session,
    int(*ssh_message_callback)(ssh_session session, ssh_message msg));

LIBSSH_API char *ssh_message_channel_request_open_originator(ssh_message msg);
LIBSSH_API int ssh_message_channel_request_open_originator_port(ssh_message msg);
LIBSSH_API char *ssh_message_channel_request_open_destination(ssh_message msg);
LIBSSH_API int ssh_message_channel_request_open_destination_port(ssh_message msg);

LIBSSH_API ssh_channel ssh_message_channel_request_channel(ssh_message msg);

LIBSSH_API char *ssh_message_channel_request_pty_term(ssh_message msg);
LIBSSH_API int ssh_message_channel_request_pty_width(ssh_message msg);
LIBSSH_API int ssh_message_channel_request_pty_height(ssh_message msg);
LIBSSH_API int ssh_message_channel_request_pty_pxwidth(ssh_message msg);
LIBSSH_API int ssh_message_channel_request_pty_pxheight(ssh_message msg);

LIBSSH_API char *ssh_message_channel_request_env_name(ssh_message msg);
LIBSSH_API char *ssh_message_channel_request_env_value(ssh_message msg);

LIBSSH_API char *ssh_message_channel_request_command(ssh_message msg);

LIBSSH_API char *ssh_message_channel_request_subsystem(ssh_message msg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SERVER_H */

/**
 * @}
 */
/* vim: set ts=2 sw=2 et cindent: */
