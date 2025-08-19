#ifndef DSMF_SM_H
#define DSMF_SM_H

#include "event.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 主状态机 */
void dsmf_state_initial(ogs_fsm_t *s, dsmf_event_t *e);
void dsmf_state_final(ogs_fsm_t *s, dsmf_event_t *e);
void dsmf_state_operational(ogs_fsm_t *s, dsmf_event_t *e);
void dsmf_state_exception(ogs_fsm_t *s, dsmf_event_t *e);

/* 会话状态机 */
void dsmf_session_state_initial(ogs_fsm_t *s, dsmf_event_t *e);
void dsmf_session_state_final(ogs_fsm_t *s, dsmf_event_t *e);
void dsmf_session_state_wait_ran_sync(ogs_fsm_t *s, dsmf_event_t *e);
void dsmf_session_state_wait_pfcp_establishment(ogs_fsm_t *s, dsmf_event_t *e);
void dsmf_session_state_operational(ogs_fsm_t *s, dsmf_event_t *e);
void dsmf_session_state_wait_pfcp_deletion(ogs_fsm_t *s, dsmf_event_t *e);
void dsmf_session_state_exception(ogs_fsm_t *s, dsmf_event_t *e);

/* PFCP 状态机 */
void dsmf_pfcp_state_initial(ogs_fsm_t *s, dsmf_event_t *e);
void dsmf_pfcp_state_final(ogs_fsm_t *s, dsmf_event_t *e);
void dsmf_pfcp_state_will_associate(ogs_fsm_t *s, dsmf_event_t *e);
void dsmf_pfcp_state_associated(ogs_fsm_t *s, dsmf_event_t *e);
void dsmf_pfcp_state_exception(ogs_fsm_t *s, dsmf_event_t *e);

#define dsmf_sm_debug(__pe) \
    ogs_debug("%s(): %s", __func__, dsmf_event_get_name(__pe))

#ifdef __cplusplus
}
#endif

#endif /* DSMF_SM_H */ 