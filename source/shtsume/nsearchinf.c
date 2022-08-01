//
//  nsearchinf.c
//  shtsume
//
//  Created by Hkijin on 2022/02/03.
//

#include <stdio.h>
#include <time.h>

#include "shtsume.h"
#include "ndtools.h"

/* --------------
 グローバル変数
 -------------- */

tsearchinf_t        g_tsearchinf;         /* 詰将棋探索情報            */
clock_t             g_prev_update;
uint64_t            g_prev_nodes;         /* nps計算用に前回nodesを保持 */

/* --------------
 スタティック関数
 -------------- */
static void tsearchpv_update_or (const sdata_t *sdata,
                                 tbase_t       *tbase);
static void tsearchpv_update_and(const sdata_t *sdata,
                                 tbase_t       *tbase);

/* --------------
 実装部
 -------------- */
void tsearchpv_update           (const sdata_t *sdata,
                                 tbase_t       *tbase)
{
    memset(g_tsearchinf.mvinf, 0, sizeof(nsearchlog_t)*TSUME_MAX_DEPTH);
    tsearchpv_update_or(sdata, tbase);
    return;
}

void tsearchinf_update          (const sdata_t *sdata,
                                 tbase_t       *tbase,
                                 clock_t        start,
                                 char          *str  )
{
    g_tsearchinf.elapsed = clock() - start;      //micro sec
    //時間切れの処理
    if(g_time_limit>=0
       && g_tsearchinf.elapsed>g_time_limit*1000)
    {
        g_suspend = true;
        return;
    }
    uint64_t nodes = g_tsearchinf.nodes;
    if(g_tsearchinf.elapsed<CLOCKS_PER_SEC*g_prev_update) return;
    
    //nps
    double
    nps =  (double)(nodes-g_prev_nodes);
    g_tsearchinf.nps = (int)nps;
    
    //hashfull
    double
    hashfull =  (double)tbase->num;
    hashfull /= (double)tbase->sz_elm;
    hashfull *= 1000;
    g_tsearchinf.hashfull = (int)hashfull;
#ifndef DEBUG
    tsearchinf_sprintf(str);
    record_log(str);
    puts(str);
#endif /* DEBUG */
    //pv情報の更新
    g_tsearchinf.depth = S_COUNT(sdata);
    g_tsearchinf.score_cp = g_root_pn;
#ifndef DEBUG
    tsearchpn_sprintf(g_str);
    record_log(g_str);
    puts(g_str);
#endif /* DEBUG */
    
    g_prev_nodes = nodes;
    g_prev_update = 1+g_tsearchinf.elapsed/CLOCKS_PER_SEC;
    return;
}

int  tsearchinf_sprintf         (char *str)
{
    int num = 0;
    num+=sprintf(str+num,"info ");
    num+=sprintf(str+num,"nodes %lld ", g_tsearchinf.nodes);
    num+=sprintf(str+num,"nps %d ", g_tsearchinf.nps);
    num+=sprintf(str+num,"hashfull %d" ,g_tsearchinf.hashfull);
    return num;
}
int  tsearchpn_sprintf          (char *str)
{
    int num = 0;
    num += sprintf(str+num, "info ");
    num += sprintf(str+num, "time %lu ",
                   g_tsearchinf.elapsed*1000/CLOCKS_PER_SEC);
    num += sprintf(str+num, "depth %u ", g_tsearchinf.depth);
    num += sprintf(str+num, "seldepth %u ", g_tsearchinf.sel_depth);
    num += sprintf(str+num, "nodes %llu ", g_tsearchinf.nodes);
    if(g_tsearchinf.score_cp){
        num += sprintf(str+num, "score cp %d ", g_tsearchinf.score_cp);
    }
    else{
        if(g_tsearchinf.score_mate){
            num += sprintf(str+num, "score mate %u ",
                           g_tsearchinf.score_mate);
        }
        else{
            num += sprintf(str+num, "score mate + ");
        }
        
    }
    char mvstr[8];
    num += sprintf(str+num, "pv");
    for(int i=0; i<g_max_pv_length; i++){
        if(g_tsearchinf.mvinf[i].move.prev_pos==0
           && g_tsearchinf.mvinf[i].move.new_pos==0) break;
        move_to_sfen(mvstr, g_tsearchinf.mvinf[i].move);
        num += sprintf(str+num, " %s", mvstr);
    }
    num += sprintf(str+num, "\n");
    return num;
}

/* ------------------
 スタティック関数実装部
 ------------------ */
void tsearchpv_update_or        (const sdata_t *sdata,
                                 tbase_t       *tbase)
{
    //着手生成
    mvlist_t *list = generate_check(sdata, tbase);
    g_tsearchinf.nodes++;
    if(!list){
        //詰将棋解答の場合、想定されない場合だがここでは投了を返すようにする。
        g_tsearchinf.mvinf[S_COUNT(sdata)].move = g_mv_toryo;
        return;
    }
    
    //局面表を参照
    sdata_t  sbuf;
    mvlist_t *tmp = list;
    while(tmp){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_key_forward(&sbuf, tmp->mlist->move);
        make_tree_lookup(&sbuf, tmp, S_TURN(sdata), tbase);
        if(g_redundant){
            if(mtt_lookup(&sbuf, S_TURN(sdata), g_mtt))
                tmp->cu = 1;
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
    //着手の並べ替え
    list = sdata_mvlist_sort(list, sdata, proof_number_comp);
    //着手を記録
    nsearchlog_t *log = &(g_tsearchinf.mvinf[S_COUNT(sdata)]);
    log->move = list->mlist->move;
#if DEBUG
    move_sprintf(log->move_str,log->move,sdata);
#endif //DEBUG
    //エラーチェック
    if(list->cu){
        g_error = true;
        sprintf(g_error_location, "%s line %d", __FILE__, __LINE__);
        return;
    }
    
    //局面を進める
    if(list && list->mlist){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        tsearchpv_update_and(&sbuf, tbase);
    }
    mvlist_free(list);
    return;
}

void tsearchpv_update_and       (const sdata_t *sdata,
                                 tbase_t       *tbase)
{
    turn_t tn = TURN_FLIP(S_TURN(sdata));
    
    //着手生成
    mvlist_t *list = generate_evasion(sdata, tbase);
    g_tsearchinf.nodes++;
    if(!list){
        //玉方の回避が無くなった場合、投了を返すようにする。
        g_tsearchinf.mvinf[S_COUNT(sdata)].move = g_mv_toryo;
        return;
    }
    
    //局面表を参照
    mvlist_t *tmp = list, *tmp1;
    sdata_t sbuf;
    while (tmp) {
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_key_forward(&sbuf, tmp->mlist->move);
        make_tree_lookup(&sbuf, tmp, tn, tbase);
        //これまでの探索で詰みがわかっている合駒がある場合、ここで展開しておく
        if(tmp->mlist->next){
            tmp1 = mvlist_alloc();
            tmp1->mlist = tmp->mlist->next;
            tmp->mlist->next = NULL;
            tmp1->next = tmp->next;
            tmp->next = tmp1;
        }
        tmp = tmp->next;
    }
    
    //着手の並べ替え
    list = sdata_mvlist_sort(list, sdata, disproof_number_comp);
    //GCによるデータ消失対策
    //GCによるデータ消失対策
    tdata_t thdata = {INFINATE-1, INFINATE-1, TSUME_MAX_DEPTH};
    while (list->tdata.pn) {
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        bns_or(&sbuf, &thdata, list, tbase);
        list = sdata_mvlist_sort(list, sdata, disproof_number_comp);
    }
    mcard_t *current = MAKE_TREE_SET_CURRENT(sdata, tn, tbase);
    if(g_redundant)mtt_setup(sdata, tn, g_mtt);
    nsearchlog_t *log = &(g_tsearchinf.mvinf[S_COUNT(sdata)]);
    log->move = list->mlist->move;
#if DEBUG
    move_sprintf(log->move_str,log->move,sdata);
#endif //DEBUG
    //局面を進める
    if(list && list->mlist){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        tsearchpv_update_or(&sbuf, tbase);
    }
    if(current) current->current = 0;
    if(g_redundant)mtt_reset(sdata, tn, g_mtt);
    mvlist_free(list);
    return;
}
