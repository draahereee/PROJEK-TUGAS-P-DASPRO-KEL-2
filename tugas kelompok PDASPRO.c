#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SIZE 8

char board[SIZE][SIZE];
int gameOver = 0; // 0 = main terus, 1 = selesai

// Inisialisasi papan
void initBoard() {
    char init[8][9] = {
        "rnbqkbnr",
        "pppppppp",
        "........",
        "........",
        "........",
        "........",
        "PPPPPPPP",
        "RNBQKBNR"
    };
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board[i][j] = init[i][j];
        }
    }
}

// Tampilkan papan
void printBoard() {
    printf("\n    a b c d e f g h\n");
    printf("   -----------------\n");
    for (int i = 0; i < SIZE; i++) {
        printf("%d | ", 8 - i);
        for (int j = 0; j < SIZE; j++) {
            printf("%c ", board[i][j]);
        }
        printf("|\n");
    }
    printf("   -----------------\n");
}

// Kolom huruf ke indeks
int colIndex(char c) {
    return c - 'a';
}

// Cek posisi valid
int validPos(int row, int col) {
    return row >= 0 && row < SIZE && col >= 0 && col < SIZE;
}

// Cek apakah raja masih ada
void checkKingStatus() {
    int foundWhite = 0, foundBlack = 0;

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == 'K') foundWhite = 1;
            if (board[i][j] == 'k') foundBlack = 1;
        }
    }

    if (!foundWhite) {
        printf("\n=== Raja putih tertangkap! HITAM MENANG! ===\n");
        gameOver = 1;
    } else if (!foundBlack) {
        printf("\n=== Raja hitam tertangkap! PUTIH MENANG! ===\n");
        gameOver = 1;
    }
}

// Fitur promosi pion
void checkPromotion() {
    for (int j = 0; j < SIZE; j++) {
        // Pion putih ke ujung atas
        if (board[0][j] == 'P') {
            char pilih;
            printf("\nPion putih di kolom %c mencapai akhir!\n", 'a' + j);
            printf("Promosikan ke (Q=Ratu, R=Benteng, B=Gajah, N=Kuda): ");
            scanf(" %c", &pilih);
            switch (pilih) {
                case 'Q': board[0][j] = 'Q'; break;
                case 'R': board[0][j] = 'R'; break;
                case 'B': board[0][j] = 'B'; break;
                case 'N': board[0][j] = 'N'; break;
                default: board[0][j] = 'Q'; printf("Otomatis jadi Ratu.\n");
            }
        }
        // Pion hitam ke ujung bawah
        if (board[7][j] == 'p') {
            char pilih;
            printf("\nPion hitam di kolom %c mencapai akhir!\n", 'a' + j);
            printf("Promosikan ke (q=Ratu, r=Benteng, b=Gajah, n=Kuda): ");
            scanf(" %c", &pilih);
            switch (pilih) {
                case 'q': board[7][j] = 'q'; break;
                case 'r': board[7][j] = 'r'; break;
                case 'b': board[7][j] = 'b'; break;
                case 'n': board[7][j] = 'n'; break;
                default: board[7][j] = 'q'; printf("Otomatis jadi Ratu.\n");
            }
        }
    }
}

// Pindahkan bidak
void movePiece(int fromRow, int fromCol, int toRow, int toCol) {
    char target = board[toRow][toCol];
    char moving = board[fromRow][fromCol];

    // Jika target adalah raja, permainan selesai
    if (target == 'K' || target == 'k') {
        board[toRow][toCol] = moving;
        board[fromRow][fromCol] = '.';
        printBoard();
        checkKingStatus();
        return;
    }

    board[toRow][toCol] = moving;
    board[fromRow][fromCol] = '.';

    checkPromotion();  // cek promosi
    checkKingStatus(); // cek apakah raja masih hidup
}

// Langkah komputer acak
void computerMove() {
    int fromRow, fromCol, toRow, toCol;
    srand(time(NULL));
    while (1) {
        fromRow = rand() % 8;
        fromCol = rand() % 8;
        toRow = rand() % 8;
        toCol = rand() % 8;
        if (board[fromRow][fromCol] >= 'a' && board[fromRow][fromCol] <= 'z') {
            if (board[toRow][toCol] == '.' || (board[toRow][toCol] >= 'A' && board[toRow][toCol] <= 'Z')) {
                printf("\nKomputer memindahkan bidak dari %c%d ke %c%d\n",
                       'a' + fromCol, 8 - fromRow, 'a' + toCol, 8 - toRow);
                movePiece(fromRow, fromCol, toRow, toCol);
                break;
            }
        }
    }
}

// Mode user vs user
void userVsUser() {
    char move1[5], move2[5];
    int turn = 1; // 1 Putih, -1 Hitam
    while (!gameOver) {
        printBoard();
        printf("\nGiliran %s\n", (turn == 1) ? "Putih (huruf besar)" : "Hitam (huruf kecil)");
        printf("Masukkan langkah (contoh a2 a3, atau 'exit' untuk keluar): ");
        scanf("%s", move1);
        if (strcmp(move1, "exit") == 0) break;
        scanf("%s", move2);

        int fromCol = colIndex(move1[0]);
        int fromRow = 8 - (move1[1] - '0');
        int toCol = colIndex(move2[0]);
        int toRow = 8 - (move2[1] - '0');

        if (!validPos(fromRow, fromCol) || !validPos(toRow, toCol)) {
            printf("Posisi tidak valid!\n");
            continue;
        }

        movePiece(fromRow, fromCol, toRow, toCol);
        turn *= -1;
    }
}

// Mode user vs komputer
void userVsComputer() {
    char move1[5], move2[5];
    while (!gameOver) {
        printBoard();
        printf("\nGiliran Anda (Putih). Masukkan langkah (contoh a2 a3, atau 'exit' untuk keluar): ");
        scanf("%s", move1);
        if (strcmp(move1, "exit") == 0) break;
        scanf("%s", move2);

        int fromCol = colIndex(move1[0]);
        int fromRow = 8 - (move1[1] - '0');
        int toCol = colIndex(move2[0]);
        int toRow = 8 - (move2[1] - '0');

        if (!validPos(fromRow, fromCol) || !validPos(toRow, toCol)) {
            printf("Posisi tidak valid!\n");
            continue;
        }

        movePiece(fromRow, fromCol, toRow, toCol);
        if (gameOver) break;

        printBoard();
        printf("\nGiliran komputer...\n");
        computerMove();
    }
}

int main() {
    int mode;
    printf("=== PERMAINAN CATUR SEDERHANA ===\n");
    printf("1. User vs User\n");
    printf("2. User vs Komputer\n");
    printf("Pilih mode: ");
    scanf("%d", &mode);

    initBoard();

    if (mode == 1)
        userVsUser();
    else if (mode == 2)
        userVsComputer();
    else
        printf("Mode tidak dikenali!\n");

    printf("\n=== Permainan selesai ===\n");
    return 0;
}
