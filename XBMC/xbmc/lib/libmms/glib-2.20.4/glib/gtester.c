/* GLib testing framework runner
 * Copyright (C) 2007 Sven Herzberg
 * Copyright (C) 2007 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <glib.h>
#include <gstdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

/* the read buffer size in bytes */
#define READ_BUFFER_SIZE 4096

/* --- prototypes --- */
static int      main_selftest   (int    argc,
                                 char **argv);
static void     parse_args      (gint           *argc_p,
                                 gchar        ***argv_p);

/* --- variables --- */
static GIOChannel  *ioc_report = NULL;
static gboolean     gtester_quiet = FALSE;
static gboolean     gtester_verbose = FALSE;
static gboolean     gtester_list_tests = FALSE;
static gboolean     gtester_selftest = FALSE;
static gboolean     subtest_running = FALSE;
static gint         subtest_exitstatus = 0;
static gboolean     subtest_io_pending = FALSE;
static gboolean     subtest_quiet = TRUE;
static gboolean     subtest_verbose = FALSE;
static gboolean     subtest_mode_fatal = TRUE;
static gboolean     subtest_mode_perf = FALSE;
static gboolean     subtest_mode_quick = TRUE;
static const gchar *subtest_seedstr = NULL;
static gchar       *subtest_last_seed = NULL;
static GSList      *subtest_paths = NULL;
static GSList      *subtest_args = NULL;
static gboolean     testcase_open = FALSE;
static guint        testcase_count = 0;
static guint        testcase_fail_count = 0;
static const gchar *output_filename = NULL;
static guint        log_indent = 0;
static gint         log_fd = -1;

/* --- functions --- */
static const char*
sindent (guint n)
{
  static const char spaces[] = "                                                                                                    ";
  int l = sizeof (spaces) - 1;
  n = MIN (n, l);
  return spaces + l - n;
}

static void G_GNUC_PRINTF (1, 2)
test_log_printfe (const char *format,
                  ...)
{
  char *result;
  int r;
  va_list args;
  va_start (args, format);
  result = g_markup_vprintf_escaped (format, args);
  va_end (args);
  do
    r = write (log_fd, result, strlen (result));
  while (r < 0 && errno == EINTR);
  g_free (result);
}

static void
terminate (void)
{
  kill (getpid(), SIGTERM);
  abort();
}

static void
testcase_close (long double duration,
                gint        exit_status,
                guint       n_forks)
{
  g_return_if_fail (testcase_open > 0);
  test_log_printfe ("%s<duration>%.6Lf</duration>\n", sindent (log_indent), duration);
  test_log_printfe ("%s<status exit-status=\"%d\" n-forks=\"%d\" result=\"%s\"/>\n",
                    sindent (log_indent), exit_status, n_forks,
                    exit_status ? "failed" : "success");
  log_indent -= 2;
  test_log_printfe ("%s</testcase>\n", sindent (log_indent));
  testcase_open--;
  if (gtester_verbose)
    g_print ("%s\n", exit_status ? "FAIL" : "OK");
  if (exit_status && subtest_last_seed)
    g_print ("GTester: last random seed: %s\n", subtest_last_seed);
  if (exit_status)
    testcase_fail_count += 1;
  if (subtest_mode_fatal && testcase_fail_count)
    terminate();
}

static void
test_log_msg (GTestLogMsg *msg)
{
  switch (msg->log_type)
    {
      guint i;
      gchar **strv;
    case G_TEST_LOG_NONE:
      break;
    case G_TEST_LOG_ERROR:
      strv = g_strsplit (msg->strings[0], "\n", -1);
      for (i = 0; strv[i]; i++)
        test_log_printfe ("%s<error>%s</error>\n", sindent (log_indent), strv[i]);
      g_strfreev (strv);
      break;
    case G_TEST_LOG_START_BINARY:
      test_log_printfe ("%s<binary file=\"%s\"/>\n", sindent (log_indent), msg->strings[0]);
      subtest_last_seed = g_strdup (msg->strings[1]);
      test_log_printfe ("%s<random-seed>%s</random-seed>\n", sindent (log_indent), subtest_last_seed);
      break;
    case G_TEST_LOG_LIST_CASE:
      g_print ("%s\n", msg->strings[0]);
      break;
    case G_TEST_LOG_START_CASE:
      testcase_count++;
      if (gtester_verbose)
        {
          gchar *sc = g_strconcat (msg->strings[0], ":", NULL);
          gchar *sleft = g_strdup_printf ("%-68s", sc);
          g_free (sc);
          g_print ("%70s ", sleft);
          g_free (sleft);
        }
      g_return_if_fail (testcase_open == 0);
      testcase_open++;
      test_log_printfe ("%s<testcase path=\"%s\">\n", sindent (log_indent), msg->strings[0]);
      log_indent += 2;
      break;
    case G_TEST_LOG_SKIP_CASE:
      if (FALSE && gtester_verbose) /* enable to debug test case skipping logic */
        {
          gchar *sc = g_strconcat (msg->strings[0], ":", NULL);
          gchar *sleft = g_strdup_printf ("%-68s", sc);
          g_free (sc);
          g_print ("%70s SKIPPED\n", sleft);
          g_free (sleft);
        }
      test_log_printfe ("%s<testcase path=\"%s\" skipped=\"1\"/>\n", sindent (log_indent), msg->strings[0]);
      break;
    case G_TEST_LOG_STOP_CASE:
      testcase_close (msg->nums[2], (int) msg->nums[0], (int) msg->nums[1]);
      break;
    case G_TEST_LOG_MIN_RESULT:
    case G_TEST_LOG_MAX_RESULT:
      test_log_printfe ("%s<performance minimize=\"%d\" maximize=\"%d\" value=\"%.16Lg\">\n",
                        sindent (log_indent), msg->log_type == G_TEST_LOG_MIN_RESULT, msg->log_type == G_TEST_LOG_MAX_RESULT, msg->nums[0]);
      test_log_printfe ("%s%s\n", sindent (log_indent + 2), msg->strings[0]);
      test_log_printfe ("%s</performance>\n", sindent (log_indent));
      break;
    case G_TEST_LOG_MESSAGE:
      test_log_printfe ("%s<message>\n%s\n%s</message>\n", sindent (log_indent), msg->strings[0], sindent (log_indent));
      break;
    }
}

static gboolean
child_report_cb (GIOChannel  *source,
                 GIOCondition condition,
                 gpointer     data)
{
  GTestLogBuffer *tlb = data;
  GIOStatus status = G_IO_STATUS_NORMAL;
  gboolean first_read_eof = FALSE, first_read = TRUE;
  gsize length = 0;
  do
    {
      guint8 buffer[READ_BUFFER_SIZE];
      GError *error = NULL;
      status = g_io_channel_read_chars (source, (gchar*) buffer, sizeof (buffer), &length, &error);
      if (first_read && (condition & G_IO_IN))
        {
          /* on some unixes (MacOS) we need to detect non-blocking fd EOF
           * by an IO_IN select/poll followed by read()==0.
           */
          first_read_eof = length == 0;
        }
      first_read = FALSE;
      if (length)
        {
          GTestLogMsg *msg;
          g_test_log_buffer_push (tlb, length, buffer);
          do
            {
              msg = g_test_log_buffer_pop (tlb);
              if (msg)
                {
                  test_log_msg (msg);
                  g_test_log_msg_free (msg);
                }
            }
          while (msg);
        }
      g_clear_error (&error);
      /* ignore the io channel status, which will report intermediate EOFs for non blocking fds */
      (void) status;
    }
  while (length > 0);
  /* g_print ("LASTIOSTATE: first_read_eof=%d condition=%d\n", first_read_eof, condition); */
  if (first_read_eof || (condition & (G_IO_ERR | G_IO_HUP)))
    {
      /* if there's no data to read and select() reports an error or hangup,
       * the fd must have been closed remotely
       */
      subtest_io_pending = FALSE;
      return FALSE;
    }
  return TRUE; /* keep polling */
}

static void
child_watch_cb (GPid     pid,
		gint     status,
		gpointer data)
{
  g_spawn_close_pid (pid);
  if (WIFEXITED (status)) /* normal exit */
    subtest_exitstatus = WEXITSTATUS (status);
  else /* signal or core dump, etc */
    subtest_exitstatus = 0xffffffff;
  subtest_running = FALSE;
}

static gchar*
queue_gfree (GSList **slistp,
             gchar   *string)
{
  *slistp = g_slist_prepend (*slistp, string);
  return string;
}

static void
unset_cloexec_fdp (gpointer fdp_data)
{
  int r, *fdp = fdp_data;
  do
    r = fcntl (*fdp, F_SETFD, 0 /* FD_CLOEXEC */);
  while (r < 0 && errno == EINTR);
}

static gboolean
launch_test_binary (const char *binary,
                    guint       skip_tests)
{
  GTestLogBuffer *tlb;
  GSList *slist, *free_list = NULL;
  GError *error = NULL;
  int argc = 0;
  const gchar **argv;
  GPid pid = 0;
  gint report_pipe[2] = { -1, -1 };
  guint child_report_cb_id = 0;
  gboolean loop_pending;
  gint i = 0;

  if (pipe (report_pipe) < 0)
    {
      if (subtest_mode_fatal)
        g_error ("Failed to open pipe for test binary: %s: %s", binary, g_strerror (errno));
      else
        g_warning ("Failed to open pipe for test binary: %s: %s", binary, g_strerror (errno));
      return FALSE;
    }

  /* setup argc */
  for (slist = subtest_args; slist; slist = slist->next)
    argc++;
  /* argc++; */
  if (subtest_quiet)
    argc++;
  if (subtest_verbose)
    argc++;
  if (!subtest_mode_fatal)
    argc++;
  if (subtest_mode_quick)
    argc++;
  else
    argc++;
  if (subtest_mode_perf)
    argc++;
  if (gtester_list_tests)
    argc++;
  if (subtest_seedstr)
    argc++;
  argc++;
  if (skip_tests)
    argc++;
  for (slist = subtest_paths; slist; slist = slist->next)
    argc++;

  /* setup argv */
  argv = g_malloc ((argc + 2) * sizeof(gchar *));
  argv[i++] = binary;
  for (slist = subtest_args; slist; slist = slist->next)
    argv[i++] = (gchar*) slist->data;
  /* argv[i++] = "--debug-log"; */
  if (subtest_quiet)
    argv[i++] = "--quiet";
  if (subtest_verbose)
    argv[i++] = "--verbose";
  if (!subtest_mode_fatal)
    argv[i++] = "--keep-going";
  if (subtest_mode_quick)
    argv[i++] = "-m=quick";
  else
    argv[i++] = "-m=slow";
  if (subtest_mode_perf)
    argv[i++] = "-m=perf";
  if (gtester_list_tests)
    argv[i++] = "-l";
  if (subtest_seedstr)
    argv[i++] = queue_gfree (&free_list, g_strdup_printf ("--seed=%s", subtest_seedstr));
  argv[i++] = queue_gfree (&free_list, g_strdup_printf ("--GTestLogFD=%u", report_pipe[1]));
  if (skip_tests)
    argv[i++] = queue_gfree (&free_list, g_strdup_printf ("--GTestSkipCount=%u", skip_tests));
  for (slist = subtest_paths; slist; slist = slist->next)
    argv[i++] = queue_gfree (&free_list, g_strdup_printf ("-p=%s", (gchar*) slist->data));
  argv[i++] = NULL;

  g_spawn_async_with_pipes (NULL, /* g_get_current_dir() */
                            (gchar**) argv,
                            NULL, /* envp */
                            G_SPAWN_DO_NOT_REAP_CHILD, /* G_SPAWN_SEARCH_PATH */
                            unset_cloexec_fdp, &report_pipe[1], /* pre-exec callback */
                            &pid,
                            NULL,       /* standard_input */
                            NULL,       /* standard_output */
                            NULL,       /* standard_error */
                            &error);
  g_slist_foreach (free_list, (void(*)(void*,void*)) g_free, NULL);
  g_slist_free (free_list);
  free_list = NULL;
  close (report_pipe[1]);

  if (!gtester_quiet)
    g_print ("(pid=%lu)\n", (unsigned long) pid);

  if (error)
    {
      close (report_pipe[0]);
      if (subtest_mode_fatal)
        g_error ("Failed to execute test binary: %s: %s", argv[0], error->message);
      else
        g_warning ("Failed to execute test binary: %s: %s", argv[0], error->message);
      g_clear_error (&error);
      g_free (argv);
      return FALSE;
    }
  g_free (argv);

  subtest_running = TRUE;
  subtest_io_pending = TRUE;
  tlb = g_test_log_buffer_new();
  if (report_pipe[0] >= 0)
    {
      ioc_report = g_io_channel_unix_new (report_pipe[0]);
      g_io_channel_set_flags (ioc_report, G_IO_FLAG_NONBLOCK, NULL);
      g_io_channel_set_encoding (ioc_report, NULL, NULL);
      g_io_channel_set_buffered (ioc_report, FALSE);
      child_report_cb_id = g_io_add_watch_full (ioc_report, G_PRIORITY_DEFAULT - 1, G_IO_IN | G_IO_ERR | G_IO_HUP, child_report_cb, tlb, NULL);
      g_io_channel_unref (ioc_report);
    }
  g_child_watch_add_full (G_PRIORITY_DEFAULT + 1, pid, child_watch_cb, NULL, NULL);

  loop_pending = g_main_context_pending (NULL);
  while (subtest_running ||     /* FALSE once child exits */
         subtest_io_pending ||  /* FALSE once ioc_report closes */
         loop_pending)          /* TRUE while idler, etc are running */
    {
      /* g_print ("LOOPSTATE: subtest_running=%d subtest_io_pending=%d\n", subtest_running, subtest_io_pending); */
      /* check for unexpected hangs that are not signalled on report_pipe */
      if (!subtest_running &&   /* child exited */
          subtest_io_pending && /* no EOF detected on report_pipe */
          !loop_pending)        /* no IO events pending however */
        break;
      g_main_context_iteration (NULL, TRUE);
      loop_pending = g_main_context_pending (NULL);
    }

  g_source_remove (child_report_cb_id);
  close (report_pipe[0]);
  g_test_log_buffer_free (tlb);

  return TRUE;
}

static void
launch_test (const char *binary)
{
  gboolean success = TRUE;
  GTimer *btimer = g_timer_new();
  gboolean need_restart;
  testcase_count = 0;
  testcase_fail_count = 0;
  if (!gtester_quiet)
    g_print ("TEST: %s... ", binary);

 retry:
  test_log_printfe ("%s<testbinary path=\"%s\">\n", sindent (log_indent), binary);
  log_indent += 2;
  g_timer_start (btimer);
  subtest_exitstatus = 0;
  success &= launch_test_binary (binary, testcase_count);
  success &= subtest_exitstatus == 0;
  need_restart = testcase_open != 0;
  if (testcase_open)
    testcase_close (0, -256, 0);
  g_timer_stop (btimer);
  test_log_printfe ("%s<duration>%.6f</duration>\n", sindent (log_indent), g_timer_elapsed (btimer, NULL));
  log_indent -= 2;
  test_log_printfe ("%s</testbinary>\n", sindent (log_indent));
  g_free (subtest_last_seed);
  subtest_last_seed = NULL;
  if (need_restart)
    {
      /* restart test binary, skipping processed test cases */
      goto retry;
    }

  if (!gtester_quiet)
    g_print ("%s: %s\n", testcase_fail_count || !success ? "FAIL" : "PASS", binary);
  g_timer_destroy (btimer);
  if (subtest_mode_fatal && !success)
    terminate();
}

static void
usage (gboolean just_version)
{
  if (just_version)
    {
      g_print ("gtester version %d.%d.%d\n", GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
      return;
    }
  g_print ("Usage: gtester [OPTIONS] testprogram...\n");
  /*        12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
  g_print ("Options:\n");
  g_print ("  -h, --help                  show this help message\n");
  g_print ("  -v, --version               print version informations\n");
  g_print ("  --g-fatal-warnings          make warnings fatal (abort)\n");
  g_print ("  -k, --keep-going            continue running after tests failed\n");
  g_print ("  -l                          list paths of available test cases\n");
  g_print ("  -m=perf, -m=slow, -m=quick -m=thorough\n");
  g_print ("                              run test cases in mode perf, slow/thorough or quick (default)\n");
  g_print ("  -p=TESTPATH                 only start test cases matching TESTPATH\n");
  g_print ("  --seed=SEEDSTRING           start all tests with random number seed SEEDSTRING\n");
  g_print ("  -o=LOGFILE                  write the test log to LOGFILE\n");
  g_print ("  -q, --quiet                 suppress per test binary output\n");
  g_print ("  --verbose                   report success per testcase\n");
}

static void
parse_args (gint    *argc_p,
            gchar ***argv_p)
{
  guint argc = *argc_p;
  gchar **argv = *argv_p;
  guint i, e;
  /* parse known args */
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--g-fatal-warnings") == 0)
        {
          GLogLevelFlags fatal_mask = (GLogLevelFlags) g_log_set_always_fatal ((GLogLevelFlags) G_LOG_FATAL_MASK);
          fatal_mask = (GLogLevelFlags) (fatal_mask | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
          g_log_set_always_fatal (fatal_mask);
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "--gtester-selftest") == 0)
        {
          gtester_selftest = TRUE;
          argv[i] = NULL;
          break;        /* stop parsing regular gtester arguments */
        }
      else if (strcmp (argv[i], "-h") == 0 || strcmp (argv[i], "--help") == 0)
        {
          usage (FALSE);
          exit (0);
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "-v") == 0 || strcmp (argv[i], "--version") == 0)
        {
          usage (TRUE);
          exit (0);
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "--keep-going") == 0 ||
               strcmp (argv[i], "-k") == 0)
        {
          subtest_mode_fatal = FALSE;
          argv[i] = NULL;
        }
      else if (strcmp ("-p", argv[i]) == 0 || strncmp ("-p=", argv[i], 3) == 0)
        {
          gchar *equal = argv[i] + 2;
          if (*equal == '=')
            subtest_paths = g_slist_prepend (subtest_paths, equal + 1);
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              subtest_paths = g_slist_prepend (subtest_paths, argv[i]);
            }
          argv[i] = NULL;
        }
      else if (strcmp ("--test-arg", argv[i]) == 0 || strncmp ("--test-arg=", argv[i], 11) == 0)
        {
          gchar *equal = argv[i] + 10;
          if (*equal == '=')
            subtest_args = g_slist_prepend (subtest_args, equal + 1);
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              subtest_args = g_slist_prepend (subtest_args, argv[i]);
            }
          argv[i] = NULL;
        }
      else if (strcmp ("-o", argv[i]) == 0 || strncmp ("-o=", argv[i], 3) == 0)
        {
          gchar *equal = argv[i] + 2;
          if (*equal == '=')
            output_filename = equal + 1;
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              output_filename = argv[i];
            }
          argv[i] = NULL;
        }
      else if (strcmp ("-m", argv[i]) == 0 || strncmp ("-m=", argv[i], 3) == 0)
        {
          gchar *equal = argv[i] + 2;
          const gchar *mode = "";
          if (*equal == '=')
            mode = equal + 1;
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              mode = argv[i];
            }
          if (strcmp (mode, "perf") == 0)
            subtest_mode_perf = TRUE;
          else if (strcmp (mode, "slow") == 0 || strcmp (mode, "thorough") == 0)
            subtest_mode_quick = FALSE;
          else if (strcmp (mode, "quick") == 0)
            {
              subtest_mode_quick = TRUE;
              subtest_mode_perf = FALSE;
            }
          else
            g_error ("unknown test mode: -m %s", mode);
          argv[i] = NULL;
        }
      else if (strcmp ("-q", argv[i]) == 0 || strcmp ("--quiet", argv[i]) == 0)
        {
          gtester_quiet = TRUE;
          gtester_verbose = FALSE;
          argv[i] = NULL;
        }
      else if (strcmp ("--verbose", argv[i]) == 0)
        {
          gtester_quiet = FALSE;
          gtester_verbose = TRUE;
          argv[i] = NULL;
        }
      else if (strcmp ("-l", argv[i]) == 0)
        {
          gtester_list_tests = TRUE;
          argv[i] = NULL;
        }
      else if (strcmp ("--seed", argv[i]) == 0 || strncmp ("--seed=", argv[i], 7) == 0)
        {
          gchar *equal = argv[i] + 6;
          if (*equal == '=')
            subtest_seedstr = equal + 1;
          else if (i + 1 < argc)
            {
              argv[i++] = NULL;
              subtest_seedstr = argv[i];
            }
          argv[i] = NULL;
        }
    }
  /* collapse argv */
  e = 1;
  for (i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;
}

int
main (int    argc,
      char **argv)
{
  guint ui;

  /* some unices need SA_RESTART for SIGCHLD to return -EAGAIN for io.
   * we must fiddle with sigaction() *before* glib is used, otherwise
   * we could revoke signal hanmdler setups from glib initialization code.
   */
  if (TRUE)
    {
      struct sigaction sa;
      struct sigaction osa;
      sa.sa_handler = SIG_DFL;
      sigfillset (&sa.sa_mask);
      sa.sa_flags = SA_RESTART;
      sigaction (SIGCHLD, &sa, &osa);
    }

  g_set_prgname (argv[0]);
  parse_args (&argc, &argv);
  if (gtester_selftest)
    return main_selftest (argc, argv);

  if (argc <= 1)
    {
      usage (FALSE);
      return 1;
    }

  if (output_filename)
    {
      log_fd = g_open (output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
      if (log_fd < 0)
        g_error ("Failed to open log file '%s': %s", output_filename, g_strerror (errno));
    }

  test_log_printfe ("<?xml version=\"1.0\"?>\n");
  test_log_printfe ("%s<gtester>\n", sindent (log_indent));
  log_indent += 2;
  for (ui = 1; ui < argc; ui++)
    {
      const char *binary = argv[ui];
      launch_test (binary);
      /* we only get here on success or if !subtest_mode_fatal */
    }
  log_indent -= 2;
  test_log_printfe ("%s</gtester>\n", sindent (log_indent));

  close (log_fd);

  return 0;
}

static void
fixture_setup (guint        *fix,
               gconstpointer test_data)
{
  g_assert_cmphex (*fix, ==, 0);
  *fix = 0xdeadbeef;
}
static void
fixture_test (guint        *fix,
              gconstpointer test_data)
{
  g_assert_cmphex (*fix, ==, 0xdeadbeef);
  g_test_message ("This is a test message API test message.");
  g_test_bug_base ("http://www.example.com/bugtracker/");
  g_test_bug ("123");
  g_test_bug_base ("http://www.example.com/bugtracker?bugnum=%s;cmd=showbug");
  g_test_bug ("456");
}
static void
fixture_teardown (guint        *fix,
                  gconstpointer test_data)
{
  g_assert_cmphex (*fix, ==, 0xdeadbeef);
}

static int
main_selftest (int    argc,
               char **argv)
{
  /* gtester main() for --gtester-selftest invokations */
  g_test_init (&argc, &argv, NULL);
  g_test_add ("/gtester/fixture-test", guint, NULL, fixture_setup, fixture_test, fixture_teardown);
  return g_test_run();
}
