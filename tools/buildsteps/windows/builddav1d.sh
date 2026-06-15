#!/bin/bash

set -e

[[ -f buildhelpers.sh ]] &&
    source buildhelpers.sh

DAV1D_VERSION_FILE=/xbmc/tools/depends/target/dav1d/DAV1D-VERSION
DAV1D_PATCHES_DIR=/xbmc/tools/depends/target/dav1d

do_loaddeps $DAV1D_VERSION_FILE
DAV1DDESTDIR=$PREFIX

echo "=== dav1d build diagnostics ==="
echo " LIBNAME    = $LIBNAME"
echo " VERSION    = $VERSION"
echo " SHA512     = $SHA512"
echo " ARCHIVE    = $ARCHIVE"
echo " PREFIX     = $PREFIX"
echo " TRIPLET    = $TRIPLET"
echo " LOCALSRCDIR= $LOCALSRCDIR"
echo " meson      = $(command -v meson || echo NOT FOUND)"
echo " ninja      = $(command -v ninja || echo NOT FOUND)"
echo " nasm       = $(command -v nasm || echo NOT FOUND)"
echo " python3    = $(command -v python3 || echo NOT FOUND)"
echo "==============================="

do_clean $1
do_download

do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$blue_color" "Configuring"

if [[ -f $DAV1D_PATCHES_DIR/01-win-sharedname.patch ]]; then
  echo "Applying dav1d windows patch"
  patch -p1 -d $LOCALSRCDIR -i $DAV1D_PATCHES_DIR/01-win-sharedname.patch -N -r -
fi

DAV1D_INSTALL_PREFIX="--prefix=$DAV1DDESTDIR --libdir=$DAV1DDESTDIR/lib"
MESON_BUILD_TYPE=release

cd $LOCALSRCDIR
rm -rf build
mkdir -p build
cd build

set +e
if [[ -z "$MESON" || "$MESON" == "$MESON_BUILD_TYPE" ]]; then
  if command -v meson >/dev/null 2>&1; then
    MESON=meson
  elif command -v python3 >/dev/null 2>&1 && python3 -m mesonbuild --version >/dev/null 2>&1; then
    MESON="python3 -m mesonbuild"
  elif command -v py >/dev/null 2>&1 && py -m mesonbuild --version >/dev/null 2>&1; then
    MESON="py -m mesonbuild"
  else
    do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$red_color" "meson not found (PATH: $PATH)"
    exit 127
  fi
fi
if [[ -z "$NINJA" || "$NINJA" == "$MESON" ]]; then
  if command -v ninja >/dev/null 2>&1; then
    NINJA=ninja
  else
    do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$red_color" "ninja not found (PATH: $PATH)"
    exit 127
  fi
fi
echo "Running: $MESON --prefix=$DAV1DDESTDIR --libdir=$DAV1DDESTDIR/lib ..."
"$MESON" \
  --prefix="$DAV1DDESTDIR" \
  --libdir="$DAV1DDESTDIR/lib" \
  --buildtype="$MESON_BUILD_TYPE" \
  --default-library=shared \
  -Denable_asm=true \
  -Denable_tools=false \
  -Denable_examples=false \
  -Denable_tests=false \
  ..
MESON_RC=$?
set -e
if [[ $MESON_RC -ne 0 ]]; then
  do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$red_color" "Configure error (rc=$MESON_RC)"
  exit $MESON_RC
fi

do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$blue_color" "Compiling"
"$NINJA" -v
if [[ $? -ne 0 ]]; then
  do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$red_color" "Compile error"
  exit 1
fi

do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$blue_color" "Installing"
"$NINJA" -v install
if [[ $? -ne 0 ]]; then
  do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$red_color" "Install error"
  exit 1
fi

# Generate MSVC-compatible import library (libdav1d.lib) from the MinGW
# .dll/.dll.a. FFmpeg is built with --toolchain=msvc and uses
# -llibdav1d, so it needs a COFF import library, not a MinGW .a.
if [[ -f "$DAV1DDESTDIR/bin/libdav1d.dll" ]]; then
  DAV1D_DLL="$DAV1DDESTDIR/bin/libdav1d.dll"
  DAV1D_DLLA="$DAV1DDESTDIR/lib/libdav1d.dll.a"
  DAV1D_DEF="$DAV1DDESTDIR/lib/libdav1d.def"
  DAV1D_IMPLIB="$DAV1DDESTDIR/lib/libdav1d.lib"

  echo "Generating $DAV1D_DEF from $DAV1D_DLL"
  if command -v gendef >/dev/null 2>&1; then
    (cd "$(dirname "$DAV1D_DLL")" && gendef "$(basename "$DAV1D_DLL")")
    GENERATED_DEF="$(dirname "$DAV1D_DLL")/libdav1d.def"
    if [[ -f "$GENERATED_DEF" ]]; then
      mv -f "$GENERATED_DEF" "$DAV1D_DEF"
      echo "  -> $(wc -l < "$DAV1D_DEF") exports in .def (from gendef)"
    else
      echo "WARNING: gendef did not produce $GENERATED_DEF, falling back to objdump"
    fi
  fi
  if [[ ! -f "$DAV1D_DEF" ]]; then
    OBJDUMP=""
    for cand in x86_64-w64-mingw32-objdump objdump; do
      if command -v "$cand" >/dev/null 2>&1; then
        OBJDUMP="$cand"
        break
      fi
    done
    if [[ -n "$OBJDUMP" ]]; then
      echo "  generating .def via $OBJDUMP"
      {
        echo "LIBRARY libdav1d.dll"
        echo "EXPORTS"
        "$OBJDUMP" -p "$DAV1D_DLL" \
          | grep -E '^\s*\[[ 0-9]+\]\s+\+base' \
          | awk '{
              for (i=1; i<=NF; i++) {
                if ($i ~ /^dav1d_/) { print $i; break }
              }
            }' \
          | grep -E '^dav1d_[a-zA-Z0-9_]+$' \
          | sort -u
      } > "$DAV1D_DEF"
      EXPORT_COUNT=$(grep -c '^dav1d_' "$DAV1D_DEF" 2>/dev/null || echo 0)
      echo "  -> $EXPORT_COUNT dav1d_* exports in .def (from objdump)"
    else
      echo "WARNING: no gendef and no objdump found, falling back to minimal .def"
      cat > "$DAV1D_DEF" <<'EOF'
LIBRARY libdav1d.dll
EXPORTS
dav1d_version
dav1d_default_settings
dav1d_open
dav1d_parse_seq_hdr
dav1d_submit_frame
dav1d_get_picture
dav1d_apply_n_tiling
dav1d_flush
dav1d_close
EOF
    fi
  fi

  echo "Generating $DAV1D_IMPLIB from $DAV1D_DEF"
  DLLTOOL=""
  for cand in x86_64-w64-mingw32-dlltool dlltool; do
    if command -v "$cand" >/dev/null 2>&1; then
      DLLTOOL="$cand"
      break
    fi
  done
  if [[ -n "$DLLTOOL" ]]; then
    "$DLLTOOL" \
      --input-def "$DAV1D_DEF" \
      --output-lib "$DAV1D_IMPLIB" \
      --dllname libdav1d.dll
  else
    echo "ERROR: dlltool not found - cannot create $DAV1D_IMPLIB"
    echo "Install mingw-w64-x86_64-tools-pkgconfig or binutils"
    exit 1
  fi
else
  echo "WARNING: $DAV1DDESTDIR/bin/libdav1d.dll not found; skipping import-lib generation"
fi

echo "=== installed files in $DAV1DDESTDIR ==="
find "$DAV1DDESTDIR" -type f \( -name "*dav1d*" -o -name "*DAV1D*" \) 2>/dev/null
echo "=== $DAV1DDESTDIR/lib contents ==="
ls -la "$DAV1DDESTDIR/lib" 2>/dev/null
echo "=== $DAV1DDESTDIR/include contents ==="
ls -la "$DAV1DDESTDIR/include" 2>/dev/null
echo "==================================="

# FFmpeg's buildffmpeg.sh does not add $PREFIX/include to its -I search
# path; it only searches $LOCALDESTDIR/include and /depends/$TRIPLET/include.
# Copy the dav1d headers and import-lib there so the MSVC `cl.exe` can find
# them via the existing -I/-LIBPATH flags.
if [[ -n "$LOCALDESTDIR" && -d "$LOCALDESTDIR" ]]; then
  echo "Mirroring dav1d headers to $LOCALDESTDIR/include for FFmpeg"
  mkdir -p "$LOCALDESTDIR/include/dav1d"
  cp -f "$DAV1DDESTDIR/include/dav1d/dav1d.h" "$LOCALDESTDIR/include/dav1d/"
  if [[ -d "$DAV1DDESTDIR/include/dav1d" ]]; then
    find "$DAV1DDESTDIR/include/dav1d" -maxdepth 1 -type f -name "*.h" -exec cp -f {} "$LOCALDESTDIR/include/dav1d/" \;
  fi
  if [[ -f "$DAV1DDESTDIR/lib/libdav1d.lib" ]]; then
    cp -f "$DAV1DDESTDIR/lib/libdav1d.lib" "$LOCALDESTDIR/lib/"
  fi
  if [[ -f "$DAV1DDESTDIR/lib/libdav1d.def" ]]; then
    cp -f "$DAV1DDESTDIR/lib/libdav1d.def" "$LOCALDESTDIR/lib/"
  fi
  if [[ -f "$DAV1DDESTDIR/bin/libdav1d.dll" ]]; then
    cp -f "$DAV1DDESTDIR/bin/libdav1d.dll" "$LOCALDESTDIR/bin/" 2>/dev/null || true
  fi
fi

do_print_status "$LIBNAME-$VERSION (${TRIPLET})" "$green_color" "Done"
exit 0
