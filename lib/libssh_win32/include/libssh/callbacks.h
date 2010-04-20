/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009 Aris Adamantiadis <aris@0xbadc0de.be>
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

/* callback.h
 * This file includes the public declarations for the libssh callback mechanism
 */

#ifndef _SSH_CALLBACK_H
#define _SSH_CALLBACK_H

#include <libssh/libssh.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SSH authentication callback.
 *
 * @param prompt        Prompt to be displayed.
 * @param buf           Buffer to save the password. You should null-terminate it.
 * @param len           Length of the buffer.
 * @param echo          Enable or disable the echo of what you type.
 * @param verify        Should the password be verified?
 * @param userdata      Userdata to be passed to the callback function. Useful
 *                      for GUI applications.
 *
 * @return              0 on success, < 0 on error.
 */
typedef int (*ssh_auth_callback) (const char *prompt, char *buf, size_t len,
    int echo, int verify, void *userdata);
typedef void (*ssh_log_callback) (ssh_session session, int priority,
    const char *message, void *userdata);
/** this callback will be called with status going from 0.0 to 1.0 during
 * connection */
typedef void (*ssh_status_callback) (ssh_session session, float status,
		void *userdata);

struct ssh_callbacks_struct {
	/** size of this structure. internal, shoud be set with ssh_callbacks_init()*/
	size_t size;
	/** User-provided data. User is free to set anything he wants here */
	void *userdata;
	/** this functions will be called if e.g. a keyphrase is needed. */
	ssh_auth_callback auth_function;
	/** this function will be called each time a loggable event happens. */
	ssh_log_callback log_function;
	/** this function gets called during connection time to indicate the percentage
	 * of connection steps completed.
	 */
  void (*connect_status_function)(void *userdata, float status);
};

typedef struct ssh_callbacks_struct * ssh_callbacks;

/** Initializes an ssh_callbacks_struct
 * A call to this macro is mandatory when you have set a new
 * ssh_callback_struct structure. Its goal is to maintain the binary
 * compatibility with future versions of libssh as the structure
 * evolves with time.
 */
#define ssh_callbacks_init(p) do {\
	(p)->size=sizeof(*(p)); \
} while(0);

/**
 * @brief Set the callback functions.
 *
 * This functions sets the callback structure to use your own callback
 * functions for auth, logging and status.
 *
 * @code
 * struct ssh_callbacks_struct cb;
 * memset(&cb, 0, sizeof(struct ssh_callbacks_struct));
 * cb.userdata = data;
 * cb.auth_function = my_auth_function;
 *
 * ssh_callbacks_init(&cb);
 * ssh_set_callbacks(session, &cb);
 * @endcode
 *
 * @param  session      The session to set the callback structure.
 *
 * @param  cb           The callback itself.
 *
 * @return 0 on success, < 0 on error.
 */
LIBSSH_API int ssh_set_callbacks(ssh_session session, ssh_callbacks cb);

#ifdef __cplusplus
}
#endif

#endif /*_SSH_CALLBACK_H */
