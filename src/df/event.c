/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: event.c
 * 组件: df
 * 添加时间: 2025年 08月 20日 星期三 11:16:02 CST
 */

#include "event.h"
#include "context.h"

static OGS_POOL(pool, df_event_t);

void df_event_init(void)
{
    ogs_pool_init(&pool, ogs_app()->pool.event);
}

void df_event_term(void)
{
    ogs_queue_term(ogs_app()->queue);
    ogs_pollset_notify(ogs_app()->pollset);
}

void df_event_final(void)
{
    ogs_pool_final(&pool);
}

df_event_t *df_event_new(df_event_e id)
{
    df_event_t *e = NULL;

    ogs_pool_alloc(&pool, &e);
    ogs_assert(e);
    memset(e, 0, sizeof(*e));

    e->h.id = id;

    return e;
}

void df_event_free(df_event_t *e)
{
    ogs_assert(e);
    ogs_pool_free(&pool, e);
}

const char *df_event_get_name(df_event_t *e)
{
    if (e == NULL)
        return OGS_FSM_NAME_INIT_SIG;

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG: 
        return OGS_FSM_NAME_ENTRY_SIG;
    case OGS_FSM_EXIT_SIG: 
        return OGS_FSM_NAME_EXIT_SIG;

    case DF_EVT_PFCP_MESSAGE:
        return "DF_EVT_PFCP_MESSAGE";

    case OGS_EVENT_SBI_CLIENT:
        return "OGS_EVENT_SBI_CLIENT";
    case OGS_EVENT_SBI_TIMER:
        return "OGS_EVENT_SBI_TIMER";

    default: 
       break;
    }

    return "UNKNOWN_EVENT";
}
