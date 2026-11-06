# Doomsday Clock untuk Windows

Hitung mundur Doomsday langsung di atas wallpaper kamu, dengan pilihan widget yang tetap terlihat saat membuka aplikasi lain. Port ini mengikuti revisi terakhir [versi macOS](../README.md).

## Cara pakai

1. Pilih **Doomsday-Clock-Windows-x64.zip** untuk komputer Intel/AMD, atau **Doomsday-Clock-Windows-arm64.zip** untuk Windows ARM.
2. Ekstrak ZIP, lalu buka **Doomsday Clock.exe**.
3. Tampilkan desktop untuk melihat countdown. Klik ikon jam **DOOM** di dekat jam taskbar untuk membuka menu. Ikon bisa berada di menu **^**.

Target sistemnya **Windows 10 versi 1809 atau lebih baru**, termasuk Windows 11. Aplikasi portable; tidak membutuhkan .NET, Python, atau instalasi tambahan. Untuk menghapusnya, keluar dari menu DOOM lalu hapus folder aplikasi.

## Tampilan dan pengaturan

- **Countdown di desktop:** teks transparan di desktop utama, di belakang ikon. Wallpaper yang sudah kamu pakai tetap terlihat, tanpa latar atau bar hitam tambahan.
- **Widget mengambang:** countdown berada di atas jendela aplikasi lain. Seret tulisannya untuk memindahkan posisi; klik kanan tulisannya untuk membuka menu.
- **Pratinjau besar:** tampilan dengan latar gelap. Tekan **F11** untuk layar penuh dan **Esc** untuk kembali.
- **Ubah tanggal countdown:** pilih tanggal dan jam dalam **WITA (UTC+8)**. Target awalnya **16 Desember 2026 pukul 00.00 WITA**; zona waktu komputer tidak mengubah target.
- **Kembalikan posisi widget:** memunculkan kembali widget di posisi yang mudah dijangkau.

Judul **DOOMSDAY IS COMING** dan angka berwarna putih, memakai font tebal yang sama. Tinggi angka **70% dari tinggi judul**, dan teks desktop dinaikkan agar tidak tertutup taskbar. Angka menampilkan bulan kalender, hari, jam, menit, dan detik. Setelah waktunya tiba, angka berhenti di nol.

**Font:** aplikasi memakai **Avenir Next Bold** jika sudah terpasang, lalu mencoba Avenir Next LT Pro. Jika keduanya tidak ada, aplikasi memakai **Segoe UI Bold**. Avenir tidak disertakan dalam distribusi ini; untuk tampilan font yang identik dengan Mac, gunakan instalasi Avenir Next untuk Windows yang kamu miliki.

Pengaturan tersimpan untuk pengguna Windows saat ini di `HKEY_CURRENT_USER\Software\DoomsdayClock`. Aplikasi berjalan lokal dan tidak mengirim data atau memutar livestream. Membuka aplikasi dua kali tidak membuat dua countdown.

## Catatan build awal

File EXE ini dikompilasi dari macOS untuk x64 dan ARM64. Tes logika countdown dan perbandingan terhadap kalender versi macOS telah dijalankan; tampilan, DPI, menu, dan integrasi Explorer **belum diuji langsung di Windows**. Build belum ditandatangani digital.

Jika countdown desktop tidak muncul, gunakan **Widget mengambang**. Integrasi desktop bergantung pada susunan jendela Explorer; aplikasi mencoba memulihkannya saat Explorer dimulai ulang atau monitor berubah. Saat inisialisasi desktop gagal terdeteksi, widget dibuka otomatis.

## Build di Windows

Unduh kode sumber dari repositori ini dan jalankan perintah berikut dari folder `windows`.

Pasang Visual Studio 2022 Build Tools dengan **Desktop development with C++** dan Windows SDK. Buka **Developer PowerShell for VS 2022** dengan target x64, masuk ke folder sumber ini, lalu jalankan:

```powershell
./build.ps1 -Architecture x64
```

Untuk ARM64, aktifkan compiler target ARM64 lalu jalankan `./build.ps1 -Architecture arm64`. Skrip membuat EXE dan ZIP di `dist`. Tes runtime ARM64 hanya dijalankan pada host ARM64. Build x64 juga menjalankan tes countdown dan tes render transparansi/font lewat `--self-test`.

## Build dari macOS atau Linux

Gunakan Python 3.9+ dan [Zig 0.15.2](https://ziglang.org/download/). Skrip tidak mengunduh atau memasang compiler otomatis.

```sh
python3 build.py --zig /path/to/zig --arch all
```

Ganti `all` dengan `x64` atau `arm64` untuk satu arsitektur. Seluruh library C++ ditautkan statis; dependensi runtime yang tersisa berasal dari Windows.

Tes logika bisa dijalankan di macOS/Linux:

```sh
c++ -std=c++17 -O2 tests/countdown_test.cpp -o /tmp/doomsday-countdown-test
/tmp/doomsday-countdown-test
# Opsional, pada Mac: bandingkan dengan kalender versi macOS.
swift tests/reference.swift > /tmp/doomsday-calendar-reference.txt
/tmp/doomsday-countdown-test /tmp/doomsday-calendar-reference.txt
```

Workflow [Build Windows](../.github/workflows/build-windows.yml) menjalankan build dan tes Windows x64. Sebelum rilis stabil, lakukan pemeriksaan di [TESTING.md](TESTING.md).

## Catatan implementasi

Transparansi memakai [layered windows dan alpha per pixel](https://learn.microsoft.com/en-us/windows/win32/winmsg/window-features). Penempatan desktop menangani struktur Explorer lama dan perubahan Windows 11 yang [dijelaskan pengembang Lively](https://github.com/lively-community/lively/issues/2074). Aplikasi tidak mengganti berkas wallpaper atau menyembunyikan ikon desktop.

Doomsday Clock adalah proyek penggemar, terinspirasi [livestream Marvel Entertainment](https://www.youtube.com/watch?v=f17J3AXVK5w).
