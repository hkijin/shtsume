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
static bool is_ou_online           (const sdata_t *sdata);

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
    bool nflag = false;                //最近接の位置で合駒できない場合 true
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
                if(invalid_drops(sdata, dest, tbase))
                {
                    //合駒以外に王手回避の手段がなく、玉の最近接位置の合駒が無駄合の場合、
                    //詰みの可能性があるのでflagを立てておく。
                    if(!mvlist) tflag = true;
                }
                else
                {
                    mlist = evasion_drop(mlist, dest, sdata);
                    //適切な持ち駒がなく、合駒着手が生成できない場合、flagを立てておく
                    if(!mlist) nflag = true;
                }
            }
            else     {
                //もしtflagがtrueでdestに玉方の利きがなければ無駄合とする。
                if(BPOS_TEST(SELF_EFFECT(sdata),dest)) 
                    tflag = false;
                if(!tflag){
                    mlist = evasion_drop(mlist, dest, sdata);
                    //適切な持ち駒がなく、合駒着手が生成できない場合、flagを立てておく
                    if(!mlist) nflag = true;
                }
            }
            
            //移動合いがあれば着手に追加
            mslist = NULL;
            //詰方玉に移動合開き王手の脅威がない場合
            if(is_ou_online(sdata)){
                if(next_ou){
                    if(!move && !mlist)
                        mslist = mlist_to_desta(mslist, dest, sdata, tbase);
                    else
                        mvlist = move_to_dest(mvlist, dest, sdata);
                }
                else       {
                    if( !move && nflag && !drop_list && !mlist )
                        mslist = mlist_to_destb(mslist, dest, move, sdata);
                    else
                        mvlist = move_to_dest(mvlist, dest, sdata);
                }
            }
            //詰方玉がある場合
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
        MLIST_SET_DROP(mlist, new_mlist, FU, dest);
    }
    return mlist;
}
// ------------------------------------------
// 詰方玉が移動合開き王手で狙えない true
// ------------------------------------------
bool is_ou_online           (const sdata_t *sdata)
{
    if(ENEMY_OU(sdata)==HAND) return true;
    
    uint8_t ou = ENEMY_OU(sdata);
    bitboard_t bb;
    if(S_TURN(sdata)){  //後手番
        //香
        BB_AND(bb,g_base_effect[SKY][ou],BB_GKY(sdata));
        if(BB_TEST(bb)) return false;
        //角馬
        BB_AND(bb,g_base_effect[SKA][ou],BB_GUK(sdata));
        if(BB_TEST(bb)) return false;
        //飛龍
        BB_AND(bb,g_base_effect[SHI][ou],BB_GRH(sdata));
        if(BB_TEST(bb)) return false;
    }
    else             {
        //香
        BB_AND(bb,g_base_effect[GKY][ou],BB_SKY(sdata));
        if(BB_TEST(bb)) return false;
        //角馬
        BB_AND(bb,g_base_effect[GKA][ou],BB_SUK(sdata));
        if(BB_TEST(bb)) return false;
        //飛龍
        BB_AND(bb,g_base_effect[GHI][ou],BB_SRH(sdata));
        if(BB_TEST(bb)) return false;
    }
    return true;
}
