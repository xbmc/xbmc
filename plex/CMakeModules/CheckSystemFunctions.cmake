include(CheckFunctionExists)

######################### CHECK FUNCTIONS 
set(functions
  alarm
  atexit
  chown
  _doprnt
  dup2
  fdatasync
  floor
  fs_stat_dev
  ftime
  ftruncate
  getcwd
  gethostbyaddr
  gethostbyname
  getpagesize
  getpass
  gettimeofday
  inet_ntoa
  inotify
  lchown
  localeconv
  memchr
  memmove
  mkdir
  modf
  mmap
  munmap
  pow
  rmdir
  select
  setenv
  setlocale
  socket
  sqrt
  strcasecmp
  strchr
  strcoll
  strcspn
  strdup
  strerror
  strftime
  strncasecmp
  strpbrk
  strrchr
  strspn
  strstr
  strtol
  strtoul
  sysinfo
  tzset
  utime
  vprintf
)

foreach(func ${functions})
  set(_HAVE_VAR HAVE_${func})
  string(TOUPPER ${_HAVE_VAR} _HAVE_VAR)
  check_function_exists(${func} ${_HAVE_VAR})
endforeach()


