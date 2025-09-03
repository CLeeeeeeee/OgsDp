/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: timer.c
 * 组件: df
 * 添加时间: 2025年 08月 26日 星期一 14:38:00 CST
 */

#include "timer.h"
#include "event.h"
#include "ogs-app.h"

const char *df_timer_get_name(int timer_id)
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
    case DF_TIMER_PFCP_ASSOCIATION:
        return "DF_TIMER_PFCP_ASSOCIATION";
    case DF_TIMER_PFCP_NO_HEARTBEAT:
        return "DF_TIMER_PFCP_NO_HEARTBEAT";
    
    /* 数据转发相关定时器 */
    case DF_TIMER_DATA_FORWARD:
        return "DF_TIMER_DATA_FORWARD";
    case DF_TIMER_DNF_DISCOVERY:
        return "DF_TIMER_DNF_DISCOVERY";
    
    default: 
       break;
    }

    ogs_error("Unknown Timer[%d]", timer_id);
    return "UNKNOWN_TIMER";
}

static void timer_send_event(int timer_id, void *data)
{
    int rv;
    df_event_t *e = NULL;
    ogs_assert(data);

    switch (timer_id) {
    case DF_TIMER_PFCP_ASSOCIATION:
    case DF_TIMER_PFCP_NO_HEARTBEAT:
        e = df_event_new(DF_EVT_PFCP_MESSAGE);
        ogs_assert(e);
        e->timer_id = timer_id;
        e->pfcp_node = data;
        break;
    case DF_TIMER_DATA_FORWARD:
    case DF_TIMER_DNF_DISCOVERY:
        e = df_event_new(DF_EVT_PFCP_MESSAGE);
        ogs_assert(e);
        e->timer_id = timer_id;
        break;
    default:
        ogs_fatal("Unknown timer id[%d]", timer_id);
        ogs_assert_if_reached();
        break;
    }

    rv = ogs_queue_push(ogs_app()->queue, (ogs_event_t *)e);
    if (rv != OGS_OK) {
        ogs_error("ogs_queue_push() failed [%d] in %s",
                (int)rv, df_timer_get_name(timer_id));
        df_event_free(e);
        return;
    }

    ogs_pollset_notify(ogs_app()->pollset);
}

void df_timer_pfcp_association(void *data)
{
    timer_send_event(DF_TIMER_PFCP_ASSOCIATION, data);
}

void df_timer_pfcp_no_heartbeat(void *data)
{
    timer_send_event(DF_TIMER_PFCP_NO_HEARTBEAT, data);
}

void df_timer_data_forward(void *data)
{
    timer_send_event(DF_TIMER_DATA_FORWARD, data);
}

void df_timer_dnf_discovery(void *data)
{
    timer_send_event(DF_TIMER_DNF_DISCOVERY, data);
}


