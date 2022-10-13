//
//  shtsume.c
//  shtsume2
//
//  Created by Hkijin on 2022/02/10.
//

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "shtsume.h"
#include "ndtools.h"


/* -------------
 グローバル変数
 ------------- */
bool         g_suspend;     //中断フラグ
bool         g_error;       //エラーフラグ
unsigned int g_root_pn;     //ルート証明数
unsigned int g_root_max;    //ルートpnの最大値
//千日手エラー検出用
//エラーなし　−１、エラー　発生深さ
int          g_loop;
bool         g_redundant;    //駒余りflag true 駒余り

/* --------------
 スタティック変数
 -------------- */
static clock_t st_start;
static tdata_t st_tsumi  = {0, INFINATE, 0};
static tdata_t st_fudumi = {INFINATE, 0, 0};
static unsigned int st_max_depth;    //最大探索深さ
static unsigned int st_max_thpn;     //最大pnしきい値
static unsigned int st_max_thdn;     //最大dnしきい値
static unsigned int st_add_thpn;     //追加探索のルートpnしきい値
static unsigned int st_max_tsh;      //最大詰み深さ（手数）
static unsigned int st_max_dsh;      //最大不詰深さ（手数）

static bool         st_symmetry;     //初形左右対称flag

/* --------------
 スタティック関数
 -------------- */
static void tsumi_proof         (const sdata_t   *sdata,
                                 mvlist_t       *mvlist,
                                 bool             pflag );
static void bn_search_or        (const sdata_t   *sdata,
                                 tdata_t      *th_tdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase );
static void bn_search_and       (const sdata_t   *sdata,
                                 tdata_t      *th_tdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase );
static void make_tree_or        (const sdata_t   *sdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase );
static void make_tree_and       (const sdata_t   *sdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase );
static void bns_plus_or         (const sdata_t   *sdata,
                                 mvlist_t        *mvlist,
                                 unsigned int     ptsh,
                                 tbase_t          *tbase);
static void bns_plus_and        (const sdata_t   *sdata,
                                 mvlist_t        *mvlist,
                                 unsigned int     ptsh,
                                 tbase_t          *tbase);
/* ----------------
 関数実装部
 ---------------- */

//コマンドラインのHELP内容をここに記載する
void print_help                (void)
{
    
    printf
    ("%s, version %s\n"
     " 使用法１: %s \n"
     "         USIに準拠した詰将棋エンジンとして使用する事ができます。\n"
     " 使用法２: コマンドラインより\n"
     "         %s [-hvkgdy][-n pn][-m size][-l lv][-i limit]\n"
     "            [-j dep][-t int] sfen_string\n"
     "　       sfen_stringで指定された局面について詰探索を試みます。\n"
     " [サポートオプション]\n"
     " h,help   : このHELPを表示して終了します。\n"
     " v,version: プログラムのバージョンを表示して終了します。\n"
     " [詰探索オプション]\n"
     " [スイッチ]\n"
     " k,kifu   : レベル毎の棋譜(.kif)を出力します。\n"
     " g,log    : 探索LOGを出力します。           \n"
     " 注）k,gオプションでの出力先はホームディレクトリ（Mac）またはインストール先(Win)\n"
     " d,display: 詰み発見時、探索後に手順確認モードへ移行します。\n"
     " y,yomi   : 探索中、読み筋表示。\n"
     " a,all    : 探索LOGで詰方全ての候補手の探索結果を出力します。\n"
     " [値指定]\n"
     " n,minpn  : 末端探索証明数を指定します。デフォルト値 4 (min 3 max 6)\n"
     " m,memory : 局面表で使用するメモリーサイズを指定します。デフォルト値 256(MByte)。\n"
     "                                               (min 1 max 65535)\n"
     " l,level  : 探索レベルを指定します。デフォルト値 0。   (min 0  max 50) \n"
     " i,limit  : 思考制限時間を指定します（秒単位）。デフォルト値　-1(無制限）\n"
     " j,ydep   : 読み筋深さ。デフォルト値 5             (min 5 max 30)\n"
     " t,yint   : 読み筋表示インターバル。デフォルト値 5(秒) (min 1 max 60)\n\n"
     ,
    PROGRAM_NAME,
    VERSION_INFO,
    PROGRAM_NAME,
    PROGRAM_NAME);
    
    return;
}

uint16_t proof_number     (mvlist_t  *mvlist)
{
    mvlist_t *list = mvlist;
    uint16_t wpn = list->tdata.pn, pn = 0;
    if(wpn == INFINATE && !list->tdata.dn) return INFINATE;
    list = list->next;
    while(list){
        if(!list->tdata.pn) break;
        pn++;
        wpn = MAX(wpn, list->tdata.pn);
        list = list->next;
    }
    return MIN(INFINATE-1, wpn+pn);
}
/*
uint16_t sub_proof_number (mvlist_t  *mvlist)
{
    mvlist_t *list = mvlist;
    uint16_t pn = 0;
    while(list){
        if(!list->tdata.pn) break;
        pn += list->tdata.pn;
        if(pn>SPN_MAX) return SPN_MAX;
        list = list->next;
    }
    return pn;
}
*/
uint16_t disproof_number  (mvlist_t  *mvlist)
{
    mvlist_t *list = mvlist;
    uint16_t wdn = list->tdata.dn, dn = 0;
    if(wdn == INFINATE && !list->tdata.pn) return INFINATE;
    list = list->next;
    while(list){
        if(!list->tdata.dn) break;
        dn++;
        wdn = MAX(wdn, list->tdata.dn);
        list = list->next;
    }
    return MIN(INFINATE-1, wdn+dn);
}
uint16_t proof_count      (mvlist_t  *mvlist)
{
    mvlist_t *list = mvlist->next;
    uint16_t cnt = 0;
    while(list){
        if(!list->tdata.pn) break;
        cnt++;
        list = list->next;
    }
    return cnt;
}
uint16_t disproof_count   (mvlist_t  *mvlist)
{
    mvlist_t *list = mvlist->next;
    uint16_t cnt = 0;
    while(list){
        if(!list->tdata.dn) break;
        cnt++;
        list = list->next;
    }
    return cnt;
}

void bn_search                  (const sdata_t   *sdata,
                                 tdata_t         *tdata,
                                 tbase_t         *tbase)
{
    /* 初期化処理 */
    memset(&g_tsearchinf, 0, sizeof(tsearchinf_t));        //探索情報
    //局面表
    g_gc_max_level = 0;               /* メモリ確保のためのGC強度の最大値  */
    g_gc_num = 0;                     /* gcの実施回数                  */
    //中断フラグ
    g_suspend       = false;           /* g_stop_received。後程廃止    */
    g_stop_received = false;           /* GUIからstopを受信したらTRUE   */
    //時間処理
    g_prev_nodes   = 0;                /* 前回update時の探索局面数       */
    g_prev_update  = g_info_interval;  /* 経過時間(searchinfのupdate用) */
    //追加探索用
    st_max_thpn    = 0;
    st_max_thdn    = 0;
    //統計用情報
    st_max_tsh     = 0;
    st_max_dsh     = 0;
    st_max_depth   = 0;
    //駒余りflag
    g_redundant = false;
    //探索エラー
    g_error        = false;
    g_loop         = -1;
    
    char filename[256];           //棋譜ファイル名
    mvlist_t mvlist;
    memset(&mvlist, 0, sizeof(mvlist_t));
    memcpy(&(mvlist.tdata), &g_tdata_init, sizeof(tdata_t));
    tdata_t th_tdata = {PROOF_MAX-1, INFINATE-1, TSUME_MAX_DEPTH};
    st_symmetry = symmetry_check(sdata);
    /*
     * 詰探索、手順組み立て
     */
    st_start = clock();
    bn_search_or(sdata, &th_tdata, &mvlist, tbase);
    
    if(!mvlist.tdata.pn)
    {
        make_tree(sdata, &mvlist, tbase);
        g_tsearchinf.score_mate = mvlist.tdata.sh;
        st_add_thpn = st_max_thpn;
        char lv = g_search_level;
        char prefix[16];
        memset(prefix, 0, sizeof(prefix));
        if(!g_commandline)
            strncpy(prefix,"info string ", strlen("info string "));
        while(lv){
            if(g_redundant) break;
            st_add_thpn++;
            sprintf(g_str, "%sLV%d %u手詰 "
                           "探索局面数 %llu "
                           "余詰探索 root_pn = %d",
                    prefix,
                    g_search_level-lv,
                    g_tsearchinf.score_mate,
                    g_tsearchinf.nodes,
                    st_add_thpn);
            record_log(g_str);
            puts(g_str);
            if(g_out_lvkif){
                sprintf(filename,"%s/tsumelv%d.kif",
                        g_user_path,g_search_level-lv);
                generate_kif_file(filename, sdata, tbase);
            }
            tbase_clear_protect(tbase);
            bns_plus(sdata, &mvlist, tbase);
            make_tree(sdata, &mvlist, tbase);
            g_tsearchinf.score_mate = mvlist.tdata.sh;
            lv--;
        }
    }
    
    memcpy(tdata, &(mvlist.tdata), sizeof(tdata_t));
    
    /*
     * 詰みであればg_searchinf上に詰手順を構築
     */
    if(!tdata->pn){
        if(g_redundant){
            sprintf(g_str,"info string 駒余り詰め検出。(出力は参考手順）\n");
            record_log(g_str); puts(g_str);
        }
        tsearchpv_update(sdata, tbase);
        if(g_commandline && g_out_lvkif){
            sprintf(filename,"%s/tsumelv%d.kif",
                    g_user_path,g_search_level);
            generate_kif_file(filename, sdata, tbase);
        }
    }
    if(g_error){
        sprintf(g_str, "info string search error occured.");
        record_log(g_str);
        puts(g_str);
    }
    
    return;
}

void bns_or                     (const sdata_t   *sdata,
                                 tdata_t      *th_tdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase )
{
    bn_search_or(sdata, th_tdata, mvlist, tbase);
    return;
}

void make_tree                  (const sdata_t   *sdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase)
{
    int i;
    for(i=0; i<g_n_make_tree; i++){
        if(g_error) break;
        tbase_clear_protect(tbase);
        if(!mvlist->inc)
            make_tree_or(sdata, mvlist, tbase);
        
        else
            g_redundant = true;
         
    }
    return;
}

void bns_plus                   (const sdata_t   *sdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase )
{
    bns_plus_or(sdata, mvlist, mvlist->tdata.sh, tbase);
    return;
}

/* ------------------
 スタティック関数実装部
 -------------------- */
void tsumi_proof                (const sdata_t   *sdata,
                                 mvlist_t       *mvlist,
                                 bool             pflag )
{
    memcpy(&(mvlist->tdata), &st_tsumi, sizeof(tdata_t));
    if(pflag)        mvlist->mkey = ENEMY_MKEY(sdata);  //無駄合い判定あり    
    else             mvlist->mkey = proof_koma(sdata);  //無駄合い判定なし
    if(MKEY_EXIST(ENEMY_MKEY(sdata))){
        mvlist->inc = 1;
        mvlist->nouse  = TOTAL_MKEY(mvlist->mkey);
        mvlist->nouse2 = TOTAL_MKEY(ENEMY_MKEY(sdata));
    }
    else             mvlist->inc = 0;
    if(MKEY_EXIST(mvlist->mkey)) mvlist->hinc = 1;
    else                         mvlist->hinc = 0;
    return;
}
/*
 　デバッグオプション　USE_REORDER
 */

//#define USE_REORDER 1
void bn_search_or               (const sdata_t   *sdata,
                                 tdata_t      *th_tdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase )
{
    tsearchinf_update(sdata, tbase, st_start, g_str);
    turn_t tn = S_TURN(sdata);
    /* --------
     * 着手生成
     -------- */
    mvlist_t *list = generate_check(sdata, tbase);
    g_tsearchinf.nodes++;
    if(!list){
        //mvlistのデータ更新
        memcpy(&(mvlist->tdata), &st_fudumi, sizeof(tdata_t));
        //反証駒：詰方が持っている持ち駒と同種の玉方持ち駒全てを詰方に移動する。
        mvlist->mkey = disproof_koma(sdata);
        //局面表への不詰データ登録、反証駒
        tbase_update(sdata, mvlist, tn, tbase);
        return;
    }
    //初形左右対称の場合、着手を右半分（１−５筋）に限定
    if(!S_COUNT(sdata) && st_symmetry){
        mvlist_t *tmp, *new_list = NULL;
        while(list){
            if(g_file[NEW_POS(list->mlist->move)]<FILE6)
            {
                tmp = list;
                list = list->next;
                tmp->next = new_list;
                new_list = tmp;
            }
            else
            {
                tmp = list;
                list = list->next;
                tmp->next = NULL;
                mvlist_free(tmp);
            }
        }
        list = new_list;
    }
    
    /* ---------------
     * 局面表を参照する。
     --------------- */
    mvlist_t *tmp = list;
    sdata_t sbuf;
    memcpy(&sbuf, sdata, sizeof(sdata_t));
    while(1){
        sdata_key_forward(&sbuf, tmp->mlist->move);
        tbase_lookup(&sbuf, tmp, tn, tbase);
        tmp->length =
        g_distance[ENEMY_OU(sdata)][NEW_POS(tmp->mlist->move)];
        if(!tmp->search &&
           S_BOARD(sdata,S_ATTACK(sdata)[0])!=SUM &&
           S_BOARD(sdata,S_ATTACK(sdata)[0])!=GUM)
            tmp->tdata.pn = MAX(tmp->tdata.pn,tmp->length);
        tmp = tmp->next;
        if(!tmp) break;
        //sbufのリセット
        S_COUNT(&sbuf)--;
        S_ZKEY(&sbuf)=S_ZKEY(sdata);
        S_TURN(&sbuf)=S_TURN(sdata);
        memcpy(&(sbuf.core.mkey),&(sdata->core.mkey),sizeof(mkey_t)*2);
    }
    
    /* --------
     * 反復深化
     -------- */
    tdata_t c_threshold;
    mcard_t *current;
    nsearchlog_t *log = &(g_tsearchinf.mvinf[S_COUNT(sdata)]);
    //着手の並べ替え
    list = sdata_mvlist_sort(list, sdata, proof_number_comp);
    while(true){
        //予想手の出力
        if(S_COUNT(sdata)==0 && !g_commandline){
            g_tsearchinf.mvinf[S_COUNT(sdata)].move = list->mlist->move;
            char mv_str[8];
            move_to_sfen(mv_str, g_tsearchinf.mvinf[S_COUNT(sdata)].move);
            sprintf(g_str, "info currmove %s\n", mv_str);
            record_log(g_str);
            puts(g_str);
        }
        //証明数、反証数等の更新
        mvlist->tdata.pn = list->tdata.pn;
        if(!S_COUNT(sdata)){
            g_root_pn = mvlist->tdata.pn;
            g_root_max = MAX(g_root_max, g_root_pn);
        }
        mvlist->tdata.dn =
        (S_COUNT(sdata))? disproof_number(list): list->tdata.dn;
        mvlist->tdata.sh = list->tdata.sh+1;
        mvlist->inc = list->inc;
        //判定
        if(mvlist->tdata.pn >= th_tdata->pn ||
           mvlist->tdata.dn >= th_tdata->dn ||
           mvlist->tdata.sh >= th_tdata->sh ||
           g_suspend       ||
           g_stop_received )
        {
            
            //詰みの場合
            if     (!mvlist->tdata.pn){
                proof_koma_or(sdata, mvlist, list);
            }
            //不詰の場合
            else if(!mvlist->tdata.dn){
                disproof_koma_or(sdata, mvlist, list);
            }
            /* ----------------------------------------------------------
                最大探索深さの見直し
                これまでの探索深さに折り返し局面での局面表の探索深さを加えた値を試用。
             --------------------------------------------------------- */
            if(st_max_depth < S_COUNT(sdata)+mvlist->tdata.sh){
                st_max_depth = S_COUNT(sdata)+mvlist->tdata.sh;
                g_tsearchinf.sel_depth = st_max_depth;
            }
            tbase_update(sdata, mvlist, S_TURN(sdata), tbase);
            if(!mvlist->tdata.pn)
                st_max_tsh = MAX(st_max_tsh, mvlist->tdata.sh);
            else if(!mvlist->tdata.dn)
                st_max_dsh = MAX(st_max_dsh, mvlist->tdata.sh);
            mvlist_free(list);
            return;
        }
        
        //しきい値の更新
        if(list->next) {
            c_threshold.pn =
            MIN(th_tdata->pn,
                MIN(INFINATE, list->search?(list->next)->tdata.pn+1:1));
            if(th_tdata->dn == INFINATE-1)
                c_threshold.dn = INFINATE-1;
            else
                c_threshold.dn =
                S_COUNT(sdata)?
                th_tdata->dn - disproof_count(list):
                th_tdata->dn;
        }
        else           {
            c_threshold.pn = th_tdata->pn;
            c_threshold.dn = th_tdata->dn;
        }
        c_threshold.sh = th_tdata->sh-1;
        
        
        //探索log
        log->move = list->mlist->move;
        move_sprintf(log->move_str,log->move,sdata);
        log->tdata = mvlist->tdata;
        log->thdata = c_threshold;
        
        //探索局面証明数、反証数にしきい値を代入。
        memcpy(&(mvlist->tdata),&c_threshold, sizeof(tdata_t));
        if(c_threshold.pn <PROOF_MAX-1)
            st_max_thpn = MAX(st_max_thpn, c_threshold.pn);
        list->search = 1;
        
        //王手千日手回避のための設定
        current = tbase_set_current(sdata, mvlist, tn, tbase);
        
        //深化
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        log->zkey = S_ZKEY(&sbuf);
        log->mkey = tn?S_GMKEY(&sbuf):S_SMKEY(&sbuf);
        bn_search_and(&sbuf, &c_threshold, list, tbase);
        memset(&(log->move), 0, sizeof(move_t));
        memset(log->move_str, 0, sizeof(char)*32);
        
        if(current) current->current = 0;
        list = sdata_mvlist_reorder(list, sdata, proof_number_comp);
    }
}

void bn_search_and              (const sdata_t   *sdata,
                                 tdata_t      *th_tdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase )
{
    //中合が発生する可能性を示すフラグ
    bool ryuma_flag = false;
    if(mvlist->length>2) ryuma_flag = true;

    turn_t tn = TURN_FLIP(S_TURN(sdata));
    //着手生成
    mvlist_t *list = generate_evasion(sdata, tbase);
    bool proof_flag = g_invalid_drops;
    g_tsearchinf.nodes++;
    if(!list){
        //無駄合い判定ありの場合の証明駒　false: あり proof_flag: なし
        tsumi_proof(sdata, mvlist, proof_flag);
        tbase_update(sdata, mvlist, tn, tbase);
        return;
    }
    
    /* ---------------
     * 局面表を参照する。
     --------------- */
    mvlist_t *tmp = list, *tmp1;
    sdata_t sbuf;
    memcpy(&sbuf, sdata, sizeof(sdata_t));
    while(1){
        sdata_key_forward(&sbuf, tmp->mlist->move);
        tbase_lookup(&sbuf, tmp, tn, tbase);
        //これまでの探索で詰みがわかっている合駒がある場合、ここで展開しておく
        if(tmp->mlist->next && !tmp->tdata.pn){
            tmp1 = mvlist_alloc();
            tmp1->mlist = tmp->mlist->next;
            tmp->mlist->next = NULL;
            tmp1->next = tmp->next;
            tmp->next = tmp1;
        }
        tmp = tmp->next;
        if(!tmp) break;
        //sbufのリセット
        S_COUNT(&sbuf)--;
        S_ZKEY(&sbuf)=S_ZKEY(sdata);
        S_TURN(&sbuf)=S_TURN(sdata);
        memcpy(&(sbuf.core.mkey),&(sdata->core.mkey),sizeof(mkey_t)*2);
    }
    
    //反復深化
    tdata_t c_threshold;
    mcard_t *current;
    nsearchlog_t *log = &(g_tsearchinf.mvinf[S_COUNT(sdata)]);
    while(true){
        //着手の並べ替え
        list = sdata_mvlist_sort(list, sdata, disproof_number_comp);
        //龍馬による王手の場合、利きの無い中合いは他の手に縮退させる。
        if(ryuma_flag)
        {
            //先頭着手が合駒で未解決の場合
            if(MV_DROP(list->mlist->move))
            {
                if(list->tdata.pn && list->tdata.dn && list->next &&
                   (list->next)->tdata.pn && (list->next)->tdata.dn
                   && !BPOS_TEST(SELF_EFFECT(sdata),NEW_POS(list->mlist->move)))
                {
                    tmp = list;
                    list = list->next;
                    mlist_t *last = mlist_last(list->mlist);
                    last->next = tmp->mlist;
                    //tmpの削除
                    tmp->mlist = NULL;
                    tmp->next = NULL;
                    mvlist_free(tmp);
                }
            }
            //その他の場合
            else
            {

                mvlist_t *prev = list;
                tmp = list->next;
                while(tmp){
                    if(!tmp->tdata.pn) break;
                    if(MV_DROP(tmp->mlist->move)
                       && !BPOS_TEST(SELF_EFFECT(sdata),
                                     NEW_POS(tmp->mlist->move)))
                    {
                        //tmpを切り離す
                        tmp1 = tmp;
                        tmp = tmp->next;
                        prev->next = tmp;
                        //listに合駒着手追加
                        mlist_t *last = mlist_last(list->mlist);
                        last->next = tmp1->mlist;
                        //tmp1を削除
                        tmp1->next = NULL;
                        tmp1->mlist = NULL;
                        mvlist_free(tmp1);
                        break;
                    }
                    prev = tmp;
                    tmp = tmp->next;
                }
            }
        }
        
        //証明数、反証数等の更新
        mvlist->tdata.dn = list->tdata.dn;
        mvlist->tdata.pn = proof_number(list);
        //mvlist->spn = sub_proof_number(list);
        mvlist->tdata.sh = list->tdata.sh+1;
        
        //判定
        if(mvlist->tdata.pn >= th_tdata->pn ||
           mvlist->tdata.dn >= th_tdata->dn ||
           mvlist->tdata.sh >= th_tdata->sh ||
           g_suspend       ||
           g_stop_received   )
        {
            
            //不詰の場合
            if      (!mvlist->tdata.dn)
            {
                disproof_koma_and(sdata, mvlist, list);
            }
            //詰みの場合
            else if(!mvlist->tdata.pn)
            {
                if(proof_flag)      {
                    mvlist->mkey = ENEMY_MKEY(sdata);
                    mvlist->inc  = list->inc;
                    mvlist->hinc = list->inc;
                    mvlist->nouse = list->nouse +
                    TOTAL_MKEY(ENEMY_MKEY(sdata)) - TOTAL_MKEY(list->mkey);
                    mvlist->nouse2 = mvlist->nouse;
                }
                else                {
                    proof_koma_and(sdata, mvlist, list);
                }
            }
            /* ----------------------------------------------------------
                最大探索深さの見直し
                これまでの探索深さに折り返し局面での局面表の探索深さを加えた値を試用。
             --------------------------------------------------------- */
            if(st_max_depth < S_COUNT(sdata)+mvlist->tdata.sh){
                st_max_depth = S_COUNT(sdata)+mvlist->tdata.sh;
                g_tsearchinf.sel_depth = st_max_depth;
            }
            tbase_update(sdata, mvlist, tn, tbase);
            if(!mvlist->tdata.dn)
                st_max_dsh = MAX(st_max_dsh, mvlist->tdata.sh);
            else if(!mvlist->tdata.pn)
                st_max_tsh = MAX(st_max_tsh, mvlist->tdata.sh);
            mvlist_free(list);
            return;
        }
        
        //しきい値の更新
        if(list->next) {
            c_threshold.dn = MIN(th_tdata->dn, (list->next)->tdata.dn+1);
            c_threshold.pn = th_tdata->pn - proof_count(list);
            c_threshold.sh = th_tdata->sh-1;
        }
        else           {
            c_threshold.pn = th_tdata->pn;
            c_threshold.dn = th_tdata->dn;
            c_threshold.sh = th_tdata->sh-1;
        }
        
        //探索log(1)
        log->move = list->mlist->move;
        move_sprintf(log->move_str,log->move,sdata);
        log->tdata = mvlist->tdata;
        log->thdata = c_threshold;
        
        //千日手回避のため、しきい値を現局面に代入。
        memcpy(&(mvlist->tdata), &c_threshold,sizeof(tdata_t));
        if(c_threshold.dn <INFINATE-1)
            st_max_thdn = MAX(st_max_thdn, c_threshold.dn);
        
        //王手千日手回避のための設定
        current = tbase_set_current(sdata, mvlist, tn, tbase);
        
        //深化
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        
        //探索log(2)
        log->zkey = S_ZKEY(&sbuf);
        log->mkey = tn?S_GMKEY(&sbuf):S_SMKEY(&sbuf);
        
        bn_search_or(&sbuf, &c_threshold, list, tbase);
        
        memset(&(log->move), 0, sizeof(move_t));
        memset(log->move_str, 0, sizeof(char)*32);
        
        if(current) current->current = 0;
        
        //合駒着手が詰みの場合、次の詰んでいない合い駒まで展開しておく。
        while(list->mlist->next && !list->tdata.pn){
            tmp = mvlist_alloc();
            tmp->mlist = list->mlist->next;
            list->mlist->next = NULL;
            tmp->next = list;
            list = tmp;
            memcpy(&sbuf, sdata, sizeof(sdata_t));
            sdata_key_forward(&sbuf, list->mlist->move);
            tbase_lookup(&sbuf, list, tn, tbase);
        }
    }
}

void make_tree_or               (const sdata_t   *sdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase )
{
    tsearchinf_update(sdata, tbase, st_start, g_str);
    
    //千日手模様のエラー検出のため
    if (S_COUNT(sdata)>=TSUME_MAX_DEPTH-1) {
        return;
    }
    tdata_t thdata =
    {g_gc_max_level+MAKE_TREE_PN_PLUS, INFINATE-1, TSUME_MAX_DEPTH };
    if(g_suspend || g_stop_received ) return;
    
    //着手生成
    mvlist_t *list = generate_check(sdata, tbase);
    g_tsearchinf.nodes++;
    if(!list){
        //関数の性質上ここに来ることはあり得ない。
        g_error = true;
        sprintf(g_error_location, "%s line %d", __FILE__, __LINE__);
        return;
    }
    
    //局面表を参照する
    mvlist_t *tmp = list;
    sdata_t sbuf;
    while(tmp){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, tmp->mlist->move);
        
        //ヒットした詰データはプロテクト状況も踏まえ、記録する。
        make_tree_lookup(&sbuf, tmp, S_TURN(sdata), tbase);
        
        //収束余詰探索用
        if(!tmp->search||(tmp->tdata.pn && tmp->tdata.pn<g_mt_min_pn))
        {
            thdata.pn = MAX(g_gc_max_level+MAKE_TREE_PN_PLUS, g_mt_min_pn);
            bn_search_and(&sbuf, &thdata, tmp, tbase);
        }
        
        tmp->length =
        g_distance[ENEMY_OU(sdata)][NEW_POS(tmp->mlist->move)];
        if(tmp->length    > 1 &&
           tmp->tdata.pn == 1 &&
           tmp->tdata.dn == 1 &&
           tmp->tdata.sh == 0 &&
           MV_DROP(tmp->mlist->move)) tmp->tdata.pn = tmp->length;
        
        tmp = tmp->next;
    }
    
    //並べ替え
    list = sdata_mvlist_sort(list, sdata, proof_number_comp);
    /* --------------------------------------------------------------------
     GCで詰みデータを削除した場合、再探索を実施する。
     ------------------------------------------------------------------- */
    while(list->tdata.pn){
        if(list->tdata.pn >= INFINATE-1){
            SDATA_PRINTF(sdata,PR_BOARD);
            printf("info string error occured at %s line %d",
                   __FILE__,__LINE__);
            exit(EXIT_FAILURE);
        }
        if(list->next) thdata.pn = MIN(INFINATE-1,(list->next)->tdata.pn+1);
        else thdata.pn = INFINATE-1;
        thdata.sh = TSUME_MAX_DEPTH-sdata->core.count-1;
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        bn_search_and(&sbuf, &thdata, list, tbase);
        list = sdata_mvlist_sort(list, sdata, proof_number_comp);
    }
    /* --------------------------------------------------------------------
     GCで詰手順が再構築された場合、親局面での詰め手数より子局面での詰手数が長い場合がある。
     これは追加探索を実施しないと検出できない詰手数をもつデータが消去されていた場合に
     発生する。その場合は復旧のため親局面が示す詰手数となるまで追加探索を実施する。
     ------------------------------------------------------------------- */
    if(mvlist->tdata.sh < list->tdata.sh && !list->inc)
    {
        //詰んでいる着手についてさらに短い詰着手がないか調べる
        tmp = list;
        uint16_t ptsh = mvlist->tdata.sh;
        while(!tmp->tdata.pn){
            memcpy(&sbuf, sdata, sizeof(sdata_t));
            sdata_move_forward(&sbuf, tmp->mlist->move);
            bns_plus_and(&sbuf, tmp, ptsh, tbase);
            tmp = tmp->next;
        }
        list = sdata_mvlist_sort(list, sdata, proof_number_comp);
        //もし全て調べて短い詰みが見つからない場合、エラーログを出力して終了する。
        if(mvlist->tdata.sh < list->tdata.sh && !list->inc)
        {
            g_error = true;
            sprintf(g_error_location, "%s line %d", __FILE__, __LINE__);
            shtsume_error_log(sdata, list, g_tbase);
            if(!g_commandline)
                sprintf(g_str,
                    "info string search error occured. %s",
                    g_errorlog_name);
            else
                sprintf(g_str,
                        "search error occured. %s",
                        g_errorlog_name);
            puts(g_str);
            record_log(g_str);
            exit(EXIT_FAILURE);
        }
    }
    if(list->cu) return;              //通常バグがなければここに来ることはあり得ない
    
    nsearchlog_t *log = &(g_tsearchinf.mvinf[S_COUNT(sdata)]);
    turn_t tn = S_TURN(sdata);

    while(!list->inc && !list->pr){
        //探索log(1)
        log->move = list->mlist->move;
        move_sprintf(log->move_str,log->move,sdata);
        
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        
        //探索log(2)
        log->zkey = S_ZKEY(&sbuf);
        log->mkey = tn?S_GMKEY(&sbuf):S_SMKEY(&sbuf);
        
        make_tree_and(&sbuf, list, tbase);
        //探索log(3)
        memset(&(log->move), 0, sizeof(move_t));
        memset(log->move_str, 0, sizeof(char)*32);
        
        list->pr = 1;
        list = sdata_mvlist_sort(list, sdata, proof_number_comp);
    }
     
    mvlist->tdata.pn = list->tdata.pn;
    mvlist->tdata.dn = list->tdata.dn;
    mvlist->tdata.sh = list->tdata.sh+1;
    
    if     (!mvlist->tdata.pn) proof_koma_or   (sdata, mvlist, list);
    else if(!mvlist->tdata.dn) disproof_koma_or(sdata, mvlist, list);
    make_tree_update(sdata, mvlist, tn, tbase);
    mvlist_free(list);
    return;
}

void make_tree_and              (const sdata_t   *sdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase )
{
    tdata_t thdata = {INFINATE-1, INFINATE-1, 2000};
    if(g_suspend || g_stop_received ) return;
    turn_t tn = TURN_FLIP(S_TURN(sdata));
    
    //着手生成
    mvlist_t *list = generate_evasion(sdata, tbase);
    bool proof_flag = g_invalid_drops;
    g_tsearchinf.nodes++;
    
    // 無駄合い判定の有無に関わらず証明駒
    if(!list){
        //無駄合い判定ありの場合の証明駒　false: あり proof_flag: なし
        tsumi_proof(sdata, mvlist, proof_flag);
        make_tree_update(sdata, mvlist, tn, tbase);
        return;
    }
    
    //局面表を参照する
    mvlist_t *tmp = list, *tmp1;
    sdata_t sbuf;
    while(tmp){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_key_forward(&sbuf, tmp->mlist->move);
        make_tree_lookup(&sbuf, tmp, tn, tbase);
        //合い駒着手は全展開しておく
        if(tmp->mlist->next){
            tmp1 = mvlist_alloc();
            tmp1->mlist = tmp->mlist->next;
            tmp->mlist->next = NULL;
            tmp1->next = tmp->next;
            tmp->next = tmp1;
        }
        tmp = tmp->next;
    }
    list = sdata_mvlist_sort(list, sdata, disproof_number_comp);
    
    //詰みデータ消失の場合
    while(list->tdata.pn){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        bn_search_or(&sbuf, &thdata, list, tbase);
        if(!list->tdata.dn){
            //不詰の場合の処理
            printf("エラー\n");
            SDATA_PRINTF(sdata, PR_BOARD|PR_ZKEY);
        }
        list = sdata_mvlist_sort(list, sdata, disproof_number_comp);
    }
    
    mcard_t *current = MAKE_TREE_SET_CURRENT(sdata, tn, tbase);
    tmp = list;
    nsearchlog_t *log = &(g_tsearchinf.mvinf[S_COUNT(sdata)]);
    while(tmp){
        if(tmp->inc||tmp->cu || tmp->pr);
        else{
            //探索log(1)
            log->move = tmp->mlist->move;
            move_sprintf(log->move_str,log->move,sdata);
            
            memcpy(&sbuf, sdata, sizeof(sdata_t));
            sdata_move_forward(&sbuf, tmp->mlist->move);
            
            //探索log(2)
            log->zkey = S_ZKEY(&sbuf);
            log->mkey = tn?S_GMKEY(&sbuf):S_SMKEY(&sbuf);
            
            make_tree_or(&sbuf, tmp, tbase);
            
            //探索log(3)
            memset(&(log->move), 0, sizeof(move_t));
            memset(log->move_str, 0, sizeof(char)*32);
            
            tmp->pr = 1;
        }
        tmp = tmp->next;
    }
    list = sdata_mvlist_sort(list, sdata, disproof_number_comp);

    if(current) current->current = 0;
    
    mvlist->tdata.pn = list->tdata.pn;
    mvlist->tdata.dn = list->tdata.dn;
    mvlist->tdata.sh = list->tdata.sh+1;
    
    if     (!mvlist->tdata.dn) disproof_koma_and(sdata, mvlist, list);
    else if(!mvlist->tdata.pn){
        if(proof_flag){             //無駄合い判定あり
            mvlist->mkey   = ENEMY_MKEY(sdata);
            mvlist->inc    = list->inc;
            mvlist->hinc   = list->inc;
            mvlist->nouse  = list->nouse +
            TOTAL_MKEY(ENEMY_MKEY(sdata)) - TOTAL_MKEY(list->mkey);
            mvlist->nouse2 = mvlist->nouse;
        }
        else
            proof_koma_and(sdata, mvlist, list);
    }
    make_tree_update(sdata, mvlist, tn, tbase);
    mvlist_free(list);
    return;
}

void bns_plus_or                (const sdata_t   *sdata,
                                 mvlist_t        *mvlist,
                                 unsigned int     ptsh,
                                 tbase_t          *tbase)
{
//#ifndef DEBUG
    tsearchinf_update(sdata, tbase, st_start, g_str);
//#endif /* DEBUG */
    if(g_suspend) return;
    //初期化
    tdata_t thdata = {1, INFINATE-1, TSUME_MAX_DEPTH};
    
    //着手生成
    mvlist_t *list = generate_check(sdata, tbase);
    g_tsearchinf.nodes++;
    if(!list){
        //不詰時の処理を書く
        printf("予期しない不詰です。%s %d\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    
    //局面表
    mvlist_t *tmp = list;
    sdata_t sbuf;
    while (tmp) {
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, tmp->mlist->move);
        make_tree_lookup(&sbuf, tmp, S_TURN(sdata), tbase);
        /* -----------------------------------------------
          GCを実施している場合、末端データの喪失が想定される。
          それらのデータは復元しておく
         ------------------------------------------------*/
        //データ消失時の処理
        if(!tmp->search)
        {
            thdata.sh = MAX(3,g_gc_max_level+1);
            bn_search_and(&sbuf, &thdata, tmp, tbase);
        }
        
        tmp->length =
        g_distance[ENEMY_OU(sdata)][NEW_POS(tmp->mlist->move)];
        
        if(tmp->length > 1    &&
           tmp->tdata.pn == 1 &&
           tmp->tdata.dn == 1 &&
           tmp->tdata.sh == 0 &&
           MV_DROP(tmp->mlist->move)) tmp->tdata.pn = tmp->length;
        tmp = tmp->next;
    }
    
    //並べ替え
    list = sdata_mvlist_sort(list, sdata, proof_number_comp);
    
    //もし駒余り詰みまたは詰め上がりであれば先頭リストの内容を反映させて終了
    if (!list->tdata.pn &&
        (list->inc ||!list->tdata.sh))
    {
        mvlist->tdata.pn = list->tdata.pn;
        mvlist->tdata.dn =
        S_COUNT(sdata) ? disproof_number(list): list->tdata.dn;
        mvlist->tdata.sh = list->tdata.sh+1;
        proof_koma_or(sdata, mvlist, list);
        make_tree_update(sdata, mvlist, S_TURN(sdata), tbase);
        mvlist_free(list);
        return;
    }
    
    //探索深さしきい値を設定
    unsigned int tsh = ptsh-1;
    
    //追加探索
    tmp = list;
    nsearchlog_t *log = &(g_tsearchinf.mvinf[S_COUNT(sdata)]);
    turn_t tn = S_TURN(sdata);
    while(tmp){
        //詰んでいる局面(ループ局面は避けておく）
        if (!tmp->tdata.pn && !tmp->cu)
        {
            tsh = MIN(tsh, tmp->tdata.sh);
            //別コースから合流した局面の場合もあるのでその場合も２重探索を避ける
            if(!tmp->pr) {
                //王手千日手回避のための設定
                mcard_t *current = tbase_set_current(sdata, mvlist, tn, tbase);
                //探索log(1)
                log->move = list->mlist->move;
                move_sprintf(log->move_str,log->move,sdata);
                
                memcpy(&sbuf, sdata, sizeof(sdata_t));
                sdata_move_forward(&sbuf, tmp->mlist->move);
                
                //探索log(2)
                log->zkey = S_ZKEY(&sbuf);
                log->mkey = tn?S_GMKEY(&sbuf):S_SMKEY(&sbuf);

                bns_plus_and(&sbuf, tmp, tsh, tbase);
                
                //探索log(3)
                memset(&(log->move), 0, sizeof(move_t));
                memset(log->move_str, 0, sizeof(char)*32);
                
                if(current) current->current = 0;
                tmp->pr = 1;
            }
        }
        
        //未解決局面
        if (    tmp->tdata.pn
            &&  tmp->tdata.pn < INFINATE-1
            && !tmp->cu
            && !tmp->pr
            && mvlist->tdata.sh >1)
        {
            thdata.pn =
            MIN(st_add_thpn,MAX(tmp->tdata.pn,g_gc_max_level))+1;
            thdata.sh = MIN(TSUME_MAX_DEPTH - S_COUNT(sdata)-1,
                            mvlist->tdata.sh+ADD_SEARCH_SH);
            memcpy(&sbuf, sdata, sizeof(sdata_t));
            sdata_move_forward(&sbuf, tmp->mlist->move);
            bn_search_and(&sbuf, &thdata, tmp, tbase);
            tmp->pr = 1;
        }
        
        tmp = tmp->next;
    }
    
    //再度並べ替え
    list = sdata_mvlist_sort(list, sdata, proof_number_comp);
    //局面表の更新
    if( mvlist->tdata.sh>list->tdata.sh || list->inc)
    {
        mvlist->tdata.pn = list->tdata.pn;
        mvlist->tdata.dn =
        S_COUNT(sdata) ? disproof_number(list): list->tdata.dn;
        mvlist->tdata.sh = list->tdata.sh+1;
        mvlist->inc = list->inc;
        if (!mvlist->tdata.pn) {
            proof_koma_or(sdata, mvlist, list);
        }
        else if (!mvlist->tdata.dn){
            disproof_koma_or(sdata, mvlist, list);
        }
        make_tree_update(sdata, mvlist, tn, tbase);
    }
    mvlist_free(list);
    return;
    
}

void bns_plus_and               (const sdata_t   *sdata,
                                 mvlist_t        *mvlist,
                                 unsigned int     ptsh,
                                 tbase_t          *tbase)
{
    if(g_suspend) return;
    //初期化処理
    turn_t tn = TURN_FLIP(S_TURN(sdata));
    tdata_t thdata = {INFINATE-1, INFINATE-1, TSUME_MAX_DEPTH};
    
    //着手生成
    mvlist_t *list = generate_evasion(sdata, tbase);
    bool proof_flag = g_invalid_drops;
    g_tsearchinf.nodes++;
    if(!list){
        //無駄合い判定ありの場合の証明駒　false: あり proof_flag: なし
        tsumi_proof(sdata, mvlist, proof_flag);
        tbase_update(sdata, mvlist, tn, tbase);
        return;
    }
    //深さしきい値が0で詰んでいない場合、この変化は目的外なので探索を中断する。
    if(!ptsh){
        make_tree_update(sdata, mvlist, tn, tbase);
        mvlist_free(list);
        return;
    }
    
    //局面表を参照する
    mvlist_t *tmp = list, *tmp1;
    sdata_t sbuf;
    while(tmp){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_key_forward(&sbuf, tmp->mlist->move);
        tbase_lookup(&sbuf, tmp, tn, tbase);
        //これまでの探索で詰みがわかっている合駒がある場合、ここで展開しておく
        if(tmp->mlist->next && !tmp->tdata.pn){
            tmp1 = mvlist_alloc();
            tmp1->mlist = tmp->mlist->next;
            tmp->mlist->next = NULL;
            tmp1->next = tmp->next;
            tmp->next = tmp1;
        }
        tmp = tmp->next;
    }
    
    //並べ替え
    list = sdata_mvlist_sort(list, sdata, disproof_number_comp);
    
    //詰みデータが消失していた場合の処理
    while(list->tdata.pn){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        bn_search_or(&sbuf, &thdata, list, tbase);
        list = sdata_mvlist_sort(list, sdata, disproof_number_comp);
    }
    
    //追加探索
    unsigned int tsh = ptsh-1;
    mcard_t *current = MAKE_TREE_SET_CURRENT(sdata, tn, tbase);
    nsearchlog_t *log = &(g_tsearchinf.mvinf[S_COUNT(sdata)]);
    while (true) {
        if (list->inc || list->pr) {
            break;
        }
        tsh = MIN(tsh, list->tdata.sh);

        //探索log(1)
        log->move = list->mlist->move;
        move_sprintf(log->move_str,log->move,sdata);
        
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);

        //探索log(2)
        log->zkey = S_ZKEY(&sbuf);
        log->mkey = tn?S_GMKEY(&sbuf):S_SMKEY(&sbuf);
        
        bns_plus_or(&sbuf, list, tsh, tbase);

        //探索log(3)
        memset(&(log->move), 0, sizeof(move_t));
        memset(log->move_str, 0, sizeof(char)*32);
        
        list->pr = 1;
        list = sdata_mvlist_sort(list, sdata, disproof_number_comp);
    }
    if (current) {
        current->current = 0;
    }
    
    mvlist->tdata.pn = proof_number(list);
    mvlist->tdata.dn = list->tdata.dn;
    mvlist->tdata.sh = list->tdata.sh+1;
    if     (!mvlist->tdata.dn){
        disproof_koma_and(sdata, mvlist, list);
    }
    else if(!mvlist->tdata.pn){
        if(proof_flag){                   //無駄合い判定あり
            mvlist->mkey = ENEMY_MKEY(sdata);
            mvlist->inc = list->inc;
            mvlist->hinc = list->inc;
            mvlist->nouse = list->nouse +
            TOTAL_MKEY(ENEMY_MKEY(sdata)) - TOTAL_MKEY(list->mkey);
        }
        else               {
            proof_koma_and(sdata, mvlist, list);
        }
    }
    
    make_tree_update(sdata, mvlist, tn, tbase);
    mvlist_free(list);
    return;
}

/*
void make_plus_or               (const sdata_t   *sdata,
                                 mvlist_t        *mvlist,
                                 unsigned int     ptpn,
                                 unsigned int     ptsh,
                                 tbase_t          *tbase)
{
#ifndef DEBUG
    tsearchinf_update(sdata, tbase, st_start, g_str);
#endif // DEBUG
    if(g_suspend) return;
    //初期化
    tdata_t thdata = {ptpn, INFINATE-1, TSUME_MAX_DEPTH};
    
    //着手生成
    mvlist_t *list = generate_check(sdata, tbase);
    g_tsearchinf.nodes++;
    if(!list){
        //不詰時の処理を書く
        printf("予期しない不詰です。%s %d\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    
    //局面表
    mvlist_t *tmp = list;
    sdata_t sbuf;
    while (tmp) {
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, tmp->mlist->move);
        make_tree_lookup(&sbuf, tmp, S_TURN(sdata), tbase);
        // -----------------------------------------------
        //  GCを実施している場合、末端データの喪失が想定される。
        //  それらのデータは復元しておく
        //------------------------------------------------
        //データ消失時の処理
        if(!tmp->search)
        {
            thdata.sh = MAX(3,g_gc_max_level+1);
            bn_search_and(&sbuf, &thdata, tmp, tbase);
        }
        
        tmp->length =
        g_distance[ENEMY_OU(sdata)][NEW_POS(tmp->mlist->move)];
        
        if(tmp->length > 1    &&
           tmp->tdata.pn == 1 &&
           tmp->tdata.dn == 1 &&
           tmp->tdata.sh == 0 &&
           MV_DROP(tmp->mlist->move)) tmp->tdata.pn = tmp->length;
        tmp = tmp->next;
    }
    
    //並べ替え
    list = sdata_mvlist_sort(list, sdata, proof_number_comp);
    
    //もし駒余り詰みまたは詰め上がりであれば先頭リストの内容を反映させて終了
    if (!list->tdata.pn && (list->inc ||!list->tdata.sh))
    {
        mvlist->tdata.pn = list->tdata.pn;
        mvlist->tdata.dn =
        S_COUNT(sdata) ? disproof_number(list): list->tdata.dn;
        mvlist->tdata.sh = list->tdata.sh+1;
        proof_koma_or(sdata, mvlist, list);
        make_tree_update(sdata, mvlist, S_TURN(sdata), tbase);
        mvlist_free(list);
        return;
    }
    
    //探索深さしきい値を設定
    unsigned int tsh = ptsh-1;
    
    //追加探索
    tmp = list;
    nsearchlog_t *log = &(g_tsearchinf.mvinf[S_COUNT(sdata)]);
    turn_t tn = S_TURN(sdata);
    while(tmp){
        //詰んでいる局面(ループ局面は避けておく）
        if (!tmp->tdata.pn && !tmp->cu)
        {
            tsh = MIN(tsh, tmp->tdata.sh);
            //別コースから合流した局面の場合もあるのでその場合も２重探索を避ける
            if(!tmp->pr) {
                //王手千日手回避のための設定
                mcard_t *current = tbase_set_current(sdata, mvlist, tn, tbase);
                //探索log(1)
                log->move = list->mlist->move;
                move_sprintf(log->move_str,log->move,sdata);
                
                memcpy(&sbuf, sdata, sizeof(sdata_t));
                sdata_move_forward(&sbuf, tmp->mlist->move);
                
                //探索log(2)
                log->zkey = S_ZKEY(&sbuf);
                log->mkey = tn?S_GMKEY(&sbuf):S_SMKEY(&sbuf);

                make_plus_and(&sbuf, tmp, ptpn, tsh, tbase);
                
                //探索log(3)
                memset(&(log->move), 0, sizeof(move_t));
                memset(log->move_str, 0, sizeof(char)*32);
                
                if(current) current->current = 0;
                tmp->pr = 1;
            }
        }
        
        //未解決局面
        if (    tmp->tdata.pn
            &&  tmp->tdata.pn < INFINATE-1
            && !tmp->cu
            && !tmp->pr
            && mvlist->tdata.sh >1)
        {
            thdata.pn =
            MIN(ptpn,MAX(tmp->tdata.pn,g_gc_max_level))+1;
            thdata.sh = MIN(TSUME_MAX_DEPTH - S_COUNT(sdata)-1,
                            mvlist->tdata.sh+ADD_SEARCH_SH);
            memcpy(&sbuf, sdata, sizeof(sdata_t));
            sdata_move_forward(&sbuf, tmp->mlist->move);
            bn_search_and(&sbuf, &thdata, tmp, tbase);
            tmp->pr = 1;
        }
        
        tmp = tmp->next;
    }
    
    //再度並べ替え
    list = sdata_mvlist_sort(list, sdata, proof_number_comp);
    //局面表の更新
    mvlist->tdata.pn = list->tdata.pn;
    mvlist->tdata.dn =
    S_COUNT(sdata) ? disproof_number(list): list->tdata.dn;
    mvlist->tdata.sh = list->tdata.sh+1;
    mvlist->inc = list->inc;
    if (!mvlist->tdata.pn) {
        proof_koma_or(sdata, mvlist, list);
    }
    else if (!mvlist->tdata.dn){
        disproof_koma_or(sdata, mvlist, list);
    }
    make_tree_update(sdata, mvlist, tn, tbase);
    mvlist_free(list);
    return;
}

void make_plus_and              (const sdata_t   *sdata,
                                 mvlist_t        *mvlist,
                                 unsigned int     ptpn,
                                 unsigned int     ptsh,
                                 tbase_t          *tbase)
{
    if(g_suspend) return;
    //初期化処理
    turn_t tn = TURN_FLIP(S_TURN(sdata));
    tdata_t thdata = {INFINATE-1, INFINATE-1, TSUME_MAX_DEPTH};
    
    //着手生成
    mvlist_t *list = generate_evasion(sdata, tbase);
    bool proof_flag = g_invalid_drops;
    g_tsearchinf.nodes++;
    if(!list){
        //無駄合い判定ありの場合の証明駒　false: あり proof_flag: なし
        tsumi_proof(sdata, mvlist, proof_flag);
        tbase_update(sdata, mvlist, tn, tbase);
        return;
    }
    //深さしきい値が0で詰んでいない場合、この変化は目的外なので探索を中断する。
    if(!ptsh){
        make_tree_update(sdata, mvlist, tn, tbase);
        return;
    }
    
    //局面表を参照する
    mvlist_t *tmp = list, *tmp1;
    sdata_t sbuf;
    while(tmp){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_key_forward(&sbuf, tmp->mlist->move);
        tbase_lookup(&sbuf, tmp, tn, tbase);
        //これまでの探索で詰みがわかっている合駒がある場合、ここで展開しておく
        if(tmp->mlist->next && !tmp->tdata.pn){
            tmp1 = mvlist_alloc();
            tmp1->mlist = tmp->mlist->next;
            tmp->mlist->next = NULL;
            tmp1->next = tmp->next;
            tmp->next = tmp1;
        }
        tmp = tmp->next;
    }
    
    //並べ替え
    list = sdata_mvlist_sort(list, sdata, disproof_number_comp);
    
    //詰みデータが消失していた場合の処理
    while(list->tdata.pn){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        bn_search_or(&sbuf, &thdata, list, tbase);
        list = sdata_mvlist_sort(list, sdata, disproof_number_comp);
    }
    
    //追加探索
    unsigned int tsh = ptsh-1;
    mcard_t *current = MAKE_TREE_SET_CURRENT(sdata, tn, tbase);
    nsearchlog_t *log = &(g_tsearchinf.mvinf[S_COUNT(sdata)]);
    while (true) {
        if (list->inc || list->pr) {
            break;
        }
        tsh = MIN(tsh, list->tdata.sh);

        //探索log(1)
        log->move = list->mlist->move;
        move_sprintf(log->move_str,log->move,sdata);
        
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);

        //探索log(2)
        log->zkey = S_ZKEY(&sbuf);
        log->mkey = tn?S_GMKEY(&sbuf):S_SMKEY(&sbuf);
        
        make_plus_or(&sbuf, list, ptpn, tsh, tbase);

        //探索log(3)
        memset(&(log->move), 0, sizeof(move_t));
        memset(log->move_str, 0, sizeof(char)*32);
        
        list->pr = 1;
        list = sdata_mvlist_sort(list, sdata, disproof_number_comp);
    }
    if (current) {
        current->current = 0;
    }
    
    mvlist->tdata.pn = proof_number(list);
    mvlist->tdata.dn = list->tdata.dn;
    mvlist->tdata.sh = list->tdata.sh+1;
    if     (!mvlist->tdata.dn){
        disproof_koma_and(sdata, mvlist, list);
    }
    else if(!mvlist->tdata.pn){
        if(proof_flag){                   //無駄合い判定あり
            mvlist->mkey = ENEMY_MKEY(sdata);
            mvlist->inc = list->inc;
            mvlist->hinc = list->inc;
            mvlist->nouse = list->nouse +
            TOTAL_MKEY(ENEMY_MKEY(sdata)) - TOTAL_MKEY(list->mkey);
        }
        else               {
            proof_koma_and(sdata, mvlist, list);
        }
    }
    
    make_tree_update(sdata, mvlist, tn, tbase);
    mvlist_free(list);
    return;
}
*/
