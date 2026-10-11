#!/bin/sh
# Removes only files installed by this project. Does not change camera daemons.
set -eu
DEST=${G3_INSTALL_DIR:-/data/bin}
[ "$(id -u)" = 0 ] || { echo 'Run as root on camera' >&2; exit 1; }
rm -f "$DEST/g3-light"
if [ -f "$DEST/.g3-light-owned-ledctl" ]; then
  rm -f "$DEST/g3-ledctl" "$DEST/.g3-light-owned-ledctl"
  echo 'Removed package-owned g3-ledctl'
else
  echo 'Preserved existing g3-ledctl (not installed by this package)'
fi
echo 'Uninstalled g3-light. Previous g3-light.bak.* backups were preserved.'
echo 'Use Aqara Home to restore your preferred indicator setting if needed.'
