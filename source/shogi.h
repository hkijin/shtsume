//
//  shogi.h
//  shshogi
//
//  Created by Hkijin on 2021/09/02.
//

#ifndef shogi_h
#define shogi_h

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ---------------------------------------------------------------------------
 【記載ルール】
 ユーザー変数、関数記載ルール
 型名             ***_t
 変数             小文字    グローバル変数     g_****   スタティック変数   st_****
 関数             小文字(小文字)
 マクロ定数（注）   大文字　　　　（注）列挙型定数も含む
 マクロ関数        大文字(小文字）
 １行80文字以内
--------------------------------------------------------------------------- */
//汎用マクロ
#ifndef MAX
#define MAX(a,b)    (((a)>=(b))?(a):(b))
#endif /* MAX */
#ifndef MIN
#define MIN(a,b)    (((a)<=(b))?(a):(b))
#endif /* MIN */

//コマンドラインヘルプ(この関数は思考エンジン内で準備する)
void print_help (void);

/* ---------------------------------------------------------------------------
 【将棋基本ライブラリ】
 
 ［駒］
　将棋の駒は８種類で先後で２通り、駒によっては成ることができます。
　これらの属性を踏まえ、0-31の数値を割り当てます。
 ［盤］
　盤上の位置表現には以下の盤IDを使用します。例えば３五の地点表す盤IDは38になります。
 
     9  8  7  6  5  4  3  2  1
  ------------------------------
  |  8  7  6  5  4  3  2  1  0 | 一
  | 17 16 15 14 13 12 11 10  9 | 二
  | 26 25 24 23 22 21 20 19 18 | 三
  | 35 34 33 32 31 30 29 28 27 | 四
  | 44 43 42 41 40 39 38 37 36 | 五
  | 53 52 51 50 49 48 47 46 45 | 六
  | 62 61 60 59 58 57 56 55 54 | 七
  | 71 70 69 68 67 66 65 64 63 | 八
  | 80 79 78 77 76 75 74 73 72 | 九
  ------------------------------

 --------------------------------------------------------------------------- */
//将棋駒（単独）
typedef enum _KType { OU, FU, KY, KE, GI, KI, KA, HI} KType;
typedef enum _Koma  { SPC = 0,
                      SFU, SKY, SKE, SGI, SKI, SKA, SHI, SOU,
                      STO, SNY, SNK, SNG, SUM = 14, SRY,
                      GFU = 17, GKY, GKE, GGI, GKI, GKA, GHI, GOU,
                      GTO, GNY, GNK, GNG, GUM = 30, GRY } Koma;

#define N_KOMAS        32
#define PROMOTED       8
#define SENTE_KOMA(k)  ((k)&&(k)<16)
#define GOTE_KOMA(k)   ((k)>16)

extern short g_koma_val [N_KOMAS];
extern char  g_nkoma_val[N_KOMAS];
extern char *g_koma_char[N_KOMAS];
extern int   g_koma_promotion[N_KOMAS];
extern int   g_is_skoma[N_KOMAS];
typedef char komainf_t;

//将棋盤
#define N_SQUARE   81   //枡目の数
#define N_RANK      9   //段の数
#define N_FILE      9   //筋の数

typedef enum _Board {
    B11,B21,B31,B41,B51,B61,B71,B81,B91,
    B12,B22,B32,B42,B52,B62,B72,B82,B92,
    B13,B23,B33,B43,B53,B63,B73,B83,B93,
    B14,B24,B34,B44,B54,B64,B74,B84,B94,
    B15,B25,B35,B45,B55,B65,B75,B85,B95,
    B16,B26,B36,B46,B56,B66,B76,B86,B96,
    B17,B27,B37,B47,B57,B67,B77,B87,B97,
    B18,B28,B38,B48,B58,B68,B78,B88,B98,
    B19,B29,B39,B49,B59,B69,B79,B89,B99
} Board;

//盤上方向
#define DR_N       -9
#define DR_E       -1
#define DR_W        1
#define DR_S        9
#define DR_NE     -10
#define DR_NW      -8
#define DR_SE       8
#define DR_SW      10
#define DR_NNE    -19
#define DR_NNW    -17
#define DR_SSE     17
#define DR_SSW     19

//盤上端部判定
#define HAS_DR_N__(p)       ((p)>8)
#define HAS_DR_E__(p)       (g_file[(p)])
#define HAS_DR_W__(p)       (g_file[(p)]<8)
#define HAS_DR_S__(p)       ((p)<72)
#define HAS_DR_NE_(p)       ((p)>8 && g_file[(p)])
#define HAS_DR_NW_(p)       ((p)>8 && g_file[(p)]<8)
#define HAS_DR_SE_(p)       ((p)<72 && g_file[(p)])
#define HAS_DR_SW_(p)       ((p)<72 && g_file[(p)]<8)
#define HAS_DR_NNE(p)       (g_file[(p)] && (p)>17)
#define HAS_DR_NNW(p)       (g_file[(p)]<8 && (p)>17)
#define HAS_DR_SSE(p)       (g_file[(p)] && (p)<63)
#define HAS_DR_SSW(p)       (g_file[(p)]<8 && (p)<63)

#define OUT_OF_N_(p)        (p)<0
#define OUT_OF_E_(p)        ((p)<0 || g_file[p]==8)
#define OUT_OF_W_(p)        (g_file[p]==0)
#define OUT_OF_S_(p)        (p)>80
#define OUT_OF_NE(p)        ((p)<0 || g_file[p]==8)
#define OUT_OF_NW(p)        ((p)<0 || g_file[p]==0)
#define OUT_OF_SE(p)        ((p)>80|| g_file[p]==8)
#define OUT_OF_SW(p)        ((p)>80|| g_file[p]==0)

#define DIR_N_(src,dest)    (g_file[src]==g_file[dest] &&\
                             g_rank[src]-g_rank[dest]>1)
#define DIR_E_(src,dest)    (g_rank[src]==g_rank[dest] &&\
                             g_file[src]-g_file[dest]>1)
#define DIR_W_(src,dest)    (g_rank[src]==g_rank[dest] &&\
                             g_file[dest]-g_file[src]>1)
#define DIR_S_(src,dest)    (g_file[src]==g_file[dest] &&\
                             g_rank[dest]-g_rank[src]>1)

#define DIR_NE(src,dest)    (g_rank[src]-g_rank[dest]>1 &&\
                             g_file[src]-g_file[dest]>1)
#define DIR_NW(src,dest)    (g_rank[src]-g_rank[dest]>1 &&\
                             g_file[dest]-g_file[src]>1)
#define DIR_SE(src,dest)    (g_rank[dest]-g_rank[src]>1 &&\
                             g_file[src]-g_file[dest]>1)
#define DIR_SW(src,dest)    (g_rank[dest]-g_rank[src]>1 &&\
                             g_file[dest]-g_file[src]>1)

/*
#define DIR_NEX(src,dest)   (g_rank[src]-g_rank[dest]>1 &&\
                            (g_file[src]-g_file[dest])==\
                            (g_rank[src]-g_rank[dest]))
#define DIR_NWX(src,dest)   (g_rank[src]-g_rank[dest]>1 &&\
                            (g_file[dest]-g_file[src])==\
                            (g_rank[src]-g_rank[dest]))
#define DIR_SEX(src,dest)   (g_rank[dest]-g_rank[src]>1 &&\
                            (g_file[src]-g_file[dest])==\
                            (g_rank[dest]-g_rank[src]))
#define DIR_SWX(src,dest)   (g_rank[dest]-g_rank[src]>1 &&\
                            (g_file[dest]-g_file[src])==\
                            (g_rank[dest]-g_rank[src]))
*/
enum {FILE1,FILE2,FILE3,FILE4,FILE5,FILE6,FILE7,FILE8,FILE9};   //縦列ID
enum {RANK1,RANK2,RANK3,RANK4,RANK5,RANK6,RANK7,RANK8,RANK9};   //横列ID
enum {RSLP0,RSLP1,RSLP2,RSLP3,RSLP4,RSLP5,RSLP6,RSLP7,RSLP8,
      RSLP9,RSLP10,RSLP11,RSLP12,RSLP13,RSLP14,RSLP15,RSLP16};  //右下斜列ID
enum {LSLP0,LSLP1,LSLP2,LSLP3,LSLP4,LSLP5,LSLP6,LSLP7,LSLP8,
      LSLP9,LSLP10,LSLP11,LSLP12,LSLP13,LSLP14,LSLP15,LSLP16};  //左下斜列ID

extern char g_file[N_SQUARE];
extern char g_rank[N_SQUARE];
extern char g_rslp[N_SQUARE];
extern char g_lslp[N_SQUARE];

extern char g_distance[N_SQUARE][N_SQUARE];

void init_distance(void);

/* --------------------------------------------------------------------------
 bitboard_t型
 将棋盤上の駒配置を表現するため,ビットID0-80の有効bitを持つデータ構造。(Fig.1)
 これらのビットIDと盤IDの紐付けを行うことにより盤上の様々な情報を01判定で記録することができます。
 ---------------------------------------------------+
 | Fig.1 Bit assignment                             |
 |  *: Board   .: not used                          |
 |  32      24      16       8       0              |
 |   +-------+-------+-------+-------+              |
 |    .....*************************** pos[0] 0-26  |
 |    .....*************************** pos[1] 27-53 |
 |    .....*************************** pos[2] 54-80 |
 ---------------------------------------------------+

-------------------------------------------------------------------------------
 盤IDとbitIDとの紐付け
 将棋盤上の各桝目とBitboardのbit idとで異なる組み合わせをもつの４通りのbitboard,
 (bb,vbb,rbb,lbb)準備します。例えば、盤上の３五地点を表現する場合、盤位置とそれぞれの
 bit idとの対応は以下のようになります。ただし、vbb,rbb,lbbの使用は飛、角、香の利きを
 算出する時のみです。
 
 例）
 盤位置 |bb |vbb|rbb|lbb|
 ------+---+---+---+---+
 （１一)| 0 | 0 |  0| 36|
 （３五)|38 |22 | 25| 55|
 ------+---+---+---+---+
 
 1, Horizontal order(bb):Normal
   9) 8) 7) 6) 5) 4) 3) 2) 1)         9 8 7 6 5 4 3 2 1
------------------------------        ------------------+
|  8  7  6  5  4  3  2  1  0 | 1)     . . . . . . . . . | 1  (.) pos[0] 0-26
| 17 16 15 14 13 12 11 10  9 | 2)     . . . . . . . . . | 2  (*) pos[1] 27-53
| 26 25 24 23 22 21 20 19 18 | 3)     . . . . . . . . . | 3  (-) pos[2] 54-80
| 35 34 33 32 31 30 29 28 27 | 4)     * * * * * * * * * | 4
| 44 43 42 41 40 39 38 37 36 | 5)     * * * * * * * * * | 5
| 53 52 51 50 49 48 47 46 45 | 6)     * * * * * * * * * | 6
| 62 61 60 59 58 57 56 55 54 | 7)     - - - - - - - - - | 7
| 71 70 69 68 67 66 65 64 63 | 8)     - - - - - - - - - | 8
| 80 79 78 77 76 75 74 73 72 | 9)     - - - - - - - - - | 9
------------------------------
 2, Vertical order(vbb)
   9) 8) 7) 6) 5) 4) 3) 2) 1)         9 8 7 6 5 4 3 2 1
------------------------------        ------------------+
| 72 63 54 45 36 27 18  9  0 | 1)     - - - * * * . . . | 1
| 73 64 55 46 37 28 19 10  1 | 2)     - - - * * * . . . | 2
| 74 65 56 47 38 29 20 11  2 | 3)     - - - * * * . . . | 3
| 75 66 57 48 39 30 21 12  3 | 4)     - - - * * * . . . | 4
| 76 67 58 49 40 31 22 13  4 | 5)     - - - * * * . . . | 5
| 77 68 59 50 41 32 23 14  5 | 6)     - - - * * * . . . | 6
| 78 69 60 51 42 33 24 15  6 | 7)     - - - * * * . . . | 7
| 79 70 61 52 43 34 25 16  7 | 8)     - - - * * * . . . | 8
| 80 71 62 53 44 35 26 17  8 | 9)     - - - * * * . . . | 9
------------------------------
 
3, Right down slope order(rbb)
   9) 8) 7) 6) 5) 4) 3) 2) 1)         9 8 7 6 5 4 3 2 1
------------------------------        ------------------+
| 36 28 21 15 10  6  3  1  0 | 1)     * * . . . . . . . | 1
| 45 37 29 22 16 11  7  4  2 | 2)     * * * . . . . . . | 2
| 53 46 38 30 23 17 12  8  5 | 3)     - * * * . . . . . | 3
| 60 54 47 39 31 24 18 13  9 | 4)     - - * * * . . . . | 4
| 66 61 55 48 40 32 25 19 14 | 5)     - - - * * * . . . | 5
| 71 67 62 56 49 41 33 26 20 | 6)     - - - - * * * . . | 6
| 75 72 68 63 57 50 42 34 27 | 7)     - - - - - * * * . | 7
| 78 76 73 69 64 58 51 43 35 | 8)     - - - - - - * * * | 8
| 80 79 77 74 70 65 59 52 44 | 9)     - - - - - - - * * | 9
------------------------------

4, Left down slope order(lbb)
   9) 8) 7) 6) 5) 4) 3) 2) 1)         9 8 7 6 5 4 3 2 1
------------------------------        ------------------+
|  0  1  3  6 10 15 21 28 36 | 1)     . . . . . . . * * | 1
|  2  4  7 11 16 22 29 37 45 | 2)     . . . . . . * * * | 2
|  5  8 12 17 23 30 38 46 53 | 3)     . . . . . * * * - | 3
|  9 13 18 24 31 39 47 54 60 | 4)     . . . . * * * - - | 4
| 14 19 25 32 40 48 55 61 66 | 5)     . . . * * * - - - | 5
| 20 26 33 41 49 56 62 67 71 | 6)     . . * * * - - - - | 6
| 27 34 42 50 57 63 68 72 75 | 7)     . * * * - - - - - | 7
| 35 43 51 58 64 69 73 76 78 | 8)     * * * - - - - - - | 8
| 44 52 59 65 70 74 77 79 80 | 9)     * * - - - - - - - | 9
------------------------------
--------------------------------------------------------------------------- */

#if defined __SSE2__
#include <x86intrin.h>

#define BB_OR(bb,b1,b2)     (bb).qpos = _mm_or_si128((b1).qpos,(b2).qpos)
#define BB_AND(bb,b1,b2)    (bb).qpos = _mm_and_si128((b1).qpos,(b2).qpos)
#define BB_ANDNOT(bb,b1,b2) (bb).qpos = _mm_andnot_si128((b2).qpos,(b1).qpos)

#define BBA_OR(bb,b1)       (bb).qpos = _mm_or_si128((bb).qpos,(b1).qpos)
#define BBA_AND(bb,b1)      (bb).qpos = _mm_and_si128((bb).qpos,(b1).qpos)
#define BBA_ANDNOT(bb,b1)   (bb).qpos = _mm_andnot_si128((b1).qpos,(bb).qpos)
#define BBA_XOR(bb,b1)      (bb).qpos = _mm_xor_si128((bb).qpos,(b1).qpos)

//bitboard上の特定bitへの駒配置
#define BB_SET(bb,p)        (bb).qpos = \
                            _mm_or_si128((bb).qpos,g_bpos[p].qpos)
#define BB_UNSET(bb,p)      (bb).qpos = \
                            _mm_andnot_si128(g_bpos[p].qpos,(bb).qpos)
#define VBB_SET(vbb,p)      (vbb).qpos = \
                            _mm_or_si128((vbb).qpos,g_vpos[p].qpos)
#define VBB_UNSET(vbb,p)    (vbb).qpos = \
                            _mm_andnot_si128(g_vpos[p].qpos,(vbb).qpos)
#define RBB_SET(rbb,p)      (rbb).qpos = \
                            _mm_or_si128((rbb).qpos,g_rpos[p].qpos)
#define RBB_UNSET(rbb,p)    (rbb).qpos = \
                            _mm_andnot_si128(g_rpos[p].qpos,(rbb).qpos)
#define LBB_SET(lbb,p)      (lbb).qpos = \
                            _mm_or_si128((lbb).qpos,g_lpos[p].qpos)
#define LBB_UNSET(lbb,p)    (lbb).qpos = \
                            _mm_andnot_si128(g_lpos[p].qpos,(lbb).qpos)

#define BB_INI(bb)          (bb).qpos = _mm_setzero_si128()
#define BB_CPY(dst,src)     (dst).qpos = _mm_load_si128(&((src).qpos))

typedef union _bitboard_t bitboard_t;
union _bitboard_t{
    uint32_t pos[3];
    __m128i qpos;
};

#elif defined __ARM_NEON__
#include <arm_neon.h>

#define BB_OR(bb,b1,b2)     (bb).qpos = vorrq_u32((b1).qpos,(b2).qpos)
#define BB_AND(bb,b1,b2)    (bb).qpos = vandq_u32((b1).qpos,(b2).qpos)
#define BB_ANDNOT(bb,b1,b2) (bb).qpos = vbicq_u32((b1).qpos,(b2).qpos)

#define BBA_OR(bb,b1)       (bb).qpos = vorrq_u32((bb).qpos,(b1).qpos)
#define BBA_AND(bb,b1)      (bb).qpos = vandq_u32((bb).qpos,(b1).qpos)
#define BBA_ANDNOT(bb,b1)   (bb).qpos = vbicq_u32((bb).qpos,(b1).qpos)
#define BBA_XOR(bb,b1)      (bb).qpos = veorq_u32((bb).qpos,(b1).qpos)

//bitboard上の特定bitへの駒配置
#define BB_SET(bb,p)        (bb).qpos = vorrq_u32((bb).qpos,g_bpos[p].qpos)
#define BB_UNSET(bb,p)      (bb).qpos = vbicq_u32((bb).qpos,g_bpos[p].qpos)
#define VBB_SET(vbb,p)      (vbb).qpos = vorrq_u32((vbb).qpos,g_vpos[p].qpos)
#define VBB_UNSET(vbb,p)    (vbb).qpos = vbicq_u32((vbb).qpos,g_vpos[p].qpos)
#define RBB_SET(rbb,p)      (rbb).qpos = vorrq_u32((rbb).qpos,g_rpos[p].qpos)
#define RBB_UNSET(rbb,p)    (rbb).qpos = vbicq_u32((rbb).qpos,g_rpos[p].qpos)
#define LBB_SET(lbb,p)      (lbb).qpos = vorrq_u32((lbb).qpos,g_lpos[p].qpos)
#define LBB_UNSET(lbb,p)    (lbb).qpos = vbicq_u32((lbb).qpos,g_lpos[p].qpos)

#define BB_INI(bb)          BBA_XOR(bb,bb)
#define BB_CPY(dst,src)     memcpy(&(dst),&(src),sizeof(bitboard_t))

typedef union _bitboard_t bitboard_t;
union _bitboard_t{
    uint32_t pos[3];
    uint32x4_t qpos;
};

#else
 
#define BB_OR(bb,b1,b2)     (bb).pos[0]=(b1).pos[0]|(b2).pos[0],\
                            (bb).pos[1]=(b1).pos[1]|(b2).pos[1],\
                            (bb).pos[2]=(b1).pos[2]|(b2).pos[2]

#define BB_AND(bb,b1,b2)    (bb).pos[0]=(b1).pos[0]&(b2).pos[0],\
                            (bb).pos[1]=(b1).pos[1]&(b2).pos[1],\
                            (bb).pos[2]=(b1).pos[2]&(b2).pos[2]

#define BB_ANDNOT(bb,b1,b2) (bb).pos[0]=(b1).pos[0]&(~((b2).pos[0])),\
                            (bb).pos[1]=(b1).pos[1]&(~((b2).pos[1])),\
                            (bb).pos[2]=(b1).pos[2]&(~((b2).pos[2]))

#define BBA_OR(bb,b1)       (bb).pos[0]|=(b1).pos[0],\
                            (bb).pos[1]|=(b1).pos[1],\
                            (bb).pos[2]|=(b1).pos[2]

#define BBA_AND(bb,b1)      (bb).pos[0]&=(b1).pos[0],\
                            (bb).pos[1]&=(b1).pos[1],\
                            (bb).pos[2]&=(b1).pos[2]

#define BBA_ANDNOT(bb,b1)   (bb).pos[0]&=~((b1).pos[0]),\
                            (bb).pos[1]&=~((b1).pos[1]),\
                            (bb).pos[2]&=~((b1).pos[2])

#define BBA_XOR(bb,b1)      (bb).pos[0]^=(b1).pos[0],\
                            (bb).pos[1]^=(b1).pos[1],\
                            (bb).pos[2]^=(b1).pos[2]

//bitboard上の特定bitへの駒配置
#define BB_SET(bb,p)        (bb).pos[0]|=g_bpos[p].pos[0],\
                            (bb).pos[1]|=g_bpos[p].pos[1],\
                            (bb).pos[2]|=g_bpos[p].pos[2]

#define BB_UNSET(bb,p)      (bb).pos[0]&=~(g_bpos[p].pos[0]),\
                            (bb).pos[1]&=~(g_bpos[p].pos[1]),\
                            (bb).pos[2]&=~(g_bpos[p].pos[2])

#define VBB_SET(vbb,p)      (vbb).pos[0]|=g_vpos[p].pos[0],\
                            (vbb).pos[1]|=g_vpos[p].pos[1],\
                            (vbb).pos[2]|=g_vpos[p].pos[2]

#define VBB_UNSET(vbb,p)    (vbb).pos[0]&=~(g_vpos[p].pos[0]),\
                            (vbb).pos[1]&=~(g_vpos[p].pos[1]),\
                            (vbb).pos[2]&=~(g_vpos[p].pos[2])

#define RBB_SET(rbb,p)      (rbb).pos[0]|=g_rpos[p].pos[0],\
                            (rbb).pos[1]|=g_rpos[p].pos[1],\
                            (rbb).pos[2]|=g_rpos[p].pos[2]

#define RBB_UNSET(rbb,p)    (rbb).pos[0]&=~(g_rpos[p].pos[0]),\
                            (rbb).pos[1]&=~(g_rpos[p].pos[1]),\
                            (rbb).pos[2]&=~(g_rpos[p].pos[2])

#define LBB_SET(lbb,p)      (lbb).pos[0]|=g_lpos[p].pos[0],\
                            (lbb).pos[1]|=g_lpos[p].pos[1],\
                            (lbb).pos[2]|=g_lpos[p].pos[2]

#define LBB_UNSET(lbb,p)    (lbb).pos[0]&=~(g_lpos[p].pos[0]),\
                            (lbb).pos[1]&=~(g_lpos[p].pos[1]),\
                            (lbb).pos[2]&=~(g_lpos[p].pos[2])

#define BB_INI(bb)          memset(&(bb), 0, sizeof(bitboard_t))
#define BB_CPY(dst,src)     memcpy(&(dst),&(src),sizeof(bitboard_t))

typedef union _bitboard_t bitboard_t;
union _bitboard_t{
    uint32_t pos[3];
};

#endif //__SSE2__

#define BB_TRUE             g_bb_pin[0]

#define BB_TEST(a)          ((a).pos[0]|(a).pos[1]|(a).pos[2])

//bitboard上の位置posの駒の有無
#define BPOS_TEST(bb,p)     ((p)<27 ? (bb).pos[0]&g_bpos[p].pos[0]:\
                             (p)<54 ? (bb).pos[1]&g_bpos[p].pos[1]:\
                             (p)<81 ? (bb).pos[2]&g_bpos[p].pos[2]:0)

extern bitboard_t g_bpos[N_SQUARE];  //Horizontal order        (-)
extern bitboard_t g_vpos[N_SQUARE];  //Vertical order          (|)
extern bitboard_t g_rpos[N_SQUARE];  //Right down slope order  (\)
extern bitboard_t g_lpos[N_SQUARE];  //Left down slope order   (/)

extern bitboard_t g_base_effect[N_KOMAS][N_SQUARE];  //effect table

extern bitboard_t g_bb_rank[N_RANK];
extern bitboard_t g_bb_file[N_FILE];
extern bitboard_t g_bb_pin[45];

extern bitboard_t g_bb_rank01;
extern bitboard_t g_bb_rank78;

extern bitboard_t g_bb_rank012;
extern bitboard_t g_bb_rank345;
extern bitboard_t g_bb_rank678;

extern bitboard_t g_bb_rank0123;
extern bitboard_t g_bb_rank5678;

void init_bpos(void);
void init_effect(void);
bitboard_t effect_emu(int pos, komainf_t koma, const bitboard_t *occupied);
bitboard_t effect_tbl(int pos,
                      komainf_t koma,
                      const bitboard_t *occupied,
                      const bitboard_t *voccupied,
                      const bitboard_t *roccupied,
                      const bitboard_t *loccupied);
#define EFFECT_EMU(pos,koma,sdata)  effect_emu\
                                    ((pos),(koma),&((sdata)->occupied))
#define EFFECT_TBL(pos,koma,sdata)  effect_tbl((pos),(koma),\
                                    &((sdata)->occupied), \
                                    &((sdata)->voccupied),\
                                    &((sdata)->roccupied),\
                                    &((sdata)->loccupied))

int        min_pos  (const bitboard_t *bb);
int        max_pos  (const bitboard_t *bb);


//将棋駒（持ち駒）
#define SENTE    0
#define GOTE     1
#define BOX      2
typedef struct _mkey_t mkey_t;
struct _mkey_t{
    unsigned int ou:11;
    unsigned int hi: 2;
    unsigned int ka: 2;
    unsigned int ki: 3;
    unsigned int gi: 3;
    unsigned int ke: 3;
    unsigned int ky: 3;
    unsigned int fu: 5;
};

enum { MKEY_EQUAL, MKEY_SUPER, MKEY_INFER, MKEY_OTHER};

#define MKEY_COMPARE(a, b) \
            ((!memcmp(&(a),&(b),sizeof(mkey_t))) ? MKEY_EQUAL : \
            (((a).fu >= (b).fu && (a).ky >= (b).ky && (a).ke >= (b).ke && \
              (a).gi >= (b).gi && (a).ki >= (b).ki && (a).ka >= (b).ka && \
              (a).hi >= (b).hi ) ? MKEY_SUPER : \
            (((a).fu <= (b).fu && (a).ky <= (b).ky && (a).ke <= (b).ke && \
              (a).gi <= (b).gi && (a).ki <= (b).ki && (a).ka <= (b).ka && \
              (a).hi <= (b).hi ) ? MKEY_INFER : MKEY_OTHER)))

#define TOTAL_MKEY(k) \
            ((k).hi+(k).ka+(k).ki+(k).gi+(k).ke+(k).ky+(k).fu)
#define MKEY_EXIST(k) \
            ((k).hi||(k).ka||(k).ki||(k).gi||(k).ke||(k).ky||(k).fu)
#define MKEY_COPY(dst,src) \
            memcpy(&(dst),&(src),sizeof(mkey_t))

unsigned int mkoma_num(mkey_t mkey, komainf_t koma);
mkey_t max_mkey(mkey_t key1, mkey_t key2);
mkey_t min_mkey(mkey_t key1, mkey_t key2);

//zobrist key
#define MAX_DEPTH      2000
#define N_ZOBRIST_SEED (N_SQUARE*N_KOMAS+MAX_DEPTH+1)
#define TURN_ADDRESS   2592
#define DEPTH_ADDRESS  2593

typedef uint64_t zkey_t;
extern zkey_t g_zkey_seed[N_ZOBRIST_SEED];
void create_seed(void);

//手番情報
typedef uint8_t  turn_t;
#define TURN_FLIP(tn)      ((tn)?(SENTE):(GOTE))
#define SELF_KOMA(tn,k)    ((tn) ? GOTE_KOMA(k):SENTE_KOMA(k))
#define ENEMY_KOMA(tn,k)   ((tn) ? SENTE_KOMA(k):GOTE_KOMA(k))

//二歩防止用フラグ
extern unsigned int g_bit[20];
typedef unsigned int flag_t;
#define FLAG(f,n)            ((f)&(g_bit[n]))
#define FLAG_SET(f,n)        ((f)|(g_bit[n]))
#define FLAG_UNSET(f,n)      ((f)&(~(g_bit[n])))

//将棋局面情報
/*
 -----------------------------------------------------------------------------
 minimum set局面情報を構造体ごとstandard set内に取り込むことにより
 処理のコンパクト化を図る。
 ----------------------------------------------------------------------------
*/
//#define SDATA_EXTENTION
//minimum set
typedef struct _ssdata_t ssdata_t;
struct _ssdata_t{
    short      count;
    turn_t     turn;
    komainf_t  board[N_SQUARE];
    mkey_t     mkey[3];
};
extern ssdata_t g_hirate;

typedef struct _sdata_t sdata_t;
//standard set
struct _sdata_t {
    ssdata_t core;              //minimal set
    zkey_t   zkey;              //zobrist key
    flag_t   fflag;             //file info. where FU can be dropped.
    char     ou[2];             //SOU, GOU
    
    bitboard_t occupied;        // occupied bb (-)
    bitboard_t voccupied;       //             (|)
    bitboard_t roccupied;       //             (\)
    bitboard_t loccupied;       //             (/)
    bitboard_t occup[2];        //
    
    // koma position (single)
    bitboard_t bb_fu[2];        // koma position
    bitboard_t bb_ky[2];        //
    bitboard_t bb_ke[2];        //
    bitboard_t bb_gi[2];        //
    bitboard_t bb_ki[2];        //
    bitboard_t bb_ka[2];        //
    bitboard_t bb_hi[2];        //
    bitboard_t bb_to[2];        //
    bitboard_t bb_ny[2];        //
    bitboard_t bb_nk[2];        //
    bitboard_t bb_ng[2];        //
    bitboard_t bb_um[2];        //
    bitboard_t bb_ry[2];        //
    
    // koma position (combination)
    bitboard_t bb_tk[2];        // KI,TO,NY,NK,NG
    bitboard_t bb_uk[2];        // KA,UM
    bitboard_t bb_rh[2];        // HI,RY
    
    // effect
    bitboard_t eff_fu[2];       // All FU
    bitboard_t effect[2];       // All koma
    
    //王手関連
    char n_oute;                //王手の数（両王手:2)
    char attack[2];             //王手している駒の位置
    //着手生成
    char pinned[N_SQUARE];      //pinに使用するマスクIDを格納
#ifdef SDATA_EXTENTION
    //簡易評価値
    short kscore;               //駒保有ベース
    char  np[2];                //入玉駒カウント
    void *user_data;
#endif //SDATA_EXTENTION
};


#define S_BOARD(sdata, pos)   (sdata)->core.board[pos]
#define S_SMKEY(sdata)        (sdata)->core.mkey[SENTE]
#define S_GMKEY(sdata)        (sdata)->core.mkey[GOTE]
#define S_COUNT(sdata)        (sdata)->core.count
#define S_TURN(sdata)         (sdata)->core.turn
#define SENTEBAN(sdata)       (sdata)->core.turn == SENTE
#define GOTEBAN(sdata)        (sdata)->core.turn == GOTE
#define S_ZKEY(sdata)         (sdata)->zkey
#define S_FFLAG(sdata)        (sdata)->fflag
#define S_NOUTE(sdata)        (sdata)->n_oute
#define S_PINNED(sdata)       (sdata)->pinned
#define S_ATTACK(sdata)       (sdata)->attack
#define S_ATK_KOMA(sdata)     S_BOARD(sdata, (sdata)->attack[0])

#ifdef SDATA_EXTENTION
#define S_KSCORE(sdata)       (sdata)->kscore
#define S_SNP(sdata)          (sdata)->np[SENTE]
#define S_GNP(sdata)          (sdata)->np[GOTE]
#endif //SDATA_EXTENTION


#define SMKEY_FU(sdata)       (sdata)->core.mkey[SENTE].fu
#define SMKEY_KY(sdata)       (sdata)->core.mkey[SENTE].ky
#define SMKEY_KE(sdata)       (sdata)->core.mkey[SENTE].ke
#define SMKEY_GI(sdata)       (sdata)->core.mkey[SENTE].gi
#define SMKEY_KI(sdata)       (sdata)->core.mkey[SENTE].ki
#define SMKEY_KA(sdata)       (sdata)->core.mkey[SENTE].ka
#define SMKEY_HI(sdata)       (sdata)->core.mkey[SENTE].hi
#define GMKEY_FU(sdata)       (sdata)->core.mkey[GOTE].fu
#define GMKEY_KY(sdata)       (sdata)->core.mkey[GOTE].ky
#define GMKEY_KE(sdata)       (sdata)->core.mkey[GOTE].ke
#define GMKEY_GI(sdata)       (sdata)->core.mkey[GOTE].gi
#define GMKEY_KI(sdata)       (sdata)->core.mkey[GOTE].ki
#define GMKEY_KA(sdata)       (sdata)->core.mkey[GOTE].ka
#define GMKEY_HI(sdata)       (sdata)->core.mkey[GOTE].hi

#define BB_SFU(sdata)         (sdata)->bb_fu[SENTE]
#define BB_SKY(sdata)         (sdata)->bb_ky[SENTE]
#define BB_SKE(sdata)         (sdata)->bb_ke[SENTE]
#define BB_SGI(sdata)         (sdata)->bb_gi[SENTE]
#define BB_SKI(sdata)         (sdata)->bb_ki[SENTE]
#define BB_SKA(sdata)         (sdata)->bb_ka[SENTE]
#define BB_SHI(sdata)         (sdata)->bb_hi[SENTE]
#define BB_STO(sdata)         (sdata)->bb_to[SENTE]
#define BB_SNY(sdata)         (sdata)->bb_ny[SENTE]
#define BB_SNK(sdata)         (sdata)->bb_nk[SENTE]
#define BB_SNG(sdata)         (sdata)->bb_ng[SENTE]
#define BB_SUM(sdata)         (sdata)->bb_um[SENTE]
#define BB_SRY(sdata)         (sdata)->bb_ry[SENTE]
#define BB_GFU(sdata)         (sdata)->bb_fu[GOTE]
#define BB_GKY(sdata)         (sdata)->bb_ky[GOTE]
#define BB_GKE(sdata)         (sdata)->bb_ke[GOTE]
#define BB_GGI(sdata)         (sdata)->bb_gi[GOTE]
#define BB_GKI(sdata)         (sdata)->bb_ki[GOTE]
#define BB_GKA(sdata)         (sdata)->bb_ka[GOTE]
#define BB_GHI(sdata)         (sdata)->bb_hi[GOTE]
#define BB_GTO(sdata)         (sdata)->bb_to[GOTE]
#define BB_GNY(sdata)         (sdata)->bb_ny[GOTE]
#define BB_GNK(sdata)         (sdata)->bb_nk[GOTE]
#define BB_GNG(sdata)         (sdata)->bb_ng[GOTE]
#define BB_GUM(sdata)         (sdata)->bb_um[GOTE]
#define BB_GRY(sdata)         (sdata)->bb_ry[GOTE]

#define BB_STK(sdata)         (sdata)->bb_tk[SENTE]
#define BB_GTK(sdata)         (sdata)->bb_tk[GOTE]
#define BB_SUK(sdata)         (sdata)->bb_uk[SENTE]
#define BB_GUK(sdata)         (sdata)->bb_uk[GOTE]
#define BB_SRH(sdata)         (sdata)->bb_rh[SENTE]
#define BB_GRH(sdata)         (sdata)->bb_rh[GOTE]

#define BB_OCC(sdata)         (sdata)->occupied
#define BB_VOC(sdata)         (sdata)->voccupied
#define BB_ROC(sdata)         (sdata)->roccupied
#define BB_LOC(sdata)         (sdata)->loccupied
#define BB_SOC(sdata)         (sdata)->occup[SENTE]
#define BB_GOC(sdata)         (sdata)->occup[GOTE]

#define EF_SFU(sdata)         (sdata)->eff_fu[SENTE]
#define EF_GFU(sdata)         (sdata)->eff_fu[GOTE]
#define SEFFECT(sdata)        (sdata)->effect[SENTE]
#define GEFFECT(sdata)        (sdata)->effect[GOTE]
#define SELF_EFFECT(sdata)    (sdata)->effect[S_TURN(sdata)]
#define ENEMY_EFFECT(sdata)   (S_TURN(sdata))?SEFFECT(sdata):GEFFECT(sdata)
        

#define INIT_SDATA_HIRATE(s)      initialize_sdata((s),&g_hirate)
#define S_SOU(sdata)              (sdata)->ou[SENTE]
#define S_GOU(sdata)              (sdata)->ou[GOTE]
#define OU_POS(sdata)             (sdata)->ou[(sdata)->core.turn]
#define SELF_OU(sdata)            OU_POS(sdata)
#define ENEMY_OU(sdata)           ((sdata)->core.turn?\
                                  (sdata)->ou[SENTE]:(sdata)->ou[GOTE])

#define SELF_MKEY(sdata)          (sdata)->core.mkey[(sdata)->core.turn]
#define SELF_FU(sdata)            (sdata)->core.mkey[(sdata)->core.turn].fu
#define SELF_KY(sdata)            (sdata)->core.mkey[(sdata)->core.turn].ky
#define SELF_KE(sdata)            (sdata)->core.mkey[(sdata)->core.turn].ke
#define SELF_GI(sdata)            (sdata)->core.mkey[(sdata)->core.turn].gi
#define SELF_KI(sdata)            (sdata)->core.mkey[(sdata)->core.turn].ki
#define SELF_KA(sdata)            (sdata)->core.mkey[(sdata)->core.turn].ka
#define SELF_HI(sdata)            (sdata)->core.mkey[(sdata)->core.turn].hi
#define ENEMY_MKEY(sdata) \
        (sdata)->core.mkey[TURN_FLIP((sdata)->core.turn)]
#define ENEMY_FU(sdata)           ENEMY_MKEY(sdata).fu
#define ENEMY_KY(sdata)           ENEMY_MKEY(sdata).ky
#define ENEMY_KE(sdata)           ENEMY_MKEY(sdata).ke
#define ENEMY_GI(sdata)           ENEMY_MKEY(sdata).gi
#define ENEMY_KI(sdata)           ENEMY_MKEY(sdata).ki
#define ENEMY_KA(sdata)           ENEMY_MKEY(sdata).ka
#define ENEMY_HI(sdata)           ENEMY_MKEY(sdata).hi


#define FU_CHECK(sdata,pos)       (!(((sdata)->fflag)&\
                                  (1<<(g_file[pos]+((sdata)->core.turn)*9))) &&\
                                  ((sdata)->core.turn?(pos)<72:(pos)>8))
#define KY_CHECK(sdata,pos)       ((sdata)->core.turn?(pos)<72:(pos)>8)
#define KE_CHECK(sdata,pos)       ((sdata)->core.turn?(pos)<63:(pos)>17)

#define SDATA_OCC_XOR(s,p) \
        BBA_XOR(BB_OCC(s),g_bpos[p]),BBA_XOR(BB_VOC(s),g_vpos[p]),\
        BBA_XOR(BB_ROC(s),g_rpos[p]),BBA_XOR(BB_LOC(s),g_lpos[p])

#define PASS        0
#define CHECKED     1
#define NIFU        2
#define ILL_POS     3

// ------------------------------------------------------
// destへ打てる持ち駒があるか
//　ある　true
//  ない　false
// ------------------------------------------------------
#define HAND_CHECK(sdata,dest)     \
        (SELF_HI(sdata)||SELF_KA(sdata)||SELF_KI(sdata)||SELF_GI(sdata)  \
         ||(SELF_KE(sdata) && KE_CHECK(sdata, dest))                     \
         ||(SELF_KY(sdata) && KY_CHECK(sdata, dest))                     \
         ||(SELF_FU(sdata) && FU_CHECK(sdata, dest)))?true:false

int  is_sdata_illegal  (sdata_t *sdata);
bitboard_t bb_to_effect(komainf_t koma,
                        const bitboard_t *bb_koma,
                        const sdata_t *sdata);
bitboard_t evasion_bb  (const sdata_t *sdata);
void sdata_bb_xor      (sdata_t *sdata, komainf_t koma, char pos);
int  oute_check        (sdata_t *sdata);
void create_pin        (sdata_t *sdata);
void create_effect     (sdata_t *sdata);
void initialize_sdata  (sdata_t *sdata, const ssdata_t *ssdata);
bool tsumi_check       (const sdata_t *sdata);

typedef struct _discpin_t discpin_t;
struct _discpin_t{
    int pin[N_SQUARE];
};
void set_discpin       (const sdata_t *sdata, discpin_t *disc);


//着手情報
/*-----------------------------------------------------------------------------
 着手データの２byte化
 投了や勝ち宣言などのActionはprev_posで定義する
 成らず、成りのActionはnew_posで定義する
 ---------------------------------------------------------------------------- */
#define HAND            100
#define PREV_POS(mv)    (mv).prev_pos
#define NEW_POS(mv)     (((mv).new_pos)&0x7f)
#define PROMOTE(mv)     (((mv).new_pos)&0x80)
#define MV_TORYO(mv)    (mv).prev_pos==TORYO
#define MV_ACTION(mv)   (mv).prev_pos>=TORYO
#define MV_MOVE(mv)     (mv).prev_pos<N_SQUARE
#define MV_DROP(mv)     (mv).prev_pos>HAND
#define MV_HAND(mv)     (mv).prev_pos-HAND

#define MV_CAPTURED(mv, sdata)  S_BOARD((sdata),NEW_POS(mv))

enum { NORMAL=0x0, PROMOTE, TORYO=0x80,CHUDAN,SENNICHITE,TIME_UP,
    ILLEGAL_MOVE,SILLEGAL_ACTION,GILLEGAL_ACTION,JISHOGI,
    KACHI,HIKIWAKE,MATTA,TSUMI,FUZUMI,ERROR
};
typedef struct _move_t move_t;
struct _move_t{
    uint8_t prev_pos;
    uint8_t new_pos;
};

extern move_t g_mv_toryo;

#define KOMA_PROMOTE(koma,src,dest) (g_koma_promotion[koma] &&\
                                    (((koma)>16)?((src)>53||(dest)>53):\
                                    ((src)<27||(dest)<27)))

#define SFU_NORMAL(dest)            ((dest)>8)
#define SKY_NORMAL(dest)            ((dest)>8)
#define SKE_NORMAL(dest)            ((dest)>17)
#define GFU_NORMAL(dest)            ((dest)<72)
#define GKY_NORMAL(dest)            ((dest)<72)
#define GKE_NORMAL(dest)            ((dest)<63)

#define SFU_PROMOTE(dest)           ((dest)<27)
#define SKY_PROMOTE(dest)           ((dest)<27)
#define SKE_PROMOTE(dest)           ((dest)<27)
#define SGI_PROMOTE(src,dest)       ((src)<27||(dest)<27)
#define SKA_PROMOTE(src,dest)       ((src)<27||(dest)<27)
#define SHI_PROMOTE(src,dest)       ((src)<27||(dest)<27)

#define GFU_PROMOTE(dest)           ((dest)>53)
#define GKY_PROMOTE(dest)           ((dest)>53)
#define GKE_PROMOTE(dest)           ((dest)>53)
#define GGI_PROMOTE(src,dest)       ((src)>53||(dest)>53)
#define GKA_PROMOTE(src,dest)       ((src)>53||(dest)>53)
#define GHI_PROMOTE(src,dest)       ((src)>53||(dest)>53)

#define MV_SET(mv,p,n,a)            (mv).prev_pos = (p),\
                                    (mv).new_pos  = (n),\
                                    (mv).new_pos  +=a*0x80

int sdata_move_forward(sdata_t *sdata, move_t move);
int sdata_key_forward (sdata_t *sdata, move_t move);
bitboard_t sdata_create_effect(sdata_t *sdata);
bool fu_tsume_check   (move_t move,   const sdata_t *sdata);


#endif /* shogi_h */
