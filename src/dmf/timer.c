/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: timer.c
 * 组件: dmf
 * 添加时间: 2025年 08月 26日 星期一 14:38:00 CST
 */

#include "timer.h"
#include "event.h"
#include "ogs-app.h"

const char *dmf_timer_get_name(int timer_id)
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
    
    /* gNB 管理相关定时器 */
    case DMF_TIMER_GNB_SYNC:
        return "DMF_TIMER_GNB_SYNC";
    case DMF_TIMER_GNB_HEARTBEAT:
        return "DMF_TIMER_GNB_HEARTBEAT";
    
    default: 
       break;
    }

    ogs_error("Unknown Timer[%d]", timer_id);
    return "UNKNOWN_TIMER";
}

static void timer_send_event(int timer_id, void *data)
{
    int rv;
    dmf_event_t *e = NULL;
    ogs_assert(data);

    switch (timer_id) {
    case DMF_TIMER_GNB_SYNC:
    case DMF_TIMER_GNB_HEARTBEAT:
        e = dmf_event_new(DMF_EVENT_GNB_SYNC);
        ogs_assert(e);
        e->h.timer_id = timer_id;
        break;
    default:
        ogs_fatal("Unknown timer id[%d]", timer_id);
        ogs_assert_if_reached();
        break;
    }

    rv = ogs_queue_push(ogs_app()->queue, e);
    if (rv != OGS_OK) {
        ogs_error("ogs_queue_push() failed [%d] in %s",
                (int)rv, dmf_timer_get_name(timer_id));
        ogs_free(e);
        return;
    }

    ogs_pollset_notify(ogs_app()->pollset);
}

void dmf_timer_gnb_sync(void *data)
{
    timer_send_event(DMF_TIMER_GNB_SYNC, data);
}

void dmf_timer_gnb_heartbeat(void *data)
{
    timer_send_event(DMF_TIMER_GNB_HEARTBEAT, data);
}
