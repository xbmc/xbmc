/*
     This file is part of libmicrohttpd
     (C) 2006, 2007, 2008, 2009 Christian Grothoff (and other contributing authors)

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
 * @file microhttpd.h
 * @brief public interface to libmicrohttpd
 * @author Christian Grothoff
 * @author Chris GauthierDickey
 *
 * All symbols defined in this header start with MHD.  MHD is a small
 * HTTP daemon library.  As such, it does not have any API for logging
 * errors (you can only enable or disable logging to stderr).  Also,
 * it may not support all of the HTTP features directly, where
 * applicable, portions of HTTP may have to be handled by clients of
 * the library.<p>
 *
 * The library is supposed to handle everything that it must handle
 * (because the API would not allow clients to do this), such as basic
 * connection management; however, detailed interpretations of headers
 * -- such as range requests -- and HTTP methods are left to clients.
 * The library does understand HEAD and will only send the headers of
 * the response and not the body, even if the client supplied a body.
 * The library also understands headers that control connection
 * management (specifically, "Connection: close" and "Expect: 100
 * continue" are understood and handled automatically).<p>
 *
 * MHD understands POST data and is able to decode certain formats
 * (at the moment only "application/x-www-form-urlencoded") if the
 * entire data fits into the allowed amount of memory for the
 * connection.  Unsupported encodings and large POST submissions are
 * provided as a stream to the main application (and thus can be
 * processed, just not conveniently by MHD).<p>
 *
 * The header file defines various constants used by the HTTP protocol.
 * This does not mean that MHD actually interprets all of these
 * values.  The provided constants are exported as a convenience
 * for users of the library.  MHD does not verify that transmitted
 * HTTP headers are part of the standard specification; users of the
 * library are free to define their own extensions of the HTTP
 * standard and use those with MHD.<p>
 *
 * All functions are guaranteed to be completely reentrant and
 * thread-safe (with the exception of 'MHD_set_connection_value',
 * which must only be used in a particular context).<p>
 *
 * NEW: Before including "microhttpd.h" you should add the necessary
 * includes to define the "uint64_t", "size_t", "fd_set", "socklen_t"
 * and "struct sockaddr" data types (which headers are needed may
 * depend on your platform; for possible suggestions consult
 * "platform.h" in the MHD distribution).
 *
 */

#ifndef MHD_MICROHTTPD_H
#define MHD_MICROHTTPD_H

#ifdef __cplusplus
extern "C"
{
#if 0                           /* keep Emacsens' auto-indent happy */
}
#endif
#endif

/**
 * Current version of the library.
 */
#define MHD_VERSION 0x00040500

/**
 * MHD-internal return code for "YES".
 */
#define MHD_YES 1

/**
 * MHD-internal return code for "NO".
 */
#define MHD_NO 0

/**
 * Constant used to indicate unknown size (use when
 * creating a response).
 */
#define MHD_SIZE_UNKNOWN  ((uint64_t) -1LL)

/**
 * HTTP response codes.
 */
#define MHD_HTTP_CONTINUE 100
#define MHD_HTTP_SWITCHING_PROTOCOLS 101
#define MHD_HTTP_PROCESSING 102

#define MHD_HTTP_OK 200
#define MHD_HTTP_CREATED 201
#define MHD_HTTP_ACCEPTED 202
#define MHD_HTTP_NON_AUTHORITATIVE_INFORMATION 203
#define MHD_HTTP_NO_CONTENT 204
#define MHD_HTTP_RESET_CONTENT 205
#define MHD_HTTP_PARTIAL_CONTENT 206
#define MHD_HTTP_MULTI_STATUS 207

#define MHD_HTTP_MULTIPLE_CHOICES 300
#define MHD_HTTP_MOVED_PERMANENTLY 301
#define MHD_HTTP_FOUND 302
#define MHD_HTTP_SEE_OTHER 303
#define MHD_HTTP_NOT_MODIFIED 304
#define MHD_HTTP_USE_PROXY 305
#define MHD_HTTP_SWITCH_PROXY 306
#define MHD_HTTP_TEMPORARY_REDIRECT 307

#define MHD_HTTP_BAD_REQUEST 400
#define MHD_HTTP_UNAUTHORIZED 401
#define MHD_HTTP_PAYMENT_REQUIRED 402
#define MHD_HTTP_FORBIDDEN 403
#define MHD_HTTP_NOT_FOUND 404
#define MHD_HTTP_METHOD_NOT_ALLOWED 405
#define MHD_HTTP_METHOD_NOT_ACCEPTABLE 406
#define MHD_HTTP_PROXY_AUTHENTICATION_REQUIRED 407
#define MHD_HTTP_REQUEST_TIMEOUT 408
#define MHD_HTTP_CONFLICT 409
#define MHD_HTTP_GONE 410
#define MHD_HTTP_LENGTH_REQUIRED 411
#define MHD_HTTP_PRECONDITION_FAILED 412
#define MHD_HTTP_REQUEST_ENTITY_TOO_LARGE 413
#define MHD_HTTP_REQUEST_URI_TOO_LONG 414
#define MHD_HTTP_UNSUPPORTED_MEDIA_TYPE 415
#define MHD_HTTP_REQUESTED_RANGE_NOT_SATISFIABLE 416
#define MHD_HTTP_EXPECTATION_FAILED 417
#define MHD_HTTP_UNPROCESSABLE_ENTITY 422
#define MHD_HTTP_LOCKED 423
#define MHD_HTTP_FAILED_DEPENDENCY 424
#define MHD_HTTP_UNORDERED_COLLECTION 425
#define MHD_HTTP_UPGRADE_REQUIRED 426
#define MHD_HTTP_RETRY_WITH 449

#define MHD_HTTP_INTERNAL_SERVER_ERROR 500
#define MHD_HTTP_NOT_IMPLEMENTED 501
#define MHD_HTTP_BAD_GATEWAY 502
#define MHD_HTTP_SERVICE_UNAVAILABLE 503
#define MHD_HTTP_GATEWAY_TIMEOUT 504
#define MHD_HTTP_HTTP_VERSION_NOT_SUPPORTED 505
#define MHD_HTTP_VARIANT_ALSO_NEGOTIATES 506
#define MHD_HTTP_INSUFFICIENT_STORAGE 507
#define MHD_HTTP_BANDWIDTH_LIMIT_EXCEEDED 509
#define MHD_HTTP_NOT_EXTENDED 510

/* See also: http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html */
#define MHD_HTTP_HEADER_ACCEPT "Accept"
#define MHD_HTTP_HEADER_ACCEPT_CHARSET "Accept-Charset"
#define MHD_HTTP_HEADER_ACCEPT_ENCODING "Accept-Encoding"
#define MHD_HTTP_HEADER_ACCEPT_LANGUAGE "Accept-Language"
#define MHD_HTTP_HEADER_ACCEPT_RANGES "Accept-Ranges"
#define MHD_HTTP_HEADER_AGE "Age"
#define MHD_HTTP_HEADER_ALLOW "Allow"
#define MHD_HTTP_HEADER_AUTHORIZATION "Authorization"
#define MHD_HTTP_HEADER_CACHE_CONTROL "Cache-Control"
#define MHD_HTTP_HEADER_CONNECTION "Connection"
#define MHD_HTTP_HEADER_CONTENT_ENCODING "Content-Encoding"
#define MHD_HTTP_HEADER_CONTENT_LANGUAGE "Content-Language"
#define MHD_HTTP_HEADER_CONTENT_LENGTH "Content-Length"
#define MHD_HTTP_HEADER_CONTENT_LOCATION "Content-Location"
#define MHD_HTTP_HEADER_CONTENT_MD5 "Content-MD5"
#define MHD_HTTP_HEADER_CONTENT_RANGE "Content-Range"
#define MHD_HTTP_HEADER_CONTENT_TYPE "Content-Type"
#define MHD_HTTP_HEADER_COOKIE "Cookie"
#define MHD_HTTP_HEADER_DATE "Date"
#define MHD_HTTP_HEADER_ETAG "ETag"
#define MHD_HTTP_HEADER_EXPECT "Expect"
#define MHD_HTTP_HEADER_EXPIRES "Expires"
#define MHD_HTTP_HEADER_FROM "From"
#define MHD_HTTP_HEADER_HOST "Host"
#define MHD_HTTP_HEADER_IF_MATCH "If-Match"
#define MHD_HTTP_HEADER_IF_MODIFIED_SINCE "If-Modified-Since"
#define MHD_HTTP_HEADER_IF_NONE_MATCH "If-None-Match"
#define MHD_HTTP_HEADER_IF_RANGE "If-Range"
#define MHD_HTTP_HEADER_IF_UNMODIFIED_SINCE "If-Unmodified-Since"
#define MHD_HTTP_HEADER_LAST_MODIFIED "Last-Modified"
#define MHD_HTTP_HEADER_LOCATION "Location"
#define MHD_HTTP_HEADER_MAX_FORWARDS "Max-Forwards"
#define MHD_HTTP_HEADER_PRAGMA "Pragma"
#define MHD_HTTP_HEADER_PROXY_AUTHENTICATE "Proxy-Authenticate"
#define MHD_HTTP_HEADER_PROXY_AUTHORIZATION "Proxy-Authorization"
#define MHD_HTTP_HEADER_RANGE "Range"
#define MHD_HTTP_HEADER_REFERER "Referer"
#define MHD_HTTP_HEADER_RETRY_AFTER "Retry-After"
#define MHD_HTTP_HEADER_SERVER "Server"
#define MHD_HTTP_HEADER_SET_COOKIE "Set-Cookie"
#define MHD_HTTP_HEADER_SET_COOKIE2 "Set-Cookie2"
#define MHD_HTTP_HEADER_TE "TE"
#define MHD_HTTP_HEADER_TRAILER "Trailer"
#define MHD_HTTP_HEADER_TRANSFER_ENCODING "Transfer-Encoding"
#define MHD_HTTP_HEADER_UPGRADE "Upgrade"
#define MHD_HTTP_HEADER_USER_AGENT "User-Agent"
#define MHD_HTTP_HEADER_VARY "Vary"
#define MHD_HTTP_HEADER_VIA "Via"
#define MHD_HTTP_HEADER_WARNING "Warning"
#define MHD_HTTP_HEADER_WWW_AUTHENTICATE "WWW-Authenticate"

/**
 * HTTP versions (used to match against the first line of the
 * HTTP header as well as in the response code).
 */
#define MHD_HTTP_VERSION_1_0 "HTTP/1.0"
#define MHD_HTTP_VERSION_1_1 "HTTP/1.1"

/**
 * HTTP methods
 */
#define MHD_HTTP_METHOD_CONNECT "CONNECT"
#define MHD_HTTP_METHOD_DELETE "DELETE"
#define MHD_HTTP_METHOD_GET "GET"
#define MHD_HTTP_METHOD_HEAD "HEAD"
#define MHD_HTTP_METHOD_OPTIONS "OPTIONS"
#define MHD_HTTP_METHOD_POST "POST"
#define MHD_HTTP_METHOD_PUT "PUT"
#define MHD_HTTP_METHOD_TRACE "TRACE"

/**
 * HTTP POST encodings, see also
 * http://www.w3.org/TR/html4/interact/forms.html#h-17.13.4
 */
#define MHD_HTTP_POST_ENCODING_FORM_URLENCODED "application/x-www-form-urlencoded"
#define MHD_HTTP_POST_ENCODING_MULTIPART_FORMDATA "multipart/form-data"

/**
 * Options for the MHD daemon.  Note that if neither
 * MHD_USER_THREAD_PER_CONNECTION nor MHD_USE_SELECT_INTERNALLY are
 * used, the client wants control over the process and will call the
 * appropriate microhttpd callbacks.<p>
 *
 * Starting the daemon may also fail if a particular option is not
 * implemented or not supported on the target platform (i.e. no
 * support for SSL, threads or IPv6).
 */
enum MHD_FLAG
{
  /**
   * No options selected.
   */
  MHD_NO_FLAG = 0,

  /**
   * Run in debug mode.  If this flag is used, the
   * library should print error messages and warnings
   * to stderr.
   */
  MHD_USE_DEBUG = 1,

  /**
   * Run in https mode.
   */
  MHD_USE_SSL = 2,

  /**
   * Run using one thread per connection.
   */
  MHD_USE_THREAD_PER_CONNECTION = 4,

  /**
   * Run using an internal thread doing SELECT.
   */
  MHD_USE_SELECT_INTERNALLY = 8,

  /**
   * Run using the IPv6 protocol (otherwise, MHD will
   * just support IPv4).
   */
  MHD_USE_IPv6 = 16,

  /**
   * Be pedantic about the protocol (as opposed to as tolerant as
   * possible).  Specifically, at the moment, this flag causes MHD to
   * reject http 1.1 connections without a "Host" header.  This is
   * required by the standard, but of course in violation of the "be
   * as liberal as possible in what you accept" norm.  It is
   * recommended to turn this ON if you are testing clients against
   * MHD, and OFF in production.
   */
  MHD_USE_PEDANTIC_CHECKS = 32,

  /**
   * Use poll instead of select. This allows sockets with fd >= FD_SETSIZE.
   * This option only works in conjunction with MHD_USE_THREAD_PER_CONNECTION
   * (at this point).
   */
  MHD_USE_POLL = 64
};

/**
 * MHD options.  Passed in the varargs portion
 * of MHD_start_daemon.
 */
enum MHD_OPTION
{

  /**
   * No more options / last option.  This is used
   * to terminate the VARARGs list.
   */
  MHD_OPTION_END = 0,

  /**
   * Maximum memory size per connection (followed by a
   * size_t).
   */
  MHD_OPTION_CONNECTION_MEMORY_LIMIT = 1,

  /**
   * Maximum number of concurrent connections to
   * accept (followed by an unsigned int).
   */
  MHD_OPTION_CONNECTION_LIMIT = 2,

  /**
   * After how many seconds of inactivity should a
   * connection automatically be timed out? (followed
   * by an unsigned int; use zero for no timeout).
   */
  MHD_OPTION_CONNECTION_TIMEOUT = 3,

  /**
   * Register a function that should be called whenever a request has
   * been completed (this can be used for application-specific clean
   * up).  Requests that have never been presented to the application
   * (via MHD_AccessHandlerCallback) will not result in
   * notifications.<p>
   *
   * This option should be followed by TWO pointers.  First a pointer
   * to a function of type "MHD_RequestCompletedCallback" and second a
   * pointer to a closure to pass to the request completed callback.
   * The second pointer maybe NULL.
   */
  MHD_OPTION_NOTIFY_COMPLETED = 4,

  /**
   * Limit on the number of (concurrent) connections made to the
   * server from the same IP address.  Can be used to prevent one
   * IP from taking over all of the allowed connections.  If the
   * same IP tries to establish more than the specified number of
   * connections, they will be immediately rejected.  The option
   * should be followed by an "unsigned int".  The default is
   * zero, which means no limit on the number of connections
   * from the same IP address.
   */
  MHD_OPTION_PER_IP_CONNECTION_LIMIT = 5,

  /**
   * Bind daemon to the supplied sockaddr. this option should be followed by a
   * 'struct sockaddr *'.  If 'MHD_USE_IPv6' is specified, the 'struct sockaddr*'
   * should point to a 'struct sockaddr_in6', otherwise to a 'struct sockaddr_in'.
   */
  MHD_OPTION_SOCK_ADDR = 6,

  /**
   * Specify a function that should be called before parsing the URI from
   * the client.  The specified callback function can be used for processing
   * the URI (including the options) before it is parsed.  The URI after
   * parsing will no longer contain the options, which maybe inconvenient for
   * logging.  This option should be followed by two arguments, the first
   * one must be of the form
   * <pre>
   *  void * my_logger(void * cls, const char * uri)
   * </pre>
   * where the return value will be passed as
   * (*con_cls) in calls to the MHD_AccessHandlerCallback
   * when this request is processed later; returning a
   * value of NULL has no special significance (however,
   * note that if you return non-NULL, you can no longer
   * rely on the first call to the access handler having
   * NULL == *con_cls on entry;)
   * "cls" will be set to the second argument following
   * MHD_OPTION_URI_LOG_CALLBACK.  Finally, uri will
   * be the 0-terminated URI of the request.
   */
  MHD_OPTION_URI_LOG_CALLBACK = 7,

  /**
   * Memory pointer for the private key (key.pem) to be used by the
   * HTTPS daemon.  This option should be followed by an
   * "const char*" argument.
   * This should be used in conjunction with 'MHD_OPTION_HTTPS_MEM_CERT'.
   */
  MHD_OPTION_HTTPS_MEM_KEY = 8,

  /**
   * Memory pointer for the certificate (cert.pem) to be used by the
   * HTTPS daemon.  This option should be followed by an
   * "const char*" argument.
   * This should be used in conjunction with 'MHD_OPTION_HTTPS_MEM_KEY'.
   */
  MHD_OPTION_HTTPS_MEM_CERT = 9,

  /**
   * Daemon credentials type.
   * This option should be followed by one of the values listed in
   * "enum MHD_GNUTLS_CredentialsType".
   */
  MHD_OPTION_CRED_TYPE = 10,

  /**
   * SSL/TLS protocol version.
   *
   * Memory pointer to a zero (MHD_GNUTLS_PROTOCOL_END) terminated
   * (const) array of 'enum MHD_GNUTLS_Protocol' values representing the
   * protocol versions to this server should support. Unsupported
   * requests will be droped by the server.
   */
  MHD_OPTION_PROTOCOL_VERSION = 11,

  /**
   * Memory pointer to a zero (MHD_GNUTLS_CIPHER_UNKNOWN)
   * terminated (const) array of 'enum MHD_GNUTLS_CipherAlgorithm'
   * representing the cipher priority order to which the HTTPS
   * daemon should adhere.
   */
  MHD_OPTION_CIPHER_ALGORITHM = 12,

  /**
   * Use the given function for logging error messages.
   * This option must be followed by two arguments; the
   * first must be a pointer to a function
   * of type "void fun(void * arg, const char * fmt, va_list ap)"
   * and the second a pointer "void*" which will
   * be passed as the "arg" argument to "fun".
   * <p>
   * Note that MHD will not generate any log messages
   * if it was compiled without the "--enable-messages"
   * flag being set.
   */
  MHD_OPTION_EXTERNAL_LOGGER = 13,

  /**
   * Number (unsigned int) of threads in thread pool. Enable
   * thread pooling by setting this value to to something
   * greater than 1. Currently, thread model must be
   * MHD_USE_SELECT_INTERNALLY if thread pooling is enabled
   * (MHD_start_daemon returns NULL for an unsupported thread
   * model).
   */
  MHD_OPTION_THREAD_POOL_SIZE = 14,

  /**
   * Additional options given in an array of "struct MHD_OptionItem".
   * The array must be terminated with an entry '{MHD_OPTION_END, 0, NULL}'.
   * An example for code using MHD_OPTION_ARRAY is:
   * <code>
   * struct MHD_OptionItem ops[] = {
   * { MHD_OPTION_CONNECTION_LIMIT, 100, NULL },
   * { MHD_OPTION_CONNECTION_TIMEOUT, 10, NULL },
   * { MHD_OPTION_END, 0, NULL }
   * };
   * d = MHD_start_daemon(0, 8080, NULL, NULL, dh, NULL,
   *                      MHD_OPTION_ARRAY, ops,
   *                      MHD_OPTION_END);
   * </code>
   * For options that expect a single pointer argument, the
   * second member of the struct MHD_OptionItem is ignored.
   * For options that expect two pointer arguments, the first
   * argument must be cast to 'intptr_t'.
   */
  MHD_OPTION_ARRAY = 15
};


/**
 * Entry in an MHD_OPTION_ARRAY.
 */
struct MHD_OptionItem
{
  /**
   * Which option is being given.  Use MHD_OPTION_END
   * to terminate the array.
   */
  enum MHD_OPTION option;

  /**
   * Option value (for integer arguments, and for options requiring
   * two pointer arguments); should be 0 for options that take no
   * arguments or only a single pointer argument.
   */
  intptr_t value;

  /**
   * Pointer option value (use NULL for options taking no arguments
   * or only an integer option).
   */
  void *ptr_value;

};


/**
 * The MHD_ValueKind specifies the source of
 * the key-value pairs in the HTTP protocol.
 */
enum MHD_ValueKind
{

  /**
   * Response header
   */
  MHD_RESPONSE_HEADER_KIND = 0,

  /**
   * HTTP header.
   */
  MHD_HEADER_KIND = 1,

  /**
   * Cookies.  Note that the original HTTP header containing
   * the cookie(s) will still be available and intact.
   */
  MHD_COOKIE_KIND = 2,

  /**
   * POST data.  This is available only if a content encoding
   * supported by MHD is used (currently only URL encoding),
   * and only if the posted content fits within the available
   * memory pool.  Note that in that case, the upload data
   * given to the MHD_AccessHandlerCallback will be
   * empty (since it has already been processed).
   */
  MHD_POSTDATA_KIND = 4,

  /**
   * GET (URI) arguments.
   */
  MHD_GET_ARGUMENT_KIND = 8,

  /**
   * HTTP footer (only for http 1.1 chunked encodings).
   */
  MHD_FOOTER_KIND = 16
};

/**
 * The MHD_RequestTerminationCode specifies reasons
 * why a request has been terminated (or completed).
 */
enum MHD_RequestTerminationCode
{

  /**
   * We finished sending the response.
   */
  MHD_REQUEST_TERMINATED_COMPLETED_OK = 0,

  /**
   * Error handling the connection (resources
   * exhausted, other side closed connection,
   * application error accepting request, etc.)
   */
  MHD_REQUEST_TERMINATED_WITH_ERROR = 1,

  /**
   * No activity on the connection for the number
   * of seconds specified using
   * MHD_OPTION_CONNECTION_TIMEOUT.
   */
  MHD_REQUEST_TERMINATED_TIMEOUT_REACHED = 2,

  /**
   * We had to close the session since MHD was being
   * shut down.
   */
  MHD_REQUEST_TERMINATED_DAEMON_SHUTDOWN = 3

};

/**
 * List of symmetric ciphers.
 * Note that not all listed algorithms are necessarily
 * supported by all builds of MHD.
 */
enum MHD_GNUTLS_CipherAlgorithm
{
  MHD_GNUTLS_CIPHER_UNKNOWN = 0,
  MHD_GNUTLS_CIPHER_NULL = 1,
  MHD_GNUTLS_CIPHER_ARCFOUR_128,
  MHD_GNUTLS_CIPHER_3DES_CBC,
  MHD_GNUTLS_CIPHER_AES_128_CBC,
  MHD_GNUTLS_CIPHER_AES_256_CBC
};

/**
 * SSL/TLS Protocol types.
 * Note that not all listed algorithms are necessarily
 * supported by all builds of MHD.
 */
enum MHD_GNUTLS_Protocol
{
  MHD_GNUTLS_PROTOCOL_END = 0,
  MHD_GNUTLS_PROTOCOL_SSL3 = 1,
  MHD_GNUTLS_PROTOCOL_TLS1_0,
  MHD_GNUTLS_PROTOCOL_TLS1_1,
  MHD_GNUTLS_PROTOCOL_TLS1_2,
  MHD_GNUTLS_PROTOCOL_VERSION_UNKNOWN = 0xff
};

/**
 * Values of this enum are used to specify what
 * information about a connection is desired.
 */
enum MHD_ConnectionInfoType
{
  /**
   * What cipher algorithm is being used.
   * Takes no extra arguments.
   */
  MHD_CONNECTION_INFO_CIPHER_ALGO,

  /**
   *
   * Takes no extra arguments.
   */
  MHD_CONNECTION_INFO_PROTOCOL,

  /**
   * Obtain IP address of the client.
   * Takes no extra arguments.
   */
  MHD_CONNECTION_INFO_CLIENT_ADDRESS
};

/**
 * Values of this enum are used to specify what
 * information about a deamon is desired.
 */
enum MHD_DaemonInfoType
{
  /**
   * Request information about the key size for
   * a particular cipher algorithm.  The cipher
   * algorithm should be passed as an extra
   * argument (of type 'enum MHD_GNUTLS_CipherAlgorithm').
   */
  MHD_DAEMON_INFO_KEY_SIZE,

  /**
   * Request information about the key size for
   * a particular cipher algorithm.  The cipher
   * algorithm should be passed as an extra
   * argument (of type 'enum MHD_GNUTLS_HashAlgorithm').
   */
  MHD_DAEMON_INFO_MAC_KEY_SIZE,

  /**
   * Request the file descriptor for the listening socket.
   * No extra arguments should be passed.
   */
  MHD_DAEMON_INFO_LISTEN_FD
};



/**
 * Handle for the daemon (listening on a socket for HTTP traffic).
 */
struct MHD_Daemon;

/**
 * Handle for a connection / HTTP request.  With HTTP/1.1, multiple
 * requests can be run over the same connection.  However, MHD will
 * only show one request per TCP connection to the client at any given
 * time.
 */
struct MHD_Connection;

/**
 * Handle for a response.
 */
struct MHD_Response;

/**
 * Handle for POST processing.
 */
struct MHD_PostProcessor;

/**
 * Callback for serious error condition. The default action is to abort().
 * @param cls user specified value
 * @param file where the error occured
 * @param line where the error occured
 * @param reason error detail, may be NULL
 */
typedef void (*MHD_PanicCallback) (void *cls,
                                   const char *file,
                                   unsigned int line,
                                   const char *reason);

/**
 * Allow or deny a client to connect.
 *
 * @param addr address information from the client
 * @param addrlen length of the address information
 * @return MHD_YES if connection is allowed, MHD_NO if not
 */
typedef int
  (*MHD_AcceptPolicyCallback) (void *cls,
                               const struct sockaddr * addr,
                               socklen_t addrlen);

/**
 * A client has requested the given url using the given method ("GET",
 * "PUT", "DELETE", "POST", etc).  The callback must call MHS
 * callbacks to provide content to give back to the client and return
 * an HTTP status code (i.e. 200 for OK, 404, etc.).
 *
 * @param cls argument given together with the function
 *        pointer when the handler was registered with MHD
 * @param url the requested url
 * @param method the HTTP method used ("GET", "PUT", etc.)
 * @param version the HTTP version string (i.e. "HTTP/1.1")
 * @param upload_data the data being uploaded (excluding HEADERS,
 *        for a POST that fits into memory and that is encoded
 *        with a supported encoding, the POST data will NOT be
 *        given in upload_data and is instead available as
 *        part of MHD_get_connection_values; very large POST
 *        data *will* be made available incrementally in
 *        upload_data)
 * @param upload_data_size set initially to the size of the
 *        upload_data provided; the method must update this
 *        value to the number of bytes NOT processed;
 * @param con_cls pointer that the callback can set to some
 *        address and that will be preserved by MHD for future
 *        calls for this request; since the access handler may
 *        be called many times (i.e., for a PUT/POST operation
 *        with plenty of upload data) this allows the application
 *        to easily associate some request-specific state.
 *        If necessary, this state can be cleaned up in the
 *        global "MHD_RequestCompleted" callback (which
 *        can be set with the MHD_OPTION_NOTIFY_COMPLETED).
 *        Initially, <tt>*con_cls</tt> will be NULL.
 * @return MHS_YES if the connection was handled successfully,
 *         MHS_NO if the socket must be closed due to a serios
 *         error while handling the request
 */
typedef int
  (*MHD_AccessHandlerCallback) (void *cls,
                                struct MHD_Connection * connection,
                                const char *url,
                                const char *method,
                                const char *version,
                                const char *upload_data,
                                size_t *upload_data_size,
                                void **con_cls);

/**
 * Signature of the callback used by MHD to notify the
 * application about completed requests.
 *
 * @param cls client-defined closure
 * @param connection connection handle
 * @param con_cls value as set by the last call to
 *        the MHD_AccessHandlerCallback
 * @param toe reason for request termination
 * @see MHD_OPTION_NOTIFY_COMPLETED
 */
typedef void
  (*MHD_RequestCompletedCallback) (void *cls,
                                   struct MHD_Connection * connection,
                                   void **con_cls,
                                   enum MHD_RequestTerminationCode toe);

/**
 * Iterator over key-value pairs.  This iterator
 * can be used to iterate over all of the cookies,
 * headers, or POST-data fields of a request, and
 * also to iterate over the headers that have been
 * added to a response.
 *
 * @return MHD_YES to continue iterating,
 *         MHD_NO to abort the iteration
 */
typedef int
  (*MHD_KeyValueIterator) (void *cls,
                           enum MHD_ValueKind kind,
                           const char *key, const char *value);

/**
 * Callback used by libmicrohttpd in order to obtain content.  The
 * callback is to copy at most "max" bytes of content into "buf".  The
 * total number of bytes that has been placed into "buf" should be
 * returned.<p>
 *
 * Note that returning zero will cause libmicrohttpd to try again,
 * either "immediately" if in multi-threaded mode (in which case the
 * callback may want to do blocking operations) or in the next round
 * if MHD_run is used.  Returning 0 for a daemon that runs in internal
 * select mode is an error (since it would result in busy waiting) and
 * will cause the program to be aborted (abort()).
 *
 * @param cls extra argument to the callback
 * @param pos position in the datastream to access;
 *        note that if an MHD_Response object is re-used,
 *        it is possible for the same content reader to
 *        be queried multiple times for the same data;
 *        however, if an MHD_Response is not re-used,
 *        libmicrohttpd guarantees that "pos" will be
 *        the sum of all non-negative return values
 *        obtained from the content reader so far.
 * @return -1 for the end of transmission (or on error);
 *  if a content transfer size was pre-set and the callback
 *  has provided fewer than that amount of data,
 *  MHD will close the connection with the client;
 *  if no content size was specified and this is an
 *  http 1.1 connection using chunked encoding, MHD will
 *  interpret "-1" as the normal end of the transfer
 *  (possibly allowing the client to perform additional
 *  requests using the same TCP connection).
 */
typedef int
  (*MHD_ContentReaderCallback) (void *cls,
				uint64_t pos,
				char *buf,
				int max);

/**
 * This method is called by libmicrohttpd if we
 * are done with a content reader.  It should
 * be used to free resources associated with the
 * content reader.
 */
typedef void (*MHD_ContentReaderFreeCallback) (void *cls);

/**
 * Iterator over key-value pairs where the value
 * maybe made available in increments and/or may
 * not be zero-terminated.  Used for processing
 * POST data.
 *
 * @param cls user-specified closure
 * @param kind type of the value
 * @param key 0-terminated key for the value
 * @param filename name of the uploaded file, NULL if not known
 * @param content_type mime-type of the data, NULL if not known
 * @param transfer_encoding encoding of the data, NULL if not known
 * @param data pointer to size bytes of data at the
 *              specified offset
 * @param off offset of data in the overall value
 * @param size number of bytes in data available
 * @return MHD_YES to continue iterating,
 *         MHD_NO to abort the iteration
 */
typedef int
  (*MHD_PostDataIterator) (void *cls,
                           enum MHD_ValueKind kind,
                           const char *key,
                           const char *filename,
                           const char *content_type,
                           const char *transfer_encoding,
                           const char *data, uint64_t off, size_t size);

/* **************** Daemon handling functions ***************** */

/**
 * Start a webserver on the given port.
 *
 * @param flags combination of MHD_FLAG values
 * @param port port to bind to
 * @param apc callback to call to check which clients
 *        will be allowed to connect; you can pass NULL
 *        in which case connections from any IP will be
 *        accepted
 * @param apc_cls extra argument to apc
 * @param dh handler called for all requests (repeatedly)
 * @param dh_cls extra argument to dh
 * @param ... list of options (type-value pairs,
 *        terminated with MHD_OPTION_END).
 * @return NULL on error, handle to daemon on success
 */
struct MHD_Daemon *MHD_start_daemon_va (unsigned int options,
                                        unsigned short port,
                                        MHD_AcceptPolicyCallback apc,
                                        void *apc_cls,
                                        MHD_AccessHandlerCallback dh,
                                        void *dh_cls, va_list ap);

/**
 * Start a webserver on the given port.  Variadic version of
 * MHD_start_daemon_va.
 *
 * @param flags combination of MHD_FLAG values
 * @param port port to bind to
 * @param apc callback to call to check which clients
 *        will be allowed to connect; you can pass NULL
 *        in which case connections from any IP will be
 *        accepted
 * @param apc_cls extra argument to apc
 * @param dh handler called for all requests (repeatedly)
 * @param dh_cls extra argument to dh
 * @return NULL on error, handle to daemon on success
 */
struct MHD_Daemon *MHD_start_daemon (unsigned int flags,
                                     unsigned short port,
                                     MHD_AcceptPolicyCallback apc,
                                     void *apc_cls,
                                     MHD_AccessHandlerCallback dh,
                                     void *dh_cls, ...);

/**
 * Shutdown an http daemon.
 *
 * @param daemon daemon to stop
 */
void MHD_stop_daemon (struct MHD_Daemon *daemon);


/**
 * Obtain the select sets for this daemon.
 *
 * @param daemon daemon to get sets from
 * @param read_fd_set read set
 * @param write_fd_set write set
 * @param except_fd_set except set
 * @param max_fd increased to largest FD added (if larger
 *               than existing value)
 * @return MHD_YES on success, MHD_NO if this
 *         daemon was not started with the right
 *         options for this call.
 */
int
MHD_get_fdset (struct MHD_Daemon *daemon,
               fd_set * read_fd_set,
               fd_set * write_fd_set, fd_set * except_fd_set, int *max_fd);

/**
 * Obtain timeout value for select for this daemon
 * (only needed if connection timeout is used).  The
 * returned value is how long select should at most
 * block, not the timeout value set for connections.
 *
 * @param daemon daemon to query for timeout
 * @param timeout set to the timeout (in milliseconds)
 * @return MHD_YES on success, MHD_NO if timeouts are
 *        not used (or no connections exist that would
 *        necessiate the use of a timeout right now).
 */
int MHD_get_timeout (struct MHD_Daemon *daemon, unsigned long long *timeout);


/**
 * Run webserver operations (without blocking unless
 * in client callbacks).  This method should be called
 * by clients in combination with MHD_get_fdset
 * if the client-controlled select method is used.
 *
 * @param daemon daemon to run
 * @return MHD_YES on success, MHD_NO if this
 *         daemon was not started with the right
 *         options for this call.
 */
int MHD_run (struct MHD_Daemon *daemon);


/* **************** Connection handling functions ***************** */

/**
 * Get all of the headers from the request.
 *
 * @param connection connection to get values from
 * @param kind types of values to iterate over
 * @param iterator callback to call on each header;
 *        maybe NULL (then just count headers)
 * @param iterator_cls extra argument to iterator
 * @return number of entries iterated over
 */
int
MHD_get_connection_values (struct MHD_Connection *connection,
                           enum MHD_ValueKind kind,
                           MHD_KeyValueIterator iterator, void *iterator_cls);

/**
 * This function can be used to add an entry to
 * the HTTP headers of a connection (so that the
 * MHD_get_connection_values function will return
 * them -- and the MHD PostProcessor will also
 * see them).  This maybe required in certain
 * situations (see Mantis #1399) where (broken)
 * HTTP implementations fail to supply values needed
 * by the post processor (or other parts of the
 * application).
 * <p>
 * This function MUST only be called from within
 * the MHD_AccessHandlerCallback (otherwise, access
 * maybe improperly synchronized).  Furthermore,
 * the client must guarantee that the key and
 * value arguments are 0-terminated strings that
 * are NOT freed until the connection is closed.
 * (The easiest way to do this is by passing only
 * arguments to permanently allocated strings.).
 *
 * @param connection the connection for which a
 *  value should be set
 * @param kind kind of the value
 * @param key key for the value
 * @param value the value itself
 * @return MHD_NO if the operation could not be
 *         performed due to insufficient memory;
 *         MHD_YES on success
 */
int
MHD_set_connection_value (struct MHD_Connection *connection,
                          enum MHD_ValueKind kind,
                          const char *key, const char *value);

/**
 * Sets the global error handler to a different implementation.  "cb"
 * will only be called in the case of typically fatal, serious
 * internal consistency issues.  These issues should only arise in the
 * case of serious memory corruption or similar problems with the
 * architecture.  While "cb" is allowed to return and MHD will then
 * try to continue, this is never safe.
 *
 * The default implementation that is used if no panic function is set
 * simply calls "abort".  Alternative implementations might call
 * "exit" or other similar functions.
 *
 * @param cb new error handler
 * @param cls passed to error handler
 */
void MHD_set_panic_func (MHD_PanicCallback cb, void *cls);

/**
 * Get a particular header value.  If multiple
 * values match the kind, return any one of them.
 *
 * @param connection connection to get values from
 * @param kind what kind of value are we looking for
 * @param key the header to look for
 * @return NULL if no such item was found
 */
const char *MHD_lookup_connection_value (struct MHD_Connection *connection,
                                         enum MHD_ValueKind kind,
                                         const char *key);

/**
 * Queue a response to be transmitted to the client (as soon as
 * possible but after MHD_AccessHandlerCallback returns).
 *
 * @param connection the connection identifying the client
 * @param status_code HTTP status code (i.e. 200 for OK)
 * @param response response to transmit
 * @return MHD_NO on error (i.e. reply already sent),
 *         MHD_YES on success or if message has been queued
 */
int
MHD_queue_response (struct MHD_Connection *connection,
                    unsigned int status_code, struct MHD_Response *response);


/* **************** Response manipulation functions ***************** */

/**
 * Create a response object.  The response object can be extended with
 * header information and then be used any number of times.
 *
 * @param size size of the data portion of the response, MHD_SIZE_UNKNOW for unknown
 * @param block_size preferred block size for querying crc (advisory only,
 *                   MHD may still call crc using smaller chunks); this
 *                   is essentially the buffer size used for IO, clients
 *                   should pick a value that is appropriate for IO and
 *                   memory performance requirements
 * @param crc callback to use to obtain response data
 * @param crc_cls extra argument to crc
 * @param crfc callback to call to free crc_cls resources
 * @return NULL on error (i.e. invalid arguments, out of memory)
 */
struct MHD_Response *MHD_create_response_from_callback (uint64_t size,
                                                        size_t block_size,
                                                        MHD_ContentReaderCallback
                                                        crc, void *crc_cls,
                                                        MHD_ContentReaderFreeCallback
                                                        crfc);

/**
 * Create a response object.  The response object can be extended with
 * header information and then be used any number of times.
 *
 * @param size size of the data portion of the response
 * @param data the data itself
 * @param must_free libmicrohttpd should free data when done
 * @param must_copy libmicrohttpd must make a copy of data
 *        right away, the data maybe released anytime after
 *        this call returns
 * @return NULL on error (i.e. invalid arguments, out of memory)
 */
struct MHD_Response *MHD_create_response_from_data (size_t size,
                                                    void *data,
                                                    int must_free,
                                                    int must_copy);

/**
 * Destroy a response object and associated resources.  Note that
 * libmicrohttpd may keep some of the resources around if the response
 * is still in the queue for some clients, so the memory may not
 * necessarily be freed immediatley.
 *
 * @param response response to destroy
 */
void MHD_destroy_response (struct MHD_Response *response);

/**
 * Add a header line to the response.
 *
 * @param response response to add a header to
 * @param header the header to add
 * @param content value to add
 * @return MHD_NO on error (i.e. invalid header or content format),
 *         or out of memory
 */
int
MHD_add_response_header (struct MHD_Response *response,
                         const char *header, const char *content);

/**
 * Delete a header line from the response.
 *
 * @param response response to remove a header from
 * @param header the header to delete
 * @param content value to delete
 * @return MHD_NO on error (no such header known)
 */
int
MHD_del_response_header (struct MHD_Response *response,
                         const char *header, const char *content);

/**
 * Get all of the headers added to a response.
 *
 * @param response response to query
 * @param iterator callback to call on each header;
 *        maybe NULL (then just count headers)
 * @param iterator_cls extra argument to iterator
 * @return number of entries iterated over
 */
int
MHD_get_response_headers (struct MHD_Response *response,
                          MHD_KeyValueIterator iterator, void *iterator_cls);


/**
 * Get a particular header from the response.
 *
 * @param response response to query
 * @param key which header to get
 * @return NULL if header does not exist
 */
const char *MHD_get_response_header (struct MHD_Response *response,
                                     const char *key);


/* ********************** PostProcessor functions ********************** */

/**
 * Create a PostProcessor.
 *
 * A PostProcessor can be used to (incrementally) parse the data
 * portion of a POST request.  Note that some buggy browsers fail to
 * set the encoding type.  If you want to support those, you may have
 * to call 'MHD_set_connection_value' with the proper encoding type
 * before creating a post processor (if no supported encoding type is
 * set, this function will fail).
 *
 * @param connection the connection on which the POST is
 *        happening (used to determine the POST format)
 * @param buffer_size maximum number of bytes to use for
 *        internal buffering (used only for the parsing,
 *        specifically the parsing of the keys).  A
 *        tiny value (256-1024) should be sufficient.
 *        Do NOT use a value smaller than 256.
 * @param iter iterator to be called with the parsed data,
 *        Must NOT be NULL.
 * @param cls first argument to ikvi
 * @return  NULL on error (out of memory, unsupported encoding),
            otherwise a PP handle
 */
struct MHD_PostProcessor *MHD_create_post_processor (struct MHD_Connection
                                                     *connection,
                                                     size_t buffer_size,
                                                     MHD_PostDataIterator
                                                     iter, void *cls);

/**
 * Parse and process POST data.
 * Call this function when POST data is available
 * (usually during an MHD_AccessHandlerCallback)
 * with the upload_data and upload_data_size.
 * Whenever possible, this will then cause calls
 * to the MHD_IncrementalKeyValueIterator.
 *
 * @param pp the post processor
 * @param post_data post_data_len bytes of POST data
 * @param post_data_len length of post_data
 * @return MHD_YES on success, MHD_NO on error
 *         (out-of-memory, iterator aborted, parse error)
 */
int
MHD_post_process (struct MHD_PostProcessor *pp,
                  const char *post_data, size_t post_data_len);

/**
 * Release PostProcessor resources.
 *
 * @param pp the PostProcessor to destroy
 * @return MHD_YES if processing completed nicely,
 *         MHD_NO if there were spurious characters / formatting
 *                problems; it is common to ignore the return
 *                value of this function
 */
int MHD_destroy_post_processor (struct MHD_PostProcessor *pp);



/* ********************** generic query functions ********************** */


/**
 * Information about a connection.
 */
union MHD_ConnectionInfo
{
  enum MHD_GNUTLS_CipherAlgorithm cipher_algorithm;
  enum MHD_GNUTLS_Protocol protocol;
  /**
   * Address information for the client.
   */
  struct sockaddr_in * client_addr;
};

/**
 * Obtain information about the given connection.
 *
 * @param connection what connection to get information about
 * @param infoType what information is desired?
 * @param ... depends on infoType
 * @return NULL if this information is not available
 *         (or if the infoType is unknown)
 */
const union MHD_ConnectionInfo *MHD_get_connection_info (struct MHD_Connection
                                                         *connection,
                                                         enum
                                                         MHD_ConnectionInfoType
                                                         infoType, ...);


/**
 * Information about an MHD daemon.
 */
union MHD_DaemonInfo
{
  /**
   * Size of the key (unit??)
   */
  size_t key_size;

  /**
   * Size of the mac key (unit??)
   */
  size_t mac_key_size;

  /**
   * Listen socket file descriptor
   */
  int listen_fd;
};

/**
 * Obtain information about the given daemon
 * (not fully implemented!).
 *
 * @param daemon what daemon to get information about
 * @param infoType what information is desired?
 * @param ... depends on infoType
 * @return NULL if this information is not available
 *         (or if the infoType is unknown)
 */
const union MHD_DaemonInfo *MHD_get_daemon_info (struct MHD_Daemon *daemon,
                                                 enum MHD_DaemonInfoType
                                                 infoType, ...);

/**
 * Obtain the version of this library
 *
 * @return static version string, e.g. "0.4.1"
 */
const char* MHD_get_version(void);

#if 0                           /* keep Emacsens' auto-indent happy */
{
#endif
#ifdef __cplusplus
}
#endif

#endif
