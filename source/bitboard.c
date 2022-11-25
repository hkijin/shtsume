//
//  bitboard.c
//  shshogi
//
//  Created by Hkijin on 2021/10/19.
//

#include <stdio.h>
#include <string.h>
#include "shogi.h"

/* ------------------------------------------------------------------------
 [スタティック変数]
 st_***_effect  駒の利きを表すbitboardを駒種および位置別に保管しているtable。
                初期化が必要。飛角香などの飛駒の利きは盤面配置に依存するためmaskを使用
                して駒配置情報を読み取り、対応した利きを返す。
 st_*mask[]     飛角香などの利きを求めるためのマスク。盤の一路内側の7*7bitの縦横斜めの
                占有bitの並びをkeyとして表引きで求める
 例1) st_hmask[RANK2]         例2) st_vmask[FILE2]
   9 8 7 6 5 4 3 2 1            9 8 7 6 5 4 3 2 1
 +-------------------+        +-------------------+
 | . . . . . . . . . | 1      | . . . . . . . . . | 1
 | . * * * * * * * . | 2      | . . . . . . . * . | 2
 | . . . . . . . . . | 3      | . . . . . . . * . | 3
 | . . . . . . . . . | 4      | . . . . . . . * . | 4
 | . . . . . . . . . | 5      | . . . . . . . * . | 5
 | . . . . . . . . . | 6      | . . . . . . . * . | 6
 | . . . . . . . . . | 7      | . . . . . . . * . | 7
 | . . . . . . . . . | 8      | . . . . . . . * . | 8
 | . . . . . . . . . | 9      | . . . . . . . . . | 9
 +-------------------+        +-------------------+
 例3) st_rmask[6]              例4) st_lmask[8]
   9 8 7 6 5 4 3 2 1            9 8 7 6 5 4 3 2 1
 +-------------------+        +-------------------+
 | . . . . . . . . . | 1      | . . . . . . . . . | 1
 | . . . * . . . . . | 2      | . . . . . . . * . | 2
 | . . . . * . . . . | 3      | . . . . . . * . . | 3
 | . . . . . * . . . | 4      | . . . . . * . . . | 4
 | . . . . . . * . . | 5      | . . . . * . . . . | 5
 | . . . . . . . * . | 6      | . . . * . . . . . | 6
 | . . . . . . . . . | 7      | . . * . . . . . . | 7
 | . . . . . . . . . | 8      | . * . . . . . . . | 8
 | . . . . . . . . . | 9      | . . . . . . . . . | 9
 +-------------------+        +-------------------+
 
 st_*mask_pos[]    mask情報初期化のためのヘルパー変数
 st_*mask_offset[]
 
 ----------------------------------------------------------------------- */

static bitboard_t st_sky_effect[N_SQUARE][128];
static bitboard_t st_gky_effect[N_SQUARE][128];
static bitboard_t st_rka_effect[2946];
static bitboard_t st_lka_effect[2946];
static bitboard_t st_hhi_effect[N_SQUARE][128];
static bitboard_t st_vhi_effect[N_SQUARE][128];

static bitboard_t st_hmask[N_RANK];                   //HI horizontal
static bitboard_t st_vmask[N_FILE];                   //HI vertical
static bitboard_t st_rlmask[17];                      //KA RL-slope
static char st_hvmask_pos[9]     = {0,0,0,1,1,1,2,2,2};
static char st_hvmask_offset[9]  = {1,10,19,1,10,19,1,10,19};
static char st_rlmask_pos[17]    = {0,0,0,0,0,0,0,1,1,1,2,2,2,2,2,2,2};
static char st_rlmask_offset[17] = {0,0,4,7,11,16,22,2,10,19,0,7,13,18,22,0,0};

static short st_ka_index[N_SQUARE] = {
       0,
       1,   2,
       3,   5,   7,
       9,  13,  17,  21,
      25,  33,  41,  49,  57,
      65,  81,  97, 113, 129, 145,
     161, 193, 225, 257, 289, 321, 353,
     385, 449, 513, 577, 641, 705, 769, 833,
     897,1025,1153,1281,1409,1537,1665,1793,1921,
    2049,2113,2177,2241,2305,2369,2433,2497,
    2561,2593,2625,2657,2689,2721,2753,
    2785,2801,2817,2833,2849,2865,
    2881,2889,2897,2905,2913,
    2921,2925,2929,2933,
    2937,2939,2941,
    2943,2944,
    2945
};           //KA
static char st_hvindex[N_SQUARE] = {
    B11,B12,B13,B14,B15,B16,B17,B18,B19,
    B21,B22,B23,B24,B25,B26,B27,B28,B29,
    B31,B32,B33,B34,B35,B36,B37,B38,B39,
    B41,B42,B43,B44,B45,B46,B47,B48,B49,
    B51,B52,B53,B54,B55,B56,B57,B58,B59,
    B61,B62,B63,B64,B65,B66,B67,B68,B69,
    B71,B72,B73,B74,B75,B76,B77,B78,B79,
    B81,B82,B83,B84,B85,B86,B87,B88,B89,
    B91,B92,B93,B94,B95,B96,B97,B98,B99
};
static char st_rindex[N_SQUARE]  = {
     0, 1, 3, 6,10,15,21,28,36,
     2, 4, 7,11,16,22,29,37,45,
     5, 8,12,17,23,30,38,46,53,
     9,13,18,24,31,39,47,54,60,
    14,19,25,32,40,48,55,61,66,
    20,26,33,41,49,56,62,67,71,
    27,34,42,50,57,63,68,72,75,
    35,43,51,58,64,69,73,76,78,
    44,52,59,65,70,74,77,79,80
};             //KA

static char st_lindex[N_SQUARE]  = {
    36,28,21,15,10, 6, 3, 1, 0,
    45,37,29,22,16,11, 7, 4, 2,
    53,46,38,30,23,17,12, 8, 5,
    60,54,47,39,31,24,18,13, 9,
    66,61,55,48,40,32,25,19,14,
    71,67,62,56,49,41,33,26,20,
    75,72,68,63,57,50,42,34,27,
    78,76,73,69,64,58,51,43,35,
    80,79,77,74,70,65,59,52,44
};             //KA

// bitboardの変換
static bitboard_t vbb_to_bb(const bitboard_t *vbb); //horizontal <-> vertical
static bitboard_t rbb_to_bb(const bitboard_t *rbb); //right slpoe to horizontal
static bitboard_t lbb_to_bb(const bitboard_t *lbb); //left slope  to horizontal

static void create_base_table   (void);
static void create_hmask        (void);
static void create_vmask        (void);
static void create_rlmask       (void);
static void create_ky_effect    (void);
static void create_ka_effect    (void);
static void create_hi_effect    (void);

static void effect_sky      (bitboard_t *bb,
                             int pos,
                             const bitboard_t *voccupied);
static void effect_gky      (bitboard_t *bb,
                             int pos,
                             const bitboard_t *voccupied);
static void effect_ka       (bitboard_t *bb,
                             int pos,
                             const bitboard_t *roccupied,
                             const bitboard_t *loccupied);
static void effect_um       (bitboard_t *bb,
                             int pos,
                             const bitboard_t *roccupied,
                             const bitboard_t *loccupied);
static void effect_hi       (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied,
                             const bitboard_t *voccupied);
static void effect_ry       (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied,
                             const bitboard_t *voccupied);

static void effect_sky_emu  (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied);
static void effect_gky_emu  (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied);
static void effect_rka_emu  (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied);
static void effect_lka_emu  (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied);
static void effect_ka_emu   (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied);
static void effect_um_emu   (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied);
static void effect_hhi_emu  (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied);
static void effect_vhi_emu  (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied);
static void effect_hi_emu   (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied);
static void effect_ry_emu   (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied);

/* --------------------
 パブリック関数の実装部
 -------------------- */

/*
 min_pos, max_pos
 bitboardより立っているbit位置idを返す.
 min_posは最小のid, max_posは最大のidを返す。
 引数　: const bitboard_t bitboard
 戻り値: bitboardのbitが立っている場合　bit位置id( 0 - 80 )
        空のbitboard                -1
 */
int min_pos(const bitboard_t *bb){
    if(bb->pos[0]) return __builtin_ctz( bb->pos[0] );
    if(bb->pos[1]) return __builtin_ctz( bb->pos[1] )+27;
    if(bb->pos[2]) return __builtin_ctz( bb->pos[2] )+54;
    return -1;
}

int max_pos(const bitboard_t *bb){
    if(bb->pos[2]) return -__builtin_clz( bb->pos[2] )+85;
    if(bb->pos[1]) return -__builtin_clz( bb->pos[1] )+58;
    if(bb->pos[0]) return -__builtin_clz( bb->pos[0] )+31;
    return -1;
}

/*
 bb_to_effect
 複数のbit（4個以下)が立っている位置bitboardより利きbitboardを算出する
 引数　: komainf_t koma            　対象の駒
 　　　: const bitboard_t *bb_koma 　駒の位置を示すbitboard
　　　 : const sdata_t *sdata     　 局面データ
 戻り値: 利きbitboard
 */

bitboard_t    bb_to_effect (komainf_t koma,
                            const bitboard_t *bb_koma,
                            const sdata_t *sdata){
    int id;
    bitboard_t eff, bb = *bb_koma;
    BB_INI(eff);
    while(1){
        id = min_pos(&bb);
        if(id<0) break;
        BBA_OR(eff, EFFECT_TBL(id, koma, sdata));
        BBA_XOR(bb, g_bpos[id]);
    }
    return eff;
}

//初期化処理
void init_bpos(void){
    int i;
    for(i=0; i<N_SQUARE; i++){
        if(i<27)      g_bpos[i].pos[0] = 1U<<i;
        else if(i<54) g_bpos[i].pos[1] = 1U<<(i-27);
        else          g_bpos[i].pos[2] = 1U<<(i-54);
    }
    for(i=0; i<N_SQUARE; i++){
        g_vpos[i] = g_bpos[st_hvindex[i]];
        g_rpos[i] = g_bpos[st_rindex[i]];
        g_lpos[i] = g_bpos[st_lindex[i]];
    }
    return;
}

void init_effect(void){
    create_base_table();
    create_hmask();
    create_vmask();
    create_rlmask();
    create_ky_effect();
    create_ka_effect();
    create_hi_effect();
    return;
}

bitboard_t effect_emu(int pos,
                      komainf_t koma,
                      const bitboard_t *occupied){
    bitboard_t bb;
    switch(koma){
        case SKY: effect_sky_emu(&bb, pos, occupied); break;
        case SKA: effect_ka_emu(&bb, pos, occupied); break;
        case SHI: effect_hi_emu(&bb, pos, occupied); break;
        case SUM: effect_um_emu(&bb, pos, occupied); break;
        case SRY: effect_ry_emu(&bb, pos, occupied); break;
        case GKY: effect_gky_emu(&bb, pos, occupied); break;
        case GKA: effect_ka_emu(&bb, pos, occupied); break;
        case GHI: effect_hi_emu(&bb, pos, occupied); break;
        case GUM: effect_um_emu(&bb, pos, occupied); break;
        case GRY: effect_ry_emu(&bb, pos, occupied); break;
        default:
            BB_CPY(bb, g_base_effect[koma][pos]);
            break;
    }
    return bb;
}

bitboard_t effect_tbl(int pos,
                      komainf_t koma,
                      const bitboard_t *occupied,
                      const bitboard_t *voccupied,
                      const bitboard_t *roccupied,
                      const bitboard_t *loccupied){
    bitboard_t bb;
    switch(koma){
        case SKY: effect_sky(&bb, pos, voccupied); break;
        case SKA: effect_ka(&bb, pos, roccupied, loccupied); break;
        case SHI: effect_hi(&bb, pos, occupied, voccupied); break;
        case SUM: effect_um(&bb, pos, roccupied, loccupied); break;
        case SRY: effect_ry(&bb, pos, occupied, voccupied); break;
        case GKY: effect_gky(&bb, pos, voccupied); break;
        case GKA: effect_ka(&bb, pos, roccupied, loccupied); break;
        case GHI: effect_hi(&bb, pos, occupied, voccupied); break;
        case GUM: effect_um(&bb, pos, roccupied, loccupied); break;
        case GRY: effect_ry(&bb, pos, occupied, voccupied); break;
        default:
            BB_CPY(bb, g_base_effect[koma][pos]);
            break;
    }
    return bb;
}

/* --------------------
 スタティック関数の実装部
 -------------------- */
bitboard_t vbb_to_bb        (const bitboard_t *vbb){
    bitboard_t bb = {0,0,0};
    int pos;
    for(pos=0; pos<N_SQUARE; pos++){
        if(BPOS_TEST(*vbb,pos))BB_SET(bb, st_hvindex[pos]);
    }
    return bb;
}

bitboard_t rbb_to_bb        (const bitboard_t *rbb){
    static char st_rindex2[N_SQUARE] = {
         0, 1, 9, 2,10,18, 3,11,19,
        27, 4,12,20,28,36, 5,13,21,
        29,37,45, 6,14,22,30,38,46,
        54, 7,15,23,31,39,47,55,63,
         8,16,24,32,40,48,56,64,72,
        17,25,33,41,49,57,65,73,26,
        34,42,50,58,66,74,35,43,51,
        59,67,75,44,52,60,68,76,53,
        61,69,77,62,70,78,71,79,80
    };     // for create rbb_to bb
    bitboard_t bb = {0,0,0};
    int pos;
    for(pos=0; pos<N_SQUARE; pos++){
        if(BPOS_TEST(*rbb, pos))BB_SET(bb,st_rindex2[pos]);
    }
    return bb;
}

bitboard_t lbb_to_bb        (const bitboard_t *lbb){
    static char st_lindex2[N_SQUARE] = {
        8, 7,17, 6,16,26, 5,15,25,
       35, 4,14,24,34,44, 3,13,23,
       33,43,53, 2,12,22,32,42,52,
       62, 1,11,21,31,41,51,61,71,
        0,10,20,30,40,50,60,70,80,
        9,19,29,39,49,59,69,79,18,
       28,38,48,58,68,78,27,37,47,
       57,67,77,36,46,56,66,76,45,
       55,65,75,54,64,74,63,73,72
    };     // for create lbb_to_bb
    bitboard_t bb = {0,0,0};
    int pos;
    for(pos=0; pos<N_SQUARE; pos++){
        if(BPOS_TEST(*lbb, pos))BB_SET(bb,st_lindex2[pos]);
    }
    return bb;
}

void create_base_table   (void){
    int pos;
    bitboard_t bb_zero = {0,0,0};
    //SFU
    for(pos=9; pos<N_SQUARE; pos++){
        if(HAS_DR_N__(pos)) BB_SET(g_base_effect[SFU][pos],pos+DR_N);
    }
    //SKY
    for(pos=9; pos<N_SQUARE; pos++){
        effect_sky_emu(&g_base_effect[SKY][pos], pos, &bb_zero);
    }
    //SKE
    for(pos=18; pos<N_SQUARE; pos++){
        if(HAS_DR_NNE(pos)) BB_SET(g_base_effect[SKE][pos], pos+DR_NNE);
        if(HAS_DR_NNW(pos)) BB_SET(g_base_effect[SKE][pos], pos+DR_NNW);
    }
    //SGI
    for(pos=0; pos<N_SQUARE; pos++){
        if(HAS_DR_NE_(pos)) BB_SET(g_base_effect[SGI][pos], pos+DR_NE);
        if(HAS_DR_N__(pos)) BB_SET(g_base_effect[SGI][pos], pos+DR_N);
        if(HAS_DR_NW_(pos)) BB_SET(g_base_effect[SGI][pos], pos+DR_NW);
        if(HAS_DR_SE_(pos)) BB_SET(g_base_effect[SGI][pos], pos+DR_SE);
        if(HAS_DR_SW_(pos)) BB_SET(g_base_effect[SGI][pos], pos+DR_SW);
    }
    //SKI(STO,SNY,SNK,SNG)
    for(pos=0; pos<N_SQUARE; pos++){
        if(HAS_DR_NE_(pos)) BB_SET(g_base_effect[SKI][pos], pos+DR_NE);
        if(HAS_DR_N__(pos)) BB_SET(g_base_effect[SKI][pos], pos+DR_N);
        if(HAS_DR_NW_(pos)) BB_SET(g_base_effect[SKI][pos], pos+DR_NW);
        if(HAS_DR_E__(pos)) BB_SET(g_base_effect[SKI][pos], pos+DR_E);
        if(HAS_DR_W__(pos)) BB_SET(g_base_effect[SKI][pos], pos+DR_W);
        if(HAS_DR_S__(pos)) BB_SET(g_base_effect[SKI][pos], pos+DR_S);
    }
    //SKA
    for(pos=0; pos<N_SQUARE; pos++){
        effect_ka_emu(&g_base_effect[SKA][pos], pos, &bb_zero);
    }
    //SHI
    for(pos=0; pos<N_SQUARE; pos++){
        effect_hi_emu(&g_base_effect[SHI][pos], pos, &bb_zero);
    }
    //SOU(GOU)
    for(pos=0; pos<N_SQUARE; pos++){
        if(HAS_DR_NE_(pos)) BB_SET(g_base_effect[SOU][pos], pos+DR_NE);
        if(HAS_DR_N__(pos)) BB_SET(g_base_effect[SOU][pos], pos+DR_N);
        if(HAS_DR_NW_(pos)) BB_SET(g_base_effect[SOU][pos], pos+DR_NW);
        if(HAS_DR_E__(pos)) BB_SET(g_base_effect[SOU][pos], pos+DR_E);
        if(HAS_DR_W__(pos)) BB_SET(g_base_effect[SOU][pos], pos+DR_W);
        if(HAS_DR_SE_(pos)) BB_SET(g_base_effect[SOU][pos], pos+DR_SE);
        if(HAS_DR_S__(pos)) BB_SET(g_base_effect[SOU][pos], pos+DR_S);
        if(HAS_DR_SW_(pos)) BB_SET(g_base_effect[SOU][pos], pos+DR_SW);
    }
    //STO,SNY,SNK,SNG
    memcpy(&g_base_effect[STO][0], &g_base_effect[SKI][0],
           sizeof(bitboard_t)*N_SQUARE);
    memcpy(&g_base_effect[SNY][0], &g_base_effect[SKI][0],
           sizeof(bitboard_t)*N_SQUARE);
    memcpy(&g_base_effect[SNK][0], &g_base_effect[SKI][0],
           sizeof(bitboard_t)*N_SQUARE);
    memcpy(&g_base_effect[SNG][0], &g_base_effect[SKI][0],
           sizeof(bitboard_t)*N_SQUARE);
    //SUM
    for(pos=0; pos<N_SQUARE; pos++){
        effect_um_emu(&g_base_effect[SUM][pos], pos, &bb_zero);
    }
    //SRY
    for(pos=0; pos<N_SQUARE; pos++){
        effect_ry_emu(&g_base_effect[SRY][pos], pos, &bb_zero);
    }
    //GFU
    for(pos=0; pos<N_SQUARE-9; pos++)
        if(HAS_DR_S__(pos)) BB_SET(g_base_effect[GFU][pos], pos+DR_S);
    //GKY
    for(pos=0; pos<N_SQUARE-9; pos++){
        effect_gky_emu(&g_base_effect[GKY][pos], pos, &bb_zero);
    }
    //GKE
    for(pos=0; pos<N_SQUARE-18; pos++){
        if(HAS_DR_SSE(pos)) BB_SET(g_base_effect[GKE][pos], pos+17);
        if(HAS_DR_SSW(pos)) BB_SET(g_base_effect[GKE][pos], pos+19);
    }
    //GGI
    for(pos=0; pos<N_SQUARE; pos++){
        if(HAS_DR_NE_(pos)) BB_SET(g_base_effect[GGI][pos], pos+DR_NE);
        if(HAS_DR_NW_(pos)) BB_SET(g_base_effect[GGI][pos], pos+DR_NW);
        if(HAS_DR_SE_(pos)) BB_SET(g_base_effect[GGI][pos], pos+DR_SE);
        if(HAS_DR_S__(pos)) BB_SET(g_base_effect[GGI][pos], pos+DR_S);
        if(HAS_DR_SW_(pos)) BB_SET(g_base_effect[GGI][pos], pos+DR_SW);
    }
    //GKI
    for(pos=0; pos<N_SQUARE; pos++){
        if(HAS_DR_N__(pos)) BB_SET(g_base_effect[GKI][pos], pos+DR_N);
        if(HAS_DR_E__(pos)) BB_SET(g_base_effect[GKI][pos], pos+DR_E);
        if(HAS_DR_W__(pos)) BB_SET(g_base_effect[GKI][pos], pos+DR_W);
        if(HAS_DR_SE_(pos)) BB_SET(g_base_effect[GKI][pos], pos+DR_SE);
        if(HAS_DR_S__(pos)) BB_SET(g_base_effect[GKI][pos], pos+DR_S);
        if(HAS_DR_SW_(pos)) BB_SET(g_base_effect[GKI][pos], pos+DR_SW);
    }
    //GKA
    memcpy(&g_base_effect[GKA][0], &g_base_effect[SKA][0],
           sizeof(bitboard_t)*N_SQUARE);
    //GHI
    memcpy(&g_base_effect[GHI][0], &g_base_effect[SHI][0],
           sizeof(bitboard_t)*N_SQUARE);
    //GOU
    memcpy(&g_base_effect[GOU][0], &g_base_effect[SOU][0],
           sizeof(bitboard_t)*N_SQUARE);
    //GTO,GNY,GNK,GNG
    memcpy(&g_base_effect[GTO][0], &g_base_effect[GKI][0],
           sizeof(bitboard_t)*N_SQUARE);
    memcpy(&g_base_effect[GNY][0], &g_base_effect[GKI][0],
           sizeof(bitboard_t)*N_SQUARE);
    memcpy(&g_base_effect[GNK][0], &g_base_effect[GKI][0],
           sizeof(bitboard_t)*N_SQUARE);
    memcpy(&g_base_effect[GNG][0], &g_base_effect[GKI][0],
           sizeof(bitboard_t)*N_SQUARE);
    //GUM
    memcpy(&g_base_effect[GUM][0], &g_base_effect[SUM][0],
           sizeof(bitboard_t)*N_SQUARE);
    //GRY
    memcpy(&g_base_effect[GRY][0], &g_base_effect[SRY][0],
           sizeof(bitboard_t)*N_SQUARE);
    return;
}

void create_hmask        (void){
    int pos, rank, file;
    for(rank=0; rank<N_RANK; rank++){
        for(file=1; file<N_FILE-1; file++){
            pos = rank*N_FILE+file;
            BBA_OR(st_hmask[rank],g_bpos[pos]);
        }
    }
    return;
}

void create_vmask        (void){
    int pos, file, rank;
    for(file=0; file<N_FILE; file++){
        for(rank=1; rank<N_RANK-1; rank++){
            pos = rank*N_FILE+file;
            BBA_OR(st_vmask[file], g_vpos[pos]);
        }
    }
    return;
}

void create_rlmask       (void){
    int i,j,rlpos;
    static char st_num[17] = {0,0,1,2,3,4,5,6,7,6,5,4,3,2,1,0,0};
    static char st_start[17] = {0,0,4,7,11,16,22,29,37,46,54,61,67,72,76,0,0};
    for(i=0; i<17; i++){
        rlpos = st_start[i];
        for(j=0; j<st_num[i]; j++){
            BBA_OR(st_rlmask[i],g_bpos[rlpos]);
            rlpos++;
        }
    }
    return;
}

void create_ky_effect    (void){
    int pos, file, i;
    bitboard_t vbb, occupied;
    for(pos=0; pos<N_SQUARE; pos++){
        file = g_file[pos];
        for(i=0; i<128; i++){
            memset(&vbb, 0, sizeof(bitboard_t));
            vbb.pos[st_hvmask_pos[file]] = i<<st_hvmask_offset[file];
            occupied = vbb_to_bb(&vbb);
            effect_sky_emu(&st_sky_effect[pos][i], pos, &occupied);
            effect_gky_emu(&st_gky_effect[pos][i], pos, &occupied);
        }
    }
    return;
}

void create_ka_effect    (void){
    static char st_rlmindex[N_SQUARE] = {
        0,
        1, 1,
        2, 2, 2,
        3, 3, 3, 3,
        4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7,
        8, 8, 8, 8, 8, 8, 8, 8, 8,
        9, 9, 9, 9, 9, 9, 9, 9,
       10,10,10,10,10,10,10,
       11,11,11,11,11,11,
       12,12,12,12,12,
       13,13,13,13,
       14,14,14,
       15,15,
       16
    };
    static char st_ka_bits[N_SQUARE]  = {
        0,
        0, 0,
        1, 1, 1,
        2, 2, 2, 2,
        3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7,
        6, 6, 6, 6, 6, 6, 6, 6,
        5, 5, 5, 5, 5, 5, 5,
        4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3,
        2, 2, 2, 2,
        1, 1, 1,
        0, 0,
        0
    };
    int pos, rpos, lpos;
    uint8_t key;
    bitboard_t mask, rmask, lmask;
    int rindex, lindex;
    for(pos=0; pos<N_SQUARE; pos++){
        //R_slope
        memset(&rmask, 0, sizeof(bitboard_t));
        rpos = st_rindex[pos];
        rindex = st_rlmindex[rpos];
        for(key=0; key<(1<<st_ka_bits[rpos]); key++){
            rmask.pos[st_rlmask_pos[rindex]] = key<<st_rlmask_offset[rindex];
            mask = rbb_to_bb(&rmask);
            effect_rka_emu(&st_rka_effect[st_ka_index[rpos]+key], pos, &mask);
        }
        //L_slope
        memset(&lmask, 0, sizeof(bitboard_t));
        lpos = st_lindex[pos];
        lindex = st_rlmindex[lpos];
        for(key=0; key<(1<<st_ka_bits[lpos]); key++){
            lmask.pos[st_rlmask_pos[lindex]] = key<<st_rlmask_offset[lindex];
            mask = lbb_to_bb(&lmask);
            effect_lka_emu(&st_lka_effect[st_ka_index[lpos]+key], pos, &mask);
        }
    }
    return;
}

void create_hi_effect    (void){
    int pos, rank, file, i;
    bitboard_t vbb, hoccupied, voccupied;
    for(pos=0; pos<N_SQUARE; pos++){
        rank = g_rank[pos];
        file = g_file[pos];
        for(i=0; i<128; i++){
            //horizontal
            memset(&hoccupied, 0, sizeof(bitboard_t));
            hoccupied.pos[st_hvmask_pos[rank]] = i<<st_hvmask_offset[rank];
            effect_hhi_emu(&st_hhi_effect[pos][i], pos, &hoccupied);
            //vertical
            memset(&vbb, 0, sizeof(bitboard_t));
            vbb.pos[st_hvmask_pos[file]] = i<<st_hvmask_offset[file];
            voccupied = vbb_to_bb(&vbb);
            effect_vhi_emu(&st_vhi_effect[pos][i], pos, &voccupied);
        }
    }
    return;
}

void effect_sky_emu         (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied){
    int i;
    memset(bb, 0, sizeof(bitboard_t));
    i=pos;
    do{
        i+=DR_N;
        if(OUT_OF_N_(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    return;
}

void effect_gky_emu         (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied){
    int i;
    memset(bb, 0, sizeof(bitboard_t));
    i=pos;
    do{
        i+=DR_S;
        if(OUT_OF_S_(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    return;
}

void effect_rka_emu         (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied){
    int i;
    memset(bb, 0, sizeof(bitboard_t));
    i=pos;
    do{
        i+=DR_NW;
        if(OUT_OF_NW(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    i=pos;
    do{
        i+=DR_SE;
        if(OUT_OF_SE(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    return;
}

void effect_lka_emu         (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied){
    int i;
    memset(bb, 0, sizeof(bitboard_t));
    i=pos;
    do{
        i+=DR_NE;
        if(OUT_OF_NE(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    i=pos;
    do{
        i+=DR_SW;
        if(OUT_OF_SW(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    return;
}

void effect_ka_emu          (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied){
    int i;
    memset(bb, 0, sizeof(bitboard_t));
    i=pos;
    do{
        i+=DR_NE;
        if(OUT_OF_NE(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    i=pos;
    do{
        i+=DR_NW;
        if(OUT_OF_NW(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    i=pos;
    do{
        i+=DR_SE;
        if(OUT_OF_SE(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    i=pos;
    do{
        i+=DR_SW;
        if(OUT_OF_SW(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    return;
}

void effect_um_emu          (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied){
    effect_ka_emu(bb, pos, occupied);
    BBA_OR(*bb, g_base_effect[SOU][pos]);
    return;
}

void effect_hhi_emu         (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied){
    int i;
    memset(bb, 0, sizeof(bitboard_t));
    i=pos;
    do{
        i+=DR_E;
        if(OUT_OF_E_(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    i=pos;
    do{
        i+=DR_W;
        if(i>=N_SQUARE) break;
        if(OUT_OF_W_(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    return;
}

void effect_vhi_emu         (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied){
    int i;
    memset(bb, 0, sizeof(bitboard_t));
    i=pos;
    do{
        i+=DR_N;
        if(OUT_OF_N_(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    i=pos;
    do{
        i+=DR_S;
        if(OUT_OF_S_(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    return;
}

void effect_hi_emu          (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied){
    int i;
    memset(bb, 0, sizeof(bitboard_t));
    i=pos;
    do{
        i+=DR_N;
        if(OUT_OF_N_(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    i=pos;
    do{
        i+=DR_E;
        if(OUT_OF_E_(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    i=pos;
    do{
        
        i+=DR_W;
        if(i>=N_SQUARE)break;
        if(OUT_OF_W_(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    i=pos;
    do{
        i+=DR_S;
        if(OUT_OF_S_(i))break;
        BB_SET(*bb,i);
    }while(!(BPOS_TEST(*occupied, i)));
    return;
}

void effect_ry_emu          (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied){
    effect_hi_emu(bb, pos, occupied);
    BBA_OR(*bb, g_base_effect[SOU][pos]);
    return;
}

void effect_sky             (bitboard_t *bb,
                             int pos,
                             const bitboard_t *voccupied){
    //posの筋を求める
    int file = g_file[pos];
    bitboard_t vbb = st_vmask[file];
    BBA_AND(vbb, *voccupied);
    //vkeyを求める
    uint8_t vkey;
    vkey = (vbb.pos[st_hvmask_pos[file]])>>st_hvmask_offset[file];
    *bb = st_sky_effect[pos][vkey];
    return;
}
void effect_gky             (bitboard_t *bb,
                             int pos,
                             const bitboard_t *voccupied){
    //posの筋を求める
    int file = g_file[pos];
    bitboard_t vbb = st_vmask[file];
    BBA_AND(vbb, *voccupied);
    //vkeyを求める
    uint8_t vkey;
    vkey = (vbb.pos[st_hvmask_pos[file]])>>st_hvmask_offset[file];
    *bb = st_gky_effect[pos][vkey];
    return;
}
void effect_ka              (bitboard_t *bb,
                             int pos,
                             const bitboard_t *roccupied,
                             const bitboard_t *loccupied){
    static char st_rmindex[N_SQUARE] = {
         0, 1, 2, 3, 4, 5, 6, 7, 8,
         1, 2, 3, 4, 5, 6, 7, 8, 9,
         2, 3, 4, 5, 6, 7, 8, 9,10,
         3, 4, 5, 6, 7, 8, 9,10,11,
         4, 5, 6, 7, 8, 9,10,11,12,
         5, 6, 7, 8, 9,10,11,12,13,
         6, 7, 8, 9,10,11,12,13,14,
         7, 8, 9,10,11,12,13,14,15,
         8, 9,10,11,12,13,14,15,16
    };
    static char st_lmindex[N_SQUARE] = {
         8, 7, 6, 5, 4, 3, 2, 1, 0,
         9, 8, 7, 6, 5, 4, 3, 2, 1,
        10, 9, 8, 7, 6, 5, 4, 3, 2,
        11,10, 9, 8, 7, 6, 5, 4, 3,
        12,11,10, 9, 8, 7, 6, 5, 4,
        13,12,11,10, 9, 8, 7, 6, 5,
        14,13,12,11,10, 9, 8, 7, 6,
        15,14,13,12,11,10, 9, 8, 7,
        16,15,14,13,12,11,10, 9, 8
    };
    //角の利きのR_slope(\:NW-SE)成分maskを準備する。
    int index = st_rmindex[pos];
    bitboard_t rmask = st_rlmask[index];
    BBA_AND(rmask, *roccupied);
    //maskからrkeyを求める。
    char rkey = (rmask.pos[st_rlmask_pos[index]])>>st_rlmask_offset[index];
    int rpos = st_rindex[pos];
    bitboard_t bb_r = st_rka_effect[st_ka_index[rpos]+rkey];
    //角の利きのL_slope(/:NE-SW)成分maskを準備する。
    index = st_lmindex[pos];
    bitboard_t lmask = st_rlmask[index];
    BBA_AND(lmask, *loccupied);
    //maskからlkeyを求める
    char lkey = (lmask.pos[st_rlmask_pos[index]])>>st_rlmask_offset[index];
    int lpos = st_lindex[pos];
    bitboard_t bb_l = st_lka_effect[st_ka_index[lpos]+lkey];
    BB_OR(*bb, bb_r, bb_l);
    return;
}

void effect_um              (bitboard_t *bb,
                             int pos,
                             const bitboard_t *roccupied,
                             const bitboard_t *loccupied){
    effect_ka(bb, pos, roccupied, loccupied);
    BBA_OR(*bb, g_base_effect[SOU][pos]);
    return;
}

void effect_hi              (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied,
                             const bitboard_t *voccupied){
    //横方向
    //posの段を求める
    int rank = g_rank[pos];
    bitboard_t hbb = st_hmask[rank];
    BBA_AND(hbb, *occupied);
    //hkeyを求める
    char hkey = (hbb.pos[st_hvmask_pos[rank]])>>st_hvmask_offset[rank];
    bitboard_t bb_h = st_hhi_effect[pos][hkey];
    //縦方向
    //posの筋を求める
    int file = g_file[pos];
    bitboard_t vbb = st_vmask[file];
    BBA_AND(vbb, *voccupied);
    //vkeyを求める
    char vkey = (vbb.pos[st_hvmask_pos[file]])>>st_hvmask_offset[file];
    bitboard_t bb_v = st_vhi_effect[pos][vkey];
    
    BB_OR(*bb, bb_h, bb_v);
    return;
}
void effect_ry              (bitboard_t *bb,
                             int pos,
                             const bitboard_t *occupied,
                             const bitboard_t *voccupied){
    effect_hi(bb, pos, occupied, voccupied);
    BBA_OR(*bb, g_base_effect[SOU][pos]);
    return;
}

