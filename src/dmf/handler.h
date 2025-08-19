#ifndef DMF_HANDLER_H
#define DMF_HANDLER_H

#include "ogs-app.h"
#include "context.h"
#include "event.h"

void dmf_handle_report(const char *gnb_id, bool is_register);
void dmf_handle_gnb_sync(const char *gnb_id, bool is_register);
void dmf_handle_gnb_deregister(const char *gnb_id);
void dmf_handle_gnb_timer_expiry(const char *gnb_id);

#endif /* DMF_HANDLER_H */ 
