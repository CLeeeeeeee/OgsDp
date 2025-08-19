#ifndef DSMF_EVENT_H
#define DSMF_EVENT_H

#include "ogs-proto.h"
#include "ogs-sbi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ogs_pfcp_node_s ogs_pfcp_node_t;
typedef struct ogs_pfcp_xact_s ogs_pfcp_xact_t;
typedef struct ogs_pfcp_message_s ogs_pfcp_message_t;
typedef struct dsmf_sess_s dsmf_sess_t;
typedef struct dsmf_df_s dsmf_df_t;

typedef enum {
    DSMF_EVT_BASE = OGS_MAX_NUM_OF_PROTO_EVENT,

    /* DN4 (PFCP) 相关事件 */
    DSMF_EVT_DN4_MESSAGE,
    DSMF_EVT_DN4_TIMER,
    DSMF_EVT_DN4_NO_HEARTBEAT,

    /* SBI 相关事件 */
    DSMF_EVT_SBI_MESSAGE,
    DSMF_EVT_SBI_TIMER,

    /* 会话管理相关事件 */
    DSMF_EVT_SESSION_ESTABLISHMENT_REQUEST,
    DSMF_EVT_SESSION_MODIFICATION_REQUEST,
    DSMF_EVT_SESSION_DELETION_REQUEST,
    DSMF_EVT_SESSION_REPORT_REQUEST,

    /* RAN 同步相关事件 */
    DSMF_EVT_RAN_SYNC_REQUEST,
    DSMF_EVT_RAN_SYNC_RESPONSE,

    DSMF_EVT_TOP,

} dsmf_event_e;

typedef struct dsmf_event_s {
    ogs_event_t h;

    ogs_pkbuf_t *pkbuf;

    /* PFCP 相关字段 */
    ogs_pfcp_node_t *pfcp_node;
    ogs_pool_id_t pfcp_xact_id;
    ogs_pfcp_message_t *pfcp_message;

    /* SBI 相关字段 */
    struct {
        ogs_sbi_request_t *request;
        ogs_sbi_response_t *response;
        ogs_sbi_message_t message;
        ogs_sbi_stream_t *stream;
        ogs_pool_id_t stream_id;
    } sbi;

    /* RAN 同步相关字段 */
    struct {
        const char *gnb_id;
        const char *session_id;
        const char *ran_addr_str;
        uint16_t ran_port;
        uint32_t teid;
        ogs_sockaddr_t *ran_addr;
    } ran_sync;

    /* 会话相关字段 */
    ogs_pool_id_t sess_id;

} dsmf_event_t;

dsmf_event_t *dsmf_event_new(int id);
const char *dsmf_event_get_name(dsmf_event_t *e);
void dsmf_event_free(dsmf_event_t *e);

#ifdef __cplusplus
}
#endif

#endif /* DSMF_EVENT_H */
