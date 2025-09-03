/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: event.c
 * 组件: dnf
 * 添加时间: 2025年 09月 01日 星期一 14:00:00 CST
 */

#include "event.h"
#include "context.h"

static OGS_POOL(pool, dnf_event_t);

void dnf_event_init(void)
{
    ogs_pool_init(&pool, ogs_app()->pool.event);
}

void dnf_event_term(void)
{
    ogs_queue_term(ogs_app()->queue);
    ogs_pollset_notify(ogs_app()->pollset);
}

void dnf_event_final(void)
{
    ogs_pool_final(&pool);
}

dnf_event_t *dnf_event_new(dnf_event_e id)
{
    dnf_event_t *e = NULL;

    ogs_pool_alloc(&pool, &e);
    ogs_assert(e);
    memset(e, 0, sizeof(*e));

    e->h.id = id;

    return e;
}

void dnf_event_free(dnf_event_t *e)
{
    ogs_assert(e);
    ogs_pool_free(&pool, e);
}

const char *dnf_event_get_name(dnf_event_t *e)
{
    if (e == NULL)
        return OGS_FSM_NAME_INIT_SIG;

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG: 
        return OGS_FSM_NAME_ENTRY_SIG;
    case OGS_FSM_EXIT_SIG: 
        return OGS_FSM_NAME_EXIT_SIG;

    case DNF_EVT_NRF_REGISTER:
        return "DNF_EVT_NRF_REGISTER";
    case DNF_EVT_NRF_DEREGISTER:
        return "DNF_EVT_NRF_DEREGISTER";
    case DNF_EVT_DATA_FORWARD:
        return "DNF_EVT_DATA_FORWARD";
    case DNF_EVT_SESSION_CREATE:
        return "DNF_EVT_SESSION_CREATE";
    case DNF_EVT_SESSION_REMOVE:
        return "DNF_EVT_SESSION_REMOVE";
    
    case OGS_EVENT_SBI_CLIENT:
        return "OGS_EVENT_SBI_CLIENT";
    case OGS_EVENT_SBI_TIMER:
        return "OGS_EVENT_SBI_TIMER";

    default: 
       break;
    }

    return "UNKNOWN_EVENT";
}
