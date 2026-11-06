#!/bin/bash
set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
mkdir -p dist
scratch=$(mktemp -d "${TMPDIR:-/tmp}/doomsday-build.XXXXXX")
trap 'rm -rf -- "$scratch"' EXIT
app="$scratch/Doomsday Clock.app"
mkdir -p "$app/Contents/MacOS"
cp Info.plist "$app/Contents/Info.plist"

for arch in arm64 x86_64; do
    xcrun --sdk macosx swiftc \
        -target "$arch-apple-macosx14.0" \
        -module-cache-path "$scratch/module-cache" \
        -O -assert-config Debug DoomsdayClock.swift -o "$scratch/DoomsdayClock-$arch"
done
xcrun lipo -create "$scratch/DoomsdayClock-arm64" "$scratch/DoomsdayClock-x86_64" \
    -output "$app/Contents/MacOS/DoomsdayClock"
/usr/bin/codesign --force --sign - "$app"
/usr/bin/codesign --verify --strict "$app"
"$app/Contents/MacOS/DoomsdayClock" --self-test
/usr/bin/ditto "$app" 'dist/Doomsday Clock.app'
/usr/bin/ditto -c -k --keepParent "$app" "$scratch/Doomsday-Clock-macOS.zip"
mv -f "$scratch/Doomsday-Clock-macOS.zip" 'dist/Doomsday-Clock-macOS.zip'
printf '\nApp: dist/Doomsday Clock.app\nZIP: dist/Doomsday-Clock-macOS.zip\n'
