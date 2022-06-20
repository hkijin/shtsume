//
//  tests.h
//  shshogi
//
//  Created by Hkijin on 2021/11/01.
//

#ifndef tests_h
#define tests_h

#include <stdio.h>
#include "shogi.h"
#include "usi.h"
#include "dtools.h"


extern char *g_sfen_defense[];

extern char *g_sfen_problem[];
extern char *g_sfen_tsume[];
extern char *g_sfen_fudume[];
extern char *g_sfen_ftest[];
extern char *g_sfen_capture[];

void builtin_func_test1(void);   //g_bpos[]の逆変換テスト
void builtin_func_test2(void);   //位置bitboardから利きbitboardへの変換

void print_hash_seed(void);      //zobrist_keyの打ち出し

//sdata表示TEST
void test_sdata_print(void);     //sdata表示関数の検証

//局面進行関数のテスト
void test_move_forward(void);









#endif /* tests_h */
