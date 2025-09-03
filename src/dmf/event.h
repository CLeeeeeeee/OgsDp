/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: event.h
 * 组件: dmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:05 CST
 */

#ifndef DMF_EVENT_H
#define DMF_EVENT_H

#include "ogs-app.h"
#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DMF_EVENT_BASE = OGS_MAX_NUM_OF_PROTO_EVENT,

    DMF_EVENT_GNB_SYNC,
    DMF_EVENT_GNB_DEREG,
    DMF_EVENT_PRINT_GNB_LIST,

    MAX_NUM_OF_DMF_EVENT,

} dmf_event_e;

typedef struct dmf_event_s {
    ogs_event_t h;
    
    ogs_sbi_request_t *request;
    ogs_sbi_response_t *response;
    ogs_sbi_message_t message;
    
    struct {
        char gnb_id[64];
        bool is_register;
    } gnb_sync;
    
    struct {
        char gnb_id[64];
    } gnb_dereg;
    
    struct {
        ogs_timer_t *timer;
    } print_gnb_list;
} dmf_event_t;

dmf_event_t *dmf_event_new(int id);
const char *dmf_event_get_name(const dmf_event_t *e);

// 事件创建辅助函数
dmf_event_t *dmf_event_gnb_sync_new(const char *gnb_id, bool is_register);
dmf_event_t *dmf_event_gnb_dereg_new(const char *gnb_id);
dmf_event_t *dmf_event_print_gnb_list_new(void);

#ifdef __cplusplus
}
#endif

#endif /* DMF_EVENT_H */

