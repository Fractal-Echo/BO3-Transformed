#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
GAME_DIR=${BO3_GAME_DIR:-}
WINE_BIN=${WINE_BIN:-wine}
EXE_NAME=${BO3_EXE_NAME:-BlackOps3.exe}

if [ -z "$GAME_DIR" ]; then
  echo "BO3_GAME_DIR is required." >&2
  exit 2
fi

if [ ! -f "$GAME_DIR/$EXE_NAME" ]; then
  echo "Game executable not found: $GAME_DIR/$EXE_NAME" >&2
  exit 2
fi

mkdir -p "$SCRIPT_DIR/logs"

export DXVK_CONFIG_FILE="$SCRIPT_DIR/dxvk.conf"
export DXVK_LOG_PATH="$SCRIPT_DIR/logs"
export DXVK_HUD=${DXVK_HUD:-version,fps,frametimes,compiler}
export WINEDLLOVERRIDES=${WINEDLLOVERRIDES:-d3d11,dxgi=n,b}

cd "$GAME_DIR"
exec "$WINE_BIN" "$EXE_NAME"

