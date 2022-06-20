//
//  usi.h
//  shshogi
//
//  Created by Hkijin on 2021/11/05.
//

#ifndef usi_h
#define usi_h

#include "shogi.h"

/*------------------------------------------------------------------------------
 USI関連ライブラリ
 -----------------------------------------------------------------------------*/

#define SZ_USIBUFFER       8192
#define SZ_FILEPATH        256

//文字列キュー
typedef struct _strqueue_t strqueue_t;
struct _strqueue_t {
    char       *str;
    strqueue_t *next;
};
strqueue_t *strqueue_push(strqueue_t *queue, const char *str_in);
strqueue_t *strqueue_pop (strqueue_t *queue, char *str_out);

//持ち時間モード
#define TM_INFINATE     -1
typedef enum _MDTime
{
    MD_KM,           // 切れ負け
    MD_BY,           // 秒読み
    MD_FY,           // フィッシャールール
    MD_INFINATE,     // 無制限
    MD_DEBUG         // デバッグ用（無制限）
} MDTime;

extern int g_time_mode;
extern int g_time_limit;

const char *read_go_time(unsigned int *tm, const char *str);

//USIオプション構造体
//check
typedef struct _checkop_t checkop_t;
struct _checkop_t
{
    char op_name[32];
    bool default_value;
};
//spin
typedef struct _spinop_t spinop_t;
struct _spinop_t
{
    char op_name[32];
    int  default_value;
    int  min;
    int  max;
};
//string
typedef struct _stringop_t stringop_t;
struct _stringop_t
{
    char op_name[32];
    char *default_string;
};
//filename
typedef struct _filenameop_t filenameop_t;
struct _filenameop_t
{
    char op_name[32];
    char *default_filename;
};

//res command
void res_usi_cmd(void);
void res_isready_cmd(void);
void res_setoption_cmd(const char *buf);
void res_usinewgame_cmd(void);
void res_position_cmd(const char *buf);
void res_gomate_cmd(const char *buf);
void res_goponder_cmd(const char *buf);
void res_go_cmd(const char *buf);
void res_stop_cmd(void);
void res_ponderhit_cmd(void);
void res_gameover_cmd(void);

//usi変数
extern unsigned int g_btime, g_wtime; //残り時間(mS)、g_btime:先手 g_wtime:後手
extern unsigned int g_byoyomi;        //秒読み(mS)
extern unsigned int g_binc, g_winc;   //１手ごとの加算時間(mS)
extern sdata_t g_sdata;               //USIデータ受け渡し用
extern char g_str[SZ_USIBUFFER];      //USIコマンドバッファ
extern bool g_stop_received;          //stopコマンド受信時　true;
extern char g_logfile_path[256];      //logfileへのpath

//LOG記録用
#define PRINT_STR(str)   puts(str), record_log(str)
extern char g_logfile_name[SZ_FILEPATH];
void create_log_filename(void);       //Logファイル名を生成する
void record_log(const char *message); //LOGファイルにmessageテキストを記録する



//sfenフォーマット読み書き
char *sfen_to_ssdata (char *str, ssdata_t *ssdata);
char *sfen_to_move   (move_t *move, char *str);
int   move_to_sfen   (char *str,  move_t move);

int usi_main (void);

//エラー処理
#define USI_UNKNOWN_MSG 0
void error_log(int errorid);

#endif /* usi_h */
