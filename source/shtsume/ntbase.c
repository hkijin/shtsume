//
//  ntbase.c
//  shtsume2
//
//  Created by Hkijin on 2022/02/10.
//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "shtsume.h"

/* -----------
 グローバル変数
 ----------- */

bool g_gc_stat;                   /*  詰み発見後, true  */
short g_gc_max_level;             /*  gcの強度         */
short g_gc_num;                   /*  gcの実施回数      */
mcard_t *g_mcard[N_MCARD_TYPE];   /*  mcardの一致タイプ */

/* --------------
 スタティック変数
 -------------- */

static char st_tpin[N_SQUARE];    /* 無駄合い判定時に使用するpinデータ */
static int  st_n_tsumi;           /* 局面表内の詰みデータ数 */
static int  st_del_tsumi;         /* 削除した詰みデータ数   */
static int  st_n_fudumi;          /* 局面表内の不詰データ数 */
static int  st_del_fudumi;        /* 削除した不詰データ数   */
static int  st_n_fumei;           /* 局面表内の不明データ数 */
static int  st_del_fumei;         /* 削除した不明データ数   */
static bool st_tsumi_delete_flag; /* true 詰みデータの削除 */
//static bool st_gc_flag;           /* gcの強度変更flag     */

/* --------------
 スタティック関数
 -------------- */

//flag false: 詰み発見前  true: 詰み発見後
static void _tbase_lookup (const sdata_t   *sdata,
                           mvlist_t       *mvlist,
                           turn_t              tn,
                           bool              flag,
                           tbase_t         *tbase );

static void tsumi_update  (const sdata_t   *sdata,
                           mvlist_t       *mvlist,
                           turn_t              tn,
                           bool              flag,
                           tbase_t         *tbase );
static void fudumi_update (const sdata_t   *sdata,
                           mvlist_t       *mvlist,
                           turn_t              tn,
                           bool              flag,
                           tbase_t         *tbase );
static void fumei_update  (const sdata_t   *sdata,
                           mvlist_t       *mvlist,
                           turn_t              tn,
                           bool              flag,
                           tbase_t         *tbase );

static void _tbase_update (const sdata_t   *sdata,
                           mvlist_t       *mvlist,
                           turn_t              tn,
                           bool              flag,
                           tbase_t         *tbase );
static bool _invalid_drops(const sdata_t   *sdata,
                           unsigned int       src,
                           unsigned int      dest,
                           tbase_t         *tbase );
static void set_tpin      (const sdata_t *sdata,
                           char *tpin             );
static bool src_check     (const sdata_t  *sdata,
                           bitboard_t        eff,
                           unsigned int     dest,
                           tbase_t        *tbase  );
static void sdata_tentative_move (sdata_t *sdata,
                                  char src,
                                  char dest,
                                  bool promote    );
static bool is_move_possible(komainf_t koma, char dest);

/* -----------
 実装
 ------------ */

tbase_t*   create_tbase     (uint64_t base_size)
{
    /* 構造体生成
     */
    tbase_t *tbase = (tbase_t *)calloc(1, sizeof(tbase_t));
    if(!tbase) return NULL;
    /* INDEX生成
       サイズはbase_sizeより大きな２のベキ乗とする。            //INDEX (メモリ）設定値
    if     (base_size<(1<<14)) tbase->sz_tbl = (1<<14);  //16384  16K    1
    else if(base_size<(1<<15)) tbase->sz_tbl = (1<<15);                  2
    else if(base_size<(1<<16)) tbase->sz_tbl = (1<<16);                  4
    else if(base_size<(1<<17)) tbase->sz_tbl = (1<<17);
    else if(base_size<(1<<18)) tbase->sz_tbl = (1<<18);
    else if(base_size<(1<<19)) tbase->sz_tbl = (1<<19);
    else if(base_size<(1<<20)) tbase->sz_tbl = (1<<20);  //1M     8M     64
    else if(base_size<(1<<21)) tbase->sz_tbl = (1<<21);
    else if(base_size<(1<<22)) tbase->sz_tbl = (1<<22);
    else if(base_size<(1<<23)) tbase->sz_tbl = (1<<23);
    else if(base_size<(1<<24)) tbase->sz_tbl = (1<<24);                  1024
    else if(base_size<(1<<25)) tbase->sz_tbl = (1<<25);                  2048
    else if(base_size<(1<<26)) tbase->sz_tbl = (1<<26);                  4096
    else if(base_size<(1<<27)) tbase->sz_tbl = (1<<27);                  8192
    else if(base_size<(1<<28)) tbase->sz_tbl = (1<<28);                  16384
    else if(base_size<(1<<29)) tbase->sz_tbl = (1<<29);                  32768
    else                       tbase->sz_tbl = (1<<30);  //1G     8G     65536
    */
    int i=14;
    while(true){
        if(i==30 || (1<<i)>base_size) {tbase->sz_tbl = (1<<i); break;}
        i++;
    }
    tbase->mask   = tbase->sz_tbl-1;
    tbase->sz_elm = base_size;
    
    //INDEX生成
    tbase->table = (zfolder_t **)calloc(tbase->sz_tbl, sizeof(zfolder_t *));
    if(!tbase->table){
        free(tbase);
        return NULL;
    }
    
    //未使用zfolder保管庫
    tbase->zstack = (zfolder_t *)calloc(tbase->sz_elm, sizeof(zfolder_t));
    if(!tbase->zstack){
        free(tbase->table);
        free(tbase);
        return NULL;
    }
    
    //未使用mcard保管庫
    tbase->mstack = (mcard_t *)calloc(tbase->sz_elm, sizeof(mcard_t));
    if(!tbase->mstack){
        free(tbase->zstack);
        free(tbase->table);
        free(tbase);
        return NULL;
    }
    
    //未使用tlist保管庫
    tbase->tstack = (tlist_t *)calloc(tbase->sz_elm, sizeof(tlist_t));
    if(!tbase->tstack){
        free(tbase->mstack);
        free(tbase->zstack);
        free(tbase->table);
        free(tbase);
        return NULL;
    }
    tbase->zpool = tbase->zstack;
    tbase->mpool = tbase->mstack;
    tbase->tpool = tbase->tstack;
    zfolder_t *zfolder = tbase->zstack;
    mcard_t *mcard = tbase->mstack;
    tlist_t *tlist = tbase->tstack;
    i=0;
    while(i<tbase->sz_elm-1){
        zfolder->next = zfolder+1;
        mcard->next = mcard+1;
        tlist->next = tlist+1;
        zfolder = zfolder->next;
        mcard = mcard->next;
        tlist = tlist->next;
        i++;
    }
    return tbase;
}

void initialize_tbase       (tbase_t  *tbase)
{
    memset(tbase->table, 0, sizeof(zfolder_t *)*tbase->sz_tbl);
    memset(tbase->zpool, 0, sizeof(zfolder_t)*tbase->sz_elm);
    memset(tbase->mpool, 0, sizeof(mcard_t)*tbase->sz_elm);
    memset(tbase->tpool, 0, sizeof(tlist_t)*tbase->sz_elm);
    
    tbase->zstack = tbase->zpool;
    tbase->mstack = tbase->mpool;
    tbase->tstack = tbase->tpool;
    uint64_t i=0;
    zfolder_t *zfolder = tbase->zstack;
    mcard_t *mcard = tbase->mstack;
    tlist_t *tlist = tbase->tstack;
    while(i<tbase->sz_elm-1){
        zfolder->next = zfolder+1;
        mcard->next = mcard+1;
        tlist->next = tlist+1;
        zfolder = zfolder->next;
        mcard = mcard->next;
        tlist = tlist->next;
        i++;
    }
    tbase->num = 0;
    tbase->pr_num = 0;
    return;
}

void destroy_tbase          (tbase_t  *tbase)
{
    free(tbase->zpool);
    free(tbase->mpool);
    free(tbase->tpool);
    free(tbase->table);
    free(tbase);
    return;
}

/* ------------------------------------------------------------------------
 局面表のガベージコレクション
 局面表内には詰みデータ、不詰みデータ、および不明データが存在する。それぞれについて
 再探索コストを考慮し、以下の方針で削除していく。
 [詰み発見前] g_gc_stat = false
 1, 不明データ pn, dn, shのどれかがgc_level以下のデータを消していく
 2, 詰みデータ sh値が偶数のデータを消していく
 3, 不詰データ sh値が偶数のデータを消していく
 
 [詰み発見後] g_gc_stat = true
 ルート局面から探索を以下の方針で再度行う。
 詰方探索　局面表で詰みのデータは展開し、展開後データ保護をする。
 玉方探索　全ての合法手を展開し、展開後引用データを保護する（駒余り詰めに使用した局面を含む）
 1, 不明データ 全て消していく
 2, 詰みデータ sh値が偶数のデータを消していく
 3, 不詰データ sh値が偶数のデータを消していく
 
 もし、保護データと詰みデータと不詰データの合計の割合が100-gc_targetを超える場合、
 1, 不明データ 全て消していく
 2, 詰みデータ sh値が４で割れないデータを消していく
 3, 不詰データ 全て消していく
 
 ----------------------------------------------------------------------- */
//データの削除条件(true: 削除　false:　保存)
static bool delete_func     (tlist_t      *tl,
                             unsigned int gc_level)
{
    //詰め方の詰みデータを削除する。
    if(!tl->tdata.pn){
        st_n_tsumi++;
        if(st_tsumi_delete_flag){
            if(!(tl->tdata.sh%2)||tl->tdata.sh<2) {
                st_del_tsumi++;
                return true;
            }
        }
        else{
            if(!(tl->tdata.sh%2)) {
                st_del_tsumi++;
                return true;
            }
        }
    }
    //玉方の不詰データを削除する
    else if(!tl->tdata.dn){
        st_n_fudumi++;
        if(gc_level<GC_DELETE_OFFSET){
            if(!(tl->tdata.sh%2)) {
                st_del_fudumi++;
                return true;
            }
        }
        else {
            if(!(tl->tdata.sh%2)||tl->tdata.sh<gc_level-2) {
                st_del_fudumi++;
                return true;
            }
        }
    }
    //不明データ
    else{
        st_n_fumei++;
        if(   tl->tdata.sh<gc_level
           || tl->tdata.pn<gc_level
           || tl->tdata.dn<gc_level )
        {
            st_del_fumei++;
            return true;
        }
    }
    return false;
}

static void counter_reset   (void)
{
    st_n_fumei = 0; st_n_tsumi = 0; st_n_fudumi = 0;
    st_del_fumei = 0; st_del_tsumi = 0; st_del_fudumi = 0;
    return;
}

void tbase_gc              (tbase_t  *tbase)
{
    unsigned int gc_level = MAX(2,g_gc_max_level);
    zfolder_t *zfolder, *new_zfolder, *tmp;
    mcard_t *mcard, *new_mcard, *mc;
    tlist_t *tlist, *new_tlist, *tl;
    st_tsumi_delete_flag = false;
    uint64_t i, delete_num = 0;
    uint64_t gc_target = tbase->sz_elm*GC_DELETE_RATE/100;
    uint64_t n_tsumi_max = tbase->sz_elm*GC_TSUMI_RATE/100;
    //st_gc_flag = false;
    //ガベージコレクション実施宣言
    sprintf(g_str, "info string Garbage collection start. "
                   "size:%llu del_target:%llu protected:%llu gc_num:%d ",
            tbase->sz_elm, gc_target, tbase->pr_num, g_gc_num);
    record_log(g_str); puts(g_str);
    do{
        if(gc_level>g_gc_max_level) g_gc_max_level = gc_level;
        //gc_level出力
        sprintf(g_str, "info string gc_level:%u", gc_level);
        record_log(g_str); puts(g_str);
        counter_reset();
        for(i=0; i<tbase->sz_tbl; i++)
        {
            zfolder = *(tbase->table+i);
            new_zfolder = NULL;
            while(zfolder){
                //zfolderの先頭を切り離し、tmpとする。
                tmp = zfolder;
                zfolder = tmp->next;
                tmp->next = NULL;
                //tmpの中身をチェックする
                mcard = tmp->mcard;
                new_mcard = NULL;
                while(mcard){
                    //mcardの先頭を切り離し、mcとする。
                    mc = mcard;
                    mcard = mc->next;
                    mc->next = NULL;
                    if(!mc->protect && !mc->current)
                    {
                        //mcの中身をチェックする。
                        tlist = mc->tlist;
                        new_tlist = NULL;
                        while(tlist){
                        //tlistの先頭を切り離し、tlとする。
                        tl = tlist;
                        tlist = tl->next;
                        tl->next = NULL;
                            
                        if(delete_func(tl, gc_level))
                        {
                            tl->next = tbase->tstack;
                            tbase->tstack = tl;
                            delete_num++;
                        }
                        else{
                            tl->next = new_tlist;
                            new_tlist = tl;
                        }
                    }
                        mc->tlist = new_tlist;
                        //mc->tlistがNULLならmcを解放する。
                        if(!mc->tlist){
                            mc->next = tbase->mstack;
                            tbase->mstack = mc;
                        }
                        //そうで無ければmcをnew_cardに連結。
                        else{
                            mc->next = new_mcard;
                            new_mcard = mc;
                        }
                    }
                    else
                    {
                        mc->next = new_mcard;
                        new_mcard = mc;
                    }
                }
                tmp->mcard = new_mcard;
                //tmp->mcardがNULLであればtmpを解放
                if(!tmp->mcard){
                    tmp->next = tbase->zstack;
                    tbase->zstack = tmp;
                }
                //そうで無ければtmpをnew_zfolderに連結
                else{
                    tmp->next = new_zfolder;
                    new_zfolder = tmp;
                }
            }
            *(tbase->table+i) = new_zfolder;
        }
        //gc_summary表示
        sprintf(g_str, "info string mate    %d/%d", st_del_tsumi, st_n_tsumi);
        record_log(g_str); puts(g_str);
        sprintf(g_str, "info string nomate  %d/%d", st_del_fudumi, st_n_fudumi);
        record_log(g_str); puts(g_str);
        sprintf(g_str, "info string unknown %d/%d", st_del_fumei, st_n_fumei);
        record_log(g_str); puts(g_str);
        sprintf(g_str,"info string delete_num/gc_target %llu/%llu",
                delete_num,gc_target);
        record_log(g_str); puts(g_str);
        if(delete_num>gc_target) break;
        if(st_n_tsumi-st_del_tsumi>n_tsumi_max) st_tsumi_delete_flag = true;
        gc_level++;
    } while(true);
    
    //gc情報を出力する。
    sprintf(g_str,
            "info string "
            "delete_rate %llu%%. "
            "nodes %llu. mate %d. nomate %d. unknown %d",
            delete_num*100/tbase->sz_elm,
            g_tsearchinf.nodes,
            st_n_tsumi-st_del_tsumi,
            st_n_fudumi-st_del_fudumi,
            st_n_fumei-st_del_fumei
            );
    
    record_log(g_str);
    puts(g_str);
    sprintf(g_str, "info string Garbage collection end.");
    record_log(g_str);
    puts(g_str);
    tbase->num -= delete_num;
    g_gc_num++;
    return;
    
}

tlist_t* tlist_alloc        (tbase_t  *tbase)
{
    while(!tbase->tstack){
        //ガベージコレクション
        tbase_gc(tbase);
    }
    tlist_t *tlist = tbase->tstack;
    tbase->tstack = tlist->next;
    memset(tlist, 0, sizeof(tlist_t));
    tbase->num++;
    return tlist;
}

void tlist_free             (tlist_t *tlist,
                             tbase_t *tbase)
{
    if(!tlist) return;
    tlist_t *last = tlist;
    while(1){
        (tbase->num)--;
        if(!last->next) break;
        last = last->next;
    }
    last->next = tbase->tstack;
    tbase->tstack = tlist;
    return;
}

mcard_t* mcard_alloc        (tbase_t  *tbase)
{
    mcard_t *mcard = tbase->mstack;
    tbase->mstack = mcard->next;
    memset(mcard, 0, sizeof(mcard_t));
    return mcard;
}

zfolder_t* zfolder_alloc    (tbase_t  *tbase)
{
    zfolder_t *zfolder = tbase->zstack;
    tbase->zstack = zfolder->next;
    memset(zfolder, 0, sizeof(zfolder_t));
    return zfolder;
}

/* データ検索関数 */
void tbase_lookup           (const sdata_t *sdata,
                             mvlist_t      *mvlist,
                             turn_t         tn,
                             tbase_t       *tbase  )
{
    _tbase_lookup(sdata, mvlist, tn, false, tbase);
#if DEBUG
    if(S_ZKEY(sdata)==0x5b0cd113a2a3d35b){
        printf("lookup ");
        FPRINTF_MKEY(stdout, S_SMKEY(sdata));
        printf("dp = %u ",S_COUNT(sdata));
        printf("(%u, %u) ",mvlist->tdata.pn, mvlist->tdata.dn);
        printf("nodes = %llu\n", g_tsearchinf.nodes);
    }
#endif //DEBUG
    return;
}

//詰探索木に利用する局面データの保護（GCで消失した場合は再構築)
void make_tree_lookup    (const sdata_t   *sdata,
                          mvlist_t       *mvlist,
                          turn_t              tn,
                          tbase_t         *tbase )
{
    _tbase_lookup(sdata, mvlist, tn, true, tbase);
    return;
}

/* 無駄合い判定用関数
   詰みデータあり         1
 　不詰、不明データあり    0
 　データ無し           -1
 */
static int _hs_tbase_lookup     (const sdata_t *sdata,
                                 turn_t         tn,
                                 tbase_t       *tbase)
{
    uint64_t address = HASH_FUNC(S_ZKEY(sdata), tbase);
    zfolder_t *zfolder = *(tbase->table+address);
    
    while(zfolder){
        if(zfolder->zkey == S_ZKEY(sdata)) break;
        zfolder = zfolder->next;
    }
    if(!zfolder) return -1;
    mcard_t *mcard = zfolder->mcard;
    mkey_t mkey;
    tn ? MKEY_COPY(mkey, S_GMKEY(sdata)):MKEY_COPY(mkey, S_SMKEY(sdata));
    unsigned int cmp_res;
    while(mcard){
        cmp_res = MKEY_COMPARE(mkey,mcard->mkey);
        //詰み
        if( cmp_res == MKEY_EQUAL||cmp_res == MKEY_SUPER )
        {
            if(!(mcard->tlist->tdata.pn))
                return 1;
        }
        //不詰
        if( cmp_res == MKEY_EQUAL||cmp_res == MKEY_INFER )
        {
            if(!(mcard->tlist->tdata.dn))
                return 0;
        }
        //不明
        if( cmp_res == MKEY_EQUAL)
        {
            if((mcard->tlist->tdata.pn))
                return 0;
        }
        
        mcard = mcard->next;
    }
    return -1;
}

bool hs_tbase_lookup            (const sdata_t *sdata,
                                 turn_t         tn,
                                 tbase_t       *tbase)
{
    if(!S_NOUTE(sdata)) return false;
    int res = _hs_tbase_lookup(sdata, tn, tbase);
    if(!g_gc_num && res>=0) return res;
    else if(!g_gc_num)      return false;
    else if(res>=0)         return res;
    /*
       gcがなされている場合、無駄合判定に使用された詰みデータが消去されている場合があるので
       確認しておく。
     */
    //着手生成
#if DEBUG
    if(S_ZKEY(sdata)==0xbbaf30394dcbb082)
    {
        SDATA_PRINTF(sdata, PR_BOARD);
    }
#endif //DEBUG
    mvlist_t *list = generate_evasion(sdata, tbase);
    if(!list) return true;
    g_tsearchinf.nodes++;
    
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
    if(!list->tdata.pn)
    {
        res = true;
    }
    else res = false;
    //ここでは証明駒および局面表のupdateは省略する
    mvlist_free(list);
    return res;
}

/* データ更新関数 */
void tbase_update           (const sdata_t *sdata,
                             mvlist_t      *mvlist,
                             turn_t         tn,
                             tbase_t       *tbase)
{
    return _tbase_update(sdata, mvlist, tn, false, tbase);
}
void make_tree_update       (const sdata_t *sdata,
                             mvlist_t      *mvlist,
                             turn_t         tn,
                             tbase_t       *tbase)
{
    return _tbase_update(sdata, mvlist, tn, true, tbase);
}

/* ------------------------------------------------------------
    tbase_set_current
  ・深化の際、千日手回避のため、該当局面にpnしきい値を設定する。
  ・局面表エントリはGCによる削除防止のためcurrent=1とする。
  ・対象は同一局面
  ・戻り値 : current設定された局面のtentry
 ------------------------------------------------------------ */
mcard_t* tbase_set_current  (const sdata_t *sdata,
                             mvlist_t      *mvlist,
                             turn_t         tn,
                             tbase_t       *tbase)
{
    uint64_t address = HASH_FUNC(S_ZKEY(sdata), tbase);
    zfolder_t *zfolder = *(tbase->table+address);
    zfolder_t *tmp = zfolder;
    mkey_t mkey;
    tn ? MKEY_COPY(mkey, S_GMKEY(sdata)):MKEY_COPY(mkey, S_SMKEY(sdata));
    while(zfolder){
        if(zfolder->zkey == S_ZKEY(sdata)) break;
        zfolder = zfolder->next;
    }
    //zfolderが無い場合、新規作成
    if(!zfolder){
        tlist_t   *new_tlist   = tlist_alloc(tbase);
        mcard_t   *new_mcard   = mcard_alloc(tbase);
        zfolder_t *new_zfolder = zfolder_alloc(tbase);
        new_tlist->dp = S_COUNT(sdata);
        memcpy(&(new_tlist->tdata),&g_tdata_init,sizeof(tdata_t));
        new_mcard->tlist = new_tlist;
        new_mcard->mkey = mkey;
        new_mcard->current = 1;
        new_mcard->cpn = mvlist->tdata.pn;
        new_zfolder->mcard = new_mcard;
        new_zfolder->zkey = S_ZKEY(sdata);
        new_zfolder->next = tmp;
        *(tbase->table+address) = new_zfolder;
        return new_mcard;
    }
    //以下はzfolderがあった場合の処理
    mcard_t *mcard = zfolder->mcard, *mc = mcard;
    while(mcard){
        if(!memcmp(&(mcard->mkey),&mkey, sizeof(mkey_t))) break;
        mcard = mcard->next;
    }
    //mcardが無い場合、新規作成
    if(!mcard){
        tlist_t *new_tlist = tlist_alloc(tbase);
        mcard_t *new_mcard = mcard_alloc(tbase);
        new_tlist->dp = S_COUNT(sdata);
        //memcpy(&(new_tlist->tdata),&(mvlist->tdata),sizeof(tdata_t));
        memcpy(&(new_tlist->tdata),&g_tdata_init,sizeof(tdata_t));
        new_mcard->tlist = new_tlist;
        new_mcard->mkey = mkey;
        new_mcard->current = 1;
        new_mcard->cpn = mvlist->tdata.pn;
        new_mcard->next = mc;
        zfolder->mcard = new_mcard;
        return new_mcard;
    }
    mcard->current = 1;
    mcard->cpn = mvlist->tdata.pn;
    return mcard;
}

/* -------------------------------------------------------------
   MAKE_TREE_SET_CURRENT  局面表のsdataに対応するデータをcuで保護する
   MAKE_TREE_SET_PROTECT　局面表のsdataに対応するデータをprで保護する
   対象は同一局面または優越局面
 ------------------------------------------------------------- */
mcard_t* make_tree_set_item (const sdata_t *sdata,
                             turn_t         tn,
                             bool           item,
                             tbase_t       *tbase)
{
    uint64_t address = HASH_FUNC(S_ZKEY(sdata), tbase);
    zfolder_t *zfolder = *(tbase->table+address);
    
    //局面表に同一盤面があるか？
    while(zfolder){
        if(zfolder->zkey == S_ZKEY(sdata)) break;
        zfolder = zfolder->next;
    }
    //zfolderが無い場合
    if(!zfolder){
        tlist_t *new_tlist = tlist_alloc(tbase);
        mcard_t *new_mcard = mcard_alloc(tbase);
        zfolder_t *new_zfolder = zfolder_alloc(tbase);
        //新規tlist作成
        new_tlist->dp = S_COUNT(sdata);
        new_tlist->tdata.pn = 1;
        new_tlist->tdata.dn = 1;
        new_tlist->tdata.sh = 0;
        //新規mcard作成
        new_mcard->mkey = tn?S_GMKEY(sdata):S_SMKEY(sdata);
        new_mcard->tlist = new_tlist;
        if(!item)new_mcard->current = 1;
        else if(!new_mcard->protect){
            new_mcard->protect = 1;
            tbase->pr_num++;
        }
        //新規zfolder作成
        new_zfolder->zkey = S_ZKEY(sdata);
        new_zfolder->mcard = new_mcard;
        new_zfolder->next = *(tbase->table+address);
        *(tbase->table+address) = new_zfolder;
        return new_mcard;
    }
    
    //mkey_t mkey = tn?S_GMKEY(sdata):S_SMKEY(sdata);
    mkey_t mkey;
    tn ? MKEY_COPY(mkey, S_GMKEY(sdata)):MKEY_COPY(mkey, S_SMKEY(sdata));
    mcard_t *mcard = zfolder->mcard;
    unsigned int cmp_res;
    while(mcard){
        cmp_res = MKEY_COMPARE(mkey, mcard->mkey);
        if     (cmp_res == MKEY_EQUAL){
            if     (!item) mcard->current = 1;
            else if(!mcard->protect){
                mcard->protect = 1;
                tbase->pr_num++;
            }
            return mcard;
        }
        else if(cmp_res == MKEY_SUPER){
            if(!mcard->tlist->tdata.pn){
                if(!item) mcard->current = 1;
                else if(!mcard->protect){
                    mcard->protect = 1;
                    tbase->pr_num++;
                }
                return mcard;
            }
        }
        mcard = mcard->next;
    }
    return NULL;
}

void tbase_clear_protect   (tbase_t       *tbase)
{
    uint64_t i;
    zfolder_t *zfolder;
    mcard_t   *mcard;
    for(i=0; i<tbase->sz_tbl; i++) {
        zfolder = *(tbase->table+i);
        while(zfolder){
            mcard = zfolder->mcard;
            while(mcard){
                mcard->protect = 0;
                mcard = mcard->next;
            }
            zfolder = zfolder->next;
        }
    }
    tbase->pr_num = 0;
    return;
}

/* ------------------------------------------------------------
 無駄合い判定
 srcの位置で王手をかけている飛び駒をdestの位置（玉に隣接した桝）に移動させ、
 詰んでいるかどうかを判定する。飛駒が成れる場合、成りも含めて判定する。
 また玉の隣接枡では王手している駒以外も動かしてみて詰み判定を行う。
 玉の近接位置での無駄合い判定 　　　　　　　　　　　invalid_drops
 玉から離れた位置での無駄合い判定（局面表を使用）   hs_invalid_drops
 詰みの場合(無駄合い)　true
 ------------------------------------------------------------ */
bool invalid_drops           (const sdata_t  *sdata,
                              unsigned int     dest,
                              tbase_t        *tbase  )
{
    //destに駒を動かす手を生成する。
    bitboard_t effect;
    set_tpin(sdata, st_tpin);
    //後手番
    if(S_TURN(sdata))
    {
        //SFU
        effect = EFFECT_TBL(dest, GFU, sdata);
        BBA_AND(effect, BB_SFU(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;
        
        //SKY
        effect = EFFECT_TBL(dest, GKY, sdata);
        BBA_AND(effect, BB_SKY(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;
        
        //SKE
        effect = EFFECT_TBL(dest, GKE, sdata);
        BBA_AND(effect, BB_SKE(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //SGI
        effect = EFFECT_TBL(dest, GGI, sdata);
        BBA_AND(effect, BB_SGI(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //SKI,STO,SNY,SNK,SNG
        effect = EFFECT_TBL(dest, GKI, sdata);
        BBA_AND(effect, BB_STK(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //SKA
        effect = EFFECT_TBL(dest, GKA, sdata);
        BBA_AND(effect, BB_SKA(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //SHI
        effect = EFFECT_TBL(dest, GHI, sdata);
        BBA_AND(effect, BB_SHI(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //SUM
        effect = EFFECT_TBL(dest, GUM, sdata);
        BBA_AND(effect, BB_SUM(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //SRY
        effect = EFFECT_TBL(dest, GRY, sdata);
        BBA_AND(effect, BB_SRY(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

    }
    //先手番
    else
    {
        //GFU
        effect = EFFECT_TBL(dest, SFU, sdata);
        BBA_AND(effect, BB_GFU(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //GKY
        effect = EFFECT_TBL(dest, SKY, sdata);
        BBA_AND(effect, BB_GKY(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //GKE
        effect = EFFECT_TBL(dest, SKE, sdata);
        BBA_AND(effect, BB_GKE(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //GGI
        effect = EFFECT_TBL(dest, SGI, sdata);
        BBA_AND(effect, BB_GGI(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //GKI,GTO,GNY,GNK,GNG
        effect = EFFECT_TBL(dest, SKI, sdata);
        BBA_AND(effect, BB_GTK(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //GKA
        effect = EFFECT_TBL(dest, SKA, sdata);
        BBA_AND(effect, BB_GKA(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //GHI
        effect = EFFECT_TBL(dest, SHI, sdata);
        BBA_AND(effect, BB_GHI(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //GUM
        effect = EFFECT_TBL(dest, SUM, sdata);
        BBA_AND(effect, BB_GUM(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

        //GRY
        effect = EFFECT_TBL(dest, SUM, sdata);
        BBA_AND(effect, BB_GUM(sdata));
        if(src_check(sdata, effect, dest, tbase)) return true;

    }
    return false;
}
/*
bool hs_invalid_drops      (const sdata_t *sdata,
                            unsigned int   src,
                            unsigned int   dest,
                            tbase_t       *tbase )
//{return false;}

{
    komainf_t koma = S_BOARD(sdata, src);
    //龍の王手のみ
    //if(koma == SKA||koma==GKA) return false;
    //if(koma == SUM||koma==GUM) return false;
    //if(koma != SRY && koma!=GRY) return false;
    sdata_t sbuf, sbuf1;
    bool flag = false; //true 駒がdestの位置で成れる。
    memcpy(&sbuf, sdata, sizeof(sdata_t));
    //srcの位置の飛び駒を削除
    S_BOARD(&sbuf, src) = SPC;
    S_ZKEY(&sbuf) ^= g_zkey_seed[koma*N_SQUARE+src];  //zkey
    
    //komaがsrcからdestに移動した場合、成れるか？
    switch(koma){
        case GKY: if(GKY_PROMOTE(dest)     ) flag = true; break;
        case GKA: if(GKA_PROMOTE(src, dest)) flag = true; break;
        case GHI: if(GHI_PROMOTE(src, dest)) flag = true; break;
        case SKY: if(SKY_PROMOTE(dest)     ) flag = true; break;
        case SKA: if(SKA_PROMOTE(src, dest)) flag = true; break;
        case SHI: if(SHI_PROMOTE(src, dest)) flag = true; break;
        default: break;
    }
    if(flag) memcpy(&sbuf1, &sbuf, sizeof(sdata_t));
    
    turn_t tn = TURN_FLIP(S_TURN(&sbuf));
    //destの位置に移動(不成）
    S_BOARD(&sbuf, dest) = koma;
    S_ZKEY(&sbuf) ^= g_zkey_seed[koma*N_SQUARE+dest];  //zkey
    if(hs_tbase_lookup(&sbuf, tn, tbase)) {
        return true;
    }
    //destの位置に移動(成）
    if(flag)
    {
        koma += PROMOTED;
        S_BOARD(&sbuf1, dest) = koma;
        S_ZKEY(&sbuf1) ^= g_zkey_seed[koma*N_SQUARE+dest];  //zkey
        if(hs_tbase_lookup(&sbuf1, tn, tbase)){
            return true;
        }
    }
    return false;
}
*/

bool hs_invalid_drops      (const sdata_t *sdata,
                            unsigned int   src,
                            unsigned int   dest,
                            tbase_t       *tbase)
//{return false;}

{
    komainf_t koma = S_BOARD(sdata, src);
    bool flag = false; //true 駒がdestの位置で成れる。
    turn_t tn = TURN_FLIP(S_TURN(sdata));
    //komaがsrcからdestに移動した場合、成れるか？
    switch(koma){
        case GKY: if(GKY_PROMOTE(dest)     ) flag = true; break;
        case GKA: if(GKA_PROMOTE(src, dest)) flag = true; break;
        case GHI: if(GHI_PROMOTE(src, dest)) flag = true; break;
        case SKY: if(SKY_PROMOTE(dest)     ) flag = true; break;
        case SKA: if(SKA_PROMOTE(src, dest)) flag = true; break;
        case SHI: if(SHI_PROMOTE(src, dest)) flag = true; break;
        default: break;
    }
    sdata_t sbuf;
    //不成でdestへ移動
    memcpy(&sbuf, sdata, sizeof(sdata_t));
    sdata_tentative_move(&sbuf, src, dest, false);
    if(hs_tbase_lookup(&sbuf, tn, tbase)) {
        return true;
    }
    //成でdestへ移動
    if(flag){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_tentative_move(&sbuf, src, dest, true);
        if(S_ZKEY(&sbuf)==0xbbaf30394dcbb082){
            SDATA_PRINTF(&sbuf, PR_BOARD);
        }
        if(hs_tbase_lookup(&sbuf, tn, tbase)) {
            return true;
        }
    }
    return false;
}

/* -----------------------------------------------------
 _tbase_lookup
 [flag] false: 詰み発見前
        true : 詰み発見後 優越詰み  prを反映
                        詰み     prを反映
 ------------------------------------------------------ */
void _tbase_lookup        (const sdata_t   *sdata,
                           mvlist_t       *mvlist,
                           turn_t              tn,
                           bool              flag,
                           tbase_t         *tbase )
{
    uint64_t address = HASH_FUNC(S_ZKEY(sdata), tbase);
    zfolder_t *zfolder = *(tbase->table+address);
    //局面表に同一盤面があるか
    while(zfolder){
        if(zfolder->zkey == S_ZKEY(sdata)) break;
        zfolder = zfolder->next;
    }
    //無い場合、g_tdata_initを返す。
    if(!zfolder){
        memcpy(&(mvlist->tdata), &g_tdata_init, sizeof(tdata_t));
        return;
    }
    //同一盤面ありの場合、持ち駒が一致、優越、劣化している局面があるか
    mcard_t *mcard = zfolder->mcard;
    unsigned int cmp_res;
    //mkey_t mkey = tn?S_GMKEY(sdata):S_SMKEY(sdata);
    mkey_t mkey;
    tn ? MKEY_COPY(mkey, S_GMKEY(sdata)):MKEY_COPY(mkey, S_SMKEY(sdata));
    memset(g_mcard, 0, sizeof(mcard_t *)*N_MCARD_TYPE);
    uint16_t tmp_pn = 1, tmp_dn = 1;
    while(mcard){
        cmp_res = MKEY_COMPARE(mkey, mcard->mkey);
        if     (cmp_res == MKEY_SUPER){
            //詰み
            if     (!mcard->tlist->tdata.pn){
                g_mcard[SUPER_TSUMI] = mcard;
                break;
                /*
                if(!g_mcard[SUPER_TSUMI]) g_mcard[SUPER_TSUMI] = mcard;
                else{
                    unsigned int res =
                    MKEY_COMPARE(g_mcard[SUPER_TSUMI]->mkey, mcard->mkey);
                    if(res == MKEY_SUPER)
                        g_mcard[SUPER_TSUMI] = mcard;
                }
                */
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
                g_mcard[INFER_FUDUMI] = mcard;
                break;

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
                break;
            }
            //その他
            else                                {
                if(!g_mcard[EQUAL_UNKNOWN]) g_mcard[EQUAL_UNKNOWN] = mcard;
            }
        }
        mcard = mcard->next;
    }
    
    //mcardの優先順位を見てデータをリターン
    if     (g_mcard[SUPER_TSUMI])  {
        mcard = g_mcard[SUPER_TSUMI];
        memcpy(&(mvlist->tdata),&(mcard->tlist->tdata),sizeof(tdata_t));
        memcpy(&(mvlist->mkey),&(mcard->mkey),sizeof(mkey_t));
        mvlist->search = 1;
        mvlist->hinc = mcard->hinc;
        mvlist->inc  = 1;
        mvlist->nouse = mcard->nouse;
        mvlist->nouse2 =
        TOTAL_MKEY(mkey)-TOTAL_MKEY(mcard->mkey)+mvlist->nouse;
        if(flag){
            mvlist->cu = 0;
            mvlist->pr = mcard->protect;
        }
        return;
    }
    if     (g_mcard[INFER_FUDUMI]) {
        mcard = g_mcard[INFER_FUDUMI];
        memcpy(&(mvlist->tdata),&(mcard->tlist->tdata),sizeof(tdata_t));
        memcpy(&(mvlist->mkey),&(mcard->mkey),sizeof(mkey_t));
        mvlist->search = 1;
        mvlist->hinc = 0;
        mvlist->inc  = 0;
        return;
    }
    if     (g_mcard[EQUAL_TSUMI])  {
        mcard = g_mcard[EQUAL_TSUMI];
        memcpy(&(mvlist->tdata),&(mcard->tlist->tdata),sizeof(tdata_t));
        memcpy(&(mvlist->mkey),&(mcard->mkey),sizeof(mkey_t));
        mvlist->search = 1;
        mvlist->hinc = mcard->hinc;
        mvlist->inc  = mcard->hinc;
        mvlist->nouse = mcard->nouse;
        mvlist->nouse2 = mcard->nouse;
        if(flag){
            mvlist->cu = mcard->current;
            mvlist->pr = mcard->protect;
        }
        return;
    }
    if     (g_mcard[EQUAL_FUDUMI]) {
        mcard = g_mcard[EQUAL_FUDUMI];
        memcpy(&(mvlist->tdata),&(mcard->tlist->tdata),sizeof(tdata_t));
        memcpy(&(mvlist->mkey),&(mcard->mkey),sizeof(mkey_t));
        mvlist->search = 1;
        mvlist->hinc = 0;
        mvlist->inc  = 0;
        return;
    }
    if     (g_mcard[EQUAL_UNKNOWN]){
        mcard = g_mcard[EQUAL_UNKNOWN];
        //手順中に同一局面があるか
        if(mcard->current){
            mvlist->tdata.pn = MAX(1,mcard->cpn);
            mvlist->tdata.dn = 1;
            mvlist->tdata.sh = 0;
            mvlist->search = 1;
            return;
        }
        //同一深さのtlistがあるか
        bool hit = false;
        tlist_t *tlist = mcard->tlist;
        while(tlist){
            if(tlist->dp == S_COUNT(sdata)){
                memcpy(&(mvlist->tdata),&(tlist->tdata),sizeof(tdata_t));
                hit = true;
            }
            tmp_pn = MAX(tmp_pn, tlist->tdata.pn);
            tmp_dn = MAX(tmp_dn, tlist->tdata.dn);
            tlist = tlist->next;
        }
        if(hit) {
            mvlist->search = 1;
        }
        //同一深さのtlistがない場合、新規作成。
        else    {
            tlist_t *new_tlist = tlist_alloc(tbase);
            new_tlist->dp = S_COUNT(sdata);
            new_tlist->tdata.pn = tmp_pn;
            new_tlist->tdata.dn = tmp_dn;
            new_tlist->next = mcard->tlist;
            mcard->tlist = new_tlist;
            memcpy(&(mvlist->tdata), &(new_tlist->tdata), sizeof(tdata_t));
        }
        return;
    }
    else                           {
        tdata_t tdata = {tmp_pn, tmp_dn, 0};
        memcpy(&(mvlist->tdata), &tdata, sizeof(tdata_t));
        return;
    }
}

/* -------------------------------------------------------------------------
  _tbase_update
 [詰み、不詰みのupdate]
 1,sdataの持ち駒で検索         データ有り→データ削除。データが無ければスルー
 2,証明駒、反証駒で検索         データ有り→update 無ければ新設
 
 [その他のupdate]
 1,sdataの持ち駒で検索         データ有り→update 無ければ新設
 
 [flag] false: 詰み発見前
        true : 詰み発見後     詰み       データ保護
                            優越詰み    データ保護
 
 ------------------------------------------------------------------------- */
void _tbase_update        (const sdata_t   *sdata,
                           mvlist_t       *mvlist,
                           turn_t              tn,
                           bool              flag,
                           tbase_t         *tbase )
{
    //pn値がルートしきい値付近のデータは保存しない
    if(mvlist->tdata.pn>INFINATE-g_root_pn && mvlist->tdata.dn) return;
 
    //局面表のリソースが少ない場合、gcで確保する。
    if(!tbase->tstack || !tbase->tstack->next) tbase_gc(tbase);
   
    //登録データが詰み
    if     (!mvlist->tdata.pn)
        tsumi_update(sdata, mvlist, tn, flag, tbase);
    //登録データが不詰み
    else if(!mvlist->tdata.dn)
        fudumi_update(sdata, mvlist, tn, flag, tbase);
    //登録データが不明
    else
        fumei_update(sdata, mvlist, tn, flag, tbase);
    return;
}

bool is_move_possible(komainf_t koma, char dest)
{
    switch(koma){
        case SFU: if(dest>8)  return true; return false; break;
        case SKY: if(dest>8)  return true; return false; break;
        case SKE: if(dest>17) return true; return false; break;
        case GFU: if(dest<72) return true; return false; break;
        case GKY: if(dest<72) return true; return false; break;
        case GKE: if(dest<63) return true; return false; break;
        default: return true; break;
    }
}
bool _invalid_drops       (const sdata_t   *sdata,
                           unsigned int       src,
                           unsigned int      dest,
                           tbase_t         *tbase )
{
    //srcの駒をdestに移した局面を作る。
    sdata_t  sbuf;
    komainf_t koma = S_BOARD(sdata, src);
    //不成でkomaがdestに移動できる場合
    if(is_move_possible(koma,dest))
    {
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_tentative_move(&sbuf, src, dest, false);
        //turn_t tn = TURN_FLIP(S_TURN(&sbuf));
        //局面が詰みであればtrueを返す。
        if(S_NOUTE(&sbuf)){
            if(tsumi_check(&sbuf)) return true;
            /*
            else if(hs_tbase_lookup(&sbuf, tn, tbase)){
                g_invalid_drops = true;
                return true;
            }
             */
        }
    }

    //駒が成れる場合
    if(KOMA_PROMOTE(koma, src, dest)){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        sdata_tentative_move(&sbuf, src, dest, true);
        turn_t tn = TURN_FLIP(S_TURN(&sbuf));
        //局面が詰みであればtrueを返す。
        if(S_NOUTE(&sbuf)){
            if(tsumi_check(&sbuf)) return true;
            else if(hs_tbase_lookup(&sbuf, tn, tbase)){
                g_invalid_drops = true;
                return true;
            }
        }
    }
    return false;
}

void set_tpin             (const sdata_t *sdata,
                           char *tpin             )
{
    memset(st_tpin, 0, sizeof(char)*N_SQUARE);
    int ou = ENEMY_OU(sdata);
    if(ou == HAND) return;
    
    bitboard_t src_bb, k_eff, o_eff;
    int src, pin;
    //後手番
    if(S_TURN(sdata)){
        //GKY
        o_eff = EFFECT_TBL(ou, SKY, sdata);
        src_bb = BB_GKY(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            k_eff = EFFECT_TBL(src, GKY, sdata);
            BBA_AND(k_eff, o_eff);
            pin = min_pos(&k_eff);
            if(pin>=0 && SENTE_KOMA(S_BOARD(sdata, pin)))
            {
                *(tpin+pin) = g_file[pin]+10;
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GKA, GUM
        o_eff = EFFECT_TBL(ou, SKA, sdata);
        src_bb = BB_GUK(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            k_eff = EFFECT_TBL(src, GKA, sdata);
            BBA_AND(k_eff, o_eff);
            pin = min_pos(&k_eff);
            if(pin>=0 && SENTE_KOMA(S_BOARD(sdata, pin))){
                if(g_rslp[ou] == g_rslp[src])
                    *(tpin+pin) = g_rslp[ou]+30;
                else if(g_lslp[ou] == g_lslp[src])
                    *(tpin+pin) = g_lslp[ou]+17;
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GHI, GRY
        o_eff = EFFECT_TBL(ou, SHI, sdata);
        src_bb = BB_GRH(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            k_eff = EFFECT_TBL(src, GHI, sdata);
            BBA_AND(k_eff, o_eff);
            pin = min_pos(&k_eff);
            if(pin>=0 && SENTE_KOMA(S_BOARD(sdata, pin))){
                if(g_rank[ou] == g_rank[src])
                    *(tpin+pin) = g_rank[ou]+1;
                else if(g_file[ou] == g_file[src])
                    *(tpin+pin) = g_file[ou]+10;
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
    }
    //先手番
    else             {
        //SKY
        o_eff = EFFECT_TBL(ou, GKY, sdata);
        src_bb = BB_SKY(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            k_eff = EFFECT_TBL(src, SKY, sdata);
            BBA_AND(k_eff, o_eff);
            pin = min_pos(&k_eff);
            if(pin>=0 && GOTE_KOMA(S_BOARD(sdata, pin)))
            {
                *(tpin+pin) = g_file[pin]+10;
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SKA, SUM
        o_eff = EFFECT_TBL(ou, GKA, sdata);
        src_bb = BB_SUK(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            k_eff = EFFECT_TBL(src, SKA, sdata);
            BBA_AND(k_eff, o_eff);
            pin = min_pos(&k_eff);
            if(pin>=0 && GOTE_KOMA(S_BOARD(sdata, pin))){
                if(g_rslp[ou] == g_rslp[src])
                    *(tpin+pin) = g_rslp[ou]+30;
                else if(g_lslp[ou] == g_lslp[src])
                    *(tpin+pin) = g_lslp[ou]+17;
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SHI, SRY
        o_eff = EFFECT_TBL(ou, GHI, sdata);
        src_bb = BB_SRH(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            k_eff = EFFECT_TBL(src, SHI, sdata);
            BBA_AND(k_eff, o_eff);
            pin = min_pos(&k_eff);
            if(pin>=0 && GOTE_KOMA(S_BOARD(sdata, pin))){
                if(g_rank[ou] == g_rank[src])
                    *(tpin+pin) = g_rank[ou]+1;
                else if(g_file[ou] == g_file[src])
                    *(tpin+pin) = g_file[ou]+10;
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
    }
    return;
}

bool src_check            (const sdata_t  *sdata,
                           bitboard_t        eff,
                           unsigned int     dest,
                           tbase_t        *tbase )
{
    int src;
    bitboard_t effect = eff;
    while(1){
        src = min_pos(&effect);
        if(src<0) break;
        if(!st_tpin[src]){
            if(_invalid_drops(sdata, src, dest, tbase)) return true;
        }
        BBA_XOR(effect, g_bpos[src]);
    }
    return false;
}

void tsumi_update         (const sdata_t   *sdata,
                           mvlist_t       *mvlist,
                           turn_t              tn,
                           bool              flag,
                           tbase_t         *tbase )
{
    uint64_t address = HASH_FUNC(S_ZKEY(sdata), tbase);
    zfolder_t *zfolder = *(tbase->table+address);
    
    //局面表に同一盤面があるか？
    while(zfolder){
        if(zfolder->zkey == S_ZKEY(sdata)) break;
        zfolder = zfolder->next;
    }
    
    //zfolderが無い場合
    if(!zfolder){
        tlist_t *new_tlist = tlist_alloc(tbase);
        mcard_t *new_mcard = mcard_alloc(tbase);
        zfolder_t *new_zfolder = zfolder_alloc(tbase);
        //新規tlist作成
        new_tlist->dp = ALL_DEPTH;
        memcpy(&(new_tlist->tdata), &(mvlist->tdata), sizeof(tdata_t));
        //新規mcard作成(mkeyは証明駒)
        //new_mcard->mkey = mvlist->mkey;
        new_mcard->tlist = new_tlist;
        memcpy(&(new_mcard->mkey),&(mvlist->mkey),sizeof(mkey_t));
        //memcpy(&(new_mcard->tlist),&new_tlist,sizeof(tlist_t));
        new_mcard->hinc = mvlist->hinc;
        new_mcard->nouse = mvlist->nouse;
        if(flag){
            new_mcard->protect = 1;
            (tbase->pr_num)++;
        }
        //新規zfolder作成
        new_zfolder->zkey = S_ZKEY(sdata);
        new_zfolder->mcard = new_mcard;
        new_zfolder->next = *(tbase->table+address);
        *(tbase->table+address) = new_zfolder;
        return;
    }
    
    //zfolderがある場合
    mcard_t *mcard = zfolder->mcard, *prev = NULL;
    //mkey_t mkey = tn?S_GMKEY(sdata):S_SMKEY(sdata);
    mkey_t mkey;
    tn ? MKEY_COPY(mkey, S_GMKEY(sdata)):MKEY_COPY(mkey, S_SMKEY(sdata));
    while(mcard){
        if(!memcmp(&mkey,&(mcard->mkey),sizeof(mkey_t))) break;
        prev = mcard;
        mcard = mcard->next;
    }
    if(mcard){
        if(!prev){ zfolder->mcard = mcard->next; }
        else     { prev->next = mcard->next;     }
        //mcardを削除
        tlist_free(mcard->tlist, tbase);
        mcard->tlist = NULL;
        mcard->next = tbase->mstack;
        tbase->mstack = mcard;
    }
    
    //証明駒の登録
    memcpy(&mkey,&(mvlist->mkey),sizeof(mkey_t));
    unsigned int cmp_res;
    mcard = zfolder->mcard;
    prev  = NULL;
    bool update = false;
    while(mcard){
        cmp_res = MKEY_COMPARE(mkey, mcard->mkey);
        if     (cmp_res == MKEY_SUPER){
            prev = mcard;
            mcard = mcard->next;
        }
        else if(cmp_res == MKEY_INFER){
            if(!prev)zfolder->mcard = mcard->next;
            else prev->next = mcard->next;
            mcard_t *mc = mcard;
            mcard = mcard->next;
            
            //mc削除
            tlist_free(mc->tlist, tbase);
            mc->tlist = NULL;
            mc->next = tbase->mstack;
            tbase->mstack = mc;
        }
        else if(cmp_res == MKEY_EQUAL){
            //詰み
            if     (!mcard->tlist->tdata.pn){
                if(  mcard->tlist->tdata.sh>mvlist->tdata.sh)
                {
                    mcard->tlist->tdata.sh = mvlist->tdata.sh;
                    mcard->hinc = mvlist->hinc;
                    mcard->nouse = mvlist->nouse;
                    if(flag && !mcard->protect){
                        mcard->protect = 1;
                        (tbase->pr_num)++;
                    }
                }
                else if(mcard->hinc!=mvlist->hinc)
                {
                    mcard->tlist->tdata.sh = mvlist->tdata.sh;
                    mcard->hinc = mvlist->hinc;
                    mcard->nouse = mvlist->nouse;
                    if(flag && !mcard->protect){
                        mcard->protect = 1;
                        (tbase->pr_num)++;
                    }
                }
                else
                {
                    if(flag && !mcard->protect){
                        mcard->protect = 1;
                        (tbase->pr_num)++;
                    }
                }
                update = true;
            }
            //不詰み
            else if(!mcard->tlist->tdata.dn){
                //あり得ないのでとバグ取りのため、中断する。
                printf("update error %s: %d\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            //その他
            else                            {
                tlist_t *tlist = mcard->tlist;
                tlist->dp = ALL_DEPTH;
                memcpy(&(tlist->tdata),&(mvlist->tdata),sizeof(tdata_t));
                if(tlist->next){
                    tlist_free(tlist->next, tbase);
                    tlist->next = NULL;
                }
                mcard->hinc = mvlist->hinc;
                mcard->nouse = mvlist->nouse;
                if(flag){
                    mcard->protect = 1;
                    (tbase->pr_num)++;
                }
                update = true;
            }
            if(mcard->hinc && !mcard->nouse){
                //あり得ないのでとバグ取りのため、中断する。
                printf("update error %s: %d\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            prev = mcard;
            mcard = mcard->next;
        }
        else{
            prev = mcard;
            mcard = mcard->next;
        }
    }
    if(!update){
        tlist_t *new_tlist = tlist_alloc(tbase);
        mcard_t *new_mcard = mcard_alloc(tbase);
        new_tlist->dp = ALL_DEPTH;
        memcpy(&(new_tlist->tdata),&(mvlist->tdata),sizeof(tdata_t));
        memcpy(&(new_mcard->mkey),&(mvlist->mkey),sizeof(mkey_t));
        new_mcard->hinc = mvlist->hinc;
        new_mcard->nouse = mvlist->nouse;
        if(flag){
            new_mcard->protect = 1;
            (tbase->pr_num)++;
        }
        new_mcard->tlist = new_tlist;
        new_mcard->next = zfolder->mcard;
        zfolder->mcard = new_mcard;
    }
    return;
}
void fudumi_update        (const sdata_t   *sdata,
                           mvlist_t       *mvlist,
                           turn_t              tn,
                           bool              flag,
                           tbase_t         *tbase )
{
    uint64_t address = HASH_FUNC(S_ZKEY(sdata), tbase);
    zfolder_t *zfolder = *(tbase->table+address);
    
    //局面表に同一盤面があるか？
    while(zfolder){
        if(zfolder->zkey == S_ZKEY(sdata)) break;
        zfolder = zfolder->next;
    }
    
    //zfolderが無い場合
    if(!zfolder){
        tlist_t *new_tlist = tlist_alloc(tbase);
        mcard_t *new_mcard = mcard_alloc(tbase);
        zfolder_t *new_zfolder = zfolder_alloc(tbase);
        //新規tlist作成
        new_tlist->dp = ALL_DEPTH;
        memcpy(&(new_tlist->tdata), &(mvlist->tdata), sizeof(tdata_t));
        //新規mcard作成(mkeyは反証駒)
        //new_mcard->mkey = mvlist->mkey;
        memcpy(&(new_mcard->mkey),&(mvlist->mkey),sizeof(mkey_t));
        new_mcard->tlist = new_tlist;
        //新規zfolder作成
        new_zfolder->zkey = S_ZKEY(sdata);
        new_zfolder->mcard = new_mcard;
        new_zfolder->next = *(tbase->table+address);
        *(tbase->table+address) = new_zfolder;
        return;
    }
    
    //zfolderがある場合
    mcard_t *mcard = zfolder->mcard, *prev = NULL;
    //mkey_t mkey = tn?S_GMKEY(sdata):S_SMKEY(sdata);
    mkey_t mkey;
    tn ? MKEY_COPY(mkey, S_GMKEY(sdata)):MKEY_COPY(mkey, S_SMKEY(sdata));
    while(mcard){
        if(!memcmp(&mkey,&(mcard->mkey),sizeof(mkey_t))) break;
        prev = mcard;
        mcard = mcard->next;
    }
    if(mcard){
        if(!prev){ zfolder->mcard = mcard->next; }
        else     { prev->next = mcard->next;     }
        //mcardを削除
        tlist_free(mcard->tlist, tbase);
        mcard->tlist = NULL;
        mcard->next = tbase->mstack;
        tbase->mstack = mcard;
    }
    
    //反証駒の登録
    memcpy(&mkey,&(mvlist->mkey),sizeof(mkey_t));
    unsigned int cmp_res;
    mcard = zfolder->mcard;
    prev  = NULL;
    bool update = false;
    while(mcard){
        cmp_res = MKEY_COMPARE(mkey, mcard->mkey);
        if     (cmp_res == MKEY_SUPER){
            if(!prev)zfolder->mcard = mcard->next;
            else prev->next = mcard->next;
            mcard_t *mc = mcard;
            mcard = mcard->next;
            
            //mc削除
            tlist_free(mc->tlist, tbase);
            mc->tlist = NULL;
            mc->next = tbase->mstack;
            tbase->mstack = mc;
        }
        else if(cmp_res == MKEY_INFER){
            prev = mcard;
            mcard = mcard->next;
        }
        else if(cmp_res == MKEY_EQUAL){
            //詰み
            if     (!mcard->tlist->tdata.pn){
                //あり得ないのとバグ取りのため、中断する。
                SDATA_PRINTF(sdata, PR_BOARD);
                printf("update error %s: %d\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            //不詰
            else if(!mcard->tlist->tdata.dn){
                update = true;
            }
            //その他
            else{
                tlist_t *tlist = mcard->tlist;
                tlist->dp = ALL_DEPTH;
                memcpy(&(tlist->tdata),&(mvlist->tdata),sizeof(tdata_t));
                if(tlist->next){
                    tlist_free(tlist->next, tbase);
                    tlist->next = NULL;
                }
                update = true;
            }
            prev = mcard;
            mcard = mcard->next;
        }
        else{
            prev = mcard;
            mcard = mcard->next;
        }
    }
    //既存持駒データなく、新規作成
    if(!update){
        tlist_t *new_tlist = tlist_alloc(tbase);
        mcard_t *new_mcard = mcard_alloc(tbase);
        new_tlist->dp = ALL_DEPTH;
        memcpy(&(new_tlist->tdata),&(mvlist->tdata),sizeof(tdata_t));
        memcpy(&(new_mcard->mkey),&(mvlist->mkey),sizeof(mkey_t));
        new_mcard->tlist = new_tlist;
        new_mcard->next = zfolder->mcard;
        zfolder->mcard = new_mcard;
    }
    return;
}
void fumei_update         (const sdata_t   *sdata,
                           mvlist_t       *mvlist,
                           turn_t              tn,
                           bool              flag,
                           tbase_t         *tbase )
{
    uint64_t address = HASH_FUNC(S_ZKEY(sdata), tbase);
    zfolder_t *zfolder = *(tbase->table+address);
    
    //局面表に同一盤面があるか？
    while(zfolder){
        if(zfolder->zkey == S_ZKEY(sdata)) break;
        zfolder = zfolder->next;
    }
    
    //zfolderが無い場合
    if(!zfolder){
        tlist_t *new_tlist = tlist_alloc(tbase);
        mcard_t *new_mcard = mcard_alloc(tbase);
        zfolder_t *new_zfolder = zfolder_alloc(tbase);
        //新規tlist作成
        new_tlist->dp = S_COUNT(sdata);
        memcpy(&(new_tlist->tdata), &(mvlist->tdata), sizeof(tdata_t));
        //新規mcard作成
        new_mcard->mkey = tn?S_GMKEY(sdata):S_SMKEY(sdata);
        new_mcard->tlist = new_tlist;
        //新規zfolder作成
        new_zfolder->zkey = S_ZKEY(sdata);
        new_zfolder->mcard = new_mcard;
        new_zfolder->next = *(tbase->table+address);
        *(tbase->table+address) = new_zfolder;
        return;
    }
    
    //folderがある場合
    mcard_t *mcard = zfolder->mcard;
    unsigned int cmp_res;
    memset(g_mcard, 0, sizeof(mcard_t *)*N_MCARD_TYPE);
    mkey_t mkey;
    tn ? MKEY_COPY(mkey, S_GMKEY(sdata)):MKEY_COPY(mkey, S_SMKEY(sdata));
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

            }
        }
        else if(cmp_res == MKEY_EQUAL){
            //詰み
            if     (!mcard->tlist->tdata.pn){
                if(!g_mcard[EQUAL_TSUMI]) g_mcard[EQUAL_TSUMI] = mcard;
            }
            //不詰み
            else if(!mcard->tlist->tdata.dn){
                if(!g_mcard[EQUAL_FUDUMI]) g_mcard[EQUAL_FUDUMI] = mcard;
            }
            //その他
            else                            {
                if(!g_mcard[EQUAL_UNKNOWN]) g_mcard[EQUAL_UNKNOWN] = mcard;
            }
        }
        
        mcard = mcard->next;
    }

    //既存データを確認
    if     (g_mcard[SUPER_TSUMI]  ){
        //この場合、局面表を元にmvlistのデータを更新する。
#if DEBUG
        printf("SUPER TSUMI data exist.\n");
        SDATA_PRINTF(sdata, PR_BOARD|PR_ZKEY);
#endif /* DEBUG */
        memcpy(&(mvlist->tdata),&(g_mcard[SUPER_TSUMI]->tlist->tdata),
               sizeof(tdata_t));
        memcpy(&(mvlist->mkey),&(g_mcard[SUPER_TSUMI]->mkey),
               sizeof(mkey_t));
        return;
    }
    
    if(g_mcard[INFER_FUDUMI] ){
        //この場合、局面表を元にmvlistのデータを更新する。
#if DEBUG
        printf("INFER FUDUMI data exist.\n");
        SDATA_PRINTF(sdata, PR_BOARD|PR_ZKEY);
#endif /* DEBUG */
        
        memcpy(&(mvlist->tdata),&(g_mcard[INFER_FUDUMI]->tlist->tdata),
               sizeof(tdata_t));
        memcpy(&(mvlist->mkey),&(g_mcard[INFER_FUDUMI]->mkey),
               sizeof(mkey_t));
        return;
    }
    if(g_mcard[EQUAL_TSUMI]  ){
#if DEBUG
        printf("EQUAL TSUMI data exist.\n");
        SDATA_PRINTF(sdata, PR_BOARD|PR_ZKEY);
#endif /* DEBUG */
        //この場合、局面表を元にmvlistのデータを更新する。
        memcpy(&(mvlist->tdata),&(g_mcard[EQUAL_TSUMI]->tlist->tdata),
               sizeof(tdata_t));
        memcpy(&(mvlist->mkey),&(g_mcard[EQUAL_TSUMI]->mkey),
               sizeof(mkey_t));
        return;
    }
    if(g_mcard[EQUAL_FUDUMI] ){
        //この場合、局面表を元にmvlistのデータを更新する。
        
        memcpy(&(mvlist->tdata),&(g_mcard[EQUAL_FUDUMI]->tlist->tdata),
               sizeof(tdata_t));
        memcpy(&(mvlist->mkey),&(g_mcard[EQUAL_FUDUMI]->mkey),
               sizeof(mkey_t));
         
#if DEBUG
        printf("EQUAL_FUDUMI data exist.\n");
        SDATA_PRINTF(sdata, PR_BOARD|PR_ZKEY);
#endif /* DEBUG */
        return;
    }
    if(g_mcard[EQUAL_UNKNOWN]){
        tlist_t *tlist = g_mcard[EQUAL_UNKNOWN]->tlist;
        tlist_t *tmp = NULL;
        while(tlist){
            if(tlist->dp == S_COUNT(sdata)) tmp = tlist;
            else tlist->tdata.pn = mvlist->tdata.pn;
            tlist = tlist->next;
        }
        //同一深さのtlistが無ければ新設する
        if(!tmp){
            tlist_t *new_tlist = tlist_alloc(tbase);
            new_tlist->dp = S_COUNT(sdata);
            memcpy(&(new_tlist->tdata),&(mvlist->tdata),sizeof(tdata_t));
            new_tlist->next = g_mcard[EQUAL_UNKNOWN]->tlist;
            g_mcard[EQUAL_UNKNOWN]->tlist = new_tlist;
        }
        //同一深さのtlistが有れば更新
        else      {
            memcpy(&(tmp->tdata),&(mvlist->tdata),sizeof(tdata_t));
        }
    }
    else{
        tlist_t *new_tlist = tlist_alloc(tbase);
        mcard_t *new_mcard = mcard_alloc(tbase);
        new_tlist->dp = S_COUNT(sdata);
        memcpy(&(new_tlist->tdata),&(mvlist->tdata),sizeof(tdata_t));
        new_mcard->tlist = new_tlist;
        memcpy(&(new_mcard->mkey),&mkey,sizeof(mkey_t));
        new_mcard->next = zfolder->mcard;
        zfolder->mcard = new_mcard;
    }
    return;
}

/*
 bmake_treeおよびtsearchpv専用の千日手検出用table
 */

mtt_t *create_mtt   (uint32_t  base_size)
{
    mtt_t *mtt = (mtt_t *)calloc(1, sizeof(mtt_t));
    if(!mtt) return NULL;
    mtt->table = (zfr_t **)calloc(base_size, sizeof(zfr_t *));
    if(!mtt->table){
        free(mtt);
        return NULL;
    }
    mtt->zstack = (zfr_t *)calloc(base_size, sizeof(zfr_t));
    if(!mtt->zstack){
        free(mtt->table);
        free(mtt);
        return NULL;
    }
    mtt->mstack = (mcd_t *)calloc(base_size, sizeof(mcd_t));
    if(!mtt->mstack){
        free(mtt->zstack);
        free(mtt->table);
        free(mtt);
        return NULL;
    }
    mtt->zpool = mtt->zstack;
    mtt->mpool = mtt->mstack;
    uint32_t i=0;
    zfr_t *zfr = mtt->zstack;
    mcd_t *mcd = mtt->mstack;
    while(i<base_size-1){
        zfr->next = zfr+1;
        mcd->next = mcd+1;
        zfr = zfr->next;
        mcd = mcd->next;
        i++;
    }
    mtt->size = base_size;
    return mtt;
}

void  init_mtt      (mtt_t     *mtt)
{
    memset(mtt->table, 0, sizeof(zfr_t *)*mtt->size);
    memset(mtt->zpool, 0, sizeof(zfr_t)*mtt->size);
    memset(mtt->mpool, 0, sizeof(mcd_t)*mtt->size);
    
    mtt->zstack = mtt->zpool;
    mtt->mstack = mtt->mpool;
    
    uint32_t i=0;
    zfr_t *zfr = mtt->zstack;
    mcd_t *mcd = mtt->mstack;
    while(i<mtt->size-1){
        zfr->next = zfr+1;
        mcd->next = mcd+1;
        zfr = zfr->next;
        mcd = mcd->next;
        i++;
    }
    mtt->num = 0;
    return;
}

void  destroy_mtt   (mtt_t     *mtt)
{
    free(mtt->zpool);
    free(mtt->mpool);
    free(mtt->table);
    free(mtt);
    return;
}

mcd_t *mcd_alloc    (mtt_t     *mtt)
{
    mcd_t *mcd = mtt->mstack;
    mtt->mstack = mcd->next;
    memset(mcd, 0, sizeof(mcd_t));
    return mcd;
}

zfr_t *zfr_alloc    (mtt_t     *mtt)
{
    zfr_t *zfr = mtt->zstack;
    mtt->zstack = zfr->next;
    memset(zfr, 0, sizeof(zfr_t));
    return zfr;
}

bool  mtt_lookup    (const sdata_t *sdata,
                     turn_t  tn,
                     mtt_t   *mtt)
{
    uint32_t address = S_ZKEY(sdata)%mtt->size;
    zfr_t *zfr = *(mtt->table+address);
    //局面表に同一盤面はあるか
    while(zfr){
        if(zfr->zkey == S_ZKEY(sdata)) break;
        zfr = zfr->next;
    }
    if(!zfr) return false;
    
    mkey_t mkey = tn?S_GMKEY(sdata):S_SMKEY(sdata);
    mcd_t *mcd = zfr->mcd;
    uint32_t cmp_res;
    while(mcd){
        cmp_res = MKEY_COMPARE(mkey,mcd->mkey);
        if(cmp_res == MKEY_EQUAL) return true;
        mcd = mcd->next;
    }
    return false;
}
void  mtt_setup     (const sdata_t *sdata,
                     turn_t  tn,
                     mtt_t   *mtt)
{
    uint32_t address = S_ZKEY(sdata)%mtt->size;
    zfr_t *zfr = *(mtt->table+address);
    //局面表に同一盤面はあるか
    while(zfr){
        if(zfr->zkey == S_ZKEY(sdata)) break;
        zfr = zfr->next;
    }
    //zfrがない場合
    if(!zfr){
        mcd_t *new_mcd = mcd_alloc(mtt);
        zfr_t *new_zfr = zfr_alloc(mtt);
        //新規mcard作成
        new_mcd->mkey = tn?S_GMKEY(sdata):S_SMKEY(sdata);
        //新規zfr作成
        new_zfr->zkey = S_ZKEY(sdata);
        new_zfr->mcd = new_mcd;
        new_zfr->next = *(mtt->table+address);
        *(mtt->table+address) = new_zfr;
        
        mtt->num++;
        return;
    }
    mkey_t mkey = tn?S_GMKEY(sdata):S_SMKEY(sdata);
    mcd_t *mcd = zfr->mcd;
    uint32_t cmp_res;
    while(mcd){
        cmp_res = MKEY_COMPARE(mkey,mcd->mkey);
        if(cmp_res == MKEY_EQUAL) return;
        mcd = mcd->next;
    }
    mcd_t *new_mcd = mcd_alloc(mtt);
    new_mcd->mkey = mkey;
    new_mcd->next = zfr->mcd;
    zfr->mcd = new_mcd;
    mtt->num++;
    return;
}
void    mtt_reset   (const sdata_t *sdata,
                     turn_t  tn,
                     mtt_t   *mtt)
{
    uint32_t address = S_ZKEY(sdata)%mtt->size;
    zfr_t *zfr = *(mtt->table+address);
    zfr_t *zprev = NULL;
    //局面表に同一盤面はあるか
    while(zfr){
        if(zfr->zkey == S_ZKEY(sdata)) break;
        zprev = zfr;
        zfr = zfr->next;
    }
    if(!zfr) return;
    mkey_t mkey = tn?S_GMKEY(sdata):S_SMKEY(sdata);
    mcd_t *mcd = zfr->mcd;
    mcd_t *prev = NULL;
    uint32_t cmp_res;
    while(mcd){
        cmp_res = MKEY_COMPARE(mkey,mcd->mkey);
        if(cmp_res == MKEY_EQUAL)
        {
            if(prev){
                prev->next = mcd->next;
                mcd->next = mtt->mstack;
                mtt->mstack = mcd;
                mtt->num--;
                return;
            }
            else if(mcd->next){
                zfr->mcd = mcd->next;
                mcd->next = mtt->mstack;
                mtt->mstack = mcd;
                mtt->num--;
                return;
            }
            //mcdがなくなった場合、zfrを削除
            else{
                mcd->next = mtt->mstack;
                mtt->mstack = mcd;
                mtt->num--;
                if(zprev){
                    zprev->next = zfr->next;
                    zfr->next = mtt->zstack;
                    mtt->zstack = zfr;
                    return;
                }
                else{
                    *(mtt->table+address) = zfr->next;
                    zfr->next = mtt->zstack;
                    mtt->zstack = zfr;
                    return;
                }
            }
        }
        prev = mcd;
        mcd = mcd->next;
    }
}

/* ---------------------------------------------------
 盤面上のsrcの位置の駒をdestの位置に変更した局面データを生成する
 （無駄合判定用)
 --------------------------------------------------- */
void        sdata_tentative_move (sdata_t *sdata,
                                  char src,
                                  char dest,
                                  bool promote     )
{
    komainf_t koma;
    
    /* board,zkey,mkey,occupied, bb_koma, fflag, ou */
    //移動元(src)の駒を削除
    koma = S_BOARD(sdata, src);
    S_BOARD(sdata, src) = SPC;
    S_ZKEY(sdata) ^= g_zkey_seed[koma*N_SQUARE+src];
    if(koma == SFU)
        S_FFLAG(sdata) =
        FLAG_UNSET(S_FFLAG(sdata), g_file[src]);
    else if(koma == GFU)
        S_FFLAG(sdata) =
        FLAG_UNSET(S_FFLAG(sdata), g_file[src]+9);
    S_KSCORE(sdata) -= g_koma_val[koma];
    
    //ビットボード処理
    SDATA_OCC_XOR(sdata, src);
    if(koma==SOU||koma==GOU) ;
    else sdata_bb_xor(sdata, koma, src);
    S_TURN(sdata)?
    (BBA_XOR(BB_SOC(sdata), g_bpos[src])):
    (BBA_XOR(BB_GOC(sdata), g_bpos[src]));
    
    //移動先に駒を置く
    if(promote) koma += PROMOTED;
    S_BOARD(sdata, dest) = koma;
    S_ZKEY(sdata) ^= g_zkey_seed[koma*N_SQUARE+dest];
    if(koma == SFU)
        S_FFLAG(sdata) = FLAG_SET(S_FFLAG(sdata), g_file[dest]);
    else if(koma == GFU)
        S_FFLAG(sdata) = FLAG_SET(S_FFLAG(sdata), g_file[dest]+9);
    S_KSCORE(sdata) += g_koma_val[koma];
    
    //ビットボード処理
    SDATA_OCC_XOR(sdata, dest);
    sdata_bb_xor(sdata, koma, dest);
    S_TURN(sdata)?
    (BBA_XOR(BB_SOC(sdata), g_bpos[dest])):
    (BBA_XOR(BB_GOC(sdata), g_bpos[dest]));
    
    /* pinned  effect */
    create_effect(sdata);
    create_pin(sdata);
    if(S_TURN(sdata)){
        if(BPOS_TEST(SEFFECT(sdata), S_GOU(sdata)))
            S_NOUTE(sdata) = oute_check(sdata);
        else {
            S_NOUTE(sdata) = 0;
        }
    }
    else{
        if(BPOS_TEST(GEFFECT(sdata), S_SOU(sdata)))
            S_NOUTE(sdata) = oute_check(sdata);
        else {
            S_NOUTE(sdata) = 0;
        }
    }
    return;
}
