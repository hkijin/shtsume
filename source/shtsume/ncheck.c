//
//  ncheck.c
//  shtsume
//
//  Created by Hkijin on 2022/02/03.
//

#include <stdio.h>
#include <assert.h>
#include "shtsume.h"
#include "dtools.h"

/* ---------------------------------------------------------------------------
 香の場合(例) l: GKY     k: G**     o: SOU
 1,GKYの利きとSOUをSKYに見立てての利きが交わること
 2,kが味方の駒であること
 3,kの位置に対応したpinを記録
 
  9 8 7 6 5 4 3 2 1
+-------------------+
| . . . . l . . . . | 1
| . . . . + . . . . | 2
| . . . . + . . . . | 3
| . . . . k . . . . | 4
| . . . . | . . . . | 5
| . . . . | . . . . | 6
| . . . . | . . . . | 7
| . . . . | . . . . | 8
| . . . . O . . . . | 9
+-------------------+
 角の場合（例) l: GKA     k: G**     o: SOU
 1,GKAの利きとSOUをSKAに見立てての利きが交わること
 2,kが味方の駒であること
 3,kとOとの位置関係より対応pinを割り出す。
 4,kの位置に対応したpinを記録。
 
 角の場合
 
  9 8 7 6 5 4 3 2 1            9 8 7 6 5 4 3 2 1
+-------------------+        +-------------------+
| . . . . . . . . b | 1      | b . . . . . . . . | 1
| . . . . . . . + . | 2      | . + . . . . . . . | 2
| . . . . . . + . . | 3      | . . + . . . . . . | 3
| . . . . . k . . . | 4      | . . . k . . . . . | 4
| . . . . / . . . . | 5      | . . . . \ . . . . | 5
| . . . / . . . . . | 6      | . . . . . \ . . . | 6
| . . / . . . . . . | 7      | . . . . . . \ . . | 7
| . / . . . . . . . | 8      | . . . . . . . \ . | 8
| O . . . . . . . . | 9      | . . . . . . . . O | 9
+-------------------+        +-------------------+
 
 手順
 1,kの位置とpinの方向を記録しておく
   static discpin_t st_discにpin_idを保管
 2,盤上駒の王手探索時にor処理で空き王手になる着手を加える。
   BB_(koma)
---------------------------------------------------------------------------- */

static discpin_t st_disc;
static mvlist_t* evasion_check  (const sdata_t *sdata, tbase_t *tbase);

mvlist_t* generate_check        (const sdata_t *sdata,
                                 tbase_t       *tbase )
{
    //自玉が王手の場合
    if(S_NOUTE(sdata)) return evasion_check(sdata, tbase);
    
    mvlist_t *mvlist = NULL;
    mlist_t  *mlist;
    int src, dest, ou;
    bitboard_t  src_bb,      //駒の位置bitboard
                dest_bb,     //駒が来れる位置のbitboard
                ou_bb,       //駒が来た場合、直接王手になる位置のbitboard
                pou_bb,      //駒がなった場合、直接王手になる位置のbitboard
                disc_bb,     //空き王手になる移動点のbitboard
                norm_bb,     //不成で王手になる位置のbitboard
                prom_bb;     //成りで王手になる位置のbitboard
    //st_disc構造体に空き王手情報を記録しておく
    set_discpin(sdata, &st_disc);
    //後手番
    if(S_TURN(sdata)){
        //無仕掛けチェック
        if(!BB_TEST(BB_GOC(sdata))){
            if(!S_GMKEY(sdata).hi && !S_GMKEY(sdata).ka && !S_GMKEY(sdata).ke && !S_GMKEY(sdata).ky)
                return NULL;
        }
        ou = S_SOU(sdata);
        //持ち駒による王手
        if(GMKEY_FU(sdata) && ou>8)  {
            dest_bb = EFFECT_TBL(ou, SFU, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                if(FU_CHECK(sdata, dest)){
                    //打歩詰めチェック
                    move_t mv = {HAND+FU, dest};
                    if(fu_tsume_check(mv, sdata)){
                        MVLIST_SET_NORM(mvlist, mlist, HAND+FU, dest);
                    }
                }
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        if(GMKEY_KY(sdata) && ou>8)  {
            dest_bb = EFFECT_TBL(ou, SKY, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, HAND+KY, dest);
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        if(GMKEY_KE(sdata) && ou>17) {
            dest_bb = EFFECT_TBL(ou, SKE, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, HAND+KE, dest);
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        if(GMKEY_GI(sdata))          {
            dest_bb = EFFECT_TBL(ou, SGI, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, HAND+GI, dest);
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        if(GMKEY_KI(sdata))          {
            dest_bb = EFFECT_TBL(ou, SKI, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, HAND+KI, dest);
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        if(GMKEY_KA(sdata))          {
            dest_bb = EFFECT_TBL(ou, SKA, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, HAND+KA, dest);
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        if(GMKEY_HI(sdata))          {
            dest_bb = EFFECT_TBL(ou, SHI, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, HAND+HI, dest);
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        //盤上駒による直接王手,空き王手
        //GFU
        ou_bb  = EFFECT_TBL(ou, SFU, sdata);
        pou_bb = EFFECT_TBL(ou, STO, sdata);
        src_bb = BB_GFU(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, GFU, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_GOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            dest = min_pos(&norm_bb);
            if(dest>=0 && GFU_NORMAL(dest))
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
            //成り
            BB_AND(prom_bb, dest_bb, pou_bb);
            BBA_OR(prom_bb, disc_bb);
            BBA_ANDNOT(prom_bb, BB_GOC(sdata));
            BBA_AND(prom_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            dest = min_pos(&prom_bb);
            if(dest>=0 && GFU_PROMOTE(dest))
                MVLIST_SET_PROM(mvlist, mlist, src, dest);
            
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GKY
        ou_bb  = EFFECT_TBL(ou, SKY, sdata);
        pou_bb = EFFECT_TBL(ou, SNY, sdata);
        src_bb = BB_GKY(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, GKY, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_GOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                if(GKY_NORMAL(dest))
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            //成り
            BB_AND(prom_bb, dest_bb, pou_bb);
            BBA_OR(prom_bb, disc_bb);
            BBA_ANDNOT(prom_bb, BB_GOC(sdata));
            BBA_AND(prom_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&prom_bb);
                if(dest<0) break;
                if(GKY_PROMOTE(dest))
                    MVLIST_SET_PROM(mvlist, mlist, src, dest);
                BBA_XOR(prom_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GKE
        ou_bb  = EFFECT_TBL(ou, SKE, sdata);
        pou_bb = EFFECT_TBL(ou, SNK, sdata);
        src_bb = BB_GKE(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, GKE, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_GOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                if(GKE_NORMAL(dest))
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            //成り
            BB_AND(prom_bb, dest_bb, pou_bb);
            BBA_OR(prom_bb, disc_bb);
            BBA_ANDNOT(prom_bb, BB_GOC(sdata));
            BBA_AND(prom_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&prom_bb);
                if(dest<0) break;
                if(GKE_PROMOTE(dest))
                    MVLIST_SET_PROM(mvlist, mlist, src, dest);
                BBA_XOR(prom_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GGI
        ou_bb  = EFFECT_TBL(ou, SGI, sdata);
        pou_bb = EFFECT_TBL(ou, SNG, sdata);
        src_bb = BB_GGI(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, GGI, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_GOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            //成り
            BB_AND(prom_bb, dest_bb, pou_bb);
            BBA_OR(prom_bb, disc_bb);
            BBA_ANDNOT(prom_bb, BB_GOC(sdata));
            BBA_AND(prom_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&prom_bb);
                if(dest<0) break;
                if(GGI_PROMOTE(src, dest))
                    MVLIST_SET_PROM(mvlist, mlist, src, dest);
                BBA_XOR(prom_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GKI,GTO,GNY,GNK,GNG
        ou_bb  = EFFECT_TBL(ou, SKI, sdata);
        src_bb = BB_GTK(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, GKI, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_GOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GOU
        src = S_GOU(sdata);
        if(src<N_SQUARE){
            norm_bb = EFFECT_TBL(src, GOU, sdata);
            BBA_ANDNOT(norm_bb, g_bb_pin[st_disc.pin[src]]);
            BBA_ANDNOT(norm_bb, BB_GOC(sdata));
            BBA_ANDNOT(norm_bb, SEFFECT(sdata));
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
        }
        //GKA
        ou_bb  = EFFECT_TBL(ou, SKA, sdata);
        pou_bb = EFFECT_TBL(ou, SUM, sdata);
        src_bb = BB_GKA(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, GKA, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_GOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            //成り
            BB_AND(prom_bb, dest_bb, pou_bb);
            BBA_OR(prom_bb, disc_bb);
            BBA_ANDNOT(prom_bb, BB_GOC(sdata));
            BBA_AND(prom_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&prom_bb);
                if(dest<0) break;
                if(GKA_PROMOTE(src, dest))
                    MVLIST_SET_PROM(mvlist, mlist, src, dest);
                BBA_XOR(prom_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GHI
        ou_bb  = EFFECT_TBL(ou, SHI, sdata);
        pou_bb = EFFECT_TBL(ou, SRY, sdata);
        src_bb = BB_GHI(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, GHI, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_GOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            //成り
            BB_AND(prom_bb, dest_bb, pou_bb);
            BBA_OR(prom_bb, disc_bb);
            BBA_ANDNOT(prom_bb, BB_GOC(sdata));
            BBA_AND(prom_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&prom_bb);
                if(dest<0) break;
                if(GHI_PROMOTE(src, dest))
                    MVLIST_SET_PROM(mvlist, mlist, src, dest);
                BBA_XOR(prom_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GUM
        ou_bb  = EFFECT_TBL(ou, SUM, sdata);
        src_bb = BB_GUM(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, GUM, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_GOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GRY
        ou_bb  = EFFECT_TBL(ou, SRY, sdata);
        src_bb = BB_GRY(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, GRY, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_GOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
    }
    //先手番
    else             {
        //無仕掛けチェック
        
        if(!BB_TEST(BB_SOC(sdata))){
            if(!S_SMKEY(sdata).hi && !S_SMKEY(sdata).ka && !S_SMKEY(sdata).ke && !S_SMKEY(sdata).ky)
                return NULL;
        }
         
        ou = S_GOU(sdata);
        //持ち駒による王手
        if(SMKEY_FU(sdata) && ou<72) {
            dest_bb = EFFECT_TBL(ou, GFU, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                if(FU_CHECK(sdata, dest)){
                    //打歩詰めチェック
                    move_t mv = {HAND+FU, dest};
                    if(fu_tsume_check(mv, sdata))
                        MVLIST_SET_NORM(mvlist, mlist, HAND+FU, dest);
                }
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        if(SMKEY_KY(sdata) && ou<72) {
            dest_bb = EFFECT_TBL(ou, GKY, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, HAND+KY, dest);
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        if(SMKEY_KE(sdata) && ou<63) {
            dest_bb = EFFECT_TBL(ou, GKE, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, HAND+KE, dest);
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        if(SMKEY_GI(sdata))          {
            dest_bb = EFFECT_TBL(ou, GGI, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, HAND+GI, dest);
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        if(SMKEY_KI(sdata))          {
            dest_bb = EFFECT_TBL(ou, GKI, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, HAND+KI, dest);
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        if(SMKEY_KA(sdata))          {
            dest_bb = EFFECT_TBL(ou, GKA, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, HAND+KA, dest);
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        if(SMKEY_HI(sdata))          {
            dest_bb = EFFECT_TBL(ou, GHI, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            while(1){
                dest = min_pos(&dest_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, HAND+HI, dest);
                BBA_XOR(dest_bb, g_bpos[dest]);
            }
        }
        //盤上駒による直接王手,空き王手
        //SFU
        ou_bb  = EFFECT_TBL(ou, GFU, sdata);
        pou_bb = EFFECT_TBL(ou, GTO, sdata);
        src_bb = BB_SFU(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, SFU, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_SOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            dest = min_pos(&norm_bb);
            if(dest>=0 && SFU_NORMAL(dest))
                MVLIST_SET_NORM(mvlist, mlist, src, dest);;
            //成り
            BB_AND(prom_bb, dest_bb, pou_bb);
            BBA_OR(prom_bb, disc_bb);
            BBA_ANDNOT(prom_bb, BB_SOC(sdata));
            BBA_AND(prom_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            dest = min_pos(&prom_bb);
            if(dest>=0 && SFU_PROMOTE(dest))
                MVLIST_SET_PROM(mvlist, mlist, src, dest);
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SKY
        ou_bb  = EFFECT_TBL(ou, GKY, sdata);
        pou_bb = EFFECT_TBL(ou, GNY, sdata);
        src_bb = BB_SKY(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, SKY, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_SOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                if(SKY_NORMAL(dest))
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            //成り
            BB_AND(prom_bb, dest_bb, pou_bb);
            BBA_OR(prom_bb, disc_bb);
            BBA_ANDNOT(prom_bb, BB_SOC(sdata));
            BBA_AND(prom_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&prom_bb);
                if(dest<0) break;
                if(SKY_PROMOTE(dest))
                    MVLIST_SET_PROM(mvlist, mlist, src, dest);
                BBA_XOR(prom_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SKE
        ou_bb  = EFFECT_TBL(ou, GKE, sdata);
        pou_bb = EFFECT_TBL(ou, GNK, sdata);
        src_bb = BB_SKE(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, SKE, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_SOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                if(SKE_NORMAL(dest))
                    MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            //成り
            BB_AND(prom_bb, dest_bb, pou_bb);
            BBA_OR(prom_bb, disc_bb);
            BBA_ANDNOT(prom_bb, BB_SOC(sdata));
            BBA_AND(prom_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&prom_bb);
                if(dest<0) break;
                if(SKE_PROMOTE(dest))
                    MVLIST_SET_PROM(mvlist, mlist, src, dest);
                BBA_XOR(prom_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SGI
        ou_bb  = EFFECT_TBL(ou, GGI, sdata);
        pou_bb = EFFECT_TBL(ou, GNG, sdata);
        src_bb = BB_SGI(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, SGI, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_SOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            //成り
            BB_AND(prom_bb, dest_bb, pou_bb);
            BBA_OR(prom_bb, disc_bb);
            BBA_ANDNOT(prom_bb, BB_SOC(sdata));
            BBA_AND(prom_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&prom_bb);
                if(dest<0) break;
                if(SGI_PROMOTE(src, dest))
                    MVLIST_SET_PROM(mvlist, mlist, src, dest);
                BBA_XOR(prom_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SKI,STO,SNY,SNK,SNG
        ou_bb  = EFFECT_TBL(ou, GKI, sdata);
        src_bb = BB_STK(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, SKI, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_SOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SOU
        src = S_SOU(sdata);
        if(src<N_SQUARE){
            norm_bb = EFFECT_TBL(src, SOU, sdata);
            BBA_ANDNOT(norm_bb, g_bb_pin[st_disc.pin[src]]);
            BBA_ANDNOT(norm_bb, BB_SOC(sdata));
            BBA_ANDNOT(norm_bb, GEFFECT(sdata));
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
        }
        //SKA
        ou_bb  = EFFECT_TBL(ou, GKA, sdata);
        pou_bb = EFFECT_TBL(ou, GUM, sdata);
        src_bb = BB_SKA(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, SKA, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_SOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            //成り
            BB_AND(prom_bb, dest_bb, pou_bb);
            BBA_OR(prom_bb, disc_bb);
            BBA_ANDNOT(prom_bb, BB_SOC(sdata));
            BBA_AND(prom_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&prom_bb);
                if(dest<0) break;
                if(SKA_PROMOTE(src, dest))
                    MVLIST_SET_PROM(mvlist, mlist, src, dest);
                BBA_XOR(prom_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SHI
        ou_bb  = EFFECT_TBL(ou, GHI, sdata);
        pou_bb = EFFECT_TBL(ou, GRY, sdata);
        src_bb = BB_SHI(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, SHI, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_SOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            //成り
            BB_AND(prom_bb, dest_bb, pou_bb);
            BBA_OR(prom_bb, disc_bb);
            BBA_ANDNOT(prom_bb, BB_SOC(sdata));
            BBA_AND(prom_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&prom_bb);
                if(dest<0) break;
                if(SHI_PROMOTE(src, dest))
                    MVLIST_SET_PROM(mvlist, mlist, src, dest);
                BBA_XOR(prom_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SUM
        ou_bb  = EFFECT_TBL(ou, GUM, sdata);
        src_bb = BB_SUM(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, GUM, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_SOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SRY
        ou_bb  = EFFECT_TBL(ou, GRY, sdata);
        src_bb = BB_SRY(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            dest_bb = EFFECT_TBL(src, SRY, sdata);
            BB_ANDNOT(disc_bb, dest_bb, g_bb_pin[st_disc.pin[src]]);
            //不成
            BB_AND(norm_bb, dest_bb, ou_bb);
            BBA_OR(norm_bb, disc_bb);
            BBA_ANDNOT(norm_bb, BB_SOC(sdata));
            BBA_AND(norm_bb, g_bb_pin[S_PINNED(sdata)[src]]);
            while(1){
                dest = min_pos(&norm_bb);
                if(dest<0) break;
                MVLIST_SET_NORM(mvlist, mlist, src, dest);
                BBA_XOR(norm_bb, g_bpos[dest]);
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
    }
    return mvlist;
}

/* ---------------------------------
 特定の持ち駒で王手ができるかチェックする。
 反証駒用　true 王手可能
 --------------------------------- */
bool drop_check                 (const sdata_t *sdata,
                                 komainf_t      drop)
{
    //自玉が王手の場合
    if(S_NOUTE(sdata)){
#if DEBUG
        //SDATA_PRINTF(sdata, PR_BOARD|PR_ZKEY);
#endif //DEBUG
        return true;     //大事をとってtrueとしておく
    }
    
    int dest, ou;
    bitboard_t  dest_bb;     //駒が来れる位置のbitboard
    //後手番
    if(S_TURN(sdata)){
        ou = S_SOU(sdata);
        if(drop == FU && ou>8){
            dest_bb = EFFECT_TBL(ou, SFU, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            if(FU_CHECK(sdata,dest)){
                //打歩詰めチェック
                move_t mv = {HAND+FU, dest};
                if(fu_tsume_check(mv, sdata)){
                    return true;
                }
                return false;
            }
        }
        if(drop == KY && ou>8){
            dest_bb = EFFECT_TBL(ou, SKY, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            return true;
        }
        if(drop == KE && ou>17){
            dest_bb = EFFECT_TBL(ou, SKE, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            return true;
        }
        if(drop == GI){
            dest_bb = EFFECT_TBL(ou, SGI, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            return true;
        }
        if(drop == KI){
            dest_bb = EFFECT_TBL(ou, SKI, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            return true;
        }
        if(drop == KA){
            dest_bb = EFFECT_TBL(ou, SKA, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            return true;
        }
        if(drop == HI){
            dest_bb = EFFECT_TBL(ou, SHI, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            return true;
        }
        else return false;
    }
    //先手番
    else             {
        ou = S_GOU(sdata);
        if(drop == FU && ou<72){
            dest_bb = EFFECT_TBL(ou, GFU, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            if(FU_CHECK(sdata,dest)){
                //打歩詰めチェック
                move_t mv = {HAND+FU, dest};
                if(fu_tsume_check(mv, sdata)){
                    return true;
                }
                return false;
            }
        }
        if(drop == KY && ou<72){
            dest_bb = EFFECT_TBL(ou, GKY, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            return true;
        }
        if(drop == KE && ou<63){
            dest_bb = EFFECT_TBL(ou, GKE, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            return true;
        }
        if(drop == GI){
            dest_bb = EFFECT_TBL(ou, GGI, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            return true;
        }
        if(drop == KI){
            dest_bb = EFFECT_TBL(ou, GKI, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            return true;
        }
        if(drop == KA){
            dest_bb = EFFECT_TBL(ou, GKA, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            return true;
        }
        if(drop == HI){
            dest_bb = EFFECT_TBL(ou, GHI, sdata);
            BBA_ANDNOT(dest_bb, BB_OCC(sdata));
            dest = min_pos(&dest_bb);
            if(dest<0) return false;
            return true;
        }
        else return false;
    }
}

/* ---------------------------------
 王手の局面で逆王手があればリストアップする
 --------------------------------- */
mvlist_t* evasion_check         (const sdata_t *sdata,
                                 tbase_t       *tbase      )
{
    mvlist_t *list = generate_evasion(sdata, tbase);
    mvlist_t *mvlist = NULL, *tmp;
    sdata_t sbuf;
    move_t move;
    while(list){
        memcpy(&sbuf, sdata, sizeof(sdata_t));
        move = list->mlist->move;
        sdata_move_forward(&sbuf, move);
        tmp = list;
        list = tmp->next;
        if(S_NOUTE(&sbuf)){
            tmp->next = mvlist;
            mvlist = tmp;
        }
        else{
            tmp->next = NULL;
            mvlist_free(tmp);
        }
    }
    return mvlist;
}
