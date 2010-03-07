#include <platform.h>
#include <microhttpd.h>
#include <time.h>

#define PORT 8888
#define FILENAME "picture.png"
#define MIMETYPE "image/png"


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


int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  unsigned char *buffer = NULL;
  struct MHD_Response *response;
  long size;
  FILE *fp;
  int ret = 0;

  if (0 != strcmp (method, "GET"))
    return MHD_NO;

  size = get_file_size (FILENAME);
  if (size != 0)
    {
      fp = fopen (FILENAME, "rb");
      if (fp)
        {
          buffer = malloc (size);

          if (buffer)
            if (size == fread (buffer, 1, size, fp))
              ret = 1;

          fclose (fp);
        }
    }

  if (!ret)
    {
      const char *errorstr =
        "<html><body>An internal server error has occured!\
                              </body></html>";

      if (buffer)
        free (buffer);

      response =
        MHD_create_response_from_data (strlen (errorstr), (void *) errorstr,
                                       MHD_NO, MHD_NO);

      if (response)
        {
          ret =
            MHD_queue_response (connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                                response);
          MHD_destroy_response (response);

          return MHD_YES;
        }
      else
        return MHD_NO;
    }

  response =
    MHD_create_response_from_data (size, (void *) buffer, MHD_YES, MHD_NO);

  MHD_add_response_header (response, "Content-Type", MIMETYPE);

  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);

  return ret;
}


int
main ()
{
  struct MHD_Daemon *daemon;

  daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
                             &answer_to_connection, NULL, MHD_OPTION_END);
  if (NULL == daemon)
    return 1;

  getchar ();

  MHD_stop_daemon (daemon);

  return 0;
}
