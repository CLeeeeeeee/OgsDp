//
// Created by 75635 on 2025/6/20.
//

#include "event.h"
#include "context.h"

dmf_event_t *dmf_event_new(int id)
{
    dmf_event_t *e = NULL;

    e = ogs_event_size(id, sizeof(dmf_event_t));
    ogs_assert(e);

    e->h.id = id;

    return e;
}

const char *dmf_event_get_name(const dmf_event_t *e)
{
    if (e == NULL) {
        return OGS_FSM_NAME_INIT_SIG;
    }

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        return OGS_FSM_NAME_ENTRY_SIG;
    case OGS_FSM_EXIT_SIG:
        return OGS_FSM_NAME_EXIT_SIG;

    case OGS_EVENT_SBI_SERVER:
        return OGS_EVENT_NAME_SBI_SERVER;
    case OGS_EVENT_SBI_CLIENT:
        return OGS_EVENT_NAME_SBI_CLIENT;
    case OGS_EVENT_SBI_TIMER:
        return OGS_EVENT_NAME_SBI_TIMER;

    case DMF_EVENT_GNB_SYNC:
        return "DMF_EVENT_GNB_SYNC";
    case DMF_EVENT_GNB_DEREG:
        return "DMF_EVENT_GNB_DEREG";
    case DMF_EVENT_PRINT_GNB_LIST:
        return "DMF_EVENT_PRINT_GNB_LIST";

    default:
        break;
    }

    ogs_error("Unknown Event[%d]", e->h.id);
    return "UNKNOWN_EVENT";
}

// 创建gNB同步事件
dmf_event_t *dmf_event_gnb_sync_new(const char *gnb_id, bool is_register)
{
    dmf_event_t *e = NULL;
    
    e = dmf_event_new(DMF_EVENT_GNB_SYNC);
    ogs_assert(e);
    
    strncpy(e->gnb_sync.gnb_id, gnb_id, sizeof(e->gnb_sync.gnb_id) - 1);
    e->gnb_sync.gnb_id[sizeof(e->gnb_sync.gnb_id) - 1] = '\0';
    e->gnb_sync.is_register = is_register;
    
    return e;
}

// 创建gNB注销事件
dmf_event_t *dmf_event_gnb_dereg_new(const char *gnb_id)
{
    dmf_event_t *e = NULL;
    
    e = dmf_event_new(DMF_EVENT_GNB_DEREG);
    ogs_assert(e);
    
    strncpy(e->gnb_dereg.gnb_id, gnb_id, sizeof(e->gnb_dereg.gnb_id) - 1);
    e->gnb_dereg.gnb_id[sizeof(e->gnb_dereg.gnb_id) - 1] = '\0';
    
    return e;
}

// 创建打印gNB列表事件
dmf_event_t *dmf_event_print_gnb_list_new(void)
{
    dmf_event_t *e = NULL;
    
    e = dmf_event_new(DMF_EVENT_PRINT_GNB_LIST);
    ogs_assert(e);
    
    return e;
}


