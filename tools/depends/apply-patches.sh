#!/bin/sh
# Apply patches to a source tree idempotently using marker files.
#
# CMake ExternalProject patch steps can rerun on an already-patched source
# tree (for example, when the patch command changes between configures but
# the source tree is not re-extracted). Plain `patch` then prompts
# interactively with "Reversed (or previously applied) patch detected!" and
# hangs the build.
#
# This wrapper records each successfully applied patch as a marker file in
# <source_dir>/.kodi-applied-patches/. The marker name embeds a checksum of
# the patch file, so the same patch is skipped on later reruns while edited
# patch files get a new marker and are applied again.
#
# Usage: apply-patches.sh <patch_program> <source_dir> <patch_file> [<patch_file> ...]

set -eu

if [ $# -lt 3 ]; then
    echo "Usage: $0 <patch_program> <source_dir> <patch_file> [<patch_file> ...]" >&2
    exit 2
fi

patch_program="$1"
source_dir="$2"
shift 2

if ! command -v "$patch_program" >/dev/null 2>&1; then
    echo "apply-patches.sh: patch program does not exist: $patch_program" >&2
    exit 1
fi

if [ ! -d "$source_dir" ]; then
    echo "apply-patches.sh: source directory does not exist: $source_dir" >&2
    exit 1
fi

marker_dir="$source_dir/.kodi-applied-patches"
mkdir -p "$marker_dir"

for patch_file in "$@"; do
    if [ ! -f "$patch_file" ]; then
        echo "apply-patches.sh: patch file does not exist: $patch_file" >&2
        exit 1
    fi

    patch_name=$(basename "$patch_file")
    patch_hash=$(cksum "$patch_file" | awk '{print $1"-"$2}')
    marker="$marker_dir/${patch_name}.${patch_hash}"

    if [ -f "$marker" ]; then
        echo "apply-patches.sh: skipping (already applied): $patch_name"
        continue
    fi

    echo "apply-patches.sh: applying $patch_name"
    # -N (--forward): skip already-applied / reversed patches without
    # silently rolling them back. --batch: never prompt interactively.
    if ! "$patch_program" -d "$source_dir" -p1 -N --batch -i "$patch_file"; then
        echo "apply-patches.sh: failed to apply $patch_name" >&2
        echo "apply-patches.sh: hint: source tree may be stale, re-extract it and try again." >&2
        exit 1
    fi

    : > "$marker"
done
