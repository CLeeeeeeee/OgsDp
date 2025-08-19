#ifndef DSMF_TIMER_H
#define DSMF_TIMER_H

#include "ogs-proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* forward declaration */
typedef enum {
    DSMF_TIMER_BASE = OGS_MAX_NUM_OF_PROTO_TIMER,

    /* PFCP 相关定时器 */
    DSMF_TIMER_PFCP_ASSOCIATION,
    DSMF_TIMER_PFCP_NO_HEARTBEAT,
    DSMF_TIMER_PFCP_NO_ESTABLISHMENT_RESPONSE,
    DSMF_TIMER_PFCP_NO_DELETION_RESPONSE,

    /* 会话管理相关定时器 */
    DSMF_TIMER_SESSION_ESTABLISHMENT,
    DSMF_TIMER_SESSION_MODIFICATION,
    DSMF_TIMER_SESSION_DELETION,
    DSMF_TIMER_RAN_SYNC,

    MAX_NUM_OF_DSMF_TIMER,

} dsmf_timer_e;

const char *dsmf_timer_get_name(int timer_id);

/* PFCP 定时器函数 */
void dsmf_timer_pfcp_association(void *data);
void dsmf_timer_pfcp_no_heartbeat(void *data);

/* 会话管理定时器函数 */
void dsmf_timer_session_establishment(void *data);
void dsmf_timer_session_modification(void *data);
void dsmf_timer_session_deletion(void *data);
void dsmf_timer_ran_sync(void *data);

#ifdef __cplusplus
}
#endif

#endif /* DSMF_TIMER_H */ 