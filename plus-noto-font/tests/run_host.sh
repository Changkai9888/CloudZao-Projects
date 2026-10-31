#!/usr/bin/env bash
set -euo pipefail
# SPDX-License-Identifier: MIT
root="$(cd "$(dirname "$0")/.." && pwd)"
out="${1:-$root/.validation/host}"
mkdir -p "$out"
python3 "$root/tests/check_assets.py" --output "$out/fonts"
python3 "$root/scripts/prepare_font.py" --size 32 --weight 500 --chars-file "$root/examples/text.txt" --output "$out/demo" --overwrite
"${CXX:-g++}" -std=c++11 -O1 -g -Wall -Wextra -Werror -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer \
  -I"$root/components/plus_font/include" "$root/components/plus_font/plus_font.cc" "$root/tests/runtime_test.cc" -o "$out/runtime-test"
"$out/runtime-test" "$out"/fonts/*.ntf
"${CXX:-g++}" -std=c++11 -O2 -Wall -Wextra -Werror -I"$root/components/plus_font/include" -I"$out/demo" \
  "$root/components/plus_font/plus_font.cc" "$out/demo/font.cc" "$root/examples/render.cpp" -o "$out/font-demo"
"$out/font-demo" "$out/demo.pbm" > "$out/demo.log"
printf '%s\n' 'PASS runtime sanitizer checks, all 18 fonts, compiled example and embedded OFL'
