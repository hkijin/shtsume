//
//  dtools.c
//  shshogi
//
//  Created by Hkijin on 2021/11/05.
//

#include <stdio.h>
#include <time.h>
#include "shogi.h"
#include "dtools.h"

/* ------------
 グローバル変数
 ------------ */
char *g_koma_str [N_KOMAS]  = {
    " ・"," 歩"," 香"," 桂"," 銀"," 金"," 角"," 飛",
    " 玉"," と"," 杏"," 圭"," 全"," 偽"," 馬"," 龍",
    " 誤","v歩","v香","v桂","v銀","v金","v角","v飛",
    "v玉","vと","v杏","v圭","v全","v偽","v馬","v龍",
};

char *g_kif_str [N_KOMAS] = {
    "","歩","香","桂","銀","金","角","飛","玉",
    "と","成香","成桂","成銀","誤","馬","龍",
    "","歩","香","桂","銀","金","角","飛","玉",
    "と","成香","成桂","成銀","誤","馬","龍"
};

char *g_kif_str2[N_KATTR]  = {
    "玉","歩","香","桂","銀","金","角","飛"
};
char *g_koma_str2[N_KATTR]  = {
    " 玉"," 歩"," 香"," 桂"," 銀"," 金"," 角"," 飛",
};

char *g_num_str  [] = {
    "１","２","３","４","５","６","７","８","９"
};

char *g_jnum_str [N_JNUM]   = {
    "","一","二","三","四","五","六","七","八","九",
    "十","十一","十二","十三","十四","十五","十六","十七","十八"
};
/*
static int sprint_elapsed(char *restrict str,
                          clock_t        tm_mv,
                          clock_t        tm_all);
 */
static int fprint_elapsed(FILE *restrict stream,
                          clock_t        tm_mv,
                          clock_t        tm_all);

static int sprint_fflag(char *restrict str,    flag_t fflag);
static int fprint_fflag(FILE *restrict stream, flag_t fflag);
static int _bitboard_sprintf(char *restrict str,
                             const bitboard_t *bb,
                             const bitboard_t *bpos);
static int _bitboard_fprintf(FILE *restrict stream,
                             const bitboard_t *bb,
                             const bitboard_t *bpos);

int bitboard_sprintf (char *restrict str, const bitboard_t  *bb){
    return _bitboard_sprintf(str, bb, g_bpos);
}
int vbitboard_sprintf(char *restrict str, const bitboard_t *vbb){
    return _bitboard_sprintf(str, vbb, g_vpos);
}
int rbitboard_sprintf(char *restrict str, const bitboard_t *rbb){
    return _bitboard_sprintf(str, rbb, g_rpos);
}
int lbitboard_sprintf(char *restrict str, const bitboard_t *lbb){
    return _bitboard_sprintf(str, lbb, g_lpos);
}


int bitboard_fprintf (FILE *restrict stream, const bitboard_t  *bb){
    return _bitboard_fprintf(stream, bb, g_bpos);
}
int vbitboard_fprintf(FILE *restrict stream, const bitboard_t *vbb){
    return _bitboard_fprintf(stream, vbb, g_vpos);
}
int rbitboard_fprintf(FILE *restrict stream, const bitboard_t *rbb){
    return _bitboard_fprintf(stream, rbb, g_rpos);
}
int lbitboard_fprintf(FILE *restrict stream, const bitboard_t *lbb){
    return _bitboard_fprintf(stream, lbb, g_lpos);
}

/*
 *  sdata_sprintf,sdata_fprintf
 *  局面情報を表示する。フラグ指定により、sdata_t構造体の各メンバーの表示ができる。
 *  [引数]
 *  str     情報を書き込む文字列のポインタまたはストリーム
 *  stream
 *  sdata   局面データ
 *  flag    表示内容の指定フラグ
 *  [戻り値] 文字数
 */
int sdata_sprintf(char *restrict str,
                  const sdata_t *sdata,
                  unsigned int flag){
    int num = 0;
    if(flag & PR_BOARD ) {
        int rank,file;
        komainf_t koma;
        num += sprintf(str+num,
                       "手数 = %d\n手番 = %s\n後手:",
                       S_COUNT(sdata), S_TURN(sdata)?"後手":"先手");
        num += sprint_mkoma(str+num, S_GMKEY(sdata));
        num+=sprintf(str+num, "  ９ ８ ７ ６ ５ ４ ３ ２ １\n"
                              "+-------------------------+\n");
        for(rank=0; rank<N_RANK; rank++){
            num += sprintf(str+num,"|");
            for(file=8; file>=0; file--){
                koma = S_BOARD(sdata, rank*N_RANK+file);
                num += sprintf(str+num,  "%s", g_koma_str[koma]);
            }
            num += sprintf(str+num, " | %s\n", g_jnum_str[rank+1]);
        }
        num += sprintf(str+num, "+-------------------------+\n先手:");
        num += sprint_mkoma(str+num, S_SMKEY(sdata));
    }
    if(flag & PR_ZKEY  ) {
        num += sprintf(str+num, "ZKEY = 0X%llX\n", sdata->zkey);
    }
    if(flag & PR_FFLAG ) {
        num += sprintf(str+num, "fflag:");
        num += sprint_fflag(str+num, sdata->fflag);
    }
    if(flag & PR_OU    ) {
        num += sprintf(str+num, "先手玉:");
        if(S_SOU(sdata) == HAND)
            num += sprintf(str+num, "なし ");
        else
            num += sprintf(str+num, "(%u%u) ",
                           S_SOU(sdata)%9+1, S_SOU(sdata)/9+1);
        num += sprintf(str+num, "後手玉:");
        if(S_GOU(sdata) == HAND)
            num += sprintf(str+num, "なし\n");
        else
            num += sprintf(str+num, "(%u%u)\n",
                           S_GOU(sdata)%9+1, S_GOU(sdata)/9+1);
    }
    if(flag & PR_OUTE  ) {
        num += sprintf(str+num, "n_oute = %d:", S_NOUTE(sdata));
        for(int i=0; i<S_NOUTE(sdata); i++)
            num += sprintf(str+num,"%d ", S_ATTACK(sdata)[i]);
        num += sprintf(str+num, "\n");
    }
#ifdef SDATA_EXTENTION
    if(flag & PR_SCORE ) {
        num += sprintf(str+num, "k_score = %d    np  %d:%d\n ",
                       S_KSCORE(sdata),S_SNP(sdata),S_GNP(sdata));
    }
#endif //SDATA_EXTENTION
    if(flag & PR_BBFU  ) {
        num += sprintf(str+num, "bb_fu[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SFU(sdata));
        num += sprintf(str+num, "bb_fu[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GFU(sdata));
    }
    if(flag & PR_BBKY  ) {
        num += sprintf(str+num, "bb_ky[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SKY(sdata));
        num += sprintf(str+num, "bb_ky[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GKY(sdata));
    }
    if(flag & PR_BBKE  ) {
        num += sprintf(str+num, "bb_ke[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SKE(sdata));
        num += sprintf(str+num, "bb_ke[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GKE(sdata));
    }
    if(flag & PR_BBGI  ) {
        num += sprintf(str+num, "bb_gi[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SGI(sdata));
        num += sprintf(str+num, "bb_gi[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GGI(sdata));
    }
    if(flag & PR_BBKI  ) {
        num += sprintf(str+num, "bb_ki[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SKI(sdata));
        num += sprintf(str+num, "bb_ki[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GKI(sdata));
    }
    if(flag & PR_BBKA  ) {
        num += sprintf(str+num, "bb_ka[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SKA(sdata));
        num += sprintf(str+num, "bb_ka[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GKA(sdata));
    }
    if(flag & PR_BBHI  ) {
        num += sprintf(str+num, "bb_hi[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SHI(sdata));
        num += sprintf(str+num, "bb_hi[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GHI(sdata));
    }
    if(flag & PR_BBTO  ) {
        num += sprintf(str+num, "bb_to[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_STO(sdata));
        num += sprintf(str+num, "bb_to[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GTO(sdata));
    }
    if(flag & PR_BBNY  ) {
        num += sprintf(str+num, "bb_ny[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SNY(sdata));
        num += sprintf(str+num, "bb_ny[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GNY(sdata));
    }
    if(flag & PR_BBNK  ) {
        num += sprintf(str+num, "bb_nk[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SNK(sdata));
        num += sprintf(str+num, "bb_nk[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GNK(sdata));
    }
    if(flag & PR_BBNG  ) {
        num += sprintf(str+num, "bb_ng[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SNG(sdata));
        num += sprintf(str+num, "bb_ng[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GNG(sdata));
    }
    if(flag & PR_BBUM  ) {
        num += sprintf(str+num, "bb_um[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SUM(sdata));
        num += sprintf(str+num, "bb_um[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GUM(sdata));
    }
    if(flag & PR_BBRY  ) {
        num += sprintf(str+num, "bb_ry[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SRY(sdata));
        num += sprintf(str+num, "bb_ry[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GRY(sdata));
    }
    if(flag & PR_BBTK  ) {
        num += sprintf(str+num, "bb_tk[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_STK(sdata));
        num += sprintf(str+num, "bb_tk[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GTK(sdata));
    }
    if(flag & PR_BBUK  ) {
        num += sprintf(str+num, "bb_uk[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SUK(sdata));
        num += sprintf(str+num, "bb_uk[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GUK(sdata));
    }
    if(flag & PR_BBRH  ) {
        num += sprintf(str+num, "bb_rh[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SRH(sdata));
        num += sprintf(str+num, "bb_rh[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GRH(sdata));
    }
    if(flag & PR_OCCUP ) {
        num += sprintf(str+num, "occupied:\n");
        num += bitboard_sprintf(str+num, &BB_OCC(sdata));
        num += sprintf(str+num, "occup[SENTE]:\n");
        num += bitboard_sprintf(str+num, &BB_SOC(sdata));
        num += sprintf(str+num, "occup[GOTE] :\n");
        num += bitboard_sprintf(str+num, &BB_GOC(sdata));
    }
    if(flag & PR_EFFU  ) {
        num += sprintf(str+num, "ef_fu[SENTE]:\n");
        num += bitboard_sprintf(str+num, &EF_SFU(sdata));
        num += sprintf(str+num, "ef_fu[GOTE] :\n");
        num += bitboard_sprintf(str+num, &EF_GFU(sdata));
    }
    if(flag & PR_EFFECT) {
        num += sprintf(str+num, "effect[SENTE]:\n");
        num += bitboard_sprintf(str+num, &SEFFECT(sdata));
        num += sprintf(str+num, "effect[GOTE] :\n");
        num += bitboard_sprintf(str+num, &GEFFECT(sdata));
    }
    if(flag & PR_PINNED) {
        char pin_id[5] = {0,0,0,0,0};
        int pin_num=0;
        int i, pos;
        //Map
        num += sprintf(str+num, "pinned:\n  9 8 7 6 5 4 3 2 1\n");
        int rank, file;
        for(rank=0; rank<N_RANK; rank++){
            num += sprintf(str+num,"%d",rank+1);
            for(file=8; file>=0; file--){
                pos = rank*N_FILE+file;
                if(S_PINNED(sdata)[pos]){
                    pin_id[pin_num] = S_PINNED(sdata)[pos];
                    pin_num++;
                }
                num += sprintf(num+str, "%2d", S_PINNED(sdata)[pos]);
            }
            num += sprintf(str+num, "\n");
        }
        //bitboard
        for(i=0; i<pin_num; i++){
            num += sprintf(str+num, "pinned %d:\n", pin_id[i]);
            num += bitboard_sprintf(str+num, &g_bb_pin[pin_id[i]]);
        }
    }
    return num;
}

int sdata_fprintf(FILE *restrict stream,
                  const sdata_t *sdata,
                  unsigned int flag){
    int num = 0;
    if(flag & PR_BOARD ) {
        int rank,file;
        komainf_t koma;
        num += fprintf(stream,
                       "手数 = %d\n手番 = %s\n後手:",
                       S_COUNT(sdata), S_TURN(sdata)?"後手":"先手");
        num += fprint_mkoma(stream, S_GMKEY(sdata));
        num+=fprintf(stream, "  ９ ８ ７ ６ ５ ４ ３ ２ １\n"
                              "+-------------------------+\n");
        for(rank=0; rank<N_RANK; rank++){
            num += fprintf(stream,"|");
            for(file=8; file>=0; file--){
                koma = S_BOARD(sdata, rank*N_RANK+file);
                num += fprintf(stream,  "%s", g_koma_str[koma]);
            }
            num += fprintf(stream, " | %s\n", g_jnum_str[rank+1]);
        }
        num += fprintf(stream, "+-------------------------+\n先手:");
        num += fprint_mkoma(stream, S_SMKEY(sdata));
    }
    if(flag & PR_ZKEY  ) {
        num += fprintf(stream, "ZKEY = 0X%llX\n", sdata->zkey);
    }
    if(flag & PR_FFLAG ) {
        num += fprintf(stream, "fflag:");
        num += fprint_fflag(stream, S_FFLAG(sdata));
    }
    if(flag & PR_OU    ) {
        num += fprintf(stream, "先手玉:");
        if(S_SOU(sdata) == HAND)
            num += fprintf(stream, "なし ");
        else
            num += fprintf(stream, "(%u%u) ",
                           S_SOU(sdata)%9+1, S_SOU(sdata)/9+1);
        num += fprintf(stream, "後手玉:");
        if(S_GOU(sdata) == HAND)
            num += fprintf(stream, "なし\n");
        else
            num += fprintf(stream, "(%u%u)\n",
                           S_GOU(sdata)%9+1, S_GOU(sdata)/9+1);
    }
    if(flag & PR_OUTE  ) {
        num += fprintf(stream, "n_oute = %d:", S_NOUTE(sdata));
        for(int i=0; i<S_NOUTE(sdata); i++)
            num += fprintf(stream,"%d ", S_ATTACK(sdata)[i]);
        num += fprintf(stream, "\n");
    }
#ifdef SDATA_EXTENTION
    if(flag & PR_SCORE ) {
        num += fprintf(stream, "k_score = %d    np  %d:%d\n ",
                       S_KSCORE(sdata),S_SNP(sdata),S_GNP(sdata));
    }
#endif //SDATA_EXTENTION
    if(flag & PR_BBFU  ) {
        num += fprintf(stream, "bb_fu[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SFU(sdata));
        num += fprintf(stream, "bb_fu[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GFU(sdata));
    }
    if(flag & PR_BBKY  ) {
        num += fprintf(stream, "bb_ky[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SKY(sdata));
        num += fprintf(stream, "bb_ky[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GKY(sdata));
    }
    if(flag & PR_BBKE  ) {
        num += fprintf(stream, "bb_ke[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SKE(sdata));
        num += fprintf(stream, "bb_ke[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GKE(sdata));
    }
    if(flag & PR_BBGI  ) {
        num += fprintf(stream, "bb_gi[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SGI(sdata));
        num += fprintf(stream, "bb_gi[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GGI(sdata));
    }
    if(flag & PR_BBKI  ) {
        num += fprintf(stream, "bb_ki[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SKI(sdata));
        num += fprintf(stream, "bb_ki[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GKI(sdata));
    }
    if(flag & PR_BBKA  ) {
        num += fprintf(stream, "bb_ka[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SKA(sdata));
        num += fprintf(stream, "bb_ka[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GKA(sdata));
    }
    if(flag & PR_BBHI  ) {
        num += fprintf(stream, "bb_hi[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SHI(sdata));
        num += fprintf(stream, "bb_hi[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GHI(sdata));
    }
    if(flag & PR_BBTO  ) {
        num += fprintf(stream, "bb_to[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_STO(sdata));
        num += fprintf(stream, "bb_to[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GTO(sdata));
    }
    if(flag & PR_BBNY  ) {
        num += fprintf(stream, "bb_ny[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SNY(sdata));
        num += fprintf(stream, "bb_ny[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GNY(sdata));
    }
    if(flag & PR_BBNK  ) {
        num += fprintf(stream, "bb_nk[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SNK(sdata));
        num += fprintf(stream, "bb_nk[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GNK(sdata));
    }
    if(flag & PR_BBNG  ) {
        num += fprintf(stream, "bb_ng[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SNG(sdata));
        num += fprintf(stream, "bb_ng[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GNG(sdata));
    }
    if(flag & PR_BBUM  ) {
        num += fprintf(stream, "bb_um[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SUM(sdata));
        num += fprintf(stream, "bb_um[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GUM(sdata));
    }
    if(flag & PR_BBRY  ) {
        num += fprintf(stream, "bb_ry[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SRY(sdata));
        num += fprintf(stream, "bb_ry[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GRY(sdata));
    }
    if(flag & PR_BBTK  ) {
        num += fprintf(stream, "bb_tk[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_STK(sdata));
        num += fprintf(stream, "bb_tk[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GTK(sdata));
    }
    if(flag & PR_BBUK  ) {
        num += fprintf(stream, "bb_uk[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SUK(sdata));
        num += fprintf(stream, "bb_uk[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GUK(sdata));
    }
    if(flag & PR_BBRH  ) {
        num += fprintf(stream, "bb_rh[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SRH(sdata));
        num += fprintf(stream, "bb_rh[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GRH(sdata));
    }
    if(flag & PR_OCCUP ) {
        num += fprintf(stream, "occupied:\n");
        num += bitboard_fprintf(stream, &BB_OCC(sdata));
        num += fprintf(stream, "occup[SENTE]:\n");
        num += bitboard_fprintf(stream, &BB_SOC(sdata));
        num += fprintf(stream, "occup[GOTE] :\n");
        num += bitboard_fprintf(stream, &BB_GOC(sdata));
    }
    if(flag & PR_EFFU  ) {
        num += fprintf(stream, "ef_fu[SENTE]:\n");
        num += bitboard_fprintf(stream, &EF_SFU(sdata));
        num += fprintf(stream, "ef_fu[GOTE] :\n");
        num += bitboard_fprintf(stream, &EF_GFU(sdata));
    }
    if(flag & PR_EFFECT) {
        num += fprintf(stream, "effect[SENTE]:\n");
        num += bitboard_fprintf(stream, &SEFFECT(sdata));
        num += fprintf(stream, "effect[GOTE] :\n");
        num += bitboard_fprintf(stream, &GEFFECT(sdata));
    }
    if(flag & PR_PINNED) {
        char pin_id[5] = {0,0,0,0,0};
        int pin_num=0;
        int i, pos;
        //Map
        num += fprintf(stream, "pinned:\n  9 8 7 6 5 4 3 2 1\n");
        int rank, file;
        for(rank=0; rank<N_RANK; rank++){
            num += fprintf(stream,"%d",rank+1);
            for(file=8; file>=0; file--){
                pos = rank*N_FILE+file;
                if(S_PINNED(sdata)[pos]){
                    pin_id[pin_num] = S_PINNED(sdata)[pos];
                    pin_num++;
                }
                num += fprintf(stream, "%2d", S_PINNED(sdata)[pos]);
            }
            num += fprintf(stream, "\n");
        }
        //bitboard
        for(i=0; i<pin_num; i++){
            num += fprintf(stream, "pinned %d:\n", pin_id[i]);
            num += bitboard_fprintf(stream, &g_bb_pin[pin_id[i]]);
        }
    }
    return num;
}

/*
 * 着手表示utility
 * 表記はkif形式に準じる
 */
int move_sprintf    (char *restrict str,
                     move_t move,
                     const sdata_t *sdata)
{
    char *koma, *opt = "";
    int num;
    if(MV_DROP(move))
    {
        koma = g_kif_str[MV_HAND(move)];
        num = sprintf(str, "%s%s%s%s打",
                      (S_TURN(sdata))?"△":"▲",             //手番
                      g_num_str [g_file[NEW_POS(move)]],   //移動後筋(file)
                      g_jnum_str[g_rank[NEW_POS(move)]+1], //移動後段(rank)
                      koma );                              //駒
    }
    else
    {
        koma = g_kif_str[S_BOARD(sdata, PREV_POS(move))];
        if(PROMOTE(move)) opt = "成";
        num = sprintf(str, "%s%s%s%s%s(%u%u)",
                      (S_TURN(sdata))?"△":"▲",            //手番
                      g_num_str [g_file[NEW_POS(move)]],  //移動後筋(file)
                      g_jnum_str[g_rank[NEW_POS(move)]+1],//移動後段(rank)
                      koma, //駒
                      opt,  //成
                      g_file[PREV_POS(move)]+1,           //移動前筋(file)
                      g_rank[PREV_POS(move)]+1  );        //移動前段(rank)
    }
    return num;
}
int move_fprintf    (FILE *restrict stream,
                     move_t move,
                     const sdata_t *sdata)
{
    char *koma, *opt = "";
    int num;
    if(MV_DROP(move))
    {
        koma = g_kif_str[MV_HAND(move)];
        num = fprintf(stream, "%s%s%s%s打",
                      (S_TURN(sdata))?"△":"▲",             //手番
                      g_num_str [g_file[NEW_POS(move)]],   //移動後筋(file)
                      g_jnum_str[g_rank[NEW_POS(move)]+1], //移動後段(rank)
                      koma );                              //駒
    }
    else
    {
        koma = g_kif_str[S_BOARD(sdata, PREV_POS(move))];
        if(PROMOTE(move)) opt = "成";
        num = fprintf(stream, "%s%s%s%s%s(%u%u)",
                      (S_TURN(sdata))?"△":"▲",            //手番
                      g_num_str [g_file[NEW_POS(move)]],  //移動後筋(file)
                      g_jnum_str[g_rank[NEW_POS(move)]+1],//移動後段(rank)
                      koma, //駒
                      opt,  //成
                      g_file[PREV_POS(move)]+1,           //移動前筋(file)
                      g_rank[PREV_POS(move)]+1  );        //移動前段(rank)
    }
    return num;
}


/*
 *  スタティック関数実装部
 */
int _bitboard_sprintf       (char *restrict str,
                             const bitboard_t *bb,
                             const bitboard_t *bpos){
    int num=0;
    num+=sprintf(str+num,"  9 8 7 6 5 4 3 2 1\n");
    int rank, file, pos;
    bitboard_t res;
    for(rank=0; rank<9; rank++){
        num+=sprintf(str+num,"%d",rank+1);
        for(file=8; file>=0; file--){
            pos = rank*9+file;
            BB_AND(res,*bb,*(bpos+pos));
            num+=sprintf(str+num,"%s",(BB_TEST(res)?" *":" ."));
        }
        num+=sprintf(str+num,"\n");
    }
    return num;
}
int _bitboard_fprintf       (FILE *restrict stream,
                             const bitboard_t *bb,
                             const bitboard_t *bpos){
    int num=0;
    num+=fprintf(stream,"  9 8 7 6 5 4 3 2 1\n");
    int rank, file, pos;
    bitboard_t res;
    for(rank=0; rank<9; rank++){
        num+=fprintf(stream,"%d",rank+1);
        for(file=8; file>=0; file--){
            pos = rank*9+file;
            BB_AND(res,*bb,*(bpos+pos));
            num+=fprintf(stream, "%s",(BB_TEST(res)?" *":" ."));
        }
        num+=fprintf(stream,"\n");
    }
    return num;
}

int sprint_mkoma(char *restrict str,    const mkey_t mkey){
    int i,num=0;
    unsigned int u;
    for(i=HI; i>=0; i--){
        u = mkoma_num(mkey, i);
        if(u){
            num+=sprintf(str+num,"%s", g_koma_str2[i]);
            if (u>1) {
                num+=sprintf(str+num,"%s", g_jnum_str[u]);
            }
        }
    }
    num+=sprintf(str+num,"\n");
    return num;
}
int fprint_mkoma(FILE *restrict stream, const mkey_t mkey){
    int i,num=0;
    unsigned int u;
    for(i=HI; i>=0; i--){
        u = mkoma_num(mkey, i);
        if(u){
            num+=fprintf(stream,"%s", g_koma_str2[i]);
            if (u>1) {
                num+=fprintf(stream,"%s", g_jnum_str[u]);
            }
        }
    }
    num+=fprintf(stream,"\n");
    return num;
}

/* ---------------------------------------------------
 以下のformatの棋譜記載用文字列を生成する
 (mmmm:ss/hh:mm:ss)
 
 --------------------------------------------------- */
/*
int sprint_elapsed(char *restrict str,
                   clock_t        tm_mv,
                   clock_t        tm_all)
{
    int num = 0;
    unsigned long time = tm_mv/CLOCKS_PER_SEC;    //経過時間の秒表示
    unsigned long fmm  = time/60;                  //前半：分表示
    unsigned long fss  = time%60;                  //前半：秒表示
    unsigned long bhh  = tm_all/3600;              //後半：時間表示
    unsigned long btime = tm_all%3600;
    unsigned long bmm = btime/60;                  //後半：分表示
    unsigned long bss = btime%60;                  //後半：秒表示
    num += sprintf(str,"(%2lu:%2lu / %2lu:%2lu:%2lu)",fmm,fss,bhh,bmm,bss);
    return num;
}
*/
int fprint_elapsed(FILE *restrict stream,
                   clock_t        tm_mv,
                   clock_t        tm_all)
{
    int num = 0;
    unsigned long time = tm_mv/CLOCKS_PER_SEC;     //経過時間の秒表示
    unsigned long fmm  = time/60;                  //前半：分表示
    unsigned long fss  = time%60;                  //前半：秒表示
    time = tm_all/CLOCKS_PER_SEC;
    unsigned long bhh = time/3600;                //後半：時間表示
    unsigned long btime = time%3600;
    unsigned long bmm = btime/60;                  //後半：分表示
    unsigned long bss = btime%60;                  //後半：秒表示
    num +=
    fprintf(stream,"(%02lu:%02lu / %02lu:%02lu:%02lu)",fmm,fss,bhh,bmm,bss);
    return num;
}

int sprint_fflag(char *restrict str,    flag_t fflag){
    int num=0;
    int i,j;
    for (i=0; i<2; i++) {
        for (j=8; j>=0; j--) {
            num += sprintf(str+num, "%s", (fflag&(1<<(i*9+j)))? "*":".");
        }
        num += sprintf(str+num, " ");
    }
    num += sprintf(str+num, "\n");
    return num;
}
int fprint_fflag(FILE *restrict stream, flag_t fflag){
    int num=0;
    int i,j;
    for (i=0; i<2; i++) {
        for (j=8; j>=0; j--) {
            num += fprintf(stream, "%s", (fflag&(1<<(i*9+j)))? "*":".");
        }
        num += fprintf(stream, " ");
    }
    num += fprintf(stream, "\n");
    return num;
}

int bod_sprintf     (char *restrict str,
                     const sdata_t *sdata)
{
    int num = 0;
    //後手の持ち駒
    num += sprintf(str+num,"後手の持駒：");
    if(S_GMKEY(sdata).hi){
        num += sprintf(str+num,"飛");
        if(S_GMKEY(sdata).hi>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_GMKEY(sdata).hi]);
        num += sprintf(str+num,"　");
    }
    if(S_GMKEY(sdata).ka){
        num += sprintf(str+num,"角");
        if(S_GMKEY(sdata).ka>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_GMKEY(sdata).ka]);
        num += sprintf(str+num,"　");
    }
    if(S_GMKEY(sdata).ki){
        num += sprintf(str+num,"金");
        if(S_GMKEY(sdata).ki>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_GMKEY(sdata).ki]);
        num += sprintf(str+num,"　");
    }
    if(S_GMKEY(sdata).gi){
        num += sprintf(str+num,"銀");
        if(S_GMKEY(sdata).gi>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_GMKEY(sdata).gi]);
        num += sprintf(str+num,"　");
    }
    if(S_GMKEY(sdata).ke){
        num += sprintf(str+num,"桂");
        if(S_GMKEY(sdata).ke>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_GMKEY(sdata).ke]);
        num += sprintf(str+num,"　");
    }
    if(S_GMKEY(sdata).ky){
        num += sprintf(str+num,"香");
        if(S_GMKEY(sdata).ky>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_GMKEY(sdata).ky]);
        num += sprintf(str+num,"　");
    }
    if(S_GMKEY(sdata).fu){
        num += sprintf(str+num,"歩");
        if(S_GMKEY(sdata).fu>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_GMKEY(sdata).fu]);
    }
    num += sprintf(str+num, "\n");
    //盤面
    num += sprintf(str+num,
                   "  ９ ８ ７ ６ ５ ４ ３ ２ １\n"
                   "+---------------------------+\n");
    int rank,file;
    komainf_t koma;
    for(rank=0; rank<N_RANK; rank++){
        num += sprintf(str+num,"|");
        for(file=8; file>=0; file--){
            koma = S_BOARD(sdata, rank*N_RANK+file);
            num += sprintf(str+num,  "%s", g_koma_str[koma]);
        }
        num += sprintf(str+num, "|%s\n", g_jnum_str[rank+1]);
    }
    num += sprintf(str+num,
                   "+---------------------------+\n");
    //先手の持ち駒
    num += sprintf(str+num,"先手の持駒：");
    if(S_SMKEY(sdata).hi){
        num += sprintf(str+num,"飛");
        if(S_SMKEY(sdata).hi>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_SMKEY(sdata).hi]);
        num += sprintf(str+num,"　");
    }
    if(S_SMKEY(sdata).ka){
        num += sprintf(str+num,"角");
        if(S_SMKEY(sdata).ka>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_SMKEY(sdata).ka]);
        num += sprintf(str+num,"　");
    }
    if(S_SMKEY(sdata).ki){
        num += sprintf(str+num,"金");
        if(S_SMKEY(sdata).ki>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_SMKEY(sdata).ki]);
        num += sprintf(str+num,"　");
    }
    if(S_SMKEY(sdata).gi){
        num += sprintf(str+num,"銀");
        if(S_SMKEY(sdata).gi>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_SMKEY(sdata).gi]);
        num += sprintf(str+num,"　");
    }
    if(S_SMKEY(sdata).ke){
        num += sprintf(str+num,"桂");
        if(S_SMKEY(sdata).ke>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_SMKEY(sdata).ke]);
        num += sprintf(str+num,"　");
    }
    if(S_SMKEY(sdata).ky){
        num += sprintf(str+num,"香");
        if(S_SMKEY(sdata).ky>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_SMKEY(sdata).ky]);
        num += sprintf(str+num,"　");
    }
    if(S_SMKEY(sdata).fu){
        num += sprintf(str+num,"歩");
        if(S_SMKEY(sdata).fu>1)
            num += sprintf(str+num,"%s", g_jnum_str[S_SMKEY(sdata).fu]);
    }
    num += sprintf(str+num, "\n");
    
    return num;
}

int bod_fprintf     (FILE *restrict stream,
                     const sdata_t *sdata)
{
    int num = 0;
    //後手の持ち駒
    num += fprintf(stream,"後手の持駒：");
    if(S_GMKEY(sdata).hi){
        num += fprintf(stream,"飛");
        if(S_GMKEY(sdata).hi>1)
            num += fprintf(stream,"%s", g_jnum_str[S_GMKEY(sdata).hi]);
        num += fprintf(stream,"　");
    }
    if(S_GMKEY(sdata).ka){
        num += fprintf(stream,"角");
        if(S_GMKEY(sdata).ka>1)
            num += fprintf(stream,"%s", g_jnum_str[S_GMKEY(sdata).ka]);
        num += fprintf(stream,"　");
    }
    if(S_GMKEY(sdata).ki){
        num += fprintf(stream,"金");
        if(S_GMKEY(sdata).ki>1)
            num += fprintf(stream,"%s", g_jnum_str[S_GMKEY(sdata).ki]);
        num += fprintf(stream,"　");
    }
    if(S_GMKEY(sdata).gi){
        num += fprintf(stream,"銀");
        if(S_GMKEY(sdata).gi>1)
            num += fprintf(stream,"%s", g_jnum_str[S_GMKEY(sdata).gi]);
        num += fprintf(stream,"　");
    }
    if(S_GMKEY(sdata).ke){
        num += fprintf(stream,"桂");
        if(S_GMKEY(sdata).ke>1)
            num += fprintf(stream,"%s", g_jnum_str[S_GMKEY(sdata).ke]);
        num += fprintf(stream,"　");
    }
    if(S_GMKEY(sdata).ky){
        num += fprintf(stream,"香");
        if(S_GMKEY(sdata).ky>1)
            num += fprintf(stream,"%s", g_jnum_str[S_GMKEY(sdata).ky]);
        num += fprintf(stream,"　");
    }
    if(S_GMKEY(sdata).fu){
        num += fprintf(stream,"歩");
        if(S_GMKEY(sdata).fu>1)
            num += fprintf(stream,"%s", g_jnum_str[S_GMKEY(sdata).fu]);
    }
    num += fprintf(stream, "\n");
    //盤面
    num += fprintf(stream,
                   "  ９ ８ ７ ６ ５ ４ ３ ２ １\n"
                   "+---------------------------+\n");
    int rank,file;
    komainf_t koma;
    for(rank=0; rank<N_RANK; rank++){
        num += fprintf(stream,"|");
        for(file=8; file>=0; file--){
            koma = S_BOARD(sdata, rank*N_RANK+file);
            num += fprintf(stream,  "%s", g_koma_str[koma]);
        }
        num += fprintf(stream, "|%s\n", g_jnum_str[rank+1]);
    }
    num += fprintf(stream,
                   "+---------------------------+\n");
    //先手の持ち駒
    num += fprintf(stream,"先手の持駒：");
    if(S_SMKEY(sdata).hi){
        num += fprintf(stream,"飛");
        if(S_SMKEY(sdata).hi>1)
            num += fprintf(stream,"%s", g_jnum_str[S_SMKEY(sdata).hi]);
        num += fprintf(stream,"　");
    }
    if(S_SMKEY(sdata).ka){
        num += fprintf(stream,"角");
        if(S_SMKEY(sdata).ka>1)
            num += fprintf(stream,"%s", g_jnum_str[S_SMKEY(sdata).ka]);
        num += fprintf(stream,"　");
    }
    if(S_SMKEY(sdata).ki){
        num += fprintf(stream,"金");
        if(S_SMKEY(sdata).ki>1)
            num += fprintf(stream,"%s", g_jnum_str[S_SMKEY(sdata).ki]);
        num += fprintf(stream,"　");
    }
    if(S_SMKEY(sdata).gi){
        num += fprintf(stream,"銀");
        if(S_SMKEY(sdata).gi>1)
            num += fprintf(stream,"%s", g_jnum_str[S_SMKEY(sdata).gi]);
        num += fprintf(stream,"　");
    }
    if(S_SMKEY(sdata).ke){
        num += fprintf(stream,"桂");
        if(S_SMKEY(sdata).ke>1)
            num += fprintf(stream,"%s", g_jnum_str[S_SMKEY(sdata).ke]);
        num += fprintf(stream,"　");
    }
    if(S_SMKEY(sdata).ky){
        num += fprintf(stream,"香");
        if(S_SMKEY(sdata).ky>1)
            num += fprintf(stream,"%s", g_jnum_str[S_SMKEY(sdata).ky]);
        num += fprintf(stream,"　");
    }
    if(S_SMKEY(sdata).fu){
        num += fprintf(stream,"歩");
        if(S_SMKEY(sdata).fu>1)
            num += fprintf(stream,"%s", g_jnum_str[S_SMKEY(sdata).fu]);
    }
    num += fprintf(stream, "\n");
    
    return num;
}

/*
 * kif形式着手表記
 */
int kif_move_sprintf (char *restrict str,
                      const sdata_t *sdata,
                      move_t         move,
                      move_t         prev,
                      clock_t        tm_mv,
                      clock_t        tm_all)
{
    int num = 0;
    int file, rank;
    //着手ID
    num += sprintf(str+num, "%4u ", S_COUNT(sdata)+1);
    if(MV_DROP(move)){
        //位置
        file = g_file[NEW_POS(move)];
        rank = g_rank[NEW_POS(move)]+1;
        num += sprintf(str+num, "%s%s", g_num_str[file], g_jnum_str[rank]);
        //駒打
        num += sprintf(str+num, "%s打", g_kif_str2[MV_HAND(move)]);
        //時間（ダミー)+改行
        num += sprintf(str+num, "      (00:00 / 00:00:00)\n");
    }
    else{
        //同
        if(NEW_POS(move)==NEW_POS(prev))
        {
            num += sprintf(str+num, "同　");
        }
        else
        {
            //位置
            file = g_file[NEW_POS(move)];
            rank = g_rank[NEW_POS(move)]+1;
            num += sprintf(str+num, "%s%s",g_num_str[file],g_jnum_str[rank]);
        }
        //移動駒
        komainf_t koma = S_BOARD(sdata,PREV_POS(move));
        num += sprintf(str+num, "%s", g_kif_str[koma]);
        //成り
        if(PROMOTE(move))
            num += sprintf(str+num, "成");
        //移動元
        file = g_file[PREV_POS(move)]+1;
        rank = g_rank[PREV_POS(move)]+1;
        num += sprintf(str+num, "(%u%u)",file,rank);
        //時間（ダミー)+改行
        if(PROMOTE(move)){
            num += sprintf(str+num, "  (00:00 / 00:00:00)\n");
        }
        else{
            num += sprintf(str+num, "    (00:00 / 00:00:00)\n");
        }
    }
    return num;
}

int kif_move_fprintf (FILE *restrict stream,
                      const sdata_t *sdata,
                      move_t         move,
                      move_t         prev,
                      clock_t        tm_mv,
                      clock_t        tm_all)
{
    int num = 0;
    int file, rank;
 
    //着手ID
    num += fprintf(stream, "%4u ", S_COUNT(sdata)+1);
    if(MV_DROP(move)){
        //位置
        file = g_file[NEW_POS(move)];
        rank = g_rank[NEW_POS(move)]+1;
        num += fprintf(stream, "%s%s", g_num_str[file], g_jnum_str[rank]);
        //駒打
        num += fprintf(stream, "%s打", g_kif_str2[MV_HAND(move)]);
        
        //時間（ダミー)+改行
        //num += fprintf(stream, "      (00:00 / 00:00:00)\n");
        
        num += fprintf(stream, "      ");
        num += fprint_elapsed(stream, tm_mv, tm_all);
        num += fprintf(stream, "\n");
        
    }
    else{
        //同
        if(NEW_POS(move)==NEW_POS(prev))
        {
            num += fprintf(stream, "同　");
        }
        else
        {
            //位置
            file = g_file[NEW_POS(move)];
            rank = g_rank[NEW_POS(move)]+1;
            num += fprintf(stream, "%s%s",g_num_str[file],g_jnum_str[rank]);
        }
        //移動駒
        komainf_t koma = S_BOARD(sdata,PREV_POS(move));
        num += fprintf(stream, "%s", g_kif_str[koma]);
        //成り
        if(PROMOTE(move)){
            num += fprintf(stream, "成");
        }
        //移動元
        file = g_file[PREV_POS(move)]+1;
        rank = g_rank[PREV_POS(move)]+1;
        num += fprintf(stream, "(%u%u)",file,rank);
        //時間（ダミー)+改行
        if(PROMOTE(move)){
            //num += fprintf(stream, "  (00:00 / 00:00:00)\n");
            
            num += fprintf(stream, "  ");
            num += fprint_elapsed(stream, tm_mv, tm_all);
            num += fprintf(stream, "\n");
        }
        else{
            //num += fprintf(stream, "    (00:00 / 00:00:00)\n");
            
            num += fprintf(stream, "    ");
            num += fprint_elapsed(stream, tm_mv, tm_all);
            num += fprintf(stream, "\n");
        }
    }
    return num;
}

