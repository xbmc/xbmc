include(CheckIncludeFiles)

######################### CHECK HEADER
set(headers
  arpa/inet
  cdio/iso9660
  dirent
  execinfo
  fcntl
  float
  inttypes
  limits
  locale
  malloc
  memory
  ndir
  netdb
  netinet/in
  nfsc/libfs
  stdbool
  stddef
  stdint
  stdlib
  strings
  string
  sys/dir
  sys/file
  sys/ioctl
  sys/mount
  sys/ndir
  sys/param
  sys/select
  sys/socket
  sys/stat
  sys/timeb
  sys/time
  sys/types
  sys/vfs
  termios
  unistd
  utime
  wchar
  wctype
)

foreach(header ${headers})
  set(_HAVE_VAR HAVE_${header}_H)
  string(TOUPPER ${_HAVE_VAR} _HAVE_VAR)
  string(REPLACE "/" "_" _HAVE_VAR ${_HAVE_VAR})
  check_include_files(${header}.h ${_HAVE_VAR})
endforeach()


