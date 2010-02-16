#include <platform.h>
#include <microhttpd.h>

#define PORT 8888

#define REALM     "\"Maintenance\""
#define USER      "a legitimate user"
#define PASSWORD  "and his password"

#define SERVERKEYFILE "server.key"
#define SERVERCERTFILE "server.pem"


char *
string_to_base64 (const char *message)
{
  const char *lookup =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  unsigned long l;
  int i;
  char *tmp;
  size_t length = strlen (message);

  tmp = malloc (length * 2);
  if (NULL == tmp)
    return tmp;

  tmp[0] = 0;

  for (i = 0; i < length; i += 3)
    {
      l = (((unsigned long) message[i]) << 16)
        | (((i + 1) < length) ? (((unsigned long) message[i + 1]) << 8) : 0)
        | (((i + 2) < length) ? ((unsigned long) message[i + 2]) : 0);


      strncat (tmp, &lookup[(l >> 18) & 0x3F], 1);
      strncat (tmp, &lookup[(l >> 12) & 0x3F], 1);

      if (i + 1 < length)
        strncat (tmp, &lookup[(l >> 6) & 0x3F], 1);
      if (i + 2 < length)
        strncat (tmp, &lookup[l & 0x3F], 1);
    }

  if (length % 3)
    strncat (tmp, "===", 3 - length % 3);

  return tmp;
}


long
get_file_size (const char *filename)
{
  FILE *fp;

  fp = fopen (filename, "rb");
  if (fp)
    {
      long size;

      if ((0 != fseek (fp, 0, SEEK_END)) || (-1 == (size = ftell (fp))))
        size = 0;

      fclose (fp);

      return size;
    }
  else
    return 0;
}

char *
load_file (const char *filename)
{
  FILE *fp;
  char *buffer;
  long size;

  size = get_file_size (filename);
  if (size == 0)
    return NULL;

  fp = fopen (filename, "rb");
  if (!fp)
    return NULL;

  buffer = malloc (size);
  if (!buffer)
    {
      fclose (fp);
      return NULL;
    }

  if (size != fread (buffer, 1, size, fp))
    {
      free (buffer);
      buffer = NULL;
    }

  fclose (fp);
  return buffer;
}

int
ask_for_authentication (struct MHD_Connection *connection, const char *realm)
{
  int ret;
  struct MHD_Response *response;
  char *headervalue;
  const char *strbase = "Basic realm=";

  response = MHD_create_response_from_data (0, NULL, MHD_NO, MHD_NO);
  if (!response)
    return MHD_NO;

  headervalue = malloc (strlen (strbase) + strlen (realm) + 1);
  if (!headervalue)
    return MHD_NO;

  strcpy (headervalue, strbase);
  strcat (headervalue, realm);

  ret = MHD_add_response_header (response, "WWW-Authenticate", headervalue);
  free (headervalue);
  if (!ret)
    {
      MHD_destroy_response (response);
      return MHD_NO;
    }

  ret = MHD_queue_response (connection, MHD_HTTP_UNAUTHORIZED, response);

  MHD_destroy_response (response);

  return ret;
}

int
is_authenticated (struct MHD_Connection *connection,
                  const char *username, const char *password)
{
  const char *headervalue;
  char *expected_b64, *expected;
  const char *strbase = "Basic ";
  int authenticated;

  headervalue =
    MHD_lookup_connection_value (connection, MHD_HEADER_KIND,
                                 "Authorization");
  if (NULL == headervalue)
    return 0;
  if (0 != strncmp (headervalue, strbase, strlen (strbase)))
    return 0;

  expected = malloc (strlen (username) + 1 + strlen (password) + 1);
  if (NULL == expected)
    return 0;

  strcpy (expected, username);
  strcat (expected, ":");
  strcat (expected, password);

  expected_b64 = string_to_base64 (expected);
  if (NULL == expected_b64)
    return 0;

  strcpy (expected, strbase);
  authenticated =
    (strcmp (headervalue + strlen (strbase), expected_b64) == 0);

  free (expected_b64);

  return authenticated;
}


int
secret_page (struct MHD_Connection *connection)
{
  int ret;
  struct MHD_Response *response;
  const char *page = "<html><body>A secret.</body></html>";

  response =
    MHD_create_response_from_data (strlen (page), (void *) page, MHD_NO,
                                   MHD_NO);
  if (!response)
    return MHD_NO;

  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);

  return ret;
}


int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  if (0 != strcmp (method, "GET"))
    return MHD_NO;
  if (NULL == *con_cls)
    {
      *con_cls = connection;
      return MHD_YES;
    }

  if (!is_authenticated (connection, USER, PASSWORD))
    return ask_for_authentication (connection, REALM);

  return secret_page (connection);
}


int
main ()
{
  struct MHD_Daemon *daemon;
  char *key_pem;
  char *cert_pem;

  key_pem = load_file (SERVERKEYFILE);
  cert_pem = load_file (SERVERCERTFILE);

  if ((key_pem == NULL) || (cert_pem == NULL))
    {
      printf ("The key/certificate files could not be read.\n");
      return 1;
    }

  daemon =
    MHD_start_daemon (MHD_USE_SELECT_INTERNALLY | MHD_USE_SSL, PORT, NULL,
                      NULL, &answer_to_connection, NULL,
                      MHD_OPTION_HTTPS_MEM_KEY, key_pem,
                      MHD_OPTION_HTTPS_MEM_CERT, cert_pem, MHD_OPTION_END);
  if (NULL == daemon)
    {
      printf ("%s\n", cert_pem);

      free (key_pem);
      free (cert_pem);

      return 1;
    }

  getchar ();

  MHD_stop_daemon (daemon);
  free (key_pem);
  free (cert_pem);

  return 0;
}
