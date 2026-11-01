#!/usr/bin/env sh
# Launch Kodi on a paired Samsung TV under the Web Inspector and forward the
# DevTools port it opens to localhost. See docs/README.WASM.md §7.4.
set -eu

APP_ID="${KODI_TIZEN_APP_ID:-kodiplayer.Kodi}"
LOCAL_PORT="${KODI_TIZEN_INSPECT_PORT:-7011}"
SDB="${SDB:-$(command -v sdb || echo "$HOME/tizen-studio/tools/sdb")}"

set -- 
if [ -n "${TIZEN_TARGET_SERIAL:-}" ]; then
  set -- -s "$TIZEN_TARGET_SERIAL"
fi

echo "Launching $APP_ID in debug mode..."
out=$("$SDB" "$@" shell 0 debug "$APP_ID" 2>&1) || { printf '%s\n' "$out" >&2; exit 1; }
printf '%s\n' "$out"

port=$(printf '%s\n' "$out" | sed -n 's/.*port: *\([0-9][0-9]*\).*/\1/p' | head -n 1)
if [ -z "$port" ]; then
  echo "No debug port in the launch output; is the TV in developer mode and the app installed?" >&2
  exit 1
fi

"$SDB" "$@" forward --remove "tcp:$LOCAL_PORT" >/dev/null 2>&1 || true
"$SDB" "$@" forward "tcp:$LOCAL_PORT" "tcp:$port"

cat <<MSG

DevTools endpoint forwarded to http://localhost:$LOCAL_PORT/
  - In Chrome open chrome://inspect, click "Configure..." next to
    "Discover network targets", add localhost:$LOCAL_PORT, then click
    "inspect" on the Kodi target.
  - The page served at http://localhost:$LOCAL_PORT/ lists targets too, but
    its bundled DevTools frontend is old and may render blank in a current
    Chrome; chrome://inspect uses Chrome's own frontend.
Kodi is already running when the inspector attaches, so startup console
output is missed.
MSG
