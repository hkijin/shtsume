//
//  dtools.h
//  shshogi
//
//  Created by Hkijin on 2021/11/05.
//

#ifndef dtools_h
#define dtools_h

#include <stdio.h>
#include "shogi.h"

/*
 * bitboard表記utility
 * bitboard bbの内容を盤面表示する。strは201byte以上必要。戻り値:書き込み長(200)
 */
int bitboard_sprintf(char *restrict str, const bitboard_t *bb);
int bitboard_fprintf(FILE *restrict stream, const bitboard_t *bb);
#define BITBOARD_PRINTF(bb)  bitboard_fprintf(stdout,bb)

int vbitboard_sprintf(char *restrict str, const bitboard_t *vbb);
int vbitboard_fprintf(FILE *restrict stream, const bitboard_t *vbb);
#define VBITBOARD_PRINTF(vbb) vbitboard_fprintf(stdout, vbb)

int rbitboard_sprintf(char *restrict str, const bitboard_t *rbb);
int rbitboard_fprintf(FILE *restrict stream, const bitboard_t *rbb);
#define RBITBOARD_PRINTF(rbb) rbitboard_fprintf(stdout, rbb)

int lbitboard_sprintf(char *restrict str, const bitboard_t *lbb);
int lbitboard_fprintf(FILE *restrict stream, const bitboard_t *lbb);
#define LBITBOARD_PRINTF(lbb) lbitboard_fprintf(stdout, lbb)

/*
 * N_KATTR  駒の種類の数
 * N_JNUM   持ち駒の表記に使用する漢数字の数
 */

#define N_KATTR     8
#define N_JNUM     19

extern char *g_koma_str [N_KOMAS];
extern char *g_koma_str2[N_KATTR];
extern char *g_kif_str  [N_KOMAS];
extern char *g_kif_str2 [N_KATTR];
extern char *g_jnum_str [N_JNUM ];

/*
 * 持駒表記utility
 */
int sprint_mkoma(char *restrict str,    const mkey_t mkey);
int fprint_mkoma(FILE *restrict stream, const mkey_t mkey);
#define PRINT_MKOMA(mkey) fprint_mkoma(stdout,mkey)
#define PRINT_MKEY(mkey) \
        printf(" 飛 角 金 銀 桂 香 歩　　\n" \
               "(%u  %u %u  %u %u  %u %02u)\n",\
                (mkey).hi,(mkey).ka,(mkey).ki,(mkey).gi,\
                (mkey).ke,(mkey).ky,(mkey).fu)

#define SPRINTF_MKEY(str,mkey) \
        sprintf((str), "(%u %u %u %u %u %u %2u)",\
               (mkey).hi, (mkey).ka, (mkey).ki, (mkey).gi,\
               (mkey).ke, (mkey).ky, (mkey).fu)

#define FPRINTF_MKEY(stream,mkey) \
        fprintf((stream),"(%u %u %u %u %u %u %2u)",\
               (mkey).hi, (mkey).ka, (mkey).ki, (mkey).gi,\
               (mkey).ke, (mkey).ky, (mkey).fu)
/*
 *　局面表記utility
 *  strのサイズは表示内容にもよるが大きく取っておく必要あり。
 *  PR_BOARDで 600byte, PR_ALLで 10Kbyte
 */

int sdata_sprintf      (char *restrict str,
                        const sdata_t *sdata,
                        unsigned int flag);
int sdata_fprintf      (FILE *restrict stream,
                        const sdata_t *sdata,
                        unsigned int flag);

#define SDATA_PRINTF(s,f)    sdata_fprintf(stdout,(s),(f))
//PR_BASIC
#define PR_BOARD    (1<<0)
#define PR_ZKEY     (1<<1)
#define PR_FFLAG    (1<<2)
#define PR_OU       (1<<3)
#define PR_OUTE     (1<<4)
#define PR_SCORE    (1<<5)
//PR_BITBOARD
#define PR_BBFU     (1<<6)
#define PR_BBKY     (1<<7)
#define PR_BBKE     (1<<8)
#define PR_BBGI     (1<<9)
#define PR_BBKI     (1<<10)
#define PR_BBKA     (1<<11)
#define PR_BBHI     (1<<12)
#define PR_BBTO     (1<<13)
#define PR_BBNY     (1<<14)
#define PR_BBNK     (1<<15)
#define PR_BBNG     (1<<16)
#define PR_BBUM     (1<<17)
#define PR_BBRY     (1<<18)
#define PR_BBTK     (1<<19)
#define PR_BBUK     (1<<20)
#define PR_BBRH     (1<<21)
#define PR_OCCUP    (1<<22)
//PR_MISC
#define PR_EFFU     (1<<23)
#define PR_EFFECT   (1<<24)
#define PR_PINNED   (1<<25)

#define PR_BASIC    (PR_BOARD|PR_ZKEY|PR_FFLAG|PR_OU|PR_OUTE|PR_SCORE)
#define PR_BITBOARD (PR_BBFU|PR_BBKY|PR_BBKE|PR_BBGI|PR_BBKI|PR_BBKA|PR_BBHI|\
                     PR_BBTO|PR_BBNY|PR_BBNK|PR_BBNG|PR_BBUM|PR_BBRY|PR_BBTK|\
                     PR_BBUK|PR_BBRH|PR_OCCUP)
#define PR_MISC     (PR_EFFU|PR_EFFECT|PR_PINNED)
#define PR_ALL      (PR_BASIC|PR_BITBOARD|PR_MISC)

/*
 * 着手表示utility
 */
int move_sprintf    (char *restrict str,
                     move_t move,
                     const sdata_t *sdata);
int move_fprintf    (FILE *restrict stream,
                     move_t move,
                     const sdata_t *sdata);
#define MOVE_PRINTF(move,sdata)     move_fprintf(stdout,(move),(sdata))

/*
 * bod形式盤面表記
 */
int bod_sprintf     (char *restrict str,
                     const sdata_t *sdata);
int bod_fprintf     (FILE *restrict stream,
                     const sdata_t *sdata);

/*
 * kif形式着手表記
 */
int kif_move_sprintf (char *restrict str,
                      const sdata_t *sdata,
                      move_t         move,
                      move_t         prev,
                      clock_t       tm_mv,
                      clock_t       tm_all);

int kif_move_fprintf (FILE *restrict stream,
                      const sdata_t *sdata,
                      move_t         move,
                      move_t         prev,
                      clock_t        tm_mv,
                      clock_t        tm_all);


#endif /* dtools_h */
