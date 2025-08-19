#include "sbi-path.h"
#include "pfcp-path.h"
#include "dn4-build.h"
#include "dn4-handler.h"
#include "timer.h"
#include "dsmf-sm.h"

static void pfcp_node_fsm_init(ogs_pfcp_node_t *node, bool try_to_associate)
{
    dsmf_event_t e;

    ogs_assert(node);

    memset(&e, 0, sizeof(e));
    e.pfcp_node = node;

    if (try_to_associate == true) {
        node->t_association = ogs_timer_add(ogs_app()->timer_mgr,
                dsmf_timer_pfcp_association, node);
        ogs_assert(node->t_association);
    }

    ogs_fsm_init(&node->sm, dsmf_pfcp_state_initial, dsmf_pfcp_state_final, &e);
}

static void pfcp_node_fsm_fini(ogs_pfcp_node_t *node)
{
    dsmf_event_t e;

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
    dsmf_event_t *e = NULL;
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

    e = dsmf_event_new(DSMF_EVT_DN4_MESSAGE);
    ogs_assert(e);

    if ((message = ogs_pfcp_parse_msg(pkbuf)) == NULL) {
        ogs_error("ogs_pfcp_parse_msg() failed");
        ogs_pkbuf_free(pkbuf);
        dsmf_event_free(e);
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
    dsmf_event_free(e);
}

int dsmf_pfcp_open(void)
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

    OGS_SETUP_PFCP_SERVER;

    return OGS_OK;
}

void dsmf_pfcp_close(void)
{
    ogs_pfcp_node_t *pfcp_node = NULL;

    ogs_list_for_each(&ogs_pfcp_self()->pfcp_peer_list, pfcp_node)
        pfcp_node_fsm_fini(pfcp_node);

    ogs_freeaddrinfo(ogs_pfcp_self()->pfcp_advertise);
    ogs_freeaddrinfo(ogs_pfcp_self()->pfcp_advertise6);

    ogs_socknode_remove_all(&ogs_pfcp_self()->pfcp_list);
    ogs_socknode_remove_all(&ogs_pfcp_self()->pfcp_list6);
}

static void sess_timeout(ogs_pfcp_xact_t *xact, void *data)
{
    ogs_assert(xact);
    ogs_assert(data);

    ogs_warn("PFCP session transaction timeout");
}

int dsmf_pfcp_send_session_establishment_request(
    ogs_pfcp_node_t *df_node,
    const char *gnb_id,
    const char *session_id,
    const char *ran_addr_str,
    uint16_t ran_port,
    uint32_t teid)
{
    dsmf_sess_t *sess = NULL;
    ogs_pfcp_f_seid_t cp_f_seid;
    ogs_pfcp_xact_t *xact = NULL;
    ogs_pkbuf_t *dn4buf = NULL;
    int rv;

    ogs_assert(df_node);
    ogs_assert(gnb_id);
    ogs_assert(session_id);
    ogs_assert(ran_addr_str);

    /* 创建会话 */
    cp_f_seid.seid = 0; /* 将在 dsmf_sess_add 中生成 */
    cp_f_seid.ipv4 = 0;
    cp_f_seid.ipv6 = 0;
    if (ogs_pfcp_self()->pfcp_addr) {
        cp_f_seid.ipv4 = ogs_pfcp_self()->pfcp_addr->sin.sin_addr.s_addr;
    }

    sess = dsmf_sess_add(&cp_f_seid);
    ogs_assert(sess);

    sess->pfcp_node = df_node;
    sess->gnb_id = ogs_strdup(gnb_id);
    sess->session_id = ogs_strdup(session_id);

    /* 解析 RAN 地址 */
    sess->ran_addr = ogs_calloc(1, sizeof(ogs_sockaddr_t));
    ogs_assert(sess->ran_addr);

    if (ogs_getaddrinfo(&sess->ran_addr, AF_INET, ran_addr_str, ran_port, 0) != OGS_OK) {
        ogs_error("[DSMF] Failed to parse RAN address: %s", ran_addr_str);
        dsmf_sess_remove(sess);
        return OGS_ERROR;
    }

    if (sess->ran_addr->ogs_sa_family == AF_INET) {
        sess->ran_addr->sin.sin_port = htons(ran_port);
    } else if (sess->ran_addr->ogs_sa_family == AF_INET6) {
        sess->ran_addr->sin6.sin6_port = htons(ran_port);
    }

    sess->ran_teid = teid;

    /* 构建 PFCP 会话建立请求 */
    dn4buf = dsmf_dn4_build_session_establishment_request(sess);
    if (!dn4buf) {
        ogs_error("[DSMF] Failed to build session establishment request");
        dsmf_sess_remove(sess);
        return OGS_ERROR;
    }

    /* 发送 PFCP 消息 */
    xact = ogs_pfcp_xact_local_create(df_node, sess_timeout, sess);
    if (!xact) {
        ogs_error("[DSMF] Failed to create PFCP transaction");
        ogs_pkbuf_free(dn4buf);
        dsmf_sess_remove(sess);
        return OGS_ERROR;
    }

    rv = ogs_pfcp_xact_commit(xact);
    if (rv != OGS_OK) {
        ogs_error("[DSMF] Failed to send session establishment request");
        ogs_pfcp_xact_delete(xact);
        dsmf_sess_remove(sess);
        return rv;
    }

    ogs_info("[DSMF] Sent session establishment request to DF: %s (session=%s, addr=%s:%d, teid=0x%x)", 
             gnb_id, session_id, ran_addr_str, ran_port, teid);

    return OGS_OK;
}
