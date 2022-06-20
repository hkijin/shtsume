//
//  tests.c
//  shshogi
//
//  Created by Hkijin on 2021/11/01.
//

#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "tests.h"

/*
 Built-in Function: int __builtin_ctz (unsigned int x)
    Returns the number of trailing 0-bits in x,
    starting at the least significant bit position.
    If x is 0, the result is undefined.
 */

static bitboard_t bb_effect(komainf_t koma, const bitboard_t *bb_koma);

void builtin_func_test1(void){
    int i;
    int res;
    printf("TEST1 min_pos & max_pos test: ");
    for(i=0; i<N_SQUARE; i++){
        res = min_pos(&g_bpos[i]);
        if(res!=i){
            printf("FAILED in min_pos\n");
            return;
        }
    }
    for(i=0; i<N_SQUARE; i++){
        res = max_pos(&g_bpos[i]);
        if(res!=i){
            printf("FAILED in max_pos\n");
            return;
        }
    }
    
    //ランダムに2点trueのbitboardを準備。
    int p1, p2, a1, a2;
    bitboard_t bb;
    p1 = rand()%N_SQUARE;
    do{ p2 = rand()%N_SQUARE; } while(p1==p2);
    bb = g_bpos[p1];
    BBA_XOR(bb, g_bpos[p2]);
    a1 = min_pos(&bb);
    a2 = max_pos(&bb);
    printf("%d %d ", a1, a2);
    if(a1 == a2) printf("FAILED\n");
    else printf("PASSED\n");
    return;
}

void builtin_func_test2(void){
    int koma;
    int pos[4];
    //駒の指定
    printf("駒?");
    scanf("%d", &koma);
    //位置1-位置4の指定
    int i;
    for(i=0; i<4; i++){
        printf("駒位置[%d]?", i);
        scanf("%d",&pos[i]);
        if(pos[i]>=N_SQUARE ||pos[i]<0) break;
    }
    //位置bitboard作成
    bitboard_t bb;
    BB_INI(bb);
    for(i=0; i<4; i++){
        if(pos[i]>=N_SQUARE||pos[i]<0) break;
        BBA_OR(bb, g_bpos[pos[i]]);
    }
    //bitboard表示
    printf("駒=%s\n", g_koma_char[koma]);
    printf("駒位置bitboard\n");
    BITBOARD_PRINTF(&bb);
    
    
    //利きbitboardの作成
    bitboard_t eff = bb_effect(koma, &bb);
    
    //利きbitboardの表示
    printf("利きbitboard\n");
    BITBOARD_PRINTF(&eff);
    
    return;
}

bitboard_t bb_effect   (komainf_t koma,
                        const bitboard_t *bb_koma){
    int id;
    bitboard_t eff, bb = *bb_koma;
    BB_INI(eff);
    while(1){
        id = min_pos(&bb);
        if(id<0) break;
        BBA_OR(eff, g_base_effect[koma][id]);
        BBA_XOR(bb, g_bpos[id]);
    }
    return eff;
}

/*
void ka_func_test(void){
    int res = KA_FUNC(30,70);
    printf("res = %d\n", res);
    BITBOARD_PRINTF(&g_bb_pin[res]);
}
*/
/*
void hi_func_test(void){
    int res = HI_FUNC(4,76);
    printf("res = %d\n", res);
    BITBOARD_PRINTF(&g_bb_pin[res]);
}
 */

void print_hash_seed(void){
    for(int i=0; i<N_KOMAS; i++){
        for(int j=0; j<N_SQUARE; j++)
            printf("%4d %3s %2d 0X%llX\n",
                   i*N_SQUARE+j,
                   g_koma_char[i],
                   j,
                   g_zkey_seed[i*N_SQUARE+j]);
    }
}

#define PRINT_TEST(a, num, sdata)        \
    printf(#a"\n"),                      \
    (num) = SDATA_PRINTF((sdata), a),    \
    printf("文字数 = %d\n", (num))

void test_sdata_print(void){
    int num;
    //平手局面
    sdata_t sbuf;
    INIT_SDATA_HIRATE(&sbuf);
    PRINT_TEST(PR_BOARD,num,&sbuf);
    PRINT_TEST(PR_ZKEY,num,&sbuf);
    PRINT_TEST(PR_FFLAG,num,&sbuf);
    PRINT_TEST(PR_OU,num,&sbuf);
    PRINT_TEST(PR_OCCUP,num,&sbuf);
    PRINT_TEST(PR_PINNED,num,&sbuf);
    PRINT_TEST(PR_OUTE,num,&sbuf);
    PRINT_TEST(PR_SCORE,num,&sbuf);
    PRINT_TEST(PR_BBFU,num,&sbuf);
    PRINT_TEST(PR_BBKY,num,&sbuf);
    PRINT_TEST(PR_BBKE,num,&sbuf);
    PRINT_TEST(PR_BBGI,num,&sbuf);
    PRINT_TEST(PR_BBKI,num,&sbuf);
    PRINT_TEST(PR_BBKA,num,&sbuf);
    PRINT_TEST(PR_BBHI,num,&sbuf);
    PRINT_TEST(PR_BBTO,num,&sbuf);
    PRINT_TEST(PR_BBNY,num,&sbuf);
    PRINT_TEST(PR_BBNK,num,&sbuf);
    PRINT_TEST(PR_BBNG,num,&sbuf);
    PRINT_TEST(PR_BBUM,num,&sbuf);
    PRINT_TEST(PR_BBRY,num,&sbuf);
    PRINT_TEST(PR_BBTK,num,&sbuf);
    PRINT_TEST(PR_BBUK,num,&sbuf);
    PRINT_TEST(PR_BBRH,num,&sbuf);
    PRINT_TEST(PR_EFFU,num,&sbuf);
    PRINT_TEST(PR_EFFECT,num,&sbuf);
    PRINT_TEST(PR_BASIC,num,&sbuf);
    PRINT_TEST(PR_BITBOARD,num,&sbuf);
    PRINT_TEST(PR_MISC,num,&sbuf);
    PRINT_TEST(PR_ALL,num,&sbuf);
    return;
}

void test_move_forward(void){
    //平手局面
    sdata_t sbuf;
    INIT_SDATA_HIRATE(&sbuf);
    move_t move ={0,0};
    PREV_POS(move) = 60;
    move.new_pos = 51;
    sdata_move_forward(&sbuf, move);
    SDATA_PRINTF(&sbuf, PR_BOARD);
    return;
}









