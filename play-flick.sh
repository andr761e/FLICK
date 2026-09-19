#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PROJECT="$ROOT/FLICK.uproject"
ENGINE_ROOT="${FLICK_UNREAL_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.6}"
BUILD="$ENGINE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh"
EDITOR="$ENGINE_ROOT/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"

[[ -x "$BUILD" ]] || { echo "Unreal Build Tool not found: $BUILD"; exit 1; }
[[ -x "$EDITOR" ]] || { echo "Unreal Editor not found: $EDITOR"; exit 1; }

"$BUILD" FLICKEditor Mac Development "$PROJECT" -waitmutex -NoHotReload
if (($#)); then
  "$EDITOR" "$PROJECT" -game -log "$@" &
else
  "$EDITOR" "$PROJECT" -game -fullscreen -log &
fi

