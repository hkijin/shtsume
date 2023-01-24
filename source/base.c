//
//  base.c
//  shshogi
//
//  Created by Hkijin on 2021/09/21.
//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "shogi.h"

static void sdata_captured  (komainf_t captured, move_t move, sdata_t *sdata);
static void sdata_mkey_sub  (komainf_t koma, move_t move, sdata_t *sdata);
static void key_captured    (komainf_t captured, sdata_t *sdata);
static void key_mkey_sub    (komainf_t koma, sdata_t *sdata);
static bool is_move_to_dest (int dest, const sdata_t *sdata);
static bool is_evasion_drop (int dest, const sdata_t *sdata);
static inline void bb_to_eff(bitboard_t       *eff,
                             komainf_t         koma,
                             const bitboard_t *bb_koma,
                             const sdata_t    *sdata      );

/* ----------------
 実装部
 ---------------- */
unsigned int mkoma_num(mkey_t mkey,
                       komainf_t koma){
    switch (koma%8) {
        case OU: return mkey.ou;
        case FU: return mkey.fu;
        case KY: return mkey.ky;
        case KE: return mkey.ke;
        case GI: return mkey.gi;
        case KI: return mkey.ki;
        case KA: return mkey.ka;
        case HI: return mkey.hi;
        default: return 0;
    }
}
mkey_t max_mkey(mkey_t key1, mkey_t key2){
    mkey_t mkey;
    mkey.ou = MAX(key1.ou, key2.ou);
    mkey.hi = MAX(key1.hi, key2.hi);
    mkey.ka = MAX(key1.ka, key2.ka);
    mkey.ki = MAX(key1.ki, key2.ki);
    mkey.gi = MAX(key1.gi, key2.gi);
    mkey.ke = MAX(key1.ke, key2.ke);
    mkey.ky = MAX(key1.ky, key2.ky);
    mkey.fu = MAX(key1.fu, key2.fu);
    return mkey;
}

mkey_t min_mkey(mkey_t key1, mkey_t key2){
    mkey_t mkey;
    mkey.ou = MIN(key1.ou, key2.ou);
    mkey.hi = MIN(key1.hi, key2.hi);
    mkey.ka = MIN(key1.ka, key2.ka);
    mkey.ki = MIN(key1.ki, key2.ki);
    mkey.gi = MIN(key1.gi, key2.gi);
    mkey.ke = MIN(key1.ke, key2.ke);
    mkey.ky = MIN(key1.ky, key2.ky);
    mkey.fu = MIN(key1.fu, key2.fu);
    return mkey;
}

void create_seed        (void){
    for(int i=N_SQUARE; i<N_ZOBRIST_SEED; i++)     {
        g_zkey_seed[i] = (((zkey_t)rand())<<49 |
                          ((zkey_t)rand())<<34 |
                          ((zkey_t)rand())<<19 |
                          ((zkey_t)rand())<<4  |(((zkey_t)rand())&0xf));
    }
    return;
}    //zobrist key

void init_distance (void){
    for(int src=0; src<N_SQUARE; src++){
        for(int dest=0; dest<N_SQUARE; dest++){
            if(  DIR_N_(src,dest)||DIR_E_(src,dest)
               ||DIR_W_(src,dest)||DIR_S_(src,dest)
               ||DIR_NE(src,dest)||DIR_NW(src,dest)
               ||DIR_SE(src,dest)||DIR_SW(src,dest))
                g_distance[src][dest] =
                MAX(abs(src%9-dest%9), abs(src/9-dest/9));
            else
                g_distance[src][dest] = 1;
        }
    }
}


void initialize_sdata(sdata_t *sdata,
                      const ssdata_t *ssdata)      {
    memset(sdata, 0, sizeof(sdata_t));
    memcpy(&(sdata->core), ssdata, sizeof(ssdata_t));
    S_SOU(sdata) = S_GOU(sdata) = HAND;
    int pos;
    komainf_t koma;
    for(pos=0; pos<N_SQUARE; pos++){
        koma = S_BOARD(sdata, pos);
        if(koma){
            //zkey
            S_ZKEY(sdata) ^= g_zkey_seed[koma*N_SQUARE+pos];
            //fflag
            if(koma == SFU)
                S_FFLAG(sdata) = FLAG_SET(S_FFLAG(sdata), g_file[pos]);
            if(koma == GFU)
                S_FFLAG(sdata) = FLAG_SET(S_FFLAG(sdata), g_file[pos]+9);
            //occupied bitboard
            SDATA_OCC_XOR(sdata, pos);
            SENTE_KOMA(koma)?
            (BBA_XOR(BB_SOC(sdata), g_bpos[pos])):
            (BBA_XOR(BB_GOC(sdata), g_bpos[pos]));
            //ou koma_bb eff_bb
            sdata_bb_xor(sdata, koma, pos);
#ifdef SDATA_EXTENTION
            //kscore
            S_KSCORE(sdata) += g_koma_val[koma];
            //np
            if(SENTE_KOMA(koma))
                S_SNP(sdata) += g_nkoma_val[koma];
            else
                S_GNP(sdata) += g_nkoma_val[koma];
#endif //SDATA_EXTENTION
        }
    }
#ifdef SDATA_EXTENTION
    //score, NPに持ち駒をを加える
    S_KSCORE(sdata) += SMKEY_FU(sdata)*g_koma_val[SFU];
    S_KSCORE(sdata) += SMKEY_KY(sdata)*g_koma_val[SKY];
    S_KSCORE(sdata) += SMKEY_KE(sdata)*g_koma_val[SKE];
    S_KSCORE(sdata) += SMKEY_GI(sdata)*g_koma_val[SGI];
    S_KSCORE(sdata) += SMKEY_KI(sdata)*g_koma_val[SKI];
    S_KSCORE(sdata) += SMKEY_KA(sdata)*g_koma_val[SKA];
    S_KSCORE(sdata) += SMKEY_HI(sdata)*g_koma_val[SHI];
    S_KSCORE(sdata) += GMKEY_FU(sdata)*g_koma_val[GFU];
    S_KSCORE(sdata) += GMKEY_KY(sdata)*g_koma_val[GKY];
    S_KSCORE(sdata) += GMKEY_KE(sdata)*g_koma_val[GKE];
    S_KSCORE(sdata) += GMKEY_GI(sdata)*g_koma_val[GGI];
    S_KSCORE(sdata) += GMKEY_KI(sdata)*g_koma_val[GKI];
    S_KSCORE(sdata) += GMKEY_KA(sdata)*g_koma_val[GKA];
    S_KSCORE(sdata) += GMKEY_HI(sdata)*g_koma_val[GHI];
    S_SNP(sdata) += SMKEY_FU(sdata)*g_nkoma_val[SFU];
    S_SNP(sdata) += SMKEY_KY(sdata)*g_nkoma_val[SKY];
    S_SNP(sdata) += SMKEY_KE(sdata)*g_nkoma_val[SKE];
    S_SNP(sdata) += SMKEY_GI(sdata)*g_nkoma_val[SGI];
    S_SNP(sdata) += SMKEY_KI(sdata)*g_nkoma_val[SKI];
    S_SNP(sdata) += SMKEY_KA(sdata)*g_nkoma_val[SKA];
    S_SNP(sdata) += SMKEY_HI(sdata)*g_nkoma_val[SHI];
    S_GNP(sdata) += GMKEY_FU(sdata)*g_nkoma_val[GFU];
    S_GNP(sdata) += GMKEY_KY(sdata)*g_nkoma_val[GKY];
    S_GNP(sdata) += GMKEY_KE(sdata)*g_nkoma_val[GKE];
    S_GNP(sdata) += GMKEY_GI(sdata)*g_nkoma_val[GGI];
    S_GNP(sdata) += GMKEY_KI(sdata)*g_nkoma_val[GKI];
    S_GNP(sdata) += GMKEY_KA(sdata)*g_nkoma_val[GKA];
    S_GNP(sdata) += GMKEY_HI(sdata)*g_nkoma_val[GHI];
#endif //SDATA_EXTENTION
    
    S_NOUTE(sdata) = oute_check(sdata);
    if(S_TURN(sdata))S_ZKEY(sdata) ^= g_zkey_seed[TURN_ADDRESS];
    create_effect(sdata);
    create_pin(sdata);
    return;
}

//tsumi_check  詰み:　true 詰みではない: false
bool tsumi_check       (const sdata_t *sdata)      {
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
    if(min_pos(&move_bb)>=0) return false;
    //両王手の場合、ここで終了
    if(S_NOUTE(sdata)>1) return true;
    //王手している駒をとる手
    int dest = S_ATTACK(sdata)[0];
    bool res = is_move_to_dest(dest, sdata);
    if(!res) return false;
    
    //王手している駒が飛び駒で無ければここで終了
    koma = S_BOARD(sdata, dest);
    if(g_is_skoma[koma]) return true;
    
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
    if(dir){
        dest = src;
        while(true){
            dest += dir;
            if(dest == S_ATTACK(sdata)[0]) break;
            if(!is_move_to_dest(dest, sdata)) return false;
            if(!is_evasion_drop(dest, sdata)) return false;
        }
    }
    return true;
}
//歩詰チェック 打ち歩詰め　false  打ち歩詰めでは無い　true
bool fu_tsume_check    (move_t move,
                        const sdata_t *sdata)      {
    sdata_t sbuf;
    memcpy(&sbuf, sdata, sizeof(sdata_t));
    sdata_move_forward(&sbuf, move);
    return !tsumi_check(&sbuf);
}

// 空王手生成のためのpin情報セット　*discにpin情報をセット
void set_discpin       (const sdata_t *sdata,
                        discpin_t *disc)           {
    memset(&(disc->pin), 0, sizeof(int)*N_SQUARE);
    int ou, src, pin;
    bitboard_t src_bb, k_eff, o_eff;
    if(S_TURN(sdata)){
        ou = S_SOU(sdata);
        //GKY
        src_bb = BB_GKY(sdata);
        while(true){
            src = max_pos(&src_bb);
            if(src<0) break;
            if(g_file[ou]==g_file[src]){
                o_eff = EFFECT_TBL(ou, SKY, sdata);
                k_eff = EFFECT_TBL(src, GKY, sdata);
                BBA_AND(k_eff, o_eff);
                if(BB_TEST(k_eff)){
                    pin = min_pos(&k_eff);
                    if(GOTE_KOMA(S_BOARD(sdata, pin))){
                        disc->pin[pin] = g_file[pin] +10;
                        break;
                    }
                }
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GKA, GUM
        src_bb = BB_GUK(sdata);
        while(true){
            src = max_pos(&src_bb);
            if(src<0) break;
            if(g_rslp[ou]==g_rslp[src]||g_lslp[ou]==g_lslp[src]){
                o_eff = EFFECT_TBL(ou, SKA, sdata);
                k_eff = EFFECT_TBL(src, GKA, sdata);
                BBA_AND(k_eff, o_eff);
                if(BB_TEST(k_eff)){
                    pin = min_pos(&k_eff);
                    if(GOTE_KOMA(S_BOARD(sdata, pin))){
                        if(g_rslp[ou] == g_rslp[src])
                            disc->pin[pin] = g_rslp[ou]+30;
                        else if(g_lslp[ou] == g_lslp[src])
                            disc->pin[pin] = g_lslp[ou]+17;
                    }
                }
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GHI, GRY
        src_bb = BB_GRH(sdata);
        while(true){
            src = max_pos(&src_bb);
            if(src<0) break;
            if(g_rank[ou]==g_rank[src]||g_file[ou]==g_file[src]){
                o_eff = EFFECT_TBL(ou, SHI, sdata);
                k_eff = EFFECT_TBL(src, GHI, sdata);
                BBA_AND(k_eff, o_eff);
                if(BB_TEST(k_eff)){
                    pin = min_pos(&k_eff);
                    if(GOTE_KOMA(S_BOARD(sdata, pin))){
                        if(g_rank[ou] == g_rank[src])
                            disc->pin[pin] = g_rank[ou]+1;
                        else if(g_file[ou] == g_file[src])
                            disc->pin[pin] = g_file[ou]+10;
                    }
                }
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
    }
    else             {
        ou = S_GOU(sdata);
        //SKY
        src_bb = BB_SKY(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            if(g_file[ou]==g_file[src]){
                o_eff = EFFECT_TBL(ou, GKY, sdata);
                k_eff = EFFECT_TBL(src, SKY, sdata);
                BBA_AND(k_eff, o_eff);
                if(BB_TEST(k_eff)){
                    pin = min_pos(&k_eff);
                    if(SENTE_KOMA(S_BOARD(sdata, pin))){
                        disc->pin[pin] = g_file[pin] +10;
                        break;
                    }
                }
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SKA, SUM
        src_bb = BB_SUK(sdata);
        while(1){
            src = max_pos(&src_bb);
            if(src<0) break;
            if(g_rslp[ou]==g_rslp[src]||g_lslp[ou]==g_lslp[src]){
                o_eff = EFFECT_TBL(ou, GKA, sdata);
                k_eff = EFFECT_TBL(src, SKA, sdata);
                BBA_AND(k_eff, o_eff);
                if(BB_TEST(k_eff)){
                    pin = min_pos(&k_eff);
                    if(SENTE_KOMA(S_BOARD(sdata, pin))){
                        if(g_rslp[ou] == g_rslp[src])
                            disc->pin[pin] = g_rslp[ou]+30;
                        else if(g_lslp[ou] == g_lslp[src])
                            disc->pin[pin] = g_lslp[ou]+17;
                    }
                }
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SHI, SRY
        src_bb = BB_SRH(sdata);
        while(1){
            src = max_pos(&src_bb);
            if(src<0) break;
            if(g_rank[ou]==g_rank[src]||g_file[ou]==g_file[src]){
                o_eff = EFFECT_TBL(ou, GHI, sdata);
                k_eff = EFFECT_TBL(src, SHI, sdata);
                BBA_AND(k_eff, o_eff);
                if(BB_TEST(k_eff)){
                    pin = min_pos(&k_eff);
                    if(SENTE_KOMA(S_BOARD(sdata, pin))){
                        if(g_rank[ou] == g_rank[src])
                            disc->pin[pin] = g_rank[ou]+1;
                        else if(g_file[ou] == g_file[src])
                            disc->pin[pin] = g_file[ou]+10;
                    }
                }
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
    }
    return;
}

int sdata_move_forward(sdata_t *sdata, move_t move){
    //投了他
    if(MV_ACTION(move)) return 0;
    komainf_t koma, captured;

    /* board,zkey,mkey,kscore,occupied, bb_koma, fflag, ou */
    //盤上移動
    if(MV_MOVE(move)){
    //移動元の駒を削除
    koma = S_BOARD(sdata,PREV_POS(move));
    S_BOARD(sdata,PREV_POS(move)) = SPC;
    S_ZKEY(sdata) ^= g_zkey_seed[koma*N_SQUARE+PREV_POS(move)];
    if(koma == SFU)
        S_FFLAG(sdata) =
        FLAG_UNSET(S_FFLAG(sdata), g_file[PREV_POS(move)]);
    else if(koma == GFU)
        S_FFLAG(sdata) =
        FLAG_UNSET(S_FFLAG(sdata), g_file[PREV_POS(move)]+9);
#ifdef SDATA_EXTENTION
    S_KSCORE(sdata) -= g_koma_val[koma];
#endif //SDATA_EXTENTION
    
    /* ビットボード処理 */
    SDATA_OCC_XOR(sdata, PREV_POS(move));
    if(koma==SOU||koma==GOU) ;
    else sdata_bb_xor(sdata, koma, PREV_POS(move));
    S_TURN(sdata)?
    (BBA_XOR(BB_GOC(sdata), g_bpos[PREV_POS(move)])):
    (BBA_XOR(BB_SOC(sdata), g_bpos[PREV_POS(move)]));

    //移動先に駒があれば駒台に移動
    captured = S_BOARD(sdata,NEW_POS(move));
    if(captured){
        S_ZKEY(sdata) ^= g_zkey_seed[captured*N_SQUARE+NEW_POS(move)];
#ifdef SDATA_EXTENTION
        S_KSCORE(sdata) -= g_koma_val[captured];
#endif //SDATA_EXTENTION
        
        SDATA_OCC_XOR(sdata, NEW_POS(move));
        sdata_bb_xor(sdata, captured, NEW_POS(move));
        S_TURN(sdata)?
        (BBA_XOR(BB_SOC(sdata), g_bpos[NEW_POS(move)])):
        (BBA_XOR(BB_GOC(sdata), g_bpos[NEW_POS(move)]));
        sdata_captured(captured, move, sdata);
    }
    //移動先に駒を置く
    if(PROMOTE(move)) koma += PROMOTED;
    S_BOARD(sdata, NEW_POS(move)) = koma;
    S_ZKEY(sdata) ^= g_zkey_seed[koma*N_SQUARE+NEW_POS(move)];
    if(koma == SFU)
        S_FFLAG(sdata) = FLAG_SET(S_FFLAG(sdata), g_file[NEW_POS(move)]);
    else if(koma == GFU)
        S_FFLAG(sdata) = FLAG_SET(S_FFLAG(sdata), g_file[NEW_POS(move)]+9);
#ifdef SDATA_EXTENTION
    S_KSCORE(sdata) += g_koma_val[koma];
#endif //SDATA_EXTENTION
    /* ビットボード処理 */
    SDATA_OCC_XOR(sdata, NEW_POS(move));
    sdata_bb_xor(sdata, koma, NEW_POS(move));
    S_TURN(sdata)?
    (BBA_XOR(BB_GOC(sdata), g_bpos[NEW_POS(move)])):
    (BBA_XOR(BB_SOC(sdata), g_bpos[NEW_POS(move)]));
}
    //持ち駒使用
    else if(MV_DROP(move)){
        koma = MV_HAND(move)+S_TURN(sdata)*16;
        //駒を置く
        S_BOARD(sdata, NEW_POS(move)) = koma;
        S_ZKEY(sdata) ^= g_zkey_seed[koma*N_SQUARE+NEW_POS(move)];
        /* ビットボード処理 */
        SDATA_OCC_XOR(sdata, NEW_POS(move));
        sdata_bb_xor(sdata, koma, NEW_POS(move));
        S_TURN(sdata)?
        (BBA_XOR(BB_GOC(sdata), g_bpos[NEW_POS(move)])):
        (BBA_XOR(BB_SOC(sdata), g_bpos[NEW_POS(move)]));
        //駒台の駒数変更,fuflagの更新
        sdata_mkey_sub(koma, move, sdata);
    }

    /* count, turn */
    S_COUNT(sdata)++;
    S_TURN(sdata) = TURN_FLIP(S_TURN(sdata));
    S_ZKEY(sdata) ^= g_zkey_seed[TURN_ADDRESS];

    /* n_oute, attack */
    //S_NOUTE(sdata) = oute_check(sdata);

    /* pinned  effect */
    create_effect(sdata);
    create_pin(sdata);
    if(S_TURN(sdata)){
        if(BPOS_TEST(SEFFECT(sdata), S_GOU(sdata)))
            S_NOUTE(sdata) = oute_check(sdata);
        else S_NOUTE(sdata) = 0;
    }
    else{
        if(BPOS_TEST(GEFFECT(sdata), S_SOU(sdata)))
            S_NOUTE(sdata) = oute_check(sdata);
        else S_NOUTE(sdata) = 0;
    }
    
    return 1;
}

int sdata_key_forward(sdata_t *sdata, move_t move){
    //局面探索専用として以下の変数のみ更新
    //zkey, mkey, turn, count
    
    //投了他
    if(MV_ACTION(move)) return 0;
    komainf_t koma, captured;
    //盤上移動
    if(MV_MOVE(move)){
        //移動元の駒を削除
        koma = S_BOARD(sdata,PREV_POS(move));
        S_ZKEY(sdata) ^= g_zkey_seed[koma*N_SQUARE+PREV_POS(move)];
        //移動先に駒があれば駒台に移動
        captured = S_BOARD(sdata,NEW_POS(move));
        if(captured){
            S_ZKEY(sdata) ^= g_zkey_seed[captured*N_SQUARE+NEW_POS(move)];
            key_captured(captured, sdata);
        }
        //移動先に駒を置く
        if(PROMOTE(move)) koma += PROMOTED;
        S_ZKEY(sdata) ^= g_zkey_seed[koma*N_SQUARE+NEW_POS(move)];
    }
    //持駒使用
    else{
        koma = MV_HAND(move)+S_TURN(sdata)*16;
        //駒を置く
        S_ZKEY(sdata) ^= g_zkey_seed[koma*N_SQUARE+NEW_POS(move)];
        //駒台の駒数変更
        key_mkey_sub(koma, sdata);
    }
    
    //手数、手番
    S_COUNT(sdata)++;
    S_TURN(sdata) = TURN_FLIP(S_TURN(sdata));
    S_ZKEY(sdata) ^= g_zkey_seed[TURN_ADDRESS];
    
    return 1;
}

void sdata_bb_xor(sdata_t *sdata, komainf_t koma, char pos)         {
    switch(koma){
        case SFU: BBA_XOR(BB_SFU(sdata),g_bpos[pos]);
                  BBA_XOR(EF_SFU(sdata),g_bpos[pos+DR_N]); break;
        case SKY: BBA_XOR(BB_SKY(sdata),g_bpos[pos]); break;
        case SKE: BBA_XOR(BB_SKE(sdata),g_bpos[pos]); break;
        case SGI: BBA_XOR(BB_SGI(sdata),g_bpos[pos]); break;
        case SKI: BBA_XOR(BB_SKI(sdata),g_bpos[pos]);
                  BBA_XOR(BB_STK(sdata),g_bpos[pos]); break;
        case SKA: BBA_XOR(BB_SKA(sdata),g_bpos[pos]);
                  BBA_XOR(BB_SUK(sdata),g_bpos[pos]); break;
        case SHI: BBA_XOR(BB_SHI(sdata),g_bpos[pos]);
                  BBA_XOR(BB_SRH(sdata),g_bpos[pos]); break;
        case SOU: S_SOU(sdata) = pos; break;
        case STO: BBA_XOR(BB_STO(sdata),g_bpos[pos]);
                  BBA_XOR(BB_STK(sdata),g_bpos[pos]); break;
        case SNY: BBA_XOR(BB_SNY(sdata),g_bpos[pos]);
                  BBA_XOR(BB_STK(sdata),g_bpos[pos]); break;
        case SNK: BBA_XOR(BB_SNK(sdata),g_bpos[pos]);
                  BBA_XOR(BB_STK(sdata),g_bpos[pos]); break;
        case SNG: BBA_XOR(BB_SNG(sdata),g_bpos[pos]);
                  BBA_XOR(BB_STK(sdata),g_bpos[pos]); break;
        case SUM: BBA_XOR(BB_SUM(sdata),g_bpos[pos]);
                  BBA_XOR(BB_SUK(sdata),g_bpos[pos]); break;
        case SRY: BBA_XOR(BB_SRY(sdata),g_bpos[pos]);
                  BBA_XOR(BB_SRH(sdata),g_bpos[pos]); break;
            
        case GFU: BBA_XOR(BB_GFU(sdata),g_bpos[pos]);
                  BBA_XOR(EF_GFU(sdata),g_bpos[pos+DR_S]); break;
        case GKY: BBA_XOR(BB_GKY(sdata),g_bpos[pos]); break;
        case GKE: BBA_XOR(BB_GKE(sdata),g_bpos[pos]); break;
        case GGI: BBA_XOR(BB_GGI(sdata),g_bpos[pos]); break;
        case GKI: BBA_XOR(BB_GKI(sdata),g_bpos[pos]);
                  BBA_XOR(BB_GTK(sdata),g_bpos[pos]); break;
        case GKA: BBA_XOR(BB_GKA(sdata),g_bpos[pos]);
                  BBA_XOR(BB_GUK(sdata),g_bpos[pos]); break;
        case GHI: BBA_XOR(BB_GHI(sdata),g_bpos[pos]);
                  BBA_XOR(BB_GRH(sdata),g_bpos[pos]); break;
        case GOU: S_GOU(sdata) = pos; break;
        case GTO: BBA_XOR(BB_GTO(sdata),g_bpos[pos]);
                  BBA_XOR(BB_GTK(sdata),g_bpos[pos]); break;
        case GNY: BBA_XOR(BB_GNY(sdata),g_bpos[pos]);
                  BBA_XOR(BB_GTK(sdata),g_bpos[pos]); break;
        case GNK: BBA_XOR(BB_GNK(sdata),g_bpos[pos]);
                  BBA_XOR(BB_GTK(sdata),g_bpos[pos]); break;
        case GNG: BBA_XOR(BB_GNG(sdata),g_bpos[pos]);
                  BBA_XOR(BB_GTK(sdata),g_bpos[pos]); break;
        case GUM: BBA_XOR(BB_GUM(sdata),g_bpos[pos]);
                  BBA_XOR(BB_GUK(sdata),g_bpos[pos]); break;
        case GRY: BBA_XOR(BB_GRY(sdata),g_bpos[pos]);
                  BBA_XOR(BB_GRH(sdata),g_bpos[pos]); break;
        default: break;
    }
    return;
}

/* -----------------
 スタティック関数実装部
------------------ */
void sdata_captured(komainf_t captured, move_t move, sdata_t *sdata){
switch(captured){
    case SFU:
        GMKEY_FU(sdata)++;
        S_FFLAG(sdata) =
        FLAG_UNSET(S_FFLAG(sdata), g_file[NEW_POS(move)]);
#ifdef SDATA_EXTENTION
        S_GNP(sdata)+=g_nkoma_val[GFU];
        S_SNP(sdata)-=g_nkoma_val[SFU];
        S_KSCORE(sdata) += g_koma_val[GFU];
#endif //SDATA_EXTENTION
        break;
    case STO:
        GMKEY_FU(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)+=g_nkoma_val[GFU];
        S_SNP(sdata)-=g_nkoma_val[STO];
        S_KSCORE(sdata) += g_koma_val[GFU];
#endif //SDATA_EXTENTION
        break;
    case SKY:
        GMKEY_KY(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)+=g_nkoma_val[GKY];
        S_SNP(sdata)-=g_nkoma_val[SKY];
        S_KSCORE(sdata) += g_koma_val[GKY];
#endif //SDATA_EXTENTION
        break;
    case SNY:
        GMKEY_KY(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)+=g_nkoma_val[GKY];
        S_SNP(sdata)-=g_nkoma_val[SNY];
        S_KSCORE(sdata) += g_koma_val[GKY];
#endif //SDATA_EXTENTION
        break;
    case SKE:
        GMKEY_KE(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)+=g_nkoma_val[GKE];
        S_SNP(sdata)-=g_nkoma_val[SKE];
        S_KSCORE(sdata) += g_koma_val[GKE];
#endif //SDATA_EXTENTION
        break;
    case SNK:
        GMKEY_KE(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)+=g_nkoma_val[GKE];
        S_SNP(sdata)-=g_nkoma_val[SNK];
        S_KSCORE(sdata) += g_koma_val[GKE];
#endif //SDATA_EXTENTION
        break;
    case SGI:
        GMKEY_GI(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)+=g_nkoma_val[GGI];
        S_SNP(sdata)-=g_nkoma_val[SGI];
        S_KSCORE(sdata) += g_koma_val[GGI];
#endif //SDATA_EXTENTION
        break;
    case SNG:
        GMKEY_GI(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)+=g_nkoma_val[GGI];
        S_SNP(sdata)-=g_nkoma_val[SNG];
        S_KSCORE(sdata) += g_koma_val[GGI];
#endif //SDATA_EXTENTION
        break;
    case SKI:
        GMKEY_KI(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)+=g_nkoma_val[GKI];
        S_SNP(sdata)-=g_nkoma_val[SKI];
        S_KSCORE(sdata) += g_koma_val[GKI];
#endif //SDATA_EXTENTION
        break;
    case SKA:
        GMKEY_KA(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)+=g_nkoma_val[GKA];
        S_SNP(sdata)-=g_nkoma_val[SKA];
        S_KSCORE(sdata) += g_koma_val[GKA];
#endif //SDATA_EXTENTION
        break;
    case SUM:
        GMKEY_KA(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)+=g_nkoma_val[GKA];
        S_SNP(sdata)-=g_nkoma_val[SUM];
        S_KSCORE(sdata) += g_koma_val[GKA];
#endif //SDATA_EXTENTION
        break;
    case SHI:
        GMKEY_HI(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)+=g_nkoma_val[GHI];
        S_SNP(sdata)-=g_nkoma_val[SHI];
        S_KSCORE(sdata) += g_koma_val[GHI];
#endif //SDATA_EXTENTION
        break;
    case SRY:
        GMKEY_HI(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)+=g_nkoma_val[GHI];
        S_SNP(sdata)-=g_nkoma_val[SRY];
        S_KSCORE(sdata) += g_koma_val[GHI];
#endif //SDATA_EXTENTION
        break;
    case GFU:
        SMKEY_FU(sdata)++;
        S_FFLAG(sdata) =
        FLAG_UNSET(S_FFLAG(sdata), g_file[NEW_POS(move)]+9);
#ifdef SDATA_EXTENTION
        S_GNP(sdata)-=g_nkoma_val[GFU];
        S_SNP(sdata)+=g_nkoma_val[SFU];
        S_KSCORE(sdata) += g_koma_val[SFU];
#endif //SDATA_EXTENTION
        break;
    case GTO:
        SMKEY_FU(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)-=g_nkoma_val[GTO];
        S_SNP(sdata)+=g_nkoma_val[SFU];
        S_KSCORE(sdata) += g_koma_val[SFU];
#endif //SDATA_EXTENTION
        break;
    case GKY:
        SMKEY_KY(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)-=g_nkoma_val[GKY];
        S_SNP(sdata)+=g_nkoma_val[SKY];
        S_KSCORE(sdata) += g_koma_val[SKY];
#endif //SDATA_EXTENTION
        break;
    case GNY:
        SMKEY_KY(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)-=g_nkoma_val[GNY];
        S_SNP(sdata)+=g_nkoma_val[SKY];
        S_KSCORE(sdata) += g_koma_val[SKY];
#endif //SDATA_EXTENTION
        break;
    case GKE:
        SMKEY_KE(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)-=g_nkoma_val[GKE];
        S_SNP(sdata)+=g_nkoma_val[SKE];
        S_KSCORE(sdata) += g_koma_val[SKE];
#endif //SDATA_EXTENTION
        break;
    case GNK:
        SMKEY_KE(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)-=g_nkoma_val[GNK];
        S_SNP(sdata)+=g_nkoma_val[SKE];
        S_KSCORE(sdata) += g_koma_val[SKE];
#endif //SDATA_EXTENTION
        break;
    case GGI:
        SMKEY_GI(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)-=g_nkoma_val[GGI];
        S_SNP(sdata)+=g_nkoma_val[SGI];
        S_KSCORE(sdata) += g_koma_val[SGI];
#endif //SDATA_EXTENTION
        break;
    case GNG:
        SMKEY_GI(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)-=g_nkoma_val[GNG];
        S_SNP(sdata)+=g_nkoma_val[SGI];
        S_KSCORE(sdata) += g_koma_val[SGI];
#endif //SDATA_EXTENTION
        break;
    case GKI:
        SMKEY_KI(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)-=g_nkoma_val[GKI];
        S_SNP(sdata)+=g_nkoma_val[SKI];
        S_KSCORE(sdata) += g_koma_val[SKI];
#endif //SDATA_EXTENTION
        break;
    case GKA:
        SMKEY_KA(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)-=g_nkoma_val[GKA];
        S_SNP(sdata)+=g_nkoma_val[SKA];
        S_KSCORE(sdata) += g_koma_val[SKA];
#endif //SDATA_EXTENTION
        break;
    case GUM:
        SMKEY_KA(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)-=g_nkoma_val[GUM];
        S_SNP(sdata)+=g_nkoma_val[SKA];
        S_KSCORE(sdata) += g_koma_val[SKA];
#endif //SDATA_EXTENTION
        break;
    case GHI:
        SMKEY_HI(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)-=g_nkoma_val[GHI];
        S_SNP(sdata)+=g_nkoma_val[SHI];
        S_KSCORE(sdata) += g_koma_val[SHI];
#endif //SDATA_EXTENTION
        break;
    case GRY:
        SMKEY_HI(sdata)++;
#ifdef SDATA_EXTENTION
        S_GNP(sdata)-=g_nkoma_val[GHI];
        S_SNP(sdata)+=g_nkoma_val[SHI];
        S_KSCORE(sdata) += g_koma_val[SHI];
#endif //SDATA_EXTENTION
        break;
    default:
        assert(false);
        break;
}
return;
}
void sdata_mkey_sub(komainf_t koma, move_t move, sdata_t *sdata)    {
    switch(koma){
        case SFU:
            SMKEY_FU(sdata)--;
            S_FFLAG(sdata) =
            FLAG_SET(S_FFLAG(sdata),g_file[NEW_POS(move)]);
            break;
        case SKY: SMKEY_KY(sdata)--; break;
        case SKE: SMKEY_KE(sdata)--; break;
        case SGI: SMKEY_GI(sdata)--; break;
        case SKI: SMKEY_KI(sdata)--; break;
        case SKA: SMKEY_KA(sdata)--; break;
        case SHI: SMKEY_HI(sdata)--; break;
        case GFU:
            GMKEY_FU(sdata)--;
            S_FFLAG(sdata) =
            FLAG_SET(S_FFLAG(sdata),g_file[NEW_POS(move)]+9);
            break;
        case GKY: GMKEY_KY(sdata)--; break;
        case GKE: GMKEY_KE(sdata)--; break;
        case GGI: GMKEY_GI(sdata)--; break;
        case GKI: GMKEY_KI(sdata)--; break;
        case GKA: GMKEY_KA(sdata)--; break;
        case GHI: GMKEY_HI(sdata)--; break;
        default:  break;
    }
}
void key_captured  (komainf_t captured, sdata_t *sdata){
    switch(captured){
        case SFU: GMKEY_FU(sdata)++; break;
        case STO: GMKEY_FU(sdata)++; break;
        case SKY: GMKEY_KY(sdata)++; break;
        case SNY: GMKEY_KY(sdata)++; break;
        case SKE: GMKEY_KE(sdata)++; break;
        case SNK: GMKEY_KE(sdata)++; break;
        case SGI: GMKEY_GI(sdata)++; break;
        case SNG: GMKEY_GI(sdata)++; break;
        case SKI: GMKEY_KI(sdata)++; break;
        case SKA: GMKEY_KA(sdata)++; break;
        case SUM: GMKEY_KA(sdata)++; break;
        case SHI: GMKEY_HI(sdata)++; break;
        case SRY: GMKEY_HI(sdata)++; break;
        case GFU: SMKEY_FU(sdata)++; break;
        case GTO: SMKEY_FU(sdata)++; break;
        case GKY: SMKEY_KY(sdata)++; break;
        case GNY: SMKEY_KY(sdata)++; break;
        case GKE: SMKEY_KE(sdata)++; break;
        case GNK: SMKEY_KE(sdata)++; break;
        case GGI: SMKEY_GI(sdata)++; break;
        case GNG: SMKEY_GI(sdata)++; break;
        case GKI: SMKEY_KI(sdata)++; break;
        case GKA: SMKEY_KA(sdata)++; break;
        case GUM: SMKEY_KA(sdata)++; break;
        case GHI: SMKEY_HI(sdata)++; break;
        case GRY: SMKEY_HI(sdata)++; break;
        default:  assert(false);     break;
    }
    return;
}
void key_mkey_sub  (komainf_t koma, sdata_t *sdata){
    switch(koma){
        case SFU: SMKEY_FU(sdata)--; break;
        case SKY: SMKEY_KY(sdata)--; break;
        case SKE: SMKEY_KE(sdata)--; break;
        case SGI: SMKEY_GI(sdata)--; break;
        case SKI: SMKEY_KI(sdata)--; break;
        case SKA: SMKEY_KA(sdata)--; break;
        case SHI: SMKEY_HI(sdata)--; break;
        case GFU: GMKEY_FU(sdata)--; break;
        case GKY: GMKEY_KY(sdata)--; break;
        case GKE: GMKEY_KE(sdata)--; break;
        case GGI: GMKEY_GI(sdata)--; break;
        case GKI: GMKEY_KI(sdata)--; break;
        case GKA: GMKEY_KA(sdata)--; break;
        case GHI: GMKEY_HI(sdata)--; break;
        default:  break;
    }
}

#define OUTE_OUT(eff,res,sdata,num)             \
    if(BB_TEST(eff)){                           \
        (res)++;                                \
        (sdata)->attack[num] = min_pos(&(eff)); \
        if((res)>1) return (res);               \
        (num)++;                                \
    }

int oute_check (sdata_t *sdata){
    char pos = SELF_OU(sdata);
    if(pos == HAND) return 0;
    
    int num = 0, res = 0;
    bitboard_t eff;
    if(S_TURN(sdata)){
        eff = EFFECT_TBL(pos, GFU, sdata);
        BBA_AND(eff, BB_SFU(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, GKY, sdata);
        BBA_AND(eff, BB_SKY(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, GKE, sdata);
        BBA_AND(eff, BB_SKE(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, GGI, sdata);
        BBA_AND(eff, BB_SGI(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, GKI, sdata);
        BBA_AND(eff, BB_STK(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, GKA, sdata);
        BBA_AND(eff, BB_SKA(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, GHI, sdata);
        BBA_AND(eff, BB_SHI(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, GUM, sdata);
        BBA_AND(eff, BB_SUM(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, GRY, sdata);
        BBA_AND(eff, BB_SRY(sdata)); OUTE_OUT(eff,res,sdata,num);
    }
    else             {
        eff = EFFECT_TBL(pos, SFU, sdata);
        BBA_AND(eff, BB_GFU(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, SKY, sdata);
        BBA_AND(eff, BB_GKY(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, SKE, sdata);
        BBA_AND(eff, BB_GKE(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, SGI, sdata);
        BBA_AND(eff, BB_GGI(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, SKI, sdata);
        BBA_AND(eff, BB_GTK(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, SKA, sdata);
        BBA_AND(eff, BB_GKA(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, SHI, sdata);
        BBA_AND(eff, BB_GHI(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, SUM, sdata);
        BBA_AND(eff, BB_GUM(sdata)); OUTE_OUT(eff,res,sdata,num);
        eff = EFFECT_TBL(pos, SRY, sdata);
        BBA_AND(eff, BB_GRY(sdata)); OUTE_OUT(eff,res,sdata,num);
    }
    return res;
}

void create_pin(sdata_t *sdata){
    int ou = SELF_OU(sdata);
    if(ou == HAND) return;
    memset(S_PINNED(sdata), 0, sizeof(char)*N_SQUARE);
    
    int src, pin;
    bitboard_t src_bb, k_eff, o_eff;
    if(S_TURN(sdata)){
        //SKY
        src_bb = BB_SKY(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            if(g_file[ou]==g_file[src])
            {
                o_eff = EFFECT_TBL(ou, GKY, sdata);
                k_eff = EFFECT_TBL(src, SKY, sdata);
                BBA_AND(k_eff, o_eff);
                pin = min_pos(&k_eff);
                if(pin>=0 && GOTE_KOMA(S_BOARD(sdata, pin))){
                    S_PINNED(sdata)[pin] = g_file[pin]+10;
                    break;
                }
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SKA, SUM
        src_bb = BB_SUK(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            if(g_rslp[ou]==g_rslp[src]||g_lslp[ou]==g_lslp[src]){
                o_eff = EFFECT_TBL(ou, GKA, sdata);
                k_eff = EFFECT_TBL(src, SKA, sdata);
                BBA_AND(k_eff, o_eff);
                pin = min_pos(&k_eff);
                if(pin>=0 && GOTE_KOMA(S_BOARD(sdata, pin))){
                    if(g_rslp[ou] == g_rslp[src])
                        S_PINNED(sdata)[pin] = g_rslp[ou]+30;
                    else if(g_lslp[ou] == g_lslp[src])
                        S_PINNED(sdata)[pin] = g_lslp[ou]+17;
                }
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //SHI, SRY
        src_bb = BB_SRH(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            if(g_rank[ou]==g_rank[src]||g_file[ou]==g_file[src]){
                o_eff = EFFECT_TBL(ou, GHI, sdata);
                k_eff = EFFECT_TBL(src, SHI, sdata);
                BBA_AND(k_eff, o_eff);
                pin = min_pos(&k_eff);
                if(pin>=0 && GOTE_KOMA(S_BOARD(sdata, pin))){
                    if(g_rank[ou] == g_rank[src])
                        S_PINNED(sdata)[pin] = g_rank[ou]+1;
                    else if(g_file[ou] == g_file[src])
                        S_PINNED(sdata)[pin] = g_file[ou]+10;
                }
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
    }
    else             {
        //GKY
        src_bb = BB_GKY(sdata);
        while(1){
            src = max_pos(&src_bb);
            if(src<0) break;
            if(g_file[ou]==g_file[src]){
                o_eff = EFFECT_TBL(ou, SKY, sdata);
                k_eff = EFFECT_TBL(src, GKY, sdata);
                BBA_AND(k_eff, o_eff);
                pin = min_pos(&k_eff);
                if(pin>=0 && SENTE_KOMA(S_BOARD(sdata, pin))){
                    S_PINNED(sdata)[pin] = g_file[pin]+10;
                    break;
                }
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GKA, GUM
       
        src_bb = BB_GUK(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            if(g_rslp[ou]==g_rslp[src]||g_lslp[ou]==g_lslp[src]){
                o_eff = EFFECT_TBL(ou, SKA, sdata);
                k_eff = EFFECT_TBL(src, GKA, sdata);
                BBA_AND(k_eff, o_eff);
                pin = min_pos(&k_eff);
                if(pin>=0 && SENTE_KOMA(S_BOARD(sdata, pin))){
                    if(g_rslp[ou] == g_rslp[src])
                        S_PINNED(sdata)[pin] = g_rslp[ou]+30;
                    else if(g_lslp[ou] == g_lslp[src])
                        S_PINNED(sdata)[pin] = g_lslp[ou]+17;
                }
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
        //GHI, GRY
        
        src_bb = BB_GRH(sdata);
        while(1){
            src = min_pos(&src_bb);
            if(src<0) break;
            if(g_rank[ou]==g_rank[src]||g_file[ou]==g_file[src]){
                o_eff = EFFECT_TBL(ou, SHI, sdata);
                k_eff = EFFECT_TBL(src, GHI, sdata);
                BBA_AND(k_eff, o_eff);
                pin = min_pos(&k_eff);
                if(pin>=0 && SENTE_KOMA(S_BOARD(sdata, pin))){
                    if(g_rank[ou] == g_rank[src])
                        S_PINNED(sdata)[pin] = g_rank[ou]+1;
                    else if(g_file[ou] == g_file[src])
                        S_PINNED(sdata)[pin] = g_file[ou]+10;
                }
            }
            BBA_XOR(src_bb, g_bpos[src]);
        }
    }
    return;
}
/*
void create_effect (sdata_t *sdata){
    bitboard_t effect, eff;
    //先手
    BB_INI(effect);
    BBA_OR(effect, EF_SFU(sdata));
    eff = bb_to_effect(SKY, &BB_SKY(sdata), sdata); BBA_OR(effect, eff);
    eff = bb_to_effect(SKE, &BB_SKE(sdata), sdata); BBA_OR(effect, eff);
    eff = bb_to_effect(SGI, &BB_SGI(sdata), sdata); BBA_OR(effect, eff);
    eff = bb_to_effect(SKI, &BB_STK(sdata), sdata); BBA_OR(effect, eff);
    if(S_SOU(sdata)<N_SQUARE){
        eff = EFFECT_TBL(S_SOU(sdata), SOU, sdata); BBA_OR(effect, eff);
    }
    eff = bb_to_effect(SKA, &BB_SKA(sdata), sdata); BBA_OR(effect, eff);
    eff = bb_to_effect(SHI, &BB_SHI(sdata), sdata); BBA_OR(effect, eff);
    eff = bb_to_effect(SUM, &BB_SUM(sdata), sdata); BBA_OR(effect, eff);
    eff = bb_to_effect(SRY, &BB_SRY(sdata), sdata); BBA_OR(effect, eff);
    SEFFECT(sdata) = effect;
    //後手
    BB_INI(effect);
    BBA_OR(effect, EF_GFU(sdata));
    eff = bb_to_effect(GKY, &BB_GKY(sdata), sdata); BBA_OR(effect, eff);
    eff = bb_to_effect(GKE, &BB_GKE(sdata), sdata); BBA_OR(effect, eff);
    eff = bb_to_effect(GGI, &BB_GGI(sdata), sdata); BBA_OR(effect, eff);
    eff = bb_to_effect(GKI, &BB_GTK(sdata), sdata); BBA_OR(effect, eff);
    if(S_GOU(sdata)<N_SQUARE){
        eff = EFFECT_TBL(S_GOU(sdata), GOU, sdata); BBA_OR(effect, eff);
    }
    eff = bb_to_effect(GKA, &BB_GKA(sdata), sdata); BBA_OR(effect, eff);
    eff = bb_to_effect(GHI, &BB_GHI(sdata), sdata); BBA_OR(effect, eff);
    eff = bb_to_effect(GUM, &BB_GUM(sdata), sdata); BBA_OR(effect, eff);
    eff = bb_to_effect(GRY, &BB_GRY(sdata), sdata); BBA_OR(effect, eff);
    GEFFECT(sdata) = effect;
    return;
}
 */
void create_effect (sdata_t *sdata){
    bitboard_t effect, eff;
    //先手
    BB_INI(effect);
    BBA_OR(effect, EF_SFU(sdata));
    bb_to_eff(&effect, SKY, &BB_SKY(sdata), sdata);
    bb_to_eff(&effect, SKE, &BB_SKE(sdata), sdata);
    bb_to_eff(&effect, SGI, &BB_SGI(sdata), sdata);
    bb_to_eff(&effect, SKI, &BB_STK(sdata), sdata);
    if(S_SOU(sdata)<N_SQUARE){
        eff = EFFECT_TBL(S_SOU(sdata), SOU, sdata); BBA_OR(effect, eff);
    }
    bb_to_eff(&effect, SKA, &BB_SKA(sdata), sdata);
    bb_to_eff(&effect, SHI, &BB_SHI(sdata), sdata);
    bb_to_eff(&effect, SUM, &BB_SUM(sdata), sdata);
    bb_to_eff(&effect, SRY, &BB_SRY(sdata), sdata);
    SEFFECT(sdata) = effect;
    //後手
    BB_INI(effect);
    BBA_OR(effect, EF_GFU(sdata));
    bb_to_eff(&effect, GKY, &BB_GKY(sdata), sdata);
    bb_to_eff(&effect, GKE, &BB_GKE(sdata), sdata);
    bb_to_eff(&effect, GGI, &BB_GGI(sdata), sdata);
    bb_to_eff(&effect, GKI, &BB_GTK(sdata), sdata);
    if(S_GOU(sdata)<N_SQUARE){
        eff = EFFECT_TBL(S_GOU(sdata), GOU, sdata); BBA_OR(effect, eff);
    }
    bb_to_eff(&effect, GKA, &BB_GKA(sdata), sdata);
    bb_to_eff(&effect, GHI, &BB_GHI(sdata), sdata);
    bb_to_eff(&effect, GUM, &BB_GUM(sdata), sdata);
    bb_to_eff(&effect, GRY, &BB_GRY(sdata), sdata); 
    GEFFECT(sdata) = effect;
    return;
}
bool is_move_to_dest (int dest, const sdata_t *sdata){
    int src;
    bitboard_t eff = SELF_EFFECT(sdata);
    if(BPOS_TEST(eff,dest)){
        bitboard_t effect;
        //後手番
        if(S_TURN(sdata)){
            //GFU
            effect = EFFECT_TBL(dest, SFU, sdata);
            BBA_AND(effect, BB_GFU(sdata));
            src = min_pos(&effect);
            if(src>=0){
                if(!S_PINNED(sdata)[src]) return false;
            }
            /*
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
             */
            //GKY
            effect = EFFECT_TBL(dest, SKY, sdata);
            BBA_AND(effect, BB_GKY(sdata));
            src = min_pos(&effect);
            if(src>=0){
                if(!S_PINNED(sdata)[src]) return false;
            }
            /*
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
             */
            //GKE
            effect = EFFECT_TBL(dest, SKE, sdata);
            BBA_AND(effect, BB_GKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GGI
            effect = EFFECT_TBL(dest, SGI, sdata);
            BBA_AND(effect, BB_GGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKI,GTO,GNY,GNK,GNG
            effect = EFFECT_TBL(dest, SKI, sdata);
            BBA_AND(effect, BB_GTK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GKA
            effect = EFFECT_TBL(dest, SKA, sdata);
            BBA_AND(effect, BB_GKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GHI
            effect = EFFECT_TBL(dest, SHI, sdata);
            BBA_AND(effect, BB_GHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GUM
            effect = EFFECT_TBL(dest, SUM, sdata);
            BBA_AND(effect, BB_GUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
            //GRY
            effect = EFFECT_TBL(dest, SRY, sdata);
            BBA_AND(effect, BB_GRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
        }
        //先手番
        else             {
            //SFU
            effect = EFFECT_TBL(dest, GFU, sdata);
            BBA_AND(effect, BB_SFU(sdata));
            src = min_pos(&effect);
            if(src>=0){
                if(!S_PINNED(sdata)[src]) return false;
            }
            /*
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
             */
            //SKY
            effect = EFFECT_TBL(dest, GKY, sdata);
            BBA_AND(effect, BB_SKY(sdata));
            src = min_pos(&effect);
            if(src>=0){
                if(!S_PINNED(sdata)[src]) return false;
            }
            /*
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
             */
            //SKE
            effect = EFFECT_TBL(dest, GKE, sdata);
            BBA_AND(effect, BB_SKE(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SGI
            effect = EFFECT_TBL(dest, GGI, sdata);
            BBA_AND(effect, BB_SGI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKI,STO,SNY,SNK,SNG
            effect = EFFECT_TBL(dest, GKI, sdata);
            BBA_AND(effect, BB_STK(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SKA
            effect = EFFECT_TBL(dest, GKA, sdata);
            BBA_AND(effect, BB_SKA(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SHI
            effect = EFFECT_TBL(dest, GHI, sdata);
            BBA_AND(effect, BB_SHI(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SUM
            effect = EFFECT_TBL(dest, GUM, sdata);
            BBA_AND(effect, BB_SUM(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
            //SRY
            effect = EFFECT_TBL(dest, GRY, sdata);
            BBA_AND(effect, BB_SRY(sdata));
            while(1){
                src = min_pos(&effect);
                if(src<0) break;
                if(!S_PINNED(sdata)[src]) return false;
                BBA_XOR(effect, g_bpos[src]);
            }
        }
    }
    return true;
}
bool is_evasion_drop (int dest, const sdata_t *sdata){
    if(SELF_HI(sdata)) return false;
    if(SELF_KA(sdata)) return false;
    if(SELF_KI(sdata)) return false;
    if(SELF_GI(sdata)) return false;
    if(SELF_KE(sdata) && KE_CHECK(sdata, dest)) return false;
    if(SELF_KY(sdata) && KY_CHECK(sdata, dest)) return false;
    if(SELF_FU(sdata) && FU_CHECK(sdata, dest)) return false;
    return true;
}

void bb_to_eff              (bitboard_t       *eff,
                             komainf_t         koma,
                             const bitboard_t *bb_koma,
                             const sdata_t    *sdata      )
{
    int pos;
    bitboard_t bb = *bb_koma;
    while(1){
        pos = min_pos(&bb);
        if(pos<0) break;
        BBA_OR(*eff, EFFECT_TBL(pos, koma, sdata));
        BBA_XOR(bb, g_bpos[pos]);
    }
}
