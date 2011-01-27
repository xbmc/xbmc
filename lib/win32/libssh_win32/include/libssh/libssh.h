/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2003-2009 by Aris Adamantiadis
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

#ifndef _LIBSSH_H
#define _LIBSSH_H

#ifdef LIBSSH_STATIC
  #define LIBSSH_API
#else
  #if defined _WIN32 || defined __CYGWIN__
    #ifdef LIBSSH_EXPORTS
      #ifdef __GNUC__
        #define LIBSSH_API __attribute__((dllexport))
      #else
        #define LIBSSH_API __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define LIBSSH_API __attribute__((dllimport))
      #else
        #define LIBSSH_API __declspec(dllimport)
      #endif
    #endif
  #else
    #if __GNUC__ >= 4
      #define LIBSSH_API __attribute__((visibility("default")))
    #else
      #define LIBSSH_API
    #endif
  #endif
#endif

#ifdef _MSC_VER
  /* Visual Studio hasn't inttypes.h so it doesn't know uint32_t */
  typedef int int32_t;
  typedef unsigned int uint32_t;
  typedef unsigned short uint16_t;
  typedef unsigned char uint8_t;
  typedef unsigned long long uint64_t;
  typedef int mode_t;
#else /* _MSC_VER */
  #include <unistd.h>
  #include <inttypes.h>
#endif /* _MSC_VER */

#ifdef _WIN32
  #include <winsock2.h>
#else /* _WIN32 */
 #include <sys/select.h> /* for fd_set * */
 #include <netdb.h>
#endif /* _WIN32 */

#define SSH_STRINGIFY(s) SSH_TOSTRING(s)
#define SSH_TOSTRING(s) #s

/* libssh version macros */
#define SSH_VERSION_INT(a, b, c) ((a) << 16 | (b) << 8 | (c))
#define SSH_VERSION_DOT(a, b, c) a ##.## b ##.## c
#define SSH_VERSION(a, b, c) SSH_VERSION_DOT(a, b, c)

/* libssh version */
#define LIBSSH_VERSION_MAJOR  0
#define LIBSSH_VERSION_MINOR  4
#define LIBSSH_VERSION_MICRO  1

#define LIBSSH_VERSION_INT SSH_VERSION_INT(LIBSSH_VERSION_MAJOR, \
                                           LIBSSH_VERSION_MINOR, \
                                           LIBSSH_VERSION_MICRO)
#define LIBSSH_VERSION     SSH_VERSION(LIBSSH_VERSION_MAJOR, \
                                       LIBSSH_VERSION_MINOR, \
                                       LIBSSH_VERSION_MICRO)

/* GCC have printf type attribute check.  */
#ifdef __GNUC__
#define PRINTF_ATTRIBUTE(a,b) __attribute__ ((__format__ (__printf__, a, b)))
#else
#define PRINTF_ATTRIBUTE(a,b)
#endif /* __GNUC__ */

#ifdef __GNUC__
#define SSH_DEPRECATED __attribute__ ((deprecated))
#else
#define SSH_DEPRECATED
#endif

#ifdef __cplusplus
extern "C" {
#endif


typedef struct ssh_agent_struct* ssh_agent;
typedef struct ssh_buffer_struct* ssh_buffer;
typedef struct ssh_channel_struct* ssh_channel;
typedef struct ssh_message_struct* ssh_message;
typedef struct ssh_pcap_file_struct* ssh_pcap_file;
typedef struct ssh_private_key_struct* ssh_private_key;
typedef struct ssh_public_key_struct* ssh_public_key;
typedef struct ssh_scp_struct* ssh_scp;
typedef struct ssh_session_struct* ssh_session;
typedef struct ssh_string_struct* ssh_string;

/* Socket type */
#ifdef _WIN32
#define socket_t SOCKET
#else
typedef int socket_t;
#endif

/* the offsets of methods */
enum ssh_kex_types_e {
	SSH_KEX=0,
	SSH_HOSTKEYS,
	SSH_CRYPT_C_S,
	SSH_CRYPT_S_C,
	SSH_MAC_C_S,
	SSH_MAC_S_C,
	SSH_COMP_C_S,
	SSH_COMP_S_C,
	SSH_LANG_C_S,
	SSH_LANG_S_C
};

#define SSH_CRYPT 2
#define SSH_MAC 3
#define SSH_COMP 4
#define SSH_LANG 5

enum ssh_auth_e {
	SSH_AUTH_SUCCESS=0,
	SSH_AUTH_DENIED,
	SSH_AUTH_PARTIAL,
	SSH_AUTH_INFO,
	SSH_AUTH_ERROR=-1
};

/* auth flags */
#define SSH_AUTH_METHOD_UNKNOWN 0
#define SSH_AUTH_METHOD_NONE 0x0001
#define SSH_AUTH_METHOD_PASSWORD 0x0002
#define SSH_AUTH_METHOD_PUBLICKEY 0x0004
#define SSH_AUTH_METHOD_HOSTBASED 0x0008
#define SSH_AUTH_METHOD_INTERACTIVE 0x0010

/* messages */
enum ssh_requests_e {
	SSH_REQUEST_AUTH=1,
	SSH_REQUEST_CHANNEL_OPEN,
	SSH_REQUEST_CHANNEL,
	SSH_REQUEST_SERVICE,
	SSH_REQUEST_GLOBAL,
};

enum ssh_channel_type_e {
	SSH_CHANNEL_UNKNOWN=0,
	SSH_CHANNEL_SESSION,
	SSH_CHANNEL_DIRECT_TCPIP,
	SSH_CHANNEL_FORWARDED_TCPIP,
	SSH_CHANNEL_X11
};

enum ssh_channel_requests_e {
	SSH_CHANNEL_REQUEST_UNKNOWN=0,
	SSH_CHANNEL_REQUEST_PTY,
	SSH_CHANNEL_REQUEST_EXEC,
	SSH_CHANNEL_REQUEST_SHELL,
	SSH_CHANNEL_REQUEST_ENV,
	SSH_CHANNEL_REQUEST_SUBSYSTEM,
	SSH_CHANNEL_REQUEST_WINDOW_CHANGE,
};

/* status flags */
#define SSH_CLOSED 0x01
#define SSH_READ_PENDING 0x02
#define SSH_CLOSED_ERROR 0x04

enum ssh_server_known_e {
	SSH_SERVER_ERROR=-1,
	SSH_SERVER_NOT_KNOWN=0,
	SSH_SERVER_KNOWN_OK,
	SSH_SERVER_KNOWN_CHANGED,
	SSH_SERVER_FOUND_OTHER,
	SSH_SERVER_FILE_NOT_FOUND,
};

#ifndef MD5_DIGEST_LEN
    #define MD5_DIGEST_LEN 16
#endif
/* errors */

enum ssh_error_types_e {
	SSH_NO_ERROR=0,
	SSH_REQUEST_DENIED,
	SSH_FATAL,
	SSH_EINTR
};

/* Error return codes */
#define SSH_OK 0     /* No error */
#define SSH_ERROR -1 /* Error of some kind */
#define SSH_AGAIN -2 /* The nonblocking call must be repeated */
#define SSH_EOF -127 /* We have already a eof */

/** \addtogroup ssh_log
 * @{
 */
 /** \brief Verbosity level for logging and help to debugging
  */

enum {
	/** No logging at all
	 */
	SSH_LOG_NOLOG=0,
	/** Only rare and noteworthy events
	 */
	SSH_LOG_RARE,
	/** High level protocol informations
	 */
	SSH_LOG_PROTOCOL,
	/** Lower level protocol infomations, packet level
	 */
	SSH_LOG_PACKET,
	/** Every function path
	 */
	SSH_LOG_FUNCTIONS
};
/** @}
 */

enum ssh_options_e {
  SSH_OPTIONS_HOST,
  SSH_OPTIONS_PORT,
  SSH_OPTIONS_PORT_STR,
  SSH_OPTIONS_FD,
  SSH_OPTIONS_USER,
  SSH_OPTIONS_SSH_DIR,
  SSH_OPTIONS_IDENTITY,
  SSH_OPTIONS_ADD_IDENTITY,
  SSH_OPTIONS_KNOWNHOSTS,
  SSH_OPTIONS_TIMEOUT,
  SSH_OPTIONS_TIMEOUT_USEC,
  SSH_OPTIONS_SSH1,
  SSH_OPTIONS_SSH2,
  SSH_OPTIONS_LOG_VERBOSITY,
  SSH_OPTIONS_LOG_VERBOSITY_STR,

  SSH_OPTIONS_CIPHERS_C_S,
  SSH_OPTIONS_CIPHERS_S_C,
  SSH_OPTIONS_COMPRESSION_C_S,
  SSH_OPTIONS_COMPRESSION_S_C
};

enum {
  /** Code is going to write/create remote files */
  SSH_SCP_WRITE,
  /** Code is going to read remote files */
  SSH_SCP_READ,
  SSH_SCP_RECURSIVE=0x10
};

enum ssh_scp_request_types {
  /** A new directory is going to be pulled */
  SSH_SCP_REQUEST_NEWDIR=1,
  /** A new file is going to be pulled */
  SSH_SCP_REQUEST_NEWFILE,
  /** End of requests */
  SSH_SCP_REQUEST_EOF,
  /** End of directory */
  SSH_SCP_REQUEST_ENDDIR,
  /** Warning received */
  SSH_SCP_REQUEST_WARNING
};

LIBSSH_API void buffer_free(ssh_buffer buffer);
LIBSSH_API void *buffer_get(ssh_buffer buffer);
LIBSSH_API uint32_t buffer_get_len(ssh_buffer buffer);
LIBSSH_API ssh_buffer buffer_new(void);

LIBSSH_API ssh_channel channel_accept_x11(ssh_channel channel, int timeout_ms);
LIBSSH_API int channel_change_pty_size(ssh_channel channel,int cols,int rows);
LIBSSH_API ssh_channel channel_forward_accept(ssh_session session, int timeout_ms);
LIBSSH_API int channel_close(ssh_channel channel);
LIBSSH_API int channel_forward_cancel(ssh_session session, const char *address, int port);
LIBSSH_API int channel_forward_listen(ssh_session session, const char *address, int port, int *bound_port);
LIBSSH_API void channel_free(ssh_channel channel);
LIBSSH_API int channel_get_exit_status(ssh_channel channel);
LIBSSH_API ssh_session channel_get_session(ssh_channel channel);
LIBSSH_API int channel_is_closed(ssh_channel channel);
LIBSSH_API int channel_is_eof(ssh_channel channel);
LIBSSH_API int channel_is_open(ssh_channel channel);
LIBSSH_API ssh_channel channel_new(ssh_session session);
LIBSSH_API int channel_open_forward(ssh_channel channel, const char *remotehost,
    int remoteport, const char *sourcehost, int localport);
LIBSSH_API int channel_open_session(ssh_channel channel);
LIBSSH_API int channel_poll(ssh_channel channel, int is_stderr);
LIBSSH_API int channel_read(ssh_channel channel, void *dest, uint32_t count, int is_stderr);
LIBSSH_API int channel_read_buffer(ssh_channel channel, ssh_buffer buffer, uint32_t count,
    int is_stderr);
LIBSSH_API int channel_read_nonblocking(ssh_channel channel, void *dest, uint32_t count,
    int is_stderr);
LIBSSH_API int channel_request_env(ssh_channel channel, const char *name, const char *value);
LIBSSH_API int channel_request_exec(ssh_channel channel, const char *cmd);
LIBSSH_API int channel_request_pty(ssh_channel channel);
LIBSSH_API int channel_request_pty_size(ssh_channel channel, const char *term,
    int cols, int rows);
LIBSSH_API int channel_request_shell(ssh_channel channel);
LIBSSH_API int channel_request_send_signal(ssh_channel channel, const char *signum);
LIBSSH_API int channel_request_sftp(ssh_channel channel);
LIBSSH_API int channel_request_subsystem(ssh_channel channel, const char *subsystem);
LIBSSH_API int channel_request_x11(ssh_channel channel, int single_connection, const char *protocol,
    const char *cookie, int screen_number);
LIBSSH_API int channel_send_eof(ssh_channel channel);
LIBSSH_API int channel_select(ssh_channel *readchans, ssh_channel *writechans, ssh_channel *exceptchans, struct
        timeval * timeout);
LIBSSH_API void channel_set_blocking(ssh_channel channel, int blocking);
LIBSSH_API int channel_write(ssh_channel channel, const void *data, uint32_t len);

LIBSSH_API void privatekey_free(ssh_private_key prv);
LIBSSH_API ssh_private_key privatekey_from_file(ssh_session session, const char *filename,
    int type, const char *passphrase);
LIBSSH_API void publickey_free(ssh_public_key key);
LIBSSH_API int ssh_publickey_to_file(ssh_session session, const char *file,
    ssh_string pubkey, int type);
LIBSSH_API ssh_string publickey_from_file(ssh_session session, const char *filename,
    int *type);
LIBSSH_API ssh_public_key publickey_from_privatekey(ssh_private_key prv);
LIBSSH_API ssh_string publickey_to_string(ssh_public_key key);
LIBSSH_API int ssh_try_publickey_from_file(ssh_session session, const char *keyfile,
    ssh_string *publickey, int *type);

LIBSSH_API int ssh_auth_list(ssh_session session);
LIBSSH_API char *ssh_basename (const char *path);
LIBSSH_API void ssh_clean_pubkey_hash(unsigned char **hash);
LIBSSH_API int ssh_connect(ssh_session session);
LIBSSH_API const char *ssh_copyright(void);
LIBSSH_API void ssh_disconnect(ssh_session session);
LIBSSH_API char *ssh_dirname (const char *path);
LIBSSH_API int ssh_finalize(void);
LIBSSH_API void ssh_free(ssh_session session);
LIBSSH_API const char *ssh_get_disconnect_message(ssh_session session);
LIBSSH_API const char *ssh_get_error(void *error);
LIBSSH_API int ssh_get_error_code(void *error);
LIBSSH_API socket_t ssh_get_fd(ssh_session session);
LIBSSH_API char *ssh_get_hexa(const unsigned char *what, size_t len);
LIBSSH_API char *ssh_get_issue_banner(ssh_session session);
LIBSSH_API int ssh_get_openssh_version(ssh_session session);
LIBSSH_API ssh_string ssh_get_pubkey(ssh_session session);
LIBSSH_API int ssh_get_pubkey_hash(ssh_session session, unsigned char **hash);
LIBSSH_API int ssh_get_random(void *where,int len,int strong);
LIBSSH_API int ssh_get_version(ssh_session session);
LIBSSH_API int ssh_get_status(ssh_session session);
LIBSSH_API int ssh_init(void);
LIBSSH_API int ssh_is_server_known(ssh_session session);
LIBSSH_API void ssh_log(ssh_session session, int prioriry, const char *format, ...) PRINTF_ATTRIBUTE(3, 4);
LIBSSH_API ssh_channel ssh_message_channel_request_open_reply_accept(ssh_message msg);
LIBSSH_API int ssh_message_channel_request_reply_success(ssh_message msg);
LIBSSH_API void ssh_message_free(ssh_message msg);
LIBSSH_API ssh_message ssh_message_get(ssh_session session);
LIBSSH_API ssh_message ssh_message_retrieve(ssh_session session, uint32_t packettype);
LIBSSH_API int ssh_message_subtype(ssh_message msg);
LIBSSH_API int ssh_message_type(ssh_message msg);
LIBSSH_API int ssh_mkdir (const char *pathname, mode_t mode);
LIBSSH_API ssh_session ssh_new(void);

LIBSSH_API int ssh_options_copy(ssh_session src, ssh_session *dest);
LIBSSH_API int ssh_options_getopt(ssh_session session, int *argcptr, char **argv);
LIBSSH_API int ssh_options_parse_config(ssh_session session, const char *filename);
LIBSSH_API int ssh_options_set(ssh_session session, enum ssh_options_e type,
    const void *value);
LIBSSH_API int ssh_pcap_file_close(ssh_pcap_file pcap);
LIBSSH_API void ssh_pcap_file_free(ssh_pcap_file pcap);
LIBSSH_API ssh_pcap_file ssh_pcap_file_new(void);
LIBSSH_API int ssh_pcap_file_open(ssh_pcap_file pcap, const char *filename);
LIBSSH_API void ssh_print_hexa(const char *descr, const unsigned char *what, size_t len);
LIBSSH_API int ssh_scp_accept_request(ssh_scp scp);
LIBSSH_API int ssh_scp_close(ssh_scp scp);
LIBSSH_API int ssh_scp_deny_request(ssh_scp scp, const char *reason);
LIBSSH_API void ssh_scp_free(ssh_scp scp);
LIBSSH_API int ssh_scp_init(ssh_scp scp);
LIBSSH_API int ssh_scp_leave_directory(ssh_scp scp);
LIBSSH_API ssh_scp ssh_scp_new(ssh_session session, int mode, const char *location);
LIBSSH_API int ssh_scp_pull_request(ssh_scp scp);
LIBSSH_API int ssh_scp_push_directory(ssh_scp scp, const char *dirname, int mode);
LIBSSH_API int ssh_scp_push_file(ssh_scp scp, const char *filename, size_t size, int perms);
LIBSSH_API int ssh_scp_read(ssh_scp scp, void *buffer, size_t size);
LIBSSH_API const char *ssh_scp_request_get_filename(ssh_scp scp);
LIBSSH_API int ssh_scp_request_get_permissions(ssh_scp scp);
LIBSSH_API size_t ssh_scp_request_get_size(ssh_scp scp);
LIBSSH_API const char *ssh_scp_request_get_warning(ssh_scp scp);
LIBSSH_API int ssh_scp_write(ssh_scp scp, const void *buffer, size_t len);
LIBSSH_API int ssh_select(ssh_channel *channels, ssh_channel *outchannels, socket_t maxfd,
    fd_set *readfds, struct timeval *timeout);
LIBSSH_API int ssh_service_request(ssh_session session, const char *service);
LIBSSH_API void ssh_set_blocking(ssh_session session, int blocking);
LIBSSH_API void ssh_set_fd_except(ssh_session session);
LIBSSH_API void ssh_set_fd_toread(ssh_session session);
LIBSSH_API void ssh_set_fd_towrite(ssh_session session);
LIBSSH_API void ssh_silent_disconnect(ssh_session session);
LIBSSH_API int ssh_set_pcap_file(ssh_session session, ssh_pcap_file pcapfile);
#ifndef _WIN32
LIBSSH_API int ssh_userauth_agent_pubkey(ssh_session session, const char *username,
    ssh_public_key publickey);
#endif
LIBSSH_API int ssh_userauth_autopubkey(ssh_session session, const char *passphrase);
LIBSSH_API int ssh_userauth_kbdint(ssh_session session, const char *user, const char *submethods);
LIBSSH_API const char *ssh_userauth_kbdint_getinstruction(ssh_session session);
LIBSSH_API const char *ssh_userauth_kbdint_getname(ssh_session session);
LIBSSH_API int ssh_userauth_kbdint_getnprompts(ssh_session session);
LIBSSH_API const char *ssh_userauth_kbdint_getprompt(ssh_session session, unsigned int i, char *echo);
LIBSSH_API int ssh_userauth_kbdint_setanswer(ssh_session session, unsigned int i,
    const char *answer);
LIBSSH_API int ssh_userauth_list(ssh_session session, const char *username);
LIBSSH_API int ssh_userauth_none(ssh_session session, const char *username);
LIBSSH_API int ssh_userauth_offer_pubkey(ssh_session session, const char *username, int type, ssh_string publickey);
LIBSSH_API int ssh_userauth_password(ssh_session session, const char *username, const char *password);
LIBSSH_API int ssh_userauth_pubkey(ssh_session session, const char *username, ssh_string publickey, ssh_private_key privatekey);
LIBSSH_API const char *ssh_version(int req_version);
LIBSSH_API int ssh_write_knownhost(ssh_session session);

LIBSSH_API void string_burn(ssh_string str);
LIBSSH_API ssh_string string_copy(ssh_string str);
LIBSSH_API void *string_data(ssh_string str);
LIBSSH_API int string_fill(ssh_string str, const void *data, size_t len);
LIBSSH_API void string_free(ssh_string str);
LIBSSH_API ssh_string string_from_char(const char *what);
LIBSSH_API size_t string_len(ssh_string str);
LIBSSH_API ssh_string string_new(size_t size);
LIBSSH_API char *string_to_char(ssh_string str);


#ifdef __cplusplus
}
#endif
#endif /* _LIBSSH_H */
/* vim: set ts=2 sw=2 et cindent: */
