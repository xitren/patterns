#!/bin/sh
set -eu

cd "$(dirname "$0")/.."

rc=0

if ls /lib/ld-musl-*.so.1 >/dev/null 2>&1; then
  echo "Detected musl-based environment (Alpine)."
  echo "ASan/TSan can be unstable or unavailable here."
  echo "Use GitHub Actions sanitizer workflow or the Ubuntu/glibc devcontainer:"
  echo "  .devcontainer/devcontainer.ubuntu.json"
  exit 2
fi

echo "==> ASan+UBSan (clang_host_asan_ubsan_linux)"
cmake --preset=clang_host_asan_ubsan_linux
cmake --build --preset=clang_host_asan_ubsan_linux

# Improve diagnostics if llvm-symbolizer exists.
SYMBOLIZER=""
for c in llvm-symbolizer llvm-symbolizer-18 llvm-symbolizer-17 llvm-symbolizer-16; do
  if command -v "$c" >/dev/null 2>&1; then
    SYMBOLIZER="$(command -v "$c")"
    break
  fi
done

if [ -n "$SYMBOLIZER" ]; then
  export LLVM_SYMBOLIZER_PATH="$SYMBOLIZER"
fi

export ASAN_OPTIONS="${ASAN_OPTIONS:-halt_on_error=1:abort_on_error=1}"
export UBSAN_OPTIONS="${UBSAN_OPTIONS:-halt_on_error=1:abort_on_error=1}"

ctest --test-dir out/build/clang_host_asan_ubsan_linux --output-on-failure || rc=$?

echo "==> TSan (clang_host_tsan_linux)"
cmake --preset=clang_host_tsan_linux
cmake --build --preset=clang_host_tsan_linux

export TSAN_OPTIONS="${TSAN_OPTIONS:-halt_on_error=1:abort_on_error=1}"
ctest --test-dir out/build/clang_host_tsan_linux --output-on-failure || rc=$?

if [ "$rc" -ne 0 ]; then
  echo "Sanitizer checks failed (exit=$rc)"
  exit "$rc"
fi

echo "OK"

