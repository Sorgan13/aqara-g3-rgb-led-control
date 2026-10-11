#!/bin/sh
# Install on Aqara Camera Hub G3 as root (BusyBox compatible).
set -eu
BASE=$(CDPATH= cd "$(dirname "$0")" && pwd)
DEST=${G3_INSTALL_DIR:-/data/bin}
MARKER="$DEST/.g3-light-owned-ledctl"

fatal() { echo "INSTALL ERROR: $*" >&2; exit 1; }
[ "$(id -u)" = 0 ] || fatal 'Run as root on the camera'
case "$(uname -m)" in armv7*|armv8*|arm) : ;; *) fatal 'This build is for ARM cameras only' ;; esac
AGENT_SOCKET=${G3_AGENT_SOCKET:-/tmp/miio_agent.socket}
[ -S "$AGENT_SOCKET" ] || fatal "Aqara ha_agent socket missing: $AGENT_SOCKET"
[ -r "$BASE/g3-light" ] || fatal 'Missing g3-light script'
[ -r "$BASE/bin/g3-ledctl-armv7" ] || fatal 'Missing bundled g3-ledctl-armv7'
command -v agetprop >/dev/null 2>&1 || fatal 'agetprop not found; is this an Aqara G3?'
case "$(agetprop persist.sys.did)" in lumi1.*) : ;; *) fatal 'Could not identify Aqara camera DID' ;; esac
# Read-only safety check before writing any binaries.
GPIO_DEBUG=${G3_GPIO_DEBUG:-/sys/kernel/debug/gpio}
[ -r "$GPIO_DEBUG" ] || fatal 'Cannot inspect kernel GPIO labels: /sys/kernel/debug/gpio'
grep -Eq 'gpio-6[[:space:]]+[(].*[|]green-gpio[[:space:]]' "$GPIO_DEBUG" || fatal 'GPIO6 is not green-gpio'
grep -Eq 'gpio-48[[:space:]]+[(].*[|]blue-gpio[[:space:]]' "$GPIO_DEBUG" || fatal 'GPIO48 is not blue-gpio'
grep -Eq 'gpio-49[[:space:]]+[(].*[|]red-gpio[[:space:]]' "$GPIO_DEBUG" || fatal 'GPIO49 is not red-gpio'
sh -n "$BASE/g3-light" || fatal 'Controller shell syntax failed'

mkdir -p "$DEST"
if [ ! -e "$DEST/g3-ledctl" ]; then
  cp "$BASE/bin/g3-ledctl-armv7" "$DEST/.g3-ledctl.new.$$"
  chmod 755 "$DEST/.g3-ledctl.new.$$"
  mv "$DEST/.g3-ledctl.new.$$" "$DEST/g3-ledctl"
  printf '%s\n' 'installed-by-g3-light' > "$MARKER"
  echo 'Installed bundled /data/bin/g3-ledctl'
else
  [ -x "$DEST/g3-ledctl" ] || fatal 'Existing g3-ledctl is not executable; refusing to overwrite'
  echo 'Preserving existing /data/bin/g3-ledctl'
fi
if [ -e "$DEST/g3-light" ]; then
  BACKUP="$DEST/g3-light.bak.$(date +%Y%m%d%H%M%S).$$"
  cp "$DEST/g3-light" "$BACKUP"
  echo "Backed up old controller: $BACKUP"
fi
cp "$BASE/g3-light" "$DEST/.g3-light.new.$$"
chmod 755 "$DEST/.g3-light.new.$$"
mv "$DEST/.g3-light.new.$$" "$DEST/g3-light"
echo "Installed $DEST/g3-light"
"$DEST/g3-light" status
printf '\nTry: %s\n' "$DEST/g3-light yellow 2"
