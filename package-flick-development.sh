#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PROJECT="$ROOT/FLICK.uproject"
ENGINE_ROOT="${FLICK_UNREAL_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.6}"
UAT="$ENGINE_ROOT/Engine/Build/BatchFiles/RunUAT.sh"
OUTPUT="$ROOT/Builds/Development"
APP_ID="$ROOT/Build/Steam/steam_appid.txt"

[[ -x "$UAT" ]] || { echo "Unreal Automation Tool not found: $UAT"; exit 1; }
[[ -f "$APP_ID" ]] || { echo "Steam test App ID not found: $APP_ID"; exit 1; }

"$UAT" BuildCookRun -project="$PROJECT" -noP4 -platform=Mac \
  -clientconfig=Development -build -cook -map=/Engine/Maps/Templates/OpenWorld \
  -stage -pak -archive -archivedirectory="$OUTPUT" -utf8output

PACKAGE="$OUTPUT/Mac"
[[ -d "$PACKAGE" ]] || { echo "Expected Mac package not found: $PACKAGE"; exit 1; }
cp "$APP_ID" "$PACKAGE/steam_appid.txt"
APP_BUNDLE="$(find "$PACKAGE" -maxdepth 2 -name 'FLICK.app' -type d -print -quit)"
if [[ -n "$APP_BUNDLE" ]]; then
  mkdir -p "$APP_BUNDLE/Contents/MacOS"
  cp "$APP_ID" "$APP_BUNDLE/Contents/MacOS/steam_appid.txt"
fi
echo "FLICK Mac Development package is ready: $PACKAGE"
