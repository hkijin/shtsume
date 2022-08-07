//
//  ntest.c
//  nsolver
//
//  Created by Hkijin on 2022/02/04.
//

#include <sys/types.h>
#include <sys/sysctl.h>
#include <stdlib.h>
#include "ntest.h"

/*
 * built in testのハンドラー
 */
static char *st_testid[] =
{
    "test1",
    NULL
};

void test_handler(const char *optarg)
{
    int i=0;
    while(st_testid[i]){
        if(!strncmp(optarg, st_testid[i], strlen(st_testid[i])))
        {
            switch(i){
                case 0:
                    tsume_test();
                    //test_generate_evasion();
                    break;
                default:
                    break;
            }
            break;
        }
        i++;
    }
    return;
}

void test_init_distance(void)
{
    init_distance();
    //g_distanceデータをprint
    for(int i=0; i<N_SQUARE; i++){
        printf("i=%2d\n", i);
        for(int j=0; j<9; j++){
            for(int k=8; k>=0; k--){
                printf(" %d", g_distance[i][j*9+k]);
            }
            printf("\n");
        }
    }
    return;
}

void tsume_test(void)
{
    //詰将棋問題のsfen文字列
    const char p_string [128] =
    //Astral390
    //"position sfen l7g/4kp1Sn/1P+R1gs3/3+p+p1s2/B1p+r1P3/"
    //"L8/+n3NgB+pP/1+p3G1+p1/P1PsP4 b Lnl5p 1";
    //図巧98番
    //"position sfen 8k/9/9/9/9/9/9/9/9 b R2GSr2b2g3s4n4l18p 1";
    //驚愕の曠野
    //"position sfen 4k4/9/9/9/9/9/9/9/9 b B4G2S9P2rb2s4n4l9p 1";
    //STARSHIP TROOPER
    //"position sfen l1pp1p2G/Ppn2R+p2/pn2k+pp2/n3+pgsP1/"
    //"l3PP+s2/B2+ppG2P/b8/Ls2S4/1rP3NgL b P 1";
    //FAIRWAY
    //"position sfen l3+P2B+p/k2+rpg+P2/3+p+rPS1n/"
    //"PP1s4p/3l+p+B1pP/L2n4g/+p4p+N2/5S1s+P/+PL+P+pg+N1G1 b - 1";
    //回転木馬
    //"position sfen G1gp3kl/G1SS+B3P/G3R4/+pp4p1+p/"
    //"5+p1n+l/4+p1nN1/3+p1SNPL/p1+p2pL1+p/b+p1S5 b r3p 1";
    //木星の旅
    //"position sfen 3pl3p/1+Pn3pk1/2+p6/4+pG3/1K3p2P/"
    //"+B5l1+B/+p3pS1+ll/p+p2PN3/3S1+sSPN b 2R2P3gn2p 1";
    //木星の旅390
    //"position sfen 3pl3p/5P3/1k7/3+p+p1+B2/1K6P/5pl2/"
    //"4+BS1+ll/4PN3/3S1+sSPN b N2P2r4gn7p 1";
    //木星の旅406
    //"position sfen 3+Bl3p/k1p2P3/9/N+B1+p+p4/"
    //"1K6P/5pl2/5S1+ll/4PN3/3S1+sSPN b 2r4gn9p 1";
    //墨酔38
    //"position sfen 9/9/6G2/2pppGRLl/2+NS3ls/"
    //"1k1+N1+p+pG1/lsb1+p3P/1+p+N2pppB/+R5GS1 b 7Pn 1";
    //岡村３八玉-10
    //"position sfen 9/9/9/9/9/9/4G4/9/3N1k3 b 3G3S2P2r2bs3n4l16p 1";
    //岡村3八玉
    //"position sfen 9/9/9/9/9/9/9/6k2/9 b 4G4SNL3P2r2b3n3l15p 1";
    //赤兎馬34手目
    //"position sfen 2s6/1P+PPgp1k1/P+pB1+p2lN/s3G2L1/2+P3L2/"
    //"2N4N1/4pr2L/Bp4ppP/+pR2P3S b N2gs3p 1";
    //ゴゴノソラ
    //"position sfen B1gs2+N1g/gP3nS1P/1ppRp2N+P/"
    //"B4p+pk1/6l2/9/n8/4KLP1L/5PSrs b gl8p 1";
    //墨酔6
    //"position sfen l6R1/3p4L/1+n6+P/P3n1R+P+B/"
    //"p3p1pS1/6n2/1Sp5k/3Pl4/3B1g1G+p b SN2gsl8p 1";
    //墨酔38
    //"position sfen 9/9/6G2/2pppGRLl/2+NS3ls/"
    //"1k1+N1+p+pG1/lsb1+p3P/1+p+N2pppB/+R5GS1 b 7Pn 1";
    //墨酔38_22
    //"position sfen 9/9/6G2/2pppG1Ll/7ls/7k1/"
    //"l3+R3P/5pppB/6GS1 b 11Prbg2s4n 1";
    //墨酔38_H
    //"position sfen 9/9/6G2/2pppG1Ll/7ls/3+Rn2k1/l7P/"
    //"5pppB/6GS1 b N9Prbg2s2n2p 1";
    //乱
    //"position sfen 1+p3lp+s1/1n2P2Pn/sp2pG1N1/"
    //"G4G3/9/8P/p1pp1p2k/LNP3B1p/S+R1B3G+s b R4P2lp 1";
    //乱変化図
    //"position sfen 1+p3lp+s1/1n2P2Pn/sp1+BpG1N1/"
    //"G4G3/9/9/3k5/LNP5p/S4+R2+s b Brg2l10p 1";
    //ミクロコスモス
    //"position sfen g1+P1k1+P+P+L/1p3P3/+R+p2pp1pl/1NNsg+p2+R/"
    //"+b+nL+P1+p3/1P3ssP1/2P1+Ps2N/4+P1P1L/+B5G1g b - 1";
    //赤兎馬478手目
    //"position sfen 2s6/1P+PPgp3/Ps4klN/4G1N2/2+P2+pL2/"
    //"2N1+B2L1/4pr2L/6ppP/4P3S b N3Prb2gs3p 1";
    //赤兎馬470手目
    //"position sfen 2s4k1/1P+PPgp1n1/Ps5L1/4G4/2+P2+pL2/"
    //"2N1+B4/4pr2L/6ppP/4P3S b 2NL3Prb2gs3p 1";
    //新桃花源
    //"position sfen p2+N3g1/P2R3+pP/g3+BnpP1/lPP1+p3p/"
    //"+LnnS2+LS+p/G+P1l+Bp+P1+p/3pp1P1g/9/4kSR1s b - 1";
    //雪月花
    //"position sfen 3npp1pp/5+n1ln/5g3/9/5rp2/"
    //"R1N5k/ss3g2l/1pppP2LG/2bb1+sS2 b GL9p 1";
    //雪月花256
    //"position sfen 3npp1pp/5+n1ln/3+R1g3/8k/5rp1l/"
    //"2N5L/5g3/1pppP2L1/2bb1+sS2 b 2G2S9p 1";
    //小涌谷
    //"position sfen 3g2n1+P/2nP1p1P1/2nS2k1+P/2G+p+P2s+p/2+p+p1LpL1/"
    //"1+p+p1+plg2/+p+p1+pN3S/2+p3sG1/9 b L2r2b 1";
    //メガロポリス
    "position sfen 2G1+n3S/1gnP+lpkP1/3SS2ps/2G+pNG1+r1/"
    "2+p+pp+ppL1/1+p+p1+p4/+p+p1+p2N2/2+p2P3/9 b R2L2b 1";
    //図巧8番
    //"position sfen 4+bn1g1/5kpPl/2l1b1s2/"
    //"3+l1N1GP/4G1+P1N/5L3/9/1+r7/R8 b GP3sn13p 1";
    //図巧7番
    //"position sfen 1nkg5/2s6/3rp4/1B7/r8/1P7/9/9/9 b BG3SN2g2n4l16p 1";
    //古典3手詰
    //"position sfen 3sks3/9/4S4/9/9/B8/9/9/9 b S2rb4g4n4l18p 1";
    //二上詰将棋金剛篇
    //"position sfen 7nl/6s2/5N2k/6G2/9/9/9/9/9 b 2RG2b2g3s2n3l18p 1";
    //二上詰将棋金剛篇
    //"position sfen 6Snl/8k/9/8s/8s/9/9/9/9 b RGSr2b3g3n3l18p 1";
    //Astral Traveler(579手詰)
    //"position sfen lp2p3g/+R4p1Sn/1P1kgsp1p/3+p+p1s2/"
    //"B1p+r1P3/L8/+n3NgB+pP/+p4G1+p1/P+pPsP4 b Lnl 1";
    //図巧100番
    //"position sfen g3p3+r/3lPG3/1s4ppp/1l1B4P/L2b1kPl1/"
    //"3+p3+R+p/3+p2+s1S/+NP+p1gS3/1g+p2P3 b 4P3n 1";
    //シンメトリー
    //"position sfen 1n1p1p3/1+Ppn1BpsG/sp1+pG+psps/1k1n+Bn2p/"
    //"2l3l2/9/l3p3l/7R1/1PP1G1gP1 b 4Pr 1";
    //イオニゼーション
    //"position sfen p+P+Pg5/+B1sg2S2/+Nn+Pg2+P1S/2+P1+P+P+P1p/"
    //"k4+P3/r1N2+P3/+p+PP5+R/L1L5+l/sB+P5g b nl2p 1";
    //橋本587手詰
    //"position sfen 7g1/4Pgl+b1/2nn1s2+P/1+p+pS1+p1PP/"
    //"+p+p+p2lk1+P/1+ppPpp3/ssPp4N/2NR1L3/2G3rLG b b 1";
    //1980
    //"position sfen 1l1G4g/S4pPSn/4+p2n1/+P+p1Brglp1/"
    //"N2s1P+R+P1/3+pk1+P1l/3+p1+s2p/1P+p+np+p2l/1g1+pB2+p1 b - 1";
    //明日香
    //"position sfen s1+P2+p+plG/1P+B+p1+PP2/g1+pn4+L/+P1sr+p3G/"
    //"1+p1+P1+p2l/5pn1k/2p1+B4/+Pp+RNgs1L1/1+P2N1S2 b - 1";
    //無銭旅行
    //"position sfen +P1K2+P+n+pr/+l+Ln2n3/+PR7/2L+p+p1k2/"
    //"3+p1+pp1B/3+P1+p3/3p3n1/pB1P4l/P5+P+p1 b 4g4sp 1";
    //一手詰めテスト
    //"position sfen 9/9/9/1B7/9/7+S1/7+R1/9/8k b rb4g3s4n4l18p 1";
    //一手詰めテスト２
    //"position sfen 8k/8p/9/9/9/9/9/9/R6L1 b r2b4g4s4n3l17p 1";
    
    //基本ライブラリの初期化
    create_seed();                        //zkey
    init_distance();                      //g_distance
    init_bpos();                          //bitboard
    init_effect();                        //effect
    srand((unsigned)time(NULL));
    
    //logfile作成
    char *path = getenv("HOME");
    strncpy(g_logfile_path,path, strlen(path));
    strncat(g_logfile_path, "/Documents/", sizeof("/Documents/"));
    create_log_filename();
    strncpy(g_user_path,g_logfile_path, sizeof(g_logfile_path));
    
    //usiコマンド受信のシミュレーション
    res_usi_cmd();
    res_setoption_cmd("setoption name USI_Ponder value true");
    res_setoption_cmd("setoption name USI_Hash value 6144" );
    //res_setoption_cmd("setoption name mt_min_pn value 4");
    //res_setoption_cmd("setoption name n_make_tree value 2");
    res_setoption_cmd("setoption name search_level value 0");
    res_setoption_cmd("setoption name out_lvkif value true");
    res_setoption_cmd("setoption name summary value true");
    char commandstr[256];
    sprintf(commandstr, "setoption name user_path value %s", g_user_path);
    //res_setoption_cmd("setoption name user_path value /Documents");
    res_setoption_cmd(commandstr);
    printf("g_usi_ponder   = %d\n", g_usi_ponder);
    printf("g_usi_hash     = %llu\n", g_usi_hash  );
    //printf("g_mt_min_pn    = %u\n", g_mt_min_pn);
    //printf("g_n_make_tree  = %u\n", g_n_make_tree );
    printf("g_search_level = %u\n", g_search_level );
    printf("g_out_lvkif    = %s\n", g_out_lvkif?"true":"false");
    printf("g_summary      = %s\n", g_summary?"true":"false");
    res_isready_cmd();
    res_position_cmd(p_string);
    
    //読み込み局面の確認
    SDATA_PRINTF(&g_sdata, PR_BOARD);
    res_gomate_cmd("go mate infinite");
    return;
}

//cpu情報の取得
void cpu_info(void)
{
    int res;
    int num;
    size_t len;
    res = sysctlbyname("hw.physicalcpu_max", &num, &len, NULL, 0);
    if(res<0) return;
    printf("physicalcpu_max  = %u \n", num);
    
    return;
}

//王手に対する合法手表示
void test_generate_evasion(void)
{
    //詰将棋問題のsfen文字列
    const char p_string [128] =
    //メタ新世界313手目
    //"position sfen 1g7/1S1+R2+p1B/N1Nrp4/2pk1L1n1/1g1p2+pg1/2sSL1+p+p1/"
    //"5+p1+p+p/N1g2+p+p2/8B w Lsl6p 1";
    //メガロポリス107手目
    //"position sfen 2G1+n1R1S/1gnP+lplP1/3S2kps/2G+pN4/"
    //"2+p2LpL1/1+p+p1+p4/+p+p1+p2N2/1B+p2P3/9 w Prbgs2p 1";
    //メガロポリス63手目
    "position sfen 2G1+n1R1S/1gnP+lplP1/3S2kps/2G+pN4/"
    "2+p+pBLpL1/1+p+p1+p4/+p+p1+p2N2/2+p2P3/9 w Prbgsp 1";
    
    //基本ライブラリの初期化
    create_seed();                        //zkey
    init_distance();                      //g_distance
    init_bpos();                          //bitboard
    init_effect();                        //effect
    srand((unsigned)time(NULL));
    
    //logfile作成
    create_log_filename();
    
    //usiコマンド受信のシミュレーション
    res_usi_cmd();
    res_setoption_cmd("setoption name USI_Ponder value true");
    res_setoption_cmd("setoption name USI_Hash value 256" );
    res_setoption_cmd("setoption name search_level value 0");
    printf("g_usi_ponder   = %d\n", g_usi_ponder);
    printf("g_usi_hash     = %llu\n", g_usi_hash  );
    printf("g_search_level = %u\n", g_search_level );
    res_isready_cmd();
    res_position_cmd(p_string);
    
    //読み込み局面の確認
    SDATA_PRINTF(&g_sdata, PR_BOARD);
    mvlist_t *mvlist = generate_evasion(&g_sdata, g_tbase);
    MVLIST_PRINTF(mvlist, &g_sdata);
    
    return;
}

/*
//古典3手詰
"position sfen 3sks3/9/4S4/9/9/B8/9/9/9 b S2rb4g4n4l18p 1",
//Astral Traveler(579手詰)
"position sfen lp2p3g/+R4p1Sn/1P1kgsp1p/3+p+p1s2/"
"B1p+r1P3/L8/+n3NgB+pP/+p4G1+p1/P+pPsP4 b Lnl 1";
//回転木馬
"position sfen G1gp3kl/G1SS+B3P/G3R4/+pp4p1+p/"
"5+p1n+l/4+p1nN1/3+p1SNPL/p1+p2pL1+p/b+p1S5 b r3p 1";
 
//fairway378h
//"position sfen l3+P2B+p/3+rpg+P2/3+p+rPS1+B/"
//"P2s4p/3l4P/L2n1p2g/1+P4+N2/3k1S1s+P/1L+p1g+N1G1 b 2Pn3p 1";
//新桃花源
//"position sfen p2+N3g1/P2R3+pP/g3+BnpP1/lPP1+p3p/"
//"+LnnS2+LS+p/G+P1l+Bp+P1+p/3pp1P1g/9/4kSR1s b - 1";
//乱
//"position sfen 1+p3lp+s1/1n2P2Pn/sp2pG1N1/"
//"G4G3/9/8P/p1pp1p2k/LNP3B1p/S+R1B3G+s b R4P2lp 1";
//二上詰将棋金剛篇
//"position sfen 7nl/6s2/5N2k/6G2/9/9/9/9/9 b 2RG2b2g3s2n3l18p 1";
//図巧8番
"position sfen 4+bn1g1/5kpPl/2l1b1s2/"
"3+l1N1GP/4G1+P1N/5L3/9/1+r7/R8 b GP3sn13p 1";
//世界大戦
//"position sfen p+R2g3S/np1P2LPP/1n3+LP2/2n2S+R2/"
//"2L2S+PpS/2+P1p3n/4Lk3/3+p3+pG/+p3+B1b2 b 2G5p 1";
//雪月花
//"position sfen 3npp1pp/5+n1ln/5g3/9/5rp2/"
//"R1N5k/ss3g2l/1pppP2LG/2bb1+sS2 b GL9p 1";
//いばらの森
//"position sfen 2l1p+P+p+P1/1g+P2LLs+p/2sN1P+pP+s/1P+rP2PsP/"
//"1L+P1B1+p+P+P/1k1N+B1+RN1/1g7/p1G5N/6g2 b - 1";
//図巧100番
//"position sfen g3p3+r/3lPG3/1s4ppp/1l1B4P/L2b1kPl1/"
//"3+p3+R+p/3+p2+s1S/+NP+p1gS3/1g+p2P3 b 4P3n 1";
//無駄合い判定
//"position sfen 8l/7sk/7R1/6B2/9/8b/9/9/9 b Sr4g2s4n3l18p 1";
//夢の旅人
//"position sfen 9/S3p3n/G2p5/1G7/2P+B4N/"
//"+rnB2S2R/N3G4/2L2ppp1/G1sks4 b 12P3l 1";
//成吉思汗(夢まぼろし51番）
//"position sfen g2g2p2/+Ngs2p2+N/2l1p2p1/1pSp5/"
//"n+Lp4S1/l6g+R/2k1+B2+p1/2B2s3/2N2R2L b P9p 1";
//ACTの夢(467手詰)
//"position sfen 1l+PG1+P3/b1P1n4/9/1l1B1P2S/L2P2P2/"
//"3+pN2l1/3+ppp1kN/pP+psS4/3+rPg+p1+R b 2P2gsnp 1";
//ミクロコスモス
"position sfen g1+P1k1+P+P+L/1p3P3/+R+p2pp1pl/1NNsg+p2+R/"
"+b+nL+P1+p3/1P3ssP1/2P1+Ps2N/4+P1P1L/+B5G1g b - 1";
//天月舞
// "position sfen Gp2ssbb1/LG7/2LLSS3/8K/"
// "2pppppp1/7n1/kL7/p1NNN4/G3G4 b R3Pr7p 1";
//ゴゴノソラ
"position sfen B1gs2+N1g/gP3nS1P/1ppRp2N+P/"
"B4p+pk1/6l2/9/n8/4KLP1L/5PSrs b gl8p 1";
//木星の旅
//"position sfen 3pl3p/1+Pn3pk1/2+p6/4+pG3/1K3p2P/"
//"+B5l1+B/+p3pS1+ll/p+p2PN3/3S1+sSPN b 2R2P3gn2p 1";
//井上305
//"position sfen gl7/p2P5/5N3/2S1sG3/2lS4p/"
//"8G/3p1pplb/1kS1rNl1+n/3+RNbG1P b 10Pp 1";
//メタ新世界700
//"position sfen 1g7/1S1+R2+p1B/N1Nrp4/2pk1L1n1/"
//"1g1p2+pg1/2sSL1+p2/6+p2/N1g2+p3/9 b LPbsl9p 1";
//メタ新世界800
//"position sfen 1g1R5/1S1r2+p1B/N1Nkp4/2p2L1n1/"
//"1g1p2+pg1/2sSL4/6+p2/N1g2+p3/9 b LPbsl10p 1";
//メタ新世界900
//"position sfen 1g1R5/1S2k1+p1B/N1N1p4/"
//"2p2L1n1/1g1p3g1/2sSL+p3/9/N1g2+p3/9 b LPrbsl11p 1";
//二上詰将棋金剛篇
//"position sfen 6Snl/8k/9/8s/8s/9/9/9/9 b RGSr2b3g3n3l18p 1";
//飛燕乱舞100
"position sfen 3s2gn1/g7+N/4lpS1l/4l+P1P1/2+p+p+pLR2/Rp6k/"
"1+p+p+p+pGB1+n/5N1p+s/5G2S b b6p 1";
//BrandnewWay
 "position sfen r2k5/l2p1+P3/1s7/1+PsL2nn1/"
 "P1+r+p2pp1/SppL+P2np/5B2n/9/9 b BSP4gl6p 1";
 //無双1番
 "position sfen 3g1n1l1/2p1g1r2/5k2S/4p1N+R1/"
 "3+p5/7N1/B8/9/9 b 2GSNb2s3l15p 1";
 //シンメトリー
 "position sfen 1n1p1p3/1+Ppn1BpsG/sp1+pG+psps/1k1n+Bn2p/"
 "2l3l2/9/l3p3l/7R1/1PP1G1gP1 b 4Pr 1";
 //メガロポリス
 "position sfen 2G1+n3S/1gnP+lpkP1/3SS2ps/2G+pNG1+r1/"
 "2+p+pp+ppL1/1+p+p1+p4/+p+p1+p2N2/2+p2P3/9 b R2L2b 1";
 //小涌谷
 "position sfen 3g2n1+P/2nP1p1P1/2nS2k1+P/2G+p+P2s+p/2+p+p1LpL1/"
 "1+p+p1+plg2/+p+p1+pN3S/2+p3sG1/9 b L2r2b 1"
 //岡村3八玉
 "position sfen 9/9/9/9/9/9/9/6k2/9 b 4G4SNL3P2r2b3n3l15p 1";
 //岡村３八玉-2
 "position sfen 9/9/9/9/9/9/5k3/9/9 b 4G3SNL3P2r2bs3n3l15p 1";
 //岡村３八玉-10
 "position sfen 9/9/9/9/9/9/4G4/9/3N1k3 b 3G3S2P2r2bs3n4l16p 1";
 //雪月花256
 "position sfen 3npp1pp/5+n1ln/3+R1g3/8k/5rp1l/"
 "2N5L/5g3/1pppP2L1/2bb1+sS2 b 2G2S9p 1"
 //墨酔38
 "position sfen 9/9/6G2/2pppGRLl/2+NS3ls/"
 "1k1+N1+p+pG1/lsb1+p3P/1+p+N2pppB/+R5GS1 b 7Pn 1";
 //FAIRWAY
 "position sfen l3+P2B+p/k2+rpg+P2/3+p+rPS1n/"
 "PP1s4p/3l+p+B1pP/L2n4g/+p4p+N2/5S1s+P/+PL+P+pg+N1G1 b - 1";
 //橋本587手詰
 "position sfen 7g1/4Pgl+b1/2nn1s2+P/1+p+pS1+p1PP/"
 "+p+p+p2lk1+P/1+ppPpp3/ssPp4N/2NR1L3/2G3rLG b b 1";
 //1980
 "position sfen 1l1G4g/S4pPSn/4+p2n1/+P+p1Brglp1/"
 "N2s1P+R+P1/3+pk1+P1l/3+p1+s2p/1P+p+np+p2l/1g1+pB2+p1 b - 1";
 //STARSHIP TROOPER
 "position sfen l1pp1p2G/Ppn2R+p2/pn2k+pp2/n3+pgsP1/"
 "l3PP+s2/B2+ppG2P/b8/Ls2S4/1rP3NgL b P 1";
 //赤兎馬34手目
 "position sfen 2s6/1P+PPgp1k1/P+pB1+p2lN/s3G2L1/2+P3L2/"
 "2N4N1/4pr2L/Bp4ppP/+pR2P3S b N2gs3p 1";
 //赤兎馬478手目
 "position sfen 2s6/1P+PPgp3/Ps4klN/4G1N2/2+P2+pL2/"
 "2N1+B2L1/4pr2L/6ppP/4P3S b N3Prb2gs3p 1";
 //驚愕の曠野
 "position sfen 4k4/9/9/9/9/9/9/9/9 b B4G2S9P2rb2s4n4l9p 1";
 //図巧98番
 "position sfen 8k/9/9/9/9/9/9/9/9 b R2GSr2b2g3s4n4l18p 1";
*/
