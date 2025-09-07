//
//  nevasion.c
//  shtsume
//
//  Created by Hkijin on 2022/02/03.
//

#include <stdio.h>
#include "shtsume.h"
#include "ndtools.h"

bool g_invalid_drops;
bool g_invalid_moves;

static mvlist_t *move_to_dest      (mvlist_t      *list,
                                    int            dest,
                                    const sdata_t *sdata);


static mvlist_t *move_to_dest_w    (mvlist_t      *list,
                                    int            dest,
                                    const sdata_t *sdata);

static mvlist_t *move_to_dest_t    (mvlist_t      *list,
                                    int            dest,
                                    const sdata_t *sdata);


static bool valid_next             (const sdata_t *sdata,
                                    move_t         move,
                                    tbase_t       *tbase );

static bool valid_long             (const sdata_t *sdata,
                                    move_t         move,
                                    tbase_t       *tbase );

mvlist_t *_move_to_dest            (mvlist_t *list,
                                    int       dest,
                                    const sdata_t *sdata,
                                    tbase_t       *tbase,
                                    bool (*func)
                                         (const sdata_t *s,
                                          move_t         m,
                                          tbase_t       *t ));

static mlist_t *mlist_to_dest      (mlist_t       *list,
                                    int            dest,
                                    const sdata_t *sdata);

static mlist_t *_mlist_to_dest     (mlist_t       *list,
                                    int            dest,
                                    const sdata_t *sdata,
                                    tbase_t       *tbase,
                                    bool (*func)(const sdata_t *s,
                                                 move_t         m,
                                                 tbase_t       *t ));

static mlist_t *mlist_to_desta     (mlist_t       *list,
                                    int            dest,
                                    const sdata_t *sdata,
                                    tbase_t       *tbase);
static mlist_t *mlist_to_destb     (mlist_t       *list,
                                    int            dest,
                                    bool           flag,
                                    const sdata_t *sdata );
static mlist_t *mlist_to_destc     (mlist_t       *list,
                                    int            dest,
                                    const sdata_t *sdata );

static mlist_t *mlist_to_destd     (mlist_t       *list,
                                    int            dest,
                                    const sdata_t *sdata,
                                    tbase_t       *tbase);

static mlist_t *evasion_drop       (mlist_t       *list,
                                    int            dest,
                                    const sdata_t *sdata);
static bool is_eou_offline         (const sdata_t *sdata);

static bool is_hand_effective      (const sdata_t *sdata,
                                    int            dest );

static bool is_dest_effect_valid   (const sdata_t *sdata,
                                    int            dest );

static bool cross_point_check      (const sdata_t *sdata,
                                    int            dest,
                                    tbase_t       *tbase);

/* ------------------------------------------------------------------------
 generate_evasion
 局面表による無駄合い判定付き王手回避着手生成関数
 [引数]
 tbase_t *tbase  局面表tbaseへのポインタ
 [戻り値]
 無駄合いを除く王手回避着手
 g_invalid_drops   局面表による無駄合い判定　  あり:true
 g_invalid_moves   無駄移動合判定            あり:true
 
 [データ構造]
 +--玉移動1
 +--・・・
 +--玉移動a
 +--駒取り1
 +--・・・
 +--駒取りb
 +--持ち駒合1->持ち駒合２->...->持ち駒合c　（玉に近い位置より)
 +--移動合1->移動合2->...->移動合d       （玉に近い位置より)
 
 [無駄合判定条件]
 持ち駒合生成時　生成前の合法手なし
 移動合生成時   生成前の合法手なし
 
 ----------------------------------------------------------------------- */

mvlist_t *generate_evasion    (const sdata_t *sdata,
                              tbase_t       *tbase  )
{
    g_invalid_drops = false;
    g_invalid_moves = false;
    mvlist_t *mvlist = NULL;
    //玉の移動
    komainf_t koma;
    int dest, ou = SELF_OU(sdata);
    mlist_t *mlist, *mslist;                   //持ち駒合、移動合直列リスト
    bitboard_t move_bb = evasion_bb(sdata);
    do{
        dest = min_pos(&move_bb);
        if(dest<0) break;
        //着手リスト(mvlist_t)生成
        MVLIST_SET_NORM(mvlist, mlist, ou, dest);
        BBA_XOR(move_bb, g_bpos[dest]);
    } while(true);
    //両王手がかかっている場合、ここで終了
    if(S_NOUTE(sdata)>1) return mvlist;
    
    //王手している駒を取る手
    dest = S_ATTACK(sdata)[0];
    mvlist = move_to_dest_t(mvlist, dest, sdata);
    
    //王手している駒が飛び駒で無ければここで終了
    koma = S_BOARD(sdata, dest);
    if(g_is_skoma[koma]) return mvlist;
    
    // -----------------
    // 合い駒
    // -----------------
    mlist_t *drop_list = NULL;         //合駒着手構成用
    mlist_t *move_list = NULL;         //移動合着手構成用
    bool hand_exist = MKEY_EXIST(ENEMY_MKEY(sdata));
    if(!mvlist && !hand_exist) g_invalid_moves = true;
    mlist_t *last;
    mvlist_t *mvlast;
    
    //王手している駒の方向を特定
    int src = SELF_OU(sdata);
    int dir = 0;
    if     (DIR_NE(src, dest)) dir = DR_NE;
    else if(DIR_N_(src, dest)) dir = DR_N;
    else if(DIR_NW(src, dest)) dir = DR_NW;
    else if(DIR_E_(src, dest)) dir = DR_E;
    else if(DIR_W_(src, dest)) dir = DR_W;
    else if(DIR_SE(src, dest)) dir = DR_SE;
    else if(DIR_S_(src, dest)) dir = DR_S;
    else if(DIR_SW(src, dest)) dir = DR_SW;
    
    int n;        //玉とdest位置との距離。　例）隣 n=1

    if(dir){
        n = 0;
        dest = src;
        //玉に近い側から合駒の有無を調べる
        while(true){
            dest += dir;
            if(dest == S_ATTACK(sdata)[0]) break;
            n++;
            // -------------------------------------------
            //合駒（持ち駒）があれば着手に追加
            // -------------------------------------------
            mlist = NULL;
            if(n==1){
                if(!invalid_drops(sdata, dest, tbase))
                    mlist = evasion_drop(mlist, dest, sdata);
            }
            else if(n==2){
                //すでに合法手がある場合
                if(mvlist || move_list){
                    mlist = evasion_drop(mlist, dest, sdata);
                }
                //ここまで合法手がない場合、無駄合判定を行う
                //合駒の箇所に玉方の有効な利きがあり、打てる持ち駒がある場合、有効合
                else if(BPOS_TEST(SELF_EFFECT(sdata),dest) &&
                        is_dest_effect_valid(sdata, dest)){
                    mlist = evasion_drop(mlist, dest, sdata);
                }
                //destに王手になるよう駒を仮置きした局面で玉に移動手段があれば有効合
                else if(cross_point_check(sdata, dest, tbase)){
                    mlist = evasion_drop(mlist, dest, sdata);
                }
            }
            else {
                //すでに合法手がある場合
                if( mvlist || move_list){
                    mlist = evasion_drop(mlist, dest, sdata);
                }
                //ここまで合法手がない場合、無駄合判定を行う
                //合駒の箇所に玉方の有効な利きがあり、打てる持ち駒がある場合、有効合
                else if(BPOS_TEST(SELF_EFFECT(sdata),dest) &&
                        is_dest_effect_valid(sdata, dest)){
                    mlist = evasion_drop(mlist, dest, sdata);
                }
            }
            //持ち駒合の統合（直列）
            if(!drop_list) drop_list = mlist;
            else {
                last = mlist_last(drop_list);
                last->next = mlist;
            }
            
            // -------------------------------------------
            // 移動合いがあれば着手に追加
            // -------------------------------------------
            mslist = NULL;
            if(n==1){
                if(mvlist || drop_list){
                    mvlist = move_to_dest_t(mvlist, dest, sdata);
                }
                //無駄合判定
                else{
                    mslist = _mlist_to_dest(mslist, dest, sdata,
                                            tbase, valid_next);
                    if(mslist) g_invalid_moves = false;
                }
            }
            else{
                if(mvlist || drop_list ){
                    mvlist = move_to_dest_t(mvlist, dest, sdata);
                }
                //無駄合判定
                else{
                    mslist = _mlist_to_dest(mslist, dest, sdata, tbase,
                                            valid_long);
                }
                 
            }
            //移動合(直列）
            if(!move_list) move_list = mslist;
            else {
                last = mlist_last(move_list);
                last->next = mslist;
            }
        }
        
    }
    if(drop_list){
        mvlist_t *dlist = mvlist_alloc();
        dlist->mlist = drop_list;
        if (mvlist) {
            mvlast = mvlist_last(mvlist);
            mvlast->next = dlist;
        }
        else        {
            mvlist = dlist;
        }
    }
    
    if(move_list){
        mvlist_t *mvvlist = mvlist_alloc();
        mvvlist->mlist = move_list;
        if(mvlist){
            mvlast = mvlist_last(mvlist);
            mvlast->next = mvvlist;
        }
        else{
            mvlist = mvvlist;
        }
    }
     
    return mvlist;
}

/* --------------------------------
 move_to_dest
 destに駒（玉以外)を動かす手を生成する。
 -------------------------------- */

mvlist_t *move_to_dest        (mvlist_t *list,
                               int dest,
                               const sdata_t *sdata)
{
    mvlist_t *mvlist = list;
    mlist_t *mlist, *tolist = NULL;
    int src;
    bitboard_t eff = SELF_EFFECT(sdata);
    if(BPOS_TEST(eff,dest)){
        bitboard_t effect;
        //後手番
        if(S_TURN(sdata)){
            //GFU
            effect = EFFECT_TBL(dest, SFU, sdata);
            BBA_AND(effect, BB_GFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GFU_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(GFU_NORMAL(dest) ){
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKY
            effect = EFFECT_TBL(dest, SKY, sdata);
            BBA_AND(effect, BB_GKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKY_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(GKY_NORMAL(dest) ){
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKE
            effect = EFFECT_TBL(dest, SKE, sdata);
            BBA_AND(effect, BB_GKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKE_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(GKE_NORMAL(dest) ){
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GGI
            effect = EFFECT_TBL(dest, SGI, sdata);
            BBA_AND(effect, BB_GGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GGI_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKI,GTO,GNY,GNK,GNG
            effect = EFFECT_TBL(dest, SKI, sdata);
            BBA_AND(effect, BB_GTK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(S_BOARD(sdata,src)==GTO){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                    else{
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKA
            effect = EFFECT_TBL(dest, SKA, sdata);
            BBA_AND(effect, BB_GKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKA_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GHI
            effect = EFFECT_TBL(dest, SHI, sdata);
            BBA_AND(effect, BB_GHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GHI_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GUM
            effect = EFFECT_TBL(dest, SUM, sdata);
            BBA_AND(effect, BB_GUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GRY
            effect = EFFECT_TBL(dest, SRY, sdata);
            BBA_AND(effect, BB_GRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
        //先手番
        else             {
            //SFU
            effect = EFFECT_TBL(dest, GFU, sdata);
            BBA_AND(effect, BB_SFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SFU_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(SFU_NORMAL(dest) ){
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKY
            effect = EFFECT_TBL(dest, GKY, sdata);
            BBA_AND(effect, BB_SKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKY_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(SKY_NORMAL(dest) ){
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKE
            effect = EFFECT_TBL(dest, GKE, sdata);
            BBA_AND(effect, BB_SKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKE_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(SKE_NORMAL(dest) ){
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SGI
            effect = EFFECT_TBL(dest, GGI, sdata);
            BBA_AND(effect, BB_SGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SGI_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKI,STO,SNY,SNK,SNG
            effect = EFFECT_TBL(dest, GKI, sdata);
            BBA_AND(effect, BB_STK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    if(S_BOARD(sdata,src)==STO){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                    else{
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKA
            effect = EFFECT_TBL(dest, GKA, sdata);
            BBA_AND(effect, BB_SKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKA_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SHI
            effect = EFFECT_TBL(dest, GHI, sdata);
            BBA_AND(effect, BB_SHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SHI_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SUM
            effect = EFFECT_TBL(dest, GUM, sdata);
            BBA_AND(effect, BB_SUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SRY
            effect = EFFECT_TBL(dest, GRY, sdata);
            BBA_AND(effect, BB_SRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
    }
    if(tolist) mvlist = mvlist_add(mvlist, tolist);
    return mvlist;
}


/* --------------------------------
 move_to_dest_w (テスト版）
 destに駒（玉以外)を動かす手を生成する。
 （大駒の移動合は縮退させる）
 -------------------------------- */

mvlist_t *move_to_dest_w      (mvlist_t *list,
                               int dest,
                               const sdata_t *sdata)
{
    mvlist_t *mvlist = list;
    mlist_t *mlist, *tolist = NULL;
    int src;
    bitboard_t eff = SELF_EFFECT(sdata);
    if(BPOS_TEST(eff,dest)){
        bitboard_t effect;
        //後手番
        if(S_TURN(sdata)){
            //GFU
            effect = EFFECT_TBL(dest, SFU, sdata);
            BBA_AND(effect, BB_GFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GFU_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(GFU_NORMAL(dest) ){
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKY
            effect = EFFECT_TBL(dest, SKY, sdata);
            BBA_AND(effect, BB_GKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKY_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(GKY_NORMAL(dest) ){
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKE
            effect = EFFECT_TBL(dest, SKE, sdata);
            BBA_AND(effect, BB_GKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKE_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(GKE_NORMAL(dest) ){
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GGI
            effect = EFFECT_TBL(dest, SGI, sdata);
            BBA_AND(effect, BB_GGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GGI_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKI,GTO,GNY,GNK,GNG
            effect = EFFECT_TBL(dest, SKI, sdata);
            BBA_AND(effect, BB_GTK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(S_BOARD(sdata,src)==GTO){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                    else{
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKA
            effect = EFFECT_TBL(dest, SKA, sdata);
            BBA_AND(effect, BB_GKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKA_PROMOTE(src,dest)){
                        //MVLIST_SET_PROM(mvlist, mlist, src, dest);
                        MLIST_SET_PROM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                    //MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    MLIST_SET_NORM(mlist, src, dest);
                    mlist->next = tolist;
                    tolist = mlist;
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GHI
            effect = EFFECT_TBL(dest, SHI, sdata);
            BBA_AND(effect, BB_GHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GHI_PROMOTE(src,dest)){
                        //MVLIST_SET_PROM(mvlist, mlist, src, dest);
                        MLIST_SET_PROM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                    //MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    MLIST_SET_NORM(mlist, src, dest);
                    mlist->next = tolist;
                    tolist = mlist;
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GUM
            effect = EFFECT_TBL(dest, SUM, sdata);
            BBA_AND(effect, BB_GUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    //MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    MLIST_SET_NORM(mlist, src, dest);
                    mlist->next = tolist;
                    tolist = mlist;
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GRY
            effect = EFFECT_TBL(dest, SRY, sdata);
            BBA_AND(effect, BB_GRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    //MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    MLIST_SET_NORM(mlist, src, dest);
                    mlist->next = tolist;
                    tolist = mlist;
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
        //先手番
        else             {
            //SFU
            effect = EFFECT_TBL(dest, GFU, sdata);
            BBA_AND(effect, BB_SFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SFU_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(SFU_NORMAL(dest) ){
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKY
            effect = EFFECT_TBL(dest, GKY, sdata);
            BBA_AND(effect, BB_SKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKY_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(SKY_NORMAL(dest) ){
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKE
            effect = EFFECT_TBL(dest, GKE, sdata);
            BBA_AND(effect, BB_SKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKE_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(SKE_NORMAL(dest) ){
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SGI
            effect = EFFECT_TBL(dest, GGI, sdata);
            BBA_AND(effect, BB_SGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SGI_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKI,STO,SNY,SNK,SNG
            effect = EFFECT_TBL(dest, GKI, sdata);
            BBA_AND(effect, BB_STK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    if(S_BOARD(sdata,src)==STO){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                    else{
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKA
            effect = EFFECT_TBL(dest, GKA, sdata);
            BBA_AND(effect, BB_SKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKA_PROMOTE(src,dest)){
                        //MVLIST_SET_PROM(mvlist, mlist, src, dest);
                        MLIST_SET_PROM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                    //MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    MLIST_SET_NORM(mlist, src, dest);
                    mlist->next = tolist;
                    tolist = mlist;
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SHI
            effect = EFFECT_TBL(dest, GHI, sdata);
            BBA_AND(effect, BB_SHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SHI_PROMOTE(src,dest)){
                        //MVLIST_SET_PROM(mvlist, mlist, src, dest);
                        MLIST_SET_PROM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                    //MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    MLIST_SET_NORM(mlist, src, dest);
                    mlist->next = tolist;
                    tolist = mlist;
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SUM
            effect = EFFECT_TBL(dest, GUM, sdata);
            BBA_AND(effect, BB_SUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    //MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    MLIST_SET_NORM(mlist, src, dest);
                    mlist->next = tolist;
                    tolist = mlist;
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SRY
            effect = EFFECT_TBL(dest, GRY, sdata);
            BBA_AND(effect, BB_SRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    //MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    MLIST_SET_NORM(mlist, src, dest);
                    mlist->next = tolist;
                    tolist = mlist;
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
    }
    if(tolist) mvlist = mvlist_add(mvlist, tolist);
    return mvlist;
}

/* --------------------------------
 move_to_dest_t(テスト版）
 destに駒（玉以外)を動かす手を生成する。
 成る、成らないは縮退させる
 ---------------------------------- */
mvlist_t *move_to_dest_t      (mvlist_t *list,
                               int dest,
                               const sdata_t *sdata)
{
    mvlist_t *mvlist = list;
    mlist_t *mlist, *plist, *tolist = NULL;
    int src;
    bitboard_t eff = SELF_EFFECT(sdata);
    bitboard_t effect;
    if(BPOS_TEST(eff,dest))
    {
        //後手番
        if(S_TURN(sdata)){
            //GFU
            effect = EFFECT_TBL(dest, SFU, sdata);
            BBA_AND(effect, BB_GFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    plist = NULL;
                    if(GFU_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, plist, src, dest);
                    }
                    if(GFU_NORMAL(dest) ){
                        if(plist){
                            MLIST_SET_NORM(mlist, src, dest);
                            plist->next = mlist;
                        }
                        else
                            MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKY
            effect = EFFECT_TBL(dest, SKY, sdata);
            BBA_AND(effect, BB_GKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    plist = NULL;
                    if(GKY_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, plist, src, dest);
                    }
                    if(GKY_NORMAL(dest) ){
                        if(plist){
                            MLIST_SET_NORM(mlist, src, dest);
                            plist->next = mlist;
                        }
                        else
                            MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKE
            effect = EFFECT_TBL(dest, SKE, sdata);
            BBA_AND(effect, BB_GKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    plist = NULL;
                    if(GKE_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, plist, src, dest);
                    }
                    if(GKE_NORMAL(dest) ){
                        if(plist){
                            MLIST_SET_NORM(mlist, src, dest);
                            plist->next = mlist;
                        }
                        else
                            MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GGI
            effect = EFFECT_TBL(dest, SGI, sdata);
            BBA_AND(effect, BB_GGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    plist = NULL;
                    if(GGI_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, plist, src, dest);
                    }
                    if(plist){
                        MLIST_SET_NORM(mlist, src, dest);
                        plist->next = mlist;
                    }
                    else
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKI,GTO,GNY,GNK,GNG
            effect = EFFECT_TBL(dest, SKI, sdata);
            BBA_AND(effect, BB_GTK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(S_BOARD(sdata,src)==GTO){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                    else{
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKA
            effect = EFFECT_TBL(dest, SKA, sdata);
            BBA_AND(effect, BB_GKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    plist = NULL;
                    if(GKA_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, plist, src, dest);
                    }
                    if(plist){
                        MLIST_SET_NORM(mlist, src, dest);
                        plist->next = mlist;
                    }
                    else
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GHI
            effect = EFFECT_TBL(dest, SHI, sdata);
            BBA_AND(effect, BB_GHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    plist = NULL;
                    if(GHI_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, plist, src, dest);
                    }
                    if(plist){
                        MLIST_SET_NORM(mlist, src, dest);
                        plist->next = mlist;
                    }
                    else
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GUM
            effect = EFFECT_TBL(dest, SUM, sdata);
            BBA_AND(effect, BB_GUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GRY
            effect = EFFECT_TBL(dest, SRY, sdata);
            BBA_AND(effect, BB_GRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
        //先手版
        else             {
            //SFU
            effect = EFFECT_TBL(dest, GFU, sdata);
            BBA_AND(effect, BB_SFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    plist = NULL;
                    if(SFU_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, plist, src, dest);
                    }
                    if(SFU_NORMAL(dest) ){
                        if(plist){
                            MLIST_SET_NORM(mlist, src, dest);
                            plist->next = mlist;
                        }
                        else
                            MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKY
            effect = EFFECT_TBL(dest, GKY, sdata);
            BBA_AND(effect, BB_SKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    plist = NULL;
                    if(SKY_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, plist, src, dest);
                    }
                    if(SKY_NORMAL(dest) ){
                        if(plist){
                            MLIST_SET_NORM(mlist, src, dest);
                            plist->next = mlist;
                        }
                        else
                            MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKE
            effect = EFFECT_TBL(dest, GKE, sdata);
            BBA_AND(effect, BB_SKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    plist = NULL;
                    if(SKE_PROMOTE(dest)){
                        MVLIST_SET_PROM(mvlist, plist, src, dest);
                    }
                    if(SKE_NORMAL(dest) ){
                        if(plist){
                            MLIST_SET_NORM(mlist, src, dest);
                            plist->next = mlist;
                        }
                        else
                            MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SGI
            effect = EFFECT_TBL(dest, GGI, sdata);
            BBA_AND(effect, BB_SGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    plist = NULL;
                    if(SGI_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, plist, src, dest);
                    }
                    if(plist){
                        MLIST_SET_NORM(mlist, src, dest);
                        plist->next = mlist;
                    }
                    else
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKI,STO,SNY,SNK,SNG
            effect = EFFECT_TBL(dest, GKI, sdata);
            BBA_AND(effect, BB_STK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(S_BOARD(sdata,src)==GTO){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                    else{
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKA
            effect = EFFECT_TBL(dest, GKA, sdata);
            BBA_AND(effect, BB_SKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    plist = NULL;
                    if(SKA_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, plist, src, dest);
                    }
                    if(plist){
                        MLIST_SET_NORM(mlist, src, dest);
                        plist->next = mlist;
                    }
                    else
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SHI
            effect = EFFECT_TBL(dest, GHI, sdata);
            BBA_AND(effect, BB_SHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    plist = NULL;
                    if(SHI_PROMOTE(src,dest)){
                        MVLIST_SET_PROM(mvlist, plist, src, dest);
                    }
                    if(plist){
                        MLIST_SET_NORM(mlist, src, dest);
                        plist->next = mlist;
                    }
                    else
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SUM
            effect = EFFECT_TBL(dest, GUM, sdata);
            BBA_AND(effect, BB_SUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SRY
            effect = EFFECT_TBL(dest, GRY, sdata);
            BBA_AND(effect, BB_SRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
    }
    if(tolist) mvlist = mvlist_add(mvlist, tolist);
    return mvlist;
}




/* -------------------------------------------------------------------
    funcの評価結果がtrueであればdestに駒を移動する着手を生成する
 　　・valid_next:destが玉の隣の場合、無駄移動合にならない着手のみを生成。
    ・valid_long:destが玉の隣以外で、無駄移動合にならない着手のみを生成。
   ------------------------------------------------------------------- */

bool valid_next               (const sdata_t *sdata,
                               move_t         move,
                               tbase_t       *tbase )
{
    sdata_t   sbuf;  //仮想局面
    memcpy(&sbuf, sdata, sizeof(sdata_t));
    sdata_pickup_table(&sbuf, PREV_POS(move), false);
    
    //詰方の玉に王手が掛かっていれば有効合
    if(ENEMY_OU(&sbuf)<HAND &&
       BPOS_TEST(SELF_EFFECT(&sbuf), ENEMY_OU(&sbuf)))
        return true;
    
    //移動元に味方の飛び駒の利きがあり、逆王手のリスクがあれば有効合
    bitboard_t bb, effect;
    int src = PREV_POS(move), pos;
    int eou = ENEMY_OU(&sbuf);
    if(eou<HAND && BPOS_TEST(SELF_EFFECT(&sbuf), src)){
        if(S_TURN(&sbuf)){
            //GKY
            bb = BB_GKY(&sbuf);
            while(1){
                pos = min_pos(&bb);
                if(pos<0) break;
                effect = EFFECT_TBL(pos, GKY, &sbuf);
                if(BPOS_TEST(effect, src)){
                    if(g_file[eou]==g_file[src] && eou > src) return true;
                }
                BBA_XOR(bb, g_bpos[pos]);
            }
            //GKA,GUM
            bb = BB_GUK(&sbuf);
            while(1){
                pos = min_pos(&bb);
                if(pos<0) break;
                effect = EFFECT_TBL(pos, GKA, &sbuf);
                if(BPOS_TEST(effect, src)){
                    if((g_rslp[eou]==g_rslp[src] && g_rslp[src]==g_rslp[pos])||
                       (g_lslp[eou]==g_lslp[src] && g_lslp[src]==g_lslp[pos])  )
                    {
                        if((eou>src && src>pos) ||
                           (eou<src && src<pos)    ) return true;
                    }
                }
                BBA_XOR(bb, g_bpos[pos]);
            }
            //GHI,GRY
            bb = BB_GRH(&sbuf);
            while(1){
                pos = min_pos(&bb);
                if(pos<0) break;
                effect = EFFECT_TBL(pos, GHI, &sbuf);
                if(BPOS_TEST(effect, src)){
                    if((g_rank[eou]==g_rank[src] && g_rank[src]==g_rank[pos])||
                       (g_file[eou]==g_file[src] && g_file[src]==g_file[pos])  )
                    {
                        if((eou>src && src>pos) ||
                           (eou<src && src<pos)    ) return true;
                    }
                }
                BBA_XOR(bb, g_bpos[pos]);
            }
        }
        else             {
            //SKY
            bb = BB_SKY(&sbuf);
            while(1){
                pos = min_pos(&bb);
                if(pos<0) break;
                effect = EFFECT_TBL(pos, SKY, &sbuf);
                if(BPOS_TEST(effect, src)){
                    if(g_file[eou]==g_file[src] && eou < src) return true;
                }
                BBA_XOR(bb, g_bpos[pos]);
            }
            //SKA,SUM
            bb = BB_SUK(&sbuf);
            while(1){
                pos = min_pos(&bb);
                if(pos<0) break;
                effect = EFFECT_TBL(pos, SKA, &sbuf);
                if(BPOS_TEST(effect, src)){
                    if((g_rslp[eou]==g_rslp[src] && g_rslp[src]==g_rslp[pos])||
                       (g_lslp[eou]==g_lslp[src] && g_lslp[src]==g_lslp[pos])  )
                    {
                        if((eou>src && src>pos) ||
                           (eou<src && src<pos)    ) return true;
                    }
                }
                BBA_XOR(bb, g_bpos[pos]);
            }
            //SHI,SRY
            bb = BB_GRH(&sbuf);
            while(1){
                pos = min_pos(&bb);
                if(pos<0) break;
                effect = EFFECT_TBL(pos, SHI, &sbuf);
                if(BPOS_TEST(effect, src)){
                    if((g_rank[eou]==g_rank[src] && g_rank[src]==g_rank[pos])||
                       (g_file[eou]==g_file[src] && g_file[src]==g_file[pos])  )
                    {
                        if((eou>src && src>pos) ||
                           (eou<src && src<pos)    ) return true;
                    }
                }
                BBA_XOR(bb, g_bpos[pos]);
            }
        }
    }
    
    // sdataの攻方持ち駒がなければ、仮想局面で無駄合判定を行う
    
    if(!MKEY_EXIST(ENEMY_MKEY(sdata))){
        bool res = invalid_drops(&sbuf, NEW_POS(move), tbase);
        if(res){
            g_invalid_moves = true;
            return false;
        }
    }
    
    //destの位置に王手になるよう攻め方の駒を移動させる。
    //いずれかの局面が詰みであれば　false
    
    return true;
}

bool valid_long               (const sdata_t *sdata,
                               move_t         move,
                               tbase_t       *tbase )
{
    sdata_t   sbuf;  //仮想局面
    memcpy(&sbuf, sdata, sizeof(sdata_t));
    sdata_pickup_table(&sbuf, PREV_POS(move), true);
    
    //自玉に移動できる場所があれば有効合
    bitboard_t move_bb = evasion_bb(&sbuf);
    if(BB_TEST(move_bb)) return true;
    
    //双玉詰将棋の場合
    if(ENEMY_OU(&sbuf)<HAND)
    {
        //詰方の玉に王手が掛かっていれば有効合
        if(BPOS_TEST(SELF_EFFECT(&sbuf), ENEMY_OU(&sbuf)))
            return true;
        //移動元の位置を空けることにより逆王手の可能性があれば有効合
        if(check_back_risk(&sbuf, PREV_POS(move)))
            return true;
        
    }
    
    //手番ごとのチェック
    int src, dest = NEW_POS(move);
    bitboard_t effect, eff;
    if(S_TURN(&sbuf)){
        eff = GEFFECT(&sbuf);
        //仮想局面において、dest箇所にpinされていない玉方利きがあれば有効合
        if(BPOS_TEST(eff, dest)){
            //利いてる駒がpinされていないことを確認
            //GFU
            effect = EFFECT_TBL(dest, SFU, &sbuf);
            BBA_AND(effect, BB_GFU(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKY
            effect = EFFECT_TBL(dest, SKY, &sbuf);
            BBA_AND(effect, BB_GKY(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKE
            effect = EFFECT_TBL(dest, SKE, &sbuf);
            BBA_AND(effect, BB_GKE(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GGI
            effect = EFFECT_TBL(dest, SGI, &sbuf);
            BBA_AND(effect, BB_GGI(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKI,GTO,GNY,GNK,GNG
            effect = EFFECT_TBL(dest, SKI, &sbuf);
            BBA_AND(effect, BB_GTK(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKA
            effect = EFFECT_TBL(dest, SKA, &sbuf);
            BBA_AND(effect, BB_GKA(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GHI
            effect = EFFECT_TBL(dest, SHI, &sbuf);
            BBA_AND(effect, BB_GHI(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GUM
            effect = EFFECT_TBL(dest, SUM, &sbuf);
            BBA_AND(effect, BB_GUM(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GRY
            effect = EFFECT_TBL(dest, SRY, &sbuf);
            BBA_AND(effect, BB_GRY(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            
        }
        
        //移動元の位置にPINされていない玉方角馬の利きがある場合、有効合
        eff = EFFECT_TBL(PREV_POS(move), SKA, &sbuf);
        BBA_AND(eff, BB_GUK(&sbuf));
        while(1){
            src = min_pos(&eff);
            if(src<0) break;
            if(!S_PINNED(&sbuf)[src]) {
                //攻め駒を移動先位置まで進めた局面を作り、詰んでなければ有効合
                sdata_t sbuf1;
                memcpy(&sbuf1, &sbuf, sizeof(sdata_t));
                sdata_tentative_move(&sbuf1, S_ATTACK(&sbuf)[0], dest, false);
                if(!tsumi_check(&sbuf1))
                    return true;
            }
            BBA_XOR(eff, g_bpos[src]);
        }
        
        //移動元の位置にPINされていない玉方飛龍の利きがある場合、有効合
        eff = EFFECT_TBL(PREV_POS(move), SHI, &sbuf);
        BBA_AND(eff, BB_GRH(&sbuf));
        while(1){
            src = min_pos(&eff);
            if(src<0) break;
            if(!S_PINNED(&sbuf)[src]) {
                //攻め駒を移動先位置まで進めた局面を作り、詰んでなければ有効合
                sdata_t sbuf1;
                memcpy(&sbuf1, &sbuf, sizeof(sdata_t));
                sdata_tentative_move(&sbuf1, S_ATTACK(&sbuf)[0], dest, false);
                if(!tsumi_check(&sbuf1))
                    return true;
            }
            BBA_XOR(eff, g_bpos[src]);
        }
        
        //移動元の位置にPINされていない玉方香の利きがある場合、有効合
        eff = EFFECT_TBL(PREV_POS(move), SKY, &sbuf);
        BBA_AND(eff, BB_GKY(&sbuf));
        while(1){
            src = min_pos(&eff);
            if(src<0) break;
            if(!S_PINNED(&sbuf)[src]) {
                //攻め駒を移動先位置まで進めた局面を作り、詰んでなければ有効合
                sdata_t sbuf1;
                memcpy(&sbuf1, &sbuf, sizeof(sdata_t));
                sdata_tentative_move(&sbuf1, S_ATTACK(&sbuf)[0], dest, false);
                if(!tsumi_check(&sbuf1))
                    return true;
            }
            BBA_XOR(eff, g_bpos[src]);
        }
    }
    else             {
        eff = SEFFECT(&sbuf);
        //仮想局面において、dest箇所にpinされていない玉方利きがあれば有効合
        if(BPOS_TEST(eff, dest)){
            //利いてる駒がpinされていないことを確認
            //SFU
            effect = EFFECT_TBL(dest, GFU, &sbuf);
            BBA_AND(effect, BB_SFU(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKY
            effect = EFFECT_TBL(dest, GKY, &sbuf);
            BBA_AND(effect, BB_SKY(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKE
            effect = EFFECT_TBL(dest, GKE, &sbuf);
            BBA_AND(effect, BB_SKE(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SGI
            effect = EFFECT_TBL(dest, GGI, &sbuf);
            BBA_AND(effect, BB_SGI(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKI,STO,SNY,SNK,SNG
            effect = EFFECT_TBL(dest, GKI, &sbuf);
            BBA_AND(effect, BB_STK(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKA
            effect = EFFECT_TBL(dest, GKA, &sbuf);
            BBA_AND(effect, BB_SKA(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SHI
            effect = EFFECT_TBL(dest, SHI, &sbuf);
            BBA_AND(effect, BB_GHI(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SUM
            effect = EFFECT_TBL(dest, GUM, &sbuf);
            BBA_AND(effect, BB_SUM(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SRY
            effect = EFFECT_TBL(dest, GRY, &sbuf);
            BBA_AND(effect, BB_SRY(&sbuf));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(&sbuf)[src]) return true;
                BBA_XOR(effect, g_bpos[src]);
            }
        }
        
        //移動元の位置にPINされていない玉方角馬の利きがある場合、有効合
        eff = EFFECT_TBL(PREV_POS(move), GKA, &sbuf);
        BBA_AND(eff, BB_SUK(&sbuf));
        while(1){
            src = min_pos(&eff);
            if(src<0) break;
            if(!S_PINNED(&sbuf)[src]) {
                //攻め駒を移動先位置まで進めた局面を作り、詰んでなければ有効合
                sdata_t sbuf1;
                memcpy(&sbuf1, &sbuf, sizeof(sdata_t));
                sdata_tentative_move(&sbuf1, S_ATTACK(&sbuf)[0], dest, false);
                if(!tsumi_check(&sbuf1))
                    return true;
            }
            BBA_XOR(eff, g_bpos[src]);
        }
        
        //移動元の位置にPINされていない玉方飛龍の利きがある場合、有効合
        eff = EFFECT_TBL(PREV_POS(move), GHI, &sbuf);
        BBA_AND(eff, BB_SRH(&sbuf));
        while(1){
            src = min_pos(&eff);
            if(src<0) break;
            if(!S_PINNED(&sbuf)[src]) {
                //攻め駒を移動先位置まで進めた局面を作り、詰んでなければ有効合
                sdata_t sbuf1;
                memcpy(&sbuf1, &sbuf, sizeof(sdata_t));
                sdata_tentative_move(&sbuf1, S_ATTACK(&sbuf)[0], dest, false);
                if(!tsumi_check(&sbuf1))
                    return true;
            }
            BBA_XOR(eff, g_bpos[src]);
        }

        //移動元の位置にPINされていない玉方香の利きがある場合、有効合
        eff = EFFECT_TBL(PREV_POS(move), GKY, &sbuf);
        BBA_AND(eff, BB_SKY(&sbuf));
        while(1){
            src = min_pos(&eff);
            if(src<0) break;
            if(!S_PINNED(&sbuf)[src]) {
                //攻め駒を移動先位置まで進めた局面を作り、詰んでなければ有効合
                sdata_t sbuf1;
                memcpy(&sbuf1, &sbuf, sizeof(sdata_t));
                sdata_tentative_move(&sbuf1, S_ATTACK(&sbuf)[0], dest, false);
                if(!tsumi_check(&sbuf1))
                    return true;
            }
            BBA_XOR(eff, g_bpos[src]);
        }
    }
    
    return false;
}


mvlist_t *_move_to_dest       (mvlist_t *list,
                               int       dest,
                               const sdata_t *sdata,
                               tbase_t       *tbase,
                               bool (*func)(const sdata_t *s,
                                            move_t         m,
                                            tbase_t       *t ))
{
    mvlist_t *mvlist = list;
    mlist_t *mlist, *tolist = NULL;
    int src;
    bitboard_t eff = SELF_EFFECT(sdata);
    if(BPOS_TEST(eff,dest)){
        bitboard_t effect;
        move_t mv;
        //後手番
        if(S_TURN(sdata)){
            //GFU
            effect = EFFECT_TBL(dest, SFU, sdata);
            BBA_AND(effect, BB_GFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(GFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKY
            effect = EFFECT_TBL(dest, SKY, sdata);
            BBA_AND(effect, BB_GKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(GKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKE
            effect = EFFECT_TBL(dest, SKE, sdata);
            BBA_AND(effect, BB_GKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(GKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata, mv, tbase))
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GGI
            effect = EFFECT_TBL(dest, SGI, sdata);
            BBA_AND(effect, BB_GGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GGI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    if(func(sdata, mv, tbase))
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKI,GTO,GNY,GNK,GNG
            effect = EFFECT_TBL(dest, SKI, sdata);
            BBA_AND(effect, BB_GTK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(S_BOARD(sdata,src)==GTO){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata, mv, tbase))
                        {
                            MLIST_SET_NORM(mlist, src, dest);
                            mlist->next = tolist;
                            tolist = mlist;
                        }
                    }
                    else{
                        MV_SET(mv,src,dest,0);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKA
            effect = EFFECT_TBL(dest, SKA, sdata);
            BBA_AND(effect, BB_GKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKA_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        /*
                        if(func(sdata, mv, tbase)){
                            MLIST_SET_PROM(mlist, src, dest);
                            mlist->next = tolist;
                            tolist = mlist;
                        }
                         */
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    /*
                    if(func(sdata, mv, tbase)){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                     */
                    if(func(sdata, mv, tbase))
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GHI
            effect = EFFECT_TBL(dest, SHI, sdata);
            BBA_AND(effect, BB_GHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GHI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        /*
                        if(func(sdata, mv, tbase)){
                            MLIST_SET_PROM(mlist, src, dest);
                            mlist->next = tolist;
                            tolist = mlist;
                        }
                         */
                        if(func(sdata, mv, tbase))
                          MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    /*
                    if(func(sdata, mv, tbase)){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                     */
                    if(func(sdata, mv, tbase))
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GUM
            effect = EFFECT_TBL(dest, SUM, sdata);
            BBA_AND(effect, BB_GUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    /*
                    if(func(sdata, mv, tbase)){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                     */
                    if(func(sdata, mv, tbase))
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GRY
            effect = EFFECT_TBL(dest, SRY, sdata);
            BBA_AND(effect, BB_GRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    /*
                    if(func(sdata, mv, tbase)){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                     */
                    if(func(sdata, mv, tbase))
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
        //先手番
        else             {
            //SFU
            effect = EFFECT_TBL(dest, GFU, sdata);
            BBA_AND(effect, BB_SFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(SFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKY
            effect = EFFECT_TBL(dest, GKY, sdata);
            BBA_AND(effect, BB_SKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(SKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKE
            effect = EFFECT_TBL(dest, GKE, sdata);
            BBA_AND(effect, BB_SKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    if(SKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SGI
            effect = EFFECT_TBL(dest, GGI, sdata);
            BBA_AND(effect, BB_SGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SGI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    if(func(sdata, mv, tbase))
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKI,STO,SNY,SNK,SNG
            effect = EFFECT_TBL(dest, GKI, sdata);
            BBA_AND(effect, BB_STK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    if(S_BOARD(sdata,src)==STO){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata, mv, tbase))
                        {
                            MLIST_SET_NORM(mlist, src, dest);
                            mlist->next = tolist;
                            tolist = mlist;
                        }
                    }
                    else{
                        MV_SET(mv,src,dest,0);
                        if(func(sdata, mv, tbase))
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKA
            effect = EFFECT_TBL(dest, GKA, sdata);
            BBA_AND(effect, BB_SKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKA_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        /*
                        if(func(sdata, mv, tbase)){
                            MLIST_SET_PROM(mlist, src, dest);
                            mlist->next = tolist;
                            tolist = mlist;
                        }
                         */
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    /*
                    if(func(sdata, mv, tbase)){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                     */
                    if(func(sdata, mv, tbase))
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SHI
            effect = EFFECT_TBL(dest, GHI, sdata);
            BBA_AND(effect, BB_SHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SHI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        /*
                        if(func(sdata, mv, tbase)){
                            MLIST_SET_PROM(mlist, src, dest);
                            mlist->next = tolist;
                            tolist = mlist;
                        }
                         */
                        if(func(sdata, mv, tbase))
                            MVLIST_SET_PROM(mvlist, mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    /*
                    if(func(sdata, mv, tbase)){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                     */
                    if(func(sdata, mv, tbase))
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SUM
            effect = EFFECT_TBL(dest, GUM, sdata);
            BBA_AND(effect, BB_SUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    /*
                    if(func(sdata, mv, tbase)){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                     */
                    if(func(sdata, mv, tbase))
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SRY
            effect = EFFECT_TBL(dest, GRY, sdata);
            BBA_AND(effect, BB_SRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    /*
                    if(func(sdata, mv, tbase)){
                        MLIST_SET_NORM(mlist, src, dest);
                        mlist->next = tolist;
                        tolist = mlist;
                    }
                     */
                    if(func(sdata, mv, tbase))
                        MVLIST_SET_NORM(mvlist, mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
    }
    if(tolist) mvlist = mvlist_add(mvlist, tolist);
    return mvlist;
}

// --------------------------------
// 移動無駄合判定
// --------------------------------

#define MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest)   \
        (new_mlist) = mlist_alloc(),                       \
        (new_mlist)->move.prev_pos = (src),                \
        (new_mlist)->move.new_pos =  (dest),               \
        (new_mlist)->next = (mlist),                       \
        (mlist) = (new_mlist)

#define MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest)   \
        (new_mlist) = mlist_alloc(),                       \
        (new_mlist)->move.prev_pos = (src),                \
        (new_mlist)->move.new_pos =  ((dest)|0x80),        \
        (new_mlist)->next = (mlist),                       \
        (mlist) = (new_mlist)

// --------------------------------
// destへの移動合を生成する。
// --------------------------------
/* ------------------------------------------------------------------------
 mlist_to_dest
 destへの移動合生成関数
 [引数]
 list   データを追加するリスト。このリストの先頭に移動合データが追加される。
 dest   移動合の移動先
 sdata  局面データ
 
 [戻り値]
 listの先頭に新たな移動合データが追加されたリスト。
 
 [データ構造]

 +--（新しい移動合）->list
 ----------------------------------------------------------------------- */

mlist_t *mlist_to_dest             (mlist_t       *list,
                                    int            dest,
                                    const sdata_t *sdata )
{
    mlist_t *mlist = list;
    mlist_t *new_mlist;
    bitboard_t eff = SELF_EFFECT(sdata);
    int src;
    //destへ移動できる駒を探す
    if(BPOS_TEST(eff,dest)){
        bitboard_t effect;
        move_t mv;
        //後手番
        if(S_TURN(sdata)){
            //GFU
            effect = EFFECT_TBL(dest, SFU, sdata);
            BBA_AND(effect, BB_GFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    if(GFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKY
            effect = EFFECT_TBL(dest, SKY, sdata);
            BBA_AND(effect, BB_GKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    if(GKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKE
            effect = EFFECT_TBL(dest, SKE, sdata);
            BBA_AND(effect, BB_GKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    if(GKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GGI
            effect = EFFECT_TBL(dest, SGI, sdata);
            BBA_AND(effect, BB_GGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GGI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKI,GTO,GNY,GNK,GNG
            effect = EFFECT_TBL(dest, SKI, sdata);
            BBA_AND(effect, BB_GTK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    MV_SET(mv,src,dest,0);
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKA
            effect = EFFECT_TBL(dest, SKA, sdata);
            BBA_AND(effect, BB_GKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKA_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GHI
            effect = EFFECT_TBL(dest, SHI, sdata);
            BBA_AND(effect, BB_GHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GHI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GUM
            effect = EFFECT_TBL(dest, SUM, sdata);
            BBA_AND(effect, BB_GUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GRY
            effect = EFFECT_TBL(dest, SRY, sdata);
            BBA_AND(effect, BB_GRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
        //先手番
        else             {
            //SFU
            effect = EFFECT_TBL(dest, GFU, sdata);
            BBA_AND(effect, BB_SFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    if(SFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKY
            effect = EFFECT_TBL(dest, GKY, sdata);
            BBA_AND(effect, BB_SKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    if(SKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKE
            effect = EFFECT_TBL(dest, GKE, sdata);
            BBA_AND(effect, BB_SKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    if(SKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SGI
            effect = EFFECT_TBL(dest, GGI, sdata);
            BBA_AND(effect, BB_SGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SGI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKI,STO,SNY,SNK,SNG
            effect = EFFECT_TBL(dest, GKI, sdata);
            BBA_AND(effect, BB_STK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKA
            effect = EFFECT_TBL(dest, GKA, sdata);
            BBA_AND(effect, BB_SKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKA_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SHI
            effect = EFFECT_TBL(dest, GHI, sdata);
            BBA_AND(effect, BB_SHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SHI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SUM
            effect = EFFECT_TBL(dest, GUM, sdata);
            BBA_AND(effect, BB_SUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SRY
            effect = EFFECT_TBL(dest, GRY, sdata);
            BBA_AND(effect, BB_SRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
    }
    return mlist;
}

mlist_t *_mlist_to_dest            (mlist_t       *list,
                                    int            dest,
                                    const sdata_t *sdata,
                                    tbase_t       *tbase,
                                    bool (*func)(const sdata_t *s,
                                                 move_t         m,
                                                 tbase_t       *t ))
{
    mlist_t *mlist = list;
    mlist_t *new_mlist;
    bitboard_t eff = SELF_EFFECT(sdata);
    int src;
    //destへ移動できる駒を探す
    if(BPOS_TEST(eff,dest)){
        bitboard_t effect;
        move_t mv;
        //後手番
        if(S_TURN(sdata)){
            //GFU
            effect = EFFECT_TBL(dest, SFU, sdata);
            BBA_AND(effect, BB_GFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                    if(GFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKY
            effect = EFFECT_TBL(dest, SKY, sdata);
            BBA_AND(effect, BB_GKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                    if(GKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKE
            effect = EFFECT_TBL(dest, SKE, sdata);
            BBA_AND(effect, BB_GKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                    if(GKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GGI
            effect = EFFECT_TBL(dest, SGI, sdata);
            BBA_AND(effect, BB_GGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    MV_SET(mv,src,dest,0);
                    if(func(sdata,mv,tbase))
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    if(GGI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKI,GTO,GNY,GNK,GNG
            effect = EFFECT_TBL(dest, SKI, sdata);
            BBA_AND(effect, BB_GTK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    MV_SET(mv,src,dest,0);
                    if(func(sdata,mv,tbase))
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKA
            effect = EFFECT_TBL(dest, SKA, sdata);
            BBA_AND(effect, BB_GKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    MV_SET(mv,src,dest,0);
                    if(func(sdata,mv,tbase))
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    if(GKA_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GHI
            effect = EFFECT_TBL(dest, SHI, sdata);
            BBA_AND(effect, BB_GHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    MV_SET(mv,src,dest,0);
                    if(func(sdata,mv,tbase))
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    if(GHI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GUM
            effect = EFFECT_TBL(dest, SUM, sdata);
            BBA_AND(effect, BB_GUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(func(sdata,mv,tbase))
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GRY
            effect = EFFECT_TBL(dest, SRY, sdata);
            BBA_AND(effect, BB_GRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(func(sdata,mv,tbase))
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
        //先手番
        else             {
            //SFU
            effect = EFFECT_TBL(dest, GFU, sdata);
            BBA_AND(effect, BB_SFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                    if(SFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKY
            effect = EFFECT_TBL(dest, GKY, sdata);
            BBA_AND(effect, BB_SKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                    if(SKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKE
            effect = EFFECT_TBL(dest, GKE, sdata);
            BBA_AND(effect, BB_SKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                    if(SKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SGI
            effect = EFFECT_TBL(dest, GGI, sdata);
            BBA_AND(effect, BB_SGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    MV_SET(mv,src,dest,0);
                    if(func(sdata,mv,tbase))
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    if(SGI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKI,STO,SNY,SNK,SNG
            effect = EFFECT_TBL(dest, GKI, sdata);
            BBA_AND(effect, BB_STK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(func(sdata,mv,tbase))
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKA
            effect = EFFECT_TBL(dest, GKA, sdata);
            BBA_AND(effect, BB_SKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    MV_SET(mv,src,dest,0);
                    if(func(sdata,mv,tbase))
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    if(SKA_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SHI
            effect = EFFECT_TBL(dest, GHI, sdata);
            BBA_AND(effect, BB_SHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    MV_SET(mv,src,dest,0);
                    if(func(sdata,mv,tbase))
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    if(SHI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(func(sdata,mv,tbase))
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SUM
            effect = EFFECT_TBL(dest, GUM, sdata);
            BBA_AND(effect, BB_SUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(func(sdata,mv,tbase))
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SRY
            effect = EFFECT_TBL(dest, GRY, sdata);
            BBA_AND(effect, BB_SRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(func(sdata,mv,tbase))
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
    }
    return mlist;
}





mlist_t *mlist_to_desta       (mlist_t       *list,
                               int            dest,
                               const sdata_t *sdata,
                               tbase_t       *tbase)
{
    mlist_t *mlist = list;
    mlist_t *new_mlist;
    bitboard_t eff = SELF_EFFECT(sdata);
    int src;
    if(BPOS_TEST(eff,dest)){
        bitboard_t effect;
        move_t mv;
        if(S_TURN(sdata)){  //後手番
            //GFU
            effect = EFFECT_TBL(dest, SFU, sdata);
            BBA_AND(effect, BB_GFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    if(GFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKY
            effect = EFFECT_TBL(dest, SKY, sdata);
            BBA_AND(effect, BB_GKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    if(GKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKE
            effect = EFFECT_TBL(dest, SKE, sdata);
            BBA_AND(effect, BB_GKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    if(GKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GGI
            effect = EFFECT_TBL(dest, SGI, sdata);
            BBA_AND(effect, BB_GGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GGI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(!invalid_moves(sdata, mv, tbase)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKI,GTO,GNY,GNK,GNG
            effect = EFFECT_TBL(dest, SKI, sdata);
            BBA_AND(effect, BB_GTK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    MV_SET(mv,src,dest,0);
                    if(!invalid_moves(sdata, mv, tbase)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKA
            effect = EFFECT_TBL(dest, SKA, sdata);
            BBA_AND(effect, BB_GKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKA_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(!invalid_moves(sdata, mv, tbase)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GHI
            effect = EFFECT_TBL(dest, SHI, sdata);
            BBA_AND(effect, BB_GHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GHI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(!invalid_moves(sdata, mv, tbase)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GUM
            effect = EFFECT_TBL(dest, SUM, sdata);
            BBA_AND(effect, BB_GUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(!invalid_moves(sdata, mv, tbase)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GRY
            effect = EFFECT_TBL(dest, SRY, sdata);
            BBA_AND(effect, BB_GRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(!invalid_moves(sdata, mv, tbase)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
        else             {  //先手番
            //SFU
            effect = EFFECT_TBL(dest, GFU, sdata);
            BBA_AND(effect, BB_SFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    if(SFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKY
            effect = EFFECT_TBL(dest, GKY, sdata);
            BBA_AND(effect, BB_SKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    if(SKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKE
            effect = EFFECT_TBL(dest, GKE, sdata);
            BBA_AND(effect, BB_SKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    if(SKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SGI
            effect = EFFECT_TBL(dest, GGI, sdata);
            BBA_AND(effect, BB_SGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SGI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(!invalid_moves(sdata, mv, tbase)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKI,STO,SNY,SNK,SNG
            effect = EFFECT_TBL(dest, GKI, sdata);
            BBA_AND(effect, BB_STK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(!invalid_moves(sdata, mv, tbase)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKA
            effect = EFFECT_TBL(dest, GKA, sdata);
            BBA_AND(effect, BB_SKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKA_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(!invalid_moves(sdata, mv, tbase)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SHI
            effect = EFFECT_TBL(dest, GHI, sdata);
            BBA_AND(effect, BB_SHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SHI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(!invalid_moves(sdata, mv, tbase)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(!invalid_moves(sdata, mv, tbase)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SUM
            effect = EFFECT_TBL(dest, GUM, sdata);
            BBA_AND(effect, BB_SUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(!invalid_moves(sdata, mv, tbase)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SRY
            effect = EFFECT_TBL(dest, GRY, sdata);
            BBA_AND(effect, BB_SRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(!invalid_moves(sdata, mv, tbase)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
    }
    return mlist;
}

mlist_t *mlist_to_destb       (mlist_t       *list,
                               int            dest,
                               bool           flag,
                               const sdata_t *sdata )
{
    mlist_t *mlist = list;
    mlist_t *new_mlist;
    bitboard_t eff = SELF_EFFECT(sdata);
    int src;
    if(BPOS_TEST(eff,dest)){
        bitboard_t effect;
        move_t mv;
        if(S_TURN(sdata)){  //後手番
            //GFU
            effect = EFFECT_TBL(dest, SFU, sdata);
            BBA_AND(effect, BB_GFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                            if(g_invalid_moves) g_invalid_moves = false;
                        }
                    }
                    if(GFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                            if(g_invalid_moves) g_invalid_moves = false;
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKY
            effect = EFFECT_TBL(dest, SKY, sdata);
            BBA_AND(effect, BB_GKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                            if(g_invalid_moves) g_invalid_moves = false;
                        }
                    }
                    if(GKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                            if(g_invalid_moves) g_invalid_moves = false;
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKE
            effect = EFFECT_TBL(dest, SKE, sdata);
            BBA_AND(effect, BB_GKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                            if(g_invalid_moves) g_invalid_moves = false;
                        }
                    }
                    if(GKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                            if(g_invalid_moves) g_invalid_moves = false;
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GGI
            effect = EFFECT_TBL(dest, SGI, sdata);
            BBA_AND(effect, BB_GGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GGI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv) || flag){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        if(g_invalid_moves) g_invalid_moves = false;
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKI,GTO,GNY,GNK,GNG
            effect = EFFECT_TBL(dest, SKI, sdata);
            BBA_AND(effect, BB_GTK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv) || flag){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        if(g_invalid_moves) g_invalid_moves = false;
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKA
            effect = EFFECT_TBL(dest, SKA, sdata);
            BBA_AND(effect, BB_GKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKA_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv) || flag){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        if(g_invalid_moves) g_invalid_moves = false;
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GHI
            effect = EFFECT_TBL(dest, SHI, sdata);
            BBA_AND(effect, BB_GHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GHI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv) || flag){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        if(g_invalid_moves) g_invalid_moves = false;
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GUM
            effect = EFFECT_TBL(dest, SUM, sdata);
            BBA_AND(effect, BB_GUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv) || flag){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        if(g_invalid_moves) g_invalid_moves = false;
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GRY
            effect = EFFECT_TBL(dest, SRY, sdata);
            BBA_AND(effect, BB_GRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv) || flag){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        if(g_invalid_moves) g_invalid_moves = false;
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
        else             {  //先手番
            //SFU
            effect = EFFECT_TBL(dest, GFU, sdata);
            BBA_AND(effect, BB_SFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                            if(g_invalid_moves) g_invalid_moves = false;
                        }
                    }
                    if(SFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                            if(g_invalid_moves) g_invalid_moves = false;
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKY
            effect = EFFECT_TBL(dest, GKY, sdata);
            BBA_AND(effect, BB_SKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                            if(g_invalid_moves) g_invalid_moves = false;
                        }
                    }
                    if(SKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                            if(g_invalid_moves) g_invalid_moves = false;
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKE
            effect = EFFECT_TBL(dest, GKE, sdata);
            BBA_AND(effect, BB_SKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                            if(g_invalid_moves) g_invalid_moves = false;
                        }
                    }
                    if(SKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                            if(g_invalid_moves) g_invalid_moves = false;
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SGI
            effect = EFFECT_TBL(dest, GGI, sdata);
            BBA_AND(effect, BB_SGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SGI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv) || flag){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        if(g_invalid_moves) g_invalid_moves = false;
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKI,STO,SNY,SNK,SNG
            effect = EFFECT_TBL(dest, GKI, sdata);
            BBA_AND(effect, BB_STK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv) || flag){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        if(g_invalid_moves) g_invalid_moves = false;
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKA
            effect = EFFECT_TBL(dest, GKA, sdata);
            BBA_AND(effect, BB_SKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKA_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv) || flag){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        if(g_invalid_moves) g_invalid_moves = false;
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SHI
            effect = EFFECT_TBL(dest, GHI, sdata);
            BBA_AND(effect, BB_SHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SHI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv) || flag){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv) || flag){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        if(g_invalid_moves) g_invalid_moves = false;
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SUM
            effect = EFFECT_TBL(dest, GUM, sdata);
            BBA_AND(effect, BB_SUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv) || flag){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        if(g_invalid_moves) g_invalid_moves = false;
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SRY
            effect = EFFECT_TBL(dest, GRY, sdata);
            BBA_AND(effect, BB_SRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv) || flag){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        if(g_invalid_moves) g_invalid_moves = false;
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
    }
    return mlist;
}

mlist_t *mlist_to_destc       (mlist_t       *list,
                               int            dest,
                               const sdata_t *sdata )
{
    mlist_t *mlist = list;
    mlist_t *new_mlist;
    bitboard_t eff = SELF_EFFECT(sdata);
    int src;
    if(BPOS_TEST(eff,dest)){
        bitboard_t effect;
        move_t mv;
        if(S_TURN(sdata)){  //後手番
            //GFU
            effect = EFFECT_TBL(dest, SFU, sdata);
            BBA_AND(effect, BB_GFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(g_invalid_moves && enemy_effect(sdata,mv))
                            g_invalid_moves = false;
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    if(GFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(g_invalid_moves && enemy_effect(sdata,mv))
                            g_invalid_moves = false;
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKY
            effect = EFFECT_TBL(dest, SKY, sdata);
            BBA_AND(effect, BB_GKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(g_invalid_moves && enemy_effect(sdata,mv))
                            g_invalid_moves = false;
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    if(GKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(g_invalid_moves && enemy_effect(sdata,mv))
                            g_invalid_moves = false;
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKE
            effect = EFFECT_TBL(dest, SKE, sdata);
            BBA_AND(effect, BB_GKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(g_invalid_moves && enemy_effect(sdata,mv))
                            g_invalid_moves = false;
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    if(GKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(g_invalid_moves && enemy_effect(sdata,mv))
                            g_invalid_moves = false;
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GGI
            effect = EFFECT_TBL(dest, SGI, sdata);
            BBA_AND(effect, BB_GGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GGI_PROMOTE(src,dest)){
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    if(g_invalid_moves && enemy_effect(sdata,mv))
                        g_invalid_moves = false;
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKI,GTO,GNY,GNK,GNG
            effect = EFFECT_TBL(dest, SKI, sdata);
            BBA_AND(effect, BB_GTK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    MV_SET(mv,src,dest,0);
                    if(g_invalid_moves && enemy_effect(sdata,mv))
                        g_invalid_moves = false;
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKA
            effect = EFFECT_TBL(dest, SKA, sdata);
            BBA_AND(effect, BB_GKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKA_PROMOTE(src,dest)){
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    if(g_invalid_moves && enemy_effect(sdata,mv))
                        g_invalid_moves = false;
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GHI
            effect = EFFECT_TBL(dest, SHI, sdata);
            BBA_AND(effect, BB_GHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GHI_PROMOTE(src,dest)){
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    if(g_invalid_moves && enemy_effect(sdata,mv))
                        g_invalid_moves = false;
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GUM
            effect = EFFECT_TBL(dest, SUM, sdata);
            BBA_AND(effect, BB_GUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(g_invalid_moves && enemy_effect(sdata,mv))
                        g_invalid_moves = false;
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GRY
            effect = EFFECT_TBL(dest, SRY, sdata);
            BBA_AND(effect, BB_GRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(g_invalid_moves && enemy_effect(sdata,mv))
                        g_invalid_moves = false;
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
        else             {  //先手番
            //SFU
            effect = EFFECT_TBL(dest, GFU, sdata);
            BBA_AND(effect, BB_SFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(g_invalid_moves && enemy_effect(sdata,mv))
                            g_invalid_moves = false;
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    if(SFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(g_invalid_moves && enemy_effect(sdata,mv))
                            g_invalid_moves = false;
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKY
            effect = EFFECT_TBL(dest, GKY, sdata);
            BBA_AND(effect, BB_SKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(g_invalid_moves && enemy_effect(sdata,mv))
                            g_invalid_moves = false;
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    if(SKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(g_invalid_moves && enemy_effect(sdata,mv))
                            g_invalid_moves = false;
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKE
            effect = EFFECT_TBL(dest, GKE, sdata);
            BBA_AND(effect, BB_SKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(g_invalid_moves && enemy_effect(sdata,mv))
                            g_invalid_moves = false;
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    if(SKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(g_invalid_moves && enemy_effect(sdata,mv))
                            g_invalid_moves = false;
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SGI
            effect = EFFECT_TBL(dest, GGI, sdata);
            BBA_AND(effect, BB_SGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SGI_PROMOTE(src,dest)){
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    if(g_invalid_moves && enemy_effect(sdata,mv))
                        g_invalid_moves = false;
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKI,STO,SNY,SNK,SNG
            effect = EFFECT_TBL(dest, GKI, sdata);
            BBA_AND(effect, BB_STK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(g_invalid_moves && enemy_effect(sdata,mv))
                        g_invalid_moves = false;
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKA
            effect = EFFECT_TBL(dest, GKA, sdata);
            BBA_AND(effect, BB_SKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKA_PROMOTE(src,dest)){
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    if(g_invalid_moves && enemy_effect(sdata,mv))
                        g_invalid_moves = false;
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SHI
            effect = EFFECT_TBL(dest, GHI, sdata);
            BBA_AND(effect, BB_SHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SHI_PROMOTE(src,dest)){
                        MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                    }
                    MV_SET(mv,src,dest,0);
                    if(g_invalid_moves && enemy_effect(sdata,mv))
                        g_invalid_moves = false;
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SUM
            effect = EFFECT_TBL(dest, GUM, sdata);
            BBA_AND(effect, BB_SUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(g_invalid_moves && enemy_effect(sdata,mv))
                        g_invalid_moves = false;
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SRY
            effect = EFFECT_TBL(dest, GRY, sdata);
            BBA_AND(effect, BB_SRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(g_invalid_moves && enemy_effect(sdata,mv))
                        g_invalid_moves = false;
                    MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
    }
    return mlist;
}

mlist_t *mlist_to_destd        (mlist_t       *list,
                                int            dest,
                                const sdata_t *sdata,
                                tbase_t       *tbase)
{
    mlist_t *mlist = list;
    mlist_t *new_mlist;
    bitboard_t eff = SELF_EFFECT(sdata);
    int src;
    if(BPOS_TEST(eff,dest)){
        bitboard_t effect;
        move_t mv;
        if(S_TURN(sdata)){  //後手番
            //GFU
            effect = EFFECT_TBL(dest, SFU, sdata);
            BBA_AND(effect, BB_GFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    if(GFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKY
            effect = EFFECT_TBL(dest, SKY, sdata);
            BBA_AND(effect, BB_GKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    if(GKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKE
            effect = EFFECT_TBL(dest, SKE, sdata);
            BBA_AND(effect, BB_GKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    if(GKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GGI
            effect = EFFECT_TBL(dest, SGI, sdata);
            BBA_AND(effect, BB_GGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GGI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKI,GTO,GNY,GNK,GNG
            effect = EFFECT_TBL(dest, SKI, sdata);
            BBA_AND(effect, BB_GTK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKA
            effect = EFFECT_TBL(dest, SKA, sdata);
            BBA_AND(effect, BB_GKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GKA_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GHI
            effect = EFFECT_TBL(dest, SHI, sdata);
            BBA_AND(effect, BB_GHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(GHI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GUM
            effect = EFFECT_TBL(dest, SUM, sdata);
            BBA_AND(effect, BB_GUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //GRY
            effect = EFFECT_TBL(dest, SRY, sdata);
            BBA_AND(effect, BB_GRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
        else             {  //先手番
            //SFU
            effect = EFFECT_TBL(dest, GFU, sdata);
            BBA_AND(effect, BB_SFU(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SFU_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    if(SFU_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKY
            effect = EFFECT_TBL(dest, GKY, sdata);
            BBA_AND(effect, BB_SKY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKY_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    if(SKY_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKE
            effect = EFFECT_TBL(dest, GKE, sdata);
            BBA_AND(effect, BB_SKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKE_PROMOTE(dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    if(SKE_NORMAL(dest) ){
                        MV_SET(mv,src,dest,0);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                        }
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SGI
            effect = EFFECT_TBL(dest, GGI, sdata);
            BBA_AND(effect, BB_SGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SGI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKI,STO,SNY,SNK,SNG
            effect = EFFECT_TBL(dest, GKI, sdata);
            BBA_AND(effect, BB_STK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKA
            effect = EFFECT_TBL(dest, GKA, sdata);
            BBA_AND(effect, BB_SKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SKA_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SHI
            effect = EFFECT_TBL(dest, GHI, sdata);
            BBA_AND(effect, BB_SHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]){
                    if(SHI_PROMOTE(src,dest)){
                        MV_SET(mv,src,dest,1);
                        if(enemy_effect(sdata, mv)){
                            MLIST_SET_MOVE_PROM(mlist, new_mlist, src, dest);
                        }
                    }
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SUM
            effect = EFFECT_TBL(dest, GUM, sdata);
            BBA_AND(effect, BB_SUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
            //SRY
            effect = EFFECT_TBL(dest, GRY, sdata);
            BBA_AND(effect, BB_SRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) {
                    MV_SET(mv,src,dest,0);
                    if(enemy_effect(sdata, mv)){
                        MLIST_SET_MOVE_NORM(mlist, new_mlist, src, dest);
                    }
                }
                BBA_XOR(effect, g_bpos[src]);
            }
        }
    }
    return mlist;
}

#define MLIST_SET_DROP(mlist, new_mlist, koma, dest)  \
        (new_mlist) = mlist_alloc(),                  \
        (new_mlist)->move.prev_pos = HAND+(koma),     \
        (new_mlist)->move.new_pos = (dest),           \
        (new_mlist)->next = (mlist),                  \
        (mlist) = (new_mlist)

mlist_t *evasion_drop         (mlist_t *list,
                               int dest,
                               const sdata_t *sdata)
{
    mlist_t *mlist = list;
    mlist_t *new_mlist;
    //持ち駒による合い駒
    if(SELF_HI(sdata)) MLIST_SET_DROP(mlist, new_mlist, HI, dest);
    if(SELF_KA(sdata)) MLIST_SET_DROP(mlist, new_mlist, KA, dest);
    if(SELF_KI(sdata)) MLIST_SET_DROP(mlist, new_mlist, KI, dest);
    if(SELF_GI(sdata)) MLIST_SET_DROP(mlist, new_mlist, GI, dest);
    if(SELF_KE(sdata) && KE_CHECK(sdata, dest)){
        MLIST_SET_DROP(mlist, new_mlist, KE, dest);
    }
    if(SELF_KY(sdata) && KY_CHECK(sdata, dest)){
        MLIST_SET_DROP(mlist, new_mlist, KY, dest);
    }
    if(SELF_FU(sdata) && FU_CHECK(sdata, dest)){
        //王手になっていれば打歩詰チェック
        int ou = ENEMY_OU(sdata);
        if((S_TURN(sdata) && dest == (ou+DR_N)) ||
           (!S_TURN(sdata) && dest == (ou+DR_S))){
            move_t mv;
            MV_SET(mv, HAND+FU, dest, 0);
            if(fu_tsume_check(mv, sdata))
                MLIST_SET_DROP(mlist, new_mlist, FU, dest);
                
        }
        else{
            MLIST_SET_DROP(mlist, new_mlist, FU, dest);
        }
    }
    return mlist;
}
// ------------------------------------------
// 詰方玉が移動合開き王手で狙えない true
// ------------------------------------------
bool is_eou_offline           (const sdata_t *sdata)
{
    if(ENEMY_OU(sdata)==HAND) return true;
    
    uint8_t ou = ENEMY_OU(sdata);
    bitboard_t bb;
    if(S_TURN(sdata)){  //後手番
        //香
        BB_AND(bb,g_base_effect[SKY][ou],BB_GKY(sdata));
        if(BB_TEST(bb)) {
            int src,dest;
            while(1){
                dest = min_pos(&bb);
                if(dest<0) break;
                src = ou;
                //詰方玉と味方香の間の駒が全て味方の駒であればfalse
                while(1){
                    src+=DR_N;
                    if(src==dest) return false;
                    if(SENTE_KOMA(S_BOARD(sdata, src))) break;
                }
                BBA_XOR(bb, g_bpos[dest]);
            }
        }
        //角馬
        BB_AND(bb,g_base_effect[SKA][ou],BB_GUK(sdata));
        if(BB_TEST(bb)) {
            int src, dest, dir;
            while(1){
                dest = min_pos(&bb);
                if(dest<0) break;
                //味方の大駒と詰方玉の位置関係より、方向を特定。
                src = ou;
                if(src>dest){
                    if(!((src-dest)%DR_SW)) dir = DR_NE;
                    else dir = DR_NW;
                }
                else{
                    if(!((dest-src)%DR_SW)) dir = DR_SW;
                    else dir = DR_SE;
                }
                //詰方玉と味方大駒の間の駒が全て味方の駒であればfalse
                while(1){
                    src+=dir;
                    if(src==dest) return false;
                    if(SENTE_KOMA(S_BOARD(sdata, src))) break;
                }
                BBA_XOR(bb, g_bpos[dest]);
            }
        }
        //飛龍
        BB_AND(bb,g_base_effect[SHI][ou],BB_GRH(sdata));
        if(BB_TEST(bb)) {
            int src, dest, dir;
            while(1){
                dest = min_pos(&bb);
                if(dest<0) break;
                //味方の大駒と詰方玉の位置関係より、方向を特定。
                src = ou;
                if(src>dest){
                    if((src-dest)<DR_S) dir = DR_E;
                    else dir = DR_N;
                }
                else{
                    if((dest-src)<DR_S) dir = DR_W;
                    else dir = DR_S;
                }
                //詰方玉と味方大駒の間の駒が全て味方の駒であればfalse
                while(1){
                    src+=dir;
                    if(src==dest) return false;
                    if(SENTE_KOMA(S_BOARD(sdata, src))) break;
                }
                BBA_XOR(bb, g_bpos[dest]);
            }
        }
    }
    else             {
        //香
        BB_AND(bb,g_base_effect[GKY][ou],BB_SKY(sdata));
        if(BB_TEST(bb)) {
            int src,dest;
            while(1){
                dest = min_pos(&bb);
                if(dest<0) break;
                src = ou;
                //詰方玉と味方香の間の駒が全て味方の駒であればfalse
                while(1){
                    src+=DR_S;
                    if(src==dest) return false;
                    if(SENTE_KOMA(S_BOARD(sdata, src))) break;
                }
                BBA_XOR(bb, g_bpos[dest]);
            }
        }
        //角馬
        BB_AND(bb,g_base_effect[GKA][ou],BB_SUK(sdata));
        if(BB_TEST(bb)) {
            int src, dest, dir;
            while(1){
                dest = min_pos(&bb);
                if(dest<0) break;
                //味方の大駒と詰方玉の位置関係より、方向を特定。
                src = ou;
                if(src>dest){
                    if(!((src-dest)%DR_SW)) dir = DR_NE;
                    else dir = DR_NW;
                }
                else{
                    if(!((dest-src)%DR_SW)) dir = DR_SW;
                    else dir = DR_SE;
                }
                //詰方玉と味方大駒の間の駒が全て味方の駒であればfalse
                while(1){
                    src+=dir;
                    if(src==dest) return false;
                    if(GOTE_KOMA(S_BOARD(sdata, src))) break;
                }
                BBA_XOR(bb, g_bpos[dest]);
            }
        }
        //飛龍
        BB_AND(bb,g_base_effect[GHI][ou],BB_SRH(sdata));
        if(BB_TEST(bb)) {
            int src, dest, dir;
            while(1){
                dest = min_pos(&bb);
                if(dest<0) break;
                //味方の大駒と詰方玉の位置関係より、方向を特定。
                src = ou;
                if(src>dest){
                    if((src-dest)<DR_S) dir = DR_E;
                    else dir = DR_N;
                }
                else{
                    if((dest-src)<DR_S) dir = DR_W;
                    else dir = DR_S;
                }
                //詰方玉と味方大駒の間の駒が全て味方の駒であればfalse
                while(1){
                    src+=dir;
                    if(src==dest) return false;
                    if(GOTE_KOMA(S_BOARD(sdata, src))) break;
                }
                BBA_XOR(bb, g_bpos[dest]);
            }
        }
    }
    return true;
}

// -------------------------------------------------------
// 有効な中合の判定基準
//    ・自玉より２マス離れた位置
//    ・詰方の角の利きがある
//    ・王手している駒は飛車か香車である
// -------------------------------------------------------

bool is_hand_effective             (const sdata_t *sdata,
                                    int            dest  )
{
    //destの位置に詰方の角が効いている
    bitboard_t effect = EFFECT_TBL(dest, SKA, sdata);
    bitboard_t ukbb = S_TURN(sdata)?BB_SUK(sdata):BB_GUK(sdata);
    BBA_AND(effect, ukbb);
    if(!BB_TEST(effect)) return false;
    
    //王手している駒が香または飛車
    komainf_t attack = S_BOARD(sdata,S_ATTACK(sdata)[0]);
    if(S_TURN(sdata)){  //後手
        if(attack != SHI && attack != SKY) return false;
    } else{             //先手
        if(attack != GHI && attack != GKY) return false;
    }

    //玉の位置から２マス目
    uint8_t d = g_distance[SELF_OU(sdata)][dest];
    if(d >2) return false;

    return true;
}

#define PIN_CHECK(sdata, effect, src)              \
        while(1){                                  \
            (src) = min_pos(&(effect));            \
            if((src)<0) break;                     \
            if(!S_PINNED(sdata)[src]) return true; \
            BBA_XOR(effect, g_bpos[src]);          \
        }

// -----------------------------------------------------
// destへの利きが有効か
// 有効（pinされていない）　true
// 無効（pinされている）   false
// -----------------------------------------------------
bool is_dest_effect_valid          (const sdata_t *sdata,
                                    int            dest )
{
    int src;
    bitboard_t effect;
    //destに利いてる駒がpinされていないことを確認する。
    if(S_TURN(sdata)){
        //GFU
        effect = EFFECT_TBL(dest, SFU, sdata);
        BBA_AND(effect, BB_GFU(sdata));
        PIN_CHECK(sdata, effect, src);
        //GKY
        effect = EFFECT_TBL(dest, SKY, sdata);
        BBA_AND(effect, BB_GKY(sdata));
        PIN_CHECK(sdata, effect, src);
        //GKE
        effect = EFFECT_TBL(dest, SKE, sdata);
        BBA_AND(effect, BB_GKE(sdata));
        PIN_CHECK(sdata, effect, src);
        //GGI
        effect = EFFECT_TBL(dest, SGI, sdata);
        BBA_AND(effect, BB_GGI(sdata));
        PIN_CHECK(sdata, effect, src);
        //GKI,GTO,GNY,GNK,GNG
        effect = EFFECT_TBL(dest, SKI, sdata);
        BBA_AND(effect, BB_GTK(sdata));
        PIN_CHECK(sdata, effect, src);
        //GKA
        effect = EFFECT_TBL(dest, SKA, sdata);
        BBA_AND(effect, BB_GKA(sdata));
        PIN_CHECK(sdata, effect, src);
        //GHI
        effect = EFFECT_TBL(dest, SHI, sdata);
        BBA_AND(effect, BB_GHI(sdata));
        PIN_CHECK(sdata, effect, src);
        //GUM
        effect = EFFECT_TBL(dest, SUM, sdata);
        BBA_AND(effect, BB_GUM(sdata));
        PIN_CHECK(sdata, effect, src);
        //GRY
        effect = EFFECT_TBL(dest, SRY, sdata);
        BBA_AND(effect, BB_GRY(sdata));
        PIN_CHECK(sdata, effect, src);
    }
    else{
        //SFU
        effect = EFFECT_TBL(dest, GFU, sdata);
        BBA_AND(effect, BB_SFU(sdata));
        PIN_CHECK(sdata, effect, src);
        //SKY
        effect = EFFECT_TBL(dest, GKY, sdata);
        BBA_AND(effect, BB_SKY(sdata));
        PIN_CHECK(sdata, effect, src);
        //SKE
        effect = EFFECT_TBL(dest, GKE, sdata);
        BBA_AND(effect, BB_SKE(sdata));
        PIN_CHECK(sdata, effect, src);
        //SGI
        effect = EFFECT_TBL(dest, GGI, sdata);
        BBA_AND(effect, BB_SGI(sdata));
        PIN_CHECK(sdata, effect, src);
        //SKI,STO,SNY,SNK,SNG
        effect = EFFECT_TBL(dest, GKI, sdata);
        BBA_AND(effect, BB_STK(sdata));
        PIN_CHECK(sdata, effect, src);
        //SKA
        effect = EFFECT_TBL(dest, GKA, sdata);
        BBA_AND(effect, BB_SKA(sdata));
        PIN_CHECK(sdata, effect, src);
        //SHI
        effect = EFFECT_TBL(dest, GHI, sdata);
        BBA_AND(effect, BB_SHI(sdata));
        PIN_CHECK(sdata, effect, src);
        //SUM
        effect = EFFECT_TBL(dest, GUM, sdata);
        BBA_AND(effect, BB_SUM(sdata));
        PIN_CHECK(sdata, effect, src);
        //SRY
        effect = EFFECT_TBL(dest, GRY, sdata);
        BBA_AND(effect, BB_SRY(sdata));
        PIN_CHECK(sdata, effect, src);
    }
    return false;
}

// ----------------------------------------------------------------
// 大駒の利きが交差する場所に合駒をした場合、逃れが発生する場合があります。
// cross_point_checkは逃れ発生の有無を検知し、以下の出力を返します。
// [逃れ例]
// 後手の持駒：飛　角　金三　銀三　桂　香四　歩十三
// ９ ８ ７ ６ ５ ４ ３ ２ １
// +---------------------------+
// | ・ ・ ・ ・ ・v金 ・ ・ ・|一
// | ・ ・ ・ ・ ・ ・ ・ ・ ・|二
// | ・ ・ ・ 歩 ・ 桂 ・v桂v銀|三
// | 飛 ・ ・ ・ ・ ・ ・v王 ・|四
// | ・v歩 ・ ・ ・ ・v歩v歩v歩|五
// | ・ ・ ・ ・ ・ 桂 ・ ・ ・|六
// | ・ ・ ・ ・ ・ ・ ・ ・ ・|七
// | ・ ・ ・ ・ ・ ・ ・ ・ ・|八
// | 馬 ・ ・ ・ ・ ・ ・ ・ ・|九
// +---------------------------+
// 先手の持駒：なし
//
// 逃れあり：true
// 逃れなし：false
// ----------------------------------------------------------------

/* ----------------------------------------------------------------
 大駒の利きが交差する場所に合駒をした場合、逃れが発生する場合があります。
 cross_point_checkは逃れ発生の有無を検知し、以下の出力を返します。
 [例1]
 後手の持駒：飛　角　金三　銀三　桂　香四　歩十三
 ９ ８ ７ ６ ５ ４ ３ ２ １
 +---------------------------+
 | ・ ・ ・ ・ ・v金 ・ ・ ・|一
 | ・ ・ ・ ・ ・ ・ ・ ・ ・|二
 | ・ ・ ・ 歩 ・ 桂 ・v桂v銀|三
 | 飛 ・ ・ ・ ・ ⚪︎ ・v王 ・|四
 | ・v歩 ・ ・ ・ ・v歩v歩v歩|五
 | ・ ・ ・ ・ ・ 桂 ・ ・ ・|六
 | ・ ・ ・ ・ ・ ・ ・ ・ ・|七
 | ・ ・ ・ ・ ・ ・ ・ ・ ・|八
 | 馬 ・ ・ ・ ・ ・ ・ ・ ・|九
 +---------------------------+
 先手の持駒：なし
 
 ⚪︎は有効合  ->true
 
 
 後手の持駒：飛　角　金三　銀三　桂　香四　歩十二
  ９ ８ ７ ６ ５ ４ ３ ２ １
 +---------------------------+
 | ・ ・ ・ ・ ・v金 ・ ・ ・|一
 | ・ ・ ・ ・ ・ ・ ・ ・ ・|二
 | ・ ・ と 歩 ・ 桂 ・v桂v銀|三
 | 飛 ・ ・ ・ ・ ⚪︎ ・v王 ・|四
 | ・v歩 ・ ・ ・ ・v歩v歩v歩|五
 | ・ ・ ・ ・ ・ 桂 ・ ・ ・|六
 | ・ ・ ・ ・ ・ ・ ・ ・ ・|七
 | ・ ・ ・ ・ ・ ・ ・ ・ ・|八
 | 馬 ・ ・ ・ ・ ・ ・ ・ ・|九
 +---------------------------+
 先手の持駒：なし

 ⚪︎は無駄合 -> false
 
  逃れあり：true
  逃れなし：false
 ---------------------------------------------------------------- */

bool cross_point_check             (const sdata_t *sdata,
                                    int            dest,
                                    tbase_t       *tbase)
{
    //destの位置に詰方の角が効いている
    bitboard_t effect = EFFECT_TBL(dest, SKA, sdata);
    bitboard_t ukbb = S_TURN(sdata)?BB_SUK(sdata):BB_GUK(sdata);
    BBA_AND(effect, ukbb);
    if(!BB_TEST(effect)) return false;
    
    //王手している駒を確かめる
    komainf_t attack = S_BOARD(sdata,S_ATTACK(sdata)[0]);
    int src = S_ATTACK(sdata)[0];
    //後手番
    if(S_TURN(sdata)){
        //王手している駒が飛車且つdestで成れない場合
        //王手している駒が香車の場合
        if((attack == SHI && !SHI_PROMOTE(src, dest)) ||
           (attack == SKY))
        {
            //destに王手している駒を移動させてみる。
            //詰んでいれば無駄合。そうでなければ有効合い
            sdata_t sbuf;
            memcpy(&sbuf, sdata, sizeof(sdata_t));
            sdata_tentative_move(&sbuf, src, dest, false);
            
            //詰みチェックで詰んでいればfalse（無駄合）
            if(ou_move_check(&sbuf)) return false;
            
            //ハッシュ探索して詰んでなければ有効合い
            mvlist_t mvlist;
            memset(&mvlist, 0, sizeof(mvlist_t));
            memcpy(&(mvlist.tdata), &g_tdata_init, sizeof(tdata_t));
            tbase_lookup(&sbuf, &mvlist, S_TURN(&sbuf), tbase);
            if(mvlist.tdata.pn) return true;
        }
    }
    
    //先手番
    else{
        //王手している駒が飛車且つdestで成れない場合
        //王手している駒が香車の場合
        if((attack == GHI && !GHI_PROMOTE(src, dest)) ||
           (attack == GKY))
        {
            //destに王手している駒を移動させてみる。
            //詰んでいれば無駄合。そうでなければ有効合い
            sdata_t sbuf;
            memcpy(&sbuf, sdata, sizeof(sdata_t));
            sdata_tentative_move(&sbuf, src, dest, false);
            
            //詰みチェックで詰んでいればfalse（無駄合）
            if(tsumi_check(&sbuf)) return false;
            
            //ハッシュ探索して詰んでなければ有効合い
            mvlist_t mvlist;
            memset(&mvlist, 0, sizeof(mvlist_t));
            memcpy(&(mvlist.tdata), &g_tdata_init, sizeof(tdata_t));
            tbase_lookup(&sbuf, &mvlist, S_TURN(&sbuf), tbase);
            if(mvlist.tdata.pn) return true;
        }
    }
    return false;
}
