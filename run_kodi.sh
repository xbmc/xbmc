#!/usr/bin/bash

. ./build_config.sh

#strict_init_order
#strict_string_checks

# Define ICU_4C_LIB for run script

ICU_4C_LIB="${INSTALL_DIR}/lib"

# If you want to use installed icu lib from gnu on Linux
#ICU_4C_LIB="/usr/lib/x86_64-linux-gnu/"

LD_LIBRARY_PATH="${ICU_4C_LIB}"
ASAN_OPTIONS=""
ASAN_OPTIONS="detect_stack_use_after_return=1:check_initialization_order=1:quarantine_size_mb=1:thread_local_quarantine_size_kb=128:redzone=64:debug=1:report_globals=2:max_malloc_fill_size=8192:max_free_fill_size=8192:print_stats=1:print_scariness=1:log_path=/tmp/address.log:verbosity=1:print_module_map=1:heap_profile=1:coverage=1:coverage_dir=/tmp" 
MALLOC_OPTIONS=""
#MALLOC_OPTIONS="LD_PRELOAD=usr/lib/x86_64-linux-gnu/libc_malloc_debug.so MALLOC_TRACE=/tmp/malloc.trace"
#ASAN_OPTIONS="detect_stack_use_after_return=1:quarantine_size_mb=1:thread_local_quarantine_size_kb=128:redzone=64:debug=1:max_malloc_fill_size=8192:max_free_fill_size=8192:log_path=/tmp/address.log:verbosity=1:heap_profile=1:coverage=1:coverage_dir=/tmp" 
# ASAN_OPTIONS="detect_leaks=1"

#ASAN_OPTIONS="detect_leaks=0"
#ASAN_OPTIONS="${ASAN_OPTIONS}" LD_LIBRARY_PATH="${LD_LIBRARY_PATH}" lldb-14 ${INSTALL_DIR}/lib/kodi/kodi-x11
#LANG=tr_TR.UTF-8 ASAN_OPTIONS="${ASAN_OPTIONS}" LD_LIBRARY_PATH="${LD_LIBRARY_PATH}" ${INSTALL_DIR}/lib/kodi/kodi-x11
ASAN_OPTIONS="${ASAN_OPTIONS}" LD_LIBRARY_PATH="${LD_LIBRARY_PATH}" ${INSTALL_DIR}/lib/kodi/kodi-x11
