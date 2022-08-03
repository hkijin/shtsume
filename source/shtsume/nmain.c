//
//  main.c
//  shtsume
//
//  Created by Hkijin on 2022/02/02.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include "shogi.h"
#include "usi.h"
#include "shtsume.h"
//#include "ntest.h"

static const struct option longopts[] = {
    {"help",    no_argument,        NULL,   'h'},
    {"version", no_argument,        NULL,   'v'},
    {"test",    required_argument,  NULL,   't'},
    {NULL,      0,                  NULL,   0  }
};

static void print_version (void);

int main(int argc, char * const argv[]) {
    //argc,argvの処理
    int optc;
    
    while((optc = getopt_long(argc, argv, "hvt:", longopts, NULL))!= -1)
        switch(optc){
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                print_version();
                exit(EXIT_SUCCESS);
                break;
            case 't':
                //テストコマンド用予約オプション
                //test_handler(optarg);
                exit(EXIT_SUCCESS);
                break;
            default:
                //USIエンジンとして使用
                break;
        }
    
    //基本ライブラリの初期化処理
    create_seed();                        //zkey
    init_distance();                      //g_distance
    init_bpos();                          //bitboard
    init_effect();                        //effect
    srand((unsigned)time(NULL));
    
    usi_main();

    return 0;
}

void print_version (void)
{
    printf("%s %s\n",PROGRAM_NAME,VERSION_INFO);
    return;
}
