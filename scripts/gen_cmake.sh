#!/usr/bin/env bash
# gen_cmake.sh — generate CMakeLists.txt from a .vcxproj (mingw-w64 cross-compile)
# Usage: ./scripts/gen_cmake.sh [-f] <PROJECT>
#   -f   overwrite existing CMakeLists.txt without asking
# Example: ./scripts/gen_cmake.sh I
set -euo pipefail

# ── helpers ──────────────────────────────────────────────────────────────────

die()  { echo "ERROR: $*" >&2; exit 1; }
warn() { echo "WARNING: $*" >&2; }

# Detect distro and print SDL2 mingw install command
sdl2_install_hint() {
  if command -v apt-get &>/dev/null; then
    echo "  sudo apt-get install libsdl2-mingw-dev mingw-w64"
  elif command -v dnf &>/dev/null; then
    echo "  sudo dnf install mingw64-SDL2 mingw64-gcc-c++"
  elif command -v pacman &>/dev/null; then
    echo "  sudo pacman -S mingw-w64-sdl2 mingw-w64-gcc"
  elif command -v zypper &>/dev/null; then
    echo "  sudo zypper install mingw64-cross-SDL2 mingw64-cross-gcc-c++"
  else
    echo "  (unknown distro — install mingw-w64 and SDL2 for mingw from your package manager)"
  fi
}

# Check that the mingw compiler is available
check_mingw() {
  if ! command -v x86_64-w64-mingw32-g++ &>/dev/null; then
    echo "ERROR: x86_64-w64-mingw32-g++ not found." >&2
    echo "Install mingw-w64 + SDL2 mingw dev package, e.g.:" >&2
    sdl2_install_hint >&2
    exit 1
  fi
}

# Check SDL2 mingw headers exist (proxy for libsdl2-mingw-dev)
check_sdl2_mingw() {
  local prefix="${SDL2_PREFIX:-/usr/x86_64-w64-mingw32}"
  local sdl_header="${prefix%/}/include/SDL2/SDL.h"
  if [ ! -f "$sdl_header" ]; then
    echo "ERROR: SDL2 mingw headers not found (expected $sdl_header)." >&2
    echo "Check SDL2_PREFIX in scripts/gen_cmake.conf or install package:" >&2
    sdl2_install_hint >&2
    exit 1
  fi
}

# Parse a single XML element value: first match of <TAG>VALUE</TAG>
xml_val() {
  local tag="$1" file="$2"
  grep -oPm1 "(?<=<${tag}>)[^<]+" "$file" 2>/dev/null || true
}

# Extract all ClCompile Include= values
cpp_sources() {
  local file="$1"
  grep -oP '(?<=<ClCompile Include=")[^"]+' "$file" 2>/dev/null || true
}

# Detect SDL2 usage: look for #include.*SDL in listed .cpp sources
uses_sdl2() {
  local src_dir="$1" vcxproj="$2"
  while IFS= read -r src; do
    local path="$src_dir/$src"
    [ -f "$path" ] || continue
    if grep -qE '#include[[:space:]]*("|<)SDL' "$path"; then
      return 0
    fi
  done < <(cpp_sources "$vcxproj")
  return 1
}

# Detect ppl.h usage in any C++/header files
uses_ppl() {
  local src_dir="$1"
  grep -rqE --include="*.h" --include="*.hpp" --include="*.cpp" --exclude="ppl.h" '#include[[:space:]]*[<"]ppl\.h[>"]' "$src_dir" 2>/dev/null
}

emit_ppl_shim() {
  local target_file="$1"
  cat << 'EOF' > "$target_file"
// ppl.h shim — provides concurrency::parallel_for via OpenMP for MinGW/GCC.
#pragma once
#ifndef PPL_SHIM_H
#define PPL_SHIM_H

#ifndef _MSC_VER

#include <functional>

namespace concurrency {

template <typename Index, typename Function>
inline void parallel_for(Index first, Index last, const Function& f) {
#ifdef _OPENMP
    #pragma omp parallel for schedule(dynamic)
    for (Index i = first; i < last; ++i)
        f(i);
#else
    for (Index i = first; i < last; ++i)
        f(i);
#endif
}

} // namespace concurrency

using concurrency::parallel_for;

#endif // !_MSC_VER
#endif // PPL_SHIM_H
EOF
}

# ── argument parsing ──────────────────────────────────────────────────────────

FORCE=0
if [ "${1:-}" = "-f" ]; then
  FORCE=1
  shift
fi

[ $# -eq 1 ] || die "Usage: $0 [-f] <PROJECT>"
PROJECT="$1"

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONF_FILE="$ROOT/scripts/gen_cmake.conf"
if [ -f "$CONF_FILE" ]; then
  # shellcheck source=/dev/null
  source "$CONF_FILE"
fi
SRC_DIR="$ROOT/$PROJECT/src"
[ -d "$SRC_DIR" ] || die "src/ not found: $SRC_DIR"

VCXPROJ=$(find "$SRC_DIR" -maxdepth 1 -name "*.vcxproj" | head -1)
[ -n "$VCXPROJ" ] || die "No .vcxproj found in $SRC_DIR"

OUT="$SRC_DIR/CMakeLists.txt"

# ── overwrite guard ───────────────────────────────────────────────────────────

if [ -f "$OUT" ] && [ "$FORCE" -eq 0 ]; then
  printf "CMakeLists.txt already exists at %s.\nOverwrite? [y/N] " "$OUT"
  read -r answer
  case "$answer" in
    [yY]*) ;;
    *) echo "Aborted."; exit 0 ;;
  esac
fi

# ── parse .vcxproj ────────────────────────────────────────────────────────────

TARGET=$(xml_val "RootNamespace" "$VCXPROJ")
[ -n "$TARGET" ] || TARGET=$(basename "$VCXPROJ" .vcxproj)

LANG_RAW=$(xml_val "LanguageStandard" "$VCXPROJ")
case "$LANG_RAW" in
  stdcpp14) CXX_STD=14 ;;
  stdcpp17) CXX_STD=17 ;;
  stdcpp20) CXX_STD=20 ;;
  stdcpp23) CXX_STD=23 ;;
  *)        CXX_STD=17 ;;  # sensible default
esac

mapfile -t SOURCES < <(cpp_sources "$VCXPROJ")
[ ${#SOURCES[@]} -gt 0 ] || die "No <ClCompile> sources found in $VCXPROJ"

SDL2=0
if uses_sdl2 "$SRC_DIR" "$VCXPROJ"; then
  SDL2=1
fi

PPL=0
if uses_ppl "$SRC_DIR"; then
  PPL=1
  PPL_FILE="$SRC_DIR/ppl.h"
  if [ ! -f "$PPL_FILE" ] || [ "$FORCE" -eq 1 ]; then
    emit_ppl_shim "$PPL_FILE"
    echo "Generated ppl.h shim: $PPL_FILE"
  fi
fi

# ── dependency checks ─────────────────────────────────────────────────────────

check_mingw
[ "$SDL2" -eq 0 ] || check_sdl2_mingw

# ── emit CMakeLists.txt ───────────────────────────────────────────────────────

{
  echo "# Auto-generated by scripts/gen_cmake.sh from $(basename "$VCXPROJ")"
  echo "# Cross-compile to Windows x64:  cmake -DCMAKE_TOOLCHAIN_FILE=<repo>/scripts/mingw-toolchain.cmake -S . -B build"
  echo ""
  echo "cmake_minimum_required(VERSION 3.20)"
  echo ""
  echo "# Auto-set MinGW toolchain for Windows cross-compilation if not passed explicitly"
  echo "if(NOT CMAKE_TOOLCHAIN_FILE AND NOT WIN32)"
  echo "  get_filename_component(_DEFAULT_TOOLCHAIN \"\${CMAKE_CURRENT_LIST_DIR}/../../scripts/mingw-toolchain.cmake\" ABSOLUTE)"
  echo "  if(EXISTS \"\${_DEFAULT_TOOLCHAIN}\")"
  echo "    set(CMAKE_TOOLCHAIN_FILE \"\${_DEFAULT_TOOLCHAIN}\" CACHE FILEPATH \"MinGW toolchain\" FORCE)"
  echo "  endif()"
  echo "endif()"
  echo ""
  echo "project($TARGET)"
  echo ""
  echo "set(CMAKE_CXX_STANDARD $CXX_STD)"
  echo "set(CMAKE_CXX_STANDARD_REQUIRED ON)"
  echo ""
  echo "add_executable($TARGET"
  for src in "${SOURCES[@]}"; do
    echo "    $src"
  done
  echo ")"
  echo ""
  echo "# src/ on include path: lets shims (ppl.h, etc.) placed here shadow system headers."
  echo "target_include_directories($TARGET PRIVATE \${CMAKE_CURRENT_SOURCE_DIR})"
  echo ""
  if [ "$SDL2" -eq 1 ]; then
    echo "# SDL2 via mingw-w64 system package"
    if [ -n "${SDL2_PREFIX:-}" ]; then
      echo "list(APPEND CMAKE_PREFIX_PATH \"${SDL2_PREFIX}\")"
    fi
    echo "# find_package(SDL2) has no config file in the mingw package; use find_path/find_library instead."
    echo "find_path(SDL2_INCLUDE_DIR NAMES SDL.h PATH_SUFFIXES SDL2)"
    echo "find_library(SDL2_LIB  NAMES SDL2)"
    echo "find_library(SDL2MAIN_LIB NAMES SDL2main)"
    echo "if(NOT SDL2_INCLUDE_DIR OR NOT SDL2_LIB)"
    echo "  message(FATAL_ERROR \"SDL2 not found — install libsdl2-mingw-dev (Debian/Ubuntu) or equivalent.\")"
    echo "endif()"
    echo "target_include_directories($TARGET PRIVATE \${SDL2_INCLUDE_DIR})"
    echo "# --whole-archive ensures SDL2main's WinMain is always linked (ld won't pull it lazily)."
    echo "target_link_libraries($TARGET PRIVATE"
    echo "    -Wl,--whole-archive \${SDL2MAIN_LIB} -Wl,--no-whole-archive"
    echo "    \${SDL2_LIB})"
    echo ""
    echo "# Copy SDL2.dll to output directory alongside the executable"
    echo "find_file(SDL2_DLL NAMES SDL2.dll PATH_SUFFIXES bin)"
    echo "if(SDL2_DLL)"
    echo "  add_custom_command(TARGET $TARGET POST_BUILD"
    echo "    COMMAND \${CMAKE_COMMAND} -E copy_if_different"
    echo "    \"\${SDL2_DLL}\""
    echo "    \"\$<TARGET_FILE_DIR:$TARGET>\""
    echo "    COMMENT \"Copying SDL2.dll next to binary\""
    echo "  )"
    echo "endif()"
  fi
  if [ "$PPL" -eq 1 ]; then
    echo ""
    echo "# OpenMP — needed by the ppl.h shim (concurrency::parallel_for)"
    echo "find_package(OpenMP)"
    echo "if(OpenMP_CXX_FOUND)"
    echo "  target_link_libraries($TARGET PRIVATE OpenMP::OpenMP_CXX)"
    echo "endif()"
  fi
} > "$OUT"


echo "Written: $OUT"
echo ""
echo "To build (from repo root):"
echo "  cmake -S $PROJECT/src -B /tmp/build_$PROJECT \\"
echo "        -DCMAKE_TOOLCHAIN_FILE=$(realpath "$ROOT/scripts/mingw-toolchain.cmake") \\"
echo "        -DCMAKE_BUILD_TYPE=Release"
echo "  cmake --build /tmp/build_$PROJECT"
echo ""
echo "Rider: Settings → Build → CMake → CMake options:"
echo "  -DCMAKE_TOOLCHAIN_FILE=\$(pwd)/scripts/mingw-toolchain.cmake"

