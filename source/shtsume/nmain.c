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
#include "ndtools.h"
//#include "ntest.h"

static bool st_display;

static const struct option longopts[] =
{
 //サポートオプション
     {"help",    no_argument,        NULL,   'h'},
     {"version", no_argument,        NULL,   'v'},
 //探索オプション
     {"kifu",    no_argument,        NULL,   'k'},  //g_out_lvkif
     {"log",     no_argument,        NULL,   'g'},  //g_summary
     {"display", no_argument,        NULL,   'd'},  //st_display
     {"minpn",   required_argument,  NULL,   'n'},  //g_mt_min_pn
     {"memory",  required_argument,  NULL,   'm'},  //g_usi_hash
     {"level",   required_argument,  NULL,   'l'},  //g_search_level
     {"limit",   required_argument,  NULL,   'i'},  //g_time_limit
};

static void print_version (void);
 
int main(int argc, char * const argv[]) {
    //argc,argvの処理
    int optc;
    
    while((optc = getopt_long(argc, argv, "hvkgdn:m:l:i:",
                              longopts, NULL))!= -1)
        switch(optc){
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                print_version();
                exit(EXIT_SUCCESS);
                break;
                
            case 'k': g_out_lvkif = true; break;
            case 'g': g_summary = true;   break;
            case 'd': st_display = true;  break;
            case 'n': g_mt_min_pn = atoi(optarg);    break;
            case 'm': g_usi_hash  = atoi(optarg);    break;
            case 'l': g_search_level = atoi(optarg); break;
            case 'i': g_time_limit = atoi(optarg);   break;
        };
    // オプション、引数なしの場合、usiエンジンとして起動
    if(argc == 1){
        //基本ライブラリの初期化処理
        create_seed();                        //zkey
        init_distance();                      //g_distance
        init_bpos();                          //bitboard
        init_effect();                        //effect
        srand((unsigned)time(NULL));
        
        usi_main();
        
        return 0;
    }
    // sfen_stringが欠落している場合、helpを表示して終了。
    else if (argc != optind+1){
        print_help();
        exit(EXIT_SUCCESS);
    }
    // コマンドラインアプリとして起動
    else{
        char p_string[128];
        strncpy(p_string, argv[optind], strlen(argv[optind]));
        //基本ライブラリの初期化処理
        create_seed();                        //zkey
        init_distance();                      //g_distance
        init_bpos();                          //bitboard
        init_effect();                        //effect
        srand((unsigned)time(NULL));
        
        //詰将棋条件の表示
        ssdata_t ssdata;
        sfen_to_ssdata(p_string, &ssdata);
        initialize_sdata(&g_sdata, &ssdata);
        printf("-----局面図-----\n");
        SDATA_PRINTF(&g_sdata, PR_BOARD);
        
        //tbaseの作成
        uint64_t size = g_usi_hash*MCARDS_PER_MBYTE-1;
        g_tbase = create_tbase(size);
        g_mtt = create_mtt(MTT_SIZE);
        
        //探索条件の表示
        printf("-----探索条件-----\n");
        printf("局面表 %llu(MByte)\n",g_usi_hash);
        printf("探索レベル %u\n", g_search_level);
        if(!g_time_limit) g_time_limit = TM_INFINATE;
        if(g_time_limit<0) printf("探索時間 制限なし\n");
        else printf("探索時間 %d(sec)\n",g_time_limit);
        printf("------------------\n");
        //詰探索
        clock_t start = clock();
        tdata_t tdata;
        
        bn_search(&g_sdata, &tdata, g_tbase);
        
        clock_t finish = clock();
        
        if(!tdata.pn){
            //詰手順、探索情報表示
            printf("詰みました。%u手詰め\n",tdata.sh);
            printf("消費時間 %lu(sec)\n", (finish-start)/CLOCKS_PER_SEC);
            printf("探索局面数 %llu\n", g_tsearchinf.nodes);
            printf("詰探索木ノード数　%llu\n", g_tbase->pr_num);
            printf("局面表登録数　%llu\n", g_tbase->num);
            tsume_print(&g_sdata, g_tbase, TP_NONE);
            if(st_display) tsume_debug(&g_sdata, g_tbase);
        }
        else if(!tdata.dn) {
            printf("不詰です。\n");
        }
        else {
            printf("その他です。");
        }
        
        initialize_tbase(g_tbase);
        init_mtt(g_mtt);
        mlist_free_stack();
        mvlist_free_stack();
        return 0;
    }
}

 




/*
static const struct option longopts[] = {
    {"help",    no_argument,        NULL,   'h'},
    {"version", no_argument,        NULL,   'v'},
    {"solve",   required_argument,  NULL,   's'},
    {"test",    required_argument,  NULL,   't'},
    {NULL,      0,                  NULL,   0  }
};

static void print_version (void);
static void solve(const char *optarg);

int main(int argc, char * const argv[]) {
    //argc,argvの処理
    int optc;
    
    while((optc = getopt_long(argc, argv, "hvs:t:", longopts, NULL))!= -1)
        switch(optc){
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                print_version();
                exit(EXIT_SUCCESS);
                break;
            case 's':
                solve(optarg);
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
*/
void print_version (void)
{
    printf("%s %s\n",PROGRAM_NAME,VERSION_INFO);
    return;
}

void solve(const char *optarg)
{
    char p_string[128];
    strncpy(p_string, "position sfen ", strlen("position sfen "));
    strncat(p_string, optarg, 112);

    //基本ライブラリの初期化
    create_seed();                        //zkey
    init_distance();                      //g_distance
    init_bpos();                          //bitboard
    init_effect();                        //effect
    srand((unsigned)time(NULL));
    
    //logfile作成
    char *path = getenv("HOME");
    strncpy(g_logfile_path,path, strlen(path));
    strncat(g_logfile_path, "/Documents/", sizeof("/Documents/"));
    create_log_filename();
    strncpy(g_user_path,g_logfile_path, sizeof(g_logfile_path));
    
    //usiコマンド受信のシミュレーション
    res_usi_cmd();
    res_setoption_cmd("setoption name USI_Ponder value true");
    res_setoption_cmd("setoption name USI_Hash value 6144" );
    res_setoption_cmd("setoption name search_level value 0");
    res_setoption_cmd("setoption name out_lvkif value true");
    res_setoption_cmd("setoption name summary value true");
    char commandstr[256];
    sprintf(commandstr, "setoption name user_path value %s", g_user_path);
    res_setoption_cmd(commandstr);
    printf("g_usi_ponder   = %d\n", g_usi_ponder);
    printf("g_usi_hash     = %llu\n", g_usi_hash  );
    printf("g_search_level = %u\n", g_search_level );
    printf("g_out_lvkif    = %s\n", g_out_lvkif?"true":"false");
    printf("g_summary      = %s\n", g_summary?"true":"false");
    res_isready_cmd();
    res_position_cmd(p_string);
    
    //読み込み局面の確認
    SDATA_PRINTF(&g_sdata, PR_BOARD);
    res_gomate_cmd("go mate infinite");
    return;
}
