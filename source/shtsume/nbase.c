//
//  nbase.c
//  shtsume
//
//  Created by Hkijin on 2022/02/03.
//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "dtools.h"
#include "shtsume.h"
#include "ndtools.h"

tdata_t g_tdata_init    = {1,1,0};
tdata_t g_tdata_tsumi   = {0,INFINATE, 0};
tdata_t g_tdata_fudumi  = {INFINATE,0,0};

void print_tdata(tdata_t *tdata)
{
    printf("(%u %u %u) ",tdata->pn, tdata->dn, tdata->sh);
    return;
}

void print_nsearch_log(nsearchlog_t *log)
{
    return;
}

/* --------------------------------------------------------------------------
 本プログラムで使用する探索アルゴリズムでは王手回避着手を以下のような２重リスト表現します
 []　（玉の移動）
 []　（王手している駒を取る）
 []->[]->[]　（王手している駒をと金でとる)
 []　（移動合い)
 []->[]->[]->[]->[]　（持ち駒使用の合い駒）
 ------------------------------------------------------------------------- */
//着手配列を扱う構造体１
//用途上、メモリの確保と開放が煩雑になるので再利用のため一時保管場所を作っておく
static mlist_t *st_mlist_stack;

//メモリの確保（新規確保か再利用）
mlist_t* mlist_alloc(void)
{
    if(!st_mlist_stack) return (mlist_t *)malloc(sizeof(mlist_t));
    mlist_t *mlist = st_mlist_stack;
    st_mlist_stack = st_mlist_stack->next;
    return mlist;
}

//メモリの開放（一時保管場所に移動）
void mlist_free(mlist_t *mlist)
{
    if(!mlist) return;
    mlist_t *last = mlist;
    while(last) {
        if(!last->next) break;
        last = last->next;
    }
    last->next = st_mlist_stack;
    st_mlist_stack = mlist;
    return;
}

//メモリの開放（一時保管場所の開放）
void mlist_free_stack(void)
{
    mlist_t *mlist;
    while(st_mlist_stack){
        mlist = st_mlist_stack;
        st_mlist_stack = st_mlist_stack->next;
        free(mlist);
    }
    return;
}

//リスト先頭への着手追加
mlist_t* mlist_add(mlist_t *mlist, move_t move)
{
    mlist_t *newlist = mlist_alloc();
    newlist->next = mlist;
    memcpy(&(newlist->move), &move, sizeof(move_t));
    return newlist;
}

//リストの最後尾
mlist_t* mlist_last(mlist_t *mlist)
{
    mlist_t *tmp = mlist;
    if(!tmp) return NULL;
    while(tmp->next) tmp = tmp->next;
    return tmp;
}

//リストの結合
mlist_t* mlist_concat(mlist_t *alist, mlist_t *blist)
{
    if(!alist) return blist;
    mlist_t *tmp = mlist_last(alist);
    tmp->next = blist;
    return alist;
}

//リスト順序の反転
mlist_t* mlist_reverse(mlist_t *mlist)
{
    mlist_t *list = mlist, *buf, *newlist = NULL;
    while(list){
        buf = list;
        list = list->next;
        buf->next = newlist;
        newlist = buf;
    }
    return newlist;
}

//着手配列を扱う構造体２
//用途上、メモリの確保と開放が煩雑になるので再利用のため一時保管場所を作っておく
static mvlist_t *st_mvlist_stack;

//メモリの確保（新規確保か再利用）
mvlist_t* mvlist_alloc(void)
{
    if(!st_mvlist_stack) return (mvlist_t *)calloc(1, sizeof(mvlist_t));
    mvlist_t *mvlist = st_mvlist_stack;
    st_mvlist_stack = st_mvlist_stack->next;
    memset(mvlist, 0, sizeof(mvlist_t));
    return mvlist;
}

//メモリの開放（一時保管場所に移動）
void mvlist_free(mvlist_t *mvlist)
{
    if(!mvlist) return;
    mvlist_t *last = mvlist;
    while(last){
        if(last->mlist)mlist_free(last->mlist);
        if(!last->next) break;
        last = last->next;
    }
    last->next = st_mvlist_stack;
    st_mvlist_stack = mvlist;
    return;
}

//メモリの開放（一時保管場所の開放）
void mvlist_free_stack(void)
{
    mvlist_t *mvlist;
    while(st_mvlist_stack){
        mvlist = st_mvlist_stack;
        st_mvlist_stack = st_mvlist_stack->next;
        free(mvlist);
    }
    return;
}

//リスト先頭への着手追加
mvlist_t* mvlist_add  (mvlist_t  *mvlist,
                       mlist_t   *mlist  )
{
    mvlist_t *newlist = mvlist_alloc();
    newlist->next = mvlist;
    newlist->mlist = mlist;
    return newlist;
}

//リストの最後尾
mvlist_t* mvlist_last (mvlist_t  *mvlist )
{
    mvlist_t *last = mvlist;
    if(!last) return NULL;
    while(last->next) last = last->next;
    return last;
}

//リストの並べ替え(qsort)
 static mvlist_t*
 _mvlist_sort_merge(mvlist_t *mv1,
                    mvlist_t *mv2,
                    const sdata_t *sdata,
                    int (*compfunc)
                        (const mvlist_t *a,
                         const mvlist_t *b,
                         const sdata_t  *s ))
 {
    mvlist_t list, *mv;
    int cmp;
 
    mv = &list;
    while(mv1 && mv2){
        cmp = compfunc(mv1, mv2, sdata);
        if(cmp <= 0){
            mv = mv->next = mv1;
            mv1 = mv1->next;
        } else {
            mv = mv->next = mv2;
            mv2 = mv2->next;
        }
    }
    mv->next = mv1 ? mv1 : mv2;
    return list.next;
 }
 
 static mvlist_t*
 _mvlist_sort_real(mvlist_t *list,
                   const sdata_t *sdata,
                   int (*compfunc)
                       (const mvlist_t *a,
                        const mvlist_t *b,
                        const sdata_t  *s))
 {
     if(!list) return NULL;
     if(!list->next) return list;
     mvlist_t *mv1 = list, *mv2 = list->next;
     while ((mv2 = mv2->next)!= NULL) {
         if((mv2 = mv2->next) == NULL) break;
         mv1 = mv1->next;
     }
     mv2 = mv1->next;
     mv1->next = NULL;
     return _mvlist_sort_merge(_mvlist_sort_real(list, sdata, compfunc),
                               _mvlist_sort_real(mv2,  sdata, compfunc),
                               sdata,
                               compfunc);
 }

mvlist_t* sdata_mvlist_sort (mvlist_t *mvlist,
                             const sdata_t *sdata,
                             int (*compfunc)
                                 (const mvlist_t *a,
                                  const mvlist_t *b,
                                  const sdata_t  *s))
{
    return _mvlist_sort_real(mvlist, sdata, compfunc);
}

/* ----------------------------------------
 　先頭以外が整列されているmvlistにおいて、
 　先頭mvlistを２番目以降の適切な場所に再配置する
 ----------------------------------------- */

mvlist_t* sdata_mvlist_reorder(mvlist_t *mvlist,
                               const sdata_t *sdata,
                               int  (*compfunc)
                                    (const mvlist_t *a,
                                     const mvlist_t *b,
                                     const sdata_t  *s))
{
    if(!mvlist->next) return mvlist;
    //先頭と２番目のデータを比較する
    int cmp = compfunc(mvlist, mvlist->next, sdata);
    if(cmp <= 0) return mvlist;
    //先頭データを切り離す。
    mvlist_t *mv = mvlist;
    mvlist_t *new_mvlist = mvlist->next;
    //先頭データを３番目以降のデータと比較する。
    mvlist_t *mv1 = new_mvlist,*mv2 = mv1->next;
    while(true){
        if(!mv2){
            mv1->next = mv;
            mv->next = NULL;
            break;
        }
        cmp = compfunc(mv, mv2, sdata);
        if(cmp <= 0){
            mv1->next = mv;
            mv->next = mv2;
            break;
        }
        mv1 = mv2;
        mv2 = mv2->next;
    }
    return new_mvlist;
}

//リストのn番目のアイテム
mvlist_t* mvlist_nth  (mvlist_t  *mvlist, unsigned int n)
{
    mvlist_t *list = mvlist;
    unsigned int i = n;
    while(i>0){
        list = list->next;
        i--;
    }
    return list;
}

//リスト長
unsigned int mvlist_length(mvlist_t *mvlist)
{
    unsigned int length = 0;
    mvlist_t *list = mvlist;
    while(list){
        length++;
        list = list->next;
    }
    return length;
}

//候補手の並べ替え用（通常版)
//詰方着手の並べ替え
int proof_number_comp     (const mvlist_t *a,
                           const mvlist_t *b,
                           const sdata_t  *s)
{
    //pnの小さい着手を優先
    if(a->tdata.pn < b->tdata.pn) return -1;
    if(a->tdata.pn > b->tdata.pn) return  1;
    //dnの小さい着手を優先
    if(a->tdata.dn < b->tdata.dn) return -1;
    if(a->tdata.dn > b->tdata.dn) return  1;
    //王手千日手は回避
    if(a->cu < b->cu) return -1;
    if(a->cu > b->cu) return  1;
    //詰んでいる着手を優先
    if (!a->tdata.pn && !a->tdata.sh) return -1;
    if (!b->tdata.pn && !b->tdata.sh) return  1;
    //持ち駒を余す着手を優先
    if(a->inc < b->inc) return  1;
    if(a->inc > b->inc) return -1;
    //より短手数で詰む着手を優先
    if (a->tdata.sh < b->tdata.sh) return -1;
    if (a->tdata.sh > b->tdata.sh) return  1;
    //駒をより多く余す着手を優先
    if (a->nouse2 < b->nouse2) return -1;
    if (a->nouse2 > b->nouse2) return  1;

    //取る手優先
    if( MV_CAPTURED(a->mlist->move, s)
       >MV_CAPTURED(b->mlist->move, s)) return -1;
    if( MV_CAPTURED(a->mlist->move, s)
       <MV_CAPTURED(b->mlist->move, s)) return  1;
    
    //動かす手優先
    if(MV_MOVE(a->mlist->move) && MV_DROP(b->mlist->move)) return -1;
    if(MV_MOVE(b->mlist->move) && MV_DROP(a->mlist->move)) return  1;
    //取って成る手優先
    /*
    if(MV_MOVE(a->mlist->move)       &&
       MV_CAPTURED(a->mlist->move,s) &&
       PROMOTE(a->mlist->move)          )  return -1;
    if(MV_MOVE(b->mlist->move)       &&
       MV_CAPTURED(b->mlist->move,s) &&
       PROMOTE(b->mlist->move)          )  return  1;
     */
    
    /*
    if( MV_CAPTURED(a->mlist->move, s)) return -1;
    if( MV_CAPTURED(b->mlist->move, s)) return 1;
    */
     
    //安い駒でとる手優先
    /*
    if(S_BOARD(s,PREV_POS(a->mlist->move))
       <S_BOARD(s,PREV_POS(b->mlist->move)))   return -1;
    if(S_BOARD(s,PREV_POS(a->mlist->move))
       >S_BOARD(s,PREV_POS(b->mlist->move)))   return  1;
     */
    //近距離優先
    if(a->length < b->length) return -1;
    if(a->length > b->length) return  1;
    
    //成る手優先
    if(PROMOTE(a->mlist->move) > PROMOTE(b->mlist->move))  return -1;
    if(PROMOTE(a->mlist->move) < PROMOTE(b->mlist->move))  return  1;
    
    //高い駒優先
    if(MV_DROP(a->mlist->move) && MV_DROP(b->mlist->move)){
        if(MV_HAND(a->mlist->move) < MV_HAND(b->mlist->move)) return  1;
        if(MV_HAND(a->mlist->move) > MV_HAND(b->mlist->move)) return -1;
    }
    
    return  -1;
}

//玉方着手の並べ替え
int disproof_number_comp  (const mvlist_t *a,
                           const mvlist_t *b,
                           const sdata_t  *s)
{
    //dnの小さい着手を優先
    if (a->tdata.dn < b->tdata.dn) return -1;
    if (a->tdata.dn > b->tdata.dn) return  1;
    //pnの小さい着手を優先
    if (a->tdata.pn < b->tdata.pn) return -1;
    if (a->tdata.pn > b->tdata.pn) return  1;
    //駒を余さない着手を優先
    if (a->inc < b->inc) return -1;
    if (a->inc > b->inc) return  1;
    if (a->nouse2 < b->nouse2) return -1;
    if (a->nouse2 > b->nouse2) return  1;
    //手数の長い着手を優先
    if (!a->tdata.pn && !b->tdata.pn)
    {
        if (a->tdata.sh < b->tdata.sh) return  1;
        if (a->tdata.sh > b->tdata.sh) return -1;
    }
    else{
        if (a->tdata.sh < b->tdata.sh) return -1;
        if (a->tdata.sh > b->tdata.sh) return  1;
    }
    //動かす手優先
    if(MV_MOVE(a->mlist->move) && MV_DROP(b->mlist->move)) return -1;
    if(MV_MOVE(b->mlist->move) && MV_DROP(a->mlist->move)) return  1;
    
    //取る手優先
    if( MV_CAPTURED(a->mlist->move, s)
       >MV_CAPTURED(b->mlist->move, s)) return -1;
    if( MV_CAPTURED(a->mlist->move, s)
       <MV_CAPTURED(b->mlist->move, s)) return  1;
    
    //安い駒でとる手優先
    
    if(S_BOARD(s,PREV_POS(a->mlist->move))
       <S_BOARD(s,PREV_POS(b->mlist->move)))   return -1;
    if(S_BOARD(s,PREV_POS(a->mlist->move))
       >S_BOARD(s,PREV_POS(b->mlist->move)))   return  1;
    
    //玉の移動優先
    if(PREV_POS(a->mlist->move)==SELF_OU(s) &&
       PREV_POS(b->mlist->move)!=SELF_OU(s)) return -1;
    if(PREV_POS(a->mlist->move)!=SELF_OU(s) &&
       PREV_POS(b->mlist->move)==SELF_OU(s)) return  1;
    
    //取って成る手優先
    /*
    if(MV_MOVE(a->mlist->move)       &&
       MV_CAPTURED(a->mlist->move,s) &&
       PROMOTE(a->mlist->move)          )  return -1;
    if(MV_MOVE(b->mlist->move)       &&
       MV_CAPTURED(b->mlist->move,s) &&
       PROMOTE(b->mlist->move)          )  return  1;
    */

    /*
    if( MV_CAPTURED(a->mlist->move, s)) return -1;
    if( MV_CAPTURED(b->mlist->move, s)) return 1;
    */
    //成る手優先
    
    if(PROMOTE(a->mlist->move) > PROMOTE(b->mlist->move))  return -1;
    if(PROMOTE(a->mlist->move) < PROMOTE(b->mlist->move))  return  1;
    
    //近距離優先
    /*
    if(a->length < b->length) return -1;
    if(a->length > b->length) return  1;
    */
    //安い駒優先
    if(MV_DROP(a->mlist->move) && MV_DROP(b->mlist->move)){
        if(MV_HAND(a->mlist->move) < MV_HAND(b->mlist->move)) return -1;
        if(MV_HAND(a->mlist->move) > MV_HAND(b->mlist->move)) return  1;
    }
    return 1;
}

//証明駒
mkey_t proof_koma  (const sdata_t *sdata)
{
    
    mkey_t mkey;
    if(S_NOUTE(sdata)==2){
        memset(&mkey, 0, sizeof(mkey_t));
        return mkey;
    }
    if(g_is_skoma[S_BOARD(sdata, S_ATTACK(sdata)[0])])
    {
        memset(&mkey, 0, sizeof(mkey_t));
        return mkey;
    }
    if(S_TURN(sdata)){
        MKEY_COPY(mkey, S_SMKEY(sdata));
        if(S_GMKEY(sdata).hi) mkey.hi = 0;
        if(S_GMKEY(sdata).ka) mkey.ka = 0;
        if(S_GMKEY(sdata).ki) mkey.ki = 0;
        if(S_GMKEY(sdata).gi) mkey.gi = 0;
        if(S_GMKEY(sdata).ke) mkey.ke = 0;
        if(S_GMKEY(sdata).ky) mkey.ky = 0;
        if(S_GMKEY(sdata).fu) mkey.fu = 0;
    }
    else{
        MKEY_COPY(mkey, S_GMKEY(sdata));
        if(S_SMKEY(sdata).hi) mkey.hi = 0;
        if(S_SMKEY(sdata).ka) mkey.ka = 0;
        if(S_SMKEY(sdata).ki) mkey.ki = 0;
        if(S_SMKEY(sdata).gi) mkey.gi = 0;
        if(S_SMKEY(sdata).ke) mkey.ke = 0;
        if(S_SMKEY(sdata).ky) mkey.ky = 0;
        if(S_SMKEY(sdata).fu) mkey.fu = 0;
    }
    return mkey;
}
void proof_koma_or (const sdata_t *sdata, mvlist_t *mvlist, mvlist_t *list)
{
    unsigned int cmp_res;
    komainf_t koma;
    mvlist->mkey = list->mkey;
    mvlist->hinc = list->hinc;
    mvlist->nouse = list->nouse;
    //駒を打つ場合
    if(MV_DROP(list->mlist->move))
    {
        koma = MV_HAND(list->mlist->move);
        switch(koma){
            case FU: (mvlist->mkey.fu)++; break;
            case KY: (mvlist->mkey.ky)++; break;
            case KE: (mvlist->mkey.ke)++; break;
            case GI: (mvlist->mkey.gi)++; break;
            case KI: (mvlist->mkey.ki)++; break;
            case KA: (mvlist->mkey.ka)++; break;
            case HI: (mvlist->mkey.hi)++; break;
            default: break;
        }
    }
    //駒を取る場合
    else if(MV_CAPTURED(list->mlist->move, sdata))
    {
        koma = (MV_CAPTURED(list->mlist->move, sdata))%PROMOTED;
        switch(koma){
            case FU:
                if   (!mvlist->mkey.fu) {mvlist->hinc = 1; mvlist->nouse += 1;}
                else (mvlist->mkey.fu)--;
                break;
            case KY:
                if   (!mvlist->mkey.ky) {mvlist->hinc = 1; mvlist->nouse += 1;}
                else (mvlist->mkey.ky)--;
                break;
            case KE:
                if   (!mvlist->mkey.ke) {mvlist->hinc = 1; mvlist->nouse += 1;}
                else (mvlist->mkey.ke)--;
                break;
            case GI:
                if   (!mvlist->mkey.gi) {mvlist->hinc = 1; mvlist->nouse += 1;}
                else (mvlist->mkey.gi)--;
                break;
            case KI:
                if   (!mvlist->mkey.ki) {mvlist->hinc = 1; mvlist->nouse += 1;}
                else (mvlist->mkey.ki)--;
                break;
            case KA:
                if   (!mvlist->mkey.ka) {mvlist->hinc = 1; mvlist->nouse += 1;}
                else (mvlist->mkey.ka)--;
                break;
            case HI:
                if   (!mvlist->mkey.hi) {mvlist->hinc = 1; mvlist->nouse += 1;}
                else (mvlist->mkey.hi)--;
                break;
            default: break;
        }
    }
     
    cmp_res = MKEY_COMPARE(SELF_MKEY(sdata), mvlist->mkey);
    if(cmp_res == MKEY_EQUAL) mvlist->inc = mvlist->hinc;
    else                      mvlist->inc = 1;
    
    mvlist->nouse2 =
    TOTAL_MKEY(SELF_MKEY(sdata))-TOTAL_MKEY(mvlist->mkey)+mvlist->nouse;
    
    return;
}
void proof_koma_and(const sdata_t *sdata, mvlist_t *mvlist, mvlist_t *list)
{
    mkey_t mkey = {0,0,0,0,0,0,0,0};
    /*
     * 親局面の証明駒を決定（すべての着手の証明駒を含む持駒を算出）
     */
    mvlist_t *tmp = list;
    while(tmp){
        mkey = max_mkey(mkey, tmp->mkey);
        tmp = tmp->next;
    }
    
    /*
     * 玉方の持駒に無い持駒は証明駒の対象から外す。
     */
    if(!(SELF_HI(sdata)) && mkey.hi != ENEMY_HI(sdata))
        mkey.hi = ENEMY_HI(sdata);
    if(!(SELF_KA(sdata)) && mkey.ka != ENEMY_KA(sdata))
        mkey.ka = ENEMY_KA(sdata);
    if(!(SELF_KI(sdata)) && mkey.ki != ENEMY_KI(sdata))
        mkey.ki = ENEMY_KI(sdata);
    if(!(SELF_GI(sdata)) && mkey.gi != ENEMY_GI(sdata))
        mkey.gi = ENEMY_GI(sdata);
    if(!(SELF_KE(sdata)) && mkey.ke != ENEMY_KE(sdata))
        mkey.ke = ENEMY_KE(sdata);
    if(!(SELF_KY(sdata)) && mkey.ky != ENEMY_KY(sdata))
        mkey.ky = ENEMY_KY(sdata);
    if(!(SELF_FU(sdata)) && mkey.fu != ENEMY_FU(sdata))
        mkey.fu = ENEMY_FU(sdata);
    
    //余り駒の算出は先頭着手を使用
    mvlist->mkey = mkey;
    if(list)
        mvlist->nouse =
        list->nouse + TOTAL_MKEY(mkey) - TOTAL_MKEY(list->mkey);
    else
        assert(list);
        
    mvlist->hinc = mvlist->nouse ? 1: 0;
    mvlist->nouse2 =
    mvlist->nouse + TOTAL_MKEY(ENEMY_MKEY(sdata)) - TOTAL_MKEY(mkey);
    mvlist->inc = mvlist->nouse2 ? 1: 0;
    return;
}

//反証駒
mkey_t _disproof_koma(const sdata_t *sdata)
{
    mkey_t mkey;
    if(S_TURN(sdata)){
        MKEY_COPY(mkey, S_GMKEY(sdata));
        //mkey = S_GMKEY(sdata);
        if(mkey.hi){ mkey.hi += S_SMKEY(sdata).hi;}
        if(mkey.ka){ mkey.ka += S_SMKEY(sdata).ka;}
        if(mkey.ki){ mkey.ki += S_SMKEY(sdata).ki;}
        if(mkey.gi){ mkey.gi += S_SMKEY(sdata).gi;}
        if(mkey.ke){ mkey.ke += S_SMKEY(sdata).ke;}
        if(mkey.ky){ mkey.ky += S_SMKEY(sdata).ky;}
        if(mkey.fu){ mkey.fu += S_SMKEY(sdata).fu;}
    }
    else{
        MKEY_COPY(mkey, S_SMKEY(sdata));
        //mkey = S_SMKEY(sdata);
        if(mkey.hi){ mkey.hi += S_GMKEY(sdata).hi;}
        if(mkey.ka){ mkey.ka += S_GMKEY(sdata).ka;}
        if(mkey.ki){ mkey.ki += S_GMKEY(sdata).ki;}
        if(mkey.gi){ mkey.gi += S_GMKEY(sdata).gi;}
        if(mkey.ke){ mkey.ke += S_GMKEY(sdata).ke;}
        if(mkey.ky){ mkey.ky += S_GMKEY(sdata).ky;}
        if(mkey.fu){ mkey.fu += S_GMKEY(sdata).fu;}
    }
    return mkey;
}
mkey_t disproof_koma(const sdata_t *sdata)
{
    mkey_t mkey;
    if(S_TURN(sdata)){
        MKEY_COPY(mkey, S_GMKEY(sdata));
        if(mkey.hi ||(S_SMKEY(sdata).hi && !drop_check(sdata, HI)))
            mkey.hi += S_SMKEY(sdata).hi;
        if(mkey.ka ||(S_SMKEY(sdata).ka && !drop_check(sdata, KA)))
            mkey.ka += S_SMKEY(sdata).ka;
        if(mkey.ki ||(S_SMKEY(sdata).ki && !drop_check(sdata, KI)))
            mkey.ki += S_SMKEY(sdata).ki;
        if(mkey.gi ||(S_SMKEY(sdata).gi && !drop_check(sdata, GI)))
            mkey.gi += S_SMKEY(sdata).gi;
        if(mkey.ke ||(S_SMKEY(sdata).ke && !drop_check(sdata, KE)))
            mkey.ke += S_SMKEY(sdata).ke;
        if(mkey.ky ||(S_SMKEY(sdata).ky && !drop_check(sdata, KY)))
            mkey.ky += S_SMKEY(sdata).ky;
        if(mkey.fu ||(S_SMKEY(sdata).fu && !drop_check(sdata, FU)))
            mkey.fu += S_SMKEY(sdata).fu;
    }
    else             {
        MKEY_COPY(mkey, S_SMKEY(sdata));
        if(mkey.hi ||(S_GMKEY(sdata).hi && !drop_check(sdata, HI)))
            mkey.hi += S_GMKEY(sdata).hi;
        if(mkey.ka ||(S_GMKEY(sdata).ka && !drop_check(sdata, KA)))
            mkey.ka += S_GMKEY(sdata).ka;
        if(mkey.ki ||(S_GMKEY(sdata).ki && !drop_check(sdata, KI)))
            mkey.ki += S_GMKEY(sdata).ki;
        if(mkey.gi ||(S_GMKEY(sdata).gi && !drop_check(sdata, GI)))
            mkey.gi += S_GMKEY(sdata).gi;
        if(mkey.ke ||(S_GMKEY(sdata).ke && !drop_check(sdata, KE)))
            mkey.ke += S_GMKEY(sdata).ke;
        if(mkey.ky ||(S_GMKEY(sdata).ky && !drop_check(sdata, KY)))
            mkey.ky += S_GMKEY(sdata).ky;
        if(mkey.fu ||(S_GMKEY(sdata).fu && !drop_check(sdata, FU)))
            mkey.fu += S_GMKEY(sdata).fu;
    }
    return mkey;
}
static mkey_t disproof_koma_unit(const sdata_t *sdata, mvlist_t *mvlist)
{
    mkey_t mkey = mvlist->mkey;
    komainf_t koma;
    //駒を打つ手
    if(MV_DROP(mvlist->mlist->move))
    {
        koma = MV_HAND(mvlist->mlist->move);
        switch(koma){
            case FU: (mkey.fu)++; break;
            case KY: (mkey.ky)++; break;
            case KE: (mkey.ke)++; break;
            case GI: (mkey.gi)++; break;
            case KI: (mkey.ki)++; break;
            case KA: (mkey.ka)++; break;
            case HI: (mkey.hi)++; break;
            default: break;
        }
    }
    //駒を取る手
    else if(MV_CAPTURED(mvlist->mlist->move, sdata))
    {
        koma = (MV_CAPTURED(mvlist->mlist->move, sdata))%PROMOTED;
        switch(koma){
            case FU: (mkey.fu)--; break;
            case KY: (mkey.ky)--; break;
            case KE: (mkey.ke)--; break;
            case GI: (mkey.gi)--; break;
            case KI: (mkey.ki)--; break;
            case KA: (mkey.ka)--; break;
            case HI: (mkey.hi)--; break;
            default: break;
        }
    }
    return mkey;
}
void disproof_koma_or(const sdata_t *sdata, mvlist_t *mvlist, mvlist_t *list)
{
    mkey_t mkey = disproof_koma_unit(sdata, list);
    mvlist_t *lt = list->next;
    while(lt){
        mkey = min_mkey(mkey, disproof_koma_unit(sdata, lt));
        lt = lt->next;
    }
    if(!SELF_HI(sdata)) mkey.hi = 0;
    if(!SELF_KA(sdata)) mkey.ka = 0;
    if(!SELF_KI(sdata)) mkey.ki = 0;
    if(!SELF_GI(sdata)) mkey.gi = 0;
    if(!SELF_KE(sdata)) mkey.ke = 0;
    if(!SELF_KY(sdata)) mkey.ky = 0;
    if(!SELF_FU(sdata)) mkey.fu = 0;
    mvlist->mkey = mkey;
    unsigned int cmp_res = MKEY_COMPARE(SELF_MKEY(sdata), mkey);
    mvlist->inc = cmp_res ? 0: 1;
    return;
}

void disproof_koma_and(const sdata_t *sdata, mvlist_t *mvlist, mvlist_t *list)
{
    komainf_t koma;
    mvlist->mkey = list->mkey;
    //駒を打つ場合
    if(MV_DROP(list->mlist->move))
    {
        turn_t tn = S_TURN(sdata);
        mkey_t dmkey = tn?S_GMKEY(sdata):S_SMKEY(sdata);   //玉方の持ち駒
        mkey_t tmkey = tn?S_SMKEY(sdata):S_GMKEY(sdata);   //詰方の持ち駒
        mkey_t mkey = list->mkey; //子ノードの反証駒
        
        koma = MV_HAND(list->mlist->move);
        //玉方の打つ駒が持ち駒にあることを確認。なければ反証駒補正。
        switch(koma){
            case FU: if(dmkey.fu+tmkey.fu == mkey.fu) mvlist->mkey.fu--; break;
            case KY: if(dmkey.ky+tmkey.ky == mkey.ky) mvlist->mkey.ky--; break;
            case KE: if(dmkey.ke+tmkey.ke == mkey.ke) mvlist->mkey.ke--; break;
            case GI: if(dmkey.gi+tmkey.gi == mkey.gi) mvlist->mkey.gi--; break;
            case KI: if(dmkey.ki+tmkey.ki == mkey.ki) mvlist->mkey.ki--; break;
            case KA: if(dmkey.ka+tmkey.ka == mkey.ka) mvlist->mkey.ka--; break;
            case HI: if(dmkey.hi+tmkey.hi == mkey.hi) mvlist->mkey.hi--; break;
            default: break;
        }
    }
    //駒を取る場合
    else if(MV_CAPTURED(list->mlist->move, sdata))
    {
        turn_t tn = S_TURN(sdata);
        mkey_t dmkey = tn?S_GMKEY(sdata):S_SMKEY(sdata);   //玉方の持ち駒
        mkey_t tmkey = tn?S_SMKEY(sdata):S_GMKEY(sdata);   //詰方の持ち駒
        mkey_t mkey = list->mkey; //子ノードの反証駒
        koma = MV_CAPTURED(list->mlist->move, sdata);
        //玉方が取った駒が玉方持ち駒にあることを確認。なければ反証駒補正
        switch(koma%PROMOTED){
            case FU: if(dmkey.fu+tmkey.fu+1==mkey.fu)mvlist->mkey.fu--;break;
            case KY: if(dmkey.ky+tmkey.ky+1==mkey.ky)mvlist->mkey.ky--;break;
            case KE: if(dmkey.ke+tmkey.ke+1==mkey.ke)mvlist->mkey.ke--;break;
            case GI: if(dmkey.gi+tmkey.gi+1==mkey.gi)mvlist->mkey.gi--;break;
            case KI: if(dmkey.ki+tmkey.ki+1==mkey.ki)mvlist->mkey.ki--;break;
            case KA: if(dmkey.ka+tmkey.ka+1==mkey.ka)mvlist->mkey.ka--;break;
            case HI: if(dmkey.hi+tmkey.hi+1==mkey.hi)mvlist->mkey.hi--;break;
            default: break;
        }
    }
    unsigned int cmp_res = MKEY_COMPARE(ENEMY_MKEY(sdata), mvlist->mkey);
    mvlist->inc = cmp_res ? 0 : 1;
    return;
}

bool symmetry_check(const sdata_t *sdata)
{
    int pos = ENEMY_OU(sdata);
    int p1,p2;
    if(g_file[pos]!=FILE5) return false;
    int rank, file;
    for(rank=RANK1; rank<=RANK9; rank++){
        for(file=FILE1; file<FILE5; file++){
            p1 = rank*9+file;
            p2 = rank*9+8-file;
            if(S_BOARD(sdata, p1)!=S_BOARD(sdata, p2))
                return false;
        }
    }
    return true;
}

