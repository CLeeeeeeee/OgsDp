#ifndef DMF_EVENT_H
#define DMF_EVENT_H

#include "ogs-app.h"
#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DMF_EVENT_BASE = OGS_MAX_NUM_OF_PROTO_EVENT,

    DMF_EVENT_SBI_SERVER,
    DMF_EVENT_SBI_CLIENT,
    DMF_EVENT_SBI_TIMER,

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

