//
//  shtsume.h
//  shshogi
//
//  Created by Hkijin on 2022/02/09.
//

#ifndef shtsume_h
#define shtsume_h

#include <limits.h>
#include "shogi.h"
#include "usi.h"
#include "dtools.h"

/* -------------------------
 詰将棋解図に特化したプログラム
 --------------------------- */
/*
 * プログラムID
 */
#define PROGRAM_NAME       "shtsume"
#define VERSION_INFO       "v0.7.1"
#define AUTHOR_NAME        "hkijin"

/*
 * USIオプション情報
 */
typedef struct _usioption_t usioption_t;
struct _usioption_t
{
    spinop_t     usi_hash;          /* 局面表に指定する容量 */
    checkop_t    usi_ponder;
    /* 固有オプションがあれば記入 */
    spinop_t     mt_min_pn;         /* Make Tree処理時の最小pn閾値 */
    spinop_t     n_make_tree;       /* Make Tree処理回数 */
    spinop_t     search_level;      /* 追加探索でのルートしきい値 */
    checkop_t    out_lvkif;         /* 追加探索での棋譜出力有無 */
    stringop_t   user_path;         /* shtsumeで使用するフォルダ名 */
    checkop_t    summary;           /* 探索レポート出力有無 */
    checkop_t    s_allmove;         /* 探索レポートの詰方全出力 */
};

extern usioption_t          g_usioption;
extern bool                 g_usi_ponder;
extern uint64_t             g_usi_hash;
extern uint16_t             g_mt_min_pn;
extern uint8_t              g_n_make_tree;
extern uint8_t              g_search_level;
extern bool                 g_out_lvkif;
extern short                g_pv_length;
extern char                 g_user_path[256];
extern bool                 g_summary;
extern bool                 g_disp_search;
extern uint32_t             g_smode;
extern bool                 g_commandline;

#define TSUME_MAX_DEPTH     2000
#define MAKE_TREE_NUM       2
#define MAKE_TREE_PN_PLUS   1
#define MAKE_TREE_MIN_PN    4
#define INFINATE            SHRT_MAX
#define PROOF_MAX           2048
#define DISPROOF_MAX        2048
#define ADD_SEARCH_SH       30

/*
 * 探索パラメータ制限値
 */
#define TBASE_SIZE_DEFAULT  256
#define TBASE_SIZE_MIN      1
#define TBASE_SIZE_MAX      65535

#define LEAF_PN_DEFAULT     4
#define LEAF_PN_MIN         3
#define LEAF_PN_MAX         10

#define MAKE_REPEAT_DEFAULT 2
#define MAKE_REPEAT_MIN     1
#define MAKE_REPEAT_MAX     5

#define SEARCH_LV_DEFAULT   0
#define SEARCH_LV_MIN       0
#define SEARCH_LV_MAX       50

#define PV_LENGTH_DEFAULT   5
#define PV_USI_DEFAULT      20
#define PV_LENGTH_MIN       5
#define PV_LENGTH_MAX       30

#define PV_INTERVAL_DEFAULT 5
#define PV_INTERVAL_MIN     5
#define PV_INTERVAL_MAX     60


/*
 * 詰探索情報を扱う構造体
 */
typedef struct _tdata_t tdata_t;
struct _tdata_t
{
    uint16_t pn;      /* proof number          */
    uint16_t dn;      /* disproof number       */
    uint16_t sh;      /* tree height from leaf */
};

void print_tdata(tdata_t *tdata);

extern tdata_t g_tdata_init;
extern tdata_t g_tdata_tsumi;
extern tdata_t g_tdata_fudumi;

/*
 * 探索ログ
 */
typedef struct _nsearchlog_t nsearchlog_t;
struct _nsearchlog_t
{
    move_t       move;
    char move_str[32];
    zkey_t       zkey;       /* zkey after move       */
    mkey_t       mkey;       /* mkey                  */
    tdata_t    thdata;       /* threshold pn,dn,sh    */
    tdata_t     tdata;
};

void print_nsearch_log(nsearchlog_t *log);

/*
 * 詰探索情報構造体
 */
typedef struct _tsearchinf_t tsearchinf_t;
struct _tsearchinf_t {
    /* pv */
    nsearchlog_t mvinf[TSUME_MAX_DEPTH];      /* 探索時の読み筋      */
    int          depth;                       /* 探索深さ           */
    int          sel_depth;                   /* 最大探索深さ        */
    int          score_cp;                    /* 評価値(ルート証明数) */
    int          score_mate;                  /* 詰手数             */
    /* 時間 */
    clock_t      elapsed;                     /* 経過時間            */
    /* 統計情報 */
    uint64_t     nodes;                       /* 探索局面数          */
    int          nps;                         /* 探索速度            */
    int          hashfull;                    /* ハッシュの使用率     */
};

extern tsearchinf_t       g_tsearchinf;
extern clock_t            g_prev_update;
extern uint64_t           g_prev_nodes;
extern clock_t            g_info_interval;


/* -------------------------------------------------------------------------
 局面表＆探索用データタイプ
 [局面表]
 tlist_t   : 初期局面からの手数とtdata_tデータを保持するリスト型コンテナ。
             pv使用およびデータ保護情報を格納。
 mcard_t   : 持駒情報(mkey)で紐つけられたリスト型コンテナ。tlist_tデータおよび
 　　　　　　　 詰みの場合の駒余り情報を格納。
 zfolder_t : 盤面情報(zkey)で紐つけられたリスト型コンテナ。mcard_tデータを格納。
 
 データ構造図
 [zfolder]-------------------------------+
 | zkey
 | [mcard]----------
 | |  mkey
 | |  [tlist]->[tlist]->...
 | |
 | |  next
 | +----------------
 |     ↓
 | [mcard]----------
 | |  mkey
 | |  [tlist]->[tlist]->...
 | |
 | |  next
 | +----------------
 |     ↓
 |    ...
 +---------------------------------------+
 [探索]
 mlist_t   : 合駒など逐次探索が有効な着手を格納。
 mvlist_t  : 探索情報データ付き着手リスト。以下の情報が付随
             mkey(証明駒、反証駒)、駒余り情報、tlist_t,mcard_t
 ------------------------------------------------------------------------- */

/*
 * tlist_t: pn,dn,sh,dpを格納。16byte
 */
#define ALL_DEPTH     -1
#define N_MCARD_TYPE   5
enum{
    SUPER_TSUMI,INFER_FUDUMI,
    EQUAL_TSUMI,EQUAL_FUDUMI,EQUAL_UNKNOWN
};

/*
 *  tlist_t 16byte
 */

typedef struct _tlist_t tlist_t;
struct _tlist_t
{
    tlist_t             *next;
    tdata_t             tdata;
    short                  dp;         // 初期局面からの深さ
};

/*
 * mcard_t: 24byte
 */
typedef struct _mcard_t mcard_t;
struct _mcard_t
{
    mkey_t               mkey;          /* 詰方の持ち駒     */
    mcard_t             *next;
    tlist_t            *tlist;          /* 深さ別情報格納    */
    unsigned int         hinc: 1;       // 詰め上がりで駒余り
    unsigned int        nouse: 6;       // 詰時余り駒数
    unsigned int      current: 1;       // pvで使用中
    unsigned int      protect: 1;       // 保護フラグ
    unsigned int          cpn:16;       // current時の証明数しきい値
    unsigned int             : 7;
};

/*
 * zfolder_t :24byte
 */

typedef struct _zfolder_t zfolder_t;
struct _zfolder_t
{
    zkey_t                zkey;
    zfolder_t            *next;
    mcard_t             *mcard;
};

typedef struct _mlist_t mlist_t;
struct _mlist_t
{
    move_t                move;
    mlist_t              *next;
};

mlist_t* mlist_alloc(void);
mlist_t* mlist_add(mlist_t *mlist, move_t move);
mlist_t* mlist_last(mlist_t *mlist);
mlist_t* mlist_concat(mlist_t *alist, mlist_t *blist);
mlist_t* mlist_reverse(mlist_t *mlist);
void mlist_free(mlist_t *mlist);
void mlist_free_stack(void);
void mlist_print(mlist_t *mlist, sdata_t *sdata);

#define SPN_MAX         2047

typedef struct _mvlist_t mvlist_t;
struct _mvlist_t
{
    mlist_t             *mlist;
    mvlist_t             *next;
    mkey_t                mkey;       /* 証明駒 反証駒 */
    tdata_t              tdata;
    unsigned int          hinc: 1;
    unsigned int           inc: 1;
    unsigned int        search: 1;
    unsigned int            cu: 1;
    unsigned int            pr: 1;
    unsigned int        length: 4;
    unsigned int         nouse: 6;    /* 持ち駒=証明駒での詰上がり時持ち駒数       */
    unsigned int        nouse2: 6;    /* 局面持ち駒での詰め上がり時持ち駒数        */
    unsigned int           spn:11;    /* 補助証明数 */
};

mvlist_t* mvlist_alloc(void);
mvlist_t* mvlist_add  (mvlist_t  *mvlist,
                       mlist_t   *mlist  );
mvlist_t* mvlist_last (mvlist_t  *mvlist );
mvlist_t* sdata_mvlist_sort (mvlist_t *mvlist,
                             const sdata_t *sdata,
                             int (*compfunc)
                                 (const mvlist_t *a,
                                  const mvlist_t *b,
                                  const sdata_t  *s));
mvlist_t* sdata_mvlist_reorder(mvlist_t *mvlist,
                               const sdata_t *sdata,
                               int  (*compfunc)
                                    (const mvlist_t *a,
                                     const mvlist_t *b,
                                     const sdata_t  *s));
mvlist_t* mvlist_nth  (mvlist_t  *mvlist, unsigned int n);
unsigned int mvlist_length(mvlist_t *mvlist);
void mvlist_free(mvlist_t *mvlist);
void mvlist_free_stack(void);
void mvlist_print(mvlist_t *mvlist, sdata_t *sdata, int flag);

#define MLIST_SET_NORM(mlist, src, dest) \
        (mlist) = mlist_alloc(),         \
        (mlist)->move.prev_pos = (src),  \
        (mlist)->move.new_pos = (dest),  \
        (mlist)->next = NULL
#define MLIST_SET_PROM(mlist, src, dest) \
        (mlist) = mlist_alloc(),         \
        (mlist)->move.prev_pos = (src),  \
        (mlist)->move.new_pos = ((dest)|0x80),\
        (mlist)->next = NULL

#define MVLIST_SET_NORM(mvlist, mlist, src, dest) \
        MLIST_SET_NORM(mlist, src, dest),         \
        (mvlist) = mvlist_add((mvlist),(mlist))
#define MVLIST_SET_PROM(mvlist, mlist, src, dest) \
        MLIST_SET_PROM(mlist, src, dest),         \
        (mvlist) = mvlist_add((mvlist),(mlist))

/*
 * 着手候補の並べ替え用
 */
//通常探索用
int proof_number_comp     (const mvlist_t *a,
                           const mvlist_t *b,
                           const sdata_t  *s );
int disproof_number_comp  (const mvlist_t *a,
                           const mvlist_t *b,
                           const sdata_t  *s );

/*
 * 証明駒、反証駒
 */
mkey_t proof_koma  (const sdata_t *sdata);
void proof_koma_or (const sdata_t *sdata, mvlist_t *mvlist, mvlist_t *list);
void proof_koma_and(const sdata_t *sdata, mvlist_t *mvlist, mvlist_t *list);
mkey_t disproof_koma(const sdata_t *sdata);
void disproof_koma_or(const sdata_t *sdata, mvlist_t *mvlist, mvlist_t *list);
void disproof_koma_and(const sdata_t *sdata, mvlist_t *mvlist, mvlist_t *list);

/*
 * 局面表
 */
#define MCARDS_PER_MBYTE    16384    /* 局面表1Mbyteあたりの要素数          */
#define GC_DELETE_RATE      20       /* 局面表削除の最小目標値(%)            */
#define GC_TSUMI_RATE       50       /* 局面表中の詰みデータ占有率の最大値(%)  */
#define GC_DELETE_OFFSET    5        /* 局面表から不詰データを消す際の基準値   */

#define HASH_FUNC(zkey, tbase)    ((zkey)&((tbase)->mask))

typedef struct _tbase_t tbase_t;
struct _tbase_t
{
    zkey_t       mask;                   /* ハッシュ関数に使用するマスク */
    uint64_t     sz_tbl;                 /* indexのサイズ            */
    uint64_t     sz_elm;                 /* 局面表で扱う要素数         */
    
    uint64_t     size;                   /* 局面表のサイズ           　*/
    uint64_t     num;                    /* 確保したmcardの要素数      */
    uint64_t     pr_num;                 /* GCから保護しているmcard数   */
    zfolder_t    **table;                /* 局面表本体                */
    zfolder_t    *zstack;                /* 未使用zfolderの先頭アドレス */
    zfolder_t    *zpool;                 /* zfolder配列先頭           */
    mcard_t      *mstack;                /* 未使用mcardの先頭アドレス   */
    mcard_t      *mpool;                 /* mcard配列先頭             */
    tlist_t      *tstack;                /* 未使用tlistの先頭アドレス   */
    tlist_t      *tpool;                 /* tlist配列先頭             */
};

tbase_t*   create_tbase     (uint64_t base_size);
void initialize_tbase       (tbase_t  *tbase);
void destroy_tbase          (tbase_t  *tbase);
void tbase_gc               (tbase_t  *tbase);

tlist_t* tlist_alloc        (tbase_t  *tbase);
void tlist_free             (tlist_t *tlist, tbase_t *tbase);
mcard_t* mcard_alloc        (tbase_t  *tbase);
zfolder_t* zfolder_alloc    (tbase_t  *tbase);


void tbase_lookup           (const sdata_t *sdata,
                             mvlist_t      *mvlist,
                             turn_t         tn,
                             tbase_t       *tbase  );
void make_tree_lookup       (const sdata_t *sdata,
                             mvlist_t      *mvlist,
                             turn_t         tn,
                             tbase_t       *tbase);
bool hs_tbase_lookup        (const sdata_t *sdata,
                             turn_t         tn,
                             tbase_t       *tbase);
void tbase_update           (const sdata_t *sdata,
                             mvlist_t      *mvlist,
                             turn_t         tn,
                             tbase_t       *tbase);
void make_tree_update       (const sdata_t *sdata,
                             mvlist_t      *mvlist,
                             turn_t         tn,
                             tbase_t       *tbase);
mcard_t* tbase_set_current  (const sdata_t *sdata,
                             mvlist_t      *mvlist,
                             turn_t         tn,
                             tbase_t       *tbase);
mcard_t* make_tree_set_item (const sdata_t *sdata,
                             turn_t         tn,
                             bool           item,
                             tbase_t       *tbase);
#define MAKE_TREE_SET_CURRENT(sdata, tn, tbase) \
        make_tree_set_item((sdata), (tn), false, (tbase))
#define MAKE_TREE_SET_PROTECT(sdata, tn, tbase) \
        make_tree_set_item((sdata), (tn), true, (tbase))

void tbase_clear_protect   (tbase_t       *tbase);

extern bool g_tsumi;
extern short g_gc_max_level;
extern short g_gc_num;
extern tbase_t *g_tbase;
extern char g_sfen_pos_str[256];
extern mcard_t *g_mcard[N_MCARD_TYPE];


/*
 * 無駄合い判定
 */

bool invalid_drops         (const sdata_t *sdata,
                            unsigned int   dest ,
                            tbase_t       *tbase );

/*
 * 着手生成
 */
mvlist_t* generate_check        (const sdata_t *sdata, tbase_t *tbase);
mvlist_t* generate_evasion      (const sdata_t *sdata, tbase_t *tbase);
bool drop_check                 (const sdata_t *sdata, komainf_t drop);
bool symmetry_check             (const sdata_t *sdata);
extern bool g_invalid_drops;

/*
 * bn探索
 */
uint16_t proof_number     (mvlist_t  *mvlist);
uint16_t disproof_number  (mvlist_t  *mvlist);
uint16_t proof_count      (mvlist_t  *mvlist);
uint16_t disproof_count   (mvlist_t  *mvlist);

uint16_t sub_proof_number (mvlist_t  *mvlist);

void bn_search                  (const sdata_t   *sdata,
                                 tdata_t         *tdata,
                                 tbase_t         *tbase);
void bns_or                     (const sdata_t   *sdata,
                                 tdata_t      *th_tdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase );
void make_tree                  (const sdata_t   *sdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase);
void bns_plus                   (const sdata_t   *sdata,
                                 mvlist_t       *mvlist,
                                 tbase_t         *tbase);

extern bool         g_suspend;
extern bool         g_error;
extern unsigned int g_root_pn;
extern unsigned int g_root_max;
extern int          g_loop;
extern bool         g_redundant;

/*
 * 出力用
 */
void tsearchpv_update           (const sdata_t *sdata,
                                 tbase_t       *tbase);
void tsearchinf_update          (const sdata_t *sdata,
                                 tbase_t       *tbase,
                                 clock_t        start,
                                 char          *str  );
int  tsearchinf_sprintf         (char *str);
int  tsearchpn_sprintf          (char *str);
#define SEND_TSEARCHPV(str)      \
        tsearchpv_sprintf(str), record_log(str), puts(str)

//探索レポート
int create_search_report(void);

/*
 * エラーログ
 */
extern char g_errorlog_name[SZ_FILEPATH];
extern char g_error_location[256];
void shtsume_error_log          (const sdata_t *sdata,
                                 mvlist_t      *mvlist,
                                 tbase_t       *tbase);
/*
 * ループ不詰解析ログ
 */
void search_error_log          (const sdata_t  *sdata,
                                tbase_t        *tbase);

/**
 * tsearchpv専用の千日手検出用table
 */

#define MTT_SIZE 2047
//mkey card
typedef struct _mcd_t mcd_t;
struct _mcd_t
{
    mkey_t      mkey;           /* 詰方の持ち駒 */
    mcd_t       *next;
};

//zkey folder
typedef struct _zfr_t zfr_t;
struct _zfr_t
{
    zkey_t      zkey;
    mcd_t       *mcd;
    zfr_t       *next;
};
typedef struct _mtt_t mtt_t;
struct _mtt_t
{
    uint32_t    size;            /* 局面表のサイズ */
    uint32_t    num;             /* mcdの要素数   */
    zfr_t       **table;         /* 局面表本体    */
    zfr_t       *zstack;         /* 未使用zfrの先頭アドレス */
    zfr_t       *zpool;          /* zfrの先頭(消去用)     */
    mcd_t       *mstack;         /* 未使用mcdの先頭アドレス */
    mcd_t       *mpool;          /* mcdの先頭(消去用)     */
};

extern mtt_t   *g_mtt;
mtt_t *create_mtt   (uint32_t  base_size);
void  init_mtt      (mtt_t     *mtt);
void  destroy_mtt   (mtt_t     *mtt);
mcd_t *mcd_alloc    (mtt_t     *mtt);
zfr_t *zfr_alloc    (mtt_t     *mtt);

bool  mtt_lookup    (const sdata_t *sdata,
                     turn_t  tn,
                     mtt_t   *mtt);
void  mtt_setup     (const sdata_t *sdata,
                     turn_t  tn,
                     mtt_t   *mtt);
void  mtt_reset     (const sdata_t *sdata,
                     turn_t  tn,
                     mtt_t   *mtt);

#endif /* shtsume_h */
