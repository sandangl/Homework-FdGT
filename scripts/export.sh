#!/usr/bin/env bash
# export.sh — package a project for delivery
# Usage: ./scripts/export.sh <project_name> <archive_name>
# Example: ./scripts/export.sh I Camera_delivery
set -euo pipefail

if [ $# -ne 2 ]; then
  echo "Usage: $0 <project_name> <archive_name>" >&2
  exit 1
fi

PROJECT="$1"
ARCHIVE="$2"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PROJECT_DIR="$ROOT/$PROJECT"
TMP="$ROOT/scripts/outputs/tmp_$PROJECT"
OUT="$ROOT/scripts/outputs/$ARCHIVE.zip"

if [ ! -d "$PROJECT_DIR" ]; then
  echo "ERROR: project directory not found: $PROJECT_DIR" >&2
  exit 1
fi

# Clean any previous tmp run
rm -rf "$TMP"
mkdir -p "$ROOT/scripts/outputs" "$TMP/screenshot" "$TMP/video" "$TMP/eseguibili"

# 1. src/ — filtered by .deliverignore (root and/or project-level)
SRC_DIR="$PROJECT_DIR/src"
EXCLUDES=()
[ -f "$ROOT/.deliverignore" ] && EXCLUDES+=(--exclude-from="$ROOT/.deliverignore")
[ -f "$PROJECT_DIR/.deliverignore" ] && EXCLUDES+=(--exclude-from="$PROJECT_DIR/.deliverignore")

if [ ${#EXCLUDES[@]} -gt 0 ]; then
  rsync -a "${EXCLUDES[@]}" "$SRC_DIR/" "$TMP/src/"
else
  echo "WARNING: no .deliverignore found at $ROOT/.deliverignore or $PROJECT_DIR/.deliverignore — copying src/ unfiltered"
  rsync -a "$SRC_DIR/" "$TMP/src/"
fi

# 2. deliverables: images → screenshot/
find "$PROJECT_DIR/deliverables" -maxdepth 1 \
  \( -iname "*.bmp" -o -iname "*.jpg" -o -iname "*.jpeg" \
     -o -iname "*.png" -o -iname "*.gif" -o -iname "*.tiff" \
     -o -iname "*.webp" \) \
  -exec cp {} "$TMP/screenshot/" \;

# 3. deliverables: video → video/
find "$PROJECT_DIR/deliverables" -maxdepth 1 \
  \( -iname "*.mp4" -o -iname "*.mov" -o -iname "*.mpeg" \
     -o -iname "*.avi" -o -iname "*.mkv" \) \
  -exec cp {} "$TMP/video/" \;

# 4. deliverables: html + README.txt → root
find "$PROJECT_DIR/deliverables" -maxdepth 1 -iname "*.html" \
  -exec cp {} "$TMP/" \;
[ -f "$PROJECT_DIR/deliverables/README.txt" ] && \
  cp "$PROJECT_DIR/deliverables/README.txt" "$TMP/"

# 5. eseguibili — Release/x64 of the .sln found in src/
SLN=$(find "$SRC_DIR" -maxdepth 1 -name "*.sln" | head -1)
if [ -z "$SLN" ]; then
  echo "ERROR: no .sln file found in $SRC_DIR" >&2
  exit 1
fi
SLN_STEM=$(basename "$SLN" .sln)
EXEC_SRC="$SRC_DIR/$SLN_STEM/Release/x64"
if [ -d "$EXEC_SRC" ]; then
  cp -r "$EXEC_SRC/." "$TMP/eseguibili/"
else
  echo "WARNING: $EXEC_SRC not found — eseguibili/ will be empty"
fi

# 6. zip (paths relative to tmp root, no top-level folder)
(cd "$TMP" && zip -r "$OUT" .)

# 7. cleanup
rm -rf "$TMP"

echo "Done: $OUT"

