//
//  nevasion.c
//  nsolver
//
//  Created by Hkijin on 2022/02/03.
//

#include <stdio.h>
#include "shtsume.h"

bool g_invalid_drops;
//generate_evasionが局面表を見て無駄合いと判定した手がある場合 true;

static mvlist_t *move_to_dest      (mvlist_t      *list,
                                    int            dest,
                                    const sdata_t *sdata);
static mlist_t *evasion_drop       (mlist_t       *list,
                                    int            dest,
                                    const sdata_t *sdata);

/* ------------------------------------------------------------------------
 generate_evasion
 局面表による無駄合い判定付き王手回避着手生成関数
 [引数]
 tbase_t *tbase  局面表tbaseへのポインタ
 [戻り値]
 無駄合いを除く王手回避着手
 g_invalid_drops
 局面表による無駄合い判定あり    true
           無駄合い判定なし    false
 ----------------------------------------------------------------------- */

mvlist_t *generate_evasion   (const sdata_t *sdata,
                              tbase_t       *tbase  )
{
    g_invalid_drops = false;
    mvlist_t *mvlist = NULL;
    //玉の移動
    komainf_t koma;
    bitboard_t move_bb; //玉が動ける場所がtrueのbitboard
    /* ------------------------------------------------------------------
     大駒や香車による王手の場合、玉の位置によって攻め方利きが変化します。
     例えば飛車の王手に対し、飛車から離れるように横方向に動く手は王手回避になりません。
     この対策として事前に自玉を外したoccupied bitboard　を作り、これを用いて攻め方
     利きを再計算しておくことが必要になります。
     ------------------------------------------------------------------ */
    int ou = SELF_OU(sdata);
    bitboard_t occupied  = BB_OCC(sdata);  BB_UNSET(occupied,ou);
    bitboard_t voccupied = BB_VOC(sdata); VBB_UNSET(voccupied,ou);
    bitboard_t roccupied = BB_ROC(sdata); RBB_UNSET(roccupied,ou);
    bitboard_t loccupied = BB_LOC(sdata); LBB_UNSET(loccupied,ou);
    koma = S_BOARD(sdata, S_ATTACK(sdata)[0]);
    bitboard_t effect = effect_tbl(S_ATTACK(sdata)[0], koma,
                                   &occupied,
                                   &voccupied,
                                   &roccupied,
                                   &loccupied);
    bitboard_t effect1 = {0,0,0};
    if(S_NOUTE(sdata)==2){
        koma = S_BOARD(sdata, S_ATTACK(sdata)[1]);
        effect1 = effect_tbl(S_ATTACK(sdata)[1], koma,
                                    &occupied,
                                    &voccupied,
                                    &roccupied,
                                    &loccupied);
    }
    
    if(S_TURN(sdata)){ //後手番
        move_bb = g_base_effect[GOU][S_GOU(sdata)];
        BBA_ANDNOT(move_bb, BB_GOC(sdata));
        BBA_ANDNOT(move_bb, SEFFECT(sdata));
        BBA_ANDNOT(move_bb, effect );
        BBA_ANDNOT(move_bb, effect1);
    }
    else{  //先手番
        move_bb = g_base_effect[SOU][S_SOU(sdata)];
        BBA_ANDNOT(move_bb, BB_SOC(sdata));
        BBA_ANDNOT(move_bb, GEFFECT(sdata));
        BBA_ANDNOT(move_bb, effect );
        BBA_ANDNOT(move_bb, effect1);
    }
    
    int dest;
    mlist_t *mlist;
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
    mvlist = move_to_dest(mvlist, dest, sdata);
    
    //王手している駒が飛び駒で無ければここで終了
    koma = S_BOARD(sdata, dest);
    if(g_is_skoma[koma]) return mvlist;
    
    //合い駒
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
    mlist_t *drop_list = NULL, *last;
    mvlist_t *mvlast;
    bool next_ou = true;
    if(dir){
        dest = src;
        while(true){
            dest += dir;
            if(dest == S_ATTACK(sdata)[0]) break;
            //移動合いがあれば着手に追加
            mvlist = move_to_dest(mvlist, dest, sdata);
            //合い駒があれば着手に追加
            mlist = NULL;
            if(next_ou){
                if(invalid_drops(sdata, dest)){}
                else if(hs_invalid_drops
                        (sdata, S_ATTACK(sdata)[0], dest, tbase))
                {
                    g_invalid_drops = true;
                }
                else
                    mlist = evasion_drop(mlist, dest, sdata);
            }
            else     {
                if(hs_invalid_drops(sdata, S_ATTACK(sdata)[0], dest, tbase))
                {
                    g_invalid_drops = true;
                }
                else {
                    mlist = evasion_drop(mlist, dest, sdata);
                }
            }
            
            next_ou = false;
            if(!drop_list) drop_list = mlist;
            else {
                last = mlist_last(drop_list);
                last->next = mlist;
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

#define MLIST_SET_DROP(mlist, new_mlist, koma, dest)  \
        (new_mlist) = mlist_alloc(),                  \
        (new_mlist)->move.prev_pos = HAND+(koma),     \
        (new_mlist)->move.new_pos = (dest),           \
        (new_mlist)->next = (mlist),                  \
        (mlist) = (new_mlist)

mlist_t *evasion_drop         (mlist_t *list,
                               int dest,
                               const sdata_t *sdata){
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
        MLIST_SET_DROP(mlist, new_mlist, FU, dest);
    }
    return mlist;
}
