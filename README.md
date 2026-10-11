# Aqara G3 RGB LED Control v1.1.0

**Unofficial, experimental local RGB status-LED control for the Aqara Camera Hub G3.** No cloud connection, firmware modification, background service, or camera connection is required by this repository.

> This project is independent and not affiliated with Aqara. It is experimental hardware-specific software. The GPIO mapping and native indicator protocol are provided for compatible G3 systems; no independent real-camera testing was performed as part of this publication.

## Features

- Native indicator commands: `on`, `off`, and `status`.
- Seven binary RGB combinations: `red`, `green`, `blue`, `yellow`, `cyan`, `magenta`, and `white`.
- Temporary colors for 1–60 seconds with restoration of the previous GPIO state.
- Self-contained static ARMv7 helper binary and its C source.
- Install-time and runtime GPIO-label checks to avoid using these pins on another model.

Seven binary combinations are supported. Arbitrary 24-bit colors and PWM are not implemented.

## Requirements and confirmed pinout

- Aqara Camera Hub G3 compatible with this mapping.
- Root access through the camera's Telnet shell.
- BusyBox/POSIX `sh`, a working `ha_agent`, and writable GPIO value files.
- Accessible `/sys/kernel/debug/gpio` labels.

The confirmed G3 mapping is active-low:

| Channel | GPIO | Active state |
| --- | ---: | --- |
| Red | 49 (`red-gpio`) | LOW (`0`) |
| Green | 6 (`green-gpio`) | LOW (`0`) |
| Blue | 48 (`blue-gpio`) | LOW (`0`) |

Do not use these GPIO numbers on another Aqara model without verifying the kernel labels.

## Install from the Release archive

Copy `g3-light-v1.1.0.tar.gz` to a trusted local HTTP server reachable by the camera, for example Home Assistant `/config/www/`. From a root Telnet shell on the camera, replace `HA_IP` with the address of that server:

```sh
wget -O /tmp/g3-light.tar.gz http://HA_IP:8123/local/g3-light-v1.1.0.tar.gz
mkdir -p /tmp/g3-light-setup
tar -xzf /tmp/g3-light.tar.gz -C /tmp/g3-light-setup
sh /tmp/g3-light-setup/g3-light/install.sh
```

No compiler is needed on the camera. The installer installs `/data/bin/g3-light` and installs the bundled `/data/bin/g3-ledctl` only when no executable helper already exists. It preserves an existing helper and backs up an existing controller before replacement.

## Usage

```sh
/data/bin/g3-light status
/data/bin/g3-light on
/data/bin/g3-light off
/data/bin/g3-light red
/data/bin/g3-light green
/data/bin/g3-light blue
/data/bin/g3-light yellow
/data/bin/g3-light cyan
/data/bin/g3-light magenta
/data/bin/g3-light white
/data/bin/g3-light green 3   # temporary color, then restore
```

`on` invokes the native indicator command and sets blue; `off` invokes the native off command and turns off all three GPIO channels. A color without a timer remains until another command or camera firmware changes the pins.

## Uninstall

From the unpacked archive:

```sh
sh /tmp/g3-light-setup/g3-light/uninstall.sh
```

The uninstaller removes only the controller and a helper that this package installed itself. It preserves pre-existing helpers and backup files.

## Restore the standard indicator

```sh
/data/bin/g3-light on
```

You can also restore the preferred indicator setting through Aqara Home. The camera's own services are not stopped by this project.

## Important warnings and limitations

- Changing the LED status may mask the camera's normal recording, streaming, or privacy indication. **Do not use this project to conceal recording or disable required privacy indicators.**
- Camera firmware can override a selected GPIO color.
- This project does not provide continuous color forcing, PWM, or arbitrary 24-bit RGB values.
- Keep root/Telnet access on a trusted LAN; do not expose the camera or Telnet to the Internet.
- The repository contains no Aqara proprietary firmware, credentials, tokens, private network addresses, or camera-specific private identifiers.

## Repository contents

```text
g3-light                 # POSIX-sh controller
install.sh               # installation/update script
uninstall.sh             # safe removal script
bin/g3-ledctl-armv7      # bundled ARMv7 executable
src/g3-ledctl.c          # helper source code
README.md                # English documentation
README.en.md             # English documentation copy
README.ru.md             # Russian documentation
LICENSE                  # MIT
SHA256SUMS               # file checksums
```

See [README.ru.md](README.ru.md) for the Russian documentation.
