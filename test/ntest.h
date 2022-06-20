//
//  ntest.h
//  nsolver
//
//  Created by Hkijin on 2022/02/04.
//

#ifndef ntest_h
#define ntest_h

#include <stdio.h>
#include "dtools.h"
#include "shtsume.h"
#include "tests.h"
#include "ndtools.h"

//テストハンドラー
void test_handler(const char *optarg);
//盤上距離テーブルの中身確認
void test_init_distance(void);
//cpu情報の取得
void cpu_info(void);
//デバッグ用シミュレーション
void tsume_test(void);
//王手に対する合法手表示
void test_generate_evasion(void);

#endif /* ntest_h */
