/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: event.h
 * 组件: df
 * 添加时间: 2025年 08月 20日 星期三 11:16:03 CST
 */

#ifndef DF_EVENT_H
#define DF_EVENT_H

#include "ogs-proto.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ogs_pfcp_node_s ogs_pfcp_node_t;
typedef struct ogs_pfcp_xact_s ogs_pfcp_xact_t;
typedef struct ogs_pfcp_message_s ogs_pfcp_message_t;
typedef struct df_sess_s df_sess_t;

typedef enum {
    DF_EVT_BASE = OGS_MAX_NUM_OF_PROTO_EVENT,

    DF_EVT_PFCP_MESSAGE,

    DF_EVT_TOP,

} df_event_e;

typedef struct df_event_s {
    ogs_event_t h;

    int timer_id;

    ogs_pkbuf_t *pkbuf;

    ogs_pfcp_node_t *pfcp_node;
    ogs_pool_id_t pfcp_xact_id;
    ogs_pfcp_message_t *pfcp_message;
} df_event_t;

OGS_STATIC_ASSERT(OGS_EVENT_SIZE >= sizeof(df_event_t));

void df_event_init(void);
void df_event_term(void);
void df_event_final(void);

df_event_t *df_event_new(df_event_e id);
void df_event_free(df_event_t *e);

const char *df_event_get_name(df_event_t *e);

#ifdef __cplusplus
}
#endif

#endif /* DF_EVENT_H */
