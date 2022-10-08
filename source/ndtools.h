//
//  ndtools.h
//  shtsume
//
//  Created by Hkijin on 2022/02/03.
//

#ifndef ndtools_h
#define ndtools_h

#include <stdio.h>
#include "shogi.h"
#include "usi.h"
#include "dtools.h"
#include "shtsume.h"

/*
 * 探索情報の表示
 */
int  search_inf_sprintf  (char *restrict str,
                          const mvlist_t *mvlist,
                          const sdata_t *sdata );
int  search_inf_fprintf  (FILE *restrict stream,
                          const mvlist_t *mvlist,
                          const sdata_t *sdata );
#define SEARCH_INF_PRINTF(mvlist, sdata) \
                        search_inf_fprintf(stdout, mvlist, sdata)
/*
 * 探索着手リストの表示
 */
//mlistの内容表示
int mlist_sprintf   (char *restrict str,
                     const mlist_t *mlist,
                     const sdata_t *sdata );
int mlist_fprintf   (FILE *restrict stream,
                     const mlist_t *mlist,
                     const sdata_t *sdata );
#define MLIST_PRINTF(mlist,sdata)   mlist_fprintf(stdout,(mlist),(sdata))
//mvlistの内容表示
int mvlist_sprintf  (char *restrict str,
                     const mvlist_t *mvlist,
                     const sdata_t *sdata );
int mvlist_fprintf  (FILE *restrict stream,
                     const mvlist_t *mvlist,
                     const sdata_t *sdata );
#define MVLIST_PRINTF(mvlist,sdata)   mvlist_fprintf(stdout,(mvlist),(sdata))

int mvlist_sprintf_with_item (char *restrict str,
                              const mvlist_t *mvlist,
                              const sdata_t *sdata );
int mvlist_fprintf_with_item (FILE *restrict stream,
                              const mvlist_t *mvlist,
                              const sdata_t *sdata );

#define MVLIST_PRINTF_ITEM(mvlist,sdata) \
        mvlist_fprintf_with_item(stdout,(mvlist),(sdata))

int mvlist_fprintf_essence   (FILE *restrict  stream,
                              const mvlist_t *mvlist,
                              const sdata_t  *sdata );
#define MVLIST_PRINTF_ESSENCE(mvlist,sdata) \
        mvlist_fprintf_essence(stdout,(mvlist),(sdata))

/*
 * 詰手順表示
 */

//全手順表示
#define TP_NONE       0
#define TP_ZKEY       (1<<0)           //詰手順にzkeyを含める
#define TP_ALLMOVE    (1<<1)           //詰方全候補手表示

void tsume_print                (const sdata_t   *sdata,
                                 tbase_t         *tbase,
                                 unsigned int     flag );

int tsume_fprint                (FILE            *stream,
                                 const sdata_t   *sdata,
                                 tbase_t         *tbase,
                                 unsigned int     flag );
        
//初手から局面ごとに表示
void tsume_debug                (const sdata_t   *sdata,
                                 tbase_t         *tbase);

/*
 * kifファイルの生成を行う
 */
void generate_kif_file          (const char      *filename,
                                 const sdata_t   *sdata   ,
                                 tbase_t         *tbase    );

/*
 * 局面表の内容を表示する。
 */

void tlist_display              (const sdata_t   *sdata,
                                 turn_t              tn,
                                 tbase_t         *tbase);

/*
 *  探索手順検証用構造体
 */
typedef struct _mvel_t mvel_t;
struct _mvel_t
{
    short  dp;
    zkey_t zkey;
    mkey_t mkey;
};

#endif /* ndtools_h */
