"""Cross-compile with Zig 0.15.2. No compiler download or system install is performed."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument('--zig', default='zig', help='Path to the Zig 0.15.2 executable')
parser.add_argument('--arch', choices=('x64', 'arm64', 'all'), default='all')
args = parser.parse_args()
root = Path(__file__).resolve().parent
zig = str(Path(args.zig).resolve()) if Path(args.zig).is_file() else args.zig
build = root / 'build'
build.mkdir(exist_ok=True)
env = os.environ.copy()
env['ZIG_GLOBAL_CACHE_DIR'] = str(build / 'zig-cache')
env['ZIG_LOCAL_CACHE_DIR'] = str(build / 'zig-local-cache')


def run(*command):
    subprocess.run(command, cwd=root, env=env, check=True)


version = subprocess.check_output([zig, 'version'], env=env, text=True).strip()
if version != '0.15.2':
    raise SystemExit(f'Expected Zig 0.15.2, found {version}. Use the tested compiler version.')
resource = build / 'app.res'
run(sys.executable, 'build_resources.py', str(resource))
for arch in ('x64', 'arm64') if args.arch == 'all' else (args.arch,):
    target = 'x86_64-windows-gnu' if arch == 'x64' else 'aarch64-windows-gnu'
    folder = root / 'dist' / f'Doomsday-Clock-Windows-{arch}'
    folder.mkdir(parents=True, exist_ok=True)
    run(zig, 'c++', '-target', target, '-std=c++17', '-O2', '-Wall', '-Wextra',
        '-Wno-missing-field-initializers', '-municode', '-Wl,--subsystem,windows',
        '-static', '-s', 'src/main.cpp', str(resource), '-o', str(folder / 'Doomsday Clock.exe'),
        '-lgdiplus', '-lcomctl32', '-lshell32', '-ladvapi32', '-lgdi32', '-luser32')
    files = [folder / 'Doomsday Clock.exe']
    for name in ('BACA-DULU.txt', 'README.md', 'TESTING.md'):
        shutil.copy2(root / name, folder)
        files.append(folder / name)
    archive = folder.with_suffix('.zip')
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as output:
        for file in files:
            output.write(file, folder.name + '/' + file.name)
    print(f'Built: {archive}', flush=True)
