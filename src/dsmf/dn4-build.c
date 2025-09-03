/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dsmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: dn4-build.c
 * 组件: dsmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:09 CST
 */

#include "dn4-build.h"
#include "ogs-core.h"

ogs_pkbuf_t *dsmf_dn4_build_session_establishment_request(dsmf_sess_t *sess)
{
    ogs_pfcp_message_t *pfcp_message = NULL;
    ogs_pfcp_session_establishment_request_t *req = NULL;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_pfcp_pdr_t *ul_pdr = NULL;
    ogs_pfcp_far_t *ul_far = NULL;
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
    
    /* 构建最小可用的 UL PDR 和 FAR，使 DF 能解析到 RAN F-TEID */
    ogs_pfcp_pdrbuf_init();

    /* 确保 session 的 PFCP BAR/QER 初始化，以避免构建时空指针 */
    /* 若需要 BAR，可按需添加；当前最小流不强制 BAR，避免依赖未公开 API */

    /* FAR: 上行转发到 CORE */
    ul_far = ogs_pfcp_far_add(&sess->pfcp);
    if (!ul_far) {
        ogs_error("ogs_pfcp_far_add() failed");
        ogs_free(pfcp_message);
        return NULL;
    }
    ul_far->dst_if = OGS_PFCP_INTERFACE_CORE;
    ul_far->apply_action = OGS_PFCP_APPLY_ACTION_FORW;

    /* PDR: 来源 ACCESS，携带 Local F-TEID=RAN addr/teid */
    ul_pdr = ogs_pfcp_pdr_add(&sess->pfcp);
    if (!ul_pdr) {
        ogs_error("ogs_pfcp_pdr_add() failed");
        ogs_free(pfcp_message);
        return NULL;
    }
    ul_pdr->src_if = OGS_PFCP_INTERFACE_ACCESS;
    ul_pdr->src_if_type_presence = true;
    ul_pdr->src_if_type = OGS_PFCP_3GPP_INTERFACE_TYPE_N3_3GPP_ACCESS;
    /* 设置外层头去除（上行GTP-U包） */
    ul_pdr->outer_header_removal_len = 1;
    if (sess->ran_addr && sess->ran_addr->ogs_sa_family == AF_INET) {
        ul_pdr->outer_header_removal.description =
            OGS_PFCP_OUTER_HEADER_REMOVAL_GTPU_UDP_IPV4;
    } else if (sess->ran_addr && sess->ran_addr->ogs_sa_family == AF_INET6) {
        ul_pdr->outer_header_removal.description =
            OGS_PFCP_OUTER_HEADER_REMOVAL_GTPU_UDP_IPV6;
    } else {
        /* 默认按 IPv4 处理 */
        ul_pdr->outer_header_removal.description =
            OGS_PFCP_OUTER_HEADER_REMOVAL_GTPU_UDP_IPV4;
    }

    /* 由 RAN 地址构造 F-TEID，并覆盖 teid 为会话级 RAN TEID */
    {
        ogs_sockaddr_t *ipv4 = NULL, *ipv6 = NULL;
        if (sess->ran_addr) {
            if (sess->ran_addr->ogs_sa_family == AF_INET) {
                ipv4 = sess->ran_addr;
                char *ip = ogs_ipstrdup(sess->ran_addr);
                ogs_info("[DSMF] Setting RAN addr in F-TEID: %s, TEID: 0x%x", ip ? ip : "NULL", sess->ran_teid);
                ogs_info("[DSMF] sess->ran_addr->sin.sin_addr.s_addr = 0x%x", sess->ran_addr->sin.sin_addr.s_addr);
                if (ip) ogs_free(ip);
            } else if (sess->ran_addr->ogs_sa_family == AF_INET6) {
                ipv6 = sess->ran_addr;
                char *ip = ogs_ipstrdup(sess->ran_addr);
                ogs_info("[DSMF] Setting RAN addr in F-TEID: %s, TEID: 0x%x", ip ? ip : "NULL", sess->ran_teid);
                if (ip) ogs_free(ip);
            }
        } else {
            ogs_error("[DSMF] sess->ran_addr is NULL!");
        }
        rv = ogs_pfcp_sockaddr_to_f_teid(ipv4, ipv6, &ul_pdr->f_teid, &ul_pdr->f_teid_len);
        if (rv != OGS_OK) {
            ogs_error("ogs_pfcp_sockaddr_to_f_teid() failed");
            ogs_free(pfcp_message);
            return NULL;
        }
        ul_pdr->f_teid.teid = sess->ran_teid;
        
        /* 验证 F-TEID 设置 */
        if (ul_pdr->f_teid.ipv4) {
            char *ip = ogs_ipv4_to_string(ul_pdr->f_teid.addr);
            ogs_info("[DSMF] F-TEID set to: %s, TEID: 0x%x", ip ? ip : "NULL", ul_pdr->f_teid.teid);
            ogs_info("[DSMF] ul_pdr->f_teid.addr = 0x%x", ul_pdr->f_teid.addr);
            if (ip) ogs_free(ip);
        }
    }

    /* 关联 FAR */
    ogs_pfcp_pdr_associate_far(ul_pdr, ul_far);

    /* 将规则写入 PFCP 请求 */
    ogs_pfcp_build_create_pdr(&req->create_pdr[0], 0, ul_pdr);
    ogs_pfcp_build_create_far(&req->create_far[0], 0, ul_far);

    /* 明确填写 PDN-Type 为 IPv4，避免 DF 侧 UE IP 配置依赖缺失 */
    req->pdn_type.presence = 1;
    req->pdn_type.u8 = OGS_PDU_SESSION_TYPE_IPV4;
    
    /* 构建消息 */
    pkbuf = ogs_pfcp_build_msg(pfcp_message);
    if (!pkbuf) {
        ogs_error("ogs_pfcp_build_msg() failed");
        ogs_free(pfcp_message);
        return NULL;
    }

    ogs_pfcp_pdrbuf_clear();
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
