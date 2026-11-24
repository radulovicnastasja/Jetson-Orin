
#!/usr/bin/env bash
set -euo pipefail

REPO="$HOME/Jetson-Orin"
DEST="$REPO/lab_06"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

mkdir -p "$DEST"

for f in encoder.c encoder.h pwm.c pwm.h deadband.csv step_response.csv; do
  if [[ ! -f "$DEST/$f" ]]; then
    if [[ -f "$REPO/lab_04/$f" ]]; then
      cp "$REPO/lab_04/$f" "$DEST/$f"
    else
      echo "Missing $f: copy it from lab04 before building" >&2
      exit 1
    fi
  fi
done

for f in main.c gamepad.c gamepad.h speed_control.c speed_control.h Makefile plot_lab06.py; do
  cp "$HERE/$f" "$DEST/$f"
done

cd "$DEST"

make clean
make

printf '\nBuild complete. To run with wheels lifted: cd %s && ./lab06 /dev/input/event6\n' "$DEST"
