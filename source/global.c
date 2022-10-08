//
//  global.c
//  shshogi
//
//  Created by Hkijin on 2021/09/21.
//

#include <stdio.h>
#include "shogi.h"

/* ------------------------------
 shshogiで使用するグローバル変数
 ------------------------------ */

/* --------------
 将棋基本ライブラリ
 -------------- */
//駒表示
char *g_koma_char[N_KOMAS]  = {
    "SPC","SFU","SKY","SKE","SGI","SKI","SKA","SHI",
    "SOU","STO","SNY","SNK","SNG","***","SUM","SRY",
    "SPC","GFU","GKY","GKE","GGI","GKI","GKA","GHI",
    "GOU","GTO","GNY","GNK","GNG","***","GUM","GRY"
};

//駒が成れる成れないの情報(成れる 1, 成れない 0)
int g_koma_promotion[N_KOMAS] = {
// SPC, FU, KY, KE, GI, KI, KA, HI, OU, TO, NY, NK, NG, **, UM, RY
    0,   1,  1,  1,  1,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,
    0,   1,  1,  1,  1,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0
};
//飛駒か否かの情報（飛駒 0, 飛駒ではない 1)
int g_is_skoma[N_KOMAS] = {
// SPC, FU, KY, KE, GI, KI, KA, HI, OU, TO, NY, NK, NG, **, UM, RY
    0,   1,  0,  1,  1,  1,  0,  0,  1,  1,  1,  1,  1,  0,  0,  0,
    0,   1,  0,  1,  1,  1,  0,  0,  1,  1,  1,  1,  1,  0,  0,  0
};

//盤関連
// posの筋を返す。
char g_file[N_SQUARE] = {
    FILE1, FILE2, FILE3, FILE4, FILE5, FILE6, FILE7, FILE8, FILE9,
    FILE1, FILE2, FILE3, FILE4, FILE5, FILE6, FILE7, FILE8, FILE9,
    FILE1, FILE2, FILE3, FILE4, FILE5, FILE6, FILE7, FILE8, FILE9,
    FILE1, FILE2, FILE3, FILE4, FILE5, FILE6, FILE7, FILE8, FILE9,
    FILE1, FILE2, FILE3, FILE4, FILE5, FILE6, FILE7, FILE8, FILE9,
    FILE1, FILE2, FILE3, FILE4, FILE5, FILE6, FILE7, FILE8, FILE9,
    FILE1, FILE2, FILE3, FILE4, FILE5, FILE6, FILE7, FILE8, FILE9,
    FILE1, FILE2, FILE3, FILE4, FILE5, FILE6, FILE7, FILE8, FILE9,
    FILE1, FILE2, FILE3, FILE4, FILE5, FILE6, FILE7, FILE8, FILE9
};
// posの段を返す。
char g_rank[N_SQUARE] = {
    RANK1, RANK1, RANK1, RANK1, RANK1, RANK1, RANK1, RANK1, RANK1,
    RANK2, RANK2, RANK2, RANK2, RANK2, RANK2, RANK2, RANK2, RANK2,
    RANK3, RANK3, RANK3, RANK3, RANK3, RANK3, RANK3, RANK3, RANK3,
    RANK4, RANK4, RANK4, RANK4, RANK4, RANK4, RANK4, RANK4, RANK4,
    RANK5, RANK5, RANK5, RANK5, RANK5, RANK5, RANK5, RANK5, RANK5,
    RANK6, RANK6, RANK6, RANK6, RANK6, RANK6, RANK6, RANK6, RANK6,
    RANK7, RANK7, RANK7, RANK7, RANK7, RANK7, RANK7, RANK7, RANK7,
    RANK8, RANK8, RANK8, RANK8, RANK8, RANK8, RANK8, RANK8, RANK8,
    RANK9, RANK9, RANK9, RANK9, RANK9, RANK9, RANK9, RANK9, RANK9
};
// posの右上がりslopeIDを返す。
char g_rslp[N_SQUARE] = {
    RSLP0, RSLP1, RSLP2, RSLP3, RSLP4, RSLP5, RSLP6, RSLP7, RSLP8,
    RSLP1, RSLP2, RSLP3, RSLP4, RSLP5, RSLP6, RSLP7, RSLP8, RSLP9,
    RSLP2, RSLP3, RSLP4, RSLP5, RSLP6, RSLP7, RSLP8, RSLP9, RSLP10,
    RSLP3, RSLP4, RSLP5, RSLP6, RSLP7, RSLP8, RSLP9, RSLP10,RSLP11,
    RSLP4, RSLP5, RSLP6, RSLP7, RSLP8, RSLP9, RSLP10,RSLP11,RSLP12,
    RSLP5, RSLP6, RSLP7, RSLP8, RSLP9, RSLP10,RSLP11,RSLP12,RSLP13,
    RSLP6, RSLP7, RSLP8, RSLP9, RSLP10,RSLP11,RSLP12,RSLP13,RSLP14,
    RSLP7, RSLP8, RSLP9, RSLP10,RSLP11,RSLP12,RSLP13,RSLP14,RSLP15,
    RSLP8, RSLP9, RSLP10,RSLP11,RSLP12,RSLP13,RSLP14,RSLP15,RSLP16
};
// posの右下がりslpoeIDを返す。
char g_lslp[N_SQUARE] = {
    LSLP8, LSLP7, LSLP6, LSLP5, LSLP4, LSLP3, LSLP2, LSLP1, LSLP0,
    LSLP9, LSLP8, LSLP7, LSLP6, LSLP5, LSLP4, LSLP3, LSLP2, LSLP1,
    LSLP10,LSLP9, LSLP8, LSLP7, LSLP6, LSLP5, LSLP4, LSLP3, LSLP2,
    LSLP11,LSLP10,LSLP9, LSLP8, LSLP7, LSLP6, LSLP5, LSLP4, LSLP3,
    LSLP12,LSLP11,LSLP10,LSLP9, LSLP8, LSLP7, LSLP6, LSLP5, LSLP4,
    LSLP13,LSLP12,LSLP11,LSLP10,LSLP9, LSLP8, LSLP7, LSLP6, LSLP5,
    LSLP14,LSLP13,LSLP12,LSLP11,LSLP10,LSLP9, LSLP8, LSLP7, LSLP6,
    LSLP15,LSLP14,LSLP13,LSLP12,LSLP11,LSLP10,LSLP9, LSLP8, LSLP7,
    LSLP16,LSLP15,LSLP14,LSLP13,LSLP12,LSLP11,LSLP10,LSLP9, LSLP8
};

char g_distance[N_SQUARE][N_SQUARE];

zkey_t g_zkey_seed[N_ZOBRIST_SEED];    // zobrist key


bitboard_t g_bpos[N_SQUARE];  //Horizontal order        (-)
bitboard_t g_vpos[N_SQUARE];  //Vertical order          (|)
bitboard_t g_rpos[N_SQUARE];  //Right down slope order  (\)
bitboard_t g_lpos[N_SQUARE];  //Left down slope order   (/)

bitboard_t g_bb_rank[N_RANK] = {
    {0X000001FF, 0X00000000, 0X00000000},    // 1:bb rank 0
    {0X0003FE00, 0X00000000, 0X00000000},    // 2:
    {0X07FC0000, 0X00000000, 0X00000000},    // 3:
    {0X00000000, 0X000001FF, 0X00000000},    // 4:
    {0X00000000, 0X0003FE00, 0X00000000},    // 5:
    {0X00000000, 0X07FC0000, 0X00000000},    // 6:
    {0X00000000, 0X00000000, 0X000001FF},    // 7:
    {0X00000000, 0X00000000, 0X0003FE00},    // 8:
    {0X00000000, 0X00000000, 0X07FC0000}     // 9:bb rank 8
};

bitboard_t g_bb_file[N_FILE] = {
    {0X00040201, 0X00040201, 0X00040201},    // 10:bb file 0
    {0X00080402, 0X00080402, 0X00080402},    // 11:
    {0X00100804, 0X00100804, 0X00100804},    // 12:
    {0X00201008, 0X00201008, 0X00201008},    // 13:
    {0X00402010, 0X00402010, 0X00402010},    // 14:
    {0X00804020, 0X00804020, 0X00804020},    // 15:
    {0X01008040, 0X01008040, 0X01008040},    // 16:
    {0X02010080, 0X02010080, 0X02010080},    // 17:
    {0X04020100, 0X04020100, 0X04020100}     // 18:bb file 8
};

/* --------------------------------------------------------------------------
 g_bb_pin[]    駒が自玉と相手の飛駒でpinされた場合、pinのマスクとなるbitboard。
               使用法は利きとpinのマスクのANDを取ることにより、動ける場所を得る。
 ------------------------------------------------------------------------- */
bitboard_t g_bb_pin[45]       = {
    {0X07FFFFFF, 0X07FFFFFF, 0X07FFFFFF},    // 0:No pin (default)
 
    {0X000001FF, 0X00000000, 0X00000000},    // 1:bb rank 0
    {0X0003FE00, 0X00000000, 0X00000000},    // 2:
    {0X07FC0000, 0X00000000, 0X00000000},    // 3:
    {0X00000000, 0X000001FF, 0X00000000},    // 4:
    {0X00000000, 0X0003FE00, 0X00000000},    // 5:
    {0X00000000, 0X07FC0000, 0X00000000},    // 6:
    {0X00000000, 0X00000000, 0X000001FF},    // 7:
    {0X00000000, 0X00000000, 0X0003FE00},    // 8:
    {0X00000000, 0X00000000, 0X07FC0000},    // 9:bb rank 8
 
    {0X00040201, 0X00040201, 0X00040201},    // 10:bb file 0
    {0X00080402, 0X00080402, 0X00080402},    // 11:
    {0X00100804, 0X00100804, 0X00100804},    // 12:
    {0X00201008, 0X00201008, 0X00201008},    // 13:
    {0X00402010, 0X00402010, 0X00402010},    // 14:
    {0X00804020, 0X00804020, 0X00804020},    // 15:
    {0X01008040, 0X01008040, 0X01008040},    // 16:
    {0X02010080, 0X02010080, 0X02010080},    // 17:
    {0X04020100, 0X04020100, 0X04020100},    // 18:bb file 8
 
    //{0X00000100, 0X00000000, 0X00000000},
    //{0X00020080, 0X00000000, 0X00000000},
 
    {0X04010040, 0X00000000, 0X00000000},    // 19: bb l45(NE-SW)
    {0X02008020, 0X00000100, 0X00000000},    // 20:
    {0X01004010, 0X00020080, 0X00000000},    // 21:
    {0X00802008, 0X04010040, 0X00000000},    // 22:
    {0X00401004, 0X02008020, 0X00000100},    // 23:
    {0X00200802, 0X01004010, 0X00020080},    // 24:
    {0X00100401, 0X00802008, 0X04010040},    // 25:
    {0X00080200, 0X00401004, 0X02008020},    // 26:
    {0X00040000, 0X00200802, 0X01004010},    // 27:
    {0X00000000, 0X00100401, 0X00802008},    // 28:
    {0X00000000, 0X00080200, 0X00401004},    // 29:
    {0X00000000, 0X00040000, 0X00200802},    // 30:
    {0X00000000, 0X00000000, 0X00100401},    // 31:
 
    //{0X00000000, 0X00000000, 0X00080200},
    //{0X00000000, 0X00000000, 0X00040000},
 
    //{0X00000001, 0X00000000, 0X00000000},
    //{0X00000202, 0X00000000, 0X00000000},
 
    {0X00040404, 0X00000000, 0X00000000},    // 32: bb r45(NW-SE)
    {0X00080808, 0X00000001, 0X00000000},    // 33:
    {0X00101010, 0X00000202, 0X00000000},    // 34:
    {0X00202020, 0X00040404, 0X00000000},    // 35:
    {0X00404040, 0X00080808, 0X00000001},    // 36:
    {0X00808080, 0X00101010, 0X00000202},    // 37:
    {0X01010100, 0X00202020, 0X00040404},    // 38:
    {0X02020000, 0X00404040, 0X00080808},    // 39:
    {0X04000000, 0X00808080, 0X00101010},    // 40:
    {0X00000000, 0X01010100, 0X00202020},    // 41:
    {0X00000000, 0X02020000, 0X00404040},    // 42:
    {0X00000000, 0X04000000, 0X00808080},    // 43:
    {0X00000000, 0X00000000, 0X01010100}     // 44:
 
    //{0X00000000, 0X00000000, 0X02020000},
    //{0X00000000, 0X00000000, 0X04000000},
};          //PIN

bitboard_t g_bb_rank012 = {0X07FFFFFF, 0X00000000, 0X00000000};
bitboard_t g_bb_rank345 = {0X00000000, 0X07FFFFFF, 0X00000000};
bitboard_t g_bb_rank678 = {0X00000000, 0X00000000, 0X07FFFFFF};

bitboard_t g_bb_rank0123 = {0X07FFFFFF, 0X000001FF, 0X00000000};
bitboard_t g_bb_rank5678 = {0X00000000, 0X07FC0000, 0X07FFFFFF};

bitboard_t g_base_effect[N_KOMAS][N_SQUARE];  //effect table

unsigned int g_bit[20]     = {
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512,
    1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288
};

ssdata_t g_hirate = {
    0,
    SENTE,
    {   GKY, GKE, GGI, GKI, GOU, GKI, GGI, GKE, GKY,
        SPC, GKA, SPC, SPC, SPC, SPC, SPC, GHI, SPC,
        GFU, GFU, GFU, GFU, GFU, GFU, GFU, GFU, GFU,
        SPC, SPC, SPC, SPC, SPC, SPC, SPC, SPC, SPC,
        SPC, SPC, SPC, SPC, SPC, SPC, SPC, SPC, SPC,
        SPC, SPC, SPC, SPC, SPC, SPC, SPC, SPC, SPC,
        SFU, SFU, SFU, SFU, SFU, SFU, SFU, SFU, SFU,
        SPC, SHI, SPC, SPC, SPC, SPC, SPC, SKA, SPC,
        SKY, SKE, SGI, SKI, SOU, SKI, SGI, SKE, SKY  },
    {{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}}
};

#ifdef SDATA_EXTENTION
short g_koma_val[N_KOMAS]  = {
    0,         //SPC
    87,        //SFU
    232,       //SKY
    257,       //SKE
    369,       //SGI
    444,       //SKI
    569,       //SKA
    642,       //SHI
    15000,     //SOU
    534,       //STO
    489,       //SNY
    510,       //SNK
    495,       //SNG
    0,         //
    827,       //SUM
    945,       //SRY
    0,         //
    -87,       //GFU
    -232,      //GKY
    -257,      //GKE
    -369,      //GGI
    -444,      //GKI
    -569,      //GKA
    -642,      //GHI
    -15000,    //GOU
    -534,      //GTO
    -489,      //GNY
    -510,      //GNK
    -495,      //GNG
    0,         //
    -827,      //GUM
    -945       //GRY
};
char  g_nkoma_val[N_KOMAS] = {
    //FU KY KE GI KI KA HI OU TO NY NK NG ** UM RY
    0, 1, 1, 1, 1, 1, 5, 5, 0, 1, 1, 1, 1, 0, 5, 5,
    0, 1, 1, 1, 1, 1, 5, 5, 0, 1, 1, 1, 1, 0, 5, 5
};
#endif //SDATA_EXTENTION

move_t g_mv_toryo = {TORYO, 0};

