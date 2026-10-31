#!/usr/bin/env bash
set -euo pipefail
# SPDX-License-Identifier: MIT
# Compile-only check with an existing ESP RISC-V toolchain, no flash/device I/O.
: "${TARGET_CXX:?Set TARGET_CXX to the installed ESP RISC-V g++ path}"
root="$(cd "$(dirname "$0")/.." && pwd)"
out="${1:-$root/.validation/target}"
mkdir -p "$out"
python3 "$root/scripts/prepare_font.py" --size 32 --weight 500 --chars-file "$root/examples/text.txt" --output "$out/font" --overwrite
for source in "$root/components/plus_font/plus_font.cc" "$out/font/font.cc" "$root/examples/render.cpp"; do
  "$TARGET_CXX" -std=c++11 -Os -fno-exceptions -fno-rtti -Wall -Wextra -Werror \
    -I"$root/components/plus_font/include" -I"$out/font" -c "$source" -o "$out/$(basename "$source").o"
done
"$TARGET_CXX" --version
printf '%s\n' 'PASS target C++ component, generated array and example compilation (not a flashed firmware)'
