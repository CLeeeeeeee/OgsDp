/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dsmf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: dsmf-sm.c
 * 组件: dsmf
 * 添加时间: 2025年 08月 20日 星期三 11:16:11 CST
 */

#include "context.h"
#include "pfcp-path.h"
#include "sbi-path.h"
#include "dn4-handler.h"
#include "dsmf-sm.h"
#include "timer.h"

/* 主状态机实现 */
void dsmf_state_initial(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sm_debug(e);
    ogs_assert(s);
    OGS_FSM_TRAN(s, &dsmf_state_operational);
}

void dsmf_state_final(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sm_debug(e);
    ogs_assert(s);
}

void dsmf_state_operational(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sess_t *sess = NULL;
    dsmf_sm_debug(e);
    ogs_assert(s);

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case DSMF_EVT_DN4_MESSAGE:
        sess = dsmf_sess_find_by_id(e->sess_id);
        if (sess) {
            ogs_fsm_dispatch(&sess->sm, e);
        } else {
            ogs_warn("Session not found for DN4 message");
        }
        break;

    case DSMF_EVT_DN4_TIMER:
        sess = dsmf_sess_find_by_id(e->sess_id);
        if (sess) {
            ogs_fsm_dispatch(&sess->sm, e);
        } else {
            ogs_warn("Session not found for DN4 timer");
        }
        break;

    case DSMF_EVT_SBI_MESSAGE:
        sess = dsmf_sess_find_by_id(e->sess_id);
        if (sess) {
            ogs_fsm_dispatch(&sess->sm, e);
        } else {
            ogs_warn("Session not found for SBI message");
        }
        break;

    case DSMF_EVT_SBI_TIMER:
        sess = dsmf_sess_find_by_id(e->sess_id);
        if (sess) {
            ogs_fsm_dispatch(&sess->sm, e);
        } else {
            ogs_warn("Session not found for SBI timer");
        }
        break;

    case DSMF_EVT_RAN_SYNC_REQUEST:
        /* 为该 gNB+session 创建/查找会话并转交到会话状态机 */
        {
            dsmf_sess_t *new_sess = NULL;
            new_sess = dsmf_sess_add_by_ran_sync(
                e->ran_sync.gnb_id,
                e->ran_sync.session_id,
                e->ran_sync.ran_addr_str,
                e->ran_sync.ran_port,
                e->ran_sync.teid);
            if (!new_sess) {
                ogs_error("Failed to create DSMF session for gnb=%s session=%s",
                          e->ran_sync.gnb_id, e->ran_sync.session_id);
                break;
            }
            /* 若 PFCP 节点未准备，尽量绑定一个 DF 节点 */
            if (!new_sess->pfcp_node) {
                ogs_pfcp_node_t *df_node = NULL;
                ogs_list_for_each(&ogs_pfcp_self()->pfcp_peer_list, df_node) {
                    new_sess->pfcp_node = df_node;
                    break;
                }
            }
            e->sess_id = new_sess->id;
            ogs_info("[DSMF] Dispatch RAN sync to session [%s]",
                     new_sess->session_ref);
            ogs_fsm_dispatch(&new_sess->sm, e);
        }
        break;

    case OGS_EVENT_SBI_CLIENT:
        {
            ogs_sbi_message_t client_msg;
            int crv;
            memset(&client_msg, 0, sizeof(client_msg));
            ogs_assert(e->h.sbi.response);
            crv = ogs_sbi_parse_response(&client_msg, e->h.sbi.response);
            if (crv != OGS_OK) {
                ogs_error("cannot parse HTTP response");
                break;
            }
            if (!strcmp(client_msg.h.service.name, OGS_SBI_SERVICE_NAME_NNRF_NFM) &&
                !strcmp(client_msg.h.resource.component[0], OGS_SBI_RESOURCE_NAME_NF_INSTANCES)) {
                ogs_sbi_nf_instance_t *nrf_instance = ogs_sbi_self()->nrf_instance;
                ogs_assert(nrf_instance);
                e->h.sbi.message = &client_msg;
                ogs_fsm_dispatch(&nrf_instance->sm, (ogs_event_t *)e);
            }
            ogs_sbi_message_free(&client_msg);
            break;
        }

    case OGS_EVENT_SBI_TIMER:
        {
            ogs_sbi_nf_instance_t *nrf_instance = ogs_sbi_self()->nrf_instance;
            if (nrf_instance)
                ogs_fsm_dispatch(&nrf_instance->sm, (ogs_event_t *)e);
            break;
        }

    default:
        ogs_error("Unknown event %s", dsmf_event_get_name(e));
        break;
    }
}

void dsmf_state_exception(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sm_debug(e);
    ogs_assert(s);
}

/* 会话状态机实现 */
void dsmf_session_state_initial(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sess_t *sess = NULL;
    dsmf_sm_debug(e);
    ogs_assert(s);

    sess = dsmf_sess_find_by_id(e->sess_id);
    ogs_assert(sess);

    OGS_FSM_TRAN(s, &dsmf_session_state_wait_ran_sync);
}

void dsmf_session_state_final(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sess_t *sess = NULL;
    dsmf_sm_debug(e);
    ogs_assert(s);

    sess = dsmf_sess_find_by_id(e->sess_id);
    ogs_assert(sess);
}

void dsmf_session_state_wait_ran_sync(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sess_t *sess = NULL;
    dsmf_sm_debug(e);
    ogs_assert(s);

    sess = dsmf_sess_find_by_id(e->sess_id);
    ogs_assert(sess);

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("Session [%s] waiting for RAN sync", sess->session_ref);
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case DSMF_EVT_RAN_SYNC_REQUEST:
        ogs_info("[DSMF] SM got RAN addr: gnb=%s session=%s addr=%s:%d teid=0x%x",
             e->ran_sync.gnb_id, e->ran_sync.session_id,
             e->ran_sync.ran_addr_str, e->ran_sync.ran_port, e->ran_sync.teid);
        /* 切换到等待 PFCP 建立状态，由该状态的 ENTRY 中统一发起 SEReq */
        OGS_FSM_TRAN(s, &dsmf_session_state_wait_pfcp_establishment);
        break;

    case DSMF_EVT_SESSION_DELETION_REQUEST:
        /* 直接删除会话 */
        dsmf_sess_remove(sess);
        OGS_FSM_TRAN(s, &dsmf_session_state_final);
        break;

    default:
        ogs_error("Unknown event %s", dsmf_event_get_name(e));
        break;
    }
}

void dsmf_session_state_wait_pfcp_establishment(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sess_t *sess = NULL;
    dsmf_sm_debug(e);
    ogs_assert(s);

    sess = dsmf_sess_find_by_id(e->sess_id);
    ogs_assert(sess);

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("Session [%s] waiting for PFCP establishment", sess->session_ref);
        /* 启动 PFCP 建立定时器 */
        sess->t_establishment = ogs_timer_add(ogs_app()->timer_mgr,
                dsmf_timer_session_establishment, &sess->id);
        ogs_assert(sess->t_establishment);
        ogs_timer_start(sess->t_establishment, ogs_time_from_sec(10));
        
        /* 发送 PFCP 会话建立请求 */
        dsmf_dn4_handle_session_establishment(sess);
        break;

    case OGS_FSM_EXIT_SIG:
        if (sess->t_establishment) {
            ogs_timer_delete(sess->t_establishment);
            sess->t_establishment = NULL;
        }
        break;

    case DSMF_EVT_DN4_MESSAGE:
        if (e->pfcp_message && e->pfcp_message->h.type == OGS_PFCP_SESSION_ESTABLISHMENT_RESPONSE_TYPE) {
            /* PFCP 会话建立成功 */
            sess->pfcp_established = true;
            OGS_FSM_TRAN(s, &dsmf_session_state_operational);
        }
        break;

    case DSMF_EVT_SBI_TIMER:
        if (e->h.timer_id == DSMF_TIMER_SESSION_ESTABLISHMENT) {
            ogs_warn("Session [%s] PFCP establishment timeout", sess->session_ref);
            OGS_FSM_TRAN(s, &dsmf_session_state_exception);
        }
        break;

    case DSMF_EVT_SESSION_DELETION_REQUEST:
        /* 取消建立，直接删除会话 */
        dsmf_sess_remove(sess);
        OGS_FSM_TRAN(s, &dsmf_session_state_final);
        break;

    default:
        ogs_error("Unknown event %s", dsmf_event_get_name(e));
        break;
    }
}

void dsmf_session_state_operational(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sess_t *sess = NULL;
    dsmf_sm_debug(e);
    ogs_assert(s);

    sess = dsmf_sess_find_by_id(e->sess_id);
    ogs_assert(sess);

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("Session [%s] operational", sess->session_ref);
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case DSMF_EVT_DN4_MESSAGE:
        /* 处理 PFCP 消息 */
        dsmf_dn4_handle_message(e->pfcp_xact_id, e);
        break;

    case DSMF_EVT_SESSION_MODIFICATION_REQUEST:
        /* 处理会话修改请求 */
        dsmf_dn4_handle_session_modification(sess);
        break;

    case DSMF_EVT_SESSION_DELETION_REQUEST:
        /* 启动会话删除流程 */
        OGS_FSM_TRAN(s, &dsmf_session_state_wait_pfcp_deletion);
        break;

    default:
        ogs_error("Unknown event %s", dsmf_event_get_name(e));
        break;
    }
}

void dsmf_session_state_wait_pfcp_deletion(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sess_t *sess = NULL;
    dsmf_sm_debug(e);
    ogs_assert(s);

    sess = dsmf_sess_find_by_id(e->sess_id);
    ogs_assert(sess);

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("Session [%s] waiting for PFCP deletion", sess->session_ref);
        /* 启动 PFCP 删除定时器 */
        sess->t_deletion = ogs_timer_add(ogs_app()->timer_mgr,
                dsmf_timer_session_deletion, &sess->id);
        ogs_assert(sess->t_deletion);
        ogs_timer_start(sess->t_deletion, ogs_time_from_sec(10));
        
        /* 发送 PFCP 会话删除请求 */
        dsmf_dn4_handle_session_deletion(sess);
        break;

    case OGS_FSM_EXIT_SIG:
        if (sess->t_deletion) {
            ogs_timer_delete(sess->t_deletion);
            sess->t_deletion = NULL;
        }
        break;

    case DSMF_EVT_DN4_MESSAGE:
        if (e->pfcp_message && e->pfcp_message->h.type == OGS_PFCP_SESSION_DELETION_RESPONSE_TYPE) {
            /* PFCP 会话删除成功 */
            dsmf_sess_remove(sess);
            OGS_FSM_TRAN(s, &dsmf_session_state_final);
        }
        break;

    case DSMF_EVT_SBI_TIMER:
        if (e->h.timer_id == DSMF_TIMER_SESSION_DELETION) {
            ogs_warn("Session [%s] PFCP deletion timeout", sess->session_ref);
            /* 强制删除会话 */
            dsmf_sess_remove(sess);
            OGS_FSM_TRAN(s, &dsmf_session_state_final);
        }
        break;

    default:
        ogs_error("Unknown event %s", dsmf_event_get_name(e));
        break;
    }
}

void dsmf_session_state_exception(ogs_fsm_t *s, dsmf_event_t *e)
{
    dsmf_sess_t *sess = NULL;
    dsmf_sm_debug(e);
    ogs_assert(s);

    sess = dsmf_sess_find_by_id(e->sess_id);
    ogs_assert(sess);

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_warn("Session [%s] in exception state", sess->session_ref);
        break;

    case OGS_FSM_EXIT_SIG:
        break;

    case DSMF_EVT_SESSION_DELETION_REQUEST:
        /* 异常状态下直接删除会话 */
        dsmf_sess_remove(sess);
        OGS_FSM_TRAN(s, &dsmf_session_state_final);
        break;

    default:
        ogs_error("Unknown event %s", dsmf_event_get_name(e));
        break;
    }
} 