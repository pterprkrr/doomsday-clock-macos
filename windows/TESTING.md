# Verifikasi versi Windows

## Sudah dijalankan pada Mac pembuat port

- 5.170 pemeriksaan logika kalender, WITA, batas detik, kedaluwarsa, tahun kabisat, tanggal, dan batas lebar layout; termasuk perbandingan dengan Foundation Calendar dari versi macOS.
- Kompilasi silang ke Windows x64 dan ARM64.
- Pemeriksaan arsitektur, subsystem GUI, resource ikon/manifest, dan DLL yang diimpor oleh EXE.

## Disiapkan untuk runner Windows; belum dijalankan

- Build MSVC, tes logika native, dan `Doomsday Clock.exe --self-test` lewat `build.ps1` / GitHub Actions.
- Tes render memeriksa sudut transparan, adanya glyph putih, repaint saat kedaluwarsa, dan rasio tinggi glyph angka 70% dari judul.

## Pemeriksaan manual sebelum rilis stabil

1. Windows 10 dan Windows 11: jalankan EXE, tekan Win+D. Wallpaper tetap terlihat, angka/judul putih, tanpa bar hitam. Ikon desktop tetap bisa diklik.
2. Buka aplikasi lain: countdown desktop tetap di belakang aplikasi. Aktifkan widget: widget berada di atas aplikasi dan bisa diseret melalui tulisannya.
3. Klik ikon tray (termasuk dari menu ^), ubah tanggal/jam, batal, simpan, lalu reset tanggal. Uji juga target yang sudah lewat.
4. Ganti zona waktu komputer: target harus tetap 16 Desember 2026 00.00 WITA. Uji sleep/resume dan pergantian menit.
5. Keluar dan buka lagi: tanggal, pilihan tampilan, dan posisi widget harus tersimpan. Buka EXE lagi ketika sudah berjalan: hanya satu proses aktif.
6. Uji skala layar 100%, 150%, dan 200%, monitor kedua, pergantian monitor utama, cabut monitor, dan taskbar di posisi berbeda. Semua teks harus terjangkau.
7. Restart Windows Explorer: ikon tray dan countdown desktop harus pulih. Uji wallpaper slideshow, virtual desktop, dan sembunyikan/tampilkan ikon desktop.
8. Pratinjau: ubah ukuran, F11, Esc, tutup, dan buka lagi. Tutup aplikasi dari menu DOOM: semua jendela hilang.
9. Uji tanpa Avenir (Segoe UI Bold) dan dengan Avenir Next Bold terpasang. Periksa angka tetap lebih kecil daripada judul.
10. Uji EXE ARM64 langsung pada Windows ARM; kompilasi silang saja tidak membuktikan perilaku runtime.
