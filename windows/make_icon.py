"""Generate the project's original clock icon; uses only Python's standard library."""
import math
import struct
from pathlib import Path


def segment_distance(x, y, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    t = max(0, min(1, ((x - ax) * dx + (y - ay) * dy) / (dx * dx + dy * dy)))
    return math.hypot(x - ax - t * dx, y - ay - t * dy)


def icon_image(size):
    pixels = bytearray()
    for py in reversed(range(size)):
        for px in range(size):
            accum = [0, 0, 0, 0]
            for sy in range(4):
                for sx in range(4):
                    x = (px + (sx + 0.5) / 4) / size * 64 - 32
                    y = (py + (sy + 0.5) / 4) / size * 64 - 32
                    radius = math.hypot(x, y)
                    ring = abs(radius - 24) < 2.3
                    hand = min(segment_distance(x, y, 0, 0, 0, -15),
                               segment_distance(x, y, 0, 0, 11, 7)) < 2.0
                    if ring or hand:
                        color = (255, 255, 255, 255)
                    elif radius < 29:
                        color = (25, 25, 25, 255)
                    else:
                        color = (0, 0, 0, 0)
                    for i, value in enumerate(color):
                        accum[i] += value
            pixels.extend(int(value / 16) for value in accum)
    header = struct.pack('<IIIHHIIIIII', 40, size, size * 2, 1, 32, 0, len(pixels), 0, 0, 0, 0)
    return header + pixels + bytes(((size + 31) // 32) * 4 * size)


sizes = (16, 24, 32, 48, 64, 128)
images = [icon_image(size) for size in sizes]
offset = 6 + 16 * len(sizes)
directory = bytearray(struct.pack('<HHH', 0, 1, len(sizes)))
for size, image in zip(sizes, images):
    directory.extend(struct.pack('<BBBBHHII', size, size, 0, 0, 1, 32, len(image), offset))
    offset += len(image)
Path(__file__).with_name('app.ico').write_bytes(directory + b''.join(images))
