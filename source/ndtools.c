//
//  ndtools.c
//  shtsume
//
//  Created by Hkijin on 2022/02/03.
//

#include <stdlib.h>
#include <errno.h>
#include "ndtools.h"

/* ---------------
   スタティック変数
 --------------- */
static bool st_disp_flag;            /* tsume_debugの終了flag */

/* ---------------
   スタティック関数
 --------------- */
static void _tsume_print_or     (const sdata_t  *sdata,
                                 tbase_t        *tbase,
                                 unsigned int    flag );
static void _tsume_print_and    (const sdata_t  *sdata,
                                 tbase_t        *tbase,
                                 unsigned int    flag );
static int _tsume_fprint_or     (FILE           *stream,
                                 const sdata_t  *sdata,
                                 tbase_t        *tbase,
                                 unsigned int    flag );
static int _tsume_fprint_and    (FILE           *stream,
                                 const sdata_t  *sdata,
                                 tbase_t        *tbase,
                                 unsigned int    flag );
static void tsume_debug_or      (const sdata_t   *sdata,
                                 tbase_t        *tbase);
static void tsume_debug_and     (const sdata_t   *sdata,
                                 tbase_t        *tbase);
static void gen_kif_file_or     (FILE *restrict  stream,
                                 const sdata_t   *sdata,
                                 tbase_t         *tbase,
                                 move_t           prev );
static void gen_kif_file_and    (FILE *restrict  stream,
                                 const sdata_t   *sdata,
                                 tbase_t         *tbase,
                                 move_t           prev );

/*
 * 探索情報の表示
 * 探索深さ、着手、証明数、反証数、
 */
int   search_inf_sprintf (char *restrict str,
                          const mvlist_t *mvlist,
                          const sdata_t *sdata )
{
    int num = 0;
    num += sprintf(str+num, " %d ", S_COUNT(sdata));
    num += sprintf(str+num, " 0X%llX ", S_ZKEY(sdata));
    num += move_sprintf(str+num, mvlist->mlist->move, sdata);
    num += sprintf(str+num, "(%u %u)\n", mvlist->tdata.pn, mvlist->tdata.dn);
    return num;
}
int   search_inf_fprintf (FILE *restrict stream,
                          const mvlist_t *mvlist,
                          const sdata_t *sdata )
{
    int num = 0;
    num += fprintf(stream, " %d ", S_COUNT(sdata));
    num += fprintf(stream, " 0X%llX ", S_ZKEY(sdata));
    num += move_fprintf(stream, mvlist->mlist->move, sdata);
    num += fprintf(stream, "(%u %u)\n", mvlist->tdata.pn, mvlist->tdata.dn);
    return num;
}

/*
 * mlistの内容表示
 */
int mlist_sprintf   (char *restrict str,
                     const mlist_t *mlist,
                     const sdata_t *sdata )
{
    int num = 0;
    const mlist_t *list = mlist;
    while (list) {
        num += move_sprintf(str+num, list->move, sdata);
        num += sprintf(str+num, " ");
        list = list->next;
    }
    num += sprintf(str+num, "\n");
    return num;
}

int mlist_fprintf   (FILE *restrict stream,
                     const mlist_t *mlist,
                     const sdata_t *sdata )
{
    int num = 0;
    const mlist_t *list = mlist;
    while (list) {
        num += move_fprintf(stream, list->move, sdata);
        num += fprintf(stream, " ");
        list = list->next;
    }
    num += fprintf(stream, "\n");
    return num;
}

/*
 * mvlistの着手表示
 */
int mvlist_sprintf  (char *restrict str,
                     const mvlist_t *mvlist,
                     const sdata_t *sdata )
{
    int num = 0;
    const mvlist_t *list = mvlist;
    while (list) {
        num += mlist_sprintf(str+num, list->mlist, sdata);
        list = list->next;
    }
    return num;
}

int mvlist_fprintf  (FILE *restrict stream,
                     const mvlist_t *mvlist,
                     const sdata_t *sdata )
{
    int num = 0;
    const mvlist_t *list = mvlist;
    while (list) {
        num += mlist_fprintf(stream, list->mlist, sdata);
        list = list->next;
    }
    return num;
}

/*
 * mvlistの内容表示
 */

int mvlist_sprintf_with_item (char *restrict str,
                              const mvlist_t *mvlist,
                              const sdata_t *sdata )
{
    int num = 0;
    int item = 0;
    num += sprintf(str+num,
                   "id 着手(*) pn,dn,sh,hinc,inc,shh,cu,pr,len,nou,nou2\n");
    const mvlist_t *list = mvlist;
    while (list) {
        item++;
        if (list->mlist) {
            num += sprintf(str+num,"%d ", item);
            num += move_sprintf(str+num, list->mlist->move, sdata);
            if (list->mlist->next) {
                num += sprintf(str+num, "*");
            }
            num += sprintf(str+num,
                           " %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                           list->tdata.pn,
                           list->tdata.dn,
                           list->tdata.sh,
                           list->hinc,
                           list->inc,
                           list->search,
                           list->cu,
                           list->pr,
                           list->length,
                           list->nouse,
                           list->nouse2
                           );
        }
        list = list->next;
    }
    return num;
}

int mvlist_fprintf_with_item (FILE *restrict stream,
                              const mvlist_t *mvlist,
                              const sdata_t *sdata )
{
    int num = 0;
    int item = 0;
    num += fprintf(stream,
                   "id 着手(*) pn,dn,sh,hinc,inc,shh,cu,pr,len,nou,nou2\n");
    const mvlist_t *list = mvlist;
    while (list) {
        item++;
        if (list->mlist) {
            num += fprintf(stream,"%d ", item);
            num += move_fprintf(stream, list->mlist->move, sdata);
            if (list->mlist->next) {
                num += fprintf(stream, "*");
            }
            num += fprintf(stream,
                           " %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                           list->tdata.pn,
                           list->tdata.dn,
                           list->tdata.sh,
                           list->hinc,
                           list->inc,
                           list->search,
                           list->cu,
                           list->pr,
                           list->length,
                           list->nouse,
                           list->nouse2
                           );
        }
        list = list->next;
    }
    return num;
}

void tsume_print                (const sdata_t   *sdata,
                                 tbase_t         *tbase,
                                 unsigned int      flag)
{
    if(g_redundant){
        printf("参考詰手順　\n"
               "手数:　着手(先頭が選択着手） \n");
    } else{
        printf("詰手順　\n"
               "手数:　着手（ PN値 残り手数 駒余り 余り駒数 )\n");
    }
    _tsume_print_or(sdata, tbase, flag);
    return;
}

int tsume_fprint                (FILE            *stream,
                                 const sdata_t   *sdata,
                                 tbase_t         *tbase,
                                 unsigned int     flag )
{
    int num = 0;
    
    if(g_redundant){
        num += fprintf(stream, "参考詰手順　\n"
                               "手数:　着手(先頭が選択着手） \n");
    } else{
        num += fprintf(stream, "詰手順　\n"
                               "手数:　着手（ PN値 残り手数 駒余り 余り駒数 )\n");
    }
    
    num += _tsume_fprint_or(stream, sdata, tbase, flag);
    return num;
}

void tsume_debug                (const sdata_t   *sdata,
                                 tbase_t         *tbase)
{
    st_disp_flag = true;
    printf("=================詰手順確認モード開始=================\n");
    tsume_debug_or(sdata, tbase);
    printf("-----------------詰手順確認モード終了-----------------\n");
    return;
}

void generate_kif_file          (const char      *filename,
                                 const sdata_t   *sdata   ,
                                 tbase_t         *tbase  )
{
    FILE *fp;
    fp = fopen(filename, "w");
    if(!fp){
        //エラー処理
        printf("info string I/O error:%s\n", strerror(errno));
        printf("info string %s\n",filename);
        printf("info string kif_file could't be created.\n");
        return;
    }
    //コメント表記　プログラム名　バージョン
    fprintf(fp,
            "#KIF version=2.0 encoding=UTF-8\n"
            "# KIF形式棋譜ファイル\n"
            "# Generated by %s (%s)\n"
            ,PROGRAM_NAME, VERSION_INFO );
    //初期局面表記
    bod_fprintf(fp, sdata);
    //対局者
    fprintf(fp, "先手：\n後手：\n手数----指手---------消費時間--\n");
    //着手
    move_t dummy = {0,100};
    gen_kif_file_or(fp, sdata, tbase, dummy);
    
    fclose(fp);
    return;
}

void tlist_display              (const sdata_t   *sdata,
                                 turn_t              tn,
                                 tbase_t         *tbase)
{
    uint64_t address = HASH_FUNC(S_ZKEY(sdata), tbase);
    zfolder_t *zfolder = *(tbase->table+address);
    //局面表に同一盤面があるか
    while(zfolder){
        if(zfolder->zkey == S_ZKEY(sdata)) break;
        zfolder = zfolder->next;
    }
    //無い場合、その旨出力
    if(!zfolder){
        printf("No data exist in tbase.\n");
        return;
    }
    //同一盤面ありの場合、
    mcard_t *mcard = zfolder->mcard;
    unsigned int cmp_res;
    //mkey_t mkey = tn?S_GMKEY(sdata):S_SMKEY(sdata);
    mkey_t mkey;
    tn ? MKEY_COPY(mkey, S_GMKEY(sdata)):MKEY_COPY(mkey, S_SMKEY(sdata));
    memset(g_mcard, 0, sizeof(mcard_t *)*N_MCARD_TYPE);
    uint16_t tmp_pn = 1;
    while(mcard){
        cmp_res = MKEY_COMPARE(mkey, mcard->mkey);
        if     (cmp_res == MKEY_SUPER){
            //詰み
            if     (!mcard->tlist->tdata.pn){
                if(!g_mcard[SUPER_TSUMI]) g_mcard[SUPER_TSUMI] = mcard;
                else{
                    unsigned int res =
                    MKEY_COMPARE(g_mcard[SUPER_TSUMI]->mkey, mcard->mkey);
                    if(res == MKEY_SUPER) g_mcard[SUPER_TSUMI] = mcard;
                }
            }
            //不詰み
            else if(!mcard->tlist->tdata.dn){
                
            }
            //その他
            else                            {

            }
        }
        else if(cmp_res == MKEY_INFER){
            //詰み
            if     (!mcard->tlist->tdata.pn){
                
            }
            //不詰み
            else if(!mcard->tlist->tdata.dn){
                if(!g_mcard[INFER_FUDUMI]) g_mcard[INFER_FUDUMI] = mcard;
                else{
                    unsigned int res =
                    MKEY_COMPARE(g_mcard[INFER_FUDUMI]->mkey, mcard->mkey);
                    if(res == MKEY_INFER) g_mcard[INFER_FUDUMI] = mcard;
                }
            }
            //その他
            else                            {
                tlist_t *tlist = mcard->tlist;
                while(tlist){
                    tmp_pn = MAX(tmp_pn, tlist->tdata.pn);
                    tlist = tlist->next;
                }
                if(mcard->current){
                    tmp_pn = MAX(tmp_pn, mcard->cpn);
                }
            }
        }
        else if(cmp_res == MKEY_EQUAL){
            //詰み
            if     (   !mcard->tlist->tdata.pn ){
                if(!g_mcard[EQUAL_TSUMI]) g_mcard[EQUAL_TSUMI] = mcard;
            }
            //不詰み
            else if(   !mcard->tlist->tdata.dn ){
                if(!g_mcard[EQUAL_FUDUMI]) g_mcard[EQUAL_FUDUMI] = mcard;
            }
            //その他
            else                                {
                if(!g_mcard[EQUAL_UNKNOWN]) g_mcard[EQUAL_UNKNOWN] = mcard;
            }
        }
        mcard = mcard->next;
    }
    //データ内容表示
    if     (g_mcard[SUPER_TSUMI])  {
        printf("優越詰み %u手\n", g_mcard[SUPER_TSUMI]->tlist->tdata.sh);
        return;
    }
    else if(g_mcard[INFER_FUDUMI]) {
        printf("劣化不詰\n");
        return;
    }
    else if(g_mcard[EQUAL_TSUMI])  {
        printf("詰み %u手\n", g_mcard[EQUAL_TSUMI]->tlist->tdata.sh);
        return;
    }
    else if(g_mcard[EQUAL_FUDUMI]) {
        printf("不詰\n");
        return;
    }
    else if(g_mcard[EQUAL_UNKNOWN]){
        printf("不明\n");
        tlist_t *tlist = g_mcard[EQUAL_UNKNOWN]->tlist;
        while(tlist){
            printf("dp=%d ", tlist->dp);
            printf("pn=%d ", tlist->tdata.pn);
            printf("dn=%d ", tlist->tdata.dn);
            printf("sh=%d \n", tlist->tdata.sh);
            tlist = tlist->next;
        }
    }
    return;
}

/* -----------------
 スタティック関数実装部
 ----------------- */

void _tsume_print_or             (const sdata_t  *sdata,
                                 tbase_t         *tbase,
                                  unsigned int    flag  )
{
    MAKE_TREE_SET_PROTECT(sdata, S_TURN(sdata), tbase);
    
    //着手生成
    mvlist_t *list = generate_check(sdata, tbase);
    g_tsearchinf.nodes++;
    if(!list){
        printf("にて不詰\n");
        return;
    }
    
    //局面表を参照
    sdata_t sbuf;
    mvlist_t *tmp = list;
    while (tmp) {
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
    
    //並べ替え
    list = sdata_mvlist_sort(list, sdata, proof_number_comp);
    //着手を表示
    if(flag & TP_ZKEY) printf("%u:0X%llX :", S_COUNT(sdata)+1, S_ZKEY(sdata));
    else               printf("%u:", S_COUNT(sdata)+1);
    tmp = list;
    while (tmp) {
        if(flag & TP_ALLMOVE){}
        else{
            if(tmp->tdata.pn) break;
        }
        MOVE_PRINTF(tmp->mlist->move, sdata);
        if(!g_redundant){
            printf("(%u %u %u %u) ",
                   tmp->tdata.pn,
                   tmp->tdata.sh,
                   tmp->inc,
                   tmp->nouse2);
        }
        tmp = tmp->next;
    }
    printf("\n");
    
    //エラーチェック
    if(list->cu){
        g_error = true;
        sprintf(g_error_location, "%s line %d", __FILE__, __LINE__);
        return;
    }
    if(!list->tdata.pn && !list->tdata.sh){
        printf("まで%u手詰め\n", S_COUNT(sdata)+1);
        return;
    }
    //着手を進める（再帰呼び出し)
    if(list && list->mlist){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        _tsume_print_and(&sbuf, tbase, flag);
    }
    mvlist_free(list);
    return;
}

void _tsume_print_and           (const sdata_t  *sdata,
                                 tbase_t        *tbase,
                                 unsigned int     flag )
{
    if(S_COUNT(sdata)>=TSUME_MAX_DEPTH) return;

    //着手生成
    mvlist_t *list = generate_evasion(sdata, tbase);
    g_tsearchinf.nodes++;
    if(!list){
        unsigned int cnt = S_COUNT(sdata);
        printf("まで%u手詰め\n", cnt);
        return;
    }
    //局面表を参照
    mvlist_t *tmp = list, *tmp1;
    sdata_t sbuf;
    turn_t tn = TURN_FLIP(S_TURN(sdata));

    while (tmp) {
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_key_forward(&sbuf, tmp->mlist->move);
        make_tree_lookup(&sbuf, tmp, tn, tbase);
        //合駒は全て展開しておく。
        if (tmp->mlist->next) {
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
    
    //着手を表示
    if(flag & TP_ZKEY) printf("%u:0X%llX :", S_COUNT(sdata)+1, S_ZKEY(sdata));
    else               printf("%u:", S_COUNT(sdata)+1);
    tmp = list;
    while (tmp) {
        MOVE_PRINTF(tmp->mlist->move, sdata);
        
        if(!g_redundant){
            printf("(%u %u %u %u) ",
                   tmp->tdata.pn,
                   tmp->tdata.sh,
                   tmp->inc,
                   tmp->nouse2);
        }
        tmp = tmp->next;
    }
    printf("\n");
    
    //着手を進める。（再帰呼び出し）
    if(list && list->mlist){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        _tsume_print_or(&sbuf, tbase, flag);
    }
    if(current) current->current = 0;
    if(g_redundant)mtt_reset(sdata, tn, g_mtt);
    mvlist_free(list);
    return;
}

int _tsume_fprint_or            (FILE           *stream,
                                 const sdata_t  *sdata,
                                 tbase_t        *tbase,
                                 unsigned int    flag )
{
    int num = 0;
    MAKE_TREE_SET_PROTECT(sdata, S_TURN(sdata), tbase);
    
    //着手生成
    mvlist_t *list = generate_check(sdata, tbase);
    g_tsearchinf.nodes++;
    if(!list){
        num += fprintf(stream, "にて不詰\n");
        return num;
    }
    
    //局面表を参照
    sdata_t sbuf;
    mvlist_t *tmp = list;
    while (tmp) {
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
    
    //並べ替え
    list = sdata_mvlist_sort(list, sdata, proof_number_comp);
    //着手を表示
    if(flag & TP_ZKEY)
        num +=
        fprintf(stream, "%u:0X%llX :", S_COUNT(sdata)+1, S_ZKEY(sdata));
    else
        num +=
        fprintf(stream, "%u:", S_COUNT(sdata)+1);
    
    tmp = list;
    while (tmp) {
        if(flag & TP_ALLMOVE){}
        else{
            if(tmp->tdata.pn) break;
        }
        num += move_fprintf(stream,tmp->mlist->move,sdata);
        
        if(!g_redundant){
            num += fprintf(stream, "(%u %u %u %u) ",
                           tmp->tdata.pn,
                           tmp->tdata.sh,
                           tmp->inc,
                           tmp->nouse2);
        }
        tmp = tmp->next;
    }
    num += fprintf(stream, "\n");
    
    //エラーチェック
    if(list->cu){
        g_error = true;
        sprintf(g_error_location, "%s line %d", __FILE__, __LINE__);
        return num;
    }
    
    //着手を進める（再帰呼び出し)
    if(list && list->mlist){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        num += _tsume_fprint_and(stream, &sbuf, tbase, flag);
    }
    mvlist_free(list);
    
    return num;
}

int _tsume_fprint_and           (FILE           *stream,
                                 const sdata_t  *sdata,
                                 tbase_t        *tbase,
                                 unsigned int    flag )
{
    int num = 0;
    if(S_COUNT(sdata)>=TSUME_MAX_DEPTH) return num;
    
    //着手生成
    mvlist_t *list = generate_evasion(sdata, tbase);
    g_tsearchinf.nodes++;
    if(!list){
        unsigned int cnt = S_COUNT(sdata);
        num += fprintf(stream, "まで%u手詰め\n", cnt);
        return num;
    }
    
    //局面表を参照
    mvlist_t *tmp = list, *tmp1;
    sdata_t sbuf;
    turn_t tn = TURN_FLIP(S_TURN(sdata));
    
    while (tmp) {
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_key_forward(&sbuf, tmp->mlist->move);
        make_tree_lookup(&sbuf, tmp, tn, tbase);
        //合駒は全て展開しておく。
        if (tmp->mlist->next) {
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
    
    //着手を表示
    if(flag & TP_ZKEY)
        num +=
        fprintf(stream, "%u:0X%llX :", S_COUNT(sdata)+1, S_ZKEY(sdata));
    else
        num +=
        fprintf(stream, "%u:", S_COUNT(sdata)+1);
    tmp = list;
    while (tmp) {
        num += move_fprintf(stream,tmp->mlist->move,sdata);
        if(!g_redundant){
            num += fprintf(stream, "(%u %u %u %u) ",
                           tmp->tdata.pn,
                           tmp->tdata.sh,
                           tmp->inc,
                           tmp->nouse2);
        }
        tmp = tmp->next;
    }
    num += fprintf(stream, "\n");
    
    //着手を進める。（再帰呼び出し）
    if(list && list->mlist){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        num += _tsume_fprint_or(stream, &sbuf, tbase, flag);
    }
    if(current) current->current = 0;
    if(g_redundant)mtt_reset(sdata, tn, g_mtt);
    mvlist_free(list);
    return num;
}

void tsume_debug_or              (const sdata_t   *sdata,
                                  tbase_t         *tbase)
{
    //着手生成
    mvlist_t *list = generate_check(sdata, tbase);
    g_tsearchinf.nodes++;
    
    if(!list){
        printf("--------------------------------------------------\n");
        SDATA_PRINTF(sdata, PR_BOARD);
        printf("%u手目\n", S_COUNT(sdata)+1);
        printf("ID:合法手(pn,dn,詰手数,駒余り,枚数)\n"
               "-------------------------------\n");
        printf("にて不詰 (b(back)/q(quit)\n");
        //戻りor終了選択
        char str[16];
        memset(str, 0, sizeof(str));
        while(true){
            fgets(str, sizeof(str)-1, stdin);
            if(mblen(str, 16)!=1){
                printf("文字コードが間違っています。再度入力してください\n");
                memset(str, 0, sizeof(str));
            }
            else if(!memcmp(str, "b", sizeof(char)*1)) break;
            else if(!memcmp(str, "q", sizeof(char)*1)){
                st_disp_flag = false;
                break;
            }
            else{
                printf("入力が間違っています。再度入力してください\n");
            }
        }
        return;
    }
    //局面表を参照
    sdata_t sbuf;
    mvlist_t *tmp = list;
    while (tmp) {
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_key_forward(&sbuf, tmp->mlist->move);
        make_tree_lookup(&sbuf, tmp, S_TURN(sdata), tbase);
        //千日手判定
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
    //並べ替え
    list = sdata_mvlist_sort(list, sdata, proof_number_comp);
    int num;
    char str[16];
    unsigned int select;
    while(true){
        printf("--------------------------------------------------\n");
        //局面表示
        SDATA_PRINTF(sdata, PR_BOARD);
        //着手表示
        printf("%u手目\n", S_COUNT(sdata)+1);
        if(g_redundant){
            printf("ID:合法手(pn,dn)\n"
                   "-------------------------------\n");
        }else{
            printf("ID:合法手(pn,dn,詰手数,駒余り,枚数)\n"
                   "-------------------------------\n");
        }
        tmp = list;
        num = 0;
        while (tmp) {
            printf("%2d: ", num); num++;
            MOVE_PRINTF(tmp->mlist->move, sdata);
            if(!g_redundant){
                printf("(%u %u %u %u %u) \n",
                       tmp->tdata.pn,
                       tmp->tdata.dn,
                       tmp->tdata.sh,
                       tmp->inc,
                       tmp->nouse2);
            } else{
                printf("(%u %u) \n",
                       tmp->tdata.pn,
                       tmp->tdata.dn);
            }
            tmp = tmp->next;
        }
        //着手＆終了選択
        if(S_COUNT(sdata))
            printf("着手を選択してください（数字/b(back)/q(quit))\n");
        else
            printf("着手を選択してください（数字/q(quit))\n");
        fgets(str, sizeof(str)-1, stdin);
        if(mblen(str, 16)!=1){
            printf("文字コードが間違っています。再度入力してください\n");
            memset(str, 0, sizeof(str));
        }
        else if(S_COUNT(sdata) && !memcmp(str, "b", sizeof(char)*1)) break;
        else if(!memcmp(str, "q", sizeof(char)*1)){
            st_disp_flag = false;
            break;
        }
        else if(str[0]<'0' || str[0]>'9'){
            printf("入力が間違っています。再度入力してください\n");
        }
        else{
            //エラーチェック
            if(list->cu){
                g_error = true;
                sprintf(g_error_location, "%s line %d", __FILE__, __LINE__);
                return;
            }
            select = atoi(str);
            tmp = mvlist_nth(list, select);
            //着手を進める（再帰呼び出し)
            if(tmp){
                memcpy(&sbuf, sdata, sizeof(sdata_t));
                sdata_move_forward(&sbuf, tmp->mlist->move);
                tsume_debug_and(&sbuf, tbase);
                if(!st_disp_flag) break;
            }
        }
    }
    mvlist_free(list);
    return;
}

void tsume_debug_and             (const sdata_t   *sdata,
                                  tbase_t         *tbase)
{
    //着手生成
    mvlist_t *list = generate_evasion(sdata, tbase);
    g_tsearchinf.nodes++;
    if(!list){
        unsigned int cnt = S_COUNT(sdata);
        printf("--------------------------------------------------\n");
        SDATA_PRINTF(sdata, PR_BOARD);
        printf("%u手目\n", S_COUNT(sdata)+1);
        printf("ID:合法手(pn,dn,詰手数,駒余り,枚数)\n"
               "-------------------------------\n");
        printf("まで%u手詰め (b(back)/q(quit))\n", cnt);
        //戻りor終了選択
        char str[16];
        memset(str, 0, sizeof(str));
        while(true){
            fgets(str, sizeof(str)-1, stdin);
            if(mblen(str, 16)!=1){
                printf("文字コードが間違っています。再度入力してください\n");
                memset(str, 0, sizeof(str));
            }
            else if(!memcmp(str, "b", sizeof(char)*1)) break;
            else if(!memcmp(str, "q", sizeof(char)*1)){
                st_disp_flag = false;
                break;
            }
            else{
                printf("入力が間違っています。再度入力してください\n");
            }
        }
        return;
    }
    //局面表を参照
    mvlist_t *tmp = list, *tmp1;
    sdata_t sbuf;
    turn_t tn = TURN_FLIP(S_TURN(sdata));
    
    while (tmp) {
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_key_forward(&sbuf, tmp->mlist->move);
        make_tree_lookup(&sbuf, tmp, tn, tbase);
        //合駒は全て展開しておく。
        if (tmp->mlist->next) {
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
    //着手の表示
    int num;
    char str[16];
    unsigned int select;
    while(true){
        printf("--------------------------------------------------\n");
        //局面の表示
        SDATA_PRINTF(sdata, PR_BOARD);
        //着手表示
        printf("%u手目\n", S_COUNT(sdata)+1);
        if(g_redundant){
            printf("ID:合法手(pn,dn)\n"
                   "-------------------------------\n");
        } else{
            printf("ID:合法手(pn,dn,詰手数,駒余り,枚数)\n"
                   "-------------------------------\n");
        }
        tmp = list;
        num = 0;
        while (tmp) {
            printf("%2d: ", num); num++;
            MOVE_PRINTF(tmp->mlist->move, sdata);
            if(!g_redundant){
                printf("(%u %u %u %u %u) \n",
                       tmp->tdata.pn,
                       tmp->tdata.dn,
                       tmp->tdata.sh,
                       tmp->inc,
                       tmp->nouse2);
            } else{
                printf("(%u %u) \n",
                       tmp->tdata.pn,
                       tmp->tdata.dn);
            }
            tmp = tmp->next;
        }
        //着手&終了選択
        printf("着手を選択してください（数字/b(back)/q(quit))\n");
        fgets(str, sizeof(str)-1, stdin);
        if(mblen(str, 16)!=1){
            printf("文字コードが間違っています。再度入力してください\n");
            memset(str, 0, sizeof(str));
        }
        else if(!memcmp(str, "b", sizeof(char)*1)) break;
        else if(!memcmp(str, "q", sizeof(char)*1)){
            st_disp_flag = false;
            break;
        }
        else if(str[0]<'0' || str[0]>'9'){
            printf("入力が間違っています。再度入力してください\n");
        }
        else{
            select = atoi(str);
            tmp = mvlist_nth(list, select);
            //着手を進める（再帰呼び出し)
            if(tmp){
                memcpy(&sbuf, sdata, sizeof(sdata_t));
                sdata_move_forward(&sbuf, tmp->mlist->move);
                tsume_debug_or(&sbuf, tbase);
                if(!st_disp_flag) break;
            }
        }
    }
    if(current) current->current = 0;
    if(g_redundant)mtt_reset(sdata, tn, g_mtt);
    mvlist_free(list);
    return;
}

void gen_kif_file_or             (FILE *restrict  stream,
                                  const sdata_t   *sdata,
                                  tbase_t         *tbase,
                                  move_t           prev  )
{
    MAKE_TREE_SET_PROTECT(sdata, S_TURN(sdata), tbase);
    
    //着手生成
    mvlist_t *list = generate_check(sdata, tbase);
    g_tsearchinf.nodes++;
    if(!list){
        printf("にて不詰\n");
        return;
    }
    
    //局面表を参照
    sdata_t sbuf;
    mvlist_t *tmp = list;
    while (tmp) {
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_key_forward(&sbuf, tmp->mlist->move);
        make_tree_lookup(&sbuf, tmp, S_TURN(sdata), tbase);
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
    
    //先頭着手を表示
    move_t move = list->mlist->move;
    clock_t elapsed = g_tsearchinf.elapsed;
    kif_move_fprintf(stream, sdata, move, prev, elapsed, elapsed);
    
    //着手を進める（再帰呼び出し)
    if(list && list->mlist){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        gen_kif_file_and(stream, &sbuf, tbase, move);
    }
    mvlist_free(list);
    return;
}

void gen_kif_file_and            (FILE *restrict  stream,
                                  const sdata_t   *sdata,
                                  tbase_t         *tbase,
                                  move_t           prev  )
{
    if(S_COUNT(sdata)>=TSUME_MAX_DEPTH) return;
    turn_t tn = TURN_FLIP(S_TURN(sdata));
    //着手生成
    mvlist_t *list = generate_evasion(sdata, tbase);
    g_tsearchinf.nodes++;
    if(!list){
        fprintf(stream, "%4u 詰み", S_COUNT(sdata)+1);
        return;
    }
    //局面表を参照
    mvlist_t *tmp = list, *tmp1;
    sdata_t sbuf;

    while (tmp) {
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_key_forward(&sbuf, tmp->mlist->move);
        make_tree_lookup(&sbuf, tmp, tn, tbase);
        //合駒は全て展開しておく。
        if (tmp->mlist->next) {
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
    
    //先頭着手を表示
    move_t move = list->mlist->move;
    kif_move_fprintf(stream, sdata, move, prev, 0, 0);
    
    //着手を進める（再帰呼び出し)
    if(list && list->mlist){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_move_forward(&sbuf, list->mlist->move);
        gen_kif_file_or(stream, &sbuf, tbase, move);
    }
    mvlist_free(list);
    return;
}
