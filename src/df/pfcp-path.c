/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 df 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: pfcp-path.c
 * 组件: df
 * 添加时间: 2025年 08月 20日 星期三 11:16:03 CST
 */

#include "context.h"
#include "event.h"
#include "ndf-handler.h"
#include "pfcp-path.h"
#include "ndf-build.h"

// 事件、定时器、状态机等可根据需要补充

static void pfcp_node_fsm_init(ogs_pfcp_node_t *node, bool try_to_associate)
{
    df_event_t e;

    ogs_assert(node);

    memset(&e, 0, sizeof(e));
    e.pfcp_node = node;

    if (try_to_associate == true) {
        /* 暂时注释掉 timer 相关代码 */
        /*
        node->t_association = ogs_timer_add(ogs_app()->timer_mgr,
                df_timer_association, node);
        ogs_assert(node->t_association);
        */
    }

    ogs_fsm_init(&node->sm, df_pfcp_state_initial, df_pfcp_state_final, &e);
}

static void pfcp_node_fsm_fini(ogs_pfcp_node_t *node)
{
    df_event_t e;

    ogs_assert(node);

    memset(&e, 0, sizeof(e));
    e.pfcp_node = node;

    ogs_fsm_fini(&node->sm, &e);

    if (node->t_association)
        ogs_timer_delete(node->t_association);
}

static void pfcp_recv_cb(short when, ogs_socket_t fd, void *data)
{
    int rv;

    df_event_t *e = NULL;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_sockaddr_t from;
    ogs_pfcp_node_t *node = NULL;
    ogs_pfcp_message_t *message = NULL;

    ogs_pfcp_status_e pfcp_status;
    ogs_pfcp_node_id_t node_id;

    ogs_assert(fd != INVALID_SOCKET);

    pkbuf = ogs_pfcp_recvfrom(fd, &from);
    if (!pkbuf) {
        ogs_error("ogs_pfcp_recvfrom() failed");
        return;
    }

    e = df_event_new(DF_EVT_PFCP_MESSAGE);
    ogs_assert(e);

    if ((message = ogs_pfcp_parse_msg(pkbuf)) == NULL) {
        ogs_error("ogs_pfcp_parse_msg() failed");
        ogs_pkbuf_free(pkbuf);
        df_event_free(e);
        return;
    }

    pfcp_status = ogs_pfcp_extract_node_id(message, &node_id);
    switch (pfcp_status) {
    case OGS_PFCP_STATUS_SUCCESS:
    case OGS_PFCP_STATUS_NODE_ID_NONE:
    case OGS_PFCP_STATUS_NODE_ID_OPTIONAL_ABSENT:
        ogs_debug("ogs_pfcp_extract_node_id() "
                "type [%d] pfcp_status [%d] node_id [%s] from %s",
                message->h.type, pfcp_status,
                pfcp_status == OGS_PFCP_STATUS_SUCCESS ?
                    ogs_pfcp_node_id_to_string_static(&node_id) :
                    "NULL",
                ogs_sockaddr_to_string_static(&from));
        break;

    case OGS_PFCP_ERROR_SEMANTIC_INCORRECT_MESSAGE:
    case OGS_PFCP_ERROR_NODE_ID_NOT_PRESENT:
    case OGS_PFCP_ERROR_NODE_ID_NOT_FOUND:
    case OGS_PFCP_ERROR_UNKNOWN_MESSAGE:
        ogs_error("ogs_pfcp_extract_node_id() failed "
                "type [%d] pfcp_status [%d] from %s",
                message->h.type, pfcp_status,
                ogs_sockaddr_to_string_static(&from));
        goto cleanup;

    default:
        ogs_error("Unexpected pfcp_status "
                "type [%d] pfcp_status [%d] from %s",
                message->h.type, pfcp_status,
                ogs_sockaddr_to_string_static(&from));
        goto cleanup;
    }

    node = ogs_pfcp_node_find(&ogs_pfcp_self()->pfcp_peer_list,
            pfcp_status == OGS_PFCP_STATUS_SUCCESS ? &node_id : NULL, &from);
    if (!node) {
        if (message->h.type == OGS_PFCP_ASSOCIATION_SETUP_REQUEST_TYPE ||
            message->h.type == OGS_PFCP_ASSOCIATION_SETUP_RESPONSE_TYPE) {
            ogs_assert(pfcp_status == OGS_PFCP_STATUS_SUCCESS);
            node = ogs_pfcp_node_add(&ogs_pfcp_self()->pfcp_peer_list,
                    &node_id, &from);
            if (!node) {
                ogs_error("No memory: ogs_pfcp_node_add() failed");
                goto cleanup;
            }
            ogs_debug("Added PFCP-Node: addr_list %s",
                    ogs_sockaddr_to_string_static(node->addr_list));

            pfcp_node_fsm_init(node, false);

        } else {
            ogs_error("Cannot find PFCP-Node: type [%d] node_id %s from %s",
                    message->h.type,
                    pfcp_status == OGS_PFCP_STATUS_SUCCESS ?
                        ogs_pfcp_node_id_to_string_static(&node_id) :
                        "NULL",
                    ogs_sockaddr_to_string_static(&from));
            goto cleanup;
        }
    } else {
        ogs_debug("Found PFCP-Node: addr_list %s",
                ogs_sockaddr_to_string_static(node->addr_list));
        ogs_expect(OGS_OK == ogs_pfcp_node_merge(
                    node,
                    pfcp_status == OGS_PFCP_STATUS_SUCCESS ?  &node_id : NULL,
                    &from));
        ogs_debug("Merged PFCP-Node: addr_list %s",
                ogs_sockaddr_to_string_static(node->addr_list));
    }

    e->pfcp_node = node;
    e->pkbuf = pkbuf;
    e->pfcp_message = message;

    rv = ogs_queue_push(ogs_app()->queue, e);
    if (rv != OGS_OK) {
        ogs_error("ogs_queue_push() failed:%d", (int)rv);
        goto cleanup;
    }

    return;

cleanup:
    ogs_pkbuf_free(pkbuf);
    ogs_pfcp_message_free(message);
    df_event_free(e);
}

int df_pfcp_open(void)
{
    ogs_socknode_t *node = NULL;
    ogs_sock_t *sock = NULL;

    /* PFCP Server */
    ogs_list_for_each(&ogs_pfcp_self()->pfcp_list, node) {
        sock = ogs_pfcp_server(node);
        if (!sock) return OGS_ERROR;

        node->poll = ogs_pollset_add(ogs_app()->pollset,
                OGS_POLLIN, sock->fd, pfcp_recv_cb, sock);
        ogs_assert(node->poll);
    }
    ogs_list_for_each(&ogs_pfcp_self()->pfcp_list6, node) {
        sock = ogs_pfcp_server(node);
        if (!sock) return OGS_ERROR;

        node->poll = ogs_pollset_add(ogs_app()->pollset,
                OGS_POLLIN, sock->fd, pfcp_recv_cb, sock);
        ogs_assert(node->poll);
    }

    /* 与 UPF 一致，统一设置 PFCP 服务器资源 */
    OGS_SETUP_PFCP_SERVER;

    return OGS_OK;
}

void df_pfcp_close(void)
{
    ogs_pfcp_node_t *pfcp_node = NULL;

    ogs_list_for_each(&ogs_pfcp_self()->pfcp_peer_list, pfcp_node)
        pfcp_node_fsm_fini(pfcp_node);

    ogs_freeaddrinfo(ogs_pfcp_self()->pfcp_advertise);
    ogs_freeaddrinfo(ogs_pfcp_self()->pfcp_advertise6);

    ogs_socknode_remove_all(&ogs_pfcp_self()->pfcp_list);
    ogs_socknode_remove_all(&ogs_pfcp_self()->pfcp_list6);
}

int df_pfcp_send_session_establishment_response(
        ogs_pfcp_xact_t *xact, df_sess_t *sess,
        ogs_pfcp_pdr_t *created_pdr[], int num_of_created_pdr)
{
    int rv;
    ogs_pkbuf_t *n4buf = NULL;
    ogs_pfcp_header_t h;

    ogs_assert(xact);

    memset(&h, 0, sizeof(ogs_pfcp_header_t));
    h.type = OGS_PFCP_SESSION_ESTABLISHMENT_RESPONSE_TYPE;
    h.seid = sess->dsmf_n4_f_seid.seid;

    n4buf = df_n4_build_session_establishment_response(
            h.type, sess, created_pdr, num_of_created_pdr);
    if (!n4buf) {
        ogs_error("df_n4_build_session_establishment_response() failed");
        return OGS_ERROR;
    }

    rv = ogs_pfcp_xact_update_tx(xact, &h, n4buf);
    if (rv != OGS_OK) {
        ogs_error("ogs_pfcp_xact_update_tx() failed");
        return OGS_ERROR;
    }

    rv = ogs_pfcp_xact_commit(xact);
    ogs_expect(rv == OGS_OK);

    return rv;
}

int df_pfcp_send_session_modification_response(
        ogs_pfcp_xact_t *xact, df_sess_t *sess,
        ogs_pfcp_pdr_t *created_pdr[], int num_of_created_pdr)
{
    int rv;
    ogs_pkbuf_t *n4buf = NULL;
    ogs_pfcp_header_t h;

    ogs_assert(xact);

    memset(&h, 0, sizeof(ogs_pfcp_header_t));
    h.type = OGS_PFCP_SESSION_MODIFICATION_RESPONSE_TYPE;
    h.seid = sess->dsmf_n4_f_seid.seid;

    n4buf = df_n4_build_session_modification_response(
            h.type, sess, created_pdr, num_of_created_pdr);
    if (!n4buf) {
        ogs_error("df_n4_build_session_modification_response() failed");
        return OGS_ERROR;
    }

    rv = ogs_pfcp_xact_update_tx(xact, &h, n4buf);
    if (rv != OGS_OK) {
        ogs_error("ogs_pfcp_xact_update_tx() failed");
        return OGS_ERROR;
    }

    rv = ogs_pfcp_xact_commit(xact);
    ogs_expect(rv == OGS_OK);

    return rv;
}

int df_pfcp_send_session_deletion_response(ogs_pfcp_xact_t *xact,
        df_sess_t *sess)
{
    int rv;
    ogs_pkbuf_t *n4buf = NULL;
    ogs_pfcp_header_t h;

    ogs_assert(xact);

    memset(&h, 0, sizeof(ogs_pfcp_header_t));
    h.type = OGS_PFCP_SESSION_DELETION_RESPONSE_TYPE;
    h.seid = sess->dsmf_n4_f_seid.seid;

    n4buf = df_n4_build_session_deletion_response(h.type, sess);
    if (!n4buf) {
        ogs_error("df_n4_build_session_deletion_response() failed");
        return OGS_ERROR;
    }

    rv = ogs_pfcp_xact_update_tx(xact, &h, n4buf);
    if (rv != OGS_OK) {
        ogs_error("ogs_pfcp_xact_update_tx() failed");
        return OGS_ERROR;
    }

    rv = ogs_pfcp_xact_commit(xact);
    ogs_expect(rv == OGS_OK);

    return rv;
}

static void sess_timeout(ogs_pfcp_xact_t *xact, void *data)
{
    df_sess_t *sess = NULL;
    ogs_pool_id_t sess_id = OGS_INVALID_POOL_ID;
    uint8_t type;

    ogs_assert(xact);
    type = xact->seq[0].type;

    ogs_assert(data);
    sess_id = OGS_POINTER_TO_UINT(data);
    ogs_assert(sess_id >= OGS_MIN_POOL_ID && sess_id <= OGS_MAX_POOL_ID);

    sess = df_sess_find_by_id(sess_id);
    if (!sess) {
        ogs_error("Session has already been removed [%d]", type);
        return;
    }

    switch (type) {
    case OGS_PFCP_SESSION_REPORT_REQUEST_TYPE:
        ogs_error("No PFCP session report response");
        break;
    default:
        ogs_error("Not implemented [type:%d]", type);
        break;
    }
}

int df_pfcp_send_session_report_request(
        df_sess_t *sess, ogs_pfcp_user_plane_report_t *report)
{
    int rv;
    ogs_pkbuf_t *n4buf = NULL;
    ogs_pfcp_header_t h;
    ogs_pfcp_xact_t *xact = NULL;

    // TODO: df_metrics_inst_global_inc(DF_METR_GLOB_CTR_SM_N4SESSIONREPORT);

    ogs_assert(sess);
    ogs_assert(report);

    memset(&h, 0, sizeof(ogs_pfcp_header_t));
    h.type = OGS_PFCP_SESSION_REPORT_REQUEST_TYPE;
    h.seid = sess->dsmf_n4_f_seid.seid;

    xact = ogs_pfcp_xact_local_create(
            sess->pfcp_node, sess_timeout, OGS_UINT_TO_POINTER(sess->id));
    if (!xact) {
        ogs_error("ogs_pfcp_xact_local_create() failed");
        return OGS_ERROR;
    }

    n4buf = ogs_pfcp_build_session_report_request(h.type, report);
    if (!n4buf) {
        ogs_error("ogs_pfcp_build_session_report_request() failed");
        return OGS_ERROR;
    }

    rv = ogs_pfcp_xact_update_tx(xact, &h, n4buf);
    if (rv != OGS_OK) {
        ogs_error("ogs_pfcp_xact_update_tx() failed");
        return OGS_ERROR;
    }

    rv = ogs_pfcp_xact_commit(xact);
    ogs_expect(rv == OGS_OK);

    return rv;
}

void df_pfcp_handle_message(ogs_pfcp_node_t *pfcp_node, ogs_pfcp_message_t *message)
{
    ogs_assert(pfcp_node);
    ogs_assert(message);

    ogs_info("Handling PFCP message: type=%d", message->h.type);

    /* 为入站消息创建/查找事务 */
    {
        int rv;
        ogs_pfcp_xact_t *xact = NULL;
        rv = ogs_pfcp_xact_receive(pfcp_node, &message->h, &xact);
        if (rv == OGS_RETRY) {
            /* 重传场景：事务层已处理，静默返回 */
            return;
        } else if (rv != OGS_OK) {
            ogs_error("ogs_pfcp_xact_receive() failed for type=%d", message->h.type);
            return;
        }

        switch (message->h.type) {
        case OGS_PFCP_HEARTBEAT_REQUEST_TYPE:
            ogs_pfcp_handle_heartbeat_request(pfcp_node, xact, &message->pfcp_heartbeat_request);
            break;
        case OGS_PFCP_HEARTBEAT_RESPONSE_TYPE:
            ogs_pfcp_handle_heartbeat_response(pfcp_node, xact, &message->pfcp_heartbeat_response);
            break;
        case OGS_PFCP_ASSOCIATION_SETUP_REQUEST_TYPE:
            ogs_pfcp_up_handle_association_setup_request(pfcp_node, xact, &message->pfcp_association_setup_request);
            ogs_info("PFCP Association Setup handled, response should be sent");
            break;
        case OGS_PFCP_ASSOCIATION_SETUP_RESPONSE_TYPE:
            ogs_pfcp_up_handle_association_setup_response(pfcp_node, xact, &message->pfcp_association_setup_response);
            break;
        case OGS_PFCP_SESSION_ESTABLISHMENT_REQUEST_TYPE:
            {
                df_sess_t *sess = df_sess_add_by_message(message);
                if (sess) {
                    OGS_SETUP_PFCP_NODE(sess, pfcp_node);
                    df_n4_handle_session_establishment_request(sess, xact, &message->pfcp_session_establishment_request);
                }
            }
            break;
        case OGS_PFCP_SESSION_MODIFICATION_REQUEST_TYPE:
            {
                df_sess_t *sess = NULL;
                if (message->h.seid_presence && message->h.seid != 0)
                    sess = df_sess_find_by_df_n4_seid(message->h.seid);
                if (sess) {
                    df_n4_handle_session_modification_request(sess, xact, &message->pfcp_session_modification_request);
                }
            }
            break;
        case OGS_PFCP_SESSION_DELETION_REQUEST_TYPE:
            {
                df_sess_t *sess = NULL;
                if (message->h.seid_presence && message->h.seid != 0)
                    sess = df_sess_find_by_df_n4_seid(message->h.seid);
                if (sess) {
                    df_n4_handle_session_deletion_request(sess, xact, &message->pfcp_session_deletion_request);
                }
            }
            break;
        default:
            ogs_warn("Unhandled PFCP message type: %d", message->h.type);
            break;
        }
    }
} 