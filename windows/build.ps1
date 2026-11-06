param([ValidateSet('x64','arm64')][string]$Architecture = 'x64')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Push-Location $PSScriptRoot
try {
    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        throw 'Buka Developer PowerShell for VS 2022 dengan C++ Build Tools dan Windows SDK, lalu jalankan build.ps1 lagi.'
    }
    if ($env:VSCMD_ARG_TGT_ARCH -ne $Architecture) {
        throw "Aktifkan compiler target $Architecture di Developer PowerShell terlebih dahulu."
    }
    New-Item -ItemType Directory -Force 'build' | Out-Null
    $folder = "dist/Doomsday-Clock-Windows-$Architecture"
    New-Item -ItemType Directory -Force $folder | Out-Null
    & rc.exe /nologo /fo build/app.res app.rc
    if ($LASTEXITCODE -ne 0) { throw 'Resource build gagal.' }
    & cl.exe /nologo /std:c++17 /EHsc /O2 /MT /W4 /utf-8 /D_CRT_SECURE_NO_WARNINGS /Fobuild/main.obj "src/main.cpp" "build/app.res" "/Fe$folder/Doomsday Clock.exe" /link /SUBSYSTEM:WINDOWS /MANIFEST:NO gdiplus.lib comctl32.lib shell32.lib advapi32.lib gdi32.lib user32.lib
    if ($LASTEXITCODE -ne 0) { throw 'Build aplikasi gagal.' }
    & cl.exe /nologo /std:c++17 /EHsc /O2 /MT /W4 /utf-8 /Fobuild/countdown_test.obj tests/countdown_test.cpp /Febuild/countdown_test.exe
    if ($LASTEXITCODE -ne 0) { throw 'Build pengujian gagal.' }
    # ARM64 binaries require an ARM64 host; never report a cross-compiled test as executed.
    $nativeArm = $env:PROCESSOR_ARCHITECTURE -eq 'ARM64' -or $env:PROCESSOR_ARCHITEW6432 -eq 'ARM64'
    if ($Architecture -eq 'x64' -or $nativeArm) {
        & ./build/countdown_test.exe
        if ($LASTEXITCODE -ne 0) { throw 'Pengujian countdown gagal.' }
        $test = Start-Process -FilePath "$folder/Doomsday Clock.exe" -ArgumentList '--self-test' -Wait -PassThru
        if ($test.ExitCode -ne 0) { throw 'Pengujian transparansi dan ukuran font gagal.' }
    } else {
        Write-Host 'Pengujian runtime ARM64 dilewati: host bukan ARM64.'
    }
    Copy-Item BACA-DULU.txt, README.md, TESTING.md $folder
    Compress-Archive -Path $folder -DestinationPath "$folder.zip" -Force
    Write-Host "Selesai: $folder.zip"
} finally {
    Pop-Location
}
