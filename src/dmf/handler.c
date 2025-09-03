/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: handler.c
 * 组件: dmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:06 CST
 */

#include "handler.h"
#include "sbi-path.h"

void dmf_handle_report(const char *gnb_id, bool is_register)
{
    if (is_register)
        dmf_context_add_gnb(gnb_id);
    else
        dmf_context_remove_gnb(gnb_id);
}

void dmf_handle_gnb_sync(const char *gnb_id, bool is_register)
{
    dmf_event_t *e = NULL;
    int rv;
    
    e = dmf_event_gnb_sync_new(gnb_id, is_register);
    ogs_assert(e);
    
    rv = ogs_queue_push(ogs_app()->queue, e);
    if (rv != OGS_OK) {
        ogs_error("Failed to push gNB sync event");
        ogs_event_free(e);
    } else {
        ogs_pollset_notify(ogs_app()->pollset);
    }
}

void dmf_handle_gnb_deregister(const char *gnb_id)
{
    dmf_event_t *e = NULL;
    int rv;
    
    e = dmf_event_gnb_dereg_new(gnb_id);
    ogs_assert(e);
    
    rv = ogs_queue_push(ogs_app()->queue, e);
    if (rv != OGS_OK) {
        ogs_error("Failed to push gNB deregister event");
        ogs_event_free(e);
    } else {
        ogs_pollset_notify(ogs_app()->pollset);
    }
} 
