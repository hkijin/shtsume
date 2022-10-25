#  shtsume
##  Featured
shtsume(**sh**ell **tsume**shogi solver)は以下の特徴を持つ詰将棋解図プログラムです。  
・コマンドライン（shell)上で詰将棋解図ができます。  
・USIプロトコルに準拠しており、詰将棋エンジンとして使用する事ができます。   
・探索レベルの設定により、特定局面の詰み/不詰判定から詰将棋作品の作意推定まで可能です。  
・探索LOGにより詰め手順中の玉方変化時の詰手数、駒余りの有無を確認することができます。

詰将棋作品の鑑賞や新作の検証に利用できるToolになれば幸いです。   
インストールおよび使用法はリリースノートを参照してください。  

##  Requirement  
対応OS  
・macOS (INTEL, M1)  

##  Goal
shtsumeの開発は以下を目標としています。  
・高効率　　少ない探索量で解を導きます。    
・省メモリ　難解な作品であっても少ないメモリで解を導きます。  
・高精度　　完全作であれば作意解（正解）を出力する。  
・高速　　　プログラムの高速化を図っていく。 

##  Achievement 
v0.2.0-beta  
メモリ６GBで超難解作を含む多くの作品で作意解導出を確認。



