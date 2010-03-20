/*
 This file is part of libmicrohttpd
 (C) 2007 Christian Grothoff

 libmicrohttpd is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by the Free Software Foundation; either version 2, or (at your
 option) any later version.

 libmicrohttpd is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with libmicrohttpd; see the file COPYING.  If not, write to the
 Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
 */

/**
 * @file tls_daemon_options_test.c
 * @brief  Common tls test functions
 * @author Sagie Amir
 */
#include "tls_test_common.h"
#include "tls_test_keys.h"
#include "gnutls.h"
#include "gnutls_datum.h"

const char test_file_data[] = "Hello World\n";

int curl_check_version (const char *req_version, ...);

void
print_test_result (int test_outcome, char *test_name)
{
#if 0
  if (test_outcome != 0)
    fprintf (stderr, "running test: %s [fail]\n", test_name);
  else
    fprintf (stdout, "running test: %s [pass]\n", test_name);
#endif
}

size_t
copyBuffer (void *ptr, size_t size, size_t nmemb, void *ctx)
{
  struct CBC *cbc = ctx;

  if (cbc->pos + size * nmemb > cbc->size)
    return 0;                   /* overflow */
  memcpy (&cbc->buf[cbc->pos], ptr, size * nmemb);
  cbc->pos += size * nmemb;
  return size * nmemb;
}

static int
file_reader (void *cls, uint64_t pos, char *buf, int max)
{
  FILE *file = cls;
  fseek (file, pos, SEEK_SET);
  return fread (buf, 1, max, file);
}

/**
 *  HTTP access handler call back
 */
int
http_ahc (void *cls, struct MHD_Connection *connection,
          const char *url, const char *method, const char *upload_data,
          const char *version, size_t *upload_data_size, void **ptr)
{
  static int aptr;
  struct MHD_Response *response;
  int ret;
  FILE *file;
  struct stat buf;

  if (0 != strcmp (method, MHD_HTTP_METHOD_GET))
    return MHD_NO;              /* unexpected method */
  if (&aptr != *ptr)
    {
      /* do never respond on first call */
      *ptr = &aptr;
      return MHD_YES;
    }
  *ptr = NULL;                  /* reset when done */

  file = fopen (url, "rb");
  if (file == NULL)
    {
      response = MHD_create_response_from_data (strlen (PAGE_NOT_FOUND),
                                                (void *) PAGE_NOT_FOUND,
                                                MHD_NO, MHD_NO);
      ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
      MHD_destroy_response (response);
    }
  else
    {
      stat (url, &buf);
      response = MHD_create_response_from_callback (buf.st_size, 32 * 1024,     /* 32k PAGE_NOT_FOUND size */
                                                    &file_reader, file,
                                                    (MHD_ContentReaderFreeCallback)
                                                    & fclose);
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
      MHD_destroy_response (response);
    }
  return ret;
}

/* HTTP access handler call back */
int
http_dummy_ahc (void *cls, struct MHD_Connection *connection,
                const char *url, const char *method, const char *upload_data,
                const char *version, size_t *upload_data_size,
                void **ptr)
{
  return 0;
}

/**
 * send a test http request to the daemon
 * @param url
 * @param cbc - may be null
 * @param cipher_suite
 * @param proto_version
 * @return
 */
/* TODO have test wrap consider a NULL cbc */
int
send_curl_req (char *url, struct CBC * cbc, char *cipher_suite,
               int proto_version)
{
  CURL *c;
  CURLcode errornum;
  c = curl_easy_init ();
#if DEBUG_HTTPS_TEST
  curl_easy_setopt (c, CURLOPT_VERBOSE, CURL_VERBOS_LEVEL);
#endif
  curl_easy_setopt (c, CURLOPT_URL, url);
  curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
  curl_easy_setopt (c, CURLOPT_TIMEOUT, 60L);
  curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 60L);

  if (cbc != NULL)
    {
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_FILE, cbc);
    }

  /* TLS options */
  curl_easy_setopt (c, CURLOPT_SSLVERSION, proto_version);
  curl_easy_setopt (c, CURLOPT_SSL_CIPHER_LIST, cipher_suite);

  /* currently skip any peer authentication */
  curl_easy_setopt (c, CURLOPT_SSL_VERIFYPEER, 0);
  curl_easy_setopt (c, CURLOPT_SSL_VERIFYHOST, 0);

  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1);

  /* NOTE: use of CONNECTTIMEOUT without also
     setting NOSIGNAL results in really weird
     crashes on my system! */
  curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1);
  if (CURLE_OK != (errornum = curl_easy_perform (c)))
    {
      fprintf (stderr, "curl_easy_perform failed: `%s'\n",
               curl_easy_strerror (errornum));
      curl_easy_cleanup (c);
      return errornum;
    }
  curl_easy_cleanup (c);

  return CURLE_OK;
}

/**
 * compile test file url pointing to the current running directory path
 * @param url - char buffer into which the url is compiled
 * @return
 */
int
gen_test_file_url (char *url, int port)
{
  int ret = 0;
  char *doc_path;
  size_t doc_path_len;
  /* setup test file path, url */
  doc_path_len = PATH_MAX > 4096 ? 4096 : PATH_MAX;
  if (NULL == (doc_path = malloc (doc_path_len)))
    {
      fprintf (stderr, MHD_E_MEM);
      return -1;
    }
  if (getcwd (doc_path, doc_path_len) == NULL)
    {
      fprintf (stderr, "Error: failed to get working directory. %s\n",
               strerror (errno));
      ret = -1;
    }
  /* construct url - this might use doc_path */
  if (sprintf (url, "%s:%d%s/%s", "https://localhost", port,
               doc_path, TEST_FILE_NAME) < 0)
    ret = -1;

  free (doc_path);
  return ret;
}

/**
 * test HTTPS file transfer
 * @param test_fd: file to attempt transferring
 */
int
test_https_transfer (FILE * test_fd, char *cipher_suite, int proto_version)
{
  int len, ret = 0;
  struct CBC cbc;
  char url[255];
  struct stat statb;
  /* used to memcmp local copy & deamon supplied copy */
  unsigned char *mem_test_file_local;

  if (0 != stat (TEST_FILE_NAME, &statb))
    {
      fprintf (stderr, "Failed to stat `%s': %s\n",
	       TEST_FILE_NAME, strerror(errno));
      return -1;
    }
  len = statb.st_size;
  cbc.buf = NULL;
  if (NULL == (mem_test_file_local = malloc (len)))
    {
      fprintf (stderr, MHD_E_MEM);
      ret = -1;
      goto cleanup;
    }

  fseek (test_fd, 0, SEEK_SET);
  if (fread (mem_test_file_local, sizeof (char), len, test_fd) != len)
    {
      fprintf (stderr, "Error: failed to read test file. %s\n",
               strerror (errno));
      ret = -1;
      goto cleanup;
    }

  if (NULL == (cbc.buf = malloc (sizeof (char) * len)))
    {
      fprintf (stderr, MHD_E_MEM);
      ret = -1;
      goto cleanup;
    }
  cbc.size = len;
  cbc.pos = 0;

  if (gen_test_file_url (url, DEAMON_TEST_PORT))
    {
      ret = -1;
      goto cleanup;
    }

  if (CURLE_OK != send_curl_req (url, &cbc, cipher_suite, proto_version))
    {
      ret = -1;
      goto cleanup;
    }

  /* compare test file & daemon responce */
  if (memcmp (cbc.buf, mem_test_file_local, len) != 0)
    {
      fprintf (stderr, "Error: local file & received file differ.\n");
      ret = -1;
    }

cleanup:
  free (mem_test_file_local);
  if (cbc.buf != NULL)
    free (cbc.buf);
  return ret;
}

/**
 * setup a mock test file which is requested from the running daemon
 * @return open file descriptor to the test file
 */
FILE *
setup_test_file ()
{
  FILE *test_fd;

  if (NULL == (test_fd = fopen (TEST_FILE_NAME, "wb+")))
    {
      fprintf (stderr, "Error: failed to open `%s': %s\n",
               TEST_FILE_NAME, strerror (errno));
      return NULL;
    }
  if (fwrite (test_file_data, sizeof (char), strlen (test_file_data), test_fd)
      != strlen (test_file_data))
    {
      fprintf (stderr, "Error: failed to write `%s. %s'\n",
               TEST_FILE_NAME, strerror (errno));
      fclose (test_fd);
      return NULL;
    }
  if (fflush (test_fd))
    {
      fprintf (stderr, "Error: failed to flush test file stream. %s\n",
               strerror (errno));
      fclose (test_fd);
      return NULL;
    }
  return test_fd;
}

/**
 * setup test case
 *
 * @param d
 * @param daemon_flags
 * @param arg_list
 * @return
 */
int
setup_testcase (struct MHD_Daemon **d, int daemon_flags, va_list arg_list)
{
  *d = MHD_start_daemon_va (daemon_flags, DEAMON_TEST_PORT,
                            NULL, NULL, &http_ahc, NULL, arg_list);

  if (*d == NULL)
    {
      fprintf (stderr, MHD_E_SERVER_INIT);
      return -1;
    }

  return 0;
}

void
teardown_testcase (struct MHD_Daemon *d)
{
  MHD_stop_daemon (d);
}

int
setup_session (MHD_gtls_session_t * session,
               MHD_gnutls_datum_t * key,
               MHD_gnutls_datum_t * cert, MHD_gtls_cert_credentials_t * xcred)
{
  int ret;
  const char *err_pos;

  MHD__gnutls_certificate_allocate_credentials (xcred);

  MHD_gtls_set_datum_m (key, srv_key_pem, strlen (srv_key_pem), &malloc);
  MHD_gtls_set_datum_m (cert, srv_self_signed_cert_pem,
                        strlen (srv_self_signed_cert_pem), &malloc);

  MHD__gnutls_certificate_set_x509_key_mem (*xcred, cert, key,
                                            GNUTLS_X509_FMT_PEM);

  MHD__gnutls_init (session, GNUTLS_CLIENT);
  ret = MHD__gnutls_priority_set_direct (*session, "NORMAL", &err_pos);
  if (ret < 0)
    {
      return -1;
    }

  MHD__gnutls_credentials_set (*session, MHD_GNUTLS_CRD_CERTIFICATE, xcred);
  return 0;
}

int
teardown_session (MHD_gtls_session_t session,
                  MHD_gnutls_datum_t * key,
                  MHD_gnutls_datum_t * cert,
                  MHD_gtls_cert_credentials_t xcred)
{

  MHD_gtls_free_datum_m (key, free);
  MHD_gtls_free_datum_m (cert, free);

  MHD__gnutls_deinit (session);

  MHD__gnutls_certificate_free_credentials (xcred);
  return 0;
}

/* TODO test_wrap: change sig to (setup_func, test, va_list test_arg) */
int
test_wrap (char *test_name, int
           (*test_function) (FILE * test_fd, char *cipher_suite,
                             int proto_version), FILE * test_fd,
           int daemon_flags, char *cipher_suite, int proto_version, ...)
{
  int ret;
  va_list arg_list;
  struct MHD_Daemon *d;

  va_start (arg_list, proto_version);
  if (setup_testcase (&d, daemon_flags, arg_list) != 0)
    {
      va_end (arg_list);
      return -1;
    }
#if 0
  fprintf (stdout, "running test: %s ", test_name);
#endif
  ret = test_function (test_fd, cipher_suite, proto_version);
#if 0
  if (ret == 0)
    {
      fprintf (stdout, "[pass]\n");
    }
  else
    {
      fprintf (stdout, "[fail]\n");
    }
#endif
  teardown_testcase (d);
  va_end (arg_list);
  return ret;
}
