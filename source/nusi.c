//
//  nusi.c
//  shtsume
//
//  Created by Hkijin on 2022/02/03.
//

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "shogi.h"
#include "usi.h"
#include "dtools.h"
#include "shtsume.h"
#include "ndtools.h"

/* 思考エンジンをUSIインターフェースを通じて動かす場合の雛形ファイルです。*/

/* -------------
 グローバル変数
 ------------- */
usioption_t g_usioption =
{
    {"USI_Hash"    , TBASE_SIZE_DEFAULT , TBASE_SIZE_MIN , TBASE_SIZE_MAX },
    {"USI_Ponder"  , true             } ,
    {"mt_min_pn"   , LEAF_PN_DEFAULT    , LEAF_PN_MIN    , LEAF_PN_MAX    },
    {"n_make_tree" , MAKE_REPEAT_DEFAULT, MAKE_REPEAT_MIN, MAKE_REPEAT_MAX},
    {"search_level", SEARCH_LV_DEFAULT  , SEARCH_LV_MIN  , SEARCH_LV_MAX  },
    {"out_lvkif"   , false             },
    {"user_path"   , "<empty>"         },
    {"summary"     , false             },
    {"s_allmove"   , false             }
};

bool          g_usi_ponder      = false;
uint64_t      g_usi_hash        = TBASE_SIZE_DEFAULT;
uint16_t      g_mt_min_pn       = LEAF_PN_DEFAULT;
uint8_t       g_n_make_tree     = MAKE_REPEAT_DEFAULT;
uint8_t       g_search_level    = SEARCH_LV_DEFAULT;
bool          g_out_lvkif       = false;
short         g_pv_length       = PV_USI_DEFAULT;
bool          g_disp_search     = false;    //読み筋情報の表示(true:表示あり)
bool          g_summary         = false;    //サマリーレポートの出力有無(true:有り)
uint32_t      g_smode           = TP_NONE;  //サマリーレポート詰方候補手　詰着手のみ

tbase_t *g_tbase;
mtt_t   *g_mtt;
char g_sfen_pos_str[256];          //GUIからの送られて来た局面文字列を保管
char g_error_location[256];        //エラー発生箇所を示す文字列
char g_user_path[256];             //shtsumeの出力ファイル格納場所

//usiコマンド
void res_usi_cmd          (void)
{
    //idコマンドを返す
    char str[128];
    sprintf(str, "id name %s (%s)\nid author %s",
            PROGRAM_NAME, VERSION_INFO, AUTHOR_NAME);
    puts(str);
    record_log(str);
    //optionコマンドを返す。
    //USI_Hash
    sprintf(str, "option name %s type spin default %d min %d max %d",
            g_usioption.usi_hash.op_name,
            g_usioption.usi_hash.default_value,
            g_usioption.usi_hash.min,
            g_usioption.usi_hash.max);
    puts(str);
    record_log(str);
    //USI_Ponder
    sprintf(str, "option name %s type check default %s",
            g_usioption.usi_ponder.op_name,
            g_usioption.usi_ponder.default_value?"true":"false");
    puts(str);
    record_log(str);
    //mt_min_pn
    sprintf(str, "option name %s type spin default %d min %d max %d",
            g_usioption.mt_min_pn.op_name,
            g_usioption.mt_min_pn.default_value,
            g_usioption.mt_min_pn.min,
            g_usioption.mt_min_pn.max);
    puts(str);
    record_log(str);
     
    //search_level
    sprintf(str, "option name %s type spin default %d min %d max %d",
            g_usioption.search_level.op_name,
            g_usioption.search_level.default_value,
            g_usioption.search_level.min,
            g_usioption.search_level.max);
    puts(str);
    record_log(str);
    
    //out_lvkif
    sprintf(str, "option name %s type check default %s",
            g_usioption.out_lvkif.op_name,
            g_usioption.out_lvkif.default_value?"true":"false");
    puts(str);
    record_log(str);
    
    //探索レポート
    sprintf(str, "option name %s type check default %s",
            g_usioption.summary.op_name,
            g_usioption.summary.default_value?"true":"false");
    puts(str);
    record_log(str);
    
    //探索レポートオプション
    sprintf(str, "option name %s type check default %s",
            g_usioption.s_allmove.op_name,
            g_usioption.s_allmove.default_value?"true":"false");
    puts(str);
    record_log(str);
    
    //棋譜＆レポート保存フォルダ
    sprintf(str, "option name %s type string default %s",
            g_usioption.user_path.op_name,
            getenv("HOME"));
    puts(str);
    record_log(str);
    
    //usiokコマンドを返す。
    sprintf(str, "usiok\n");
    puts(str);
    record_log(str);
    return;
}

//isreadyコマンド
void res_isready_cmd      (void)
{
    //tbaseの作成
    uint64_t size = g_usi_hash*MCARDS_PER_MBYTE-1;
    g_tbase = create_tbase(size);
    g_mtt = create_mtt(MTT_SIZE);
    g_disp_search = true;
    
    //OKを返す
    char str[12];
    sprintf(str, "readyok\n");
    puts(str);
    record_log(str);
    return;
}

//set_optionコマンド
void res_setoption_cmd    (const char *buf)
{
    char str[32];
    size_t len;
    if     (!strncmp(buf,         "setoption name USI_Ponder value ",
                     len = strlen("setoption name USI_Ponder value ")))
    {
        if     (!strncmp(buf+len, "true",  strlen("true")))
        {
            g_usi_ponder = true;
            sprintf(str, "setoption USI_Ponder true\n");
            record_log(str);
        }
            
        else if(!strncmp(buf+len, "false", strlen("false")))
        {
            g_usi_ponder = false;
            sprintf(str, "setoption USI_Ponder false\n");
            record_log(str);
        }
    }
    else if(!strncmp(buf,         "setoption name USI_Hash value ",
                     len = strlen("setoption name USI_Hash value ")))
    {
        g_usi_hash = atoi(buf+len);
        sprintf(str, "setoption USI_Hash %llu\n", g_usi_hash);
        record_log(str);
    }
    else if(!strncmp(buf,         "setoption name mt_min_pn value ",
                     len = strlen("setoption name mt_min_pn value ")))
    {
        g_mt_min_pn = atoi(buf+len);
        sprintf(str, "setoption g_mt_min_pn %u\n", g_mt_min_pn);
        record_log(str);
    }
    else if(!strncmp(buf,         "setoption name n_make_tree value ",
                     len = strlen("setoption name n_make_tree value ")))
    {
        g_n_make_tree = atoi(buf+len);
        sprintf(str, "setoption n_make_tree %u\n", g_n_make_tree);
        record_log(str);
    }
    else if(!strncmp(buf,         "setoption name search_level value ",
                     len = strlen("setoption name search_level value ")))
    {
        g_search_level = atoi(buf+len);
        sprintf(str, "setoption search_level %u\n", g_search_level);
        record_log(str);
    }
    
    else if(!strncmp(buf,         "setoption name user_path value ",
                     len = strlen("setoption name user_path value ")))
    {
        const char *upath = buf+len;
        strncpy(g_user_path, upath, strlen(upath)-1);
#if DEBUG
        printf("g_user_path = %s\n",g_user_path);
#endif //DEBUG
    }
    
    else if(!strncmp(buf,         "setoption name out_lvkif value ",
                     len = strlen("setoption name out_lvkif value ")))
    {
        if     (!strncmp(buf+len, "true",  strlen("true")))
        {
            g_out_lvkif = true;
            sprintf(str, "setoption out_lvkif true\n");
            record_log(str);
        }
            
        else if(!strncmp(buf+len, "false", strlen("false")))
        {
            g_out_lvkif = false;
            sprintf(str, "setoption out_lvkif false\n");
            record_log(str);
        }
    }

    else if(!strncmp(buf,         "setoption name summary value ",
                     len = strlen("setoption name summary value ")))
    {
        if     (!strncmp(buf+len, "true",  strlen("true")))
        {
            g_summary = true;
            sprintf(str, "setoption summary true\n");
            record_log(str);
        }
            
        else if(!strncmp(buf+len, "false", strlen("false")))
        {
            g_summary = false;
            sprintf(str, "setoption summary false\n");
            record_log(str);
        }
    }
    
    else if(!strncmp(buf,         "setoption name s_allmove value ",
                     len = strlen("setoption name s_allmove value ")))
    {
        if     (!strncmp(buf+len, "true",  strlen("true")))
        {
            g_smode = (TP_NONE|TP_ALLMOVE);
            sprintf(str, "setoption s_allmove true\n");
            record_log(str);
        }
            
        else if(!strncmp(buf+len, "false", strlen("false")))
        {
            g_smode = TP_NONE;
            sprintf(str, "setoption s_allmove false\n");
            record_log(str);
        }
    }
    return;
}

//usinewgameコマンド
void res_usinewgame_cmd   (void)
{
    //デバッグ用
    sprintf(g_str, "usinewgame cmd received.\n");
    record_log(g_str);
}

//positionコマンド
void res_position_cmd     (const char *buf)
{
    //デバッグ用
    strncpy(g_sfen_pos_str, buf, sizeof(char)*256);
    sprintf(g_str, "position cmd received.\n");
    record_log(g_str);
    
    memcpy(g_str, buf, strlen(buf)+1);
    //局面読み込み
    int len = strlen("position ");
    char *ch = g_str+len;
    if     (!strncmp(ch, "startpos ", 9))
    {
        INIT_SDATA_HIRATE(&g_sdata);          //平手局面を準備
        ch += 9;
    }
    else if(!strncmp(ch, "sfen "    , 5))
    {
        ssdata_t ssdata;
        ch += 5;
        ch = sfen_to_ssdata(ch, &ssdata);
        initialize_sdata(&g_sdata, &ssdata);  //sfen記述法に基づき局面読み込み
    }
    else
    {
        error_log(USI_UNKNOWN_MSG); //ありえないのでエラー表示して異常終了
    }
    //着手読み込み
    //詰将棋の問題局面として平手局面＋着手も考慮
    if(!strncmp(ch, "moves ", 6)){
        ch += 6;
        move_t move;
        while(*ch){
            if(*ch == ' ') break;
            ch = sfen_to_move(&move, ch);
            //局面表に登録
            sdata_move_forward(&g_sdata, move);
        }
    }
    return;
}

//goコマンド
void res_gomate_cmd       (const char *buf)
{
    //デバッグ用
    sprintf(g_str, "go mate cmd received.\n");
    record_log(g_str);
    //制限時間情報の取得
    memcpy(g_str, buf, strlen(buf)+1);
    int len = strlen("go mate ");
    const char *ch = g_str+len;
    g_time_mode = MD_DEBUG;
    if(!strncmp(ch, "infinite", strlen("infinite")))
    {
        g_time_mode = MD_INFINATE;
        g_time_limit = TM_INFINATE;
    }
    else
    {
        g_time_mode = MD_KM;
        g_time_limit = atoi(ch);
    }
    //詰探索開始
#if DEBUG
    clock_t start = clock();
#endif /* DEBUG */
    tdata_t tdata;
    bn_search(&g_sdata, &tdata, g_tbase);

    if      (!tdata.pn) {
        if(g_error){
            sprintf(g_str, "info string failed to create move list\n");
            record_log(g_str);
            puts(g_str);
            sprintf(g_str, "checkmate nomate");
            record_log(g_str);
            puts(g_str);
        }
        else{
            //チェックメイトコマンドを返す
            int num = 0;
            num += sprintf(g_str+num, "checkmate ");
            int i=0;
            move_t mv;
            for(i=0; i<TSUME_MAX_DEPTH; i++){
                mv = g_tsearchinf.mvinf[i].move;
                if (MV_TORYO(mv)) break;
                num += move_to_sfen(g_str+num, mv);
                num += sprintf(g_str+num, " ");
            }
            record_log(g_str);
            puts(g_str);
            if(g_summary) create_search_report();
            
    #if DEBUG
            clock_t finish = clock();
            printf("経過時間 %lu(sec)\n", (finish-start)/CLOCKS_PER_SEC);
            printf("探索局面数 %llu\n", g_tsearchinf.nodes);
            printf("詰探索木ノード数　%llu\n", g_tbase->pr_num);
            printf("局面表登録数　%llu\n", g_tbase->num);
            tsume_print(&g_sdata, g_tbase, TP_ZKEY);
            tsume_debug(&g_sdata, g_tbase);
    #endif /* DEBUG */
        }
    }
    else if (!tdata.dn) {
        //checkmate nomate
        sprintf(g_str, "checkmate nomate");
        record_log(g_str);
        puts(g_str);
    }
    else                {
        sprintf(g_str,"info string 王手千日手.");
        puts(g_str);
        record_log(g_str);
#if DEBUG
        tsume_debug(&g_sdata, g_tbase);
#endif //DEBUG
        sprintf(g_str, "checkmate nomate");
        puts(g_str);
        record_log(g_str);
    }
    initialize_tbase(g_tbase);
    init_mtt(g_mtt);
    mlist_free_stack();
    mvlist_free_stack();
    return;
}

void res_goponder_cmd     (const char *buf)
{
    //デバッグ用
    sprintf(g_str, "go ponder cmd received.\n");
    record_log(g_str);
    /*
     * エンジンはponderを送らないので省略。
     */
}

void res_go_cmd           (const char *buf)
{
    //デバッグ用
    sprintf(g_str, "go cmd received.\n");
    record_log(g_str);
    //詰将棋専用エンジンなのでこの場合、即投了する。
    move_t best_move = g_mv_toryo;
    
    char mv_ch[12];
    move_to_sfen(mv_ch, best_move);
    sprintf(g_str, "bestmove %s", mv_ch);
    puts(g_str);
    record_log(g_str);
    return;
}

//stopコマンド
void res_stop_cmd         (void)
{
    //デバッグ用
    sprintf(g_str, "stop cmd received.\n");
    record_log(g_str);
    /*
     * 詰探索実施中にstopコマンドを受けた場合、グローバル変数g_stop_recievedがtrue
     * に成るため、中断されるが、ここで変数をリセットしておく。
     */
    g_stop_received = false;
}

/*
 ponderhit
 エンジンが先読み中、
 前回のbestmoveコマンドでエンジンが予想した通りの手を相手が指した時に送ります。
 エンジンはこれを受信すると、先読み思考から通常の思考に切り替わることになり、
 任意の時点でbestmoveで指し手を返すことができます。
 */
void res_ponderhit_cmd    (void)
{
    /*
     * エンジンはponderを送らないので省略。
     */
}

//gameoverコマンド
void res_gameover_cmd     (void)
{
    //デバッグ用
    sprintf(g_str, "gameover cmd received.\n");
    record_log(g_str);
}

/* -------------------------------------------------------------------------
 探索レポート
 以下の情報を含む探索レポートを作成する。
 ・ファイル名　search_reportyymmddhhmmss.log
 ・プログラム＆バージョン
 ・設定（メモリ他）
 ・解図時間
 ・探索局面数
 ・GC回数
 ・初期局面
 ・詰め手順詳細
 ------------------------------------------------------------------------ */
int create_search_report(void)
{
    //ファイル名設定
    time_t now;
    time(&now);
    struct tm t = *localtime(&now);
    char s[16];
    strftime(s, 16, "%Y%m%d%H%M%S", &t);
    char filename[256];
    sprintf(filename,"%s/search_report%s.log",g_user_path,s);
    int num = 0;
    FILE *fp = fopen(filename, "w");
    if(!fp){
        num += fprintf(stdout,
                       "info string error search_report could not be opened.");
        return num;
    }
    //プログラム＆バージョン
    num += fprintf(fp, "%s(%s)\n", PROGRAM_NAME, VERSION_INFO);
    //設定値
    num += fprintf(fp,
                   "設定値\n"
                   "USI_HASH    : %llu(Mbyte)\n"
                   "search_level: %u\n"
                   "mt_min_pn   : %u\n",
                   g_usi_hash, g_search_level, g_mt_min_pn);
    //解図時間
    clock_t tsec = g_tsearchinf.elapsed/CLOCKS_PER_SEC;
    num += fprintf(fp,
                   "解図時間  　%lu(sec) (%lu:%lu:%lu)\n",
                   tsec,tsec/3600,(tsec%3600)/60,(tsec%3600)%60);
    //探索局面数
    num += fprintf(fp,
                   "探索局面数　%llu\n", g_tsearchinf.nodes);
    //ルートpn
    num += fprintf(fp,
                   "ルートpn　　%u\n", g_root_max);
    //GC回数
    num += fprintf(fp,
                   "GC回数　　 %u(%u)\n", g_gc_num ,g_gc_max_level);
    //初期局面
    num += sdata_fprintf(fp, &g_sdata, PR_BOARD);
    //詰め手順表示
    num += tsume_fprint(fp, &g_sdata, g_tbase, g_smode);
    
    fclose(fp);
    return num;
}

void shtsume_error_log          (const sdata_t *sdata,
                                 mvlist_t      *mvlist,
                                 tbase_t       *tbase)
{
    //filename
    char s[16];
    time_t now;
    time(&now);
    struct tm t = *localtime(&now);
    strftime(s, 16, "%Y%m%d%H%M%S", &t);
    sprintf(g_errorlog_name, "%s/errlog%s.txt",g_user_path,s);
    FILE *fp = fopen(g_errorlog_name, "w");
    if(!fp){
        perror("logfile could not be opened.");
        exit (EXIT_FAILURE);
    }
    //エラー情報記入
    //日時,環境、プログラムバージョン
    fprintf(fp,"%s %s\n",PROGRAM_NAME,VERSION_INFO);
    fprintf(fp,"g_usi_hash = %llu(MByte)\n", g_usi_hash);
    //発生状況
    fprintf(fp, "発生場所　　　 : %s\n", g_error_location);
    fprintf(fp, "コンパイル日時 : %s %s\n", __DATE__, __TIME__);
    //局面情報
    fprintf(fp, "局面情報(sfen): %s\n", g_sfen_pos_str);
    fprintf(fp, "初期局面　　　 :\n");
    sdata_fprintf(fp, &g_sdata, PR_BOARD|PR_ZKEY);
    fprintf(fp, "\n");
    //探索情報
    fprintf(fp, "エラー局面　　 :\n");
    sdata_fprintf(fp, sdata, PR_BOARD|PR_ZKEY);
    mvlist_fprintf_with_item(fp, mvlist,sdata);
    fprintf(fp, "探索局面数　　 : %llu\n\n", g_tsearchinf.nodes);
    
    //gc情報
    fprintf(fp, "GC状況\n");
    fprintf(fp,
            "g_gc_max_level =      %u\n"
            "g_gc_num       =      %u\n\n",
            g_gc_max_level, g_gc_num);
    //探索状況
    fprintf(fp, "探索状況\n");
    char mkey_str[32];
    for(int i=0; i<S_COUNT(sdata); i++){
        SPRINTF_MKEY(mkey_str, g_tsearchinf.mvinf[i].mkey);
        fprintf(fp, "%d: %s 0x%llx %s\n",i,
                g_tsearchinf.mvinf[i].move_str,
                g_tsearchinf.mvinf[i].zkey,
                mkey_str);
    }
    fclose(fp);
    return;
}
