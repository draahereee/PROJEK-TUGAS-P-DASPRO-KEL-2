/* catur_menu_final_v3.c
   Catur lengkap PvP & PvC dengan:
   - castling (king-side & queen-side)
   - en-passant
   - promosi interaktif (Q/R/B/N)
   - check / checkmate detection
   - history, halfmove & fullmove
   - warna acak, tampilkan siapa putih/hitam
   - exit command saat bermain
   Dibuat untuk kestabilan dan mempertahankan semua fitur.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define SIZE 8
#define MAX_MOVES 1024
#define MAX_HISTORY 4096

/* ===== State ===== */
char board[SIZE][SIZE];
int lastFromR=-1, lastFromC=-1, lastToR=-1, lastToC=-1;
int halfmoveClock = 0; // resets on pawn move or capture
int fullmoveNumber = 1;
int gameOver = 0;

// castling flags
int whiteKingMoved = 0, blackKingMoved = 0;
int whiteRookA_Moved = 0, whiteRookH_Moved = 0;
int blackRookA_Moved = 0, blackRookH_Moved = 0;

// en-passant target (square where pawn would land if capturing en-passant)
int epR = -1, epC = -1;

// move history
char history[MAX_HISTORY][32];
int historyCount = 0;

typedef struct {
    int fr, fc, tr, tc;
    char movedPiece;
    char capturedPiece;
    int prevHalfmoveClock;
    // flags snapshot (not used heavily here but kept for completeness)
} Move;

/* ===== Utility ===== */
int colIndex(char c) { return c - 'a'; }
int validPos(int r,int c) { return r>=0 && r<8 && c>=0 && c<8; }

void initBoard() {
    const char *init[8] = {
        "rnbqkbnr",
        "pppppppp",
        "........",
        "........",
        "........",
        "........",
        "PPPPPPPP",
        "RNBQKBNR"
    };
    for (int r=0;r<8;r++) for (int c=0;c<8;c++) board[r][c]=init[r][c];
    lastFromR=lastFromC=lastToR=lastToC=-1;
    halfmoveClock = 0; fullmoveNumber = 1; historyCount = 0; gameOver = 0;
    whiteKingMoved = blackKingMoved = 0;
    whiteRookA_Moved = whiteRookH_Moved = 0;
    blackRookA_Moved = blackRookH_Moved = 0;
    epR = epC = -1;
}

const char *RESET = "\x1b[0m";
const char *REV = "\x1b[7m";

void printBoard() {
    printf("\n    a b c d e f g h\n");
    printf("   -----------------\n");
    for (int r=0;r<8;r++) {
        printf("%d | ", 8-r);
        for (int c=0;c<8;c++) {
            int hl = (r==lastFromR && c==lastFromC) || (r==lastToR && c==lastToC);
            if (hl) printf("%s", REV);
            putchar(board[r][c]);
            if (hl) printf("%s", RESET);
            printf(" ");
        }
        printf("|\n");
    }
    printf("   -----------------\n");
    printf("Halfmove clock: %d | Fullmove: %d\n", halfmoveClock, fullmoveNumber);
    if (historyCount>0) {
        int start = historyCount>8 ? historyCount-8 : 0;
        printf("History (last): ");
        for (int i=start;i<historyCount;i++) printf("%s ", history[i]);
        printf("\n");
    }
}

/* ===== Attack detection ===== */
int isSquareAttacked(int r,int c,char bySide) {
    if (!validPos(r,c)) return 0;
    // pawns
    if (bySide=='w') {
        int rr=r+1;
        if (validPos(rr,c-1) && board[rr][c-1]=='P') return 1;
        if (validPos(rr,c+1) && board[rr][c+1]=='P') return 1;
    } else {
        int rr=r-1;
        if (validPos(rr,c-1) && board[rr][c-1]=='p') return 1;
        if (validPos(rr,c+1) && board[rr][c+1]=='p') return 1;
    }
    // knights
    int kr[8]={-2,-2,-1,-1,1,1,2,2}, kc[8]={-1,1,-2,2,-2,2,-1,1};
    for (int k=0;k<8;k++) {
        int rr=r+kr[k], cc=c+kc[k]; if (!validPos(rr,cc)) continue;
        char ch = board[rr][cc];
        if (bySide=='w' && ch=='N') return 1;
        if (bySide=='b' && ch=='n') return 1;
    }
    // orthogonal sliding
    int orr[4]={-1,1,0,0}, orc[4]={0,0,-1,1};
    for (int d=0;d<4;d++){
        int rr=r+orr[d], cc=c+orc[d];
        while (validPos(rr,cc)){
            char ch=board[rr][cc]; if (ch!='.'){
                if (bySide=='w' && (ch=='R' || ch=='Q')) return 1;
                if (bySide=='b' && (ch=='r' || ch=='q')) return 1;
                break;
            }
            rr+=orr[d]; cc+=orc[d];
        }
    }
    // diagonal sliding
    int dgr[4]={-1,-1,1,1}, dgc[4]={-1,1,-1,1};
    for (int d=0;d<4;d++){
        int rr=r+dgr[d], cc=c+dgc[d];
        while (validPos(rr,cc)){
            char ch=board[rr][cc]; if (ch!='.'){
                if (bySide=='w' && (ch=='B' || ch=='Q')) return 1;
                if (bySide=='b' && (ch=='b' || ch=='q')) return 1;
                break;
            }
            rr+=dgr[d]; cc+=dgc[d];
        }
    }
    // king
    for (int rr=r-1; rr<=r+1; rr++) for (int cc=c-1; cc<=c+1; cc++){
        if (!validPos(rr,cc) || (rr==r&&cc==c)) continue;
        char ch = board[rr][cc]; if (bySide=='w' && ch=='K') return 1; if (bySide=='b' && ch=='k') return 1;
    }
    return 0;
}

/* ===== Helpers: path clear for sliding pieces ===== */
int pathClear(int fr,int fc,int tr,int tc) {
    int dr = (tr>fr) ? 1 : (tr<fr) ? -1 : 0;
    int dc = (tc>fc) ? 1 : (tc<fc) ? -1 : 0;
    int rr = fr+dr, cc = fc+dc;
    while (rr!=tr || cc!=tc) {
        if (!validPos(rr,cc)) return 0;
        if (board[rr][cc] != '.') return 0;
        rr += dr; cc += dc;
    }
    return 1;
}

/* ===== Move legality (pattern only) ===== */
int isLegalPatternMove(int fr,int fc,int tr,int tc) {
    if (!validPos(fr,fc) || !validPos(tr,tc)) return 0;
    if (fr==tr && fc==tc) return 0;
    char p = board[fr][fc]; if (p=='.') return 0;
    char target = board[tr][tc];
    // cannot capture own piece
    if (target != '.') {
        if ((isupper(p) && isupper(target)) || (islower(p) && islower(target))) return 0;
    }
    // piece-specific
    if (p=='P' || p=='p') {
        int dir = (p=='P') ? -1 : 1;
        // single forward
        if (tc==fc && tr==fr+dir && target=='.') return 1;
        // double from starting rank
        if (tc==fc && tr==fr+2*dir && target=='.') {
            int midr = fr+dir; if (board[midr][fc]=='.') {
                if ((p=='P' && fr==6) || (p=='p' && fr==1)) return 1;
            }
        }
        // captures
        if ((tr==fr+dir) && (tc==fc+1 || tc==fc-1) && target!='.') return 1;
        // en-passant handled in generator
        return 0;
    }
    if (p=='N' || p=='n') {
        int dr = abs(tr-fr), dc = abs(tc-fc);
        return (dr==2 && dc==1) || (dr==1 && dc==2);
    }
    if (p=='B' || p=='b') {
        if (abs(tr-fr) == abs(tc-fc)) return pathClear(fr,fc,tr,tc);
        return 0;
    }
    if (p=='R' || p=='r') {
        if (tr==fr || tc==fc) return pathClear(fr,fc,tr,tc);
        return 0;
    }
    if (p=='Q' || p=='q') {
        if (tr==fr || tc==fc) return pathClear(fr,fc,tr,tc);
        if (abs(tr-fr) == abs(tc-fc)) return pathClear(fr,fc,tr,tc);
        return 0;
    }
    if (p=='K' || p=='k') {
        if (abs(tr-fr)<=1 && abs(tc-fc)<=1) return 1;
        // castling handled in generator
        return 0;
    }
    return 0;
}

/* ===== Would move leave own king in check? (simulate) ===== */
void applyMoveTemp(int fr,int fc,int tr,int tc, char *savedFrom, char *savedTo) {
    *savedFrom = board[fr][fc]; *savedTo = board[tr][tc];
    board[tr][tc] = board[fr][fc]; board[fr][fc] = '.';
}
void unapplyMoveTemp(int fr,int fc,int tr,int tc, char savedFrom, char savedTo) {
    board[fr][fc] = savedFrom; board[tr][tc] = savedTo;
}

int wouldLeaveKingInCheck(int fr,int fc,int tr,int tc) {
    char savedFrom, savedTo; applyMoveTemp(fr,fc,tr,tc,&savedFrom,&savedTo);
    // find king of the mover
    char mover = savedFrom; char king = (isupper(mover)?'K':'k');
    int kr=-1,kc=-1;
    for (int r=0;r<8;r++) for (int c=0;c<8;c++) if (board[r][c]==king) { kr=r; kc=c; }
    int attacked = 1; // default assume attacked if king missing
    if (kr!=-1) attacked = isSquareAttacked(kr,kc, isupper(mover)?'b':'w');
    unapplyMoveTemp(fr,fc,tr,tc,savedFrom,savedTo);
    return attacked;
}

/* ===== Generate legal moves (no check leaving king) ===== */
typedef struct {int fr,fc,tr,tc; char promo;} GenMove;
GenMove genList[MAX_MOVES]; int genCount=0;

void addGenMove(int fr,int fc,int tr,int tc,char promo) {
    if (genCount < MAX_MOVES) { genList[genCount].fr=fr; genList[genCount].fc=fc; genList[genCount].tr=tr; genList[genCount].tc=tc; genList[genCount].promo=promo; genCount++; }
}

void generateLegalMoves(char side) {
    genCount = 0;
    for (int r=0;r<8;r++) for (int c=0;c<8;c++) {
        char p = board[r][c]; if (p=='.') continue;
        if (side=='w' && !(p>='A' && p<='Z')) continue;
        if (side=='b' && !(p>='a' && p<='z')) continue;
        for (int tr=0; tr<8; tr++) for (int tc=0; tc<8; tc++) {
            if (!isLegalPatternMove(r,c,tr,tc)) {
                // handle en-passant pattern: when target is ep square and pattern is pawn diagonal and target currently empty
                if ((p=='P' || p=='p') && epR!=-1 && tr==epR && tc==epC && (tr==r-1 || tr==r+1) && abs(tc-c)==1 && board[tr][tc]=='.') {
                    // proceed (will fully verify later)
                } else continue;
            }
            // handle pawn promotions separately
            if ((p=='P' && tr==0) || (p=='p' && tr==7)) {
                char promos[4] = { (isupper(p)?'Q':'q'), (isupper(p)?'R':'r'), (isupper(p)?'B':'b'), (isupper(p)?'N':'n') };
                for (int k=0;k<4;k++) {
                    // simulate promotion and king-safety
                    char savedFrom = board[r][c], savedTo = board[tr][tc];
                    board[tr][tc] = promos[k]; board[r][c] = '.';
                    int kingSafe = 1;
                    char king = isupper(savedFrom)?'K':'k'; int kr=-1,kc=-1;
                    for (int rr=0;rr<8;rr++) for (int cc=0;cc<8;cc++) if (board[rr][cc]==king) { kr=rr; kc=cc; }
                    if (kr!=-1) kingSafe = !isSquareAttacked(kr,kc, isupper(savedFrom)?'b':'w');
                    board[r][c]=savedFrom; board[tr][tc]=savedTo;
                    if (kingSafe) addGenMove(r,c,tr,tc,promos[k]);
                }
                continue;
            }
            // en-passant verification
            int isEP = 0;
            if ((p=='P' || p=='p') && epR!=-1 && tr==epR && tc==epC && board[tr][tc]=='.' && abs(tc-c)==1 && ((p=='P' && tr==r-1) || (p=='p' && tr==r+1))) {
                int capR = (p=='P') ? tr+1 : tr-1;
                if (validPos(capR,tc) && ((p=='P' && board[capR][tc]=='p') || (p=='p' && board[capR][tc]=='P'))) {
                    isEP = 1;
                } else continue;
            }
            // handle castling pattern (king moves two squares)
            if ((p=='K' && r==7 && c==4 && (tr==7 && (tc==6 || tc==2))) ||
                (p=='k' && r==0 && c==4 && (tr==0 && (tc==6 || tc==2)))) {
                if (p=='K') {
                    if (whiteKingMoved) continue;
                    if (tc==6) { if (whiteRookH_Moved) continue; if (board[7][5]!='.'||board[7][6]!='.') continue; if (board[7][7]!='R') continue;
                                 if (isSquareAttacked(7,4,'b')||isSquareAttacked(7,5,'b')||isSquareAttacked(7,6,'b')) continue; }
                    else { if (whiteRookA_Moved) continue; if (board[7][3]!='.'||board[7][2]!='.'||board[7][1]!='.') continue; if (board[7][0]!='R') continue;
                           if (isSquareAttacked(7,4,'b')||isSquareAttacked(7,3,'b')||isSquareAttacked(7,2,'b')) continue; }
                } else {
                    if (blackKingMoved) continue;
                    if (tc==6) { if (blackRookH_Moved) continue; if (board[0][5]!='.'||board[0][6]!='.') continue; if (board[0][7]!='r') continue;
                                 if (isSquareAttacked(0,4,'w')||isSquareAttacked(0,5,'w')||isSquareAttacked(0,6,'w')) continue; }
                    else { if (blackRookA_Moved) continue; if (board[0][3]!='.'||board[0][2]!='.'||board[0][1]!='.') continue; if (board[0][0]!='r') continue;
                           if (isSquareAttacked(0,4,'w')||isSquareAttacked(0,3,'w')||isSquareAttacked(0,2,'w')) continue; }
                }
            }
            // Now test king safety by simulating (include castling rook move and en-passant captured pawn)
            int leaves = 0;
            char savedFrom = board[r][c], savedTo = board[tr][tc];
            char epCaptured = '.';
            int capR=-1, capC=-1;
            if (isEP) {
                capR = (savedFrom=='P') ? tr+1 : tr-1;
                capC = tc;
                epCaptured = board[capR][capC];
                board[capR][capC] = '.';
            }
            board[tr][tc] = board[r][c]; board[r][c] = '.';
            int castRfrom=-1, castCfrom=-1, castRto=-1, castCto=-1;
            if ((savedFrom=='K' && r==7 && c==4 && (tr==7 && (tc==6 || tc==2))) ||
                (savedFrom=='k' && r==0 && c==4 && (tr==0 && (tc==6 || tc==2)))) {
                if (savedFrom=='K') {
                    if (tc==6) { castRfrom=7; castCfrom=7; castRto=7; castCto=5; }
                    else { castRfrom=7; castCfrom=0; castRto=7; castCto=3; }
                } else {
                    if (tc==6) { castRfrom=0; castCfrom=7; castRto=0; castCto=5; }
                    else { castRfrom=0; castCfrom=0; castRto=0; castCto=3; }
                }
                if (castRfrom!=-1) {
                    board[castRto][castCto] = board[castRfrom][castCfrom];
                    board[castRfrom][castCfrom] = '.';
                }
            }
            // find mover king
            char king = isupper(savedFrom)?'K':'k';
            int kr=-1,kc=-1;
            for (int rr=0;rr<8;rr++) for (int cc=0;cc<8;cc++) if (board[rr][cc]==king) { kr=rr; kc=cc; }
            if (kr==-1) leaves = 1; else {
                if (isSquareAttacked(kr,kc, isupper(savedFrom)?'b':'w')) leaves = 1;
            }
            // undo
            if (castRfrom!=-1) {
                board[castRfrom][castCfrom] = board[castRto][castCto];
                board[castRto][castCto] = '.';
            }
            board[r][c] = savedFrom; board[tr][tc] = savedTo;
            if (isEP && capR!=-1) board[capR][capC] = epCaptured;
            if (!leaves) {
                addGenMove(r,c,tr,tc, 0);
            }
        }
    }
}

/* ===== Make / unmake moves (update halfmove clock, flags) ===== */
void makeMoveStruct(Move *m) {
    m->movedPiece = board[m->fr][m->fc];
    m->capturedPiece = board[m->tr][m->tc];
    m->prevHalfmoveClock = halfmoveClock;
    // update halfmove clock
    if (m->movedPiece=='P' || m->movedPiece=='p' || m->capturedPiece!='.') halfmoveClock = 0; else halfmoveClock++;
    // detect en-passant capture
    int isEP = 0;
    if ((m->movedPiece=='P' || m->movedPiece=='p') && m->tr==epR && m->tc==epC && m->capturedPiece=='.') {
        int capR = (m->movedPiece=='P') ? m->tr+1 : m->tr-1;
        if (validPos(capR,m->tc) && ((m->movedPiece=='P' && board[capR][m->tc]=='p') || (m->movedPiece=='p' && board[capR][m->tc]=='P'))) {
            isEP = 1;
            m->capturedPiece = board[capR][m->tc];
            board[capR][m->tc] = '.';
        }
    }
    // move
    board[m->tr][m->tc] = board[m->fr][m->fc];
    board[m->fr][m->fc] = '.';
    // castling rook move
    if (m->movedPiece=='K' && m->fr==7 && m->fc==4 && (m->tc==6 || m->tc==2)) {
        if (m->tc==6) { board[7][5] = board[7][7]; board[7][7] = '.'; }
        else { board[7][3] = board[7][0]; board[7][0] = '.'; }
    } else if (m->movedPiece=='k' && m->fr==0 && m->fc==4 && (m->tc==6 || m->tc==2)) {
        if (m->tc==6) { board[0][5] = board[0][7]; board[0][7] = '.'; }
        else { board[0][3] = board[0][0]; board[0][0] = '.'; }
    }
    // update castling flags
    if (m->movedPiece=='K') whiteKingMoved = 1;
    if (m->movedPiece=='k') blackKingMoved = 1;
    if (m->movedPiece=='R' && m->fr==7 && m->fc==0) whiteRookA_Moved = 1;
    if (m->movedPiece=='R' && m->fr==7 && m->fc==7) whiteRookH_Moved = 1;
    if (m->movedPiece=='r' && m->fr==0 && m->fc==0) blackRookA_Moved = 1;
    if (m->movedPiece=='r' && m->fr==0 && m->fc==7) blackRookH_Moved = 1;
    if (m->capturedPiece=='R' && m->tr==7 && m->tc==0) whiteRookA_Moved = 1;
    if (m->capturedPiece=='R' && m->tr==7 && m->tc==7) whiteRookH_Moved = 1;
    if (m->capturedPiece=='r' && m->tr==0 && m->tc==0) blackRookA_Moved = 1;
    if (m->capturedPiece=='r' && m->tr==0 && m->tc==7) blackRookH_Moved = 1;
    // handle en-passant target: if pawn moved two squares, set ep square, else clear
    epR = epC = -1;
    if (m->movedPiece=='P' && m->fr==6 && m->tr==4) { epR = 5; epC = m->fc; }
    else if (m->movedPiece=='p' && m->fr==1 && m->tr==3) { epR = 2; epC = m->fc; }
    // promotion handling: caller should set promoted piece into board after calling makeMoveStruct if needed
}

/* ===== Promotion interactive helper (used by user moves) ===== */
char askPromotionPiece(int isWhite) {
    char choice = 'Q';
    printf("Promosi pion! Pilih (Q/R/B/N): "); fflush(stdout);
    if (scanf(" %c",&choice)==1) {
        while (getchar()!='\n');
        choice = toupper(choice);
        if (strchr("QRBN",choice)==NULL) choice='Q';
    } else {
        while (getchar()!='\n');
        choice='Q';
    }
    return isWhite?choice:tolower(choice);
}

/* ===== History record ===== */
void recordHistory(int fr,int fc,int tr,int tc) {
    if (historyCount < MAX_HISTORY) {
        snprintf(history[historyCount],32, "%c%d-%c%d", 'a'+fc, 8-fr, 'a'+tc, 8-tr);
        historyCount++;
    }
}

/* ===== Score simple ===== */
int pieceScore(char p) {
    switch(p){case 'P': return 1; case 'p': return -1; case 'N': return 3; case 'n': return -3; case 'B': return 3; case 'b': return -3; case 'R': return 5; case 'r': return -5; case 'Q': return 9; case 'q': return -9; default: return 0;}
}
int totalScore(){int s=0; for (int r=0;r<8;r++) for (int c=0;c<8;c++) s+=pieceScore(board[r][c]); return s;}

/* ===== AI Semi-random: prefer captures else random ===== */
void aiMove_SemiSmart() {
    generateLegalMoves('b');
    if (genCount==0) { printf("Komputer tidak punya langkah legal.\n"); gameOver=1; return; }
    int captureIdx = -1; for (int i=0;i<genCount;i++) { if (board[genList[i].tr][genList[i].tc] != '.') { captureIdx = i; break; } }
    int chosen = (captureIdx!=-1) ? captureIdx : (rand() % genCount);
    Move m; m.fr = genList[chosen].fr; m.fc = genList[chosen].fc; m.tr = genList[chosen].tr; m.tc = genList[chosen].tc;
    // apply as struct to keep clock etc.
    makeMoveStruct(&m);
    // auto-promotion to queen for AI
    if (board[m.tr][m.tc]=='p' && m.tr==7) board[m.tr][m.tc]='q';
    if (board[m.tr][m.tc]=='P' && m.tr==0) board[m.tr][m.tc]='Q';
    lastFromR=m.fr; lastFromC=m.fc; lastToR=m.tr; lastToC=m.tc; recordHistory(m.fr,m.fc,m.tr,m.tc);
    if (m.movedPiece>='a' && m.movedPiece<='z') fullmoveNumber++;
    printf("Komputer: %c%d -> %c%d\n", 'a'+m.fc, 8-m.fr, 'a'+m.tc, 8-m.tr);
}

/* ===== Endgame checks (checkmate/stalemate/50-move/insufficient) ===== */
int inCheck(char side) {
    char K = (side=='w')?'K':'k'; int kr=-1,kc=-1;
    for (int r=0;r<8;r++) for (int c=0;c<8;c++) if (board[r][c]==K){kr=r;kc=c;}
    if (kr==-1) return 0;
    return isSquareAttacked(kr,kc, side=='w'?'b':'w');
}

int hasAnyLegalMove(char side){ generateLegalMoves(side); return genCount>0; }

int insufficientMaterial(){
    int wP=0,wN=0,wB=0,wR=0,wQ=0; int bP=0,bN=0,bB=0,bR=0,bQ=0;
    for (int r=0;r<8;r++) for (int c=0;c<8;c++){ char p=board[r][c];
        switch(p){ case 'P': wP++; break; case 'N': wN++; break; case 'B': wB++; break; case 'R': wR++; break; case 'Q': wQ++; break;
                   case 'p': bP++; break; case 'n': bN++; break; case 'b': bB++; break; case 'r': bR++; break; case 'q': bQ++; break; }
    }
    if (wP+bP+wR+bR+wQ+bQ==0) {
        if (wN+wB==0 && bN+bB==0) return 1;
        if ((wN+wB==1) && (bN+bB==0)) return 1;
        if ((bN+bB==1) && (wN+wB==0)) return 1;
    }
    return 0;
}

void checkGameEndConditionsAndReport(char playerTurn, char mode, char playerColor) {
    // check kings existence
    int foundW=0, foundB=0;
    for (int r=0;r<8;r++) for (int c=0;c<8;c++){ if (board[r][c]=='K') foundW=1; if (board[r][c]=='k') foundB=1;}
    if (!foundW){ printf("\n=== Raja putih hilang! HITAM MENANG! ===\n"); gameOver=1; return; }
    if (!foundB){ printf("\n=== Raja hitam hilang! PUTIH MENANG! ===\n"); gameOver=1; return; }
    if (halfmoveClock>=100){ printf("\n=== Draw by 50-move rule ===\n"); gameOver=1; return; }
    if (insufficientMaterial()){ printf("\n=== Draw by insufficient material ===\n"); gameOver=1; return; }
    // check stalemate / checkmate for side to move (playerTurn tells previous mover in some calls)
    if (!hasAnyLegalMove(playerTurn)) {
        if (inCheck(playerTurn)) {
            // checkmate: other side won
            if (playerTurn=='w') {
                printf("\n=== CHECKMATE! HITAM MENANG! ===\n");
                if (mode=='C') {
                    // if playerColor == 'b' and black won when player is black
                    if (playerColor=='b') printf("Kamu menang!\n"); else printf("Kamu kalah!\n");
                }
            } else {
                printf("\n=== CHECKMATE! PUTIH MENANG! ===\n");
                if (mode=='C') {
                    if (playerColor=='w') printf("Kamu menang!\n"); else printf("Kamu kalah!\n");
                }
            }
            gameOver=1; return;
        } else {
            printf("\n=== STALEMATE! DRAW ===\n"); gameOver=1; return;
        }
    }
}

/* ===== Input parsing (accept many formats) ===== */
int parseSquare(const char *s, int *r, int *c) {
    if (!s || strlen(s)<2) return 0;
    char file = tolower((unsigned char)s[0]); char rank = s[1];
    if (file<'a' || file>'h') return 0; if (rank<'1' || rank>'8') return 0;
    *c = colIndex(file); *r = 8 - (rank - '0'); return 1;
}

// read move pair; supports "e2 e4", "e2e4", "O-O", "O-O-O", "exit"
int readMovePair(char *outA, size_t lena, char *outB, size_t lenb) {
    char line[256];
    if (!fgets(line, sizeof(line), stdin)) return 0;
    // trim newline
    size_t L = strlen(line);
    if (L>0 && line[L-1]=='\n') line[L-1] = '\0';
    // trim leading spaces
    char *s = line;
    while (*s && isspace((unsigned char)*s)) s++;
    if (!*s) return 0;
    // normalize
    for (char *p=s; *p; ++p) if (*p=='-' || *p==',') *p=' ';
    // tokenize
    char *tok1 = strtok(s, " \t");
    char *tok2 = strtok(NULL, " \t");
    if (!tok1) return 0;
    // exit
    if (strcasecmp(tok1,"exit")==0) { strncpy(outA,"exit",lena-1); outA[lena-1]='\0'; outB[0]='\0'; return 1;}
    // handle O-O
    char tmp[16]; int ti=0;
    for (int i=0; tok1[i] && i<15; ++i) tmp[ti++]=toupper((unsigned char)tok1[i]); tmp[ti]='\0';
    if (strcmp(tmp,"O-O")==0 || strcmp(tmp,"0-0")==0) { strncpy(outA,"O-O",lena-1); outA[lena-1]='\0'; outB[0]='\0'; return 1; }
    if (strcmp(tmp,"O-O-O")==0 || strcmp(tmp,"0-0-0")==0) { strncpy(outA,"O-O-O",lena-1); outA[lena-1]='\0'; outB[0]='\0'; return 1; }
    if (tok2==NULL && strlen(tok1)==4) {
        outA[0]=tolower((unsigned char)tok1[0]); outA[1]=tok1[1]; outA[2]='\0';
        outB[0]=tolower((unsigned char)tok1[2]); outB[1]=tok1[3]; outB[2]='\0';
        return 1;
    }
    if (!tok2) return 0;
    outA[0]=tolower((unsigned char)tok1[0]); outA[1]=tok1[1]; outA[2]='\0';
    outB[0]=tolower((unsigned char)tok2[0]); outB[1]=tok2[1]; outB[2]='\0';
    return 1;
}

/* ===== Game loops (User vs User and User vs Computer) ===== */
void userVsUserLoop() {
    int turn = 1; // 1 = white to move, -1 = black to move
    char a[16], b[16];
    while (!gameOver) {
        printBoard();
        printf("\nGiliran %s\n", turn==1 ? "Putih" : "Hitam");
        printf("Masukkan langkah (contoh a2 a3 atau e2e4 atau O-O), atau 'exit': ");
        if (!readMovePair(a,sizeof(a),b,sizeof(b))) { printf("Input tidak terbaca atau EOF. Kembali ke menu.\n"); break; }
        if (strcmp(a,"exit")==0) { printf("Keluar dari permainan.\n"); break; }
        // handle O-O
        if (strcmp(a,"O-O")==0 || strcmp(a,"O-O-O")==0) {
            int fr = (turn==1) ? 7 : 0;
            int fc = 4;
            int tr = fr;
            int tc = (strcmp(a,"O-O")==0) ? 6 : 2;
            // validate via generator
            generateLegalMoves(turn==1 ? 'w' : 'b');
            int legal=0; for (int i=0;i<genCount;i++) if (genList[i].fr==fr && genList[i].fc==fc && genList[i].tr==tr && genList[i].tc==tc) { legal=1; break; }
            if (!legal) { printf("Castling tidak sah!\n"); continue; }
            Move m = {fr,fc,tr,tc,0,0,0}; makeMoveStruct(&m);
            // no promotion for castling
            lastFromR=fr; lastFromC=fc; lastToR=tr; lastToC=tc; recordHistory(fr,fc,tr,tc);
            if (m.movedPiece>='a' && m.movedPiece<='z') fullmoveNumber++;
            checkGameEndConditionsAndReport(turn==1 ? 'b' : 'w', 'P', 'w');
            turn *= -1; continue;
        }
        int fr,fc,tr,tc;
        if (!parseSquare(a,&fr,&fc) || !parseSquare(b,&tr,&tc)) { printf("Format salah!\n"); continue; }
        if (!validPos(fr,fc) || !validPos(tr,tc)) { printf("Posisi tidak valid!\n"); continue; }
        char piece = board[fr][fc];
        if (turn==1 && !(piece>='A'&&piece<='Z')){ printf("Itu bukan bidak putih!\n"); continue;}
        if (turn==-1 && !(piece>='a'&&piece<='z')){ printf("Itu bukan bidak hitam!\n"); continue; }
        // generate moves and check legality
        generateLegalMoves(turn==1 ? 'w' : 'b');
        int legal=0; for (int i=0;i<genCount;i++) if (genList[i].fr==fr && genList[i].fc==fc && genList[i].tr==tr && genList[i].tc==tc) { legal=1; break; }
        if (!legal){ printf("Langkah tidak sah!\n"); continue; }
        Move m = {fr,fc,tr,tc,0,0,0}; makeMoveStruct(&m);
        // promotion interactive: if pawn reached last rank, ask choice (we already moved, so piece is there)
        if ((board[tr][tc]=='P' && tr==0) || (board[tr][tc]=='p' && tr==7)) {
            int isWhite = (board[tr][tc]=='P');
            char prom = askPromotionPiece(isWhite);
            board[tr][tc] = isWhite ? prom : tolower(prom);
        }
        lastFromR=fr; lastFromC=fc; lastToR=tr; lastToC=tc; recordHistory(fr,fc,tr,tc);
        if (m.movedPiece>='a' && m.movedPiece<='z') fullmoveNumber++;
        checkGameEndConditionsAndReport(turn==1 ? 'b' : 'w', 'P', 'w');
        turn *= -1;
    }
}

void userVsComputerLoop(char playerColor) {
    // playerColor == 'w' means human plays white; if 'b' human plays black
    char human = playerColor; char computer = (playerColor=='w')?'b':'w';
    int turn = 1; // 1 -> white to move; -1 -> black to move
    // If human is black, computer moves first (white first)
    if (turn==1 && computer=='w') {
        // computer first move
        generateLegalMoves('w'); if (genCount>0) { Move m; m.fr=genList[0].fr; m.fc=genList[0].fc; m.tr=genList[0].tr; m.tc=genList[0].tc; makeMoveStruct(&m);
            if (board[m.tr][m.tc]=='p' && m.tr==7) board[m.tr][m.tc]='q';
            if (board[m.tr][m.tc]=='P' && m.tr==0) board[m.tr][m.tc]='Q';
            lastFromR=m.fr; lastFromC=m.fc; lastToR=m.tr; lastToC=m.tc; recordHistory(m.fr,m.fc,m.tr,m.tc);
            printf("Komputer (putih) membuka: %c%d -> %c%d\n", 'a'+m.fc, 8-m.fr, 'a'+m.tc, 8-m.tr);
            if (m.movedPiece>='a' && m.movedPiece<='z') fullmoveNumber++;
        }
        turn = -1;
    }
    while (!gameOver) {
        printBoard();
        printf("\nGiliran %s\n", turn==1 ? "Putih" : "Hitam");
        if ((turn==1 && human=='w') || (turn==-1 && human=='b')) {
            // human move
            char a[16], b[16]; printf("Masukkan langkah (contoh a2 a3 atau e2e4 atau O-O), atau 'exit': ");
            if (!readMovePair(a,sizeof(a),b,sizeof(b))) { printf("Input tidak terbaca atau EOF. Kembali ke menu.\n"); break; }
            if (strcmp(a,"exit")==0) { printf("Keluar dari permainan.\n"); break; }
            // handle O-O
            if (strcmp(a,"O-O")==0 || strcmp(a,"O-O-O")==0) {
                int fr = (turn==1) ? 7 : 0;
                int fc = 4;
                int tr = fr;
                int tc = (strcmp(a,"O-O")==0) ? 6 : 2;
                generateLegalMoves(turn==1 ? 'w' : 'b');
                int legal=0; for (int i=0;i<genCount;i++) if (genList[i].fr==fr && genList[i].fc==fc && genList[i].tr==tr && genList[i].tc==tc) { legal=1; break; }
                if (!legal) { printf("Castling tidak sah!\n"); continue; }
                Move mm = {fr,fc,tr,tc,0,0,0}; makeMoveStruct(&mm);
                lastFromR=fr; lastFromC=fc; lastToR=tr; lastToC=tc; recordHistory(fr,fc,tr,tc);
                if (mm.movedPiece>='a' && mm.movedPiece<='z') fullmoveNumber++;
                // check end conditions for opponent
                checkGameEndConditionsAndReport(turn==1 ? 'b' : 'w', 'C', human);
                turn = -turn;
                continue;
            }
            int fr,fc,tr,tc;
            if (!parseSquare(a,&fr,&fc) || !parseSquare(b,&tr,&tc)) { printf("Format salah!\n"); continue; }
            if (!validPos(fr,fc) || !validPos(tr,tc)) { printf("Posisi tidak valid!\n"); continue; }
            char piece = board[fr][fc];
            if (turn==1 && !(piece>='A'&&piece<='Z')){ printf("Itu bukan bidak putih!\n"); continue;}
            if (turn==-1 && !(piece>='a'&&piece<='z')){ printf("Itu bukan bidak hitam!\n"); continue; }
            generateLegalMoves(turn==1 ? 'w' : 'b');
            int legal=0; for (int i=0;i<genCount;i++) if (genList[i].fr==fr && genList[i].fc==fc && genList[i].tr==tr && genList[i].tc==tc) { legal=1; break; }
            if (!legal){ printf("Langkah tidak sah!\n"); continue; }
            Move m = {fr,fc,tr,tc,0,0,0}; makeMoveStruct(&m);
            // promotion for human: ask choice if reached last rank
            if ((board[tr][tc]=='P' && tr==0) || (board[tr][tc]=='p' && tr==7)) {
                int isWhite = (board[tr][tc]=='P');
                char prom = askPromotionPiece(isWhite);
                board[tr][tc] = isWhite ? prom : tolower(prom);
            }
            lastFromR=fr; lastFromC=fc; lastToR=tr; lastToC=tc; recordHistory(fr,fc,tr,tc);
            if (m.movedPiece>='a' && m.movedPiece<='z') fullmoveNumber++;
            // check end conditions
            checkGameEndConditionsAndReport(turn==1 ? 'b' : 'w', 'C', human);
            turn = -turn;
        } else {
            // computer move
            printf("Giliran komputer (%s)\n", (turn==1)?"Putih":"Hitam");
            if (turn==1) { generateLegalMoves('w'); }
            else { generateLegalMoves('b'); }
            if (genCount==0) { // no moves -> check end
                checkGameEndConditionsAndReport(turn, 'C', human);
                break;
            }
            // choose capture if possible
            int captureIdx=-1;
            for (int i=0;i<genCount;i++) if (board[genList[i].tr][genList[i].tc] != '.') { captureIdx=i; break; }
            int chosen = (captureIdx!=-1)?captureIdx:(rand()%genCount);
            Move m; m.fr=genList[chosen].fr; m.fc=genList[chosen].fc; m.tr=genList[chosen].tr; m.tc=genList[chosen].tc;
            makeMoveStruct(&m);
            // auto promote AI pawns to queen
            if (board[m.tr][m.tc]=='p' && m.tr==7) board[m.tr][m.tc]='q';
            if (board[m.tr][m.tc]=='P' && m.tr==0) board[m.tr][m.tc]='Q';
            lastFromR=m.fr; lastFromC=m.fc; lastToR=m.tr; lastToC=m.tc; recordHistory(m.fr,m.fc,m.tr,m.tc);
            if (m.movedPiece>='a' && m.movedPiece<='z') fullmoveNumber++;
            printf("Komputer: %c%d -> %c%d\n", 'a'+m.fc, 8-m.fr, 'a'+m.tc, 8-m.tr);
            // check end conditions
            checkGameEndConditionsAndReport(turn==1 ? 'b' : 'w', 'C', human);
            turn = -turn;
        }
    }
}

/* ===== Menu and main ===== */
void menu() {
    int choice;
    char playerColor; // 'w' or 'b'
    while (1) {
        printf("=== CATUR ===\n1. Player vs Player\n2. Player vs Computer\n0. Keluar\nPilih mode: ");
        if (scanf("%d",&choice)!=1) { while(getchar()!='\n'); continue; }
        while(getchar()!='\n'); // consume newline
        if (choice==0) { printf("Terima kasih, keluar.\n"); break; }
        initBoard();
        // randomize colors
        if (choice==1) {
            if (rand()%2) { playerColor='w'; } else { playerColor='b'; }
            printf("PvP: Player A = %s, Player B = %s\n", (playerColor=='w')?"Putih":"Hitam", (playerColor=='w')?"Hitam":"Putih");
            // In PvP we don't track which player is "human" separately — both are human
            userVsUserLoop();
        } else if (choice==2) {
            // pick human color randomly
            if (rand()%2) playerColor='w'; else playerColor='b';
            printf("PvC: Kamu bermain sebagai %s\n", (playerColor=='w') ? "Putih" : "Hitam");
            userVsComputerLoop(playerColor);
        } else {
            printf("Pilihan salah!\n");
        }
    }
}

int main(){
    srand((unsigned int)time(NULL));
    menu();
    return 0;
}
