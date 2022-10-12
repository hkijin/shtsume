@echo off
rem   文字コードをutf-8に変更しておく。
chcp 65001

rem ########################################################################
rem 　---Windows コマンドライン環境でshtsumeを使用する場合のサンプル集---
rem [使用方法]
rem 1,このファイルをshtsumeがインストールしてあるディレクトリにコピーします。
rem 2,ターミナルを立ち上げ、このファイルがあるディレクトリに移動します。
rem 3,エディタでこのファイルを以下のように変更します。
rem   下記のサンプルコマンドから試したいコマンドをアクティブにします。（先頭の"rem"を消す）
rem 4,変更を保存した後、ターミナルから"sample"と入力し、リターンします。
rem 5,詰将棋解図結果が出力されます。
rem
rem ########################################################################

rem ### 以下はサンプルコマンドです。###
rem   古典詰将棋を解き、その後検討モードに移行します。
shtsume -d "3sks3/9/4S4/9/9/B8/9/9/9 b S2rb4g4n4l18p 1"

rem   メモリ128Mbyteで将棋図巧５番を解く設定です。  
rem shtsume -m 128 "n+B1sS4/1R1g5/1Ls6/2k6/2n6/3L5/R8/9/9 b B2P3gs2n2l16p 1" 

rem   メモリ256Mbyteで将棋図巧９９番（煙詰め）を解く設定です。
rem shtsume -m 256 "k1+P4n1/2L+P2sL1/r4+P+P1P/+BpP+Pl1+Rg1/NP1S+PP+p1g/2L+p1g+P1+P/Ps1G1+P1N1/1sN1P4/B8 b - 1"
