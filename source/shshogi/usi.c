//
//  usi.c
//  shshogi
//
//  Created by Hkijin on 2021/09/21.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include "shogi.h"
#include "usi.h"

/* ------------
 グローバル変数
 ------------ */
unsigned int g_btime, g_wtime;        // 残り時間（btime:先手、wtime:後手）(mS)
unsigned int g_byoyomi;               // 秒読み時間(mS)
unsigned int g_binc, g_winc;          // １手ごとの加算時間を表示します。(mS)
int g_time_mode;                      // 持ち時間方式
int g_time_limit;                     // １手あたりの目標持ち時間
char g_logfile_name[SZ_FILEPATH];     // USI起動時に生成されるログファイル
char g_errorlog_name[SZ_FILEPATH];    // エラー発生時に生成されるログファイル
sdata_t g_sdata;                      // USIデータ受け渡し用

bool g_stop_received = false;         // stopコマンド受信の有無
clock_t g_ponderhit_time;             // ponderhitの時刻
char g_str[SZ_USIBUFFER];             // USIコマンドバッファ

char g_logfile_path[256];

#if DEBUG
bool g_log_enable = false;            //通信LOGを出力したい場合はtrueに変更する。
#else
bool g_log_enable = false;
#endif /* DEBUG */

/* -----------
 スタティック変数
 ----------- */
static int  st_sfen_pos[N_SQUARE] = {
    B91,B81,B71,B61,B51,B41,B31,B21,B11,
    B92,B82,B72,B62,B52,B42,B32,B22,B12,
    B93,B83,B73,B63,B53,B43,B33,B23,B13,
    B94,B84,B74,B64,B54,B44,B34,B24,B14,
    B95,B85,B75,B65,B55,B45,B35,B25,B15,
    B96,B86,B76,B66,B56,B46,B36,B26,B16,
    B97,B87,B77,B67,B57,B47,B37,B27,B17,
    B98,B88,B78,B68,B58,B48,B38,B28,B18,
    B99,B89,B79,B69,B59,B49,B39,B29,B19
};
static char *st_usi_pos[N_SQUARE] = {
    "1a","2a","3a","4a","5a","6a","7a","8a","9a",
    "1b","2b","3b","4b","5b","6b","7b","8b","9b",
    "1c","2c","3c","4c","5c","6c","7c","8c","9c",
    "1d","2d","3d","4d","5d","6d","7d","8d","9d",
    "1e","2e","3e","4e","5e","6e","7e","8e","9e",
    "1f","2f","3f","4f","5f","6f","7f","8f","9f",
    "1g","2g","3g","4g","5g","6g","7g","8g","9g",
    "1h","2h","3h","4h","5h","6h","7h","8h","9h",
    "1i","2i","3i","4i","5i","6i","7i","8i","9i"
};
static char *st_usi_koma[8]       = {
    "*","P","L","N","S","G","B","R"
};
static pthread_mutex_t  st_lock;
static strqueue_t  *st_que = NULL;
static struct timespec st_rqpt = { 0, 1000000};

/* -----------
 スタティック関数
 ----------- */
static void *receiving_thread(void *arg);
static void retrieve_message(char *buf);

/* -----------
 実装部
 ----------- */
strqueue_t *strqueue_push(strqueue_t *queue,
                          const char *str_in){
    strqueue_t *que;
    unsigned long len = strlen(str_in);
    
    //キューの確保
    que = (strqueue_t *)calloc(1, sizeof(strqueue_t));
    if(!que){
        perror("memory error");
        exit(EXIT_FAILURE);
    }
    que->str = (char *)calloc(1, sizeof(char)*(len+1));
    if(!que->str){
        perror("memory error");
        exit(EXIT_FAILURE);
    }
    memcpy(que->str, str_in, sizeof(char)*len);
    que->next = queue;
    return que;
}
strqueue_t *strqueue_pop (strqueue_t *queue,
                          char *str_out)     {
    if(!queue) return NULL;
    if(!queue->next){
        unsigned long len = strlen(queue->str);
        strncpy(str_out, queue->str, len+1);
        free(queue->str);
        free(queue);
        return NULL;
    }
    strqueue_t *que = queue, *prev = NULL;
    while(que->next) {
        prev = que;
        que = que->next;
    }
    unsigned long len = strlen(que->str);
    strncpy(str_out, que->str, len+1);
    free(que->str);
    free(que);
    if(prev) prev->next = NULL;
    return queue;
}


void create_log_filename(void)      {
    if(!g_log_enable) return;
    char s[16];
    time_t now;
    time(&now);
    struct tm t = *localtime(&now);
    strftime(s, 16, "%Y%m%d%H%M%S", &t);
    sprintf(g_logfile_name,"%s/usilog%s.txt",g_logfile_path,s);
    FILE *fp = fopen(g_logfile_name, "w");
    if(!fp){
        perror("logfile could not be opened.");
        exit (EXIT_FAILURE);
    }
    fprintf(fp, "usi engine start at %s\n", s);
    fclose(fp);
    return;
}

void record_log(const char *message){
    if(!g_log_enable) return;
    FILE *fp;
    fp = fopen(g_logfile_name, "a");
    assert(fp != NULL);
    fprintf(fp,"%s\n", message);
    fclose(fp);
    return;
}

/*
 -----------------------------------------------------------------------------
 sfen形式の盤面情報読み込み
 [引数]
 str: "sfen "の次にくる盤面情報のポインタ
 ssdata: 盤面情報を格納するssdata_t構造体
 [戻り値]
 sfen形式の盤面情報+空白の次に来る文字列ポインタ
 ----------------------------------------------------------------------------
 */
char *sfen_to_ssdata(char *str, ssdata_t *ssdata){
    char *ch = str;
    memset(ssdata, 0, sizeof(ssdata_t));
    int pos = 0;
    //盤面情報
    while(pos<N_SQUARE){
        switch(*ch){
            case 'K': ssdata->board[st_sfen_pos[pos]] = SOU; pos++; ch++; break;
            case 'R': ssdata->board[st_sfen_pos[pos]] = SHI; pos++; ch++; break;
            case 'B': ssdata->board[st_sfen_pos[pos]] = SKA; pos++; ch++; break;
            case 'G': ssdata->board[st_sfen_pos[pos]] = SKI; pos++; ch++; break;
            case 'S': ssdata->board[st_sfen_pos[pos]] = SGI; pos++; ch++; break;
            case 'N': ssdata->board[st_sfen_pos[pos]] = SKE; pos++; ch++; break;
            case 'L': ssdata->board[st_sfen_pos[pos]] = SKY; pos++; ch++; break;
            case 'P': ssdata->board[st_sfen_pos[pos]] = SFU; pos++; ch++; break;
            case 'k': ssdata->board[st_sfen_pos[pos]] = GOU; pos++; ch++; break;
            case 'r': ssdata->board[st_sfen_pos[pos]] = GHI; pos++; ch++; break;
            case 'b': ssdata->board[st_sfen_pos[pos]] = GKA; pos++; ch++; break;
            case 'g': ssdata->board[st_sfen_pos[pos]] = GKI; pos++; ch++; break;
            case 's': ssdata->board[st_sfen_pos[pos]] = GGI; pos++; ch++; break;
            case 'n': ssdata->board[st_sfen_pos[pos]] = GKE; pos++; ch++; break;
            case 'l': ssdata->board[st_sfen_pos[pos]] = GKY; pos++; ch++; break;
            case 'p': ssdata->board[st_sfen_pos[pos]] = GFU; pos++; ch++; break;
            case '1': pos+=1; ch++; break;
            case '2': pos+=2; ch++; break;
            case '3': pos+=3; ch++; break;
            case '4': pos+=4; ch++; break;
            case '5': pos+=5; ch++; break;
            case '6': pos+=6; ch++; break;
            case '7': pos+=7; ch++; break;
            case '8': pos+=8; ch++; break;
            case '9': pos+=9; ch++; break;
            case '/': ch++; break;
            case '+':
                ch++;
                switch(*ch){
                    case 'R': ssdata->board[st_sfen_pos[pos]] = SRY;
                        pos++; ch++;break;
                    case 'B': ssdata->board[st_sfen_pos[pos]] = SUM;
                        pos++; ch++; break;
                    case 'S': ssdata->board[st_sfen_pos[pos]] = SNG;
                        pos++; ch++; break;
                    case 'N': ssdata->board[st_sfen_pos[pos]] = SNK;
                        pos++; ch++; break;
                    case 'L': ssdata->board[st_sfen_pos[pos]] = SNY;
                        pos++; ch++; break;
                    case 'P': ssdata->board[st_sfen_pos[pos]] = STO;
                        pos++; ch++; break;
                    case 'r': ssdata->board[st_sfen_pos[pos]] = GRY;
                        pos++; ch++; break;
                    case 'b': ssdata->board[st_sfen_pos[pos]] = GUM;
                        pos++; ch++; break;
                    case 's': ssdata->board[st_sfen_pos[pos]] = GNG;
                        pos++; ch++; break;
                    case 'n': ssdata->board[st_sfen_pos[pos]] = GNK;
                        pos++; ch++; break;
                    case 'l': ssdata->board[st_sfen_pos[pos]] = GNY;
                        pos++; ch++; break;
                    case 'p': ssdata->board[st_sfen_pos[pos]] = GTO;
                        pos++; ch++; break;
                    default: break; //error
                }
                break;
            default: break; //error
        }
    }
    ch++;
    //手番情報
    if     (!strncmp(ch, "b ", 2)) ssdata->turn = SENTE;
    else if(!strncmp(ch, "w ", 2)) ssdata->turn = GOTE ;
    ch += 2;
    //持ち駒情報
    int n=1;     //持ち駒の数
    while(*ch!=' '){
        switch(*ch){
            case '-': break;  //持ち駒なし
            case 'R': ssdata->mkey[SENTE].hi = n; n=1; break;
            case 'B': ssdata->mkey[SENTE].ka = n; n=1; break;
            case 'G': ssdata->mkey[SENTE].ki = n; n=1; break;
            case 'S': ssdata->mkey[SENTE].gi = n; n=1; break;
            case 'N': ssdata->mkey[SENTE].ke = n; n=1; break;
            case 'L': ssdata->mkey[SENTE].ky = n; n=1; break;
            case 'P': ssdata->mkey[SENTE].fu = n; n=1; break;
            case 'r': ssdata->mkey[GOTE].hi = n; n=1; break;
            case 'b': ssdata->mkey[GOTE].ka = n; n=1; break;
            case 'g': ssdata->mkey[GOTE].ki = n; n=1; break;
            case 's': ssdata->mkey[GOTE].gi = n; n=1; break;
            case 'n': ssdata->mkey[GOTE].ke = n; n=1; break;
            case 'l': ssdata->mkey[GOTE].ky = n; n=1; break;
            case 'p': ssdata->mkey[GOTE].fu = n; n=1; break;
            case '1': ch++;
                switch(*ch){
                    case '0': n=10; break;
                    case '1': n=11; break;
                    case '2': n=12; break;
                    case '3': n=13; break;
                    case '4': n=14; break;
                    case '5': n=15; break;
                    case '6': n=16; break;
                    case '7': n=17; break;
                    case '8': n=18; break;
                    default: break; //エラー処理
                }
                break;
            case '2': n=2; break;
            case '3': n=3; break;
            case '4': n=4; break;
            case '5': n=5; break;
            case '6': n=6; break;
            case '7': n=7; break;
            case '8': n=8; break;
            case '9': n=9; break;
            default: break; //エラー処理
        }
        ch++;
    }
    ch++;
    
    //手数情報
    if(!strncmp(ch, "1 ", 2)){ ssdata->count = 0; ch+=2;}
    else {} //エラー処理
    
    return ch;
}

/*
 -----------------------------------------------------------------------------
 sfen形式の着手情報読み込み
 [引数]
 move:着手を格納するポインタ
 str:着手文字列の先頭ポインタ
 [戻り値]
 着手文字列+空白の次に来る文字列ポインタ
 -----------------------------------------------------------------------------
 */
char *sfen_to_move   (move_t *move, char *str){
    char *ch = str;
    memset(move, 0, sizeof(move_t));
    int rank=0, file=0;  //盤面 rank:段 file:列
    int mode=0;          //0:盤上 1:持ち駒
    
    //1文字目
    if(*ch>'0' && *ch<='9') file = *ch-'1';
    else switch(*ch){
            case 'R': move->prev_pos = HAND+HI; mode=1; break;
            case 'B': move->prev_pos = HAND+KA; mode=1; break;
            case 'G': move->prev_pos = HAND+KI; mode=1; break;
            case 'S': move->prev_pos = HAND+GI; mode=1; break;
            case 'N': move->prev_pos = HAND+KE; mode=1; break;
            case 'L': move->prev_pos = HAND+KY; mode=1; break;
            case 'P': move->prev_pos = HAND+FU; mode=1; break;
            default: break; //エラー処理
        }
    ch++;
    if(*ch>='a' && *ch<='i') rank = *ch-'a';
    ch++;
    if(!mode) move->prev_pos = rank*9+file;
    //3文字目
    file = *ch-'1';
    ch++;
    //4文字目
    rank = *ch-'a';
    ch++;
    move->new_pos = rank*9+file;
    //5文字目
    if(*ch == ' ') ch++;
    else if(!strncmp(ch, "+ ", 2)){
        move->new_pos |= 0x80;
        ch+=2;
    }
    else {} //エラー処理
    return ch;
}

/*
 -----------------------------------------------------------------------------
 着手情報のsfen形式での書き出し
 [引数]
 str :着手文字列の先頭ポインタ
 move:着手
 [戻り値]
 書き込んだ文字数
 -----------------------------------------------------------------------------
 */
int   move_to_sfen   (char *str, move_t move){
    int num = 0;
    //投了
    if(MV_TORYO(move))
        num = sprintf(str, "resign");
    //盤上着手の場合
    else if(MV_MOVE(move)){
        if(PROMOTE(move))
            num = sprintf(str, "%s%s+", st_usi_pos[PREV_POS(move)],
                          st_usi_pos[NEW_POS(move)]);
        else
            num = sprintf(str, "%s%s", st_usi_pos[PREV_POS(move)],
                          st_usi_pos[NEW_POS(move)]);
    }
    //持ち駒使用の場合
    else{
        num = sprintf(str, "%s*%s", st_usi_koma[MV_HAND(move)],
                      st_usi_pos[NEW_POS(move)]);
    }
    return num;
}

/* ----------
 usi_main
 ----------- */

int usi_main (void){
    //初期化処理
    setvbuf(stdout, NULL, _IONBF, 0);     //将棋所から利用する場合、この設定が必要。
    setvbuf(stdin , NULL, _IONBF, 0);
    char *path = getenv("HOME");
    strncpy(g_logfile_path,path, strlen(path));
    create_log_filename();
    
    pthread_mutex_init(&st_lock, NULL);
    //受信専用スレッド稼働
    pthread_t thread;
    int err = pthread_create(&thread, NULL, receiving_thread, NULL);
    if(err){
        printf("failed to begin thread. %s %d\n",
               __FILE__, __LINE__);
        return -1;
    }
    
    //メインイベントループ
    char buf[SZ_USIBUFFER], msg[SZ_USIBUFFER];
    while(true){
        //対局待ちループ
        while(true){
            //受信コマンドの取得
            retrieve_message(buf);
            
            //受信logへの記録
            sprintf(msg, ">%s", buf);
            record_log(msg);
            
            //メッセージ処理
            if     (!strncmp(buf, "usi ", 4))        res_usi_cmd();
            else if(!strncmp(buf, "isready", 7))     res_isready_cmd();
            else if(!strncmp(buf, "setoption", 9))   res_setoption_cmd(buf);
            else if(!strncmp(buf, "usinewgame", 10)) {
                res_usinewgame_cmd();
                break;
            }
        }
        //対局ループ
        while(true){
            //受信コマンドの取得
            retrieve_message(buf);
            
            //受信logへの記録
            sprintf(msg, ">%s", buf);
            record_log(msg);
            
            //メッセージ処理
            if     (!strncmp(buf, "go mate", 7))    res_gomate_cmd(buf);
            else if(!strncmp(buf, "go ponder", 9))  res_goponder_cmd(buf);
            else if(!strncmp(buf, "go", 2))         res_go_cmd(buf);
            else if(!strncmp(buf, "position", 8))   res_position_cmd(buf);
            else if(!strncmp(buf, "stop", 4))         {
                res_stop_cmd();
                break;
            }
            else if(!strncmp(buf, "ponderhit", 9))  res_ponderhit_cmd();
            else if(!strncmp(buf, "gameover", 8))     {
                res_gameover_cmd();
                break;
            }
        }
    }
    return 0;
}

/* -------------------
 スタティック関数実装部
 ------------------- */

/* ---------------------------------------------------------------------------
 receiving_thread
 usiメッセージの受信boxコマンド受信専用スレッド
 pthread_createによる呼び出しを行い、思考中でもコマンド受信ができるようにする。
 緊急コマンドを受け取った場合、グローバル変数を変化させて緊急事態を連絡する
 と共に必要によりメッセージをコマンドキュー(st_que)に追加する
 [引数]
 arg  : 使用しないのでNULLを指定する
 [戻り値]
 --------------------------------------------------------------------------- */
void *receiving_thread       (void *arg){
    char buf[SZ_USIBUFFER];
    size_t len;
    while(true){
        buf[0] = 0;
        fgets(buf, SZ_USIBUFFER, stdin);
        len = strlen(buf);
        if(buf[len-1] == '\n') buf[len-1]=' ';
        
        if(!strncmp(buf, "quit", strlen("quit"))){
            pthread_mutex_destroy(&st_lock);
            exit(EXIT_SUCCESS);
        }
        //排他ロック
        pthread_mutex_lock(&st_lock);
        
        if(!strncmp(buf, "stop", strlen("stop"))){
            g_stop_received = true;
        }
        if(!strncmp(buf, "gameover", strlen("gameover"))){
            g_stop_received = true;
        }
        if(!strncmp(buf, "ponderhit", strlen("ponderhit"))){
            //ponderhit後の動作定義の検討後に指定
        }
        //bufの内容をコマンドキューに追加する
        st_que = strqueue_push(st_que, buf);
        //排他ロック解除
        pthread_mutex_unlock(&st_lock);
    }
}
/* ---------------------------------------------------------------------------
 retrieve_message
 
 --------------------------------------------------------------------------- */
void retrieve_message        (char *buf){
    while(!st_que) nanosleep(&st_rqpt, NULL);
    pthread_mutex_lock(&st_lock);
    st_que = strqueue_pop(st_que, buf);
    pthread_mutex_unlock(&st_lock);
    return;
}

const char *read_go_time(unsigned int *tm, const char *str){
    //読み込むべき数値文字列を特定する
    const char *s = str;
    char string[16];
    memset(string, '\0', 16);
    int len = 0;
    while(*s!=' '){ s++; len++;}
    memcpy(string, str, len);
    *tm = atoi(string);
    str+=len+1;
    return str;
}

static char *st_usierror[] =
{
    "unknown usi message.",
};
void error_log(int errorid)
{
    char str[64];
    sprintf(str, "usi_error :%s\n", st_usierror[errorid]);
    record_log(str);
    exit(EXIT_FAILURE);
}
