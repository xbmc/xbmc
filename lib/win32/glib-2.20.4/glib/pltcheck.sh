#!/bin/sh

LANG=C

status=0

if ! which readelf 2>/dev/null >/dev/null; then
	echo "'readelf' not found; skipping test"
	exit 0
fi

for so in .libs/lib*.so; do
	echo Checking $so for local PLT entries
	# g_string_insert_c is used in g_string_append_c_inline
	# unaliased.  Couldn't find a way to fix it.
	# Same for g_once_init_enter
	readelf -r $so | grep 'JU\?MP_SLOT\?' | \
		grep -v '\<g_string_insert_c\>' | \
		grep -v '\<g_atomic_[a-z]*_[sg]et\>' | \
		grep -v '\<g_once_init_enter_impl\>' | \
		grep -v '\<g_bit_' | \
		grep '\<g_' && status=1
done

exit $status
