#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PROJECT="$ROOT/FLICK.uproject"
ENGINE_ROOT="${FLICK_UNREAL_ENGINE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
UAT="$ENGINE_ROOT/Engine/Build/BatchFiles/RunUAT.sh"
OUTPUT="$ROOT/Builds/Development"
APP_ID="$ROOT/Build/Steam/steam_appid.txt"

[[ -x "$UAT" ]] || { echo "Unreal Automation Tool not found: $UAT"; exit 1; }
[[ -f "$APP_ID" ]] || { echo "Steam test App ID not found: $APP_ID"; exit 1; }

"$UAT" BuildCookRun -WaitForUATMutex -project="$PROJECT" -noP4 -platform=Mac \
  -clientconfig=Development -build -cook -map=/Engine/Maps/Templates/OpenWorld \
  -stage -pak -archive -archivedirectory="$OUTPUT" -utf8output

PACKAGE="$OUTPUT/Mac"
STAGED_APP="$ROOT/Saved/StagedBuilds/Mac/FLICK.app"
[[ -d "$PACKAGE" ]] || { echo "Expected Mac package not found: $PACKAGE"; exit 1; }
[[ -d "$STAGED_APP" ]] || { echo "Expected staged Mac app not found: $STAGED_APP"; exit 1; }

# UE's Mac archive step copies the bare app from Binaries/Mac. Replace it with
# the fully staged bundle, which includes the cooked UE payload.
rsync -a --delete "$STAGED_APP/" "$PACKAGE/FLICK.app/"

cp "$APP_ID" "$PACKAGE/steam_appid.txt"
APP_BUNDLE="$PACKAGE/FLICK.app"
MACOS_DIR="$APP_BUNDLE/Contents/MacOS"
ICU_SOURCE="$ENGINE_ROOT/Engine/Content/Internationalization"
ICU_DESTINATION="$APP_BUNDLE/Contents/UE/Engine/Content/Internationalization"
RUNTIME_LIBRARIES=(
  "$ROOT/Binaries/Mac/libtbb.12.dylib"
  "$ROOT/Binaries/Mac/libtbbmalloc.2.dylib"
  "$ENGINE_ROOT/Engine/Binaries/ThirdParty/Apple/MetalShaderConverter/Mac/libmetalirconverter.dylib"
)

[[ -d "$ICU_SOURCE" ]] || { echo "Unreal ICU data not found: $ICU_SOURCE"; exit 1; }
mkdir -p "$MACOS_DIR" "$ICU_DESTINATION"
cp "$APP_ID" "$MACOS_DIR/steam_appid.txt"
rsync -a "$ICU_SOURCE/" "$ICU_DESTINATION/"
for library in "${RUNTIME_LIBRARIES[@]}"; do
  [[ -f "$library" ]] || { echo "Required Mac runtime library not found: $library"; exit 1; }
  cp "$library" "$MACOS_DIR/"
done

codesign --force --deep --sign - "$APP_BUNDLE"
codesign --verify --deep --strict "$APP_BUNDLE"
echo "FLICK Mac Development package is ready: $PACKAGE"
