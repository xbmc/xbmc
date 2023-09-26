#!/bin/sh

# found on https://github.com/webosbrew/dev-utils/tree/main/scripts

EXE="$1"

if [ ! -f "${EXE}" ]; then
  echo "Usage: $0 executable"
  exit 1
fi

if [ ! -d "${WEBOS_ROOTFS}" ]; then
  echo 'WEBOS_ROOTFS is not a directory'
  exit 1
fi

lib_search_paths="${WEBOS_ROOTFS}/lib:${WEBOS_ROOTFS}/usr/lib:${WEBOS_LD_LIBRARY_PATH}"

required_syms=$(nm --dynamic --extern-only --undefined-only "${EXE}" | grep ' [U] ' | tr -s ' ' | cut -d ' ' -f 3)

needed_libs=$(objdump -p "${EXE}" | grep NEEDED | tr -s ' ' | cut -d ' ' -f 3)
found_libs=""
has_missing=0

for lib in ${needed_libs}; do
  lib_found=0
  OLDIFS=$IFS
  IFS=:
  for path in ${lib_search_paths}; do
    lib_path="${path}/${lib}"
    if [ -f "${lib_path}" ]; then
      lib_found=1
      found_libs="${found_libs} ${lib_path}"
    fi
  done
  IFS=$OLDIFS
  if [ ${lib_found} = 0 ]; then
    has_missing=1
    echo "Missing library: ${lib}"
  fi
done

# shellcheck disable=SC2086
lib_syms=$(nm --dynamic --extern-only --defined-only ${found_libs} | grep ' [a-zA-Z] ' | cut -d ' ' -f 3 | tr -s '@')

for sym in ${required_syms}; do
  if ! echo "${lib_syms}" | grep -q "${sym}"; then
    has_missing=1
    sym_name=$(echo "${sym}" | cut -d '@' -f 1 | c++filt)
    if echo "${sym}" | grep -q '@'; then
      sym_ver="@$(echo ${sym} | cut -d '@' -f 2)"
    fi
    echo "Missing symbol: ${sym_name}${sym_ver}"
  fi
done

if [ ${has_missing} = 0 ]; then
  echo "All OK."
fi

return $has_missing
