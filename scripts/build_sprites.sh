#!/usr/bin/env bash
#
# Build the game's sprite sheet from .aseprite source files.
#
# Pipeline:
#   1. For each res/aseprite/*.aseprite, export every frame as an individual PNG
#      named "{title}_{tag}_{tagframe}.png" into a temp directory.
#   2. Run TexturePacker on the temp directory to pack all frames into a single
#      sheet at res/spritesheets/sprites.{png,json}.
#
# Overridable env vars:
#   SRC_DIR   default: <repo>/res/aseprite
#   OUT_DIR   default: <repo>/res/spritesheets
#   OUT_NAME  default: sprites          (yields sprites.png + sprites.json)
#   MAX_SIZE  default: 4096
#
# Requires: aseprite, TexturePacker on PATH.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

SRC_DIR="${SRC_DIR:-$REPO_DIR/res/aseprite}"
OUT_DIR="${OUT_DIR:-$REPO_DIR/res/spritesheets}"
OUT_NAME="${OUT_NAME:-sprites}"
MAX_SIZE="${MAX_SIZE:-4096}"

command -v aseprite      >/dev/null || { echo "aseprite not found on PATH" >&2; exit 1; }
command -v TexturePacker >/dev/null || { echo "TexturePacker not found on PATH" >&2; exit 1; }

[ -d "$SRC_DIR" ] || { echo "source dir does not exist: $SRC_DIR" >&2; exit 1; }

shopt -s nullglob
ase_files=("$SRC_DIR"/*.aseprite "$SRC_DIR"/*.ase)
if [ "${#ase_files[@]}" -eq 0 ]; then
    echo "no .aseprite / .ase files found in $SRC_DIR" >&2
    exit 1
fi

FRAMES_DIR="$(mktemp -d -t fg_frames.XXXXXX)"
trap 'rm -rf "$FRAMES_DIR"' EXIT

echo "exporting frames -> $FRAMES_DIR"
for ase in "${ase_files[@]}"; do
    echo "  $(basename "$ase")"
    # Sprites in this project are organized by aseprite layers (one leaf
    # layer = one sprite). The Lua script renders each leaf layer's frames
    # to "<layer>_<frame>.png" in $FRAMES_DIR, skipping group layers.
    aseprite \
        --batch \
        "$ase" \
        --script-param "outdir=$FRAMES_DIR" \
        --script "$SCRIPT_DIR/export_leaf_frames.lua"
done

frame_count=$(find "$FRAMES_DIR" -maxdepth 1 -type f -name '*.png' | wc -l)
if [ "$frame_count" -eq 0 ]; then
    echo "aseprite produced no frames" >&2
    exit 1
fi
echo "exported $frame_count frame(s)"

mkdir -p "$OUT_DIR"

echo "packing -> $OUT_DIR/$OUT_NAME.png"
TexturePacker \
    --sheet "$OUT_DIR/$OUT_NAME.png" \
    --data  "$OUT_DIR/$OUT_NAME.json" \
    --format phaser-json-array \
    --algorithm MaxRects \
    --pack-mode Best \
    --trim-mode None \
    --disable-rotation \
    --extrude 0 \
    --padding 0 \
    --max-size "$MAX_SIZE" \
    --trim-sprite-names \
    "$FRAMES_DIR"

echo "done."
