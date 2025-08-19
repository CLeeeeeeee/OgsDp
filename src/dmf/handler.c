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
