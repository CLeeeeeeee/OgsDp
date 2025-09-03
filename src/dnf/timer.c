/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: timer.c
 * 组件: dnf
 * 添加时间: 2025年 08月 26日 星期一 14:38:00 CST
 */

#include "timer.h"
#include "event.h"
#include "dnf-sm.h"
#include "ogs-app.h"

const char *dnf_timer_get_name(int timer_id)
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
    
    /* 数据转发相关定时器 */
    case DNF_TIMER_DATA_FORWARD:
        return "DNF_TIMER_DATA_FORWARD";
    case DNF_TIMER_SESSION_CLEANUP:
        return "DNF_TIMER_SESSION_CLEANUP";
    
    default: 
       break;
    }

    ogs_error("Unknown Timer[%d]", timer_id);
    return "UNKNOWN_TIMER";
}

static void timer_send_event(int timer_id, void *data)
{
    int rv;
    dnf_event_t *e = NULL;
    ogs_assert(data);

    switch (timer_id) {
    case DNF_TIMER_DATA_FORWARD:
    case DNF_TIMER_SESSION_CLEANUP:
        e = dnf_event_new(DNF_EVT_DATA_FORWARD);
        break;
    default:
        ogs_fatal("Unknown timer id[%d]", timer_id);
        ogs_assert_if_reached();
        break;
    }

    rv = ogs_queue_push(ogs_app()->queue, e);
    if (rv != OGS_OK) {
        ogs_error("ogs_queue_push() failed [%d] in %s",
                (int)rv, dnf_timer_get_name(timer_id));
        dnf_event_free(e);
        return;
    }

    ogs_pollset_notify(ogs_app()->pollset);
}

void dnf_timer_data_forward(void *data)
{
    timer_send_event(DNF_TIMER_DATA_FORWARD, data);
}

void dnf_timer_session_cleanup(void *data)
{
    timer_send_event(DNF_TIMER_SESSION_CLEANUP, data);
}
