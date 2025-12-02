Deskripsi:
Program ini adalah permainan catur lengkap dalam bahasa C dengan fitur:
  ~Castling (king-side & queen-side)
  ~En-passant
  ~Promosi interaktif (Q/R/B/N)
  ~Check / checkmate detection
  ~History, halfmove & fullmove number
  ~Pemilihan warna secara acak dan menampilkan giliran putih/hitam
  ~Bisa keluar kapan saja menggunakan perintah exit
  ~Program ini cocok untuk dimainkan oleh 2 pemain (PvP) atau pemain vs komputer (PvC).

Persiapan Software
  Sebelum menjalankan program, pastikan Anda sudah memiliki software berikut:
  ~Code::Blocks (IDE untuk bahasa C/C++)
    ->Website: https://www.codeblocks.org/downloads
    ->Pilih versi yang sudah include MinGW (compiler) agar langsung bisa menjalankan C.
  ~Git (untuk mengambil kode dari GitHub)
    ->Website: https://git-scm.com/downloads
    ->Bisa juga menggunakan Git Bash yang sudah terinstal bersamaan dengan Git.

Cara Mengambil Kode dari GitHub
  ->Buka repository GitHub kami (contoh link):
    https://github.com/username/nama-repo-catur
  ->Klik tombol Code → pilih HTTPS → salin URL.
    Contoh: https://github.com/username/nama-repo-catur.git
  ->Buka terminal / Git Bash, lalu jalankan perintah:
  ->git clone https://github.com/username/nama-repo-catur.git
  ->Masuk ke folder hasil clone:
  ->cd nama-repo-catur
  ->Sekarang semua file program sudah ada di komputer Anda.

Cara Menjalankan Program di Code::Blocks
  ->Buka Code::Blocks → pilih File → Open…
  ->Cari file catur_menu_final_v3.c di folder yang sudah di-clone → buka file tersebut.
  ->Jika belum ada project Code::Blocks, bisa buat project baru:
  ->Pilih File → New → Project → Console Application → C
  ->Beri nama project dan arahkan ke folder nama-repo-catur
  ->Tambahkan file catur_menu_final_v3.c ke project
  ->Setelah file terbuka di Code::Blocks, klik Build → Build and Run (atau tekan F9).
  ->Program akan berjalan di console, dan akan menampilkan papan catur.
  ->Masukkan langkah sesuai format:
    Contoh langkah: e2 e4 atau e2e4
    Castling: O-O (king-side) atau O-O-O (queen-side)
    Keluar dari permainan: ketik exit
