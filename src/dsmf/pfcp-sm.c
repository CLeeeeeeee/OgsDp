/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dsmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: pfcp-sm.c
 * 组件: dsmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:09 CST
 */

#include "sbi-path.h"
#include "pfcp-path.h"
#include "dn4-handler.h"
#include "timer.h"
#include "dsmf-sm.h"
#include "context.h"

static void pfcp_restoration(ogs_pfcp_node_t *node) __attribute__((unused));
static void reselect_df(ogs_pfcp_node_t *node) __attribute__((unused));
static void node_timeout(ogs_pfcp_xact_t *xact, void *data);

void dsmf_pfcp_state_initial(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sm_debug(e);
    ogs_assert(s);
    OGS_FSM_TRAN(s, &dsmf_pfcp_state_will_associate);
}

void dsmf_pfcp_state_final(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sm_debug(e);
    ogs_assert(s);
}

void dsmf_pfcp_state_will_associate(ogs_fsm_t *s, dsmf_event_t *e)
{
    ogs_pfcp_node_t *node = NULL;
    ogs_pfcp_xact_t *xact = NULL;
    dsmf_sm_debug(e);
    ogs_assert(s);

    node = e->pfcp_node;
    ogs_assert(node);

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("[DSMF] PFCP will associate with DF: %s", ogs_sockaddr_to_string_static(node->addr_list));
        /* 主动发起 PFCP Association Setup 请求，确保尽快建立关联（带回调） */
        ogs_pfcp_cp_send_association_setup_request(node, node_timeout);
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case DSMF_EVT_DN4_MESSAGE:
        if (e->pfcp_message && e->pfcp_message->h.type == OGS_PFCP_ASSOCIATION_SETUP_REQUEST_TYPE) {
            /* 处理关联建立请求 */
            xact = ogs_pfcp_xact_find_by_id(e->pfcp_xact_id);
            if (xact) {
                ogs_pfcp_cp_handle_association_setup_request(node, xact, &e->pfcp_message->pfcp_association_setup_request);
            }
        } else if (e->pfcp_message && e->pfcp_message->h.type == OGS_PFCP_ASSOCIATION_SETUP_RESPONSE_TYPE) {
            OGS_FSM_TRAN(s, &dsmf_pfcp_state_associated);
        } else if (e->pfcp_message && e->pfcp_message->h.type == OGS_PFCP_HEARTBEAT_REQUEST_TYPE) {
            /* 处理心跳请求 */
            xact = ogs_pfcp_xact_find_by_id(e->pfcp_xact_id);
            if (xact) {
                ogs_pfcp_handle_heartbeat_request(node, xact, &e->pfcp_message->pfcp_heartbeat_request);
            }
        } else if (e->pfcp_message && e->pfcp_message->h.type == OGS_PFCP_HEARTBEAT_RESPONSE_TYPE) {
            /* 处理心跳响应 */
            xact = ogs_pfcp_xact_find_by_id(e->pfcp_xact_id);
            if (xact) {
                ogs_pfcp_handle_heartbeat_response(node, xact, &e->pfcp_message->pfcp_heartbeat_response);
            }
        } else if (e->pfcp_message && e->pfcp_message->h.type == OGS_PFCP_VERSION_NOT_SUPPORTED_RESPONSE_TYPE) {
            /* 若 DF 返回版本不支持，暂时保持在 will_associate，稍后可降级/兼容 */
            ogs_warn("[DSMF] Received PFCP Version Not Supported Response");
        } else {
            dsmf_dn4_handle_message(e->pfcp_xact_id, e);
        }
        break;

    case DSMF_EVT_DN4_TIMER:
        if (e->h.timer_id == DSMF_TIMER_PFCP_ASSOCIATION) {
            ogs_warn("PFCP association timeout");
            /* 重试关联 */
            if (node->t_association) {
                ogs_timer_start(node->t_association, ogs_time_from_sec(10));
            }
        }
        break;

    default:
        ogs_error("Unknown event %s", dsmf_event_get_name(e));
        break;
    }
}

void dsmf_pfcp_state_associated(ogs_fsm_t *s, dsmf_event_t *e)
{
    ogs_pfcp_node_t *node = NULL;
    ogs_pfcp_xact_t *xact = NULL;
    dsmf_sm_debug(e);
    ogs_assert(s);

    node = e->pfcp_node;
    ogs_assert(node);

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("[DSMF] PFCP associated with DF: %s", ogs_sockaddr_to_string_static(node->addr_list));
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case DSMF_EVT_DN4_MESSAGE:
        if (e->pfcp_message && e->pfcp_message->h.type == OGS_PFCP_HEARTBEAT_REQUEST_TYPE) {
            /* 处理心跳请求 */
            xact = ogs_pfcp_xact_find_by_id(e->pfcp_xact_id);
            if (xact) {
                ogs_pfcp_handle_heartbeat_request(node, xact, &e->pfcp_message->pfcp_heartbeat_request);
            }
        } else if (e->pfcp_message && e->pfcp_message->h.type == OGS_PFCP_HEARTBEAT_RESPONSE_TYPE) {
            /* 处理心跳响应 */
            xact = ogs_pfcp_xact_find_by_id(e->pfcp_xact_id);
            if (xact) {
                ogs_pfcp_handle_heartbeat_response(node, xact, &e->pfcp_message->pfcp_heartbeat_response);
            }
        } else {
            dsmf_dn4_handle_message(e->pfcp_xact_id, e);
        }
        break;

    case DSMF_EVT_DN4_TIMER:
        if (e->h.timer_id == DSMF_TIMER_PFCP_NO_HEARTBEAT) {
            ogs_warn("PFCP no heartbeat");
            /* 发送心跳请求 */
            ogs_pfcp_send_heartbeat_request(node, node_timeout);
        }
        break;

    default:
        ogs_error("Unknown event %s", dsmf_event_get_name(e));
        break;
    }
}

void dsmf_pfcp_state_exception(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sm_debug(e);
    ogs_assert(s);
}

static void pfcp_restoration(ogs_pfcp_node_t *node)
{
    dsmf_sess_t *sess = NULL;

    ogs_assert(node);

    ogs_list_for_each(&dsmf_self()->sess_list, sess) {
        if (sess->pfcp_node == node) {
            ogs_fsm_dispatch(&sess->sm, NULL);
        }
    }
}

static void reselect_df(ogs_pfcp_node_t *node)
{
    dsmf_sess_t *sess = NULL;

    ogs_assert(node);

    ogs_list_for_each(&dsmf_self()->sess_list, sess) {
        if (sess->pfcp_node == node) {
            /* TODO: 实现 DF 重选逻辑 */
            ogs_warn("DF reselection not implemented yet");
        }
    }
}

static void node_timeout(ogs_pfcp_xact_t *xact, void *data)
{
    ogs_assert(xact);
    ogs_assert(data);

    ogs_warn("PFCP transaction timeout");
}


