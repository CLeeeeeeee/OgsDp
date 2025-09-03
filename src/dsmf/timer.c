/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dsmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: timer.c
 * 组件: dsmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:08 CST
 */

#include "timer.h"
#include "event.h"
#include "ogs-core.h"

const char *dsmf_timer_get_name(int timer_id)
{
    switch (timer_id) {
    case OGS_TIMER_NF_INSTANCE_REGISTRATION_INTERVAL:
        return OGS_TIMER_NAME_NF_INSTANCE_REGISTRATION_INTERVAL;
    case OGS_TIMER_NF_INSTANCE_HEARTBEAT_INTERVAL:
        return OGS_TIMER_NAME_NF_INSTANCE_HEARTBEAT_INTERVAL;
    case OGS_TIMER_NF_INSTANCE_NO_HEARTBEAT:
        return OGS_TIMER_NAME_NF_INSTANCE_NO_HEARTBEAT;
    case OGS_TIMER_NF_INSTANCE_VALIDITY:
        return OGS_TIMER_NAME_NF_INSTANCE_VALIDITY;
    case OGS_TIMER_SUBSCRIPTION_VALIDITY:
        return OGS_TIMER_NAME_SUBSCRIPTION_VALIDITY;
    case OGS_TIMER_SUBSCRIPTION_PATCH:
        return OGS_TIMER_NAME_SUBSCRIPTION_PATCH;
    case OGS_TIMER_SBI_CLIENT_WAIT:
        return OGS_TIMER_NAME_SBI_CLIENT_WAIT;
    
    /* PFCP 相关定时器 */
    case DSMF_TIMER_PFCP_ASSOCIATION:
        return "DSMF_TIMER_PFCP_ASSOCIATION";
    case DSMF_TIMER_PFCP_NO_HEARTBEAT:
        return "DSMF_TIMER_PFCP_NO_HEARTBEAT";
    case DSMF_TIMER_PFCP_NO_ESTABLISHMENT_RESPONSE:
        return "DSMF_TIMER_PFCP_NO_ESTABLISHMENT_RESPONSE";
    case DSMF_TIMER_PFCP_NO_DELETION_RESPONSE:
        return "DSMF_TIMER_PFCP_NO_DELETION_RESPONSE";
    
    /* 会话管理相关定时器 */
    case DSMF_TIMER_SESSION_ESTABLISHMENT:
        return "DSMF_TIMER_SESSION_ESTABLISHMENT";
    case DSMF_TIMER_SESSION_MODIFICATION:
        return "DSMF_TIMER_SESSION_MODIFICATION";
    case DSMF_TIMER_SESSION_DELETION:
        return "DSMF_TIMER_SESSION_DELETION";
    case DSMF_TIMER_RAN_SYNC:
        return "DSMF_TIMER_RAN_SYNC";
    
    default: 
       break;
    }

    ogs_error("Unknown Timer[%d]", timer_id);
    return "UNKNOWN_TIMER";
}

static void timer_send_event(int timer_id, void *data)
{
    int rv;
    dsmf_event_t *e = NULL;
    ogs_assert(data);

    switch (timer_id) {
    case DSMF_TIMER_PFCP_ASSOCIATION:
    case DSMF_TIMER_PFCP_NO_HEARTBEAT:
        e = dsmf_event_new(DSMF_EVT_DN4_TIMER);
        ogs_assert(e);
        e->h.timer_id = timer_id;
        e->pfcp_node = data;
        break;
    case DSMF_TIMER_PFCP_NO_ESTABLISHMENT_RESPONSE:
    case DSMF_TIMER_PFCP_NO_DELETION_RESPONSE:
        e = dsmf_event_new(DSMF_EVT_DN4_TIMER);
        ogs_assert(e);
        e->h.timer_id = timer_id;
        /* TODO: 需要正确传递会话 ID */
        break;
    case DSMF_TIMER_SESSION_ESTABLISHMENT:
    case DSMF_TIMER_SESSION_MODIFICATION:
    case DSMF_TIMER_SESSION_DELETION:
    case DSMF_TIMER_RAN_SYNC:
        e = dsmf_event_new(DSMF_EVT_SBI_TIMER);
        ogs_assert(e);
        e->h.timer_id = timer_id;
        e->sess_id = *(ogs_pool_id_t *)data;
        break;
    default:
        ogs_fatal("Unknown timer id[%d]", timer_id);
        ogs_assert_if_reached();
        break;
    }

    rv = ogs_queue_push(ogs_app()->queue, e);
    if (rv != OGS_OK) {
        ogs_error("ogs_queue_push() failed [%d] in %s",
                (int)rv, dsmf_timer_get_name(timer_id));
        dsmf_event_free(e);
    }
}

void dsmf_timer_pfcp_association(void *data)
{
    timer_send_event(DSMF_TIMER_PFCP_ASSOCIATION, data);
}

void dsmf_timer_pfcp_no_heartbeat(void *data)
{
    timer_send_event(DSMF_TIMER_PFCP_NO_HEARTBEAT, data);
}

void dsmf_timer_session_establishment(void *data)
{
    timer_send_event(DSMF_TIMER_SESSION_ESTABLISHMENT, data);
}

void dsmf_timer_session_modification(void *data)
{
    timer_send_event(DSMF_TIMER_SESSION_MODIFICATION, data);
}

void dsmf_timer_session_deletion(void *data)
{
    timer_send_event(DSMF_TIMER_SESSION_DELETION, data);
}

void dsmf_timer_ran_sync(void *data)
{
    timer_send_event(DSMF_TIMER_RAN_SYNC, data);
} 