#include "context.h"
#include "dn4-handler.h"
#include "dn4-build.h"
#include "timer.h"
#include "ogs-core.h"

/* PFCP 定时器常量 */
#define DSMF_PFCP_ASSOCIATION_TIMEOUT     ogs_time_from_sec(10)
#define DSMF_PFCP_HEARTBEAT_TIMEOUT       ogs_time_from_sec(30)
#define DSMF_PFCP_ESTABLISHMENT_TIMEOUT   ogs_time_from_sec(10)
#define DSMF_PFCP_DELETION_TIMEOUT        ogs_time_from_sec(10)

static void session_timeout(ogs_pfcp_xact_t *xact, void *data)
{
    ogs_assert(xact);
    ogs_assert(data);
    
    ogs_warn("PFCP session timeout");
}

void dsmf_dn4_handle_message(ogs_pool_id_t xact_id, dsmf_event_t *e)
{
    ogs_pfcp_xact_t *xact = NULL;
    
    ogs_assert(e);
    
    if (xact_id != OGS_INVALID_POOL_ID) {
        xact = ogs_pfcp_xact_find_by_id(xact_id);
        if (!xact) {
            ogs_warn("PFCP transaction not found: %d", xact_id);
            return;
        }
    }
    
    if (e->pfcp_message) {
        ogs_info("Received PFCP message: type=%d", e->pfcp_message->h.type);
        
        /* 处理 PFCP 消息 */
        switch (e->pfcp_message->h.type) {
        case OGS_PFCP_ASSOCIATION_SETUP_REQUEST_TYPE:
            /* 处理关联建立请求 */
            break;
        case OGS_PFCP_ASSOCIATION_SETUP_RESPONSE_TYPE:
            /* 处理关联建立响应 */
            break;
        case OGS_PFCP_HEARTBEAT_REQUEST_TYPE:
            /* 处理心跳请求 */
            break;
        case OGS_PFCP_HEARTBEAT_RESPONSE_TYPE:
            /* 处理心跳响应 */
            break;
        case OGS_PFCP_SESSION_ESTABLISHMENT_RESPONSE_TYPE:
            /* 处理会话建立响应 */
            break;
        case OGS_PFCP_SESSION_MODIFICATION_RESPONSE_TYPE:
            /* 处理会话修改响应 */
            break;
        case OGS_PFCP_SESSION_DELETION_RESPONSE_TYPE:
            /* 处理会话删除响应 */
            break;
        default:
            ogs_warn("Unhandled PFCP message type: %d", e->pfcp_message->h.type);
            break;
        }
    }
    
    if (xact) {
        ogs_pfcp_xact_commit(xact);
    }
}

void dsmf_dn4_handle_timer(ogs_pool_id_t xact_id, dsmf_event_t *e)
{
    ogs_pfcp_xact_t *xact = NULL;
    
    ogs_assert(e);
    
    if (xact_id != OGS_INVALID_POOL_ID) {
        xact = ogs_pfcp_xact_find_by_id(xact_id);
        if (!xact) {
            ogs_warn("PFCP transaction not found: %d", xact_id);
            return;
        }
    }
    
    ogs_info("PFCP timer expired: timer_id=%d", e->h.timer_id);
    
    /* 处理定时器事件 */
    switch (e->h.timer_id) {
    case DSMF_TIMER_PFCP_ASSOCIATION:
        /* 处理关联超时 */
        break;
    case DSMF_TIMER_PFCP_NO_HEARTBEAT:
        /* 处理心跳超时 */
        break;
    case DSMF_TIMER_PFCP_NO_ESTABLISHMENT_RESPONSE:
        /* 处理建立响应超时 */
        break;
    case DSMF_TIMER_PFCP_NO_DELETION_RESPONSE:
        /* 处理删除响应超时 */
        break;
    default:
        ogs_warn("Unknown PFCP timer: %d", e->h.timer_id);
        break;
    }
    
    if (xact) {
        ogs_pfcp_xact_commit(xact);
    }
}

void dsmf_dn4_handle_session_establishment(dsmf_sess_t *sess)
{
    ogs_pfcp_xact_t *xact = NULL;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_pfcp_header_t h;
    int rv;
    
    ogs_assert(sess);
    ogs_assert(sess->pfcp_node);
    
    ogs_info("Sending PFCP Session Establishment Request for session [%s]", sess->session_ref);
    
    /* 创建 PFCP 事务 */
    xact = ogs_pfcp_xact_local_create(sess->pfcp_node, session_timeout, OGS_UINT_TO_POINTER(sess->id));
    if (!xact) {
        ogs_error("Failed to create PFCP transaction");
        return;
    }
    
    /* 设置事务参数 */
    xact->local_seid = sess->dsmf_dn4_seid;
    
    /* 构建 PFCP 消息头 */
    memset(&h, 0, sizeof(ogs_pfcp_header_t));
    h.type = OGS_PFCP_SESSION_ESTABLISHMENT_REQUEST_TYPE;
    h.seid = sess->dsmf_dn4_seid;
    
    /* 构建 PFCP 会话建立请求 */
    pkbuf = dsmf_dn4_build_session_establishment_request(sess);
    if (!pkbuf) {
        ogs_error("Failed to build PFCP Session Establishment Request");
        ogs_pfcp_xact_delete(xact);
        return;
    }
    
    /* 更新事务 */
    rv = ogs_pfcp_xact_update_tx(xact, &h, pkbuf);
    if (rv != OGS_OK) {
        ogs_error("ogs_pfcp_xact_update_tx() failed");
        ogs_pfcp_xact_delete(xact);
        return;
    }
    
    /* 发送请求 */
    rv = ogs_pfcp_xact_commit(xact);
    if (rv != OGS_OK) {
        ogs_error("Failed to send PFCP Session Establishment Request");
        return;
    }
    
    ogs_info("PFCP Session Establishment Request sent successfully");
}

void dsmf_dn4_handle_session_modification(dsmf_sess_t *sess)
{
    ogs_pfcp_xact_t *xact = NULL;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_pfcp_header_t h;
    int rv;
    
    ogs_assert(sess);
    ogs_assert(sess->pfcp_node);
    
    ogs_info("Sending PFCP Session Modification Request for session [%s]", sess->session_ref);
    
    /* 创建 PFCP 事务 */
    xact = ogs_pfcp_xact_local_create(sess->pfcp_node, session_timeout, OGS_UINT_TO_POINTER(sess->id));
    if (!xact) {
        ogs_error("Failed to create PFCP transaction");
        return;
    }
    
    /* 设置事务参数 */
    xact->local_seid = sess->dsmf_dn4_seid;
    
    /* 构建 PFCP 消息头 */
    memset(&h, 0, sizeof(ogs_pfcp_header_t));
    h.type = OGS_PFCP_SESSION_MODIFICATION_REQUEST_TYPE;
    h.seid = sess->dsmf_dn4_seid;
    
    /* 构建 PFCP 会话修改请求 */
    pkbuf = dsmf_dn4_build_session_modification_request(sess);
    if (!pkbuf) {
        ogs_error("Failed to build PFCP Session Modification Request");
        ogs_pfcp_xact_delete(xact);
        return;
    }
    
    /* 更新事务 */
    rv = ogs_pfcp_xact_update_tx(xact, &h, pkbuf);
    if (rv != OGS_OK) {
        ogs_error("ogs_pfcp_xact_update_tx() failed");
        ogs_pfcp_xact_delete(xact);
        return;
    }
    
    /* 发送请求 */
    rv = ogs_pfcp_xact_commit(xact);
    if (rv != OGS_OK) {
        ogs_error("Failed to send PFCP Session Modification Request");
        return;
    }
    
    ogs_info("PFCP Session Modification Request sent successfully");
}

void dsmf_dn4_handle_session_deletion(dsmf_sess_t *sess)
{
    ogs_pfcp_xact_t *xact = NULL;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_pfcp_header_t h;
    int rv;
    
    ogs_assert(sess);
    ogs_assert(sess->pfcp_node);
    
    ogs_info("Sending PFCP Session Deletion Request for session [%s]", sess->session_ref);
    
    /* 创建 PFCP 事务 */
    xact = ogs_pfcp_xact_local_create(sess->pfcp_node, session_timeout, OGS_UINT_TO_POINTER(sess->id));
    if (!xact) {
        ogs_error("Failed to create PFCP transaction");
        return;
    }
    
    /* 设置事务参数 */
    xact->local_seid = sess->dsmf_dn4_seid;
    
    /* 构建 PFCP 消息头 */
    memset(&h, 0, sizeof(ogs_pfcp_header_t));
    h.type = OGS_PFCP_SESSION_DELETION_REQUEST_TYPE;
    h.seid = sess->dsmf_dn4_seid;
    
    /* 构建 PFCP 会话删除请求 */
    pkbuf = dsmf_dn4_build_session_deletion_request(sess);
    if (!pkbuf) {
        ogs_error("Failed to build PFCP Session Deletion Request");
        ogs_pfcp_xact_delete(xact);
        return;
    }
    
    /* 更新事务 */
    rv = ogs_pfcp_xact_update_tx(xact, &h, pkbuf);
    if (rv != OGS_OK) {
        ogs_error("ogs_pfcp_xact_update_tx() failed");
        ogs_pfcp_xact_delete(xact);
        return;
    }
    
    /* 发送请求 */
    rv = ogs_pfcp_xact_commit(xact);
    if (rv != OGS_OK) {
        ogs_error("Failed to send PFCP Session Deletion Request");
        return;
    }
    
    ogs_info("PFCP Session Deletion Request sent successfully");
}
