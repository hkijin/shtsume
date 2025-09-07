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
 ----------------------------------------------------------------------- */

mvlist_t *generate_evasion   (const sdata_t *sdata,
                              tbase_t       *tbase  )
{
    g_invalid_drops = false;
    g_invalid_moves = false;
    mvlist_t *mvlist = NULL;
    //玉の移動
    int ou = SELF_OU(sdata);
    komainf_t koma;
    bitboard_t move_bb = evasion_bb(sdata);
    int dest;
    mlist_t *mlist,          //持ち駒合リスト
            *mslist;         //移動合直列リスト
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
    
    // -----------------
    // 合い駒
    // -----------------
    
    mlist_t *drop_list = NULL;         //合駒着手構成用
    mlist_t *move_list = NULL;         //移動合着手構成用
    bool next_ou = true;               //合駒の位置が玉の隣の場合、true
    bool tflag = false;                //詰み可能性flag true:詰み可能性あり
    bool nflag = false;                //適切な持ち駒なく、合駒不可の場合 true
    bool move = (mvlist)?true:false;   //合駒以外の着手の有無を示すflag.
    mlist_t *last;
    mvlist_t *mvlast;
    
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
    
    if(dir){
        dest = src;
        while(true){
            dest += dir;
            if(dest == S_ATTACK(sdata)[0]) break;
            //合い駒があれば着手に追加
            mlist = NULL;
            if(next_ou){
                if(!invalid_drops(sdata, dest, tbase))
                {
                    mlist = evasion_drop(mlist, dest, sdata);
                    //適切な持ち駒がなく、合駒着手生成できない場合、nflagを立てる
                    if(!mlist)  nflag = true;
                }
                //合駒以外に王手回避の手段がなく、玉の最近接合が不可能の場合、
                //詰みの可能性があるのでtflagを立てておく。
                if(!mvlist) tflag = true;
            }
            else     {
                //合駒が有効となる条件を満たせばtflagを解除する
                if(BPOS_TEST(SELF_EFFECT(sdata),dest)){
                    if(is_dest_effect_valid(sdata, dest))
                        //打てる持ち駒があれば
                        if(HAND_CHECK(sdata, dest))
                            tflag = false;
                }
                    
                if(!tflag || is_hand_effective(sdata, dest)){
                    mlist = evasion_drop(mlist, dest, sdata);
                    //適切な持ち駒がなく、合駒着手が生成できない場合、flagを立てておく
                    if(!mlist) nflag = true;
                    //else nflag = false;
                }
            }
            
            //移動合いがあれば着手に追加
            mslist = NULL;
            //詰方玉がない、または詰方玉に移動合開き王手の脅威がない場合
            if(is_eou_offline(sdata)){
                if(next_ou){
                    if(!move && !mlist) //合駒以外の着手、持ち駒合着手が存在しない場合
                        mslist = mlist_to_desta(mslist, dest, sdata, tbase);
                    else
                        mvlist = move_to_dest(mvlist, dest, sdata);
                }
                else       {
                    if( !move && nflag && !drop_list && !mlist )
                        //合駒以外の着手なし
                        //最近接位置で適切な合駒ない
                        //持ち駒による合駒の生成なし
                        mslist = mlist_to_destb(mslist, dest, move, sdata);
                    else
                        mvlist = move_to_dest(mvlist, dest, sdata);
                }
            }
            //詰方玉があり、移動合開き王手の脅威がある場合、
            else{
                if(next_ou){
                    if(!move && !mlist)
                        mslist = mlist_to_desta(mslist, dest, sdata, tbase);
                    else
                        mvlist = move_to_dest(mvlist, dest, sdata);
                }
                else       {
                    if(!move && nflag && !drop_list && !mlist )
                        mslist = mlist_to_destc(mslist, dest, sdata);
                    else
                        mvlist = move_to_dest(mvlist, dest, sdata);
                }
            }
            next_ou = false;
            //持ち駒合
            if(!drop_list) drop_list = mlist;
            else {
                last = mlist_last(drop_list);
                last->next = mlist;
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
    //合駒以外の手があれば移動無駄合判定flagはfalseとしておく。
    if(move) g_invalid_moves = false;
    
    return mvlist;
}

/* --------------------------------
 move_to_dest　（destに駒あり）
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
