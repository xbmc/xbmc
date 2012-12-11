include_directories(pcre)
add_definitions(-DLINK_SIZE=2 -DHAVE_CONFIG_H)
set(pcre_SRCS
  pcre/pcre_chartables.c
  pcre/pcre_compile.c
  pcre/pcre_config.c
  pcre/pcre_dfa_exec.c
  pcre/pcre_exec.c
  pcre/pcre_fullinfo.c
  pcre/pcre_get.c
  pcre/pcre_globals.c
  pcre/pcre_info.c
  pcre/pcre_maketables.c
  pcre/pcre_newline.c
  pcre/pcre_ord2utf8.c
  pcre/pcre_refcount.c
  pcre/pcre_study.c
  pcre/pcre_tables.c
  pcre/pcre_try_flipped.c
  pcre/pcre_ucd.c
  pcre/pcre_valid_utf8.c
  pcre/pcre_version.c
  pcre/pcre_xclass.c
  pcre/pcrecpp.cc
  pcre/pcreposix.c
)
add_library(pcre ${pcre_SRCS})
