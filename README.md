# Doomsday Clock

Hitung mundur Avengers: Doomsday langsung di atas wallpaper Mac atau PC Windows kamu. Aktifkan widget mengambang kalau ingin countdown tetap terlihat saat membuka aplikasi lain.

Target awalnya **16 Desember 2026 pukul 00.00 WITA (UTC+8)**. Tanggal dan jam bisa diubah lewat menu **DOOM**.

![Pratinjau Doomsday Clock di macOS](preview.png)

## Pemasangan

Pilih ZIP yang sesuai dari [halaman unduhan](https://github.com/pterprkrr/doomsday-clock-macos/releases/latest), lalu ekstrak.

| Perangkat | Pilih file |
| --- | --- |
| Mac Apple Silicon atau Intel dengan macOS 14 Sonoma ke atas | `Doomsday-Clock-macOS.zip` |
| PC Windows 10 atau 11 dengan prosesor Intel atau AMD 64-bit | `Doomsday-Clock-Windows-x64.zip` |
| PC Windows 10 atau 11 berbasis ARM | `Doomsday-Clock-Windows-arm64.zip` |

- **Mac** · Pindahkan **Doomsday Clock.app** ke Applications dan buka. Menu **DOOM** ada di menu bar atas. Jika macOS menahan aplikasi yang belum dinotariskan, ikuti [panduan Apple](https://support.apple.com/102445).
- **Windows** · Buka **Doomsday Clock.exe**. Ikon **DOOM** ada di dekat jam taskbar, kadang di balik panah **^**. Versi awal Windows menargetkan Windows 10 versi 1809 ke atas. Build belum ditandatangani digital, dan integrasi dengan desktop masih perlu diuji langsung.

## Penggunaan

Tampilkan desktop untuk melihat countdown di atas wallpaper yang sedang kamu pakai. Dari menu **DOOM**, kamu bisa menyalakan widget mengambang, membuka pratinjau besar, atau mengganti tanggal dan jam. Di Windows, seret tulisan widget untuk memindahkannya.

Judul dan angka berwarna putih. Tinggi angka 70% dari tinggi judul. Mac memakai Avenir Next Bold. Windows memakai font yang sama jika tersedia, atau Segoe UI Bold.

## Build dari kode sumber

**macOS** · Siapkan Xcode atau Command Line Tools, lalu jalankan dari folder utama repositori.

```sh
bash build.sh
open 'dist/Doomsday Clock.app'
```

**Windows** · Kode sumber ada di [folder windows](windows/). Panduan build dengan Visual Studio atau Zig ada di [README Windows](windows/README.md).

Proyek penggemar dari [pterprkrr](https://x.com/pterprkrr), terinspirasi [livestream Marvel Entertainment](https://www.youtube.com/watch?v=f17J3AXVK5w).
