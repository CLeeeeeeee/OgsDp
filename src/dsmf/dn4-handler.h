#ifndef DSMF_DN4_HANDLER_H
#define DSMF_DN4_HANDLER_H

#include "ogs-pfcp.h"
#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

void dsmf_dn4_handle_message(ogs_pool_id_t xact_id, dsmf_event_t *e);
void dsmf_dn4_handle_timer(ogs_pool_id_t xact_id, dsmf_event_t *e);

/* 会话管理函数 */
void dsmf_dn4_handle_session_establishment(dsmf_sess_t *sess);
void dsmf_dn4_handle_session_modification(dsmf_sess_t *sess);
void dsmf_dn4_handle_session_deletion(dsmf_sess_t *sess);

#ifdef __cplusplus
}
#endif

#endif /* DSMF_DN4_HANDLER_H */
