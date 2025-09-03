/*
 * U - 自定义组件文件
 * 此文件是用户添加的自定义组件 dnf 的一部分
 * 不是原始 Open5GS 代码库的一部分
 * 
 * 文件: dnf-sm.c
 * 组件: dnf
 * 添加时间: 2025年 08月 25日 星期一 10:16:14 CST
 */

#include "context.h"
#include "dnf-sm.h"

void dnf_state_initial(ogs_fsm_t *s, dnf_event_t *e)
{
    ogs_assert(s);
    ogs_assert(e);

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("DNF FSM: initial state entered");
        break;
    case OGS_FSM_EXIT_SIG:
        ogs_info("DNF FSM: initial state exited");
        break;
    case DNF_EVT_NRF_REGISTER:
        /* 注册到 NRF */
        if (dnf_nrf_register() == OGS_OK) {
            ogs_info("DNF FSM: NRF registration successful");
            OGS_FSM_TRAN(s, &dnf_state_operational);
        } else {
            ogs_error("DNF FSM: NRF registration failed");
            /* 可以添加重试逻辑 */
        }
        break;
    case OGS_EVENT_SBI_TIMER: {
        ogs_sbi_nf_instance_t *nrf_instance = ogs_sbi_self()->nrf_instance;
        if (nrf_instance)
            ogs_fsm_dispatch(&nrf_instance->sm, (ogs_event_t *)e);
        break;
    }
    case OGS_EVENT_SBI_CLIENT:
        /* 交由通用 SBI 层处理；DNF 不拦截客户端响应 */
        break;
    default:
        ogs_trace("DNF FSM: ignore event %d", e->h.id);
        break;
    }
}

void dnf_state_final(ogs_fsm_t *s, dnf_event_t *e)
{
    ogs_assert(s);
    ogs_assert(e);

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("DNF FSM: final state entered");
        break;
    case OGS_FSM_EXIT_SIG:
        ogs_info("DNF FSM: final state exited");
        break;
    case OGS_EVENT_SBI_CLIENT:
        /* 不处理客户端响应，避免误报 unknown event */
        break;
    default:
        ogs_trace("DNF FSM: ignore event %d", e->h.id);
        break;
    }
}

void dnf_state_operational(ogs_fsm_t *s, dnf_event_t *e)
{
    dnf_sess_t *sess = NULL;

    ogs_assert(s);
    ogs_assert(e);

    switch (e->h.id) {
    case OGS_FSM_ENTRY_SIG:
        ogs_info("DNF FSM: operational state entered");
        break;
    case OGS_FSM_EXIT_SIG:
        ogs_info("DNF FSM: operational state exited");
        break;
    case DNF_EVT_NRF_DEREGISTER:
        /* 从 NRF 注销 */
        dnf_nrf_deregister();
        OGS_FSM_TRAN(s, &dnf_state_final);
        break;
    case DNF_EVT_SESSION_CREATE:
        /* 创建会话 */
        sess = dnf_sess_add(e->session_create.teid, e->session_create.ran_addr);
        if (sess) {
            ogs_info("DNF FSM: session created for TEID[0x%x]", e->session_create.teid);
        } else {
            ogs_error("DNF FSM: failed to create session for TEID[0x%x]", e->session_create.teid);
        }
        break;
    case DNF_EVT_SESSION_REMOVE:
        /* 移除会话 */
        sess = dnf_sess_find_by_teid(e->session_remove.teid);
        if (sess) {
            dnf_sess_remove(sess);
            ogs_info("DNF FSM: session removed for TEID[0x%x]", e->session_remove.teid);
        } else {
            ogs_warn("DNF FSM: session not found for TEID[0x%x]", e->session_remove.teid);
        }
        break;
    case DNF_EVT_DATA_FORWARD:
        /* 数据转发 */
        if (e->data_forward.sess && e->data_forward.pkbuf) {
            if (dnf_forward_data(e->data_forward.sess, e->data_forward.pkbuf) == OGS_OK) {
                ogs_trace("DNF FSM: data forwarded successfully");
            } else {
                ogs_error("DNF FSM: data forwarding failed");
            }
        } else {
            ogs_error("DNF FSM: invalid data forward event");
        }
        break;
    case OGS_EVENT_SBI_TIMER: {
        ogs_sbi_nf_instance_t *nrf_instance = ogs_sbi_self()->nrf_instance;
        if (nrf_instance)
            ogs_fsm_dispatch(&nrf_instance->sm, (ogs_event_t *)e);
        break;
    }
    default:
        ogs_error("DNF FSM: unknown event %d", e->h.id);
        break;
    }
}
