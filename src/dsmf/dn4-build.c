#include "dn4-build.h"
#include "ogs-core.h"

ogs_pkbuf_t *dsmf_dn4_build_session_establishment_request(dsmf_sess_t *sess)
{
    ogs_pfcp_message_t *pfcp_message = NULL;
    ogs_pfcp_session_establishment_request_t *req = NULL;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_pfcp_node_id_t node_id;
    ogs_pfcp_f_seid_t f_seid;
    int len, rv;
    
    ogs_assert(sess);
    
    ogs_info("Building PFCP Session Establishment Request for session [%s]", sess->session_ref);

    /* 创建 PFCP 消息 */
    pfcp_message = ogs_calloc(1, sizeof(*pfcp_message));
    if (!pfcp_message) {
        ogs_error("ogs_calloc() failed");
        return NULL;
    }

    /* 设置消息类型并获取指针 */
    pfcp_message->h.type = OGS_PFCP_SESSION_ESTABLISHMENT_REQUEST_TYPE;
    req = &pfcp_message->pfcp_session_establishment_request;
    
    /* Node ID */
    rv = ogs_pfcp_sockaddr_to_node_id(&node_id, &len);
    if (rv != OGS_OK) {
        ogs_error("ogs_pfcp_sockaddr_to_node_id() failed");
        ogs_free(pfcp_message);
        return NULL;
    }
    req->node_id.presence = 1;
    req->node_id.data = &node_id;
    req->node_id.len = len;
    
    /* F-SEID */
    rv = ogs_pfcp_sockaddr_to_f_seid(&f_seid, &len);
    if (rv != OGS_OK) {
        ogs_error("ogs_pfcp_sockaddr_to_f_seid() failed");
        ogs_free(pfcp_message);
        return NULL;
    }
    f_seid.seid = htobe64(sess->dsmf_dn4_seid);
    req->cp_f_seid.presence = 1;
    req->cp_f_seid.data = &f_seid;
    req->cp_f_seid.len = len;
    
    /* TODO: 添加 PDR、FAR 等规则 */
    
    /* 构建消息 */
    pkbuf = ogs_pfcp_build_msg(pfcp_message);
    if (!pkbuf) {
        ogs_error("ogs_pfcp_build_msg() failed");
        ogs_free(pfcp_message);
        return NULL;
    }

    ogs_free(pfcp_message);
    return pkbuf;
}

ogs_pkbuf_t *dsmf_dn4_build_session_modification_request(dsmf_sess_t *sess)
{
    ogs_pfcp_message_t *pfcp_message = NULL;
    ogs_pfcp_session_modification_request_t *req = NULL;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_pfcp_f_seid_t f_seid;
    int len, rv;

    ogs_assert(sess);
    
    ogs_info("Building PFCP Session Modification Request for session [%s]", sess->session_ref);

    /* 创建 PFCP 消息 */
    pfcp_message = ogs_calloc(1, sizeof(*pfcp_message));
    if (!pfcp_message) {
        ogs_error("ogs_calloc() failed");
        return NULL;
    }

    req = &pfcp_message->pfcp_session_modification_request;

    /* F-SEID */
    rv = ogs_pfcp_sockaddr_to_f_seid(&f_seid, &len);
    if (rv != OGS_OK) {
        ogs_error("ogs_pfcp_sockaddr_to_f_seid() failed");
        ogs_free(pfcp_message);
        return NULL;
    }
    f_seid.seid = htobe64(sess->dsmf_dn4_seid);
    req->cp_f_seid.presence = 1;
    req->cp_f_seid.data = &f_seid;
    req->cp_f_seid.len = len;
    
    /* TODO: 添加需要修改的规则 */
    
    /* 构建消息 */
    pkbuf = ogs_pfcp_build_msg(pfcp_message);
    if (!pkbuf) {
        ogs_error("ogs_pfcp_build_msg() failed");
        ogs_free(pfcp_message);
        return NULL;
    }

    ogs_free(pfcp_message);
    return pkbuf;
}

ogs_pkbuf_t *dsmf_dn4_build_session_deletion_request(dsmf_sess_t *sess)
{
    ogs_pfcp_message_t *pfcp_message = NULL;
    ogs_pkbuf_t *pkbuf = NULL;

    ogs_assert(sess);

    ogs_info("Building PFCP Session Deletion Request for session [%s]", sess->session_ref);
    
    /* 创建 PFCP 消息 */
    pfcp_message = ogs_calloc(1, sizeof(*pfcp_message));
    if (!pfcp_message) {
        ogs_error("ogs_calloc() failed");
        return NULL;
    }

    /* 设置消息类型 */
    pfcp_message->h.type = OGS_PFCP_SESSION_DELETION_REQUEST_TYPE;
    
    /* 构建消息 */
    pkbuf = ogs_pfcp_build_msg(pfcp_message);
    if (!pkbuf) {
        ogs_error("ogs_pfcp_build_msg() failed");
        ogs_free(pfcp_message);
        return NULL;
    }

    ogs_free(pfcp_message);
    return pkbuf;
}
