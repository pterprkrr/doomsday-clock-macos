"""Write standard Win32 .res records without needing a host-native resource compiler.

The Windows/MSVC build uses app.rc; cross-builds embed the same icon and manifest.
Format: https://learn.microsoft.com/windows/win32/menurc/resource-file-formats
"""
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def padded(value):
    return value + bytes((-len(value)) % 4)


def resource(kind, name, data, language=0x409):
    header = struct.pack('<IIHHHHIHHII', len(data), 32, 0xFFFF, kind,
                         0xFFFF, name, 0, 0x1030, language, 0, 0)
    return header + padded(data)


def wide(text):
    return (text + '\0').encode('utf-16le')


def block(key, value=b'', children=b'', text=False):
    value_length = len(value) // 2 if text else len(value)
    body = padded(struct.pack('<HHH', 0, value_length, int(text)) + wide(key))
    body += value
    if children:
        body = padded(body) + children
    return struct.pack('<H', len(body)) + body[2:]


def version_info():
    strings = {
        'FileDescription': 'Doomsday Clock for Windows', 'FileVersion': '1.0.0',
        'ProductName': 'Doomsday Clock', 'ProductVersion': '1.0.0',
        'OriginalFilename': 'Doomsday Clock.exe',
    }
    entries = b''.join(padded(block(k, wide(v), text=True)) for k, v in strings.items())
    string_table = block('040904B0', children=entries, text=True)
    string_info = block('StringFileInfo', children=string_table, text=True)
    translation = block('Translation', struct.pack('<HH', 0x409, 1200))
    var_info = block('VarFileInfo', children=translation, text=True)
    fixed = struct.pack('<13I', 0xFEEF04BD, 0x10000, 0x10000, 0, 0x10000, 0,
                        0x3F, 0, 0x40004, 1, 0, 0, 0)
    return block('VS_VERSION_INFO', fixed, padded(string_info) + var_info)


icon = (ROOT / 'app.ico').read_bytes()
reserved, kind, count = struct.unpack_from('<HHH', icon)
assert reserved == 0 and kind == 1
records = [resource(0, 0, b'', 0)]
group = bytearray(struct.pack('<HHH', 0, 1, count))
for i in range(count):
    w, h, colors, reserved, planes, depth, size, offset = struct.unpack_from('<BBBBHHII', icon, 6 + i * 16)
    assert offset + size <= len(icon)
    image_id = i + 11
    group.extend(struct.pack('<BBBBHHIH', w, h, colors, reserved, planes, depth, size, image_id))
    records.append(resource(3, image_id, icon[offset:offset + size]))
records.append(resource(14, 101, group))
records.append(resource(24, 1, (ROOT / 'app.manifest').read_bytes()))
records.append(resource(16, 1, version_info()))
destination = Path(sys.argv[1])
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_bytes(b''.join(records))
