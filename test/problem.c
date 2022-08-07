//
//  problem.c
//  shshogi
//
//  Created by Hkijin on 2021/11/15.
//

#include <stdio.h>
#include "shogi.h"
#include "tests.h"

//基本テスト用
//王手回避関数テスト用
char *g_sfen_defense[] = {
    "ln3gknl/4gs1S1/3p1p1p1/p3p1p1p/1pRP5/P3P3P/1P3PPP1/4G1SK1/LN3G1NL"
    " w RBPbsp 1",
    "7nl/5+B3/8p/6+R1k/8s/7Ps/9/9/9 w rb4g3n3l16p 1",           //2
    "3p2knl/4BB3/9/4S4/9/8L/9/9/9 w GP2r3g3s3n2l16p 1",
    "4+R3l/9/6+R25kS2/9/9/9/9/9 w L2b4g3s4n2l18p 1",
    "ln3g1nl/4gskb1/3p1pSp1/p3p1p1p/1prP5/P3P3P/1PB2PPP1/2R1G1SK1/LN3G1NL"
    " w Psp 1" ,
    NULL
};
//王手生成関数テスト用
char *g_sfen_oute[] = {
    // エラー局面
    "9/9/9/9/9/7n1/6NPP/6P1r/2+p4K1 w R2B4G4SN4L13Pnp 1",
    "6Snl/5+B3/8p/7+R1/9/4S2Ps/7k1/9/9 b rb4gs3n3l16p 1",
    //持ち駒による王手
    "4k4/9/9/9/9/9/9/9/9 b P2r2b4g4s4n4l17p 1",     //裸玉　先手番　持ち駒 歩
    "9/9/9/9/9/9/9/9/4K4 w 2R2B4G4S4N4L17Pp 1",     //裸玉　後手番　持ち駒 歩
    "4k4/9/9/9/9/9/9/9/9 b L2r2b4g4s4n3l18p 1",     //裸玉　先手番　持ち駒 香
    "9/9/9/9/9/9/9/9/4K4 w 2R2B4G4S4N3L18Pl 1",     //裸玉　後手番　持ち駒 香
    "4k4/9/9/9/9/9/9/9/9 b N2r2b4g4s3n4l18p 1",     //裸玉　先手番　持ち駒 桂
    "9/9/9/9/9/9/9/9/4K4 w 2R2B4G4S3N4L18Pn 1",     //裸玉　後手番　持ち駒 桂
    "4k4/9/9/9/9/9/9/9/9 b S2r2b4g3s4n4l18p 1",     //裸玉　先手番　持ち駒 銀
    "9/9/9/9/9/9/9/9/4K4 w 2R2B4G3S4N4L18Ps 1",     //裸玉　後手番　持ち駒 銀
    "4k4/9/9/9/9/9/9/9/9 b G2r2b3g4s4n4l18p 1",     //裸玉　先手番　持ち駒 金
    "9/9/9/9/9/9/9/9/4K4 w 2R2B3G4S4N4L18Pg 1",     //裸玉　後手番　持ち駒 金
    "4k4/9/9/9/9/9/9/9/9 b B2rb4g4s4n4l18p 1",      //裸玉　先手番　持ち駒 角
    "9/9/9/9/9/9/9/9/4K4 w 2RB4G4S4N4L18Pb 1",      //裸玉　後手番　持ち駒 角
    "4k4/9/9/9/9/9/9/9/9 b Rr2b4g4s4n4l18p 1",      //裸玉　先手番　持ち駒 飛
    "9/9/9/9/9/9/9/9/4K4 w R2B4G4S4N4L18Pr 1",      //裸玉　後手番　持ち駒 飛
    //打ち歩詰め回避
    "6lkn/9/7S1/9/9/9/9/9/9 b P2r2b4g3s3n3l17p 1",  //打ち歩詰め　先手番
    "9/9/9/9/9/9/1s7/9/NKL6 w 2R2B4G3S3N3L17Pp 1",  //打ち歩詰め　先手番
    //盤上の駒による王手
    "4k4/9/PPPPPPPPP/9/9/9/9/9/9 b 2r2b4g4s4n4l9p 1",  //先手番　歩
    "9/9/9/9/9/9/ppppppppp/9/4K4 w 2R2B4G4S4N4L9P 1",  //後手番　歩
    "4k4/4p4/9/9/9/9/9/9/3LLLL2 b 2r2b4g4s4n18p 1",    //先手番　香
    "2llll3/9/9/9/9/9/9/4P4/4K4 w 2R2B4G4S4N18P 1",    //後手番　香
    "4k4/9/4N4/3N1N3/4N4/9/9/9/9 b 2r2b4g4s4l18p 1",   //先手番　桂
    "9/9/9/9/4n4/3n1n3/4n4/9/4K4 w 2R2B4G4S4L18P 1",   //後手番　桂
    "3SkS3/9/3S1S3/9/9/9/9/9/9 b 2r2b4g4n4l18p 1",     //先手番　銀
    "9/9/9/9/9/9/3s1s3/9/3sKs3 w 2R2B4G4N4L18P 1",     //後手番　銀
    "4G4/9/2G1k1G2/9/4G4/9/9/9/9 b 2r2b4s4n4l18p 1",   //先手番　金
    "9/9/9/9/4g4/9/2g1K1g2/9/4g4 w 2R2B4S4N4L18P 1",   //後手番　金
    "5B3/9/4k4/9/9/9/7B1/9/9 b 2r4g4s4n4l18p 1",       //先手番　角
    "9/9/1b7/9/9/9/4K4/9/3b5 w 2R4G4S4N4L18P 1",       //後手番　角
    "9/9/2R6/9/4k4/9/7R1/9/9 b 2b4g4s4n4l18p 1",       //先手番　飛
    "9/9/2r6/9/4K4/9/7r1/9/9 w 2B4G4S4N4L18P 1",       //後手番　飛
    "4+P4/3+P1+P3/4k4/2+P3+P2/4+P4/9/9/9/9 b 2r2b4g4s4n4l12p 1", //先手番　と
    "9/9/9/9/4+p4/2+p3+p2/4K4/3+p1+p3/4+p4 w 2R2B4G4S4N4L12P 1", //後手番　と
    
    //空き王手
    "8B/9/9/5K3/9/3k5/9/9/b8 b 2r4g4s4n4l18p 1",       //先手番　玉
    "8B/9/9/5K3/9/3k5/9/9/b8 w 2R4G4S4N4L18P 1",       //先手番　玉
    
    NULL    //terminal
};

//静止探索テスト用
char *g_sfen_capture[] = {
    "7n1/6k2/3g5/2p1pp2l/9/l3P1P2/Gr3PN2/K1G6/LN7 b 2SN6Pr2bg2sl6p 1",
    "lr2k4/9/3g1s3/p1p1ppp2/1S7/P3P1P1L/1K3PN2/9/+p8 b G2NL3Pr2b2g2snl5p 1",
    NULL
};

//次の一手形式サンプル
//出典 https://www.shogitown.com/school/judge/judgetop.html
char *g_sfen_problem[] = {
    //"ln6l/1k1+P5/1pp1g2p1/p2pp1p1p/5S3/2P1Pb2P/PP1s5/LG1+p5/KN4+rNL"
    //" b BG2Prg2snp 1",     //2  3
    //"lnB5l/k2+P5/1pp1g2p1/p2pp1p1p/5S3/2P1Pb2P/PP1s5/LG1+p5/KN4+rNL"
    //" b G2Prg2snp 1",
    //NULL,
    //"l3+R1snk/1r1+P4l/2npgp1pp/p5p2/4P1s2/1pPS1P3/PP4PPP/2G1g+p2K/LN4bNL"
    //" b BSgp 1",           //41 50
    //NULL,
    "ln3g1nl/4gskb1/3p1p1p1/p3p1p1p/1prP5/P3P3P/1PB2PPP1/2R1G1SK1/LN3G1NL"
    " b SPsp 1",           //0  1
    "ln1g3nl/1ks1g1r2/1ppp2bp1/p3p3p/5pRP1/P1P1P3P/1P1P1P3/1BKSG4/LN1G3NL"
    " w SPsp 1",
    "lr4knl/6g2/2ng1ssp1/p1p1ppp1p/1p1p3P1/P1PPP1P1P/1PSG1PN2/1KG4R1/LN6L"
    " b BSb 1",            //1  2
    "ln6l/1k1+P5/1pp1g2p1/p2pp1p1p/5S3/2P1Pb2P/PP1s5/LG1+p5/KN4+rNL"
    " b BG2Prg2snp 1",     //2  3
    "ln1g1r1nl/1ks6/pppp3pb/5sp1p/3Npp1P1/2P3P1P/PP1P1P3/1BKSG2R1/L2G4L"
    " b SNgp 1",           //3  4
    "ln3gbnl/1r1sg1k2/5p1p1/p1pppsp1p/9/P2BSPP1P/1+p3S1P1/2G2GK2/LN2R2NL"
    " b 3Pp 1",            //4  5
    "ln1gk2nl/1r1s1sgb1/p1pppp1pp/1p4p2/7P1/2P6/PPSPPPP1P/1B5R1/LN1GKGSNL"
    " b - 1",              //5  6
    "ln5nl/2kg1+R1s1/ppppp2pb/6pPp/3s5/2P1P4/PP1P1PN1P/2KS+p4/LN4+r1L"
    " b GSPb2g 1",         //6  7
    "ln1g3nl/1ks1r1gb1/1ppp3p1/p4pp1p/9/P1PB2P1P/1P1P1PN2/2KSG2R1/LN1G4L"
    " b S2Psp 1",          //7  8
    "ln3ksnl/1r2g2b1/3p1ppgp/p1psp4/1p4S2P1PPP1P2/1P3P1pP/1BGSG1R2/LN1K3NL"
    " b P 1",              //8  9
    "1n3+P1nl/r3g1sk1/l2p1g1p1/2p2pP1p/1p7/p1PPPP1PP/1PS1S4/PKG6/1N4+rNL"
    " b BGLbsp 1",         //9  10
    "ln3+P2l/4g4/1p1p3p1/2p1sppkp/p2n5/2P1pPPGP/PP7/LS1+p5/KNG2r+rNL"
    " b 2SP2bg 1",         //10 11
    "l2+P5/5l2l/2n1sk1+B1/p1pp1pp1p/1p4g2P1PPS4/1PS1bPNP1/1KG+p5/L3+r4"
    " b GSNPrgn2p 1",      //11 12
    "ln1gk1snl/1r4gb1/3p4p/ps2ppp2/1pp6/P1P1P3P/1PSP1PP2/1BG1GS1R1/LN2K2NL"
    " b Pp 1",             //12 13
    "ln5n1/5bgk1/3s1gspl/p2pppp2/1rp4Np/P1PSP1PP1/1P1G1P3/1KGS1B1R1/LN6L"
    " b 3p 1",             //13 14
    "ln1g2+R1l/1ks1g4/pppp3p1/6psp/5p3/2P1p1PbP/PP1P1P3/1BKS5/LN1GG3+r"
    " b SN2Pnl 1",         //14 15
    "l6nl/3r1ggk1/2n1spppp/pb7/1p7/P1PPPPP1P/1PSG2N2/1KGB3R1/LN6L"
    " b S2Ps2p 1",         //15 16
    "lnsgkgsnl/6r2/pppppp1pp/9/7P12P3p2/PP1PPPb1P/1SG2S1R1/LN2KG1NL"
    " b BP 1",             //16 17
    "lR5nl/4ggk2/4sp1pp/p1p1p1p2/1p1Nb4/P1P6/1P3PPPP/4G1SK1/L4G1NL"
    " b BS2Prsnp 1",       //17 18
    "l5knl/1r1sP1g2/2n2p1+R1/p1pP1sp1p/1p7/P1PG1PP1P/1PS6/2G2s3/LNK5L"
    " b GN2P2b2p 1",       //18 21
    "lnsgk2nl/4r1gb1/pppp1pppp/9/4s2P1/2P6/PP1P1PP1P/1B1KGS1R1/LNSG3NL"
    " b Pp 1",             //19 22
    "ln1g3nl/1ks3rb1/1p1g5/p1ppppsRp/9/P1P1PS2P/1P1P1P3/1BKSG4/LN1G3NL"
    " b 2P2p 1",           //20 23
    "ln1g2+R1l/1ks6/1ppp3p1/p3p1p1p/5P1n1/P1PSPpb1P/1P1P5/2KS1g3/LNG3+rNL"
    " b BGPsp 1",          //21 24
    "l6nl/3+P1kg2/p2s1psp1/3P2p1p/1p2PP1P1/2PSnbP1P/PP3+pN2/2GR5/LNK5L"
    " b RGbgsp 1",         //22 27
    "l6nl/1r3bgk1/2np1gsp1/p1ps1p2p/1p5P1/P1PP1PS1P/1PSG5/1KGB3R1/LN5NL"
    " b 2P2p 1",           //23 28
    "7nl/1r4gk1/2ns1gsp1/2p1ppp1p/lp1P3P1/2P1PPPBP/NPSG1SN2/1KG4R1/8L"
    " b B2Plp 1",          //24 29
    "l1+R2g1nl/3+P1sk2/3p1p1p1/p5p1p/1p1n14/P4P2P/1PP3PP1/1+r2+p1SK1/L4G1NL"
    " b BGPbg2sn 1",       //25 31
    "kng3+R1+B/lsg5l/pppp3p1/6p1p/7P1/P1P3PsP/1P1PSP+b2/2KSG4/LN1G3+rl"
    " b 3P2n 1",           //26 32
    "ln3g1nl/1r1sg1kb1/p2p1p1p1/2P1p1p1p/1psP5/4P3P/PPS2PPP1/3RG1SK1/LN3G1NL"
    " b BP 1",             //27 34
    "ln3k1nl/1r1sg1g2/p2p2spp/1ppbppp2/6P2/2PPP4/PPSG1P1PP/2G2S1R1/LNBK3NL"
    " b - 1",              //28 35
    "l2g3nl/1+b2g1k2/p2psp3/4p1pRp/1p3P3/2PPP1P2/PP6P/2GS1+r3/LNK4NL"
    " b GSN2Pbsp 1",       //29 36
    "l4+R1nl/3P4k/2n+P3pp/p4gp2/1pGp5/P1P1P1P2/1PSG1P2P/1KG1s4/LN3br1L"
    " b N3Pb2s 1",         //30 37
    "ln3g1nl/1Rsg2sb1/5p1k1/p2Pp1ppp/9/P1P1P3P/4SPPP1/L3G1SK1/1N3G1NL"
    " b B2Pr2p 1",         //31 38
    "ln3g1nl/1r1sg1kb1/p3sp1p1/4p1p1p/1ppp5/2PPPP2P/PPBS1GPP1/3R2SK1/LN3G1NL"
    " b - 1",              //32 39
    "ln1g3nl/1ks1gr1b1/1pppp4/p3sPp1p/7p1/P1P1S1PPP/1P1PP4/1BK1GR3/LNSG4L"
    " b Pn 1",             //33 40
    "l3G2nl/2g4+R1/1kSs3pp/1Bps1pp2/3N3NP/p1P2PPP1/1+p3S3/P8/LKG5L"
    " b B2Prgn3p 1",       //34 42
    "ln+RR1n1nl/4+Bs1k1/p4p1pp/4G4/2p1P1p2/PBPP5/1P3PPPP/1S3ggSL/L1G4NK"
    " b 2Psp 1",           //35 44
    "ln3S1ln/4+B1gk1/p1p1Gp1pp/3p2p2/4Pg1PP/3P2P2/PP3PN2/K2+r5/L2+p5"
    " b RBGSNL2P2s 1",     //36 45
    "ln5kl/6sg1/2pp1+B+Bp1/p4pp1p/4pg3/P1P5P/1P1PGP3/2SKP1r2/LN4+rNL"
    " b GSsn3p 1",         //37 46
    "l3+R3l/6gk1/3psg1p1/p1p3pnp/1p2P2N1/P1PP2PPP/1PS6/1KG3+r2/LN1b4L"
    " b 2SNPbg2p 1",       //38 47
    "6knl/4+Ps3/3p2ppp/p5s2/1p1G5/2P2BPnP/PPNP5/3GS3L/L5+r1K"
    " b RBGSNL2Pg3p 1",    //39 48
    "l4k2l/3+P3s1/5pn2/p1p3p1p/6n2/P1P1pP3/1P4PpP/2r3S1L/LN4GNK"
    " b B2GSPrbgs3p 1",    //40 49
    "l3+R1snk/1r1+P4l/2npgp1pp/p5p2/4P1s2/1pPS1P3/PP4PPP/2G1g+p2K/LN4bNL"
    " b BSgp 1",           //41 50
    NULL                   //terminal
};

//詰将棋　出典：二上詰将棋　金剛篇 他
char *g_sfen_tsume[]   = {
    /*
    "k1+P4n1/2L+P2sL1/r4+P+P1P/+BpP+Pl1+Rg1NP1S+PP+p1g/"
    "2L+p1g+P1+P/Ps1G1+P1N1/1sN1P4/B8 b - 1",
    NULL,
     */
    "3sks3/9/4S4/9/9/B8/9/9/9 b S2rb4g4n4l18p 1",                //古典３手詰め
    "6Snl/8k/9/8s/8s/9/9/9/9 b RGSr2b3g3n3l18p 1",               //1
    "7nl/5+B1k1/8p/7s1/9/7Ps/9/9/9 b R2Srb4g3n3l16p 1",          //2
    "7nl/6s2/5N2k/6G2/9/9/9/9/9 b 2RG2b2g4s2n3l18p 1",           //3
    "5+b1nl/7R1/7lk/6p1p/4B4/6G2/8+p/9/9 b RS3g3s3n2l15p 1",     //4
    "3p1k1nl/5p3/9/4S4/8B/8L/9/9/9 b BG2r3g3s3n2l16p 1",         //5
    "4+R2nl/6k2/4B1p2/6g1p/9/9/9/9/9 b GSPrb2g3s3n3l15p 1",      //6
    "6snl/7+P1/5p3/6S1k/6+p1p/9/9/9/9 b 2BG2r3g2s3n3l14p 1",     //7
    "7nl/6k2/7pb/5Rp1p/5P3/7P1/9/9/9 b G2Srb3g2s3n3l13p 1",      //8
    "7nl/5+P1k1/5+B2p/8s/7+pP/9/9/9/9 b 3SN2rb4g2n3l14p 1",      //9
    "7nl/3+P1gks1/4R4/3+B3p16P1s/9/9/9/9 b GSrb2gs3n3l15p 1",    //10
    "3Bl1Snl/9/3s3kp/5b3/5N2P/9/9/9/9 b RSr4gs2n2l16p 1",        //11
    "5g1nl/2+P1g1k2/3b1pp1+P/4R4/9/9/9/9/9 b B2Sr2g2s3n3l14p 1", //12
    "7nl/5gBk1/3p2pb1/7N1/7Pp/9/9/9/9 b G2S2N2r2g2s3l14p 1",     //13
    "7nl/3R2sk1/9/6ppp/5N3/9/9/9/9 b BGPrb3g3s2n3l14p 1",        //14
    "5g1nl/7sk/5p3/5lG2/6B2/9/9/9/9 b G2N2rbg3sn2l17p 1",        //15
    "7nl/5PRg1/7k1/6s2/7P1/8N/9/9/9 b RB2Gbg3s2n3l16p 1",        //16
    "5g1nl/5sk2/5s3/7G1/7B1/9/9/9/9 b RSNrb2gs2n3l18p 1",        //17
    "6snl/9/6p1k/5L+R2/6+p2/9/9/9/9 b BNLrb4g3s2nl16p 1",        //18
    "5+P1nl/6k2/4R2gp/4Np3/9/6P2/9/9/9 b 2SNr2b3g2sn3l14p 1",    //19
    "5+P1nl/5G1k1/4R2pp/4N1r2/9/6N2/9/9/9 b 2S2b3g2sn3l15p 1",   //20
    "7nl/3s1+Bsk1/9/6p1P/7pp/6N2/9/9/9 b 2Rb4g2s2n3l14p 1",      //21
    "5g1nl/7k1/7p1/4Ps+B2/8p/9/9/9/9 b RG2Nrb2g3sn3l15p 1",      //22
    "4n2nl/5+R3/4Pp1k1/3p5/5P3/9/9/9/9 b R2GN2b2g4sn3l14p 1",    //23
    "6gnl/6S2/5+P1Sk/6l1p/7pb/9/9/9/9 b BGS2r2gs3n3l15p 1",      //24
    "5r1nl/6R2/5+BP1k/7gg/9/6p2/9/9/9 b 4Sb2g3n3l16p 1",         //25
    "8k9/9/5+b2r/6R1N/9/9/9/9 b 2Gb2g4s3n4l18p 1",               //26
    "6S1+P/4P1k1B/6+b1p/5L3/5r3/9/9/9/9 b GLr3g3s4n2l16p 1",     //27
    "5pr1l/5gkP1/4+R2pp/9/6S2/9/9/9/9 b G3N2b2g3sn3l15p 1",      //28
    "9/9/9/9/6+p2/7+p1/9/5Rg2/6k2 b R2G2bg4s4n4l16p 1",          //29
    "4gs2l/5k1rp/8s/4B4/3+b5/9/9/9/9 b RGS2gs4n3l17p 1",         //30
    "6kn1/5P2+B/9/4Lp1b1/4N4/9/6r2/6r2/9 b GP3g4s2n3l15p 1",     //31
    "7n1/4+BR2l/6+bk1/3p4p/6PP1/8P/6N2/7S1/9 b Lr4g3s2n2l13p 1", //32
    "5s2l/6pk1/8P/7G1/7N1/6R2/9/9/9 b R2b3g3s3n3l16p 1",         //33
    "7+R1/8L/5+BNp1/8k/5S2p/9/9/9/9 b SPrb4g2s3n3l15p 1",        //34
    "6pkl/9/7s1/5P+B1p/9/9/9/9/9 b 2G2N2rb2g3s2n3l15p 1",        //35
    "4n1g2/7kB/4+B1n2/8p/9/9/9/9/9 b GS2P2r2g3s2n4l15p 1",       //36
    "5+B2l/6G1k/6g2/7p1/6P1+b/7P1/9/9/9 b 2G2r4s4n3l15p 1",      //37
    "8l/6gk1/4g3n5gBpG/9/9/9/9/9 b B2S2r2s3n3l17p 1",            //38
    "8l/4S1k2/5R3/5s1gp/6p2/9/9/9/9 b GS2Pr2b2gs4n3l14p 1",      //39
    "6kl1/5gb1+R/4L4/6p2/9/9/9/9/9 b 2G2NLrbg4s2nl17p 1",        //40
    "8l/7k1/5Sn1p/7N1/6s1P/9/9/9/9 b BN2L2rb4g2snl16p 1",        //41
    "4p4/4R1gk1/3b2g2/5p3/7pR/9/9/9/9 b GSNbg3s3n4l15p 1",       //42
    "4pk2l/2R1g3B/7G1/4Pp1p1/9/9/9/9/9 b SPrb2g3s4n3l13p 1",     //43
    "4g3l/4l1k2/5gn2/3B2p1p/9/9/9/9/9 b RBGNrg4s2n2l16p 1",      //44
    "8l/6+P1p/6Rsk/9/5p1+bs/6sP1/9/9/9 b B2Gr2gs4n3l14p 1",      //45
    "3p2k2/3+R1g2P/4+bp1P1/6p2/7Ns/9/9/9/9 b 3Srb3g3n4l13p 1",   //46
    "9/7k1/5p3/6Rsb/9/9/9/9/9 b B2Gr2g3s4n4l17p 1",              //47
    "9/7k1/5bR1p/5bP2/9/9/9/9/9 b NLPr4g4s3n3l15p 1",            //48
    "7n1/6R2/7p1/7k1/9/7Gp/9/9/9 b B2Prb3g4s3n4l14p 1",          //49
    NULL   //terminal
};

//逃れ問題　出典：逃れ将棋　森信雄
char *g_sfen_fudume[] = {
    "9/9/9/4+b4/3P5/9/4P1g1P/4+p3K/8L b 2RB3G4S4N3L14P 1",      //1
    "9/9/2p6/p2P5/2P6/P8/1p7/1K1+b5/LN7 b 2R3G4S3N3L12Pbg 1",   //2
    "9/9/7l1/9/5r37K1/5PP1P/3+p5/7N1 b RB4G4S3N3L14Pb 1",       //3
    "9/9/9/8p/6p2/6r1P/4P1K2/9/4+pPP1+p b RB4G4S4N4L10Pb 1",    //4
    "9/9/7l1/9/4g4/6K2/4P4/5P2+b/5s3 b 2RB3G3S4N3L15Pp 1",      //5
    "9/9/3P+b4/2L6/9/8P/6g1K/9/8L b 2R3G4S4N2L16Pb 1",          //6
    "9/9/9/9/6s1p/9/5P2P4P2KL3r3g1 b R2B3G3S4N3L13Pp 1",        //7
    "9/9/9/4+b4/9/6s27KP/7P1/2+p3PN1 b RB4G3S3N4L13Prp 1",      //8
    "9/9/9/9/3BR1n2/4b4/3+r2PPP/6K2/7N1 b 4G3S2N4L15Ps 1",      //9
    "9/5r3/9/9/6r2/4+B3P/5PP1b/7K1/9 b 3G4S4N4L15Pg 1",         //10
    "9/9/9/9/8+B/9/4+b2sP/6sK1/9 b 2R4G2S4N4L16Pp 1",           //11
    "9/9/8n/9/9/6P2/4sPpP1/6K2/4P2N1 b 2RB3G3SN4L11Pbgn2p 1",   //12
    "9/9/3p5/1p7/9/P1P6/1P1S+r4/Kg7/LN7 b RB2G3S3N3L13Pbg 1",   //13
    "9/6r1+P/5+B1+P1/9/4R2S1/6p2/6+bK1/9/6S2 b 3G2S4N4L15Pg 1", //14
    "9/9/1sn6/1R7/9/np7/3+b5/LK7/1NP6 b RB3G3SN3L15Pgp 1",      //15
    "9/9/7+P1/6p2/7l1/6s2/4+b4/7K1/4P1BNL b 2R3G3S2N2L15P 1",   //16
    "9/9/9/3g5/9/3p5/P1PKP+b3/5S3/LN7 b 2RBG2S3N3L14P2gs 1",    //17
    NULL
};

//出典: 2021-04-22 叡王戦　羽生善治九段 vs 森内俊之九段
//実戦テスト
char *g_sfen_ftest[] = {
    "lnsgkgsnl/1r5b1/ppppppppp/9/9/9/PPPPPPPPP/1B5R1/"
    "LNSGKGSNL b - 1",   //平手
    "lnsgkgsnl/1r5b1/ppppppppp/9/9/2P6/PP1PPPPPP/1B5R1/"
    "LNSGKGSNL w - 1",   //1
    "lnsgkgsnl/1r5b1/p1ppppppp/1p7/9/2P6/PP1PPPPPP/1B5R1/"
    "LNSGKGSNL b - 1",   //2
    "lnsgkgsnl/1r5b1/p1ppppppp/1p7/9/2P6/PP1PPPPPP/1B1S3R1/"
    "LN1GKGSNL w - 1",   //3
    "lnsgkgsnl/1r5b1/p1pppp1pp/1p4p2/9/2P6/PP1PPPPPP/1B1S3R1/"
    "LN1GKGSNL b - 1",   //4
    "lnsgkgsnl/1r5b1/p1pppp1pp/1p4p2/9/2P6/PPSPPPPPP/1B5R1/"
    "LN1GKGSNL w - 1",   //5
    "ln1gkgsnl/1r1s3b1/p1pppp1pp/1p4p2/9/2P6/PPSPPPPPP/1B5R1/"
    "LN1GKGSNL b - 1",   //6
    "ln1gkgsnl/1r1s3b1/p1pppp1pp/1p4p2/9/2P4P1/PPSPPPP1P/1B5R1/"
    "LN1GKGSNL w - 1",   //7
    "ln1gkg1nl/1r1s1s1b1/p1pppp1pp/1p4p2/9/2P4P1/PPSPPPP1P/1B5R1/"
    "LN1GKGSNL b - 1",   //8
    "ln1gkg1nl/1r1s1s1b1/p1pppp1pp/1p4p2/9/2P4P1/PPSPPPP1P/1B3S1R1/"
    "LN1GKG1NL w - 1",   //9
    "ln1gk2nl/1r1s1sgb1/p1pppp1pp/1p4p2/9/2P4P1/PPSPPPP1P/1B3S1R1/"
    "LN1GKG1NL b - 1",   //10
    "ln1gk2nl/1r1s1sgb1/p1pppp1pp/1p4p2/9/2P4P1/PPSPPPP1P/1BG2S1R1/"
    "LN2KG1NL w - 1",    //11
    "ln1g1k1nl/1r1s1sgb1/p1pppp1pp/1p4p2/9/2P4P1/PPSPPPP1P/1BG2S1R1/"
    "LN2KG1NL b - 1",    //12
    "ln1g1k1nl/1r1s1sgb1/p1pppp1pp/1p4p2/9/2P4P1/PPSPPPP1P/1BG2S1R1/"
    "LN1K1G1NL w - 1",   //13
    "ln1g1k1nl/1r1s1sgb1/p2ppp1pp/1pp3p2/9/2P4P1/PPSPPPP1P/1BG2S1R1/"
    "LN1K1G1NL b - 1",   //14
    "ln1g1k1nl/1r1s1sgb1/p2ppp1pp/1pp3p2/9/2P1P2P1/PPSP1PP1P/1BG2S1R1/"
    "LN1K1G1NL w - 1",   //15
    "ln1g1k1nl/1r1s1sgb1/p2ppp1pp/2p3p2/1p7/2P1P2P1/PPSP1PP1P/1BG2S1R1/"
    "LN1K1G1NL b - 1",   //16
    "ln1g1k1nl/1r1s1sgb1/p2ppp1pp/2p3p2/1p7/2P1P2P1/PPSP1PP1P/1BG1GS1R1/"
    "LN1K3NL w - 1",     //17
    "ln1g1k1nl/1r3sgb1/p1sppp1pp/2p3p2/1p7/2P1P2P1/PPSP1PP1P/1BG1GS1R1/"
    "LN1K3NL b - 1",     //18
    "ln1g1k1nl/1r3sgb1/p1sppp1pp/2p3p2/1p7/2PPP2P1/PPS2PP1P/1BG1GS1R1/"
    "LN1K3NL w - 1",     //19
    "ln1g1k1nl/1r3sgb1/p2ppp1pp/2ps2p2/1p7/2PPP2P1/PPS2PP1P/1BG1GS1R1/"
    "LN1K3NL b - 1",     //20
    "ln1g1k1nl/1r3sgb1/p2ppp1pp/2ps2p2/1p7/2PPP2P1/PPSG1PP1P/1BG2S1R1/"
    "LN1K3NL w - 1",     //21
    "ln1g1k1nl/1r3sgb1/p2ppp1pp/3s2p2/1pp6/2PPP2P1/PPSG1PP1P/1BG2S1R1/"
    "LN1K3NL b - 1",     //22
    "ln1g1k1nl/1r3sgb1/p2ppp1pp/3s2p2/1pP6/3PP2P1/PPSG1PP1P/1BG2S1R1/"
    "LN1K3NL w P 1",     //23
    "ln1g1k1nl/1r3sgb1/p2ppp1pp/6p2/1ps6/3PP2P1/PPSG1PP1P/1BG2S1R1/"
    "LN1K3NL b Pp 1",    //24
    "ln1g1k1nl/1r3sgb1/p2ppp1pp/6p2/1ps4P1/3PP4/PPSG1PP1P/1BG2S1R1/"
    "LN1K3NL w Pp 1",    //25
    "ln1g1k1nl/1r3sgb1/p2ppp1pp/6p2/2s4P1/1p1PP4/PPSG1PP1P/1BG2S1R1/"
    "LN1K3NL b Pp 1",    //26
    "ln1g1k1nl/1r3sgb1/p2ppp1pp/6p2/2s4P1/1P1PP4/P1SG1PP1P/1BG2S1R1/"
    "LN1K3NL w 2Pp 1",   //27
    "ln1g1k1nl/1r3sgb1/p2ppp1pp/6p2/7P1/1s1PP4/P1SG1PP1P/1BG2S1R1/"
    "LN1K3NL b 2P2p 1",  //28
    "ln1g1k1nl/1r3sgb1/p2ppp1pp/6p2/7P1/1S1PP4/P2G1PP1P/1BG2S1R1/"
    "LN1K3NL w S2P2p 1", //29
    "ln1g1k1nl/5sgb1/p2ppp1pp/6p2/7P1/1r1PP4/P2G1PP1P/1BG2S1R1/"
    "LN1K3NL b S2Ps2p 1",//30
    "ln1g1k1nl/5sgb1/p2ppp1pp/6p2/7P1/1r1PP4/PP1G1PP1P/1BG2S1R1/"
    "LN1K3NL w SPs2p 1", //31
    "ln1g1k1nl/5sgb1/p2ppp1pp/1r4p2/7P1/3PP4/PP1G1PP1P/1BG2S1R1/"
    "LN1K3NL b SPs2p 1", //32
    "ln1g1k1nl/5sgb1/p2ppp1pp/1r4p2/7P1/3PP4/PP1GSPP1P/1BG4R1/"
    "LN1K3NL w SPs2p 1", //33
    "ln1g1k1nl/5sgb1/p2ppp1p1/1r4p1p/7P1/3PP4/PP1GSPP1P/1BG4R1/"
    "LN1K3NL b SPs2p 1", //34
    "ln1g1k1nl/5sgb1/p2ppp1p1/1r4p1p/7P1/3PP3P/PP1GSPP2/1BG4R1/"
    "LN1K3NL w SPs2p 1", //35
    "ln1g1k1nl/5sgb1/3ppp1p1/pr4p1p/7P1/3PP3P/PP1GSPP2/1BG4R1/"
    "LN1K3NL b SPs2p 1", //36
    "ln1g1k1nl/5sgb1/3ppp1p1/pr4p1p/7P1/3PP3P/PP1GSPP2/1BG4R1/"
    "LNK4NL w SPs2p 1",  //37
    "ln1g1k1nl/5sgb1/3ppp1p1/1r4p1p/p6P1/3PP3P/PP1GSPP2/1BG4R1/"
    "LNK4NL b SPs2p 1",  //38
    "ln1g1k1nl/5sgb1/3ppp1p1/1r4p1p/p2P3P1/4P3P/PP1GSPP2/1BG4R1/"
    "LNK4NL w SPs2p 1",  //39
    "ln1g1k1nl/5sg2/3ppp1p1/1r4p1p/p2P3P1/4P3P/PP1GSPP2/1+bG4R1/"
    "LNK4NL b SPbs2p 1", //40
    "ln1g1k1nl/5sg2/3ppp1p1/1r4p1p/p2P3P1/4P3P/PP1GSPP2/1KG4R1/"
    "LN5NL w BSPbs2p 1", //41
    "l2g1k1nl/5sg2/2nppp1p1/1r4p1p/p2P3P1/4P3P/PP1GSPP2/1KG4R1/"
    "LN5NL b BSPbs2p 1", //42
    "l2g1k1nl/5sg2/2nppp1p1/1r4p1p/p2P3P1/3BP3P/PP1GSPP2/1KG4R1/"
    "LN5NL w SPbs2p 1",  //43
    "l2g1k1nl/5sg2/2npppbp1/1r4p1p/p2P3P1/3BP3P/PP1GSPP2/1KG4R1/"
    "LN5NL b SPs2p 1",   //44
    "l2g1k1nl/5sg2/2npppbp1/1r4p1p/p2PP2P1/3B4P/PP1GSPP2/1KG4R1/"
    "LN5NL w SPs2p 1",   //45
    "l2g1k1nl/5sg2/2npppbp1/1r4p1p/p1pPP2P1/3B4P/PP1GSPP2/1KG4R1/"
    "LN5NL b SPsp 1",    //46
    "l2g1k1nl/5sg2/2npppbp1/1r4p1p/p1pPP2P1/1S1B4P/PP1GSPP2/1KG4R1/"
    "LN5NL w Psp 1",     //47
    "l2g1k1nl/5sg2/3pppbp1/1r4p1p/p1pnP2P1/1S1B4P/PP1GSPP2/1KG4R1/"
    "LN5NL b Ps2p 1",    //48
    "l2g1k1nl/5sg2/3pppbp1/1r4p1p/p1pnP2P1/1S1BS3P/PP1G1PP2/1KG4R1/"
    "LN5NL w Ps2p 1",    //49
    "l2g1k1nl/5sg2/3pppbp1/1r1s2p1p/p1pnP2P1/1S1BS3P/PP1G1PP2/1KG4R1/"
    "LN5NL b P2p 1",     //50
    "l2g1k1nl/5sg2/3pppbp1/1r1s2p1p/p1pnP2P1/1S1BS1P1P/PP1G1P3/1KG4R1/"
    "LN5NL w P2p 1",     //51
    "l2g1k1nl/5sg2/3p1pbp1/1r1sp1p1p/p1pnP2P1/1S1BS1P1P/PP1G1P3/1KG4R1/"
    "LN5NL b P2p 1",     //52
    "l2g1k1nl/5sg2/3p1pbp1/1r1sp1pPp/p1pnP4/1S1BS1P1P/PP1G1P3/1KG4R1/"
    "LN5NL w P2p 1",     //53
    "l2g1k1nl/5sg2/3p1pb2/1r1sp1ppp/p1pnP4/1S1BS1P1P/PP1G1P3/1KG4R1/"
    "LN5NL b P3p 1",     //54
    "l2g1k1nl/5sg2/3p1pb2/1r1sp1ppp/p1pnP1P2/1S1BS3P/PP1G1P3/1KG4R1/"
    "LN5NL w P3p 1",     //55
    "l2g1k1nl/5sg2/3p1pb2/1r1s2ppp/p1pnp1P2/1S1BS3P/PP1G1P3/1KG4R1/"
    "LN5NL b P4p 1",     //56
    "l2g1k1nl/5sg2/3p1pb2/1r1s2Ppp/p1pnp4/1S1BS3P/PP1G1P3/1KG4R1/"
    "LN5NL w 2P4p 1",    //57
    "l2g1k1nl/5sgb1/3p1p3/1r1s2Ppp/p1pnp4/1S1BS3P/PP1G1P3/1KG4R1/"
    "LN5NL b 2P4p 1",    //58
    "l2g1k1nl/5sgb1/3p1p3/1r1s2Ppp/p1pSp4/1S1B4P/PP1G1P3/1KG4R1/"
    "LN5NL w N2P4p 1",   //59
    "l2g1k1nl/5sgb1/3p1p3/1r4Ppp/p1psp4/1S1B4P/PP1G1P3/1KG4R1/"
    "LN5NL b N2Ps4p 1",  //60
    "l2g1k1nl/5sgb1/3p1p3/1r4Ppp/p1Bsp4/1S6P/PP1G1P3/1KG4R1/"
    "LN5NL w N3Ps4p 1",  //61
    "l2g1k1nl/5sgb1/3p1p3/6rpp/p1Bsp4/1S6P/PP1G1P3/1KG4R1/"
    "LN5NL b N3Ps5p 1",  //62
    "l2g1k1nl/5sgb1/3p1p3/6rpp/p1Bsp1p2/1S6P/PP1G1P3/1KG4R1/"
    "LN5NL w N2Ps5p 1",
    "l2g1k1nl/5sgb1/3p1p3/6rpp/p1Bs2P2/1S2p3P/PP1G1P3/1KG4R1/"
    "LN5NL b N2Ps5p 1",
    "l2g1k1nl/5sgb1/3p1p3/6rpp/p1Bs2P2/1S2p3P/PPPG1P3/1KG4R1/"
    "LN5NL w NPs5p 1",
    "l2g1k1nl/5sgb1/3p1p3/7pp/p1Bs2r2/1S2p3P/PPPG1P3/1KG4R1/"
    "LN5NL b NPs6p 1",
    "l2g1k1nl/5sgb1/3p1p3/7Rp/p1Bs2r2/1S2p3P/PPPG1P3/1KG6/"
    "LN5NL w N2Ps6p 1",
    "l2g1k1nl/5sgb1/3p1p1p1/7Rp/p1Bs2r2/1S2p3P/PPPG1P3/1KG6/"
    "LN5NL b N2Ps5p 1",
    "l2g1k1nl/5sgb1/3p1p1p1/1R6p/p1Bs2r2/1S2p3P/PPPG1P3/1KG6/"
    "LN5NL w N2Ps5p 1",
    "l2g1k1nl/5sg2/3p1p1pb/1R6p/p1Bs2r2/1S2p3P/PPPG1P3/1KG6/"
    "LN5NL b N2Ps5p 1",
    "l+R1g1k1nl/5sg2/3p1p1pb/8p/p1Bs2r2/1S2p3P/PPPG1P3/1KG6/"
    "LN5NL w N2Ps5p 1",
    "l+Rpg1k1nl/5sg2/3p1p1pb/8p/p1Bs2r2/1S2p3P/PPPG1P3/1KG6/"
    "LN5NL b N2Ps4p 1",
    "l+Rpg1k1nl/5sg2/3p1p1pb/8p/p1Bs2r2/1S2p3P/PPPG1PN2/1KG6/"
    "LN5NL w 2Ps4p 1",
    "l+Rpg1k1nl/5sg2/3p1p1pb/8p/p1Bsr4/1S2p3P/PPPG1PN2/1KG6/"
    "LN5NL b 2Ps4p 1",
    "l+Rpg1k1nl/5sg2/3p1p1pb/8p/p1Bsr4/1S2p3P/PPPG1PN2/1KG1P4/"
    "LN5NL w Ps4p 1",
    "l+Rpg1k1nl/5sg2/3p1p1pb/8p/p1Bsr4/1S2p1p1P/PPPG1PN2/1KG1P4/"
    "LN5NL b Ps3p 1",
    "l+Rpg1k1nl/3P1sg2/3p1p1pb/8p/p1Bsr4/1S2p1p1P/PPPG1PN2/1KG1P4/"
    "LN5NL w s3p 1",
    "l+Rp2k1nl/3g1sg2/3p1p1pb/8p/p1Bsr4/1S2p1p1P/PPPG1PN2/1KG1P4/"
    "LN5NL b s4p 1",
    "l1+R2k1nl/3g1sg2/3p1p1pb/8p/p1Bsr4/1S2p1p1P/PPPG1PN2/1KG1P4/"
    "LN5NL w Ps4p 1",
    "l1+R1sk1nl/3g1sg2/3p1p1pb/8p/p1Bsr4/1S2p1p1P/PPPG1PN2/1KG1P4/"
    "LN5NL b P4p 1",
    "+R3sk1nl/3g1sg2/3p1p1pb/8p/p1Bsr4/1S2p1p1P/PPPG1PN2/1KG1P4/"
    "LN5NL w LP4p 1",
    "+R3sk1nl/3g1sg2/3p1p1pb/8p/p1Bsr4/1S2p3P/PPPG1P+p2/1KG1P4/"
    "LN5NL b LPn4p 1",
    "+R3sk1nl/3g1sg2/3p1p1pb/8p/p1Bsr4/1S2p3P/PPPG1PN2/1KG1P4/"
    "LN6L w L2Pn4p 1",
    "+R3sk1nl/3g1sg2/3p1p1pb/8p/p1Bsr4/1S2p1p1P/PPPG1PN2/1KG1P4/"
    "LN6L b L2Pn3p 1",
    "+R3sk1nl/3g1sg2/3p1pPpb/8p/p1Bsr4/1S2p1p1P/PPPG1PN2/1KG1P4/"
    "LN6L w LPn3p 1",
    "+R3sk2l/3g1sg2/3p1pnpb/8p/p1Bsr4/1S2p1p1P/PPPG1PN2/1KG1P4/"
    "LN6L b LPn4p 1",
    "+R3sk2l/3g1sg2/3p1pnpb/8p/p1Bsr3P/1S2p1p2/PPPG1PN2/1KG1P4/"
    "LN6L w LPn4p 1",
    "+R3sk2l/3g1sg2/3p1pnpb/8p/p1Bsr3P/1S2p4/PPPG1P+p2/1KG1P4/"
    "LN6L b LP2n4p 1",
    "+R3sk2l/3g1sg2/3p1pnpb/8P/p1Bsr4/1S2p4/PPPG1P+p2/1KG1P4/"
    "LN6L w L2P2n4p 1",
    "+R3sk2l/3g1sg2/3p1pnp1/8P/p1Bsr1b2/1S2p4/PPPG1P+p2/1KG1P4/"
    "LN6L b L2P2n4p 1",
    "+R3sk2l/3g1sg2/3p1pnp1/6P1P/p1Bsr1b2/1S2p4/PPPG1P+p2/1KG1P4/"
    "LN6L w LP2n4p 1",
    "+R3sk2l/3g1sg2/3p1pnp1/2n3P1P/p1Bsr1b2/1S2p4/PPPG1P+p2/1KG1P4/"
    "LN6L b LPn4p 1",
    "+R3sk2l/3g1sg2/3p1p+Pp1/2n5P/p1Bsr1b2/1S2p4/PPPG1P+p2/1KG1P4/"
    "LN6L w NLPn4p 1",
    "+R3sk2l/3g1s3/3p1pgp1/2n5P/p1Bsr1b2/1S2p4/PPPG1P+p2/1KG1P4/"
    "LN6L b NLPn5p 1",
    "+R3sk2l/3g1s3/3p1pgp1/2n5P/p1Bsr1b2/1S2pN3/PPPG1P+p2/1KG1P4/"
    "LN6L w LPn5p 1",
    "+R3sk2l/3g1s3/3p1pgp1/2n5P/p1Bsr1b2/1S1npN3/PPPG1P+p2/1KG1P4/"
    "LN6L b LP5p 1",
    "+R3sk2l/3g1s3/3p1pgp1/2n5P/p1Bsr1b2/1S1npN3/PPPG1P+p2/1K1GP4/"
    "LN6L w LP5p 1",
    "+R3sk2l/3g1s3/3p1pgp1/2n5P/p1Bsr1b2/1S1npN3/PPPG1+p3/1K1GP4/"
    "LN6L b LP6p 1",
    "+R3sk2l/3g1s3/3p1pgp1/2n3P1P/p1Bsr1b2/1S1npN3/PPPG1+p3/1K1GP4/"
    "LN6L w L6p 1",
    "+R3sk2l/3g1sg2/3p1p1p1/2n3P1P/p1Bsr1b2/1S1npN3/PPPG1+p3/1K1GP4/"
    "LN6L b L6p 1",
    "+R3sk2l/3g1sg2/3p1p1p1/2n3P1P/p1Bsr1b2/1S1GpN3/PPP2+p3/1K1GP4/"
    "LN6L w NL6p 1",
    "+R3sk2l/3g1sg2/3p1p1p1/6P1P/p1Bsr1b2/1S1npN3/PPP2+p3/1K1GP4/"
    "LN6L b NLg6p 1",
    "+R3sk2l/3g1sg2/3p1pLp1/6P1P/p1Bsr1b2/1S1npN3/PPP2+p3/1K1GP4/"
    "LN6L w Ng6p 1",
    "+R3sk2l/3g1s1g1/3p1pLp1/6P1P/p1Bsr1b2/1S1npN3/PPP2+p3/1K1GP4/"
    "LN6L b Ng6p 1",
    "+R3sk2l/3g1s1g1/3p1pLp+P/6P2/p1Bsr1b2/1S1npN3/PPP2+p3/1K1GP4/"
    "LN6L w Ng6p 1",
    "+R3sk3/3g1s1g1/3p1pLpl/6P2/p1Bsr1b2/1S1npN3/PPP2+p3/1K1GP4/"
    "LN6L b Ng7p 1",
    "+R3sk3/3g1s1g1/3p1pLpl/6P2/p1Bsr1b2/1S1npN3/PPPN1+p3/1K1GP4/"
    "LN6L w g7p 1",
    "+R3sk3/3grs1g1/3p1pLpl/6P2/p1Bs2b2/1S1npN3/PPPN1+p3/1K1GP4/"
    "LN6L b g7p 1",
    "+R3sk3/3grs1g1/3p1pLp+L/6P2/p1Bs2b2/1S1npN3/PPPN1+p3/1K1GP4/"
    "LN7 w Lg7p 1",
    "+R3sk3/3grs1g1/3p1pLpb/6P2/p1Bs5/1S1npN3/PPPN1+p3/1K1GP4/"
    "LN7 b Lgl7p 1",
    "+R3sk3/3grs1g1/3p1pLpb/6P2/p1BsL4/1S1npN3/PPPN1+p3/1K1GP4/"
    "LN7 w gl7p 1",
    "+R3sk3/3grs1g1/3p1pLp1/6P2/p1BsL4/1S1npb3/PPPN1+p3/1K1GP4/"
    "LN7 b gnl7p 1",
    "+R3sk3/3g+Ls1g1/3p1pLp1/6P2/p1Bs5/1S1npb3/PPPN1+p3/1K1GP4/"
    "LN7 w Rgnl7p 1",
    "+R3s4/3gks1g1/3p1pLp1/6P2/p1Bs5/1S1npb3/PPPN1+p3/1K1GP4/"
    "LN7 b Rgn2l7p 1",
    NULL
};
